/******************  MODULE NAME:  MEASURE.C   ***************************/
/**       DATE: 1/1/94                                                  **/
/**          BY:    NLW, SLF; ERG                                       **/
/** DESCRIPTION:    Functions for computing the result of retrofit      **/
/**                                                                     **/
/** Note: A number of these retrofits must be evaluated after a base    **/
/** case run so that selected results of UA calcs are available         **/
/**                                                                     **/
/** REVISIONS:  10/18/94 SLF Allowed for no CLG file.                   **/
/**             11/16/94 SLF Reordered retrofits for Replacement Htg.   **/
/**             12/29/98 MJF Extensive re-write for mdi-> usage and     **/
/**                          removal of GetData and PutData calls       **/
/**             4/2000   MJF Major change in the structure of these     **/
/**                          functions. 1) All follow the same proto    **/
/**                          to support the retroFunc[] simplification  **/
/**                          and all write to the same results structure**/
/**                          indexed by the retorfit index              **/
/**                          Also, combined the measures in measure1.c  **/
/**                          and measure2.c into this single module     **/
/**                                                                     **/
/*************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "wa_engine.h"

static float ceiling_duct_volume(void);
static float floor_duct_volume(void);
static void initialize_fuel_types();
static void save_energy_results();
static void no_heating_or_cooling_savings();
static void additional_cost(float cost);
static void append_component_code_to_current_results(char *code);

void copy_mdi(MDI **dest, MDI *src) {
  // int retval;

  ASSERT(src, sprintf(msg, "No MDI to copy"));

  if (*dest)
    free(*dest); // free up any existing data in dest

  ASSERT((*dest = (MDI *)calloc(1, sizeof(MDI))), sprintf(msg, "Out of memory in copy_mdi")); // start afresh

  memcpy(*dest, src, sizeof(MDI)); // the basic structure, eg. counts

}

// Special note on retrofit functions.  They should all have the
// same prototype.  They should use the defined constants in def.h
// for measures, materials, and defined key parameters. They should
// reset the mir->flgRetrofits[ndx] to zero on entry and set it only if the
// retrofit is actually applied.  They should directly effect the mdi
// structure and fill in the first set of fields in the mor[ndx]
// structure.

// Also note:  If the routine does NOT compute a BCR in the GLBResult[ndx]
// struture, then a subsequent call to mhea_energy_use and GetBCR in retrofit.c
// will do the calculation.  This is useful for baseload measures which
// may not have any programmed interaction with the heating or cooling
// load calculations.

/******************* FUNCTION NAME: retro_replace_heating *****************/
/**        DATE:    December 1993                                       **/
/**          BY:    NW, SLF                                             **/
/** DESCRIPTION:    Called by the ...Retrofits() functions.             **/
/**                 Replace gas and oil burning systems.                **/
/** REVISIONS:      11/04/94 SLF - Added Kerosene heating.              **/
/**                 11/16/94 SLF - Modified for Replacment Heating.     **/
/**                 12/29/98 MJF - new prototype and globals            **/
/**                 9/8/01 MJF Rewrite for new normalized prototype     **/
/**                        and mutually exclusive with Furnace Tuneup   **/
/**                 10/8/03 MBG modified to allow fuel switching for    **/
/**                    heating system replacement                       **/
/**                 10/6/08 MBG modified to have tuneup required take  **/
/**                    precedence over replacement required            **/
/*************************************************************************/
void retro_replace_heating(void) {
  int ndx = M_CMS_REPLACE_HEATING_SYSTEM;
  int iMat;

  mir->flgRetrofits[ndx] = FALSE;

  /******************************
  If replacing the furnace is required, copy the replacement
  furnace specifications to the primary furnace data.  Only
  the primary furnace will be considered for replacement.
  Skip the whole retrofit if replacement equipment data is missing
  or if tuneup measure declared required.
  ******************************/

  if (mir->flgRetrofits[M_CMS_TUNE_HEATING_SYSTEM] || 
    mdi->htg.tuneup == YES ||
    mdi->htr.equip_type == ET_NONE ||
    (mdi->htr.efficiency_percent == 0.0 && mdi->htr.efficiency_hspf == 0.0 && mdi->htr.efficiency_cop == 0.0))
    return;

  mdi->htg.equip_type = mdi->htr.equip_type;
  mdi->htg.capacity = mdi->htr.capacity;
  mdi->htg.duct_location = mdi->htr.duct_location;
  mdi->htg.duct_insl = mdi->htr.duct_insl;
  mdi->htg.efficiency_percent = mdi->htr.efficiency_percent;
  mdi->htg.efficiency_hspf = mdi->htr.efficiency_hspf;
  mdi->htg.efficiency_cop = mdi->htr.efficiency_cop;
  mdi->htg.eff_units = mdi->htr.eff_units;
  //  mdi->htg.fuel_type         = mdi->htr.fuel_type;        // yes, fuel switch possible

  // note that this replaces just the primary heating system without
  // changing the percent of heating load supplied by that system
  // if the replacement is intended as a replacement for both the
  // primary and secondary system, then we need to reset the .ht2
  // structure (blank it) and set the percent of load served to 100
  // for the .htg structure

  // If measure chosen from retrofit options combo box,
  // make it required.

  if (mdi->htr.replacement == YES) {
    mir->Results[mir->Rndx].measure_required = TRUE;
    if (mdi->htr.incl_costs == YES)
      mir->Results[mir->Rndx].measure_priority = MPS_EQUIP_REQUIRED;
    else
      mir->Results[mir->Rndx].measure_priority = MPS_EQUIP_REQUIRED_NO_SIR;
  }

  switch (mdi->htr.fuel_type) {
  case ELECTRIC:
    iMat = M_MAT_REPLACE_ELEC_FURNACE;
    mir->Results[mir->Rndx].material_id = M_MAT_REPLACE_ELEC_FURNACE;
    break;
  case NATURAL_GAS:
    iMat = M_MAT_REPLACE_GAS_FURNACE;
    break;
  case OIL:
    iMat = M_MAT_REPLACE_OIL_KERO_FURNACE;
    break;
  case PROPANE:
    iMat = M_MAT_REPLACE_PROPANE_FURNACE;
    break;
  default:
    iMat = M_MAT_REPLACE_OIL_KERO_FURNACE;
    break;
  }

  // we added the ability to put the material and labor cost
  // directly in the htr structure in 8.3.3 MJF 9/07

  // so, here we put our specific costs back into the rmc
  // structure so the subsequent material cost lookups in the
  // GetBCR function work as expected

  mdi->rmc[iMat].material = mdi->htr.material_cost;
  mdi->rmc[iMat].labor = mdi->htr.labor_cost;
  mdi->rmc[iMat].extra = 0.0;

  mir->flgRetrofits[ndx] = TRUE;
  STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
  STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

  // Here is where we do the fuel switch possibly

  initialize_fuel_types();
  mir->Results[mir->Rndx].PstHtgFuel = mdi->htr.fuel_type;
  mir->Results[mir->Rndx].fQuant = 1;
  mir->Results[mir->Rndx].measure_id = ndx;
  mir->Results[mir->Rndx].audit_section_id = M_HEATING;
  mir->Results[mir->Rndx].material_id = iMat;

  mir->Rndx++;

  return;
}

/******************* FUNCTION NAME: retro_seal_ducts  *********************/
/**         DATE:  December 1993                                        **/
/**           BY:  NW, SLF                                                      **/
/**  DESCRIPTION:  Called by the ...Retrofits() functions.                  **/
/** Reduces duct loss factor.                                                       **/
/*************************************************************************/
void retro_seal_ducts(void) {
  int HTGDuct = 0; // assume there is no duct work
  int CLGDuct = 0; 
  // int CL2Duct = 0;

  int ndx  = M_CMS_SEAL_DUCTS;
  int iMat = M_MAT_SEAL_DUCTS;

  mir->flgRetrofits[ndx] = FALSE;

  // lets see if there is duct work for any heating or
  // cooling system in the home

  if (mdi->htg.duct_location == DL_FLOOR || mdi->htg.duct_location == DL_CEILING)
    HTGDuct = 1;

  if (mdi->clg.equip_type != 0 && mdi->clg.equip_type != CE_NONE) // we have a primary cooling system
  {
    if (mdi->clg.duct_location == DL_FLOOR || // with a duct
        mdi->clg.duct_location == DL_CEILING)
      CLGDuct = 1;
  }

  // there is never any secondary system duct because we
  // hid those controls starting with version 7.2.4  MJF 8/02  MJF #70

  // if (mdi->cl2.equip_type != 0 && // a secondary system
  //     mdi->cl2.equip_type != CE_NONE) {
  //   if (mdi->cl2.clg_duct_location == DL_FLOOR || // with a duct
  //       mdi->cl2.clg_duct_location == DL_CEILING)
  //     CL2Duct = 1;
  // }

  /************************
  If retrofit is applied, reduce duct loss factor by 25% in the
  mhea_infiltration_losses(), HtgConsmptn(), and ClgConsmptn() functions.
  This measure will always be applied if there are ducts.
  *************************/

  // Added condition that the duct sealing cost be non-zero in the
  // input data.  If so, then the measure is reported regardless of
  // SIR -- this matches NEAT  (MJF 3/18/03)

  //if ((HTGDuct || CLGDuct || CL2Duct) && mdi->inf.evaluate_duct_sealing == YES && mdi->inf.duct_seal_cost > 0.0) {     MJF #70
  
  if ((HTGDuct || CLGDuct) && mdi->inf.evaluate_duct_sealing == YES && mdi->inf.duct_seal_cost > 0.0) {
    mir->flgRetrofits[ndx] = TRUE;
    STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
    STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

    initialize_fuel_types();
    mir->Results[mir->Rndx].fQuant = 1;
    mir->Results[mir->Rndx].fAddCost = mdi->inf.duct_seal_cost;

    mir->Results[mir->Rndx].costi2 = mdi->inf.duct_seal_cost;
    //mir->Results[mir->Rndx].typei2 = MT_DUCT_SEALING;
    mir->Results[mir->Rndx].typei2 = MT_MISCELLANEOUS_SUPPLIES;
    STRCPY(mir->Results[mir->Rndx].desci2, "Duct Sealing");

    // we want this measure to come after furnaces and BEFORE
    // general air sealing IF the BCR > min -- so no required flag

    mir->Results[mir->Rndx].measure_priority = MPS_DUCT_SEAL;
    mir->Results[mir->Rndx].measure_required = TRUE;

    mir->Results[mir->Rndx].measure_id = ndx;
    mir->Results[mir->Rndx].audit_section_id = M_DUCTS_AND_INFILTRATION;
    mir->Results[mir->Rndx].material_id = iMat;

    mir->Rndx++;
  }

  return;
}

/******************* FUNCTION NAME: retro_air_seal  ***********************/
/**         DATE:  December 1993                                        **/
/**           BY:  NW, SLF                                                      **/
/**  DESCRIPTION:  Called by the ...Retrofits() functions.                  **/
/** Formerly retroInsulate_Ducts.                                                   **/
/** Reduces infiltration cfm.                                                       **/
/*************************************************************************/
void retro_air_seal(void) {
  
  static int added_message = FALSE;

  int ndx =  M_CMS_GENERAL_AIR_SEALING;
  int iMat = M_MAT_GENERAL_AIR_SEALING;

  mir->flgRetrofits[ndx] = FALSE;

  mir->infiltration_treatment = infiltration_reduction_treatment(mdi->inf);

  // This measure is applied only if a non zero air leakage reduction
  // cost is entered.  If the cost is entered, then the measure will
  // always be reported regardless of SIR  (MJF 3/18/03)

  if (mdi->inf.air_leak_red_cost > 0.0) {
    mir->flgRetrofits[ndx] = TRUE;
    STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
    STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

    initialize_fuel_types();
    mir->Results[mir->Rndx].fQuant = 1;
    mir->Results[mir->Rndx].fAddCost = mdi->inf.air_leak_red_cost;

    mir->Results[mir->Rndx].costi2 = mdi->inf.air_leak_red_cost;
    //mir->Results[mir->Rndx].typei2 = MT_INFILTRATION_REDUCTION;
    mir->Results[mir->Rndx].typei2 = MT_MISCELLANEOUS_SUPPLIES;
    STRCPY(mir->Results[mir->Rndx].desci2, "Infiltration Reduction");

    mir->Results[mir->Rndx].measure_priority = MPS_INFILTRATION_REDUCTION;
    mir->Results[mir->Rndx].measure_required = TRUE;

    mir->Results[mir->Rndx].measure_id = ndx;
    mir->Results[mir->Rndx].audit_section_id = M_DUCTS_AND_INFILTRATION;
    mir->Results[mir->Rndx].material_id = iMat;

    if (mir->infiltration_treatment == INF_FULL_MEASURE && mir->flgWhichPass == CUMULATIVE && added_message == FALSE) {
      add_mhea_message("MHEA assumes that infiltration reduction will be performed in parallel to measures "
                       "selected by the audit and according to guidelines chosen by the auditor.  MHEA can "
                       "evaluate the cost-effectiveness of infiltration reduction efforts, but it will not direct the work.");
      add_mhea_message("The audit strongly suggests, but does not necessarily require, the use of existing "
                       "infiltration reduction procedures using a blower-door. The blower-door establishes if "
                       "infiltration reduction is necessary, then helps locate leaks and monitor progress in their elimination.");
      added_message = TRUE;
    }

    mir->Rndx++;
  }

  return;
}

/******************* FUNCTION NAME: retro_insulate_wall_batt **************/
/**         DATE:  December 1993                                        **/
/**           BY:  NLW              SLF                                 **/
/**  DESCRIPTION:  Called by the ...Retrofits() functions.              **/
/** The retroInsulate_Wall retrofit will be applied if airspace         **/
/** greater than 1 inches and no other wall retrofit applied.           **/
/*************************************************************************/
void retro_insulate_wall_batt(void) {
  int ndx =  M_CMS_WALL_FIBERGLASS_BATT_INSL;
  int iMat = M_MAT_WALL_FIBERGLASS_BATT_INSL;

  mir->flgRetrofits[ndx] = FALSE;

  /**********************************
  This retrofit adds 3.5" of batt insulation to the walls if other
  wall insul retrofits are not applied and the insulatable airspace is
  greater than 1 inch.  (SLF changed per NREL memo 7/12/94)
  ***********************************/

  if (mir->flgRetrofits[M_CMS_WALL_CELLULOSE_LOOSE_INSL] ||
    mir->flgRetrofits[M_CMS_WALL_FIBERGLASS_LOOSE_INSL] ||
    mir->fWalInsAirSpace < 1.0)
    return;

  // NW,9/11/95: First approximation of reduced effectiveness of wall insulation
  //      due to 33% uninsulatable spaces
  // Above eliminated 6/03, replaced by separate paths for insulatable and
  // uninsulatable areas in ua_wal.  MBG

  //      mdi->wal.batt_insl += 3.5 * 0.67;
  mdi->wal.batt_insl += 3.5;       // increase thickness (inches)
  mdi->wal.wall_vent = WV_NOTVENT; // wall is now treated as unvented

  /*************************
  NW, ERG 7/16/94 - Modify wall insulation quantify to account for
  reduced area due to framing and other obstructions.  Assume 0.85
  for framing area and 0.85 for general reduction factor (RF).
  See NREL notes p. 34.1.
  *************************/

  mir->flgRetrofits[ndx] = TRUE;
  STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
  STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

  initialize_fuel_types();
  //STRCPY(mir->Results[mir->Rndx].sName, "Wall Fiberglass Batt");
  ASSERT(mir->fNetWalArea > 10.0, sprintf(msg, "Assertion Failure"));

  /**********************************************************************
  Replace fixed 0.85 "general reduction factor" with actual fraction of
  uninsulatable area from ua_wal.c. This includes user-defined area,
  mdi->wal.uninsulatable_area.  MBG 6/03
  **********************************************************************/

  mir->Results[mir->Rndx].fQuant = (float)(mir->fNetWalArea * 0.85 * (1.0f - mir->fFractWallUnins));
  //      mir->Results[mir->Rndx].fQuant = mir->fNetWalArea * 0.85 * 0.85;
  //      mir->Results[mir->Rndx].fQuant -= mdi->wal.uninsulatable_area;

  additional_cost(mdi->wal.add_cost);

  mir->Results[mir->Rndx].measure_id = ndx;
  mir->Results[mir->Rndx].audit_section_id = M_WALL;
  mir->Results[mir->Rndx].material_id = iMat;

  mir->Rndx++;

  return;
}

/******************* FUNCTION NAME: retro_insulate_wall_batt_add***********/
/**         DATE:  December 1993                                        **/
/**           BY:  NLW              SLF                                 **/
/**  DESCRIPTION:  Called by the ...Retrofits() functions.              **/
/** The retroInsulate_Wall retrofit will be applied if airspace         **/
/** greater than 1 inches and no other wall retrofit applied.           **/
/*************************************************************************/
void retro_insulate_wall_batt_add(void) {
  int ndx =  M_CMS_WALL_FIBERGLASS_BATT_INSL_ADD;
  int iMat = M_MAT_WALL_FIBERGLASS_BATT_INSL_ADD;

  mir->flgRetrofits[ndx] = FALSE;

  /**********************************
  This retrofit adds 3.5" of batt insulation to the walls if other
  wall insul retrofits are not applied and the insulatable airspace is
  greater than 1 inch.  (SLF changed per NREL memo 7/12/94)
  ***********************************/

  // additions too, same logic, possible new instance record
  // of this type of measure.

  if (mir->flgRetrofits[M_CMS_WALL_CELLULOSE_LOOSE_INSL_ADD] || 
    mir->flgRetrofits[M_CMS_WALL_FIBERGLASS_LOOSE_INSL_ADD] ||
    mir->fAWLInsAirSpace < 1.0)
    return; 

  mdi->awl.batt_insl += (float)(3.5 * 0.67);
  mdi->awl.wall_vent = WV_NOTVENT;
  mir->flgRetrofits[ndx] = TRUE;
  STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
  STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

  initialize_fuel_types();
  STRCPY(mir->Results[mir->Rndx].sName, "Wall Fiberglass Batt [Addition]");
  ASSERT(mir->fNetAWLArea > 10.0, sprintf(msg, "Assertion Failure"));
  mir->Results[mir->Rndx].fQuant = (float)(mir->fNetAWLArea * 0.85 * 0.85);
  STRCPY(mir->Results[mir->Rndx].sMaterial, "Wall fiberglass batt insl [Addition]");

  additional_cost(mdi->awl.add_cost);

  mir->Results[mir->Rndx].measure_id = ndx;
  mir->Results[mir->Rndx].audit_section_id = M_WALL_ADDITION;
  mir->Results[mir->Rndx].material_id = iMat;

  mir->Rndx++;

  return;
}

/******************* FUNCTION NAME: retro_insulate_wall_cellulose *********/
/**         DATE:  December 1993                                        **/
/**           BY:  NLW              SLF                                 **/
/**  DESCRIPTION:  Called by the ...Retrofits() functions.              **/
/** The retroInsulate_Wall retrofit will be applied if airspace         **/
/** greater than 1 inch and no other wall retrofit applied.             **/
/** (Wall blown with loose cellulose.)                                  **/
/*************************************************************************/
void retro_insulate_wall_cellulose(void) {
  int ndx =  M_CMS_WALL_CELLULOSE_LOOSE_INSL;
  int iMat = M_MAT_WALL_CELLULOSE_LOOSE_INSL;

  float density = mdi->key.density_of_loose_cellulose_insulation;
  float bagsize = mdi->key.bag_size_for_loose_cellulose_insulation;
  float quant;
  float fAdjWallDepth = mir->fWalInsAirSpace; // Depth of added insulation
  // adjusted for compression of existing insulation.
  float fDepthChange = 0.0f; // Change in insulations depth due to compression;

  mir->flgRetrofits[ndx] = FALSE;

  /**********************************
  This retrofit fills the insulatable air space with loose cellulose if
  other wall retrofits are not applied and the insulatable airspace is
  greater than 1 inch.
  ***********************************/

  if (mir->flgRetrofits[M_CMS_WALL_FIBERGLASS_BATT_INSL] || 
    mir->flgRetrofits[M_CMS_WALL_FIBERGLASS_LOOSE_INSL] ||
    mir->fWalInsAirSpace < 1.0)
    return;

  // quantity is in units of bags.  we first calculate the
  // volume of the space minus a framing-area factor , then based on the
  // volume we get total weight using a in-situ density nuymber then
  // divide the total weight of insulation by the bag size to
  // get the number of bags.  We round up the final figure to
  // an integer number of bags (if more than 10% of a bag is used)

  /*********************************************************
  Compute the depth of insulation to be added accounting for
  compression of the existing insulation.
  *********************************************************/

  if (density > mir->fDensExistBatInsul) {
    fDepthChange = mir->fWallInsDepth * (1.0f - mir->fDensExistBatInsul / density);
    fAdjWallDepth = mir->fWalInsAirSpace + fDepthChange;
  }

  ASSERT(mir->fNetWalArea > 10.0, sprintf(msg, "Assertion Failure"));
  ASSERT(bagsize != 0, sprintf(msg, "Assertion Failure"));

  /**********************************************************************
  Replace fixed 0.85 "general reduction factor" with actual fraction of
  uninsulatable area from ua_wal.c. This includes user-defined area,
  mdi->wal.uninsulatable_area.  MBG 6/03
  **********************************************************************/

  //      quant = (mir->fNetWalArea * 0.85 * 0.85 - mdi->wal.uninsulatable_area ) *
  //              (mir->fWalInsAirSpace / 12.0) * density / bagsize;
  quant = (float)((mir->fNetWalArea * 0.85 * (1.0f - mir->fFractWallUnins)) * (fAdjWallDepth / 12.0) * density / bagsize);
  if ((quant - (int)(quant)) > 0.10)
    quant = (float)((int)(quant + 1.0F)); // yes, a double cast so the compiler will shut up
  else
    quant = (float)((int)(quant));

  // NW,9/11/95: First approximation of reduced effectiveness of wall insulation
  //      due to 33% uninsulatable spaces
  // Above eliminated 6/03, replaced by separate paths for insulatable and
  // uninsulatable areas in ua_wal.  MBG

  //      mdi->wal.loose_insl += mir->fWalInsAirSpace * 0.67;
  mdi->wal.loose_insl += mir->fWalInsAirSpace + fDepthChange;
  mdi->wal.wall_vent = WV_NOTVENT; // wall is now treated as unvented

  mir->flgRetrofits[ndx] = TRUE;
  STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
  STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

  initialize_fuel_types();
  //STRCPY(mir->Results[mir->Rndx].sName, "Wall Cellulose Loose");
  mir->Results[mir->Rndx].fQuant = quant;

  additional_cost(mdi->wal.add_cost);

  mir->Results[mir->Rndx].measure_id = ndx;
  mir->Results[mir->Rndx].audit_section_id = M_WALL;
  mir->Results[mir->Rndx].material_id = iMat;

  mir->Rndx++;

  return;
}

/******************* FUNCTION NAME: retro_insulate_wall_cellulose_add *****/
/**         DATE:  December 1993                                        **/
/**           BY:  NLW              SLF                                 **/
/**  DESCRIPTION:  Called by the ...Retrofits() functions.              **/
/** The retroInsulate_Wall retrofit will be applied if airspace         **/
/** greater than 1 inch and no other wall retrofit applied.             **/
/** (Wall blown with loose cellulose.)                                  **/
/*************************************************************************/
void retro_insulate_wall_cellulose_add(void) {
  int ndx =  M_CMS_WALL_CELLULOSE_LOOSE_INSL_ADD;
  int iMat = M_MAT_WALL_CELLULOSE_LOOSE_INSL_ADD;

  float density = mdi->key.density_of_loose_cellulose_insulation;
  float bagsize = mdi->key.bag_size_for_loose_cellulose_insulation;
  float quant;
  float fAdjWallDepth = mir->fAWLInsAirSpace; // Depth of added insulation
  // adjusted for compression of existing insulation.

  mir->flgRetrofits[ndx] = FALSE;

  if (mir->flgRetrofits[M_CMS_WALL_FIBERGLASS_BATT_INSL_ADD] ||
    mir->flgRetrofits[M_CMS_WALL_FIBERGLASS_LOOSE_INSL_ADD] ||
    mir->fAWLInsAirSpace < 1.0)
    return;

  // quantity is in units of bags.  we first calculate the
  // volume of the space minus a framing-area factor , then based on the
  // volume we get total weight using a in-situ density nuymber then
  // divide the total weight of insulation by the bag size to
  // get the number of bags.  We round up the final figure to
  // an integer number of bags (if more than 10% of a bag is used)

  /*********************************************************
  Compute the depth of insulation to be added accounting for
  compression of the existing insulation.
  *********************************************************/

  if (density > mir->fDensExistBatInsul)
    fAdjWallDepth = mir->fAWLInsAirSpace + mir->fAWallInsDepth * (1.0f - mir->fDensExistBatInsul / density);

  ASSERT(mir->fNetAWLArea > 10.0, sprintf(msg, "Assertion Failure"));
  ASSERT(bagsize != 0, sprintf(msg, "Assertion Failure"));

  //      quant = (mir->fNetAWLArea * 0.85 * 0.85) *
  //              (mir->fAWLInsAirSpace / 12.0) * density / bagsize;
  quant = (float)((mir->fNetAWLArea * 0.85 * 0.85) * (fAdjWallDepth / 12.0) * density / bagsize);
  if ((quant - (int)(quant)) > 0.10)
    quant = (float)((int)(quant + 1.0F)); // intentional double cast
  else
    quant = (float)((int)(quant));

  // NW,9/11/95: First approximation of reduced effectiveness of wall insulation
  //      due to 33% uninsulatable spaces

  mdi->awl.loose_insl += (float)(mir->fAWLInsAirSpace * 0.67);
  mdi->awl.wall_vent = WV_NOTVENT; // wall is now treated as unvented
  mir->flgRetrofits[ndx] = TRUE;
  STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
  STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

  initialize_fuel_types();
  //STRCPY(mir->Results[mir->Rndx].sName, "Wall Cellulose Loose [Addition]");
  mir->Results[mir->Rndx].fQuant = quant;
  //STRCPY(mir->Results[mir->Rndx].sMaterial, "Wall cellulose loose [Addition]");

  additional_cost(mdi->awl.add_cost);

  mir->Results[mir->Rndx].measure_id = ndx;
  mir->Results[mir->Rndx].audit_section_id = M_WALL_ADDITION;
  mir->Results[mir->Rndx].material_id = iMat;

  mir->Rndx++;

  return;
}

/******************* FUNCTION NAME: retro_insulate_wall_fiberglass ********/
/**         DATE:  December 1993                                        **/
/**           BY:  NLW, SLF                                             **/
/**  DESCRIPTION:  Called by the ...Retrofits() functions.              **/
/** The retroInsulate_Wall retrofit will be applied if airspace         **/
/** greater than 1 inches and no other wall retrofit applied.           **/
/** (Wall blown with loose fiberglass.)                                 **/
/*************************************************************************/
void retro_insulate_wall_fiberglass(void) {
  int ndx =  M_CMS_WALL_FIBERGLASS_LOOSE_INSL;
  int iMat = M_MAT_WALL_FIBERGLASS_LOOSE_INSL;

  float density = mdi->key.density_of_loose_fiberglass_insulation;
  float bagsize = mdi->key.bag_size_for_loose_fiberglass_insulation;
  float quant;
  float fAdjWallDepth = mir->fWalInsAirSpace; // Depth of added insulation
  // adjusted for compression of existing insulation.
  float fDepthChange = 0.0f; // Change in insulations depth due to compression;

  mir->flgRetrofits[ndx] = FALSE;

  /**********************************
  This retrofit fills the insulatable air space with loose fiberglass if
  other wall retrofits are not applied and the insulatable airspace is
  greater than 1 inch.
  ***********************************/

  if (mir->flgRetrofits[M_CMS_WALL_FIBERGLASS_BATT_INSL] || 
    mir->flgRetrofits[M_CMS_WALL_CELLULOSE_LOOSE_INSL] ||
    mir->fWalInsAirSpace < 1.0)
    return;

  // quantity is in units of bags.  we first calculate the
  // volume of the space minus a framing-area factor , then based on the
  // volume we get total weight using a in-situ density nuymber then
  // divide the total weight of insulation by the bag size to
  // get the number of bags.  We round up the final figure to
  // an integer number of bags (if more than 10% of a bag is used)

  /*********************************************************
  Compute the depth of insulation to be added accounting for
  compression of the existing insulation.
  *********************************************************/

  if (density > mir->fDensExistBatInsul) {
    fDepthChange = mir->fWallInsDepth * (1.0f - mir->fDensExistBatInsul / density);
    fAdjWallDepth = mir->fWalInsAirSpace + fDepthChange;
  }

  ASSERT(mir->fNetWalArea > 10.0, sprintf(msg, "Assertion Failure"));
  ASSERT(bagsize != 0, sprintf(msg, "Assertion Failure"));

  /**********************************************************************
  Replace fixed 0.85 "general reduction factor" with actual fraction of
  uninsulatable area from ua_wal.c. This includes user-defined area,
  mdi->wal.uninsulatable_area.  MBG 6/03
  **********************************************************************/

  //      quant = (mir->fNetWalArea * 0.85 * 0.85 - mdi->wal.uninsulatable_area ) *
  //              (mir->fWalInsAirSpace / 12.0) * density / bagsize;
  quant = (float)((mir->fNetWalArea * 0.85 * (1.0f - mir->fFractWallUnins)) * (fAdjWallDepth / 12.0) * density / bagsize);
  if ((quant - (int)(quant)) > 0.10)
    quant = (float)((int)(quant + 1)); // double cast
  else
    quant = (float)((int)(quant));

  // NW,9/11/95: First approximation of reduced effectiveness of wall insulation
  //      due to 33% uninsulatable spaces
  // Above eliminated 6/03, replaced by separate paths for insulatable and
  // uninsulatable areas in ua_wal.  MBG

  //      mdi->wal.loose_insl += mir->fWalInsAirSpace * 0.67;
  mdi->wal.loose_insl += mir->fWalInsAirSpace + fDepthChange;
  mdi->wal.wall_vent = WV_NOTVENT; // wall is now treated as unvented

  mir->flgRetrofits[ndx] = TRUE;
  STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
  STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

  initialize_fuel_types();
  //STRCPY(mir->Results[mir->Rndx].sName, "Wall Fiberglass Loose");
  mir->Results[mir->Rndx].fQuant = quant;

  additional_cost(mdi->wal.add_cost);

  mir->Results[mir->Rndx].measure_id = ndx;
  mir->Results[mir->Rndx].audit_section_id = M_WALL;
  mir->Results[mir->Rndx].material_id = iMat;

  mir->Rndx++;

  return;
}

/******************* FUNCTION NAME: retro_insulate_wall_fiberglass_add ****/
/**         DATE:  December 1993                                        **/
/**           BY:  NLW, SLF                                             **/
/**  DESCRIPTION:  Called by the ...Retrofits() functions.              **/
/** The retroInsulate_Wall retrofit will be applied if airspace         **/
/** greater than 1 inches and no other wall retrofit applied.           **/
/** (Wall blown with loose fiberglass.)                                 **/
/*************************************************************************/
void retro_insulate_wall_fiberglass_add(void) {
  int ndx =  M_CMS_WALL_FIBERGLASS_LOOSE_INSL_ADD;
  int iMat = M_MAT_WALL_FIBERGLASS_LOOSE_INSL_ADD;

  float density = mdi->key.density_of_loose_fiberglass_insulation;
  float bagsize = mdi->key.bag_size_for_loose_fiberglass_insulation;
  float quant;
  float fAdjWallDepth = mir->fWalInsAirSpace; // Depth of added insulation
  // adjusted for compression of existing insulation.

  mir->flgRetrofits[ndx] = FALSE;

  if (mir->flgRetrofits[M_CMS_WALL_FIBERGLASS_BATT_INSL_ADD] || 
    mir->flgRetrofits[M_CMS_WALL_CELLULOSE_LOOSE_INSL_ADD] ||
    mir->fAWLInsAirSpace < 1.0)
    return;

  // quantity is in units of bags.  we first calculate the
  // volume of the space minus a framing-area factor , then based on the
  // volume we get total weight using a in-situ density nuymber then
  // divide the total weight of insulation by the bag size to
  // get the number of bags.  We round up the final figure to
  // an integer number of bags (if more than 10% of a bag is used)

  /*********************************************************
  Compute the depth of insulation to be added accounting for
  compression of the existing insulation.
  *********************************************************/

  if (density > mir->fDensExistBatInsul) {
    fAdjWallDepth = mir->fAWLInsAirSpace + mir->fAWallInsDepth * (1.0f - mir->fDensExistBatInsul / density);
  }

  ASSERT(mir->fNetAWLArea > 10.0, sprintf(msg, "Assertion Failure"));
  ASSERT(bagsize != 0, sprintf(msg, "Assertion Failure"));

  quant = (float)((mir->fNetAWLArea * 0.85 * 0.85) * (fAdjWallDepth / 12.0) * density / bagsize);
  if ((quant - (int)(quant)) > 0.10)
    quant = (float)((int)(quant + 1)); // double cast
  else
    quant = (float)((int)(quant));

  // NW,9/11/95: First approximation of reduced effectiveness of wall insulation
  //      due to 33% uninsulatable spaces

  mdi->awl.loose_insl += (float)(mir->fAWLInsAirSpace * 0.67);
  mdi->awl.wall_vent = WV_NOTVENT; // wall is now treated as unvented
  mir->flgRetrofits[ndx] = TRUE;
  STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
  STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

  initialize_fuel_types();
  //STRCPY(mir->Results[mir->Rndx].sName, "Wall Fiberglass Loose [Addition]");
  mir->Results[mir->Rndx].fQuant = quant;
  //STRCPY(mir->Results[mir->Rndx].sMaterial, "Wall fiberglass loose insl [Addition]");

  additional_cost(mdi->awl.add_cost);

  mir->Results[mir->Rndx].measure_id = ndx;
  mir->Results[mir->Rndx].audit_section_id = M_WALL_ADDITION;
  mir->Results[mir->Rndx].material_id = iMat;

  mir->Rndx++;

  return;
}

/******************* FUNCTION NAME: retro_insulate_belly_cellulose ********/
/**         DATE:  Dec 1993                                             **/
/**           BY:  NLW, SLF                                             **/
/**  DESCRIPTION:  Called by the ...Retrofits() functions.              **/
/** The retroInsulate_Belly retrofit will be applied if the existing    **/
/** average depth of insulation in the belly is less than the belly     **/
/** depth.  This retrofit may be applied if there is an air space       **/
/** between the floor and a batt/blanket attached to the joists.        **/
/*************************************************************************/
void retro_insulate_belly_cellulose(void) {
  int ndx =  M_CMS_BELLY_CELLULOSE_LOOSE_INSL;
  int iMat = M_MAT_BELLY_CELLULOSE_LOOSE_INSL;

  float density = mdi->key.density_of_loose_cellulose_insulation;
  float densitycntr = mir->fDensBellyCelInsul;
  float bagsize = mdi->key.bag_size_for_loose_cellulose_insulation;
  float quant;
  float fABelly;
  float fDuctVolume = 0.0;               // Volume of ductwork, cf
  float fAdjWingDepth = mir->fWngAirSpace; // Total depth of added insulation
  // accounting for compression of existing insulation
  float fAdjCntrDepth = mir->fBellyAirSpace;
  static float fDensExistWing, fDensExistCntr;

  mir->flgRetrofits[ndx] = FALSE;

  fABelly = (float)(mdi->gnl.width * mdi->gnl.length * 0.5);

  fDuctVolume = floor_duct_volume();

  if (mir->flgRetrofits[M_CMS_BELLY_FIBERGLASS_LOOSE_INSL] ||
    mir->fBellyAirSpace < 2.0)
    return;

  /********************************
  *  Determine if the existing density should be for FB loose or batt
  ********************************/

  if (mir->flgWhichPass == FIRST_PASS) {
    if (mdi->flr.belly_mineral_insl > mdi->flr.belly_loose_insl)
      fDensExistCntr = mir->fDensExistBatInsul;
    else
      fDensExistCntr = DENSITY_EXIST_FG_INSUL;

    if (mdi->flr.wing_mineral_insl > mdi->flr.wing_loose_insl)
      fDensExistWing = mir->fDensExistBatInsul;
    else
      fDensExistWing = DENSITY_EXIST_FG_INSUL;
  }

  /* There is no reason to apply more than 8 inches of blown */
  /* insulation in the belly, so here is where we limit it, MJF 11/02 */
  /* If air space is restricted to < 8 inches, account for compression */

  if (mir->fBellyAirSpace > 8.0) {
    mir->fBellyAirSpace = 8.0;
    mir->flgLimitBellyInsul = TRUE;
  } else if (densitycntr > fDensExistCntr)
    fAdjCntrDepth = mir->fBellyAirSpace + mir->fBellyInsDepth * (1.0f - fDensExistCntr / densitycntr);

  /*********************************************************
  Compute the depth of insulation to be added in the wings
  accounting for compression of the existing insulation.
  *********************************************************/

  if (density > fDensExistWing)
    fAdjWingDepth = mir->fWngAirSpace + mir->fWingInsDepth * (1.0f - fDensExistWing / density);

  /********************
  SLF 7/14/94 - Changed per Beta reviewer comments.
  fRetroQuant[6] is passed out in bags.
  bags = sf * (in/in per ft) * (lb/cf cellulose) * (bags/lb)
  (Conversion numbers came from page 92.19 of SH Notes.)
  *********************
  ERG, NLW, 7/16/94  -  Adjust insulation volume calculations for
  framing and wing considerations.  Insulatable airspace includes
  space between floor and belly, space between the joist and belly,
  and space in the wings (if any), less the volume of ducts (if any).
  Per prior NREL definitions, a joist factor of 10% will be applied
  in lieu of width-length dependent calculations.
  See NREL notes pages 92.6 and 92.11 thru 92.15.

  (NW,9/1/85: Note the formula below assumes area of wings = area belly)

  ********************/

  ASSERT(bagsize != 0, sprintf(msg, "Assertion Failure"));

  //      quant = (((fABelly * 0.9) * (mir->fBellyAirSpace / 12.0)) +
  //              ((fABelly * 0.1) * (mir->fJoistAirSpace / 12.0)) +
  //              (fABelly * (mir->fWngAirSpace / 12.0)) -
  //              fDuctVolume ) * density / bagsize;

  quant =
      (float)(((fABelly * 0.9 * fAdjCntrDepth / 12.0 + fABelly * 0.1 * mir->fJoistAirSpace / 12.0 - fDuctVolume) * densitycntr +
               (fABelly * fAdjWingDepth / 12.0) * density) /
              bagsize);

  if ((quant - (int)(quant)) > 0.10)
    quant = (float)((int)(quant + 1)); // double cast
  else
    quant = (float)((int)(quant));

  /***************************
  Add the belly free space depth to the depth of the loose insulation.
  Also redefine belly condition to "good".
  *****************************/
  // NW, 9/6/95: Reduce belly insulation effectiveness by 50% for uninsulatable
  //      airspace

  mdi->flr.belly_loose_insl += (float)(mir->fBellyAirSpace * 1.0);

  // NW,9/1/95: add insulation adjustment for wings
  mdi->flr.wing_loose_insl = fAdjWingDepth; // MBG 7/03

  // NW, 9/5/96: eliminate condition improvement due to overstated retrofit savings
  // MJF 4/2/03 Added this effect back in now that we treat belly
  // condition derating differently in ua_flr.c

  mdi->flr.belly_condition = BC_GOOD;

  mir->flgRetrofits[ndx] = TRUE;
  STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
  STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

  initialize_fuel_types();
  //STRCPY(mir->Results[mir->Rndx].sName, "Belly Cellulose Loose");
  mir->Results[mir->Rndx].fQuant = quant;

  additional_cost(mdi->flr.add_cost);

  mir->Results[mir->Rndx].measure_id = ndx;
  mir->Results[mir->Rndx].audit_section_id = M_FLOOR;
  mir->Results[mir->Rndx].material_id = iMat;

  mir->Rndx++;

  return;
}

/******************* FUNCTION NAME: retro_insulate_belly_cellulose_add ****/
/**         DATE:  Dec 1993                                             **/
/**           BY:  NLW, SLF                                             **/
/**  DESCRIPTION:  Called by the ...Retrofits() functions.              **/
/** The retroInsulate_Belly retrofit will be applied if the existing    **/
/** average depth of insulation in the belly is less than the belly     **/
/** depth.  This retrofit may be applied if there is an air space       **/
/** between the floor and a batt/blanket attached to the joists.        **/
/*************************************************************************/
void retro_insulate_belly_cellulose_add(void) {
  int ndx =  M_CMS_BELLY_CELLULOSE_LOOSE_INSL_ADD;
  int iMat = M_MAT_BELLY_CELLULOSE_LOOSE_INSL_ADD;

  float density = mdi->key.density_of_loose_cellulose_insulation;
  float bagsize = mdi->key.bag_size_for_loose_cellulose_insulation;
  float quant;
  float fAdjFloorDepth = mdi->afl.avail_insl;
  static float fDensExist;

  mir->flgRetrofits[ndx] = FALSE;

  // note that this is the same test as the belly blow with cellulose
  // for the main home (ie these should be considered together) with
  // the addition of a test for the available air space in the addition

  if (mir->flgRetrofits[M_CMS_BELLY_FIBERGLASS_LOOSE_INSL] ||
      mdi->afl.add_floor_type == SLABONGRADE ||
      mdi->afl.avail_insl < 2.0)
    return;

  /********************************
  *  Determine if the existing density should be for FB loose or batt
  ********************************/

  if (mir->flgWhichPass == FIRST_PASS) {
    if (mdi->afl.mineral_insl > mdi->afl.loose_insl)
      fDensExist = mir->fDensExistBatInsul;
    else
      fDensExist = DENSITY_EXIST_FG_INSUL;
  }

  /* There is no reason to apply more than 8 inches of blown */
  /* insulation in the addition floor, so here is where we limit it, MJF 11/02 */

  if (mdi->afl.avail_insl > 8.0) {
    mdi->afl.avail_insl = fAdjFloorDepth = 8.0;
    mir->flgLimitBellyInsulAdd = TRUE;
  } else if (density > fDensExist)
    fAdjFloorDepth = mdi->afl.avail_insl + mir->fAFloorInsDepth * (1.0f - fDensExist / density);

  /********************
  SLF 7/14/94 - Changed per Beta reviewer comments.
  fRetroQuant[6] is passed out in bags.
  bags = ft * ft * (in/in per ft) * (lb/cf cellulose) * (bags/lb)
  (Conversion numbers came from page 92.19 of SH Notes.)
  *********************
  NW 7/14/94 - reduce insulation volume by 10% framing factor
  ********************/

  ASSERT(bagsize != 0, sprintf(msg, "Assertion Failure"));

  //      quant = (mdi->afl.length * mdi->afl.width * 0.9) *
  //              (mdi->afl.avail_insl / 12.0) * density / bagsize;
  quant = (float)((mdi->afl.length * mdi->afl.width * 0.9) * (fAdjFloorDepth / 12.0) * density / bagsize);
  if ((quant - (int)(quant)) > 0.10)
    quant = (float)((int)(quant + 1)); // double cast
  else
    quant = (float)((int)(quant));

  /***************************
  Add the belly free space depth to the depth of the loose insulation.
  *****************************/
  mdi->afl.loose_insl += mdi->afl.avail_insl;
  mdi->afl.avail_insl = 0.0;

  mir->flgRetrofits[ndx] = TRUE;
  STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
  STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

  initialize_fuel_types();
  //STRCPY(mir->Results[mir->Rndx].sName, "Belly Cellulose Loose [Addition]");
  mir->Results[mir->Rndx].fQuant = quant;
  //STRCPY(mir->Results[mir->Rndx].sMaterial, "Belly cellulose loose insl [Addition]");

  //Currently there is no additional cost for MHEA floor additions MJF 8/17/11

  mir->Results[mir->Rndx].measure_id = ndx;
  mir->Results[mir->Rndx].audit_section_id = M_FLOOR_ADDITION;
  mir->Results[mir->Rndx].material_id = iMat;

  mir->Rndx++;

  return;
}

/******************* FUNCTION NAME: retro_insulate_belly_fiberglass *******/
/**         DATE:  Dec 1993                                             **/
/**           BY:  NLW, SLF                                             **/
/**  DESCRIPTION:  Called by the ...Retrofits() functions.              **/
/** The retroInsulate_Belly retrofit will be applied if the existing    **/
/** average depth of insulation in the belly is less than the belly     **/
/** depth.  This retrofit may be applied if there is an air space       **/
/** between the floor and a batt/blanket attached to the joists.        **/
/*************************************************************************/
void retro_insulate_belly_fiberglass(void) {
  int ndx =  M_CMS_BELLY_FIBERGLASS_LOOSE_INSL;
  int iMat = M_MAT_BELLY_FIBERGLASS_LOOSE_INSL;

  float density = mdi->key.density_of_loose_fiberglass_insulation;
  float densitycntr = mir->fDensBellyLFGInsul;
  float bagsize = mdi->key.bag_size_for_loose_fiberglass_insulation;
  float quant;
  float fABelly;
  float fDuctVolume = 0.0;               // Volume of ductwork, cf
  float fAdjWingDepth = mir->fWngAirSpace; // Total depth of added insulation
  // accounting for compression of existing insulation
  float fAdjCntrDepth = mir->fBellyAirSpace;
  static float fDensExistWing, fDensExistCntr, fCompExistLooseIns;

  mir->flgRetrofits[ndx] = FALSE;

  fABelly = (float)(mdi->gnl.width * mdi->gnl.length * 0.5);

  fDuctVolume = floor_duct_volume();

  if (mir->flgRetrofits[M_CMS_BELLY_CELLULOSE_LOOSE_INSL] ||
    mir->fBellyAirSpace < 2.0)
    return;

  /********************************
  *  Determine if the existing density should be for FB loose or batt
  ********************************/

  if (mir->flgWhichPass == FIRST_PASS) {
    if (mdi->flr.belly_mineral_insl > mdi->flr.belly_loose_insl)
      fDensExistCntr = mir->fDensExistBatInsul;
    else
      fDensExistCntr = DENSITY_EXIST_FG_INSUL;

    if (mdi->flr.wing_mineral_insl > mdi->flr.wing_loose_insl)
      fDensExistWing = mir->fDensExistBatInsul;
    else
      fDensExistWing = DENSITY_EXIST_FG_INSUL;

    fCompExistLooseIns = mdi->flr.belly_loose_insl * fDensExistWing / density;
  }

  /* There is no reason to apply more than 8 inches of blown */
  /* insulation in the belly, so here is where we limit it, MJF 11/02 */
  /* If air space is restricted to < 8 inches, account for compression */

  if (mir->fBellyAirSpace > 8.0) {
    mir->fBellyAirSpace = 8.0;
    mir->flgLimitBellyInsul = TRUE;
  } else if (densitycntr > fDensExistCntr)
    fAdjCntrDepth = mir->fBellyAirSpace + mir->fBellyInsDepth * (1.0f - fDensExistCntr / densitycntr);

  /*********************************************************
  Compute the depth of insulation to be added accounting for
  compression of the existing insulation.
  *********************************************************/

  if (density > fDensExistWing)
    fAdjWingDepth = mir->fWngAirSpace + mir->fWingInsDepth * (1.0f - fDensExistWing / density);

  /********************
  SLF 7/14/94 - Changed per Beta reviewer comments.
  fRetroQuant[6] is passed out in bags.
  bags = sf * (in/in per ft) * (lb/cf cellulose) * (bags/lb)
  (Conversion numbers came from page 92.19 of SH Notes.)
  *********************
  ERG, NLW, 7/16/94  -  Adjust insulation volume calculations for
  framing and wing considerations.  Insulatable airspace includes
  space between floor and belly, space between the joist and belly,
  and space in the wings (if any), less the volume of ducts (if any).
  Per prior NREL definitions, a joist factor of 10% will be applied
  in lieu of width-length dependent calculations.
  See NREL notes pages 92.6 and 92.11 thru 92.15.

  (NW,9/1/85: Note the formula below assumes area of wings = area belly)

  ********************/

  ASSERT(bagsize != 0, sprintf(msg, "Assertion Failure"));

  //      quant = (((fABelly * 0.9) * (mir->fBellyAirSpace / 12.0)) +
  //              ((fABelly * 0.1) * (mir->fJoistAirSpace / 12.0)) +
  //              (fABelly * (mir->fWngAirSpace / 12.0)) -
  //              fDuctVolume ) * density / bagsize;

  quant =
      (float)(((fABelly * 0.9 * fAdjCntrDepth / 12.0 + fABelly * 0.1 * mir->fJoistAirSpace / 12.0 - fDuctVolume) * densitycntr +
               (fABelly * fAdjWingDepth / 12.0) * density) /
              bagsize);
  if ((quant - (int)(quant)) > 0.10)
    quant = (float)((int)(quant + 1));
  else
    quant = (float)((int)(quant));

  // MJF 9/01, note that wing insulation depth was NOT increased
  // in code prior to this date. The potential is that fiberglass
  // belly insulation measure savings were understated.
  // belly condition change is no longer included

  // MJF 4/2/03 Added this effect back in now that we treat belly
  // condition derating differently in ua_flr.c
  mdi->flr.belly_condition = BC_GOOD;

  mdi->flr.belly_loose_insl += mir->fBellyAirSpace;
  mdi->flr.wing_loose_insl = fAdjWingDepth + fCompExistLooseIns;
  // MBG 7/03

  mir->flgRetrofits[ndx] = TRUE;
  STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
  STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

  initialize_fuel_types();
  //STRCPY(mir->Results[mir->Rndx].sName, "Belly Fiberglass Loose");
  mir->Results[mir->Rndx].fQuant = quant;

  additional_cost(mdi->flr.add_cost);

  mir->Results[mir->Rndx].measure_id = ndx;
  mir->Results[mir->Rndx].audit_section_id = M_FLOOR;
  mir->Results[mir->Rndx].material_id = iMat;

  mir->Rndx++;

  return;
}

/******************* FUNCTION NAME: retro_insulate_belly_fiberglass_add ***/
/**         DATE:  Dec 1993                                             **/
/**           BY:  NLW, SLF                                             **/
/**  DESCRIPTION:  Called by the ...Retrofits() functions.              **/
/** The retroInsulate_Belly retrofit will be applied if the existing    **/
/** average depth of insulation in the belly is less than the belly     **/
/** depth.  This retrofit may be applied if there is an air space       **/
/** between the floor and a batt/blanket attached to the joists.        **/
/*************************************************************************/
void retro_insulate_belly_fiberglass_add(void) {
  int ndx =  M_CMS_BELLY_FIBERGLASS_LOOSE_INSL_ADD;
  int iMat = M_MAT_BELLY_FIBERGLASS_LOOSE_INSL_ADD;

  float density = mdi->key.density_of_loose_fiberglass_insulation;
  float bagsize = mdi->key.bag_size_for_loose_fiberglass_insulation;
  float quant;
  float fAdjFloorDepth = mdi->afl.avail_insl;
  static float fDensExist;

  mir->flgRetrofits[ndx] = FALSE;

  // note that this is the same test as the belly blow with cellulose
  // for the main home (ie these should be considered together) with
  // the addition of a test for the available air space in the addition

  if (mir->flgRetrofits[M_CMS_BELLY_CELLULOSE_LOOSE_INSL] ||
    mdi->afl.add_floor_type == SLABONGRADE ||
    mdi->afl.avail_insl < 2.0)
    return;

  /********************************
  *  Determine if the existing density should be for FB loose or batt
  ********************************/

  if (mir->flgWhichPass == FIRST_PASS) {
    if (mdi->afl.mineral_insl > mdi->afl.loose_insl)
      fDensExist = mir->fDensExistBatInsul;
    else
      fDensExist = DENSITY_EXIST_FG_INSUL;
  }

  /* There is no reason to apply more than 8 inches of blown */
  /* insulation in the addition floor, so here is where we limit it, MJF 11/02 */

  if (mdi->afl.avail_insl > 8.0) {
    mdi->afl.avail_insl = 8.0;
    mir->flgLimitBellyInsulAdd = TRUE;
  } else if (density > fDensExist)
    fAdjFloorDepth = mdi->afl.avail_insl + mir->fAFloorInsDepth * (1.0f - fDensExist / density);

  /********************
  SLF 7/14/94 - Changed per Beta reviewer comments.
  fRetroQuant[6] is passed out in bags.
  bags = ft * ft * (in/in per ft) * (lb/cf cellulose) * (bags/lb)
  (Conversion numbers came from page 92.19 of SH Notes.)
  *********************
  NW 7/14/94 - reduce insulation volume by 10% framing factor
  ********************/

  ASSERT(bagsize != 0, sprintf(msg, "Assertion Failure"));

  //      quant = (mdi->afl.length * mdi->afl.width * 0.9) *
  //              (mdi->afl.avail_insl / 12.0) * density / bagsize;
  quant = (float)((mdi->afl.length * mdi->afl.width * 0.9) * (fAdjFloorDepth / 12.0) * density / bagsize);

  if ((quant - (int)(quant)) > 0.10)
    quant = (float)((int)(quant + 1));
  else
    quant = (float)((int)(quant));

  /***************************
  Add the belly free space depth to the depth of the loose insulation.
  *****************************/
  mdi->afl.loose_insl += mdi->afl.avail_insl;
  mdi->afl.avail_insl = 0.0;

  mir->flgRetrofits[ndx] = TRUE;
  STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
  STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

  initialize_fuel_types();
  //STRCPY(mir->Results[mir->Rndx].sName, "Belly Fiberglass Loose [Addition]");
  mir->Results[mir->Rndx].fQuant = quant;
  //STRCPY(mir->Results[mir->Rndx].sMaterial, "Belly fiberglass loose insl [Addition]");

  //Currently there is no additional cost for MHEA floor additions MJF 8/17/11    

  mir->Results[mir->Rndx].measure_id = ndx;
  mir->Results[mir->Rndx].audit_section_id = M_FLOOR_ADDITION;
  mir->Results[mir->Rndx].material_id = iMat;

  mir->Rndx++;

  return;
}

/******************* FUNCTION NAME: retro_insulate_roof_cellulose *********/
/**         DATE:  Dec 1993                                             **/
/**           BY:  NLW          SLF                                     **/
/**  DESCRIPTION:  Called by the ...Retrofits() functions.              **/
/*************************************************************************/
void retro_insulate_roof_cellulose(void) {
  int ndx =  M_CMS_ROOF_CELLULOSE_LOOSE_INSL;
  int iMat = M_MAT_ROOF_CELLULOSE_LOOSE_INSL;

  float density;
  float bagsize = mdi->key.bag_size_for_loose_cellulose_insulation;
  float quant;
  float fARoof;
  float fDuctVolume = 0.0; // volume of ductwork, cf

  // Additional variables needed to implement DeSoto's bag quantities

  float fHeightRoofPeak = mdi->rof.ceiling_height; // Height of bowstring roof peak
  float VIns,                                      // Volume of insulation
      Dins,                                        // Depth of existing insulation (in.)
      VFrm,                                        // Volume of framing
      VDome,                                       // Volume of empty attic cavity
      VTot,                                        // Total volume of attic space
      VExist,                                      // Volume of existing insulation
      TrussH = 2.,                                 // Truss Height (in) (set to match DeSoto assumption)
      TrussW = 2.,                                 // Truss Width (in) (set to match DeSoto assumption)
      TrussSep = 16.,                              // Truss Separation (in) (from DeSoto assumptions)
      fDedge = 2.5,                                // Ceiling depth at outer edge (from DeSoto's assumptions)
      Densexist,                                   // Density of existing insulation (from DeSoto's assumptions)
      Area = mir->fAreaCeilingTotal,               // Ceiling area from ua_rof.c
      fSpaceComp;                                  // Space in flat roof for insulation after compression
      //fDepthChange; // Change in depth of insulation which can be added due to compression

  mir->flgRetrofits[ndx] = FALSE;

  // MJF 9/01 For some reason the previous code was looking for
  // 3 or more inches available space, while all the other roof
  // insulation measures in the addition and for fiberglass were looking
  // for 2 or more inches... so I switched this to 2 inches for consistency

  if (mir->flgRetrofits[M_CMS_ROOF_FIBERGLASS_LOOSE_INSL] ||
   mir->fRofInsAirSpace < 2.0)
    return;

  // Set existing density equal to density of majority of existing insulation
  // batt or blown.  7/03  MBG

  if (mir->fRofThicknessBattIns > mir->fRofThicknessLFGIns)
    Densexist = mir->fDensExistBatInsul;
  else
    Densexist = DENSITY_EXIST_FG_INSUL;

  // Set densities of existing and installed insulation. Note, DeSoto's
  // original equations assumed 0.9 lb/cuft. This assumes existing is FG!
  //   MBG 6/03
  // Assume denspack insulation for all but pitched roof type

  if (mdi->rof.roof_type == RT_PITCHED) {
    density = mir->fDensExistCelInsul;
    //fDepthChange = 0.0f;
  } else {
    density = mdi->key.density_of_loose_cellulose_insulation;
    //fDepthChange = mir->fThicknessCeilIns * (1.0f - Densexist / density);
  }

  fARoof = mdi->gnl.width * mdi->gnl.length;

  fDuctVolume = ceiling_duct_volume();

  /********************
  SLF 7/14/94 - Changed per Beta reviewer comments.
  fRetroQuant[8] is passed out in bags.
  bags = sf * (in/in per ft) * (lb/cf cellulose) * (bags/lb)
  (Conversion numbers came from page 92.19 of SH Notes.)
  *********************/

  ASSERT(bagsize != 0, sprintf(msg, "Assertion Failure"));

  fSpaceComp = mir->fRofInsAirSpace + mir->fThicknessCeilIns * (1.0f - Densexist / density);

  if (mdi->rof.roof_type == RT_BOWSTRING) {

    Dins = mir->fThicknessCeilIns;
    VExist = (float)(Dins * Area / 12.0 * Densexist / density);
    VTot = (float)(Area * (fHeightRoofPeak + fDedge) / 24.0);
    VDome = VTot - VExist;
    VFrm = (float)(TrussH * TrussW / 144. * Area * 24. / TrussSep);
    VIns = VDome - VFrm - fDuctVolume;
    quant = VIns * density / bagsize;
    mdi->rof.loose_insl = fSpaceComp + mir->fRofThicknessLFGIns * Densexist / density;
  }

  /*******************
  NW, ERG 7/16/94 - Apply adjustment for framing and duct volume to
  reduce insulatable volume in roof.  Assume frame factor of 10%.
  ********************/

  else if (mdi->rof.roof_type == RT_FLAT) {
    quant = (float)((((fARoof * 0.9) * (fSpaceComp / 12.0)) - fDuctVolume) * density / bagsize);
    mdi->rof.loose_insl = fSpaceComp + mir->fRofThicknessLFGIns * Densexist / density;
  }

  else { // Pitched
    quant = (float)((((fARoof * 0.9) * (mir->fRofInsAirSpace / 12.0)) - fDuctVolume) * density / bagsize);
    mdi->rof.loose_insl += mir->fRofInsAirSpace;
  }

  if ((quant - (int)(quant)) > 0.10)
    quant = (float)((int)(quant + 1));
  else
    quant = (float)((int)(quant));

  /****************************
  Make the retrofit insulation thickness equal to the insulatable
  air space minus the thickness of any existing insulation.
  Adjustment for duct losses are made in the infiltration,
  heating and cooling consumption functions.
  *****************************/

  mir->flgRetrofits[ndx] = TRUE;
  STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
  STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

  initialize_fuel_types();
  //STRCPY(mir->Results[mir->Rndx].sName, "Roof Cellulose Loose");
  mir->Results[mir->Rndx].fQuant = quant;

  additional_cost(mdi->rof.add_cost);

  mir->Results[mir->Rndx].measure_id = ndx;
  mir->Results[mir->Rndx].audit_section_id = M_CEILING;
  mir->Results[mir->Rndx].material_id = iMat;

  mir->Rndx++;

  return;
}

/******************* FUNCTION NAME: retro_insulate_roof_cellulose_add *****/
/**         DATE:  Dec 1993                                             **/
/**           BY:  NLW          SLF                                     **/
/**  DESCRIPTION:  Called by the ...Retrofits() functions.              **/
/*************************************************************************/
void retro_insulate_roof_cellulose_add(void) {
  int ndx =  M_CMS_ROOF_CELLULOSE_LOOSE_INSL_ADD;
  int iMat = M_MAT_ROOF_CELLULOSE_LOOSE_INSL_ADD;

  float density;
  float bagsize = mdi->key.bag_size_for_loose_cellulose_insulation;
  float fARoof;
  float Densexist; // Density of existing insulation (from DeSoto's assumptions)
  float quant;
  //float fDepthChange; // Change in depth of insulation which can be added
  //   due to compression
  float fSpaceComp; // Space in flat roof for insulation after compression

  mir->flgRetrofits[ndx] = FALSE;

/******************
  Recalculate roof area for addition.
  ******************/

  fARoof = mdi->afl.length * mdi->afl.width;

  // MJF 9/01, note there was an error here in the original code
  // which was testing the mir->flgRetrofits for this retrofit rather
  // than the loose fiberglass retrofit.  The result is that blown
  // cellulose in the addition roof was never considered if the
  // rest of the house was receiving blown cellulose.

  if (mir->flgRetrofits[M_CMS_ROOF_FIBERGLASS_LOOSE_INSL_ADD] ||
    mir->fARFInsAirSpace < 2.0 ||
    fARoof == 0.0)
    return;

  /************************
  NW, 7/16/94 - Adjustments to roof area for different roof types
  not applied at this time as volumes are based on ceiling area and
  effective insulatable space above the ceiling.
  ************************/

  // Set existing density equal to density of majority of existing insulation
  // batt or blown.  7/03  MBG

  if (mir->fRofThicknessBattIns_Add > mir->fRofThicknessLFGIns_Add)
    Densexist = mir->fDensExistBatInsul;
  else
    Densexist = DENSITY_EXIST_FG_INSUL;

  // Set densities of existing and installed insulation, assumes existing
  // FG!  Modified to reflect MH changes of 6/03 on 3/08.  MBG
  // Assume denspack insulation for all but pitched roof type

  if (mdi->awl.wall_config == WC_FLAT) {
    density = mdi->key.density_of_loose_cellulose_insulation;
    //fDepthChange = mir->fThicknessCeilIns_Add * (1.0f - Densexist / density);
  } else {
    density = mir->fDensExistCelInsul;
    //fDepthChange = 0.0f;
  }

  /********************
  SLF 7/14/94 - Changed per Beta reviewer comments.
  fRetroQuant[8] is passed out in bags.
  bags = sf * (in/in per ft) * (lb/cf cellulose) * (bags/lb)
  (Conversion numbers came from page 92.19 of SH Notes.)
  *********************
  NW, ERG 7/16/94 - Apply adjustment for framing and duct volume to
  reduce insulatable volume in roof.  Assume frame factor of 10%.
  ********************/

  ASSERT(bagsize != 0, sprintf(msg, "Assertion Failure"));

  fSpaceComp = mir->fARFInsAirSpace + mir->fThicknessCeilIns_Add * (1.0f - Densexist / density);

  if (mdi->awl.wall_config == WC_FLAT) {
    quant = (float)(((fARoof * 0.9) * (fSpaceComp / 12.0)) * density / bagsize);
    mdi->arf.loose_insl = fSpaceComp + mir->fRofThicknessLFGIns_Add * Densexist / density;
  }

  else { // Pitched
    quant = (float)(((fARoof * 0.9) * (mir->fARFInsAirSpace / 12.0)) * density / bagsize);
    mdi->arf.loose_insl += mir->fARFInsAirSpace;
  }

  if ((quant - (int)(quant)) > 0.10)
    quant = (float)((int)(quant + 1));
  else
    quant = (float)((int)(quant));

  /****************************
  Make the retrofit insulation thickness equal to the insulatable
  air space minus the thickness of any existing insulation.
  *****************************/

  mir->flgRetrofits[ndx] = TRUE;
  STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
  STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

  initialize_fuel_types();
  //STRCPY(mir->Results[mir->Rndx].sName, "Roof Cellulose Loose [Addition]");
  mir->Results[mir->Rndx].fQuant = quant;
  //STRCPY(mir->Results[mir->Rndx].sMaterial, "Roof cellulose loose insl [Addition]");

  additional_cost(mdi->arf.add_cost);

  mir->Results[mir->Rndx].measure_id = ndx;
  mir->Results[mir->Rndx].audit_section_id = M_CEILING_ADDITION;
  mir->Results[mir->Rndx].material_id = iMat;

  mir->Rndx++;

  return;
}

/******************* FUNCTION NAME: retro_insulate_roof_fiberglass *******/
/**         DATE:  Dec 1993                                            **/
/**           BY:  NLW              SLF                                **/
/**  DESCRIPTION:  Called by the ...Retrofits() functions.             **/
/************************************************************************/
void retro_insulate_roof_fiberglass(void) {
  int ndx =  M_CMS_ROOF_FIBERGLASS_LOOSE_INSL;
  int iMat = M_MAT_ROOF_FIBERGLASS_LOOSE_INSL;

  float density;
  float bagsize = mdi->key.bag_size_for_loose_fiberglass_insulation;
  float quant;
  float fARoof;
  float fDuctVolume = 0.0; // volume of ductwork, cf

  // Additional variables needed to implement DeSoto's bag quantities

  float fHeightRoofPeak = mdi->rof.ceiling_height; // Height of bowstring roof peak
  float VIns,                                      // Volume of insulation
      Dins,                                        // Depth of existing insulation (in.)
      VFrm,                                        // Volume of framing
      VDome,                                       // Volume of empty attic cavity
      VTot,                                        // Total volume of attic space
      VExist,                                      // Volume of existing insulation
      TrussH = 2.,                                 // Truss Height (in) (set to match DeSoto assumption)
      TrussW = 2.,                                 // Truss Width (in) (set to match DeSoto assumption)
      TrussSep = 16.,                              // Truss Separation (in) (from DeSoto assumptions)
      fDedge = 2.5,                                // Ceiling depth at outer edge (from DeSoto's assumptions)
      Densexist,                                   // Density of existing insulation
      Area = mir->fAreaCeilingTotal,
      // Ceiling area from ua_rof.c
      fSpaceComp;   // Space in flat roof for insulation after compression
      //fDepthChange; // Change in depth of insulation which can be added
  //   due to compression

  mir->flgRetrofits[ndx] = FALSE;


  // MJF 9/01 For some reason the previous code was looking for
  // 3 or more inches available space, while all the other roof
  // insulation measures in the addition and for fiberglass were looking
  // for 2 or more inches... so I switched this to 2 inches for consistency

  // change to >= 2.0 inches from just > 2.0 inches due to the new
  // logic for pitched roofs returning exactly 2.0 as the insulatable space

  if (mir->flgRetrofits[M_CMS_ROOF_CELLULOSE_LOOSE_INSL] ||
    mir->fRofInsAirSpace < 2.0)
    return;

  // Set existing density equal to density of majority of existing insulation
  // batt or blown.  7/03  MBG

  if (mir->fRofThicknessBattIns > mir->fRofThicknessLFGIns)
    Densexist = mir->fDensExistBatInsul;
  else
    Densexist = DENSITY_EXIST_FG_INSUL;

  // Set densities of existing and installed insulation. Note, DeSoto's
  // original equations assumed 0.9 lb/cuft. This assumes existing is FG!
  //   MBG 6/03
  // Assume denspack insulation for all but pitched roof type

  if (mdi->rof.roof_type == RT_PITCHED) {
    density = DENSITY_EXIST_FG_INSUL;
    //fDepthChange = 0.0f;
  } else {
    density = mdi->key.density_of_loose_fiberglass_insulation;
    //fDepthChange = mir->fThicknessCeilIns * (1.0f - Densexist / density);
  }

  fARoof = mdi->gnl.width * mdi->gnl.length;

  fDuctVolume = ceiling_duct_volume();


  /********************
  SLF 7/14/94 - Changed per Beta reviewer comments.
  fRetroQuant[8] is passed out in bags.
  bags = sf * (in/in per ft) * (lb/cf cellulose) * (bags/lb)
  (Conversion numbers came from page 92.19 of SH Notes.)
  *********************/

  ASSERT(bagsize != 0, sprintf(msg, "Assertion Failure"));

  fSpaceComp = mir->fRofInsAirSpace + mir->fThicknessCeilIns * (1.0f - Densexist / density);

  if (mdi->rof.roof_type == RT_BOWSTRING) {

    Dins = mir->fThicknessCeilIns;
    VExist = (float)(Dins * Area / 12.0 * Densexist / density);
    VTot = (float)(Area * (fHeightRoofPeak + fDedge) / 24.0);
    VDome = VTot - VExist;
    VFrm = (float)(TrussH * TrussW / 144. * Area * 24. / TrussSep);
    VIns = VDome - VFrm - fDuctVolume;
    quant = VIns * density / bagsize;
    mdi->rof.loose_insl = fSpaceComp + mir->fRofThicknessLFGIns * Densexist / density;
  }

  /*******************
  NW, ERG 7/16/94 - Apply adjustment for framing and duct volume to
  reduce insulatable volume in roof.  Assume frame factor of 10%.
  ********************/

  else if (mdi->rof.roof_type == RT_FLAT) {
    quant = (float)((((fARoof * 0.9) * (fSpaceComp / 12.0)) - fDuctVolume) * density / bagsize);
    mdi->rof.loose_insl = fSpaceComp + mir->fRofThicknessLFGIns * Densexist / density;
  }

  else { // Pitched
    quant = (float)((((fARoof * 0.9) * (mir->fRofInsAirSpace / 12.0)) - fDuctVolume) * density / bagsize);
    mdi->rof.loose_insl += mir->fRofInsAirSpace;
  }

  if ((quant - (int)(quant)) > 0.10)
    quant = (float)((int)(quant + 1));
  else
    quant = (float)((int)(quant));

  /****************************
  Make the retrofit insulation thickness equal to the insulatable
  air space minus the thickness of any existing insulation.
  Adjustment for duct losses are made in the infiltration,
  heating and cooling consumption functions.
  *****************************/

  mir->flgRetrofits[ndx] = TRUE;
  STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
  STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

  initialize_fuel_types();
  //STRCPY(mir->Results[mir->Rndx].sName, "Roof Fiberglass Loose");
  mir->Results[mir->Rndx].fQuant = quant;

  additional_cost(mdi->rof.add_cost);

  mir->Results[mir->Rndx].measure_id = ndx;
  mir->Results[mir->Rndx].audit_section_id = M_CEILING;
  mir->Results[mir->Rndx].material_id = iMat;

  mir->Rndx++;

  return;
}

/******************* FUNCTION NAME: retro_insulate_roof_fiberglass_add ***/
/**         DATE:  Dec 1993                                            **/
/**           BY:  NLW              SLF                                **/
/**  DESCRIPTION:  Called by the ...Retrofits() functions.             **/
/************************************************************************/
void retro_insulate_roof_fiberglass_add(void) {
  int ndx =  M_CMS_ROOF_FIBERGLASS_LOOSE_INSL_ADD;
  int iMat = M_MAT_ROOF_FIBERGLASS_LOOSE_INSL_ADD;

  float density;
  float bagsize = mdi->key.bag_size_for_loose_fiberglass_insulation;
  float fARoof;
  float Densexist; // Density of existing insulation
  float quant;
  //float fDepthChange; // Change in depth of insulation which can be added
  //   due to compression
  float fSpaceComp; // Space in flat roof for insulation after compression

  mir->flgRetrofits[ndx] = FALSE;

  fARoof = mdi->afl.width * mdi->afl.length;

  // change to >= 2.0 inches from just > 2.0 inches due to the new
  // logic for pitched roofs returning exactly 2.0 as the insulatable space

  if (mir->flgRetrofits[M_CMS_ROOF_CELLULOSE_LOOSE_INSL_ADD] ||
    mir->fARFInsAirSpace < 2.0 || 
    fARoof == 0.0)
    return;

  // Set existing density equal to density of majority of existing insulation
  // batt or blown.  7/03  MBG

  if (mir->fRofThicknessBattIns_Add > mir->fRofThicknessLFGIns_Add)
    Densexist = mir->fDensExistBatInsul;
  else
    Densexist = DENSITY_EXIST_FG_INSUL;

  // Set densities of existing and installed insulation. Note, DeSoto's
  // original equations assumed 0.9 lb/cuft. This assumes existing is FG!
  //   MBG 6/03
  // Assume denspack insulation for all but pitched roof type

  if (mdi->awl.wall_config == WC_FLAT) {
    density = mdi->key.density_of_loose_fiberglass_insulation;
    //fDepthChange = mir->fThicknessCeilIns_Add * (1.0f - Densexist / density);
  } else {
    density = DENSITY_EXIST_FG_INSUL;
    //fDepthChange = 0.0f;
  }

  /********************
  SLF 7/14/94 - Changed per Beta reviewer comments.
  fRetroQuant[8] is passed out in bags.
  bags = sf * (in/in per ft) * (lb/cf cellulose) * (bags/lb)
  (Conversion numbers came from page 92.19 of SH Notes.)
  ********************/

  ASSERT(bagsize != 0, sprintf(msg, "Assertion Failure"));

  fSpaceComp = mir->fARFInsAirSpace + mir->fThicknessCeilIns_Add * (1.0f - Densexist / density);

  if (mdi->awl.wall_config == WC_FLAT) {
    quant = (float)(((fARoof * 0.9) * (fSpaceComp / 12.0)) * density / bagsize);
    mdi->arf.loose_insl = fSpaceComp + mir->fRofThicknessLFGIns_Add * Densexist / density;
  }

  else { // Pitched
    quant = (float)(((fARoof * 0.9) * (mir->fARFInsAirSpace / 12.0)) * density / bagsize);
    mdi->arf.loose_insl += mir->fARFInsAirSpace;
  }

  if ((quant - (int)(quant)) > 0.10)
    quant = (float)((int)(quant + 1));
  else
    quant = (float)((int)(quant));

  mir->flgRetrofits[ndx] = TRUE;
  STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
  STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

  initialize_fuel_types();
  //STRCPY(mir->Results[mir->Rndx].sName, "Roof Fiberglass Loose [Addition]");
  mir->Results[mir->Rndx].fQuant = quant;
  //STRCPY(mir->Results[mir->Rndx].sMaterial, "Roof Fiberglass loose insl [Addition]");

  additional_cost(mdi->arf.add_cost);

  mir->Results[mir->Rndx].measure_id = ndx;
  mir->Results[mir->Rndx].audit_section_id = M_CEILING_ADDITION;
  mir->Results[mir->Rndx].material_id = iMat;

  mir->Rndx++;

  return;
}

/******************* FUNCTION NAME: retro_skirt ***************************/
/**         DATE:  Dec 1993                                             **/
/**           BY:  NLW,SLF                                              **/
/**  DESCRIPTION:  Called by the ...Retrofits() functions.              **/
/** Skirting will be applied if belly insulation is toggled off.        **/
/** Air film in floor calculation is adjusted.                          **/
/*************************************************************************/
void retro_skirt(void) {
  int ndx =  M_CMS_ADD_SKIRTING;
  int iMat = M_MAT_ADD_SKIRTING;

  float fSkirtLength;

  mir->flgRetrofits[ndx] = FALSE;

  fSkirtLength = (float)((2.0 * mdi->gnl.width) + (2.0 * mdi->gnl.length));

  // only if there is no existing skirt (this is an added skirt measure)
  // and we are not doing belly insulation

  if (mdi->flr.skirt == YES ||
      mir->flgRetrofits[M_CMS_BELLY_CELLULOSE_LOOSE_INSL] ||
      mir->flgRetrofits[M_CMS_BELLY_FIBERGLASS_LOOSE_INSL])
    return;

  /********************
  SLF 7/14/94 - Changed per Beta reviewer comments.
  Material/Labor costs are given per sf of 3 ft. tall skirting.
  fRetroQuant[10] is passed out in sf.
  ********************/

  mdi->flr.skirt = YES;

  // if we are going to end up doing this added skirting to the
  // addition, then lets subtract the length of the common wall
  // so the appropriate quatities show up for the main house and
  // addition instances of this measure, MJF 9/01

  if ((mdi->afl.length * mdi->afl.width) != 0.0 && mdi->afl.skirt != YES)
    fSkirtLength -= mdi->afl.length;

  mir->flgRetrofits[ndx] = TRUE;
  STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
  STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

  initialize_fuel_types();
  //STRCPY(mir->Results[mir->Rndx].sName, "Add Skirting");
  mir->Results[mir->Rndx].fQuant = (float)(fSkirtLength * 3.0);

  mir->Results[mir->Rndx].measure_id = ndx;
  mir->Results[mir->Rndx].audit_section_id = M_FLOOR;
  mir->Results[mir->Rndx].material_id = iMat;

  mir->Rndx++;

  return;
}

/******************* FUNCTION NAME: retro_skirt_add ***********************/
/**         DATE:  Dec 1993                                             **/
/**           BY:  NLW,SLF                                              **/
/**  DESCRIPTION:  Called by the ...Retrofits() functions.              **/
/** Skirting will be applied if belly insulation is toggled off.        **/
/** Air film in floor calculation is adjusted.                          **/
/*************************************************************************/
void retro_skirt_add(void) {
  int ndx  = M_CMS_ADD_SKIRTING_ADD;
  int iMat = M_MAT_ADD_SKIRTING_ADD;

  mir->flgRetrofits[ndx] = FALSE;

  // only if there is no existing skirt (this is an added skirt measure)
  // and we are not doing belly insulation

  // if there is an addition area and no skirt, then add that
  // as a separate instance of this measure.  MJF 9/01 Added the
  // conditional tests to skip this measure on the addition if we
  // have done either of the belly fill measures -- which is assumed
  // to be applied to the available underfloor space in the addition
  // as well.

  // Issue #115, the skirt boolean is not available for additions.  
  // Use exposed floor as a proxy for missing skirting in addtion
  // MJF 1/2020

  if ((mdi->afl.length * mdi->afl.width) == 0.0 ||
      mdi->afl.add_floor_type != EXPOSEDFLOOR ||
      mir->flgRetrofits[M_CMS_BELLY_CELLULOSE_LOOSE_INSL_ADD] ||
      mir->flgRetrofits[M_CMS_BELLY_FIBERGLASS_LOOSE_INSL_ADD])
    return;

  /********************
  SLF 7/14/94 - Changed per Beta reviewer comments.
  Material/Labor costs are given per sf of 3 ft. tall skirting.
  fRetroQuant[10] is passed out in sf.
  ********************/

  mdi->afl.add_floor_type = CRAWL;      // no longer an exposed floor adding skirting

  // note the addition of the afl.length to the skirting length
  // for this instance of the measure for the addition.

  mir->flgRetrofits[ndx] = TRUE;
  STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
  STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

  initialize_fuel_types();
  //STRCPY(mir->Results[mir->Rndx].sName, "Add Skirting [Addition]");
  mir->Results[mir->Rndx].fQuant = (float)((2.0 * mdi->afl.width + mdi->afl.length) * 3.0);

  mir->Results[mir->Rndx].measure_id = ndx;
  mir->Results[mir->Rndx].audit_section_id = M_FLOOR_ADDITION;
  mir->Results[mir->Rndx].material_id = iMat;

  mir->Rndx++;

  return;
}

/******************* FUNCTION NAME: retro_white_coat **********************/
/**         DATE:  Dec 1993                                             **/
/**           BY:  NLW, SLF                                             **/
/**  DESCRIPTION:  Called by the ...Retrofits() functions.              **/
/*************************************************************************/
void retro_white_coat(void) {
  int ndx  = M_CMS_WHITE_ROOF_COATING;
  int iMat = M_MAT_WHITE_ROOF_COATING;

  float fARoof;

  // MJF 9/01 special note, prior versions had no way of clearing
  // the flgNoClg which was set = 1 above -- which means that this
  // retrofit option was ALWAYS skipped....

  mir->flgRetrofits[ndx] = FALSE;

  // skip the addition if you are not doing
  // the white coating on the main home, note
  // that we do not consider this retrofit if
  // the home has a pitched roof

  // #370 do not exclude pitched roofs as they are likel low slope typical manfactured housing construction
  // if (mdi->rof.roof_color == RC_LIGHT || 
  //   mdi->rof.roof_type == RT_PITCHED || 
  //   mir->flgNoCLG == TRUE) // no primary cooling sys
  //   return;

  if (mdi->rof.roof_color == RC_LIGHT || // already have a light colored roof
    mir->flgNoCLG == TRUE) // no primary cooling sys
    return;

  fARoof = mdi->gnl.width * mdi->gnl.length;    // assume no overhangs
  switch (mdi->rof.roof_type){
    case RT_BOWSTRING:
      fARoof *= 1.05F;
      break;
    case RT_PITCHED:
      fARoof *= MOBILE_ROOF_TO_ATTIC_AREA_RATIO;
      break;
    default:
      break;
  }

  mdi->rof.roof_color = RC_LIGHT;

  mir->flgRetrofits[ndx] = TRUE;
  STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
  STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

  initialize_fuel_types();
  //STRCPY(mir->Results[mir->Rndx].sName, "White Coat Roof");
  mir->Results[mir->Rndx].fQuant = fARoof;

  mir->Results[mir->Rndx].measure_id = ndx;
  mir->Results[mir->Rndx].audit_section_id = M_CEILING;
  mir->Results[mir->Rndx].material_id = iMat;

  mir->Rndx++;

  return;
}

/******************* FUNCTION NAME: retro_white_coat_add ******************/
/**         DATE:  Dec 1993                                             **/
/**           BY:  NLW, SLF                                             **/
/**  DESCRIPTION:  Called by the ...Retrofits() functions.              **/
/*************************************************************************/
void retro_white_coat_add(void) {
  int ndx  = M_CMS_WHITE_ROOF_COATING_ADD;
  int iMat = M_MAT_WHITE_ROOF_COATING_ADD;

  float fARoof;

  mir->flgRetrofits[ndx] = FALSE;

  // no overhangs, stick built addition with pitched roof assumed #370
  fARoof = mdi->afl.length * mdi->afl.width * STICK_BUILT_ROOF_TO_ATTIC_AREA_RATIO;

  if (fARoof == 0.0 || 
    mdi->arf.roof_color == RC_LIGHT || 
    mir->flgNoCLG == TRUE)
    return;

  mdi->arf.roof_color = RC_LIGHT;

  mir->flgRetrofits[ndx] = TRUE;
  STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
  STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

  initialize_fuel_types();
  //STRCPY(mir->Results[mir->Rndx].sName, "White Coat Roof [Addition]");
  mir->Results[mir->Rndx].fQuant = fARoof;

  mir->Results[mir->Rndx].measure_id = ndx;
  mir->Results[mir->Rndx].audit_section_id = M_CEILING_ADDITION;
  mir->Results[mir->Rndx].material_id = iMat;

  mir->Rndx++;

  return;
}

// The same core retrofit logic applies to both main home doors and doors in the addtion
// furthermore the M_DOR struct is used for both
static void retro_door_each(MDI *original, M_DOR *door, int measure_id, int audit_section_id, int material_id){

  //char name[256];
  //STRCPY(name, measure_name);

  //#256
  //if (door->replace == YES)
  //  STRCAT(name, " Requried");

  if (mir->flgWhichPass == CUMULATIVE) {
    if (!comma_delimited_strstr(mir->sComponents, door->code))
      return;
  } 

  //Get the number of doors
  float fNumDorTotal = door->num_n + door->num_s + door->num_e + door->num_w;

  if (fNumDorTotal > 0.0) {

    append_component_code_to_current_results(door->code);

    // Specify the door improvements in the audit
    door->door_type = REPLACEMENT;
    door->leakiness = LEAK_TIGHT;     // #245
    door->leak_coef = door_leakage_coef(door->leakiness);

    /***********************************
    MBG 10/2/08 - Call mhea_energy_use() to compute the energy
    saved by a single door description.
    ***********************************/

    mhea_energy_use();
    save_energy_results();

    mir->flgRetrofits[measure_id] = TRUE;
    STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[measure_id].measure_name);
    STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[material_id].retro_name);

    initialize_fuel_types();
    //STRCPY(mir->Results[mir->Rndx].sName, name);
    mir->Results[mir->Rndx].fQuant += fNumDorTotal;

    mir->Results[mir->Rndx].measure_id = measure_id;
    mir->Results[mir->Rndx].audit_section_id = audit_section_id;
    mir->Results[mir->Rndx].material_id = material_id;

    additional_cost(door->cost_replace * fNumDorTotal);

    mhea_measure_sir(mir->Rndx);            // assign the BCR
    mir->Results[mir->Rndx].flgGetBCR = 1;  // Set global flag so that measure will receive 0.6 adjustment

    if (door->replace == YES) {
      mir->Results[mir->Rndx].measure_required = TRUE; 
      if (door->inc_sir == TRUE)      
        mir->Results[mir->Rndx].measure_priority = MPS_ENVELOPE_REQUIRED; // include cost in SIR
      else
        mir->Results[mir->Rndx].measure_priority = MPS_ENVELOPE_REQUIRED_NO_SIR; // don't include in SIR
    } else {
      mir->Results[mir->Rndx].measure_required = FALSE; 
      mir->Results[mir->Rndx].measure_priority = MPS_SIR;
    }

    if (mir->Results[mir->Rndx].fQuant > 0.0)
      mir->Rndx++;
  }
    return;
}

/******************* FUNCTION NAME: retro_door ****************************/
/**         DATE:  Dec 1993                                             **/
/**           BY:  NW                                                   **/
/**      REVISED:  12/29/98 MJF                                         **/
/**  DESCRIPTION:  Called by the ...Retrofits() function                **/
/** Replace wood doors, if present, with standard mobile home door      **/
/**     MODIFIED:  10/08 MBG Separate routines for MH and Addition      **/
/**                 and compute BCR internal to routine to allow each   **/
/**                 door component to be considered individually to     **/
/**                 allow individual additional costs.                  **/
/*************************************************************************/
void retro_door(void) {
  int ndx = M_CMS_REPLACE_DOORS;
  MDI *original = NULL;

  mir->flgRetrofits[ndx] = FALSE;

  if(mir->flgWhichPass != CUMULATIVE){
    copy_mdi(&original, mdi); // Save original mdi structure, allocating memory for original
  }

  for (int i = 0; i < mdi->num_dor; i++) {
    if (mir->flgWhichPass == FIRST_PASS)
      copy_mdi(&mdi, original); // Get copy of original mdi
    retro_door_each(original, &mdi->dor[i], ndx, M_DOOR, M_MAT_REPLACE_DOORS);
  }

  if(mir->flgWhichPass != CUMULATIVE){
    copy_mdi(&mdi, original); // back to original MDI structure
    free(original);           // free memory allocated in this procedure
  }
  return;
}

/******************* FUNCTION NAME: retro_door_add ****************************/
/**         DATE:  Dec 1993                                             **/
/**           BY:  NW                                                   **/
/**      REVISED:  12/29/98 MJF                                         **/
/**  DESCRIPTION:  Called by the ...Retrofits() function                **/
/**  Replace wood doors, if present, with standard mobile home door      **/
/**     MODIFIED:  10/08 MBG Separate routines for MH and Addition      **/
/**                 and compute BCR internal to routine to allow each   **/
/**                 door component to be considered individually to     **/
/**                 allow individual additional costs.                  **/
/*************************************************************************/
void retro_door_add(void) {
  int ndx = M_CMS_REPLACE_DOORS_ADD;
  MDI *original = NULL;

  mir->flgRetrofits[ndx] = FALSE;

  if(mir->flgWhichPass != CUMULATIVE){
    copy_mdi(&original, mdi); // Save original mdi structure, allocating memory for original
  }

  for (int i = 0; i < mdi->num_adr; i++) {
    if (mir->flgWhichPass == FIRST_PASS)
      copy_mdi(&mdi, original); // Get copy of original mdi
    retro_door_each(original, &mdi->adr[i], ndx, M_DOOR_ADDITION, M_MAT_REPLACE_DOORS_ADD);
  }

  if(mir->flgWhichPass != CUMULATIVE){
    copy_mdi(&mdi, original); // back to original MDI structure
    free(original);           // free memory allocated in this procedure
  }
  return;
}

/******************* FUNCTION NAME: retro_storm_door ***********************/
/**         DATE:  Dec 1993                                             **/
/**           BY:  NW                                                   **/
/**      REVISED:  12/29/98 MJF                                         **/
/**  DESCRIPTION:  Called by the ...Retrofits() functions.              **/
/** Add storm doors if not present to all doors                         **/
/*************************************************************************/
void retro_storm_door(void) {
  int ndx  = M_CMS_STORM_DOORS;
  int iMat = M_MAT_STORM_DOORS;

  float fNumDorTotal = 0.0;

  mir->flgRetrofits[ndx] = FALSE;

  for (int i = 0; i < mdi->num_dor; i++) {
    /*************************
    Determine if storm door is present.  If not, add to total and
    reset flag to indicate presence of retrofit storm door.
    **************************/
    if (mdi->dor[i].storm != YES) {
      /***************************
      Get the number of doors
      **************************/

      fNumDorTotal = mdi->dor[i].num_n + mdi->dor[i].num_s + mdi->dor[i].num_e + mdi->dor[i].num_w;

      append_component_code_to_current_results(mdi->dor[i].code);

      if (fNumDorTotal > 0.0) {

        mdi->dor[i].storm = YES;

        mir->flgRetrofits[ndx] = TRUE;
        STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
        STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

        initialize_fuel_types();
        //STRCPY(mir->Results[mir->Rndx].sName, "Storm Doors");
        mir->Results[mir->Rndx].fQuant += fNumDorTotal;

        mir->Results[mir->Rndx].measure_id = ndx;
        mir->Results[mir->Rndx].audit_section_id = M_DOOR;
        mir->Results[mir->Rndx].material_id = iMat;

      }
    }
  }

  // if we have implemented this measure in the main part of
  // the house, then increment our instance counter

  if (mir->Results[mir->Rndx].fQuant > 0.0)
    mir->Rndx++;

  return;
}

/******************* FUNCTION NAME: retro_storm_door_add *******************/
/**         DATE:  Dec 1993                                             **/
/**           BY:  NW                                                   **/
/**      REVISED:  12/29/98 MJF                                         **/
/**  DESCRIPTION:  Called by the ...Retrofits() functions.              **/
/** Add storm doors if not present to all doors                         **/
/*************************************************************************/
void retro_storm_door_add(void) {
  int ndx  = M_CMS_STORM_DOORS_ADD;
  int iMat = M_MAT_STORM_DOORS_ADD;

  float fNumDorTotal = 0.0;

  mir->flgRetrofits[ndx] = FALSE;

  // Ditto for the doors in any additions

  for (int i = 0; i < mdi->num_adr; i++) {
    /*************************
    Determine if storm door is present.  If not, add to total and
    reset flag to indicate presence of retrofit storm door.
    **************************/
    if (mdi->adr[i].storm != YES) {
      /***************************
      Get the number of doors
      **************************/

      fNumDorTotal = mdi->adr[i].num_n + mdi->adr[i].num_s + mdi->adr[i].num_e + mdi->adr[i].num_w;

      append_component_code_to_current_results(mdi->adr[i].code);

      if (fNumDorTotal > 0.0) {

        mdi->adr[i].storm = YES;

        mir->flgRetrofits[ndx] = TRUE;
        STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
        STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

        initialize_fuel_types();
        //STRCPY(mir->Results[mir->Rndx].sName, "Storm Doors [Addition]");
        mir->Results[mir->Rndx].fQuant += fNumDorTotal;

        mir->Results[mir->Rndx].measure_id = ndx;
        mir->Results[mir->Rndx].audit_section_id = M_DOOR_ADDITION;
        mir->Results[mir->Rndx].material_id = iMat;

      }
    }
  }

  // if we have implemented this measure in the addition to
  // the house, then increment our instance counter

  if (mir->Results[mir->Rndx].fQuant > 0.0)
    mir->Rndx++;

  return;
}

/******************* FUNCTION NAME: retro_window  *************************/
/**         DATE:  Dec 1993                                             **/
/**           BY:  NLW                                                  **/
/**      REVISED:  12/29/98 MJF                                         **/
/**  DESCRIPTION:  Called by the ...Retrofits() functions.              **/
/** Adds up the area of windows that will be replaced.                  **/
/*************************************************************************/
void retro_window(void) {
  int ndx  = M_CMS_REPLACE_WINDOWS;
  int iMat = M_MAT_REPLACE_WINDOWS;

  float fNumWinTotal;
  MDI *original = NULL;

  mir->flgRetrofits[ndx] = FALSE;

  if(mir->flgWhichPass != CUMULATIVE){
    copy_mdi(&original, mdi); // Save original mdi structure, allocating memory for original
  }

  /***************
  Deterimine the infiltration savings from the window replacement measure
  separately so that it can be subtracted from the general infiltration
  savings during the final tally of savings. Do not permanently change
  the window characteristics in this loop.
  ***************/

  if (mir->flgWhichPass == CUMULATIVE && mir->Rndx != 0) {

    /****************************************************************************
    First loop through window descriptions to determe air leakage reduction,
    Cumulative pass for specific window related to instance of measure.
    ****************************************************************************/

    for (int i = 0; i < mdi->num_win; i++) {
      if (!comma_delimited_strstr(mir->sComponents, mdi->win[i].code))
        continue;
      else if (mdi->win[i].imeas_applied != 0)
        continue; // Prevent more than one measure per window MBG 4/07
      else {
        /***************************
        Get the total number of windows of this window type.
        **************************/
        fNumWinTotal = mdi->win[i].num_n + mdi->win[i].num_s + mdi->win[i].num_e + mdi->win[i].num_w;

        if (fNumWinTotal > 0.0) {

          /*************************
          Reset window leakage rating
          *************************/
          float save_leak_coef = mdi->win[i].leak_coef;
          mdi->win[i].leak_coef = window_leakage_coef(MEC_REPLACEMENT);

          /***********************************
          MBG 3/11/04 - Call mhea_energy_use() first with only window
          leakage changed in order to determine energy savings
          which should be attributed to this measure instead of
          general infiltration reduction.
          ***********************************/

          mir->flgWhichPass = NOT_BASE_CASE;
          mhea_energy_use();
          mir->flgWhichPass = CUMULATIVE;
          // mir->fInfMeasEnrgyHtg[ndx] += mir->Results[mir->Rndx - 1].fEnerPstHtg - mir->fHeating_Energy * mir->fAdj_Htg;
          // mir->fInfMeasEnrgyClg[ndx] += mir->Results[mir->Rndx - 1].fEnerPstClg - mir->fCooling_Energy * mir->fAdj_Clg;
          mir->fInfMeasEnrgyHtg[ndx] += mir->fPre_Heating - mir->fHeating_Energy * mir->fAdj_Htg;
          mir->fInfMeasEnrgyClg[ndx] += mir->fPre_Cooling - mir->fCooling_Energy * mir->fAdj_Clg;
          mdi->win[i].leak_coef = save_leak_coef;
        }
      }
    } // End loop through window descriptions

    if (cmds.debug_level & D_MHEA_WIN_DETAIL) {
      //fprintf(stderr, "fEnerPreHtg(kBtu)   = %6.2f\n", mir->Results[mir->Rndx - 1].fEnerPstHtg/1000);
      fprintf(stderr, "fEnerPreHtg(kBtu)      = %6.2f\n", mir->fPre_Heating/1000);
      fprintf(stderr, "fCurrent_Heating(kBtu) = %6.2f\n", mir->fHeating_Energy/1000);
      fprintf(stderr, "InfEnrgyHtgSaved(kBtu) = %6.2f\n", mir->fInfMeasEnrgyHtg[ndx]/1000);
      fprintf(stderr, "InfEnrgyClgSaved(kBtu) = %6.2f\n", mir->fInfMeasEnrgyClg[ndx]/1000);
    }
  }

  /****************************************
  Second loop through window descriptions
  ****************************************/

  for (int i = 0; i < mdi->num_win; i++) {
    if (mir->flgWhichPass == CUMULATIVE) {
      /*************************
      If cumulative pass, execute for only window description associated
      with instance of measure
      **************************/

      if (!comma_delimited_strstr(mir->sComponents, mdi->win[i].code))
        continue;
      if (mdi->win[i].imeas_applied != 0)
        continue; // Prevent more than one measure per window MBG 4/07
    }

    else { // mir->flgWhichPass = FIRST_PASS

      /*************************
      Get the type of window (flat window, bay window, etc.) and the type
      of glazing (double, single, etc.).  If it is not a sliding glass
      door or door window, then this retrofit will be implemented for
      that window.  Implicit in this test is that any prior storm window
      retrofits will have changed the window glazing type
      **************************/

      if (mdi->win[i].window_type == GLASSDOORMHEA || mdi->win[i].window_type == DOORWINMHEA ||
          mdi->win[i].window_type == SKYLIGHTMHEA) {
        if (mdi->win[i].retrofit_option == MWS_REPLACE) {
          char buffer[MESSAGE_LEN];
          sprintf(buffer, "A required window replacement measure was SKIPPED for window: %s because it can not be applied to window type: %d", mdi->win[i].code, mdi->win[i].window_type);
          add_mhea_message(buffer);
        }
        continue;
      }
      else if (mdi->win[i].glazing_type != GT_SINGLE && mdi->win[i].retrofit_option != MWS_REPLACE)
        continue;
      else if (mdi->win[i].retrofit_option != MWS_EVALUATE_ALL && mdi->win[i].retrofit_option != MWS_REPLACE)
        continue;

    } // End mir->flgWhichPass = FIRST_PASS only execution

    append_component_code_to_current_results(mdi->win[i].code);

    /***************************
    Get the total number of windows of this window type.
    **************************/
    fNumWinTotal = mdi->win[i].num_n + mdi->win[i].num_s + mdi->win[i].num_e + mdi->win[i].num_w;

    if (fNumWinTotal > 0.0) {

      if (mir->flgWhichPass == FIRST_PASS)
        copy_mdi(&mdi, original); // Get copy of original mdi

      /*************************
      Reset glass type and leakage for retrofit window
      *************************/
      mdi->win[i].glazing_type = GT_DBL;
      mdi->win[i].leak_coef = window_leakage_coef(MEC_REPLACEMENT);

      /***********************************
      MBG 3/11/04 - Call mhea_energy_use() to compute the energy
      saved by a single window description.
      ***********************************/

      mhea_energy_use();
      save_energy_results();

      if (mir->flgWhichPass == CUMULATIVE) {
        // Prevent multiple retrofit measures from being applied to the same window  MBG 4/07
        mdi->win[i].imeas_applied = M_CMS_REPLACE_WINDOWS;    
      }

      mir->flgRetrofits[ndx] = TRUE;
      STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
      STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

      initialize_fuel_types();
      //STRCPY(mir->Results[mir->Rndx].sName, "Window Replacement");

      // unit inches is units of price calculation

      mir->Results[mir->Rndx].fQuant += (mdi->win[i].width + mdi->win[i].height) * fNumWinTotal;

      mir->Results[mir->Rndx].measure_id = ndx;
      mir->Results[mir->Rndx].audit_section_id = M_WINDOW;
      mir->Results[mir->Rndx].material_id = iMat;

      // the materials report is in units of window count not UI

      mir->Results[mir->Rndx].fReportQuant += fNumWinTotal;
      STRCPY(mir->Results[mir->Rndx].sReportUnits, "Each");

      additional_cost(mdi->win[i].cost_replace * fNumWinTotal);

      mhea_measure_sir(mir->Rndx); // assign the BCR
      mir->Results[mir->Rndx].flgGetBCR = 1;
      // Set global flag so that measure will receive 0.6 adjustment

      // If measure chosen from retrofit options combo box,
      // make it required.

      if (mdi->win[i].retrofit_option == MWS_REPLACE) {
        mir->Results[mir->Rndx].measure_required = TRUE;
        if (mdi->win[i].inc_sir == TRUE)
          mir->Results[mir->Rndx].measure_priority = MPS_ENVELOPE_REQUIRED;
        else
          mir->Results[mir->Rndx].measure_priority = MPS_ENVELOPE_REQUIRED_NO_SIR;
      }

      // if we have implemented this measure in the main part of
      // the house, then increment our instance counter

      if (mir->Results[mir->Rndx].fQuant > 0.0)
        mir->Rndx++;
      //if (WeatherData) {free(WeatherData); WeatherData = NULL;}
    }

  } // End loop through window descriptions

  // GKA/MJF Issue #83
  if(mir->flgWhichPass != CUMULATIVE){
    copy_mdi(&mdi, original); // back to original MDI structure
    free(original);           // free memory allocated in this procedure
  }

  return;
}

/******************* FUNCTION NAME: retro_window_add **********************/
/**         DATE:  Dec 1993                                             **/
/**           BY:  NLW                                                  **/
/**      REVISED:  12/29/98 MJF                                         **/
/**  DESCRIPTION:  Called by the ...Retrofits() functions.              **/
/** Adds up the area of windows that will be replaced.                  **/
/*************************************************************************/
void retro_window_add(void) {
  int ndx  = M_CMS_REPLACE_WINDOWS_ADD;
  int iMat = M_MAT_REPLACE_WINDOWS_ADD;

  float fNumWinTotal;
  MDI *original = NULL;

  mir->flgRetrofits[ndx] = FALSE;

  if(mir->flgWhichPass != CUMULATIVE){
    copy_mdi(&original, mdi); // Save original mdi structure, allocating memory for original
  }

  /***************
  Deterimine the infiltration savings from the window replacement measure
  separately so that it can be subtracted from the general infiltration
  savings during the final tally of savings. Do not permanently change
  the window characteristics in this loop.
  ***************/

  if (mir->flgWhichPass == CUMULATIVE && mir->Rndx != 0) {

    /****************************************************************************
    First loop through window descriptions to determe air leakage reduction,
    Cumulative pass for specific window related to instance of measure.
    ****************************************************************************/

    for (int i = 0; i < mdi->num_awn; i++) {
      if (!comma_delimited_strstr(mir->sComponents, mdi->awn[i].code))
        continue;
      else if (mdi->awn[i].imeas_applied != 0)
        continue; // Prevent more than one measure per window MBG 4/07
      else {
        /***************************
        Get the total number of windows of this window type.
        **************************/
        fNumWinTotal = mdi->awn[i].num_n + mdi->awn[i].num_s + mdi->awn[i].num_e + mdi->awn[i].num_w;

        if (fNumWinTotal > 0.0) {

          /*************************
          Reset window leakage rating
          *************************/
          float save_leak_coef = mdi->awn[i].leak_coef;
          mdi->awn[i].leak_coef = window_leakage_coef(MEC_REPLACEMENT);

          /***********************************
          MBG 3/11/04 - Call mhea_energy_use() first with only window
          leakage changed in order to determine energy savings
          which should be attributed to this measure instead of
          general infiltration reduction.
          ***********************************/

          mir->flgWhichPass = NOT_BASE_CASE;
          mhea_energy_use();
          mir->flgWhichPass = CUMULATIVE;
          // mir->fInfMeasEnrgyHtg[ndx] += mir->Results[mir->Rndx - 1].fEnerPstHtg - mir->fHeating_Energy * mir->fAdj_Htg;
          // mir->fInfMeasEnrgyClg[ndx] += mir->Results[mir->Rndx - 1].fEnerPstClg - mir->fCooling_Energy * mir->fAdj_Clg;
          mir->fInfMeasEnrgyHtg[ndx] += mir->fPre_Heating - mir->fHeating_Energy * mir->fAdj_Htg;
          mir->fInfMeasEnrgyClg[ndx] += mir->fPre_Cooling - mir->fCooling_Energy * mir->fAdj_Clg;
          mdi->awn[i].leak_coef = save_leak_coef;
        }
      }
    } // End loop through window descriptions

    if (cmds.debug_level & D_MHEA_WIN_DETAIL) {
      //fprintf(stderr, "fEnerPreHtg   = %6.2f\n", mir->Results[mir->Rndx - 1].fEnerPstHtg);
      fprintf(stderr, "fEnerPreHtg(kBtu)      = %6.2f\n", mir->fPre_Heating/1000);
      fprintf(stderr, "fCurrent_Heating(kBtu) = %6.2f\n", mir->fHeating_Energy/1000);
      fprintf(stderr, "InfEnrgyHtgSaved(kBtu) = %6.2f\n", mir->fInfMeasEnrgyHtg[ndx]/1000);
      fprintf(stderr, "InfEnrgyClgSaved(kBtu) = %6.2f\n", mir->fInfMeasEnrgyClg[ndx]/1000);
    }
  }

  /****************************************
  Second loop through window descriptions
  ****************************************/

  for (int i = 0; i < mdi->num_awn; i++) {
    if (mir->flgWhichPass == CUMULATIVE) {
      /*************************
      If cumulative pass, execute for only window description associated
      with instance of measure
      **************************/

      if (!comma_delimited_strstr(mir->sComponents, mdi->awn[i].code))
        continue;
      if (mdi->awn[i].imeas_applied != 0)
        continue; // Prevent more than one measure per window MBG 4/07
    }

    else { // mir->flgWhichPass = FIRST_PASS

      /*************************
      Get the type of window (flat window, bay window, etc.) and the type
      of glazing (double, single, etc.).  If it is not a sliding glass
      door or door window, then this retrofit will be implemented for
      that window.  Implicit in this test is that any prior storm window
      retrofits will have changed the window glazing type
      **************************/

      if (mdi->awn[i].window_type == GLASSDOORMHEA || mdi->awn[i].window_type == DOORWINMHEA ||
          mdi->awn[i].window_type == SKYLIGHTMHEA){
        if (mdi->awn[i].retrofit_option == MWS_REPLACE) {
          char buffer[MESSAGE_LEN];
          sprintf(buffer, "A required window replacement measure was SKIPPED for window: %s because it can not be applied to window type: %d", mdi->awn[i].code, mdi->awn[i].window_type);
          add_mhea_message(buffer);
        }
        continue; 
      }
      else if (mdi->awn[i].glazing_type != GT_SINGLE && mdi->awn[i].retrofit_option != MWS_REPLACE)
        continue;
      else if (mdi->awn[i].retrofit_option != MWS_EVALUATE_ALL && mdi->awn[i].retrofit_option != MWS_REPLACE)
        continue;

    } // End mir->flgWhichPass = FIRST_PASS only execution

    append_component_code_to_current_results(mdi->awn[i].code);

    /***************************
    Get the total number of windows of this window type.
    **************************/
    fNumWinTotal = mdi->awn[i].num_n + mdi->awn[i].num_s + mdi->awn[i].num_e + mdi->awn[i].num_w;

    if (fNumWinTotal > 0.0) {

      if (mir->flgWhichPass == FIRST_PASS)
        copy_mdi(&mdi, original); // Get copy of original mdi

      /*************************
      Reset glass type and leakage for retrofit window
      *************************/
      mdi->awn[i].glazing_type = GT_DBL;
      mdi->awn[i].leak_coef = window_leakage_coef(MEC_REPLACEMENT);

      // New MEC window leakage coefficient

      /***********************************
      MBG 3/11/04 - Call mhea_energy_use() to compute the energy
      saved by a single window description.
      ***********************************/

      mhea_energy_use();
      save_energy_results();

      if (mir->flgWhichPass == CUMULATIVE) {
        // Prevent multiple retrofit measures from being applied to the same window  MBG 4/07
        mdi->awn[i].imeas_applied = M_CMS_REPLACE_WINDOWS_ADD;
      }

      mir->flgRetrofits[ndx] = TRUE;
      STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
      STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

      initialize_fuel_types();
      //STRCPY(mir->Results[mir->Rndx].sName, "Window Replacement [Addition]");

      // unit inches is units of price calculation

      mir->Results[mir->Rndx].fQuant += (mdi->awn[i].width + mdi->awn[i].height) * fNumWinTotal;

      mir->Results[mir->Rndx].measure_id = ndx;
      mir->Results[mir->Rndx].audit_section_id = M_WINDOW_ADDITION;
      mir->Results[mir->Rndx].material_id = iMat;

      // the materials report is in units of window count not UI

      mir->Results[mir->Rndx].fReportQuant += fNumWinTotal;
      STRCPY(mir->Results[mir->Rndx].sReportUnits, "Each");

      additional_cost(mdi->awn[i].cost_replace * fNumWinTotal);

      mhea_measure_sir(mir->Rndx); // assign the BCR
      mir->Results[mir->Rndx].flgGetBCR = 1;
      // Set global flag so that measure will receive 0.6 adjustment

      // If measure chosen from retrofit options combo box,
      // make it required.

      if (mdi->awn[i].retrofit_option == MWS_REPLACE) {
        mir->Results[mir->Rndx].measure_required = TRUE;
        if (mdi->awn[i].inc_sir == TRUE)
          mir->Results[mir->Rndx].measure_priority = MPS_ENVELOPE_REQUIRED;
        else
          mir->Results[mir->Rndx].measure_priority = MPS_ENVELOPE_REQUIRED_NO_SIR;
      }

      // if we have implemented this measure in the main part of
      // the house, then increment our instance counter

      if (mir->Results[mir->Rndx].fQuant > 0.0)
        mir->Rndx++;
      //if (WeatherData) {free(WeatherData); WeatherData = NULL;}
    }

  } // End loop through window descriptions

  // GKA/MJF Issue #83
  if(mir->flgWhichPass != CUMULATIVE){
    copy_mdi(&mdi, original); // back to original MDI structure
    free(original);           // free memory allocated in this procedure
  }

  return;
}

/******************* FUNCTION NAME: retro_plastic_storm ********************/
/**         DATE:  Dec 1993                                             **/
/**           BY:  NLW                                                  **/
/**      REVISED:  12/29/98 MJF                                         **/
/**  DESCRIPTION:  Called by the ...Retrofits() functions.              **/
/** Adds up the area of windows that will be retrofited.                **/
/*************************************************************************/
void retro_plastic_storm(void) {
  int ndx  = M_CMS_PLASTIC_STORM_WINDOWS;
  int iMat = M_MAT_PLASTIC_STORM_WINDOWS;

  float fNumWinTotal;

  MDI *original = NULL;

  mir->flgRetrofits[ndx] = FALSE;

  if(mir->flgWhichPass != CUMULATIVE){
    copy_mdi(&original, mdi); // Save original mdi structure, allocating memory for original
  }

  /***************
  Deterimine the infiltration savings from the plastic storm measure
  separately so that it can be subtracted from the general infiltration
  savings during the final tally of savings. Do not permanently change
  the window characteristics in this loop.
  ***************/

  if (mir->flgWhichPass == CUMULATIVE && mir->Rndx != 0) {

    /****************************************************************************
    First loop through window descriptions to determe air leakage reduction,
    Cumulative pass for specific window related to instance of measure.
    ****************************************************************************/

    for (int i = 0; i < mdi->num_win; i++) {

      if (cmds.debug_level & D_MHEA_WIN_DETAIL){
        fprintf(stderr, "i:%d mir->sComponents: %s\n mdi->win[i].code:%s\n mdi->win[i].imeas_applied:%d\n", i, mir->sComponents, mdi->win[i].code, mdi->win[i].imeas_applied);
      }

      if (!comma_delimited_strstr(mir->sComponents, mdi->win[i].code))
        continue;
      else if (mdi->win[i].imeas_applied != 0)
        continue; // Prevent more than one measure per window  MBG 4/07
      else {
        /***************************
        Get the total number of windows of this window type.
        **************************/

        fNumWinTotal = mdi->win[i].num_n + mdi->win[i].num_s + mdi->win[i].num_e + mdi->win[i].num_w;

        if (fNumWinTotal > 0.0) {
          /*************************
          Reset window leakiness
          *************************/
          float save_leak_coef = mdi->win[i].leak_coef;
          mdi->win[i].leak_coef = (float)((POWC((1 / (POWC(1 / save_leak_coef, 1.25) + 27.77)), 0.8)));

          /***********************************
          MBG 1/10/05 - Call mhea_energy_use() first with only window
          leakage changed in order to determine energy savings
          which should be attributed to this measure instead of
          general infiltration reduction.
          ***********************************/

          mir->flgWhichPass = NOT_BASE_CASE;
          mhea_energy_use();
          mir->flgWhichPass = CUMULATIVE;
          // mir->fInfMeasEnrgyHtg[ndx] += mir->Results[mir->Rndx - 1].fEnerPstHtg - mir->fHeating_Energy * mir->fAdj_Htg;
          // mir->fInfMeasEnrgyClg[ndx] += mir->Results[mir->Rndx - 1].fEnerPstClg - mir->fCooling_Energy * mir->fAdj_Clg;
          mir->fInfMeasEnrgyHtg[ndx] += mir->fPre_Heating - mir->fHeating_Energy * mir->fAdj_Htg;
          mir->fInfMeasEnrgyClg[ndx] += mir->fPre_Cooling - mir->fCooling_Energy * mir->fAdj_Clg;
          mdi->win[i].leak_coef = save_leak_coef;
        }
      }
    } // End loop through window descriptions

    if (cmds.debug_level & D_MHEA_WIN_DETAIL) {
      //fprintf(stderr, "fEnerPreHtg(kBtu)  = %6.2f\n", mir->Results[mir->Rndx - 1].fEnerPstHtg/1000);
      fprintf(stderr, "fEnerPreHtg(kBtu)      = %6.2f\n", mir->fPre_Heating/1000);
      fprintf(stderr, "fCurrent_Heating(kBtu) = %6.2f\n", mir->fHeating_Energy/1000);
      fprintf(stderr, "InfEnrgyHtgSaved(kBtu) = %6.2f\n", mir->fInfMeasEnrgyHtg[ndx]/1000);
      fprintf(stderr, "InfEnrgyClgSaved(kBtu) = %6.2f\n", mir->fInfMeasEnrgyClg[ndx]/1000);
    }
  }

  /****************************************
  Second loop through window descriptions
  ****************************************/

  for (int i = 0; i < mdi->num_win; i++) {
    if (mir->flgWhichPass == CUMULATIVE) {
      /*************************
      If cumulative pass, execute for only window description associated
      with instance of measure
      **************************/

      if (!comma_delimited_strstr(mir->sComponents, mdi->win[i].code))
        continue;
      if (mdi->win[i].imeas_applied != 0)
        continue; // Prevent more than one measure per window MBG 4/07
    }

    else { // mir->flgWhichPass = FIRST_PASS

      /*************************
      Get the type of window (flat window, bay window, etc.) and the
      type of glazing (double, single, etc.).  If it is not a sliding
      glass door, then this retrofit will be implemented for that window.
      Implicit in this test is that any preceeding storm retrofit will
      have already changed the window type and thus avoid selecting both
      plastic and glass storm windows.
      **************************/

      if (mdi->win[i].retrofit_option != MWS_EVALUATE_ALL && mdi->win[i].retrofit_option != MWS_ADD_PLASTIC_STORM)
        continue;

      if (mdi->win[i].window_type == GLASSDOORMHEA ||
          (mdi->win[i].glazing_type != GT_DBL && mdi->win[i].glazing_type != GT_SINGLE)){
        if (mdi->win[i].retrofit_option == MWS_ADD_PLASTIC_STORM) {
          char buffer[MESSAGE_LEN];
          sprintf(buffer, "A required storm window measure was SKIPPED for window: %s because it can not be applied to the existing window or glazing type", mdi->win[i].code);
          add_mhea_message(buffer);
        }
        continue;
      }

    } // End mir->flgWhichPass = FIRST_PASS only execution

    append_component_code_to_current_results(mdi->win[i].code);

    /***************************
    Get the total number of windows of this window type.
    **************************/
    fNumWinTotal = mdi->win[i].num_n + mdi->win[i].num_s + mdi->win[i].num_e + mdi->win[i].num_w;

    if (fNumWinTotal > 0.0) {

      if (mir->flgWhichPass == FIRST_PASS)
        copy_mdi(&mdi, original); // Get copy of original mdi

      /*************************
      Reset glass type for retrofit window
      *************************/
      if (mdi->win[i].glazing_type == GT_SINGLE) {
        mdi->win[i].glazing_type = GT_SINGLEPLASTIC;
      }

      if (mdi->win[i].glazing_type == GT_DBL) {
        mdi->win[i].glazing_type = GT_DOUBLEPLASTIC;
      }
      float save_leak_coef = mdi->win[i].leak_coef;
      mdi->win[i].leak_coef = (float)((POWC((1 / (POWC(1 / save_leak_coef, 1.25) + 27.77)), 0.8)));

      /***********************************
      MBG 3/11/04 - Call mhea_energy_use() to compute the energy
      saved by a single window description.
      ***********************************/

      mhea_energy_use();
      save_energy_results();

      if (mir->flgWhichPass == CUMULATIVE) {
        // Prevent multiple retrofit measures from being applied to the same window  MBG 4/07
        mdi->win[i].imeas_applied = M_CMS_PLASTIC_STORM_WINDOWS;
      } else {
        mdi->win[i].leak_coef = save_leak_coef;
      }

      initialize_fuel_types();

      mir->flgRetrofits[ndx] = TRUE;
      STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
      STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

      //STRCPY(mir->Results[mir->Rndx].sName, "Plastic Storm Windows");
      mir->Results[mir->Rndx].fQuant += (float)((mdi->win[i].width / 12.0) * (mdi->win[i].height / 12.0) * fNumWinTotal);

      mir->Results[mir->Rndx].measure_id = ndx;
      mir->Results[mir->Rndx].audit_section_id = M_WINDOW;
      mir->Results[mir->Rndx].material_id = iMat;

      mir->Results[mir->Rndx].fReportQuant += fNumWinTotal;
      STRCPY(mir->Results[mir->Rndx].sReportUnits, "Each");

      additional_cost(mdi->win[i].cost_add_plastic_storm * fNumWinTotal);

      mhea_measure_sir(mir->Rndx); // assign the BCR
      mir->Results[mir->Rndx].flgGetBCR = 1;
      // Set global flag so that measure will receive 0.6 adjustment

      // If measure chosen from retrofit options combo box,
      // make it required.

      if (mdi->win[i].retrofit_option == MWS_ADD_PLASTIC_STORM) {
        mir->Results[mir->Rndx].measure_required = TRUE;
        if (mdi->win[i].inc_sir == TRUE)
          mir->Results[mir->Rndx].measure_priority = MPS_ENVELOPE_REQUIRED;
        else
          mir->Results[mir->Rndx].measure_priority = MPS_ENVELOPE_REQUIRED_NO_SIR;
      }

      // if we have implemented this measure in the main part of
      // the house, then increment our instance counter

      if (mir->Results[mir->Rndx].fQuant > 0.0)
        mir->Rndx++;
      //if (WeatherData) {free(WeatherData); WeatherData = NULL;}
    }

  } // End loop through window descriptions

  // GKA/MJF Issue #83
  if(mir->flgWhichPass != CUMULATIVE){
    copy_mdi(&mdi, original); // back to original MDI structure
    free(original);           // free memory allocated in this procedure
  }

  return;
}

/******************* FUNCTION NAME: retro_plastic_storm_add ****************/
/**         DATE:  Dec 1993                                             **/
/**           BY:  NLW                                                  **/
/**      REVISED:  12/29/98 MJF                                         **/
/**  DESCRIPTION:  Called by the ...Retrofits() functions.              **/
/** Adds up the area of windows that will be retrofited.                **/
/*************************************************************************/
void retro_plastic_storm_add(void) {
  int ndx  = M_CMS_PLASTIC_STORM_WINDOWS_ADD;
  int iMat = M_MAT_PLASTIC_STORM_WINDOWS_ADD;

  float fNumWinTotal;
  MDI *original = NULL;

  mir->flgRetrofits[ndx] = FALSE;

  if(mir->flgWhichPass != CUMULATIVE){
    copy_mdi(&original, mdi); // Save original mdi structure, allocating memory for original
  }

  /***************
  Deterimine the infiltration savings from the plastic storm measure
  separately so that it can be subtracted from the general infiltration
  savings during the final tally of savings. Do not permanently change
  the window characteristics in this loop.
  ***************/

  if (mir->flgWhichPass == CUMULATIVE && mir->Rndx != 0) {

    /****************************************************************************
    First loop through window descriptions to determe air leakage reduction,
    Cumulative pass for specific window related to instance of measure.
    ****************************************************************************/

    for (int i = 0; i < mdi->num_awn; i++) {
      if (!comma_delimited_strstr(mir->sComponents, mdi->awn[i].code))
        continue;
      else if (mdi->awn[i].imeas_applied != 0)
        continue; // Prevent more than one measure per window  MBG 4/07
      else {
        /***************************
        Get the total number of windows of this window type.
        **************************/

        fNumWinTotal = mdi->awn[i].num_n + mdi->awn[i].num_s + mdi->awn[i].num_e + mdi->awn[i].num_w;

        if (fNumWinTotal > 0.0) {

          /*************************
          Reset window leakiness
          *************************/
          float save_leak_coef = mdi->awn[i].leak_coef;
          mdi->awn[i].leak_coef = (float)((POWC((1 / (POWC(1 / save_leak_coef, 1.25) + 27.77)), 0.8)));

          /***********************************
          MBG 1/10/05 - Call mhea_energy_use() first with only window
          leakage changed in order to determine energy savings
          which should be attributed to this measure instead of
          general infiltration reduction.
          ***********************************/

          mir->flgWhichPass = NOT_BASE_CASE;
          mhea_energy_use();
          mir->flgWhichPass = CUMULATIVE;
          // mir->fInfMeasEnrgyHtg[ndx] += mir->Results[mir->Rndx - 1].fEnerPstHtg - mir->fHeating_Energy * mir->fAdj_Htg;
          // mir->fInfMeasEnrgyClg[ndx] += mir->Results[mir->Rndx - 1].fEnerPstClg - mir->fCooling_Energy * mir->fAdj_Clg;
          mir->fInfMeasEnrgyHtg[ndx] += mir->fPre_Heating - mir->fHeating_Energy * mir->fAdj_Htg;
          mir->fInfMeasEnrgyClg[ndx] += mir->fPre_Cooling - mir->fCooling_Energy * mir->fAdj_Clg;
          mdi->awn[i].leak_coef = save_leak_coef;
        }
      }
    } // End loop through window descriptions

    if (cmds.debug_level & D_MHEA_WIN_DETAIL) {
      //fprintf(stderr, "fEnerPreHtg(kBtu)  = %6.2f\n", mir->Results[mir->Rndx - 1].fEnerPstHtg/1000);
      fprintf(stderr, "fEnerPreHtg(kBtu)      = %6.2f\n", mir->fPre_Heating/1000);
      fprintf(stderr, "fCurrent_Heating(kBtu) = %6.2f\n", mir->fHeating_Energy/1000);
      fprintf(stderr, "InfEnrgyHtgSaved(kBtu) = %6.2f\n", mir->fInfMeasEnrgyHtg[ndx]/1000);
      fprintf(stderr, "InfEnrgyClgSaved(kBtu) = %6.2f\n", mir->fInfMeasEnrgyClg[ndx]/1000);
    }
  }

  /****************************************
  Second loop through window descriptions
  ****************************************/

  for (int i = 0; i < mdi->num_awn; i++) {
    if (mir->flgWhichPass == CUMULATIVE) {
      /*************************
      If cumulative pass, execute for only window description associated
      with instance of measure
      **************************/

      if (!comma_delimited_strstr(mir->sComponents, mdi->awn[i].code))
        continue;
      if (mdi->awn[i].imeas_applied != 0)
        continue; // Prevent more than one measure per window MBG 4/07
    }

    else { // mir->flgWhichPass = FIRST_PASS

      /*************************
      Get the type of window (flat window, bay window, etc.) and the
      type of glazing (double, single, etc.).  If it is not a sliding
      glass door, then this retrofit will be implemented for that window.
      Implicit in this test is that any preceeding storm retrofit will
      have already changed the window type and thus avoid selecting both
      plastic and glass storm windows.
      **************************/

      if (mdi->awn[i].retrofit_option != MWS_EVALUATE_ALL && mdi->awn[i].retrofit_option != MWS_ADD_PLASTIC_STORM)
        continue;

      if (mdi->awn[i].window_type == GLASSDOORMHEA ||
          (mdi->awn[i].glazing_type != GT_DBL && mdi->awn[i].glazing_type != GT_SINGLE)){
        if (mdi->awn[i].retrofit_option == MWS_ADD_PLASTIC_STORM) {
          char buffer[MESSAGE_LEN];
          sprintf(buffer, "A required storm window measure was SKIPPED for window: %s because it can not be applied to the existing window or glazing type", mdi->awn[i].code);
          add_mhea_message(buffer);
        }
        continue;
      }

    } // End mir->flgWhichPass = FIRST_PASS only execution

    append_component_code_to_current_results(mdi->awn[i].code);

    /***************************
    Get the total number of windows of this window type.
    **************************/
    fNumWinTotal = mdi->awn[i].num_n + mdi->awn[i].num_s + mdi->awn[i].num_e + mdi->awn[i].num_w;

    if (fNumWinTotal > 0.0) {

      if (mir->flgWhichPass == FIRST_PASS)
        copy_mdi(&mdi, original); // Get copy of original mdi

      /*************************
      Reset glass type for retrofit window
      *************************/
      if (mdi->awn[i].glazing_type == GT_SINGLE) {
        mdi->awn[i].glazing_type = GT_SINGLEPLASTIC;
      }

      if (mdi->awn[i].glazing_type == GT_DBL) {
        mdi->awn[i].glazing_type = GT_DOUBLEPLASTIC;
      }

      float save_leak_coef = mdi->awn[i].leak_coef;
      mdi->awn[i].leak_coef = (float)((POWC((1 / (POWC(1 / save_leak_coef, 1.25) + 27.77)), 0.8)));

      /***********************************
      MBG 3/11/04 - Call mhea_energy_use() to compute the energy
      saved by a single window description.
      ***********************************/

      mhea_energy_use();
      save_energy_results();

      if (mir->flgWhichPass == CUMULATIVE) {
        // Prevent multiple retrofit measures from being applied to the same window  MBG 4/07
        mdi->awn[i].imeas_applied = M_CMS_PLASTIC_STORM_WINDOWS_ADD;
      } else {
        mdi->awn[i].leak_coef = save_leak_coef;
      }

      initialize_fuel_types();

      mir->flgRetrofits[ndx] = TRUE;
      STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
      STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

      //STRCPY(mir->Results[mir->Rndx].sName, "Plastic Storm Windows [Addition]");
      mir->Results[mir->Rndx].fQuant += (float)((mdi->awn[i].width / 12.0) * (mdi->awn[i].height / 12.0) * fNumWinTotal);

      mir->Results[mir->Rndx].measure_id = ndx;
      mir->Results[mir->Rndx].audit_section_id = M_WINDOW_ADDITION;
      mir->Results[mir->Rndx].material_id = iMat;

      mir->Results[mir->Rndx].fReportQuant += fNumWinTotal;
      STRCPY(mir->Results[mir->Rndx].sReportUnits, "Each");

      additional_cost(mdi->awn[i].cost_add_plastic_storm * fNumWinTotal);

      mhea_measure_sir(mir->Rndx); // assign the BCR
      mir->Results[mir->Rndx].flgGetBCR = 1;
      // Set global flag so that measure will receive 0.6 adjustment

      // If measure chosen from retrofit options combo box,
      // make it required.

      if (mdi->awn[i].retrofit_option == MWS_ADD_PLASTIC_STORM) {
        mir->Results[mir->Rndx].measure_required = TRUE;
        if (mdi->awn[i].inc_sir == TRUE)
          mir->Results[mir->Rndx].measure_priority = MPS_ENVELOPE_REQUIRED;
        else
          mir->Results[mir->Rndx].measure_priority = MPS_ENVELOPE_REQUIRED_NO_SIR;
      }

      // if we have implemented this measure in the main part of
      // the house, then increment our instance counter

      if (mir->Results[mir->Rndx].fQuant > 0.0)
        mir->Rndx++;
      //if (WeatherData) {free(WeatherData); WeatherData = NULL;}
    }

  } // End loop through window descriptions

  // GKA/MJF Issue #83
  if(mir->flgWhichPass != CUMULATIVE){
    copy_mdi(&mdi, original); // back to original MDI structure
    free(original);           // free memory allocated in this procedure
  }

  return;
}

/******************* FUNCTION NAME: retro_glass_storm ********************/
/**         DATE:  Dec 1993                                           **/
/**           BY:  NLW                                                **/
/**      REVISED:  12/29/98 MJF                                       **/
/**  DESCRIPTION:  Called by the ...Retrofits() functions.            **/
/** Adds up the area of windows that will be retrofited.              **/
/***********************************************************************/
void retro_glass_storm(void) {
  int ndx  = M_CMS_GLASS_STORM_WINDOWS;
  int iMat = M_MAT_GLASS_STORM_WINDOWS;

  float fNumWinTotal;
  MDI *original = NULL;

  mir->flgRetrofits[ndx] = FALSE;

  if(mir->flgWhichPass != CUMULATIVE){
    copy_mdi(&original, mdi); // Save original mdi structure, allocating memory for original
  }

  /***************
  Deterimine the infiltration savings from the glass storm measure
  separately so that it can be subtracted from the general infiltration
  savings during the final tally of savings. Do not permanently change
  the window characteristics in this loop.
  ***************/

  if (mir->flgWhichPass == CUMULATIVE && mir->Rndx != 0) {

    /****************************************************************************
    First loop through window descriptions to determe air leakage reduction,
    Cumulative pass for specific window related to instance of measure.
    ****************************************************************************/

    for (int i = 0; i < mdi->num_win; i++) {
      if (!comma_delimited_strstr(mir->sComponents, mdi->win[i].code))
        continue;
      else if (mdi->win[i].imeas_applied != 0)
        continue; // Prevent more than one measure per window  MBG 4/07
      else {
        /***************************
        Get the total number of windows of this window type.
        **************************/

        fNumWinTotal = mdi->win[i].num_n + mdi->win[i].num_s + mdi->win[i].num_e + mdi->win[i].num_w;

        if (fNumWinTotal > 0.0) {

          /*************************
          Reset window leakiness
          *************************/
          float save_leak_coef = mdi->win[i].leak_coef;
          mdi->win[i].leak_coef = window_storm_leak_coef(save_leak_coef);

          /***********************************
          MBG 1/10/05 - Call mhea_energy_use() first with only window
          leakage changed in order to determine energy savings
          which should be attributed to this measure instead of
          general infiltration reduction.
          ***********************************/

          mir->flgWhichPass = NOT_BASE_CASE;
          mhea_energy_use();
          mir->flgWhichPass = CUMULATIVE;

          // mir->fInfMeasEnrgyHtg[ndx] += mir->Results[mir->Rndx - 1].fEnerPstHtg - mir->fHeating_Energy * mir->fAdj_Htg;
          // mir->fInfMeasEnrgyClg[ndx] += mir->Results[mir->Rndx - 1].fEnerPstClg - mir->fCooling_Energy * mir->fAdj_Clg;
          mir->fInfMeasEnrgyHtg[ndx] += mir->fPre_Heating - mir->fHeating_Energy * mir->fAdj_Htg;
          mir->fInfMeasEnrgyClg[ndx] += mir->fPre_Cooling - mir->fCooling_Energy * mir->fAdj_Clg;
          mdi->win[i].leak_coef = save_leak_coef;
        }
      }
    } // End loop through window descriptions

    if (cmds.debug_level & D_MHEA_WIN_DETAIL) {
      //fprintf(stderr, "fEnerPreHtg(kBtu)  = %6.2f\n", mir->Results[mir->Rndx - 1].fEnerPstHtg/1000);
      fprintf(stderr, "fEnerPreHtg(kBtu)      = %6.2f\n", mir->fPre_Heating/1000);
      fprintf(stderr, "fCurrent_Heating(kBtu) = %6.2f\n", mir->fHeating_Energy/1000);
      fprintf(stderr, "InfEnrgyHtgSaved(kBtu) = %6.2f\n", mir->fInfMeasEnrgyHtg[ndx]/1000);
      fprintf(stderr, "InfEnrgyClgSaved(kBtu) = %6.2f\n", mir->fInfMeasEnrgyClg[ndx]/1000);
    }
  }

  /****************************************
  Second loop through window descriptions
  ****************************************/

  for (int i = 0; i < mdi->num_win; i++) {
    if (mir->flgWhichPass == CUMULATIVE) {
      /*************************
      If cumulative pass, execute for only window description associated
      with instance of measure
      **************************/

      if (!comma_delimited_strstr(mir->sComponents, mdi->win[i].code))
        continue;
      if (mdi->win[i].imeas_applied != 0)
        continue; // Prevent more than one measure per window MBG 4/07
    }

    else { // mir->flgWhichPass = FIRST_PASS

      /*************************
      Get the type of window (flat window, bay window, etc.) and the
      type of glazing (double, single, etc.).  If it is not a sliding
      glass door, then this retrofit will be implemented for that window.
      Implicit in this test is that any preceeding storm retrofit will
      have already changed the window type and thus avoid selecting both
      plastic and glass storm windows.
      **************************/

      if (mdi->win[i].retrofit_option != MWS_EVALUATE_ALL && mdi->win[i].retrofit_option != MWS_ADD_GLASS_STORM)
        continue;

      if (mdi->win[i].window_type == GLASSDOORMHEA ||
          (mdi->win[i].glazing_type != GT_DBL && mdi->win[i].glazing_type != GT_SINGLE)){
        if (mdi->win[i].retrofit_option == MWS_ADD_GLASS_STORM) {
          char buffer[MESSAGE_LEN];
          sprintf(buffer, "A required storm window measure was SKIPPED for window: %s because it can not be applied to the existing window or glazing type", mdi->win[i].code);
          add_mhea_message(buffer);
        }
        continue;
    }

    } // End mir->flgWhichPass = FIRST_PASS only execution

    append_component_code_to_current_results(mdi->win[i].code);

    /***************************
    Get the total number of windows of this window type.
    **************************/
    fNumWinTotal = mdi->win[i].num_n + mdi->win[i].num_s + mdi->win[i].num_e + mdi->win[i].num_w;

    if (fNumWinTotal > 0.0) {

      if (mir->flgWhichPass == FIRST_PASS)
        copy_mdi(&mdi, original); // Get copy of original mdi

      /*************************
      Reset glass type for retrofit window
      *************************/
      if (mdi->win[i].glazing_type == GT_SINGLE) {
        mdi->win[i].glazing_type = GT_SINGLEGLASS;
      }

      if (mdi->win[i].glazing_type == GT_DBL) {
        mdi->win[i].glazing_type = GT_DOUBLEGLASS;
      }
      float save_leak_coef = mdi->win[i].leak_coef;
      mdi->win[i].leak_coef = window_storm_leak_coef(save_leak_coef);

      /***********************************
      MBG 3/11/04 - Call mhea_energy_use() to compute the energy
      saved by a single window description.
      ***********************************/

      mhea_energy_use();
      save_energy_results();

      if (mir->flgWhichPass == CUMULATIVE) {
        // Prevent multiple retrofit measures from being applied to the same window  MBG 4/07
        mdi->win[i].imeas_applied = M_CMS_GLASS_STORM_WINDOWS;
      } else {
        mdi->win[i].leak_coef = save_leak_coef;
      }

      initialize_fuel_types();

      mir->flgRetrofits[ndx] = TRUE;
      STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
      STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

      //STRCPY(mir->Results[mir->Rndx].sName, "Glass Storm Windows");
      mir->Results[mir->Rndx].fQuant += (float)((mdi->win[i].width / 12.0) * (mdi->win[i].height / 12.0) * fNumWinTotal);

      mir->Results[mir->Rndx].measure_id = ndx;
      mir->Results[mir->Rndx].audit_section_id = M_WINDOW;
      mir->Results[mir->Rndx].material_id = iMat;

      mir->Results[mir->Rndx].fReportQuant += fNumWinTotal;
      STRCPY(mir->Results[mir->Rndx].sReportUnits, "Each");

      additional_cost(mdi->win[i].cost_add_glass_storm * fNumWinTotal);

      mhea_measure_sir(mir->Rndx); // assign the BCR
      mir->Results[mir->Rndx].flgGetBCR = 1;
      // Set global flag so that measure will receive 0.6 adjustment

      // If measure chosen from retrofit options combo box,
      // make it required.

      if (mdi->win[i].retrofit_option == MWS_ADD_GLASS_STORM) {
        mir->Results[mir->Rndx].measure_required = TRUE;
        if (mdi->win[i].inc_sir == TRUE)
          mir->Results[mir->Rndx].measure_priority = MPS_ENVELOPE_REQUIRED;
        else
          mir->Results[mir->Rndx].measure_priority = MPS_ENVELOPE_REQUIRED_NO_SIR;
      }

      // if we have implemented this measure in the main part of
      // the house, then increment our instance counter

      if (mir->Results[mir->Rndx].fQuant > 0.0)
        mir->Rndx++;
      //if (WeatherData) {free(WeatherData); WeatherData = NULL;}
    }

  } // End loop through window descriptions

  // GKA/MJF Issue #83
  if(mir->flgWhichPass != CUMULATIVE){
    copy_mdi(&mdi, original); // back to original MDI structure
    free(original);           // free memory allocated in this procedure
  }

  return;
}

/******************* FUNCTION NAME: retro_glass_storm_add ****************/
/**         DATE:  Dec 1993                                           **/
/**           BY:  NLW                                                **/
/**      REVISED:  12/29/98 MJF                                       **/
/**  DESCRIPTION:  Called by the ...Retrofits() functions.            **/
/** Adds up the area of windows that will be retrofited.              **/
/***********************************************************************/
void retro_glass_storm_add(void) {
  int ndx  = M_CMS_GLASS_STORM_WINDOWS_ADD;
  int iMat = M_MAT_GLASS_STORM_WINDOWS_ADD;

  float fNumWinTotal;
  MDI *original = NULL;

  mir->flgRetrofits[ndx] = FALSE;

  if(mir->flgWhichPass != CUMULATIVE){
    copy_mdi(&original, mdi); // Save original mdi structure, allocating memory for original
  }

  /***************
  Deterimine the infiltration savings from the glass storm measure
  separately so that it can be subtracted from the general infiltration
  savings during the final tally of savings. Do not permanently change
  the window characteristics in this loop.
  ***************/

  if (mir->flgWhichPass == CUMULATIVE && mir->Rndx != 0) {

    /****************************************************************************
    First loop through window descriptions to determe air leakage reduction,
    Cumulative pass for specific window related to instance of measure.
    ****************************************************************************/

    for (int i = 0; i < mdi->num_awn; i++) {
      if (!comma_delimited_strstr(mir->sComponents, mdi->awn[i].code))
        continue;
      else if (mdi->awn[i].imeas_applied != 0)
        continue; // Prevent more than one measure per window  MBG 4/07
      else {
        /***************************
        Get the total number of windows of this window type.
        **************************/

        fNumWinTotal = mdi->awn[i].num_n + mdi->awn[i].num_s + mdi->awn[i].num_e + mdi->awn[i].num_w;

        if (fNumWinTotal > 0.0) {

          /*************************
          Reset window leakiness
          *************************/
          float save_leak_coef = mdi->awn[i].leak_coef;
          mdi->awn[i].leak_coef = window_storm_leak_coef(save_leak_coef);

          /***********************************
          MBG 1/10/05 - Call mhea_energy_use() first with only window
          leakage changed in order to determine energy savings
          which should be attributed to this measure instead of
          general infiltration reduction.
          ***********************************/

          mir->flgWhichPass = NOT_BASE_CASE;
          mhea_energy_use();
          mir->flgWhichPass = CUMULATIVE;
          // mir->fInfMeasEnrgyHtg[ndx] += mir->Results[mir->Rndx - 1].fEnerPstHtg - mir->fHeating_Energy * mir->fAdj_Htg;
          // mir->fInfMeasEnrgyClg[ndx] += mir->Results[mir->Rndx - 1].fEnerPstClg - mir->fCooling_Energy * mir->fAdj_Clg;
          mir->fInfMeasEnrgyHtg[ndx] += mir->fPre_Heating - mir->fHeating_Energy * mir->fAdj_Htg;
          mir->fInfMeasEnrgyClg[ndx] += mir->fPre_Cooling - mir->fCooling_Energy * mir->fAdj_Clg;
          mdi->awn[i].leak_coef = save_leak_coef;
        }
      }
    } // End loop through window descriptions

    if (cmds.debug_level & D_MHEA_WIN_DETAIL) {
      //fprintf(stderr, "fEnerPreHtg(kBtu)  = %6.2f\n", mir->Results[mir->Rndx - 1].fEnerPstHtg/1000);
      fprintf(stderr, "fEnerPreHtg(kBtu)      = %6.2f\n", mir->fPre_Heating/1000);
      fprintf(stderr, "fCurrent_Heating(kBtu) = %6.2f\n", mir->fHeating_Energy/1000);
      fprintf(stderr, "InfEnrgyHtgSaved(kBtu) = %6.2f\n", mir->fInfMeasEnrgyHtg[ndx]/1000);
      fprintf(stderr, "InfEnrgyClgSaved(kBtu) = %6.2f\n", mir->fInfMeasEnrgyClg[ndx]/1000);
    }
  }

  /****************************************
  Second loop through window descriptions
  ****************************************/

  for (int i = 0; i < mdi->num_awn; i++) {
    if (mir->flgWhichPass == CUMULATIVE) {
      /*************************
      If cumulative pass, execute for only window description associated
      with instance of measure
      **************************/

      if (!comma_delimited_strstr(mir->sComponents, mdi->awn[i].code))
        continue;
      if (mdi->awn[i].imeas_applied != 0)
        continue; // Prevent more than one measure per window MBG 4/07
    }

    else { // mir->flgWhichPass = FIRST_PASS

      /*************************
      Get the type of window (flat window, bay window, etc.) and the
      type of glazing (double, single, etc.).  If it is not a sliding
      glass door, then this retrofit will be implemented for that window.
      Implicit in this test is that any preceeding storm retrofit will
      have already changed the window type and thus avoid selecting both
      plastic and glass storm windows.
      **************************/

      if (mdi->awn[i].retrofit_option != MWS_EVALUATE_ALL && mdi->awn[i].retrofit_option != MWS_ADD_GLASS_STORM)
        continue;

      if (mdi->awn[i].window_type == GLASSDOORMHEA ||
          (mdi->awn[i].glazing_type != GT_DBL && mdi->awn[i].glazing_type != GT_SINGLE)){
        if (mdi->win[i].retrofit_option == MWS_ADD_GLASS_STORM) {
          char buffer[MESSAGE_LEN];
          sprintf(buffer, "A required storm window measure was SKIPPED for window: %s because it can not be applied to the existing window or glazing type", mdi->win[i].code);
          add_mhea_message(buffer);
        }
        continue;
    }

    } // End mir->flgWhichPass = FIRST_PASS only execution

    append_component_code_to_current_results(mdi->awn[i].code);

    /***************************
    Get the total number of windows of this window type.
    **************************/
    fNumWinTotal = mdi->awn[i].num_n + mdi->awn[i].num_s + mdi->awn[i].num_e + mdi->awn[i].num_w;

    if (fNumWinTotal > 0.0) {

      if (mir->flgWhichPass == FIRST_PASS)
        copy_mdi(&mdi, original); // Get copy of original mdi

      /*************************
      Reset glass type for retrofit window
      *************************/
      if (mdi->awn[i].glazing_type == GT_SINGLE) {
        mdi->awn[i].glazing_type = GT_SINGLEGLASS;
      }

      if (mdi->awn[i].glazing_type == GT_DBL) {
        mdi->awn[i].glazing_type = GT_DOUBLEGLASS;
      }

      float save_leak_coef = mdi->awn[i].leak_coef;
      mdi->awn[i].leak_coef = window_storm_leak_coef(save_leak_coef);

      /***********************************
      MBG 3/11/04 - Call mhea_energy_use() to compute the energy
      saved by a single window description.
      ***********************************/

      mhea_energy_use();
      save_energy_results();

      if (mir->flgWhichPass == CUMULATIVE) {
        // Prevent multiple retrofit measures from being applied to the same window  MBG 4/07
        mdi->awn[i].imeas_applied = M_CMS_GLASS_STORM_WINDOWS_ADD;
      } else {
        mdi->awn[i].leak_coef = save_leak_coef;
      }

      initialize_fuel_types();

      mir->flgRetrofits[ndx] = TRUE;
      STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
      STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

      //STRCPY(mir->Results[mir->Rndx].sName, "Glass Storm Windows [Addition]");
      mir->Results[mir->Rndx].fQuant += (float)((mdi->awn[i].width / 12.0) * (mdi->awn[i].height / 12.0) * fNumWinTotal);

      mir->Results[mir->Rndx].measure_id = ndx;
      mir->Results[mir->Rndx].audit_section_id = M_WINDOW_ADDITION;
      mir->Results[mir->Rndx].material_id = iMat;

      mir->Results[mir->Rndx].fReportQuant += fNumWinTotal;
      STRCPY(mir->Results[mir->Rndx].sReportUnits, "Each");

      additional_cost(mdi->awn[i].cost_add_glass_storm * fNumWinTotal);

      mhea_measure_sir(mir->Rndx); // assign the BCR
      mir->Results[mir->Rndx].flgGetBCR = 1;
      // Set global flag so that measure will receive 0.6 adjustment

      // If measure chosen from retrofit options combo box,
      // make it required.

      if (mdi->awn[i].retrofit_option == MWS_ADD_GLASS_STORM) {
        mir->Results[mir->Rndx].measure_required = TRUE;
        if (mdi->awn[i].inc_sir == TRUE)
          mir->Results[mir->Rndx].measure_priority = MPS_ENVELOPE_REQUIRED;
        else
          mir->Results[mir->Rndx].measure_priority = MPS_ENVELOPE_REQUIRED_NO_SIR;
      }

      // if we have implemented this measure in the main part of
      // the house, then increment our instance counter

      if (mir->Results[mir->Rndx].fQuant > 0.0)
        mir->Rndx++;
      //if (WeatherData) {free(WeatherData); WeatherData = NULL;}
    }
  } // End loop through window descriptions

  // GKA/MJF Issue #83
  if(mir->flgWhichPass != CUMULATIVE){
    copy_mdi(&mdi, original); // back to original MDI structure
    free(original);           // free memory allocated in this procedure
  }

  return;
}

/******************* FUNCTION NAME: retro_awning **************************/
/**         DATE:  Dec 1993                                             **/
/**           BY:  NLW                                                  **/
/**      REVISED:  12/29/98 MJF                                         **/
/**  DESCRIPTION:  Called by the ...Retrofits() functions.              **/
/** Adds up the area of windows that will be retrofited.                **/
/*************************************************************************/
void retro_awning(void) {
  int ndx  = M_CMS_ADD_AWNINGMHEAS;
  int iMat = M_MAT_ADD_AWNINGMHEAS;

  float fNumWinTotal;

  mir->flgRetrofits[ndx] = FALSE;

  if (mir->flgNoCLG == TRUE) // don't apply shade if no cooling
    return;

  for (int i = 0; i < mdi->num_win; i++) {

    /*************************
    Apply awnings to non-north windows which are not shaded
    exclusing of shade screens
    **************************/

    if (mdi->win[i].ext_shading == ES_NONEMHEA && !mir->flgRetrofits[M_CMS_ADD_SHADE_SCREENS]) {
      /***************************
      Get the number of windows, excluding north windows.
      **************************/
      fNumWinTotal = mdi->win[i].num_s + mdi->win[i].num_e + mdi->win[i].num_w;

      if (fNumWinTotal > 0.0) {
        /*************************
        Reset shade type for retrofit window
        *************************/
        mdi->win[i].ext_shading = ES_AWNINGMHEA;

        mir->flgRetrofits[ndx] = TRUE;
        STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
        STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

        initialize_fuel_types();
        //STRCPY(mir->Results[mir->Rndx].sName, "Add Awnings");
        mir->Results[mir->Rndx].fQuant += fNumWinTotal;

        append_component_code_to_current_results(mdi->win[i].code);

        mir->Results[mir->Rndx].measure_id = ndx;
        mir->Results[mir->Rndx].audit_section_id = M_WINDOW;
        mir->Results[mir->Rndx].material_id = iMat;

      }
    }
  }

  // if we have implemented this measure in the main part of
  // the house, then increment our instance counter

  if (mir->Results[mir->Rndx].fQuant > 0.0)
    mir->Rndx++;

  return;
}

/******************* FUNCTION NAME: retro_awning_add **********************/
/**         DATE:  Dec 1993                                             **/
/**           BY:  NLW                                                  **/
/**      REVISED:  12/29/98 MJF                                         **/
/**  DESCRIPTION:  Called by the ...Retrofits() functions.              **/
/** Adds up the area of windows that will be retrofited.                **/
/*************************************************************************/
void retro_awning_add(void) {
  int ndx  = M_CMS_ADD_AWNINGMHEAS_ADD;
  int iMat = M_MAT_ADD_AWNINGMHEAS_ADD;

  float fNumWinTotal;

  mir->flgRetrofits[ndx] = FALSE;

  // now for windows in the addition

  if (mir->flgNoCLG == TRUE) // don't apply shade if no cooling
    return;

  for (int i = 0; i < mdi->num_awn; i++) {

    /*************************
    Apply awnings to non-north windows which are not shaded
    exclusing of shade screens
    **************************/

    if (mdi->awn[i].ext_shading == ES_NONEMHEA && !mir->flgRetrofits[M_CMS_ADD_SHADE_SCREENS_ADD]) {
      /***************************
      Get the number of windows, excluding north windows.
      **************************/
      fNumWinTotal = mdi->awn[i].num_s + mdi->awn[i].num_e + mdi->awn[i].num_w;

      if (fNumWinTotal > 0.0) {
        /*************************
        Reset shade type for retrofit window
        *************************/
        mdi->awn[i].ext_shading = ES_AWNINGMHEA;

        mir->flgRetrofits[ndx] = TRUE;
        STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
        STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

        initialize_fuel_types();
        //STRCPY(mir->Results[mir->Rndx].sName, "Add Awnings [Addition]");
        mir->Results[mir->Rndx].fQuant += fNumWinTotal;

        append_component_code_to_current_results(mdi->awn[i].code);

        mir->Results[mir->Rndx].measure_id = ndx;
        mir->Results[mir->Rndx].audit_section_id = M_WINDOW_ADDITION;
        mir->Results[mir->Rndx].material_id = iMat;

      }
    }
  }

  // if we have implemented this measure in the main part of
  // the house, then increment our instance counter

  if (mir->Results[mir->Rndx].fQuant > 0.0)
    mir->Rndx++;

  return;
}

/******************* FUNCTION NAME: retro_shade  **************************/
/**         DATE:  Dec 1993                                             **/
/**           BY:  NLW                                                  **/
/**      REVISED:  12/29/98 MJF                                         **/
/**  DESCRIPTION:  Called by the ...Retrofits() functions.              **/
/** Adds up the area of windows that will be retrofited.                **/
/*************************************************************************/
void retro_shade(void) {
  int ndx  = M_CMS_ADD_SHADE_SCREENS;
  int iMat = M_MAT_ADD_SHADE_SCREENS;

  float fNumWinTotal;

  mir->flgRetrofits[ndx] = FALSE;

  if (mir->flgNoCLG == TRUE)
    return; // don't apply shade if no cooling

  for (int i = 0; i < mdi->num_win; i++) {

    /*************************
    Apply shade screens to non-north windows which are not shaded
    exclusive of awnings
    **************************/

    if (mdi->win[i].ext_shading == ES_NONEMHEA && !mir->flgRetrofits[M_CMS_ADD_AWNINGMHEAS]) {
      /***************************
      Get the number of windows, excluding north windows.
      **************************/
      fNumWinTotal = mdi->win[i].num_s + mdi->win[i].num_e + mdi->win[i].num_w;

      if (fNumWinTotal > 0.0) {
        /*************************
        Reset shade type for retrofit window
        *************************/
        mdi->win[i].ext_shading = ES_SUNSCREEN;

        mir->flgRetrofits[ndx] = TRUE;
        STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
        STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

        initialize_fuel_types();
        //STRCPY(mir->Results[mir->Rndx].sName, "Add Shade Screens");
        mir->Results[mir->Rndx].fQuant += (float)((mdi->win[i].width / 12.0) * (mdi->win[i].height / 12.0) * fNumWinTotal);

        append_component_code_to_current_results(mdi->win[i].code);

        mir->Results[mir->Rndx].measure_id = ndx;
        mir->Results[mir->Rndx].audit_section_id = M_WINDOW;
        mir->Results[mir->Rndx].material_id = iMat;

      }
    }
  }

  // if we have implemented this measure in the main part of
  // the house, then increment our instance counter

  if (mir->Results[mir->Rndx].fQuant > 0.0)
    mir->Rndx++;

  return;
}

/******************* FUNCTION NAME: retro_shade_add  **********************/
/**         DATE:  Dec 1993                                             **/
/**           BY:  NLW                                                  **/
/**      REVISED:  12/29/98 MJF                                         **/
/**  DESCRIPTION:  Called by the ...Retrofits() functions.              **/
/** Adds up the area of windows that will be retrofited.                **/
/*************************************************************************/
void retro_shade_add(void) {
  int ndx  = M_CMS_ADD_SHADE_SCREENS_ADD;
  int iMat = M_MAT_ADD_SHADE_SCREENS_ADD;

  float fNumWinTotal;

  mir->flgRetrofits[ndx] = FALSE;

  if (mir->flgNoCLG == TRUE)
    return; // don't apply shade if no cooling

  // now for windows in the addition

  for (int i = 0; i < mdi->num_awn; i++) {

    /*************************
    Apply shade screens to non-north windows which are not shaded
    exclusive of awnings
    **************************/

    if (mdi->awn[i].ext_shading == ES_NONEMHEA && !mir->flgRetrofits[M_CMS_ADD_AWNINGMHEAS]) {
      /***************************
      Get the number of windows, excluding north windows.
      **************************/
      fNumWinTotal = mdi->awn[i].num_s + mdi->awn[i].num_e + mdi->awn[i].num_w;

      if (fNumWinTotal > 0.0) {
        /*************************
        Reset shade type for retrofit window
        *************************/
        mdi->awn[i].ext_shading = ES_SUNSCREEN;

        mir->flgRetrofits[ndx] = TRUE;
        STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
        STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

        initialize_fuel_types();
        //STRCPY(mir->Results[mir->Rndx].sName, "Add Shade Screens [Addition]");
        mir->Results[mir->Rndx].fQuant += (float)((mdi->awn[i].width / 12.0) * (mdi->awn[i].height / 12.0) * fNumWinTotal);

        append_component_code_to_current_results(mdi->awn[i].code);

        mir->Results[mir->Rndx].measure_id = ndx;
        mir->Results[mir->Rndx].audit_section_id = M_WINDOW_ADDITION;
        mir->Results[mir->Rndx].material_id = iMat;
      }
    }
  }

  // if we have implemented this measure in the main part of
  // the house, then increment our instance counter

  if (mir->Results[mir->Rndx].fQuant > 0.0)
    mir->Rndx++;

  return;
}

/******************* FUNCTION NAME: retro_smart_thermostat  ************************/
/**         DATE:  Dec   1993                                           **/
/**           BY:  NW, SLF                                              **/
/**      REVISED:  12/29/98 MJF                                         **/
/**  DESCRIPTION:  Called by the ...Retrofits() functions.              **/
/** In order for the setback retrofit to be carried out, the HVAC       **/
/** system must be a furnace with central air or heat pump heating      **/
/** and cooling.  See if statement below for exact test.  Currently,    **/
/** this code does not check to see if some setback is already being    **/
/** implemented in heating or cooling.  In the future, this should      **/
/** also be checked as it would affect the costing of the retrofit.     **/
/*************************************************************************/
void retro_smart_thermostat(void) {
  int ndx  = M_CMS_SETBACK_THERMOSTAT;
  int iMat = M_MAT_SETBACK_THERMOSTAT;

  float setback = mdi->key.thermostat_setback_amount;

  mir->flgRetrofits[ndx] = FALSE;

  /******************************
  If the heating equipment is not wood, coal, or none and if the
  heating night setpoint is greater than 60 and the difference
  between day and night setpoints is less than 5 degrees, then
  subtract the number of degrees specified in Setup from the heating
  nighttime setpoint.

  10/08 MBG Added checking to see that a setback thermostat doesn't
  already exist using new input parameter.

  All thermostat information is defined in the primary screen so
  no secondary tests will be performed.
  ******************************/

  if (!(mdi->htg.equip_type == ET_HEATPUMP || mdi->htg.equip_type == ET_NONE || mdi->htg.fuel_type == WOOD ||
        mdi->htg.fuel_type == COAL)) {
    if ((mdi->key.heating_setpoint_night > 60.0) &&
        (fabs(mdi->key.heating_setpoint_day - mdi->key.heating_setpoint_night) < 5.0) &&
        (mdi->htg.smart_thermostat != YES)) {
      mdi->key.heating_setpoint_night -= setback;

      mir->flgRetrofits[ndx] = TRUE;
      STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
      STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

      initialize_fuel_types();
      //STRCPY(mir->Results[mir->Rndx].sName, "Setback [heating]");
      mir->Results[mir->Rndx].fQuant = 1;

      mir->Results[mir->Rndx].measure_id = ndx;
      mir->Results[mir->Rndx].audit_section_id = M_HEATING;
      mir->Results[mir->Rndx].material_id = iMat;
    }
  }

// The cooling setback was having negative energy impacts as of
// 10/02 and the decision was made to remove cooling impacts of
// setback (the logic is that few people actually will want to
// sleep in warmer temperatures).  MJF


  // /******************************
  // Is there a primary cooling data structure.
  // ******************************/

  // if (mir->flgNoCLG == FALSE)
  // {

  //   /******************************
  //   If the cooling equipment is not Room AC or Evap. and if the
  //   cooling night setpoint is less than 85 and the difference
  //   between day and night setpoints is less than 5 degrees, then
  //   add 5 degrees to the cooling nighttime setpoint.

  //   All thermostat information is defined in the primary screen so
  //   no secondary tests will be performed.
  //   ******************************/

  //   if( !(mdi->clg.equip_type == CE_ROOMAC ||
  //     mdi->clg.equip_type == CE_EVAPORATIVE) )
  //   {
  //     if((mdi->key.cooling_setpoint_night < 85.0) &&
  //       (fabs(mdi->key.cooling_setpoint_night -
  //       mdi->key.cooling_setpoint_day) < 5.0) )
  //     {
  //       mdi->key.cooling_setpoint_night += setback;

  //       // note how we append the sName rather than assign

  //       mir->flgRetrofits[ndx] = TRUE;
  //       initialize_fuel_types();
  //       STRCAT(mir->Results[mir->Rndx].sName, "[Cooling]");
  //       mir->Results[mir->Rndx].measure_id = ndx;
  //       mir->Results[mir->Rndx].fQuant = 1;
  //       mir->Results[mir->Rndx].material_id = M_MAT_SETBACK_THERMOSTAT;
  //     }
  //   }
  // }

  if (mir->Results[mir->Rndx].fQuant > 0.0)
    mir->Rndx++;

  return;
}

/******************* FUNCTION NAME: retro_tune_heating    *****************/
/**         DATE:  December 1993                                        **/
/**           BY:  NW, SLF                                              **/
/**      REVISED:  12/29/98 MJF                                         **/
/**  DESCRIPTION:  Called by the ...Retrofits() functions.              **/
/** Increases all heating system efficiencies, both primary and         **/
/** secondary systems.  The amount of increase is in the key parameters **/
/*************************************************************************/
void retro_tune_heating(void) {
  int ndx  = M_CMS_TUNE_HEATING_SYSTEM;
  int iMat = M_MAT_TUNE_HEATING_SYSTEM;
  static int added_message = FALSE;

  mir->flgRetrofits[ndx] = FALSE;

  /**************************
  Even on first pass, prevent evaluating tuneup if furnace replacement
  is required and tuneup is not. This allows the replacement to be
  required but not included in the whole house SIR.
  ***************************/

  if(mdi->htr.replacement == YES && mdi->htg.tuneup != YES)
    return;

  if(mdi->htg.fuel_type == WOOD) return;
  // MHEA doesn't correctly model tuning up wood systems. MBG 2/11

  if(mdi->htg.equip_type == ET_SPACEHEAT) return;
  // Don't tune-up space heaters.  MBG 6/11

  /****************
  Don't do retrofit if furnace replacement is already anticipated
  ****************/

  if (!mir->flgRetrofits[M_CMS_REPLACE_HEATING_SYSTEM] && mdi->htg.fuel_type != ELECTRIC) {

    // should note how the retrofit effects energy here
    // MJF 9/01

    mir->flgRetrofits[ndx] = TRUE;
    STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
    STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

    float deleff, e0, e1, e2, c1, s1, c2, fEfficiency;

    if (mdi->htg.eff_units == HE_COP) {
      fEfficiency = mdi->htg.efficiency_cop;
    } else if (mdi->htg.eff_units == HE_HSPF) {
      fEfficiency = mdi->htg.efficiency_hspf / 3.413F;
    } else {
      fEfficiency = (float)(mdi->htg.efficiency_percent / 100.0);
    }

    deleff = 0.0f;
    e0 = .65f;
    e2 = .76f;

    ASSERT(mdi->htg.fuel_type == NATURAL_GAS || 
      mdi->htg.fuel_type == PROPANE || 
      mdi->htg.fuel_type == KEROSENE || 
      mdi->htg.fuel_type == OIL, sprintf(msg, "Incorrect fuel for tuneup"));

    if (mdi->htg.fuel_type == NATURAL_GAS || mdi->htg.fuel_type == PROPANE) {
      e1 = .70f, c1 = 0.67f, s1 = .04f;
    } // Gas or Propane
    else if (mdi->htg.fuel_type == KEROSENE || mdi->htg.fuel_type == OIL) {
      e1 = .69f, c1 = 1.0f, s1 = .07f;
    }

    c2 = 0.0f;
    if (fEfficiency <= e0) {
      c2 = 3.0f;
      deleff = s1;
    } else if (fEfficiency <= e1) {
      c2 = 2.0f;
      deleff = s1;
    } else if (fEfficiency <= e2) {
      c2 = 1.0f;
      deleff = c1 * (e2 - fEfficiency);
    }

    if (mdi->htg.equip_type != ET_FURNACE && mdi->htg.equip_type != ET_HEATPUMP)
      c2 = 0.0f;
    if (mdi->inf.evaluate_duct_sealing != YES)
      deleff += 0.02f * c2;

    if (deleff < 0.001f) {
      if (mdi->htg.tuneup == YES && mir->flgWhichPass == CUMULATIVE && added_message == FALSE) {
        sprintf(mir->sMsg, "Heating Tune Up required but there are no efficiency gains beyond: %f", fEfficiency);
        add_mhea_message(mir->sMsg);
        added_message = TRUE;
      }
      // no change to mdi, but continue
    } else {
      fEfficiency += deleff;
      mdi->htg.eff_units = HE_COP;
      mdi->htg.efficiency_cop = fEfficiency;
    }

    initialize_fuel_types();
    //sprintf(mir->Results[mir->Rndx].sName, "Tune Heating System");
    mir->Results[mir->Rndx].fQuant = 1;

    mir->Results[mir->Rndx].measure_id = ndx;
    mir->Results[mir->Rndx].audit_section_id = M_HEATING;
    mir->Results[mir->Rndx].material_id = iMat;

    // If measure chosen from retrofit options combo box,
    // make it required.

    if (mdi->htg.tuneup == YES) {
      mir->Results[mir->Rndx].measure_required = TRUE;
      if (mdi->htg.inc_sir == TRUE)
        mir->Results[mir->Rndx].measure_priority = MPS_EQUIP_REQUIRED;
      else
        mir->Results[mir->Rndx].measure_priority = MPS_EQUIP_REQUIRED_NO_SIR;
    }

    mir->Rndx++;
  }

  return;
}

/******************* FUNCTION NAME: retro_evaporative_cooling ********************/
/**         DATE:  December 1993                                        **/
/**           BY:  NW, SLF                                              **/
/**      REVISED:  12/29/98 MJF                                         **/
/**  DESCRIPTION:  Called by the ...Retrofits() functions.              **/
/** Replace DX cooling with evaporative cooling system                  **/
/*************************************************************************/
void retro_evaporative_cooling(void) {
  int ndx  = M_CMS_EVAPORATIVE_COOLING;
  int iMat = M_MAT_EVAPORATIVE_COOLING;

  float fRatedCOP;

  mir->flgRetrofits[ndx] = FALSE;

  if (mir->flgNoCLG == TRUE)
    return;

  // User must have chosen evaporative cooling as replacement. 8/25/09 MBG

  if (mdi->clr.equip_type != CE_EVAPORATIVE)
    return;

  // changes both primary and secondary systems
  // to evaporative coolers if they are central or roomAC
  // so if we have central or room AC with a low COP
  // and we are not doing a tune up or replacement
  // then lets consider doing an evaporative cooler

  // would be NICE if we could test to see if the
  // climate (humidity and cooling degree days)
  // is appropriate here.  MJF 9/01

  // First the Primary System

  fRatedCOP = get_cooling_cop(mdi->clg.eff_units, mdi->clg.efficiency_cop, mdi->clg.efficiency_seer, mdi->clg.efficiency_eer);

  if ((mdi->clg.equip_type == CE_CENTRALAC || mdi->clg.equip_type == CE_ROOMAC) && fRatedCOP < 2.5 &&
      !mir->flgRetrofits[M_CMS_TUNE_COOLING_SYSTEM] && !mir->flgRetrofits[M_CMS_REPLACE_DX_COOLING_EQUIP]) {

    mdi->clg.equip_type = CE_EVAPORATIVE;
    mdi->clg.saturating_eff_for_evaporative = mdi->key.saturating_eff_for_evaporative_rplcmnt;

    mir->flgRetrofits[ndx] = TRUE;
    STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
    STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

    initialize_fuel_types();
    //STRCPY(mir->Results[mir->Rndx].sName, "Evaporative Cooling");
    mir->Results[mir->Rndx].fQuant += 1;

    mir->Results[mir->Rndx].measure_id = ndx;
    mir->Results[mir->Rndx].audit_section_id = M_COOLING;
    mir->Results[mir->Rndx].material_id = iMat;

  }

  // Then the secondary system

  // if (mdi->cl2.eff_units == CE_COP)
  //   fRatedCOP = mdi->cl2.eff;
  // else
  //   fRatedCOP = (float)(mdi->cl2.eff / 3.413);
  fRatedCOP = get_cooling_cop(mdi->cl2.eff_units, mdi->cl2.efficiency_cop, mdi->cl2.efficiency_seer, mdi->cl2.efficiency_eer);

  if ((mdi->cl2.equip_type == CE_CENTRALAC || mdi->cl2.equip_type == CE_ROOMAC) && fRatedCOP < 2.5 &&
      !mir->flgRetrofits[M_CMS_TUNE_COOLING_SYSTEM] && !mir->flgRetrofits[M_CMS_REPLACE_DX_COOLING_EQUIP]) {

    mdi->cl2.equip_type = CE_EVAPORATIVE;
    mdi->cl2.saturating_eff_for_evaporative = mdi->key.saturating_eff_for_evaporative_rplcmnt;

    mir->flgRetrofits[ndx] = TRUE;
    STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
    STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

    initialize_fuel_types();
    //STRCPY(mir->Results[mir->Rndx].sName, "Evaporative Cooling");
    mir->Results[mir->Rndx].fQuant += 1;

    mir->Results[mir->Rndx].measure_id = ndx;
    mir->Results[mir->Rndx].audit_section_id = M_COOLING;
    mir->Results[mir->Rndx].material_id = iMat;

  }

  if (mir->Results[mir->Rndx].fQuant > 0.0)
    mir->Rndx++;

  return;
}

/******************* FUNCTION NAME: retro_tune_cooling ********************/
/**         DATE:  December 1993                                        **/
/**           BY:  NW, SLF                                              **/
/**      REVISED:  12/29/98 MJF                                         **/
/**  DESCRIPTION:  Called by the ...Retrofits() functions.              **/
/** Tune all cooling systems                                            **/
/*************************************************************************/
void retro_tune_cooling(void) {
  int ndx  = M_CMS_TUNE_COOLING_SYSTEM;
  int iMat = M_MAT_TUNE_COOLING_SYSTEM;

  float fThresholdCOP = 3.0;        // above this don't tune
  float fRatedCOP;
  float fImprovementFactor = 1.15;  // home much to improve existing efficiency

  mir->flgRetrofits[ndx] = FALSE;

  if (mir->flgNoCLG == TRUE) // no cooling systems to modify
    return;

  /**************************
  Even on first pass, prevent evaluating tuneup if cooling replacement
  is required and tuneup is not. This allows the replacement to be
  required but not included in the whole house SIR.
  ***************************/

  if (mdi->clr.replacement == YES && mdi->clg.tuneup != YES )
    return;

  // apply the retrofit to the primary system

  fRatedCOP = get_cooling_cop(mdi->clg.eff_units, mdi->clg.efficiency_cop, mdi->clg.efficiency_seer, mdi->clg.efficiency_eer);


  if (mdi->clg.tuneup == YES || (!mir->flgRetrofits[M_CMS_EVAPORATIVE_COOLING] && !mir->flgRetrofits[M_CMS_REPLACE_DX_COOLING_EQUIP] &&
      mdi->clg.equip_type != CE_NONE && fRatedCOP < fThresholdCOP)) {

    if (mdi->clg.equip_type == CE_EVAPORATIVE) {
      mdi->clg.saturating_eff_for_evaporative = mdi->key.saturating_eff_for_evaporative_tune_up;
    } else {
      switch (mdi->clg.eff_units) {
      case CE_COP:
        mdi->clg.efficiency_cop *= fImprovementFactor;
        break;
      case CE_SEER:
        mdi->clg.efficiency_seer *= fImprovementFactor;
        break;
      case CE_EER:
        mdi->clg.efficiency_eer *= fImprovementFactor;
        break;
      }
    }

    mir->flgRetrofits[ndx] = TRUE;
    initialize_fuel_types();
    mir->Results[mir->Rndx].fQuant = 1;

    if (mdi->clg.inc_sir == TRUE)
      mir->Results[mir->Rndx].measure_priority = MPS_EQUIP_REQUIRED;    // Issue #335
    else
      mir->Results[mir->Rndx].measure_priority = MPS_EQUIP_REQUIRED_NO_SIR;

    // Issue #335
    if (mdi->clg.tuneup == YES) {
      mir->Results[mir->Rndx].measure_required = TRUE;
      if (mdi->clg.inc_sir == TRUE)
        mir->Results[mir->Rndx].measure_priority = MPS_EQUIP_REQUIRED;
      else
        mir->Results[mir->Rndx].measure_priority = MPS_EQUIP_REQUIRED_NO_SIR;
    }

    mir->Results[mir->Rndx].measure_id = ndx;
    mir->Results[mir->Rndx].audit_section_id = M_COOLING;
    mir->Results[mir->Rndx].material_id = iMat;

  }

  // also apply to the secondary system if it exists # 334 since this is what was
  // happening in the consumption.c old code.  Credit was given for tuning both
  // primary and secondary and COP was set to 3.0 regardless of the secondary
  // system type or the existing COP

  fRatedCOP = get_cooling_cop(mdi->cl2.eff_units, mdi->cl2.efficiency_cop, mdi->cl2.efficiency_seer, mdi->cl2.efficiency_eer);

  if (mdi->cl2.equip_type != CE_NONE) {
    if (mdi->clg.tuneup == YES || (!mir->flgRetrofits[M_CMS_EVAPORATIVE_COOLING] && !mir->flgRetrofits[M_CMS_REPLACE_DX_COOLING_EQUIP] && fRatedCOP < fThresholdCOP)) {

      if (mdi->cl2.equip_type == CE_EVAPORATIVE) {
        mdi->cl2.saturating_eff_for_evaporative = mdi->key.saturating_eff_for_evaporative_tune_up;
      } else {
        switch (mdi->cl2.eff_units) {
        case CE_COP:
          mdi->cl2.efficiency_cop *= fImprovementFactor;
          break;
        case CE_SEER:
          mdi->cl2.efficiency_seer *= fImprovementFactor;
          break;
        case CE_EER:
          mdi->cl2.efficiency_eer *= fImprovementFactor;
          break;
        }
      }

      mir->flgRetrofits[ndx] = TRUE;
      initialize_fuel_types();
      mir->Results[mir->Rndx].fQuant += 1;

      mir->Results[mir->Rndx].measure_id = ndx;
      mir->Results[mir->Rndx].audit_section_id = M_COOLING;
      mir->Results[mir->Rndx].material_id = M_MAT_TUNE_COOLING_SYSTEM;

    }
  }

  if (mir->flgRetrofits[ndx] == TRUE){
    sprintf(mir->sMsg, "%s [%d]", mdi->cms[ndx].measure_name, (int) mir->Results[mir->Rndx].fQuant);
    STRCPY(mir->Results[mir->Rndx].sName, mir->sMsg);
    STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);
    mir->Rndx++;
  }

  return;
}

/******************* FUNCTION NAME: retro_replace_cooling *****************/
/**         DATE:  December 1993                                        **/
/**           BY:  NW, SLF                                              **/
/**      REVISED:  12/29/98 MJF                                         **/
/**  DESCRIPTION:  Called by the ...Retrofits() functions.              **/
/** Replace DX cooling with high efficiency system                      **/
/*************************************************************************/
void retro_replace_cooling(void) {
  int ndx = M_CMS_REPLACE_DX_COOLING_EQUIP;
  int iMat;

  //char stype[50];
  //int type;
  //float fRatedCOP;

  mir->flgRetrofits[ndx] = FALSE;

  if (mir->flgNoCLG == TRUE) // no systems to modify
    return;

  /******************************
  If replacing the cooling system is required, copy the replacement
  cooling specifications to the primary cooling system data.  Only
  the primary cooling system will be considered for replacement.
  Skip the whole retrofit if replacement equipment data is missing
  or if tuneup measure declared required.
  ******************************/

  if (mir->flgRetrofits[M_CMS_TUNE_COOLING_SYSTEM] || mdi->clg.tuneup == YES)
    return;

  if (mdi->clr.equip_type == CE_NONE ||
      (mdi->clr.efficiency_cop == 0.0 && mdi->clr.efficiency_seer == 0.0 && mdi->clr.efficiency_eer == 0.0))
    return;

  /*******************
  If central or room AC and tune-up and evap cooler replacement
  are not anticipated, replace cooling system with similar higher
  efficiency equipment.

  Removed considertion of efficiency of exiting equipment to determine IF the
  calculation is done.  We were limiting consideration to existing system COP
  less than 2.5   MJF Issue #137 1/20

  Changed to replace the entire clg structure with a new clr structure
  with version 8.6
  ********************/

  //fRatedCOP = get_cooling_cop(mdi->clg.eff_units, mdi->clg.efficiency_cop, mdi->clg.efficiency_seer, mdi->clg.efficiency_eer);

  if (mdi->clr.replacement == YES ||
      ((mdi->clg.equip_type == CE_CENTRALAC || mdi->clg.equip_type == CE_ROOMAC || mdi->clg.equip_type == CE_HEATPUMP) &&
       !mir->flgRetrofits[M_CMS_EVAPORATIVE_COOLING] && 
       !mir->flgRetrofits[M_CMS_TUNE_COOLING_SYSTEM] )) {

    // Issue #156 fix possible uninitialized type assignment

    ASSERT(mdi->clg.equip_type == CE_CENTRALAC || 
      mdi->clg.equip_type == CE_ROOMAC || 
      mdi->clg.equip_type == CE_HEATPUMP, sprintf(msg, "Must have the right equipment type for replacement"));

    if (mdi->clg.equip_type == CE_CENTRALAC) {
      iMat = M_MAT_REPLACE_CLG_CENTRAL;
      //STRCPY(stype, "[Central]");
    }
    if (mdi->clg.equip_type == CE_HEATPUMP) {
      iMat = M_MAT_REPLACE_CLG_HT_PUMP;
      //STRCPY(stype, "[Heat Pump]");
    }
    if (mdi->clg.equip_type == CE_ROOMAC) {
      iMat = M_MAT_REPLACE_CLG_ROOM_AC;
      //(stype, "[Room AC]");
    }

    // STRCPY(mdi->clg.description, mdi->clr.description);
    mdi->clg.equip_type = mdi->clr.equip_type;
    mdi->clg.eff_units = mdi->clr.eff_units;
    mdi->clg.efficiency_cop = mdi->clr.efficiency_cop;
    mdi->clg.efficiency_seer = mdi->clr.efficiency_seer;
    mdi->clg.efficiency_eer = mdi->clr.efficiency_eer;
    mdi->clg.capacity = mdi->clr.capacity;
    mdi->clg.duct_location = mdi->clr.clg_duct_location;
    mdi->clg.duct_insl = mdi->clr.clg_duct_insl;

    // note that this replaces just the cooling system without
    // changing the percent of cooling load supplied by that system
    // if the replacement is intended as a replacement for both the
    // primary and secondary system, then we need to reset the .cl2
    // structure (blank it) and set the percent of load served to 100
    // for the .clg structure

    if (mdi->clr.replacement == TRUE) {
      mir->Results[mir->Rndx].measure_required = TRUE;
      if (mdi->clr.incl_costs == TRUE)
        mir->Results[mir->Rndx].measure_priority = MPS_EQUIP_REQUIRED;
      else
        mir->Results[mir->Rndx].measure_priority = MPS_EQUIP_REQUIRED_NO_SIR;
    }

    // we added the ability to put the material and labor cost
    // directly in the htr structure in 8.3.3 MJF 9/07

    // so, here we put our specific costs back into the rmc
    // structure so the subsequent material cost lookups in the
    // GetBCR function work as expected

    mdi->rmc[iMat].material = mdi->clr.material_cost;
    mdi->rmc[iMat].labor = mdi->clr.labor_cost;
    mdi->rmc[iMat].extra = 0.0;

    mir->flgRetrofits[ndx] = TRUE;
    STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
    STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

    initialize_fuel_types();
    //STRCPY(mir->Results[mir->Rndx].sName, "Replace Cooling System ");
    //STRCAT(mir->Results[mir->Rndx].sName, stype);
    mir->Results[mir->Rndx].fQuant = 1;

    mir->Results[mir->Rndx].measure_id = ndx;
    mir->Results[mir->Rndx].audit_section_id = M_COOLING;
    mir->Results[mir->Rndx].material_id = iMat;

    mir->Rndx++;
  }
  return;
}

// This is the first of the baseload measure routines.  Because base
// load routines should call mhea_measure_sir directly, they will not cause
// a recalculation of the heating and cooling loads in the building, see
// the retrofit.c module for details

// Lighting Retrofits

void retro_replace_lighting(void) {
  int ndx  = M_CMS_LIGHTING_RETROFITS;

  BCR_RES *res;

  if (mir->flgRetrofits[ndx] == TRUE) // should not be called more than once in a pass
    return;

  mir->flgRetrofits[ndx] = FALSE;

  // separate calculation for each lighting replacement record

  for (int nc = 0; nc < mdi->num_ltg; nc++) {
    LTG ltg = mdi->ltg[nc];
    if (ltg.new_lamp_count <= 0)
      continue;

    int nm = mir->Rndx;
    res = &mir->Results[nm];
    mir->flgRetrofits[ndx] = TRUE;
    STRCPY(res->sName, mdi->cms[ndx].measure_name);
    //sprintf(res->sName, "Lighting Replacement [%d]", ltg.new_lamp_count);

    initialize_fuel_types();
    { char r_type[TYPE_LEN + 1];
    sprintf(r_type, "%s %0.1f watts", lighting_type_name(ltg.new_lamp_type), ltg.new_lamp_watts);
    STRCPY(res->sMaterial, r_type);
    }
    STRCPY(res->sComponents, ltg.code);
    STRCPY(res->sUnits, "Each Bulb");

    res->fEnerPreHtg = mir->fPre_Heating;    // no heating or cooling changes for now
    res->fEnerPstHtg = mir->fPre_Heating;
    res->fEnerPreClg = mir->fPre_Cooling;
    res->fEnerPstClg = mir->fPre_Cooling;

    // determine before and after annual energy (Btus)
    // 365 days/yr /1000 * 3413 Btu/kWh = 1245.75 conversion factor

    res->fEnerPreBas = ltg.exist_lamp_watts * ltg.exist_hours_per_day * ltg.exist_lamp_count * 1245.75f;
    res->fEnerPstBas = ltg.new_lamp_watts * ltg.new_hours_per_day * ltg.new_lamp_count * 1245.75f;

    // quantity of which material and additional cost
    // override the retrofit lifetime with lamp life calculation

    res->measure_id = ndx;
    res->audit_section_id = M_LIGHTING;
    res->material_id = M_MAT_FROM_AUDIT;

    // the lifetime of lamps in the rmc table is expressed in
    // thousands of hours of lamp opertion

    res->fLife = ltg.new_lifetime_hrs / ltg.new_hours_per_day / 365.0f;  // years decimal mhea_measure_sir() interpolates

    if (res->fLife > MAX_LIGHT_LIFE) res->fLife = MAX_LIGHT_LIFE;

    res->fInitCost = (ltg.install_cost_per_lamp + ltg.added_cost_per_lamp) * ltg.new_lamp_count + ltg.added_cost;

    res->fQuant = ltg.new_lamp_count;
    res->fUnitMaterial = ltg.install_cost_per_lamp;
    res->fUnitLabor = ltg.added_cost_per_lamp;

    // res->costi2 = (ltg.install_cost_per_lamp + ltg.added_cost_per_lamp) * ltg.new_lamp_count;
    // res->typei2 = MT_LIGHTING;
    // STRCPY(res->desci2, res->sMaterial);

    if (ltg.added_cost > 0.0f) {
      res->costi1 = ltg.added_cost;
    } else {
      res->costi1 = 0.0f;
    }

    mhea_measure_sir(nm); // compute the BCR

    mir->Rndx++;
    }  // next ltg, individual measures
}

/******************* FUNCTION NAME: retro_refrigerator      ***************/
/**         DATE:  9/24/01                                              **/
/**           BY:  MJF                                                  **/
/**      REVISED:                                                       **/
/**  DESCRIPTION:                                                       **/
/*************************************************************************/
void retro_refrigerator(void) {

  // local variables used in analysis

  char source_exist;         // 'T' for AHAM table or label, 'M' for metering
  float Ce;                  // annual kWh consumption of existing, all corrections applied
  float Cn;                  // annual kWh consumption of new refrigerator
  float space_temp;          // ambient temperature around refrigerator (tia or measured)
  float kwh_per_day;         // existing refrigerator kWh/day, unadjusted (except for open_factor and defrost for metered data)
  float age_factor;          // factor adjusting refrig consumption for age
  float defrost_factor;      // factor to adjust metered consumption for defrost cycles
  float min_defrost = 12.0f; // assumed minutes duration of defrost cycle
  float doorsealfactor;      // fraction degradation due to door seal condition (Blasnik)
  float occupancyfactor;     // occupancy factor (Blasnik)
  float totalfactor_exist = 0.0f;   // overall factor for existing unit
  float totalfactor_replace = 0.0f; // overall factor for replacement unit
  float Tia = 75.0f;                // AHAM standard ambient temperature for ratings.

  int ndx = M_CMS_REFRIGERATOR_REPLACEMENT;

  // local variables used as placehold for constant
  // candidates for parameter set definitions table

  // float open_factor=0.882f;   // factor to adjust metered cons for door openings

  mir->flgRetrofits[ndx] = FALSE;

  if ((mdi->ref.meter_energy_reading < 0.001f && mdi->ref.label_kwh_per_year < 0.001f) || mdi->ref.replace_kwh_per_year < 0.001f)
    return; // No input to compute measure

  // decide if we are given AHAM/label or metering information and
  // set the source character flag appropriately

  if (mdi->ref.label_kwh_per_year > 0.001f && mdi->ref.label_year_id != 0) {

    // Existing unit consumption from AHAM or label
    source_exist = 'T';
    space_temp = Tia; // AHAM base temperature
    kwh_per_day = mdi->ref.label_kwh_per_year / 365.0f;
  } else if (mdi->ref.meter_energy_reading > 0.001f && mdi->ref.meter_energy_interval > 0.001) {

    // Existing unit cons. from metered data
    source_exist = 'M';
    space_temp = mdi->ref.meter_temperature;
    if (mdi->ref.meter_includes_defrost == YES) {
      mdi->ref.meter_energy_interval -= min_defrost;
      mdi->ref.meter_energy_reading -= mdi->key.refrigerator_defrost_cycle_energy;
    }
    kwh_per_day = mdi->ref.meter_energy_reading / mdi->ref.meter_energy_interval * 60.0f * 24.0f;
    //   kwh_per_day /= open_factor;
  } else // did not get enough data, so we are returning w/o analysis
    return;

  no_heating_or_cooling_savings();

  // make adjustments for unit being in unconditioned space #75 *****/

  if (mdi->ref.location_id == HEATED || mdi->ref.location_id == 0) { // In conditioned space
    Cn = mdi->ref.replace_kwh_per_year;
    Ce = kwh_per_day * 365.0f;
  }

  else {            //  Determine consumptions in uncond. space

    int m;
    float uncond_temp[13];     // constrained estimate of uncond space ambient temp by month
    float lowlimit;            // lower limit for space temperature
    float upperlimit;          // upper limit for space temperature
    float monthsptemp;

    Cn = Ce = 0.0f; //  zero the accumlators
    for (m = 1; m <= MONTHS; m++) {
      // This differs from the NEAT calculation in that NEAT uses an adjusted
      // sub basement temperature for the unintentionally conditioned space  MJF #75
      //ASSERT(mir->ftoad[m], sprintf(msg, "Need monthly outdoor drybulb temperature"));
      //monthsptemp = mir->ftoad[m];
      monthsptemp = cwd->avg_daytime_temp[m];
      if (mdi->ref.location_id == UNHEATED) { // Determine consumptions in uncond. space
        lowlimit = 30.0f; // Space temp min of 35 F;
        upperlimit = Tia + 20.0f;
      }      // Space temp max of 20 + room temp
      else { // Determine consumptions in unintentionally cond. space
        lowlimit = 45.0f; // Space temp min of 45 F;
        upperlimit = Tia + 10.0f;
      } // Space temp max of 10 + room temp
      uncond_temp[m] = MAX(monthsptemp, lowlimit);
      uncond_temp[m] = MIN(uncond_temp[m], upperlimit);

      Cn += mdi->ref.replace_kwh_per_year / 365 * cwd->days_in_month[m] * (1.0f - 0.025f * (Tia - uncond_temp[m]));
      Ce += kwh_per_day * cwd->days_in_month[m] * (1.0f - 0.025f * (space_temp - uncond_temp[m]));
    }
  }

  // adjust existing unit consumption for door seal condition

  if (source_exist == 'T') {
    switch (mdi->ref.door_seal_condition_id) {
    case SEAL_GOOD:
      doorsealfactor = 0.0f;
      break;
    case SEAL_SOME_DETERIORATION:
      doorsealfactor = 0.05f;
      break;
    case SEAL_GAPS_VISIBLE:
      doorsealfactor = 0.15f;
      break;
    default:
      doorsealfactor = 0.0f;
    }
    totalfactor_exist += doorsealfactor;
  }

  // apply occupancy factor to both existing and new refrigerators

  switch ((int)mdi->gnl.avg_no_occupants) {
  case 0:
    occupancyfactor = 0.0f;
    break;
  case 1:
    occupancyfactor = 0.05f;
    break;
  case 2:
    occupancyfactor = 0.1f;
    break;
  case 3:
    occupancyfactor = 0.13f;
    break;
  case 4:
    occupancyfactor = 0.15f;
    break;
  case 5:
    occupancyfactor = 0.16f;
    break;
  default:
    occupancyfactor = 0.0f;
    break;
  }
  if (mdi->gnl.avg_no_occupants > 5.0f)
    occupancyfactor = 0.16f;
  totalfactor_exist += occupancyfactor;
  totalfactor_replace += occupancyfactor;

  // adjust for age of unit if existing unit consumption is from label/AHAM

  if (source_exist == 'T') {
    switch (mdi->ref.label_year_id) {
    case AGE_NEW:
      age_factor = 0.0f;
      break;
    case AGE_5_10:
      age_factor = 0.05f;
      break;
    case AGE_10_15:
      age_factor = 0.1f;
      break;
    case AGE_15:
      age_factor = 0.15f;
      break;
    default:
      age_factor = 0.0f;
    }
    totalfactor_exist += age_factor;
  }

  Ce *= (1.0f + totalfactor_exist);
  Cn *= (1.0f + totalfactor_replace);

  // adjust for defrost cycle

  if (source_exist == 'T' || mdi->ref.meter_manual_defrost == YES)
    defrost_factor = 1.0f;
  else
    defrost_factor = 1.08f;
  Ce *= defrost_factor;

  mir->Results[mir->Rndx].fEnerPreBas = Ce * fuel_heat_content(ELECTRIC) * 1e6;
  mir->Results[mir->Rndx].fEnerPstBas = Cn * fuel_heat_content(ELECTRIC) * 1e6;

  mir->flgRetrofits[ndx] = TRUE;
  STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
  //STRCPY(mir->Results[mir->Rndx].sName, "Refrigerator Replacement");

  initialize_fuel_types();

  STRCPY(mir->Results[mir->Rndx].sMaterial, "Refrigerator ");
  STRCPY(mir->Results[mir->Rndx].sUnits, "Ea ");

  mir->Results[mir->Rndx].fQuant = 1;
  mir->Results[mir->Rndx].fLife = (float)(mdi->ref.replace_life);
  mir->Results[mir->Rndx].fInitCost = mdi->ref.replace_install_cost + mdi->ref.replace_added_cost;

  mir->Results[mir->Rndx].measure_id = ndx;
  mir->Results[mir->Rndx].audit_section_id = M_REFRIGERATOR;
  mir->Results[mir->Rndx].material_id = M_MAT_FROM_AUDIT;

  mir->Results[mir->Rndx].costi2 = mdi->ref.replace_install_cost;
  mir->Results[mir->Rndx].typei2 = MT_REFRIGERATORS;
  STRCPY(mir->Results[mir->Rndx].desci2, mdi->ref.replace_manufacturer);
  STRCAT(mir->Results[mir->Rndx].desci2, " - ");
  STRCAT(mir->Results[mir->Rndx].desci2, mdi->ref.replace_model);

  mir->Results[mir->Rndx].costi3 = mdi->ref.replace_added_cost;
  mir->Results[mir->Rndx].typei3 = MT_LABOR;
  STRCPY(mir->Results[mir->Rndx].desci3, "Installation Labor");

  mhea_measure_sir(mir->Rndx); // compute the BCR

  mir->Rndx++;
  return;
}

/******************* FUNCTION NAME: retro_water_heater_insulation       ****************/
/**         DATE:  9/24/01                                              **/
/**           BY:  MJF                                                  **/
/**      REVISED:                                                       **/
/**  DESCRIPTION:                                                       **/
/*************************************************************************/
void retro_water_heater_insulation(void) {
  int ndx  = M_CMS_WATER_HEATER_TANK_INS;
  int iMat = M_MAT_DWH_TANK_WRAP;

  float dwh_area, dwh_gal, dwh_upre;
  int m, dwhFuel;
  float dwh_ambtemp[MONTHS + 1], tia, dwh_eff;
  float dwhRpre, enpre, enpst;
  float dwh_radded = mdi->key.water_heater_wrap_added_r_value;

  dwhFuel = mdi->dwh.exist_fuel_type_id; // make code easier to read

  if (dwhFuel == 0)  
    return;
  if (mdi->dwh.exist_energy_factor == 0)
    return;   // #270

  mir->flgRetrofits[ndx] = FALSE;

  // start with our standard fuel types

  initialize_fuel_types();

  // then specify the before and after base load fuel types

  mir->Results[mir->Rndx].PreBasFuel = (enum FUEL)mdi->dwh.exist_fuel_type_id;
  mir->Results[mir->Rndx].PstBasFuel = (enum FUEL)mdi->dwh.exist_fuel_type_id;

  // Is mutually exclusive with heater replacement

  if (mir->flgRetrofits[M_CMS_WATER_HEATER_REPLACEMENT] || mdi->dwh.replace == TRUE)
    return;

  // No tank to insulate #270
  if (mdi->dwh.exist_type == WH_TANKLESS)
    return;

  // here is the new code that determines if the
  // rvalue of the insulation is derived from the type
  // and thickness, or directly from the rvalue on the
  // label,  MJF 5/01

  if (mdi->dwh.exist_rvalue > 0.1)
    dwhRpre = mdi->dwh.exist_rvalue;
  else
    dwhRpre = water_heater_insulation_rpi(mdi->dwh.exist_insul_type_id) * mdi->dwh.exist_insul_thick;

  // estimate the dwh surface area using the configuration (determined
  // by fuel type) and the capacity in gallons.

  if (mdi->dwh.exist_gal > 0)
    dwh_gal = mdi->dwh.exist_gal;
  else
    dwh_gal = 30.0f;

  if (dwhFuel == WH_ELECTRIC)
    dwh_area = 5.08f + 0.2975f * dwh_gal;
  else
    dwh_area = 3.92f + 0.2740f * dwh_gal;

  // here are some conditions used to decide if we are going
  // to do the tank wrap

  if (mdi->dwh.exist_tank_wrap == YES) // already insulated
    return;

  if (dwhRpre > 8.0f && mdi->dwh.exist_tank_location_id == HEATED)
    return; // Conditioned, R > 8
  if (dwhRpre > 9.9f && dwhFuel != WH_ELECTRIC)
    return; // Non Electric, R > 9.9
  if (dwhRpre > 12.4f)
    return; // R > 12.4

  tia = (float)((mdi->key.heating_setpoint_day + // new 4/3/02 MJF
                 mdi->key.heating_setpoint_night + mdi->key.cooling_setpoint_day +
                 mdi->key.cooling_setpoint_night) /
                4.0);

  dwh_upre = (float)(1.0f / (0.66 + dwhRpre));
  switch (mdi->dwh.exist_tank_location_id) { // set our monthly dwh ambient temp array
  case UNHEATED: {                           // Nonconditioned
    for (m = 1; m <= MONTHS + 1 - 1; m++)
      //dwh_ambtemp[m] = mir->ftoad[m];
      dwh_ambtemp[m] =  cwd->avg_daytime_temp[m];
    break;
  }
  //    case UNINTENTIONALLY_HEATED:  {                  // Unintentionall Conditoned
  //      for(m=1;m<=MONTHS + 1-1;m++)
  //         dwh_ambtemp[m] = mir->ftoad[m]; break; }
  default: { // Conditioned
    for (m = 1; m <= MONTHS + 1 - 1; m++)
      dwh_ambtemp[m] = tia;
    break;
  }
  }

  enpre = enpst = 0.0f;
  for (m = 1; m <= MONTHS + 1 - 1; m++) {
    enpre += (float)(dwh_area * (dwh_upre) * (DWH_TEMP - dwh_ambtemp[m]) * 24 * cwd->days_in_month[m]);
    enpst += (float)(dwh_area * (1. / (1. / dwh_upre + dwh_radded)) * (DWH_TEMP - dwh_ambtemp[m]) * 24 * cwd->days_in_month[m]);
  }

  dwh_eff = mdi->dwh.exist_energy_factor;

  ASSERT(dwh_eff != 0, sprintf(msg, "Assertion Failure"));

  mir->Results[mir->Rndx].fEnerPreBas = enpre / dwh_eff; //  Btu
  mir->Results[mir->Rndx].fEnerPstBas = enpst / dwh_eff; //  Btu

  no_heating_or_cooling_savings();

  mir->flgRetrofits[ndx] = TRUE;
  STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
  STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

  //sprintf(mir->Results[mir->Rndx].sName, "Water Heater Tank Insulation");
  mir->Results[mir->Rndx].fQuant = 1.0;

  mir->Results[mir->Rndx].measure_id = ndx;
  mir->Results[mir->Rndx].audit_section_id = M_WATER_HEATING;
  mir->Results[mir->Rndx].material_id = iMat;

  mhea_measure_sir(mir->Rndx);

  mir->Rndx++;
  return;
}

/******************* FUNCTION NAME: retro_water_heater_pipe_insulation       ****************/
/**         DATE:  9/24/01                                              **/
/**           BY:  MJF                                                  **/
/**      REVISED:                                                       **/
/**  DESCRIPTION:                                                       **/
/*************************************************************************/
void retro_water_heater_pipe_insulation(void) {
  int ndx  = M_CMS_WATER_HEATER_PIPE_INS;
  int iMat = M_MAT_DWH_PIPE_INS;

  int m, dwhFuel;
  float tempval, dwh_ambtemp[MONTHS + 1], tia;
  // Make constant or have read in
  float dwh_pipe_length = 5.0f, dwh_pipe_diam_in = 0.84f;
  float kins = 0.0225f, ins_thickness_in = 0.5f, ho_ins = 1.35f;
  float ho_unins, tavg, tdelta, kair, dwh_eff;
  //float titemp, tatemp, debug1, ensav;
  float titemp, tatemp, ensav;
  float qconv, qrad, qins, qsaved, r1, r2;

  mir->flgRetrofits[ndx] = FALSE;

  if (mdi->dwh.exist_pipe_insul == YES) // already insulated
    return;

  dwhFuel = mdi->dwh.exist_fuel_type_id; // make code easier to read

  if (dwhFuel == 0)
    return;

  if (mdi->dwh.exist_energy_factor == 0)
    return;   // #270

  // start with our standard fuel types

  initialize_fuel_types();

  // then specify the before and after base load fuel types

  mir->Results[mir->Rndx].PreBasFuel = (enum FUEL)dwhFuel;
  mir->Results[mir->Rndx].PstBasFuel = (enum FUEL)dwhFuel;

  tia = (float)((mdi->key.heating_setpoint_day + mdi->key.heating_setpoint_night +
                 mdi->key.cooling_setpoint_day + mdi->key.cooling_setpoint_night) /
                4.0);

  switch (mdi->dwh.exist_tank_location_id) {
  case UNHEATED: { // Nonconditioned
    for (m = 1; m <= MONTHS + 1 - 1; m++)
      //dwh_ambtemp[m] = mir->ftoad[m];
      dwh_ambtemp[m] = cwd->avg_daytime_temp[m];
    break;
  }
  //    case UNINTENTIONALLY_HEATED:  {       // Unintentionall Conditoned
  //      for(m=1;m<=MONTHS + 1-1;m++)  dwh_ambtemp[m] = mir->ftoad[m]; break; }
  default: { // Conditioned
    for (m = 1; m <= MONTHS + 1 - 1; m++)
      dwh_ambtemp[m] = tia;
    break;
  }
  }

  ensav = 0.0f;
  r1 = dwh_pipe_diam_in / 24.0f;
  r2 = r1 + ins_thickness_in / 12.0f;
  tempval = PI * dwh_pipe_diam_in / 12.0f;
  titemp = (DWH_TEMP + 460.0f) / 100.0f;
  for (m = 1; m <= MONTHS + 1 - 1; m++) {
    tavg = (dwh_ambtemp[m] + DWH_TEMP) / 2.0f;
    tdelta = DWH_TEMP - dwh_ambtemp[m];
    kair = (float)(2.059e-5 * tavg + 0.01334);

    ASSERT(dwh_pipe_length != 0, sprintf(msg, "Assertion Failure"));

    //debug1 = (float)(tdelta / dwh_pipe_length * EXPC(-.008698 * tavg));
    ho_unins = (float)(23.1 * kair * POWC((tdelta / dwh_pipe_length * EXPC(-.008698 * tavg)), 0.25));
    qconv = tempval * ho_unins * tdelta;
    tatemp = (dwh_ambtemp[m] + 460.0f) / 100.0f;
    qrad = (float)(tempval * 0.1714 * 0.8f * (POWC(titemp, 4.0f) - POWC(tatemp, 4.0f)));

    ASSERT(kins != 0, sprintf(msg, "Assertion Failure"));

    qins = (float)(2.0f * PI * tdelta / (log((double)(r2 / r1)) / kins + 1.0f / (r2 * ho_ins)));
    qsaved = qconv + qrad - qins;
    ensav += qsaved * 24 * cwd->days_in_month[m];
  }

  dwh_eff = mdi->dwh.exist_energy_factor;

  ASSERT(dwh_pipe_length != 0, sprintf(msg, "Assertion Failure"));
  ensav /= (float)(dwh_eff / dwh_pipe_length);
  mir->Results[mir->Rndx].fEnerPreBas = ensav; //  Btu
  mir->Results[mir->Rndx].fEnerPstBas = 0.0f;

  no_heating_or_cooling_savings();

  mir->flgRetrofits[ndx] = TRUE;
  STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
  STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

  //sprintf(mir->Results[mir->Rndx].sName, "Water Heater Pipe Insulation");
  mir->Results[mir->Rndx].fQuant = 1.0;

  mir->Results[mir->Rndx].measure_id = ndx;
  mir->Results[mir->Rndx].audit_section_id = M_WATER_HEATING;
  mir->Results[mir->Rndx].material_id = iMat;

  mhea_measure_sir(mir->Rndx);

  mir->Rndx++;

  return;
}

/******************* FUNCTION NAME: retro_show_heads      ****************/
/**         DATE:  9/24/01                                              **/
/**           BY:  MJF                                                  **/
/**      REVISED:                                                       **/
/**  DESCRIPTION:                                                       **/
/*************************************************************************/
void retro_show_heads(void) {
  int ndx  = M_CMS_LOW_FLOW_SHOWERHEADS;
  int iMat = M_MAT_DWH_SHOWERHEAD;

  float shwr_oldW, shwr_newW, shwr_delT_old = 50, shwr_delT_new = 54;
  float dwh_eff;
  //int dwhFuel;
  float shwr_gpm_new = mdi->key.low_flow_shower_head_flow_rate;

  mir->flgRetrofits[ndx] = FALSE;

  if (mdi->dwh.shower_usage_per_day == 0 || mdi->dwh.shower_heads == 0 || mdi->dwh.shower_gpm <= 0.1)
    return;

  if (mdi->dwh.exist_energy_factor == 0)
    return;   // #270

  mir->flgRetrofits[ndx] = TRUE;
  mir->Results[mir->Rndx].PreBasFuel = (enum FUEL)mdi->dwh.exist_fuel_type_id;
  mir->Results[mir->Rndx].PstBasFuel = (enum FUEL)mdi->dwh.exist_fuel_type_id;

  //dwhFuel = mdi->dwh.exist_fuel_type_id; // make code easier to read

  dwh_eff = mdi->dwh.exist_energy_factor;

  shwr_oldW = 365.0f * mdi->dwh.shower_usage_per_day * mdi->dwh.shower_gpm / 1000.0f;

  shwr_newW = 365.0f * mdi->dwh.shower_usage_per_day * shwr_gpm_new / 1000.0f;

  ASSERT(dwh_eff != 0, sprintf(msg, "Assertion Failure"));
  mir->Results[mir->Rndx].fEnerPreBas = shwr_oldW * shwr_delT_old * 8340.0f / dwh_eff;

  mir->Results[mir->Rndx].fEnerPstBas = shwr_newW * shwr_delT_new * 8340.0f / dwh_eff;

  no_heating_or_cooling_savings();

  mir->flgRetrofits[ndx] = TRUE;
  STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
  STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);
  //sprintf(mir->Results[mir->Rndx].sName, "Low-Flow Showerheads");

  mir->Results[mir->Rndx].fQuant = (float)(mdi->dwh.shower_heads);

  mir->Results[mir->Rndx].measure_id = ndx;
  mir->Results[mir->Rndx].audit_section_id = M_WATER_HEATING;
  mir->Results[mir->Rndx].material_id = iMat;

  mhea_measure_sir(mir->Rndx);

  mir->Rndx++;

  return;
}

/******************* FUNCTION NAME: retro_water_heater   ****************/
/**         DATE:  9/24/01                                              **/
/**           BY:  MJF                                                  **/
/**      REVISED:                                                       **/
/**  DESCRIPTION:                                                       **/
/*************************************************************************/
void retro_water_heater(void) {
  int ndx = M_CMS_WATER_HEATER_REPLACEMENT;

 // Constants
  float den = 8.29f;          // water density (lb/gal)
  float Cp = 1.0f;            // specific heat of water (Btu/lb-F)
  float Pon_hp = 0.5f * 3415; // MM:Rated input power of HPWH in heat pump mode [Btu/h]
  float RE_hp = 2.42f;        // MM:Recovery efficiency of HPWH in heat pump mode
  float HP_cap = 3500.0f;     // MM:Cooling capacity of heat pump water heater [Btu/h]

  // Variables needed from input
  float EF[POST_RETROFIT + 1];        // Energy factor of existing[0] and replacement[1] units
  float RE[POST_RETROFIT + 1];        // Recovery efficiency of existing and replacement units
  float Pon[POST_RETROFIT + 1];       // Rated input power of units [Btu/h]
  float Tanksz[POST_RETROFIT + 1];    // Tank size [gal]
  enum WH_EQUIP_TYPE dwh_type[POST_RETROFIT + 1];   // equipment type

  // Values fixed by assumption
  float Tin = 54.0f;    // Inlet water temperature [F]
  float atmp = 70.0f;   // Assumed average annual outdoor temperature

  // Computed variables
  //char water_heater_type[POST_RETROFIT + 1][50];   // determined from fuel type and EF RE values
  float vol;                                // Daily hot water consumption
  float age2;                               // Number of occupants between ages 6 - 13
  float age3;                               // Number of occupants between ages 14 - 64
  float athome;                             // = 0 -> assumes no adult home in day, = 1 -> otherwise;
  float UA[POST_RETROFIT + 1];              // UA for existing and new water heater insulation (computed)  
  //int dwh_fuel[POST_RETROFIT + 1];          // fuel type id for existing and new water heater
  float tia;                                // Average annual indoor air temperature
  float dwh_annual_cons[POST_RETROFIT + 1]; // Annual existing[0] and replacement[1] annual energy consumption in MMBtu
  float dwh_ambtemp[MONTHS + 1];            // Ambient monthly temperature at location of water heater [F]

  // Just for heat pump water heater calculations
  float PA_hp[MONTHS + 1];        // MM:Performance adjustment factor
  float HP_on[MONTHS + 1];        // MM:fraction of time when heat pump is operating
  float HP_frac[MONTHS + 1];      // MM:Fraction of water heating load that is satisfied by heat pump mode
  float temp1, temp2[MONTHS + 1]; // MM:temporary variables
  float temp3[MONTHS + 1], temp4;
  float dwh_clg[MONTHS + 1];      // MM:Space cooling added by heat pump water heater [Btu/day]

  double a1 = 0.00019738;         // MM:Coefficients for performance adjustment due to ambient temperature
  double a2 = 0.01842;
  double a3 = 1.3222625;

  float free_heat = 0;
  // #334 NO, do not restore the mdi!
  //float save_free_heat_from_interior_sources_day = mdi->key.free_heat_from_interior_sources_day;
  //float save_free_heat_from_interior_sources_night = mdi->key.free_heat_from_interior_sources_night;

  mir->flgRetrofits[ndx] = FALSE;

  // Is mutually exclusive with tank wrap

  if (mir->flgRetrofits[M_CMS_WATER_HEATER_TANK_INS])
    return;

  // Here's our check to make sure that we have valid input
  // for the replacment

  if (water_heater_replace_data_check(mdi->dwh) == FALSE)
    return;

  if (mdi->dwh.replace == TRUE) { /* replacement required 9/09 */
    mir->Results[mir->Rndx].measure_required = TRUE;
    if (mdi->dwh.inc_sir == TRUE)
      mir->Results[mir->Rndx].measure_priority = MPS_EQUIP_REQUIRED;    // Issue #157 changed from envelop to equip sorting
    else
      mir->Results[mir->Rndx].measure_priority = MPS_EQUIP_REQUIRED_NO_SIR;
  }

  // dwh_fuel[PRE_RETROFIT] = mdi->dwh.exist_fuel_type_id;
  // dwh_fuel[POST_RETROFIT] = mdi->dwh.replace_fuel_type_id;

  dwh_type[PRE_RETROFIT] = mdi->dwh.exist_type;
  dwh_type[POST_RETROFIT] = mdi->dwh.replace_type;

  // disallow fuel switch from fossil to electric.
  // Eliminated 9/09 - 8.6

  //  if((dwh_fuels[PRE_RETROFIT]== WH_PROPANE || dwh_fuels[PRE_RETROFIT]== WH_NATURAL_GAS) &&
  //      dwh_fuels[POST_RETROFIT]== WH_ELECTRIC) return;

  // allow fuel switch
  // start with our standard fuel types
  initialize_fuel_types();
  mir->Results[mir->Rndx].PreBasFuel = (enum FUEL)mdi->dwh.exist_fuel_type_id;
  mir->Results[mir->Rndx].PstBasFuel = (enum FUEL)mdi->dwh.replace_fuel_type_id;

  // #270 default values now are assigned upstream of the measure

  EF[PRE_RETROFIT]  = mdi->dwh.exist_energy_factor;
  EF[POST_RETROFIT] = mdi->dwh.replace_energy_factor;

  RE[PRE_RETROFIT]  = mdi->dwh.exist_recovery_efficiency;
  RE[POST_RETROFIT] = mdi->dwh.replace_recovery_efficiency;

  Pon[PRE_RETROFIT] = mdi->dwh.exist_input * 1000.0f;
  if (mdi->dwh.exist_input_units_id == WH_KW)
    Pon[PRE_RETROFIT] *= 3.415f;
  Pon[POST_RETROFIT] = mdi->dwh.replace_input * 1000.0f;
  if (mdi->dwh.replace_input_units_id == WH_KW)
    Pon[POST_RETROFIT] *= 3.415f;

  Tanksz[PRE_RETROFIT] = mdi->dwh.exist_gal;
  Tanksz[POST_RETROFIT] = mdi->dwh.replace_gal;

  // Set tank ambiant temperature

  tia = (float)((mdi->key.heating_setpoint_day + // new 4/3/02 MJF
                 mdi->key.heating_setpoint_night + 
                 mdi->key.cooling_setpoint_day +
                 mdi->key.cooling_setpoint_night) /
                4.0);

  switch (mdi->dwh.exist_tank_location_id) {
  case UNHEATED: { // Nonconditioned
    for (int m = 1; m <= MONTHS; m++)
      dwh_ambtemp[m] = cwd->avg_drybulb_temp[m];
    break;
  }
  // NEAT uses a sub basement adjusted monthly temperature, but that is not 
  // appropriate or available for manufactured housing which takes a simple average of outdoor and
  // indoor temperatures as an estimate for the temperature of an unintentionally conditioned space like
  // an enclosed porch MJF 1/2020 #140
  case UNINTENTIONALLY_HEATED: { // Unintentionally Conditoned
    for (int m = 1; m <= MONTHS; m++)
      dwh_ambtemp[m] = (cwd->avg_drybulb_temp[m] + tia) / 2.0;
    break;
  }
  default: { // Conditioned
    for (int m = 1; m <= MONTHS; m++)
      dwh_ambtemp[m] = tia;
    break;
  }
  }

  if (mdi->gnl.avg_no_occupants < 4.0f)
    age3 = mdi->gnl.avg_no_occupants;
  else
    age3 = 3.0f;

  if (mdi->gnl.avg_no_occupants < 4.0f)
    age2 = 0.0f;
  else
    age2 = mdi->gnl.avg_no_occupants - 3.0f;

  if (mdi->gnl.avg_no_occupants < 3.0f)
    athome = 0.0f;
  else
    athome = 1.0f;

  // now do our before/after calculations

  for (int unit = PRE_RETROFIT; unit <= POST_RETROFIT; unit++) {

    // For tankless water heaters, the Tanksz[] factor will be zero

    vol = -1.78f + 0.9744f * mdi->gnl.avg_no_occupants + 10.5178f * age2 + 15.3052f * age3 - 0.1277f * DWH_TEMP +
          0.1437f * Tanksz[unit] - 0.1794f * Tin + 0.5155f * atmp + 10.2191f * athome;

    // Compute the UA's for existing and new water heaters

    // ASSERT(EF[unit] != 0, sprintf(msg, "Need water heater energy factors"));
    // ASSERT(RE[unit] != 0, sprintf(msg, "Need water heater recovery efficiency"));
    // ASSERT(Pon[unit] != 0, sprintf(msg, "Need water heater input energy rating"));

    // MM:Incorporated calculations for heat pump water heater
    // HPWH is identified when EF > RE

    // if (dwh_fuel[unit] == WH_ELECTRIC)
    //   STRCPY(water_heater_type[unit], "Electric");
    // else
    //   STRCPY(water_heater_type[unit], "Gas");

    //if (EF[unit] <= RE[unit] || dwh_fuel[unit] != WH_ELECTRIC) { // Standard water heater

    switch (dwh_type[unit]) {

      case WH_STORAGE:
      case WH_TANKLESS:

        // if (EF[unit] == RE[unit])
        //   STRCAT(water_heater_type[unit], " Tankless");
        // else
        //   STRCAT(water_heater_type[unit], " Standard");

        UA[unit] = (1.0f / EF[unit] - 1.0f / RE[unit]) / 67.5f / (24.0f / 41094.0f - 1.0f / RE[unit] / Pon[unit]);

        // Compute the annual energy consumption for existing and new water heaters.
        // first zero the accumulator

        dwh_annual_cons[unit] = 0;
        for (int m = 1; m <= MONTHS; m++) {
          dwh_annual_cons[unit] +=
              (vol * den * Cp * (DWH_TEMP - Tin) / RE[unit] * (1.0f - UA[unit] * (DWH_TEMP - dwh_ambtemp[m]) / Pon[unit]) +
               24.0f * UA[unit] * (DWH_TEMP - dwh_ambtemp[m])) * cwd->days_in_month[m];
          dwh_clg[m] = 0.0;  // no free heat effects #221
        }

        if (unit == PRE_RETROFIT) {
          mir->Results[mir->Rndx].fEnerPreBas = dwh_annual_cons[unit];
          mir->Results[mir->Rndx].fEnerPreHtg = mir->fPre_Heating;
          mir->Results[mir->Rndx].fEnerPreClg = mir->fPre_Cooling;
        } else {
          mir->Results[mir->Rndx].fEnerPstBas = dwh_annual_cons[unit];
          mir->Results[mir->Rndx].fEnerPstHtg = mir->fPre_Heating;
          mir->Results[mir->Rndx].fEnerPstClg = mir->fPre_Cooling;
        }

        break;

      //} else {    // Heat Pump Water Heater
      case WH_HEAT_PUMP:

        //STRCAT(water_heater_type[unit], " Heat Pump");

        UA[unit] = (1.0f / EF[unit] - 1.0f / RE_hp) / 67.5f / (24.0f / 41094.0f - 1.0f / RE_hp / Pon_hp);

        dwh_annual_cons[unit] = 0;
        for (int m = 1; m <= MONTHS; m++) {
          PA_hp[m] = (float)(a1 * dwh_ambtemp[m] * dwh_ambtemp[m] - a2 * dwh_ambtemp[m] + a3);

          if (dwh_ambtemp[m] > 40)
            HP_on[m] = (1 - vol * den * Cp * (DWH_TEMP - Tin) / 9500 / 100);
          else
            HP_on[m] = 0;

          HP_frac[m] = HP_on[m] * Pon_hp * RE_hp * PA_hp[m] /
                       (HP_on[m] * Pon_hp * RE_hp * PA_hp[m] + (1 - HP_on[m]) * Pon[unit] * RE[unit]);

          temp1 = vol * den * Cp * (DWH_TEMP - Tin);
          temp2[m] = HP_frac[m] / RE_hp / PA_hp[m] * (1.0f - UA[unit] * (DWH_TEMP - dwh_ambtemp[m]) / Pon_hp);
          temp3[m] = (1 - HP_frac[m]) / RE[unit] * (1.0f - UA[unit] * (DWH_TEMP - dwh_ambtemp[m]) / Pon[unit]);
          temp4 = 24 * UA[unit] * (DWH_TEMP - dwh_ambtemp[m]);

          dwh_annual_cons[unit] += (temp1 * (temp2[m] + temp3[m]) + temp4) * cwd->days_in_month[m];

          dwh_clg[m] = HP_cap * temp1 / (Pon_hp * RE_hp * PA_hp[m]);    // free heat effect #221
        }

        // Re run the energy model to compute the effects of changes in the free heat

        // Effects on Free Heat
        // Note there is no effect for standard water heaters or heaters in unconditioned space

        for (int m = 1; m <= MONTHS; m++) {
          if (mdi->dwh.exist_tank_location_id == HEATED)
            free_heat += (float)(dwh_clg[m] / 24.0f);   // in Btu/hr average for day
        }
        free_heat /= 12.0;  // average Btu/hr for year

        // HP WHs remove heat from the space so you get a heating penaly and a cooling benefit
        mdi->key.free_heat_from_interior_sources_day -= free_heat;
        mdi->key.free_heat_from_interior_sources_night -= free_heat;

        /***********************************
        MBG 10/2/08 - Call mhea_energy_use() to compute the energy
        saved by a single door description.
        ***********************************/

        mhea_energy_use();

        // Restore our original key parameter values
        // #334  NO we do NOT want to step back from the key parameter changes
        // because that is the alteration to the building description for the
        // implemented measure and it needs to carry forward in the cumulative pass.
        // If the measure is to be backed out because
        // it is the first pass or because the measure is not cost effective
        // in the cumulative pass, then the mdi copy/restore will take care of this
        //mdi->key.free_heat_from_interior_sources_day = save_free_heat_from_interior_sources_day;
        //mdi->key.free_heat_from_interior_sources_night = save_free_heat_from_interior_sources_night;

        if (unit == PRE_RETROFIT) {
          mir->Results[mir->Rndx].fEnerPreBas = dwh_annual_cons[unit];
          mir->Results[mir->Rndx].fEnerPreHtg = mir->fHeating_Energy;
          mir->Results[mir->Rndx].fEnerPreClg = mir->fCooling_Energy;
        } else {
          mir->Results[mir->Rndx].fEnerPstBas = dwh_annual_cons[unit];
          mir->Results[mir->Rndx].fEnerPstHtg = mir->fHeating_Energy;
          mir->Results[mir->Rndx].fEnerPstClg = mir->fCooling_Energy;
        }
        break;

      default:
        ASSERT(FALSE, sprintf(msg, "Unrecognized water heater type: %d",dwh_type[unit]));
        break;
    }

    // if (cmds.debug_level & D_NORMAL) {
    //   fprintf(stderr, "\nWATER HEATER: %d", unit);        
    //   fprintf(stderr, "\nUA[unit]: %8.3f", UA[unit]);
    //   fprintf(stderr, "\ndwh_annual_cons[unit]: %8.3f", dwh_annual_cons[unit]);
    //   fprintf(stderr, "\nfree_heat: %8.3f", free_heat);
    // }

  }   // pre and post

  // Material cost and life now comes directly from the input
  // and our material list for this and all other 'equipment library'
  // items goes into the other_materials array

  mir->flgRetrofits[ndx] = TRUE; // Evaluate measure
  STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
  //STRCPY(mir->Results[mir->Rndx].sName, "Water Heater Replacement ");
  STRCPY(mir->Results[mir->Rndx].sMaterial, "Water Htr");
  STRCPY(mir->Results[mir->Rndx].sUnits, "Ea ");
  mir->Results[mir->Rndx].fQuant = 1;
  mir->Results[mir->Rndx].fLife = (float)(mdi->dwh.replace_life);
  mir->Results[mir->Rndx].fInitCost = mdi->dwh.replace_install_cost + mdi->dwh.replace_added_cost;

  mir->Results[mir->Rndx].measure_id = ndx;
  mir->Results[mir->Rndx].audit_section_id = M_WATER_HEATING;
  mir->Results[mir->Rndx].material_id = M_MAT_FROM_AUDIT;

  mir->Results[mir->Rndx].costi2 = mdi->dwh.replace_install_cost;
  mir->Results[mir->Rndx].typei2 = MT_HOT_WATER_EQUIPMENT;
  STRCPY(mir->Results[mir->Rndx].desci2, mdi->dwh.replace_manufacturer);
  STRCAT(mir->Results[mir->Rndx].desci2, " - ");
  STRCAT(mir->Results[mir->Rndx].desci2, mdi->dwh.replace_model);

  mir->Results[mir->Rndx].costi3 = mdi->dwh.replace_added_cost;
  mir->Results[mir->Rndx].typei3 = MT_LABOR;
  STRCPY(mir->Results[mir->Rndx].desci3, "Installation Labor");

  mhea_measure_sir(mir->Rndx); // compute the BCR

  mir->Rndx++;

  return;
}

/******************* FUNCTION NAME: retro_window_sealing    ****************/
/**         DATE:  8/15/06                                              **/
/**           BY:  MBG                                                  **/
/**      REVISED:                                                       **/
/**  DESCRIPTION:  Decreases the leakage coefficient of a window        **/
/*************************************************************************/
void retro_window_sealing(void) {
  int ndx  = M_CMS_WINDOW_SEALING;
  int iMat = M_MAT_WINDOW_SEALING;

  float fNumWinTotal;
  MDI *original = NULL;

  mir->flgRetrofits[ndx] = FALSE;

  if(mir->flgWhichPass != CUMULATIVE){
    copy_mdi(&original, mdi); // Save original mdi structure, allocating memory for original
  }

  /***************
  Deterimine the infiltration savings from the window replacement measure
  separately so that it can be subtracted from the general infiltration
  savings during the final tally of savings. Do not permanently change
  the window characteristics in this loop.
  ***************/

  if (mir->flgWhichPass == CUMULATIVE && mir->Rndx != 0) {

    /****************************************************************************
    First loop through window descriptions to determe air leakage reduction,
    Cumulative pass for specific window related to instance of measure.
    ****************************************************************************/

    for (int i = 0; i < mdi->num_win; i++) {
      if (!comma_delimited_strstr(mir->sComponents, mdi->win[i].code))
        continue;
      else if (mdi->win[i].imeas_applied != 0)
        continue; // Prevent more than one measure per window  MBG 4/07
      else {
        /***************************
        Get the total number of windows of this window type.
        **************************/

        fNumWinTotal = mdi->win[i].num_n + mdi->win[i].num_s + mdi->win[i].num_e + mdi->win[i].num_w;

        if (fNumWinTotal > 0.0) {

          /*************************
          Reset window leakage rating
          *************************/
          // fleakcoef = mdi->win[i].leak_coef;
          // mdi->win[i].leak_coef = mdi->win[i].leak_coef - SEALING_FRACTION * (mdi->win[i].leak_coef - SEALED_WINDOW_LEAKAGE_COEF);
          // if (mdi->win[i].leak_coef > fleakcoef)
          //   mdi->win[i].leak_coef = fleakcoef;
          float save_leak_coef = mdi->win[i].leak_coef;
          mdi->win[i].leak_coef = window_sealing_leak_coef(save_leak_coef);

          /***********************************
          MBG 3/11/04 - Call mhea_energy_use() first with only window
          leakage changed in order to determine energy savings
          which should be attributed to this measure instead of
          general infiltration reduction.
          ***********************************/

          mir->flgWhichPass = NOT_BASE_CASE;
          mhea_energy_use();
          mir->flgWhichPass = CUMULATIVE;
          // mir->fInfMeasEnrgyHtg[ndx] += mir->Results[mir->Rndx - 1].fEnerPstHtg - mir->fHeating_Energy * mir->fAdj_Htg;
          // mir->fInfMeasEnrgyClg[ndx] += mir->Results[mir->Rndx - 1].fEnerPstClg - mir->fCooling_Energy * mir->fAdj_Clg;
          mir->fInfMeasEnrgyHtg[ndx] += mir->fPre_Heating - mir->fHeating_Energy * mir->fAdj_Htg;
          mir->fInfMeasEnrgyClg[ndx] += mir->fPre_Cooling - mir->fCooling_Energy * mir->fAdj_Clg;
          mdi->win[i].leak_coef = save_leak_coef;
        }
      }

    } // End loop through window descriptions

    if (cmds.debug_level & D_MHEA_WIN_DETAIL) {
      //fprintf(stderr, "fEnerPreHtg(kBtu)  = %6.2f\n", mir->Results[mir->Rndx - 1].fEnerPstHtg/1000);
      fprintf(stderr, "fEnerPreHtg(kBtu)      = %6.2f\n", mir->fPre_Heating/1000);
      fprintf(stderr, "fCurrent_Heating(kBtu) = %6.2f\n", mir->fHeating_Energy/1000);
      fprintf(stderr, "InfEnrgyHtgSaved(kBtu) = %6.2f\n", mir->fInfMeasEnrgyHtg[ndx]/1000);
      fprintf(stderr, "InfEnrgyClgSaved(kBtu) = %6.2f\n", mir->fInfMeasEnrgyClg[ndx]/1000);
    }
  }

  /****************************************
  Second loop through window descriptions
  ****************************************/

  for (int i = 0; i < mdi->num_win; i++) {
    if (mir->flgWhichPass == CUMULATIVE) {
      /*************************
      If cumulative pass, execute for only window description associated
      with instance of measure
      **************************/

      if (!comma_delimited_strstr(mir->sComponents, mdi->win[i].code))
        continue;
      if (mdi->win[i].imeas_applied != 0)
        continue; // Prevent more than one measure per window MBG 4/07
    }

    else { // mir->flgWhichPass = FIRST_PASS

      /*************************
      Get the type of window (flat window, bay window, etc.) and the type
      of glazing (double, single, etc.).  If it is not a sliding glass
      door or door window, then this retrofit will be implemented for
      that window.
      **************************/

      if (mdi->win[i].window_type == GLASSDOORMHEA || mdi->win[i].window_type == DOORWINMHEA ||
          mdi->win[i].window_type == SKYLIGHTMHEA)
        continue;

      if (mdi->win[i].retrofit_option != MWS_EVALUATE_ALL && mdi->win[i].retrofit_option != MWS_SEAL)
        continue;

    } // End mir->flgWhichPass = FIRST_PASS only execution

    append_component_code_to_current_results(mdi->win[i].code);

    /***************************
    Get the total number of windows of this window type.
    **************************/
    fNumWinTotal = mdi->win[i].num_n + mdi->win[i].num_s + mdi->win[i].num_e + mdi->win[i].num_w;

    if (fNumWinTotal > 0.0) {

      if (mir->flgWhichPass == FIRST_PASS)
        copy_mdi(&mdi, original); // Get copy of original mdi

      /*************************
      Set the new leakage coefficient for window
      *************************/

      // fleakcoef = mdi->win[i].leak_coef;
      // mdi->win[i].leak_coef = mdi->win[i].leak_coef - SEALING_FRACTION * (mdi->win[i].leak_coef - SEALED_WINDOW_LEAKAGE_COEF);
      // if (mdi->win[i].leak_coef > fleakcoef)
      //   mdi->win[i].leak_coef = fleakcoef;
      float save_leak_coef = mdi->win[i].leak_coef;
      mdi->win[i].leak_coef = window_sealing_leak_coef(save_leak_coef);

      /***********************************
      MBG 3/11/04 - Call mhea_energy_use() to compute the energy
      saved by a single window description.
      ***********************************/

      mhea_energy_use();
      save_energy_results();

      if (mir->flgWhichPass == CUMULATIVE) {
        // Prevent multiple retrofit measures from being applied to the same window  MBG 4/07
        mdi->win[i].imeas_applied = M_CMS_WINDOW_SEALING;
      } else {
        mdi->win[i].leak_coef = save_leak_coef;
      }

      initialize_fuel_types();

      mir->flgRetrofits[ndx] = TRUE;
      STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
      STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

      //STRCPY(mir->Results[mir->Rndx].sName, "Window Sealing");
      mir->Results[mir->Rndx].fQuant += fNumWinTotal;
      mir->Results[mir->Rndx].fReportQuant += fNumWinTotal;
      STRCPY(mir->Results[mir->Rndx].sReportUnits, "Each");

      mir->Results[mir->Rndx].measure_id = ndx;
      mir->Results[mir->Rndx].audit_section_id = M_WINDOW;
      mir->Results[mir->Rndx].material_id = iMat;

      additional_cost(mdi->win[i].cost_seal * fNumWinTotal);

      mhea_measure_sir(mir->Rndx); // assign the BCR
      mir->Results[mir->Rndx].flgGetBCR = 1;
      // Set global flag so that measure will receive 0.6 adjustment

      // If measure chosen from retrofit options combo box,
      // make it required.

      if (mdi->win[i].retrofit_option == MWS_SEAL) {
        mir->Results[mir->Rndx].measure_required = TRUE;
        if (mdi->win[i].inc_sir == TRUE)
          mir->Results[mir->Rndx].measure_priority = MPS_ENVELOPE_REQUIRED;
        else
          mir->Results[mir->Rndx].measure_priority = MPS_ENVELOPE_REQUIRED_NO_SIR;
      }

      // if we have implemented this measure in the main part of
      // the house, then increment our instance counter

      if (mir->Results[mir->Rndx].fQuant > 0.0)
        mir->Rndx++;
      // GKA Free the weather data
      //if (WeatherData) {free(WeatherData); WeatherData = NULL;}

    }
  } // End loop through window descriptions

  // GKA/MJF Issue #83
  if(mir->flgWhichPass != CUMULATIVE){
    copy_mdi(&mdi, original); // back to original MDI structure
    free(original);           // free memory allocated in this procedure
  }

  return;
}

/******************* FUNCTION NAME: retro_window_sealing_add ***************/
/**         DATE:  8/15/06                                              **/
/**           BY:  MBG                                                  **/
/**      REVISED:                                                       **/
/**  DESCRIPTION:  Decreases the leakage coefficient of a windows in    **/
/**                    addition                                         **/
/*************************************************************************/
void retro_window_sealing_add(void) {
  int ndx  = M_CMS_WINDOW_SEALING_ADD;
  int iMat = M_MAT_WINDOW_SEALING_ADD;

  float fNumWinTotal;
  MDI *original = NULL;

  mir->flgRetrofits[ndx] = FALSE;

  if(mir->flgWhichPass != CUMULATIVE){
    copy_mdi(&original, mdi); // Save original mdi structure, allocating memory for original
  }

  /***************
  Deterimine the infiltration savings from the window sealing measure
  separately so that it can be subtracted from the general infiltration
  savings during the final tally of savings. Do not permanently change
  the window characteristics in this loop.
  ***************/

  if (mir->flgWhichPass == CUMULATIVE && mir->Rndx != 0) {

    /****************************************************************************
    First loop through window descriptions to determe air leakage reduction,
    Cumulative pass for specific window related to instance of measure.
    ****************************************************************************/

    for (int i = 0; i < mdi->num_awn; i++) {
      if (!comma_delimited_strstr(mir->sComponents, mdi->awn[i].code))
        continue;
      else if (mdi->awn[i].imeas_applied != 0)
        continue; // Prevent more than one measure per window  MBG 4/07
      else {
        /***************************
        Get the total number of windows of this window type.
        **************************/
        fNumWinTotal = mdi->awn[i].num_n + mdi->awn[i].num_s + mdi->awn[i].num_e + mdi->awn[i].num_w;

        if (fNumWinTotal > 0.0) {

          /*************************
          Reset window leakage rating #255
          *************************/
          // fleakcoef = mdi->awn[i].leak_coef;
          // mdi->awn[i].leak_coef = mdi->awn[i].leak_coef - SEALING_FRACTION * (mdi->awn[i].leak_coef - SEALED_WINDOW_LEAKAGE_COEF);
          // if (mdi->awn[i].leak_coef > fleakcoef)
          //   mdi->awn[i].leak_coef = fleakcoef;
          float save_leak_coef = mdi->awn[i].leak_coef;
          mdi->awn[i].leak_coef = window_sealing_leak_coef(save_leak_coef);

          /***********************************
          MBG 3/11/04 - Call mhea_energy_use() first with only window
          leakage changed in order to determine energy savings
          which should be attributed to this measure instead of
          general infiltration reduction.
          ***********************************/

          mir->flgWhichPass = NOT_BASE_CASE;
          mhea_energy_use();
          mir->flgWhichPass = CUMULATIVE;
          // mir->fInfMeasEnrgyHtg[ndx] += mir->Results[mir->Rndx - 1].fEnerPstHtg - mir->fHeating_Energy * mir->fAdj_Htg;
          // mir->fInfMeasEnrgyClg[ndx] += mir->Results[mir->Rndx - 1].fEnerPstClg - mir->fCooling_Energy * mir->fAdj_Clg;
          mir->fInfMeasEnrgyHtg[ndx] += mir->fPre_Heating - mir->fHeating_Energy * mir->fAdj_Htg;
          mir->fInfMeasEnrgyClg[ndx] += mir->fPre_Cooling - mir->fCooling_Energy * mir->fAdj_Clg;
          mdi->awn[i].leak_coef = save_leak_coef;
        }
      }
    } // End loop through window descriptions

    if (cmds.debug_level & D_MHEA_WIN_DETAIL) {
      //fprintf(stderr, "fEnerPreHtg(kBtu)  = %6.2f\n", mir->Results[mir->Rndx - 1].fEnerPstHtg/1000);
      fprintf(stderr, "fEnerPreHtg(kBtu)      = %6.2f\n", mir->fPre_Heating/1000);
      fprintf(stderr, "fCurrent_Heating(kBtu) = %6.2f\n", mir->fHeating_Energy/1000);
      fprintf(stderr, "InfEnrgyHtgSaved(kBtu) = %6.2f\n", mir->fInfMeasEnrgyHtg[ndx]/1000);
      fprintf(stderr, "InfEnrgyClgSaved(kBtu) = %6.2f\n", mir->fInfMeasEnrgyClg[ndx]/1000);
    }
  }

  /****************************************
  Second loop through window descriptions
  ****************************************/

  for (int i = 0; i < mdi->num_awn; i++) {
    if (mir->flgWhichPass == CUMULATIVE) {
      /*************************
      If cumulative pass, execute for only window description associated
      with instance of measure
      **************************/

      if (!comma_delimited_strstr(mir->sComponents, mdi->awn[i].code))
        continue;
      if (mdi->awn[i].imeas_applied != 0)
        continue; // Prevent more than one measure per window MBG 4/07
    }

    else { // mir->flgWhichPass = FIRST_PASS

      /*************************
      Get the type of window (flat window, bay window, etc.) and the type
      of glazing (double, single, etc.).  If it is not a sliding glass
      door or door window, then this retrofit will be implemented for
      that window.
      **************************/

      if (mdi->awn[i].window_type == GLASSDOORMHEA || mdi->awn[i].window_type == DOORWINMHEA ||
          mdi->awn[i].window_type == SKYLIGHTMHEA)
        continue;

      if (mdi->awn[i].retrofit_option != MWS_EVALUATE_ALL && mdi->awn[i].retrofit_option != MWS_SEAL)
        continue;

    } // End mir->flgWhichPass = FIRST_PASS only execution

    append_component_code_to_current_results(mdi->awn[i].code);

    /***************************
    Get the total number of windows of this window type.
    **************************/
    fNumWinTotal = mdi->awn[i].num_n + mdi->awn[i].num_s + mdi->awn[i].num_e + mdi->awn[i].num_w;

    if (fNumWinTotal > 0.0) {

      if (mir->flgWhichPass == FIRST_PASS)
        copy_mdi(&mdi, original); // Get copy of original mdi

      /*************************
      Set the new leakage coefficient for window
      *************************/

      // fleakcoef = mdi->awn[i].leak_coef;
      // mdi->awn[i].leak_coef = mdi->awn[i].leak_coef - SEALING_FRACTION * (mdi->awn[i].leak_coef - SEALED_WINDOW_LEAKAGE_COEF);
      // if (mdi->awn[i].leak_coef > fleakcoef)
      //   mdi->awn[i].leak_coef = fleakcoef;
      float save_leak_coef = mdi->awn[i].leak_coef;
      mdi->awn[i].leak_coef = window_sealing_leak_coef(save_leak_coef);

      /***********************************
      MBG 3/11/04 - Call mhea_energy_use() to compute the energy
      saved by a single window description.
      ***********************************/

      mhea_energy_use();
      save_energy_results();

      if (mir->flgWhichPass == CUMULATIVE) {
        // Prevent multiple retrofit measures from being applied to the same window  MBG 4/07
        mdi->awn[i].imeas_applied = M_CMS_WINDOW_SEALING_ADD;
      } else {
        mdi->awn[i].leak_coef = save_leak_coef;
      }

      initialize_fuel_types();

      mir->flgRetrofits[ndx] = TRUE;
      STRCPY(mir->Results[mir->Rndx].sName, mdi->cms[ndx].measure_name);
      STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->rmc[iMat].retro_name);

      //STRCPY(mir->Results[mir->Rndx].sName, "Window Sealing [Addition]");
      mir->Results[mir->Rndx].fQuant += fNumWinTotal;
      mir->Results[mir->Rndx].fReportQuant += fNumWinTotal;
      STRCPY(mir->Results[mir->Rndx].sReportUnits, "Each");

      mir->Results[mir->Rndx].measure_id = ndx;
      mir->Results[mir->Rndx].audit_section_id = M_WINDOW_ADDITION;
      mir->Results[mir->Rndx].material_id = iMat;

      additional_cost(mdi->awn[i].cost_seal * fNumWinTotal);

      mhea_measure_sir(mir->Rndx); // assign the BCR
      mir->Results[mir->Rndx].flgGetBCR = 1;
      // Set global flag so that measure will receive 0.6 adjustment

      // If measure chosen from retrofit options combo box,
      // make it required.

      if (mdi->awn[i].retrofit_option == MWS_SEAL) {
        mir->Results[mir->Rndx].measure_required = TRUE;
        if (mdi->awn[i].inc_sir == TRUE)
          mir->Results[mir->Rndx].measure_priority = MPS_ENVELOPE_REQUIRED;
        else
          mir->Results[mir->Rndx].measure_priority = MPS_ENVELOPE_REQUIRED_NO_SIR;
      }

      // if we have implemented this measure in the main part of
      // the house, then increment our instance counter

      if (mir->Results[mir->Rndx].fQuant > 0.0)
        mir->Rndx++;
      //if (WeatherData) {free(WeatherData); WeatherData = NULL;}

    }

  } // End loop through window descriptions

  // GKA/MJF Issue #83
  if(mir->flgWhichPass != CUMULATIVE){
    copy_mdi(&mdi, original); // back to original MDI structure
    free(original);           // free memory allocated in this procedure
  }

  return;
}
/******************* FUNCTION NAME: retro_itemized         ****************/
/**         DATE:  9/24/01                                              **/
/**           BY:  MJF                                                  **/
/**      REVISED:                                                       **/
/**  DESCRIPTION:  Add our itemized costs to mor array           **/
/*************************************************************************/
void retro_itemized(void) {
  int ndx = M_CMS_ITEMIZED_COST;
  int life;
  float fEnrgyconv;
  int fuel_type;

  for (int i = 0; i < mdi->num_itc; i++) {

    initialize_fuel_types();
    STRCPY(mir->Results[mir->Rndx].sName, mdi->itc[i].measure);
    // STRCAT(mir->Results[mir->Rndx].sName, "[item]");
    mir->Results[mir->Rndx].fQuant = 1;
    STRCPY(mir->Results[mir->Rndx].sMaterial, mdi->itc[i].material);
    life = ROUND(mdi->itc[i].life);
    mir->Results[mir->Rndx].fLife = (float)(life);
    //mir->Results[mir->Rndx].measure_id = mdi->itc[i].measure_id;   TODO figure out user defined measures

    mir->Results[mir->Rndx].measure_id = ndx;
    mir->Results[mir->Rndx].audit_section_id = M_ITEMIZED_COST;
    mir->Results[mir->Rndx].material_id = M_MAT_FROM_AUDIT;
    mir->Results[mir->Rndx].component_id = mdi->itc[i].component_id;

    // We will always lump the cost into costi2  Issue #12 MJF 1/20
    //if (mdi->itc[i].measure_id == 0) {
      mir->Results[mir->Rndx].costi2 = mdi->itc[i].cost;
      //mir->Results[mir->Rndx].typei2 = MT_ITEMIZED_MEASURE_COST;
      mir->Results[mir->Rndx].typei2 = MT_NONE;
      if (strlen(mdi->itc[i].material))
        STRCPY(mir->Results[mir->Rndx].desci2, mdi->itc[i].material);
      else
        STRCPY(mir->Results[mir->Rndx].desci2, "Itemized Material");
    //}

    // all itemized cost items are required, but only those marked
    // with inc_sir perk to the top of the list
    // user-defined items do not go to top of list and are not required (MGB 5/06)
    // all itemized records should have a cost figure

    mir->Results[mir->Rndx].measure_required = TRUE;
    mir->Results[mir->Rndx].fInitCost = mdi->itc[i].cost;

    if (mdi->itc[i].inc_sir == YES) {
      if (mdi->itc[i].savings == 0)
        mir->Results[mir->Rndx].measure_priority = MPS_ITEMIZED_NO_SAVE_SIR;
      else
        mir->Results[mir->Rndx].measure_priority = MPS_ITEMIZED_W_SAVE_SIR; // include in total SIR calc

      // place itemized costs first
      mir->Results[mir->Rndx].material_id = M_MAT_FROM_AUDIT;
    } else {
      mir->Results[mir->Rndx].measure_priority = MPS_ITEMIZED_NO_SIR;      // exclude from total SIR calc
      mir->Results[mir->Rndx].material_id = N_MAT_FROM_AUDIT;
    }

    if (mdi->itc[i].savings != 0) {
      mir->Results[mir->Rndx].measure_required = FALSE;
      fEnrgyconv = 1.0f;
      if (mdi->itc[i].units == EN_THERMS)
        fEnrgyconv = THERM_PER_MMBTU; // therms to MMBtu
      if (mdi->itc[i].units == EN_MMBTU)
        fEnrgyconv = 1.0f; // MMBtu to MMBtu
      if (mdi->itc[i].units == EN_KWH)
        fEnrgyconv = MMBTU_PER_KWH; // kWh to MMBtu

      // we may have a generic fuel type that needs
      // to be set to one of our standard fuel types

      fuel_type = mdi->itc[i].fuel; // base ONE here
      if (fuel_type == PRIMARY_HEAT)
        fuel_type = mdi->htg.fuel_type;
      if (fuel_type == WATER_HEAT)
        fuel_type = mdi->dwh.exist_fuel_type_id;
      if (fuel_type == 0)
        fuel_type = NATURAL_GAS; // just so ifl is never undefined

      mir->Results[mir->Rndx].fEnerSavTot = mdi->itc[i].savings * fEnrgyconv;
      mir->Results[mir->Rndx].fCostAnnSavTot = mir->Results[mir->Rndx].fEnerSavTot * fuel_cost_per_mmbtu(fuel_type);
      mir->Results[mir->Rndx].fCostSavTot = mir->Results[mir->Rndx].fEnerSavTot * pw_fuel_cost(fuel_type, life);

      ASSERT(mir->Results[mir->Rndx].fInitCost != 0, sprintf(msg, "Assertion Failure"));

      mir->Results[mir->Rndx].fBCR = calculate_sir(mir->Results[mir->Rndx].fCostSavTot,
                                                   mir->Results[mir->Rndx].fInitCost); // SIR calculation

      mir->Results[mir->Rndx].measure_priority = MPS_SIR; // SIR sort into list of measures
    } else {
      mir->Results[mir->Rndx].fEnerSavTot = 0;
      mir->Results[mir->Rndx].fCostSavTot = 0;
      mir->Results[mir->Rndx].fBCR = 0;
    }

    mir->Rndx++;
  }
}

/***************************************************************************
** Function Name: ceiling_duct_volume
**          Date: December 29, 1998
**     Author(s): Mark Fishbaugher
**
**  DESCRIPTION: Just a simple calculation I saw repeated lots of places
**  so I decided to make it a function
**************************************************************************/
static float ceiling_duct_volume(void) {
  float fDuctsPresent = 0.0F;    // flag
  float fDuctAreaSingle = 0.89F; // X-sectional area, sf
  float fDuctAreaDouble = 1.78F; // X-sectional area, sf

  // first see if there are any ducts -- we return zero of no
  // ducts are present

  if (mdi->htg.duct_location == DL_CEILING || mdi->clg.duct_location == DL_CEILING)
    fDuctsPresent = 1.0;

  // duct area depends on weather the home is a single
  // or double wide.

  if (mdi->gnl.width <= 20.0)
    return (fDuctsPresent * fDuctAreaSingle * mdi->gnl.length);
  else
    return (fDuctsPresent * fDuctAreaDouble * mdi->gnl.length);
}

/***************************************************************************
** Function Name: floor_duct_volume
**          Date: December 29, 1998
**     Author(s): Mark Fishbaugher
**
**  DESCRIPTION: Just a simple calculation I saw repeated lots of places
**  so I decided to make it a function
**************************************************************************/
static float floor_duct_volume(void) {
  float fDuctsPresent = 0.0F;    // flag
  float fDuctAreaSingle = 0.89F; // X-sectional area, sf
  float fDuctAreaDouble = 1.78F; // X-sectional area, sf

  // first see if there are any ducts -- we return zero of no
  // ducts are present

  if (mdi->htg.duct_location == DL_FLOOR || mdi->clg.duct_location == DL_FLOOR)
    fDuctsPresent = 1.0;

  // duct area depends on weather the home is a single
  // or double wide.

  if (mdi->gnl.width <= 20.0)
    return (fDuctsPresent * fDuctAreaSingle * mdi->gnl.length);
  else
    return (fDuctsPresent * fDuctAreaDouble * mdi->gnl.length);
}

/***************************************************************************
** Function Name: initialize_fuel_types
**          Date: 9/17/01
**     Author(s): Mark Fishbaugher
**
**  DESCRIPTION: Initializes the list of before and after retrofit
**  fuel types for heating, cooling, and baseload.  Each retrofit
**  function should call this routine, then make changes if some
**  fuel switching is to take place
**************************************************************************/
static void initialize_fuel_types() {
  mir->Results[mir->Rndx].PreHtgFuel = mdi->htg.fuel_type;
  mir->Results[mir->Rndx].PstHtgFuel = mdi->htg.fuel_type;
  mir->Results[mir->Rndx].PreClgFuel = ELECTRIC;
  mir->Results[mir->Rndx].PstClgFuel = ELECTRIC;
  mir->Results[mir->Rndx].PreBasFuel = ELECTRIC;
  mir->Results[mir->Rndx].PstBasFuel = ELECTRIC;
}

static void save_energy_results() {
  mir->Results[mir->Rndx].fEnerPreHtg = mir->fPre_Heating;
  mir->Results[mir->Rndx].fEnerPstHtg = mir->fHeating_Energy;
  mir->Results[mir->Rndx].fEnerPreClg = mir->fPre_Cooling;
  mir->Results[mir->Rndx].fEnerPstClg = mir->fCooling_Energy;

  if (mir->flgWhichPass == CUMULATIVE) {
    mir->Results[mir->Rndx].fEnerPstHtg *= mir->fAdj_Htg;
    mir->Results[mir->Rndx].fEnerPstClg *= mir->fAdj_Clg;
    mir->fPre_Heating = mir->Results[mir->Rndx].fEnerPstHtg;    // increment for next measure WITHIN a multi-measure loop retrofit
    mir->fPre_Cooling = mir->Results[mir->Rndx].fEnerPstClg;
  }
}

static void no_heating_or_cooling_savings() {
  mir->Results[mir->Rndx].fEnerPreHtg = mir->fPre_Heating;
  mir->Results[mir->Rndx].fEnerPstHtg = mir->fPre_Heating;
  mir->Results[mir->Rndx].fEnerPreClg = mir->fPre_Cooling;
  mir->Results[mir->Rndx].fEnerPstClg = mir->fPre_Cooling;
}

static void additional_cost(float cost) {
  mir->Results[mir->Rndx].fAddCost = cost;      // in the SIR calculation
  mir->Results[mir->Rndx].costi2 = cost;        // only for disaggregated cost reporting (not double counted)
  if (cost > 0.0f) {
    //mir->Results[mir->Rndx].typei2 = MT_ADDED_COST_FROM_AUDIT_FORM;
    mir->Results[mir->Rndx].typei2 = MT_MISCELLANEOUS_SUPPLIES;
    STRCPY(mir->Results[mir->Rndx].desci2, "Additional Cost");
  }
}

static void append_component_code_to_current_results(char *code){
  if (strlen(mir->Results[mir->Rndx].sComponents))
    STRNCAT(mir->Results[mir->Rndx].sComponents, COMMA, 1);
  STRCAT(mir->Results[mir->Rndx].sComponents, code);
}

