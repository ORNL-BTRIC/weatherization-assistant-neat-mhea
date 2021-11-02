/***************************************************************************
* MODULE:   infiltration.c            CREATED:    March 5 2019
*
* AUTHOR:   Mark Fishbaugher  Fishbaugher and Associates  1999
*
* MDESC:    Calculations for ducts and infiltration
****************************************************************************/

#include "wa_engine.h"

static float limit_to_zero(float value);

/***************************************************************************
 ** Function Name: limit_to_zero
 **          Date: March 4, 2019
 **     Author(s): Mark Fishbaugher
 **
 **  DESCRIPTION: To dry out some code that limits negative values to zero
 **************************************************************************/
static float limit_to_zero(float value) { return (value < 0.0f) ? 0.0f : value; }

/***************************************************************************
 ** Function Name: duct_leakage_NEAT
 **          Date: July 27, 2000
 **     Author(s): Mike Gettings
 **      Refactor: 3/5/2019 MJF
 **
 **  DESCRIPTION: assignments from bld structure for duct leakage
 **  and whole house infiltration, also computes estimate of
 **  pre/post duct leakage
 **************************************************************************/
void duct_leakage_neat(void) {

  if (ndi->inf.pre_inf_pa == 0.0f && ndi->inf.pre_inf_cfm > .01) // handle case where pre cfm is filled in but not pre-pa
    ndi->inf.pre_inf_pa = BASE_PRESSURE;                         // default if missing

  if (ndi->inf.evaluate_duct_sealing == YES && ndi->inf.duct_seal_method != DUCT_BLOWER) {
    // no reassign necessary
  } else {
    ndi->inf.post_duct_seal_cfm = ndi->inf.pre_inf_cfm;
    ndi->inf.post_duct_seal_pa = ndi->inf.pre_inf_pa;
  }

  switch (ndi->inf.duct_seal_method) {

    case PRE_POST_WHOLE:   // Pre/Post Whole House Blower-Door Measurements

      // Determine duct leakage reduction as difference in house readings
      // Form pre/post duct leakages based on assumed post sealed leakage
      ASSERT(ndi->inf.pre_inf_pa, sprintf(msg, "Need non zero pressure for cfm correction"));
      ASSERT(ndi->inf.post_duct_seal_pa, sprintf(msg, "Need non zero pressure for cfm correction"));

      ndi->inf.pre_duct_50_cfm =
          limit_to_zero(DEFAULT_CFM_POST_SEALED_DUCT_LEAKAGE_50 +
                        blower_door_corrected_cfm((BASE_PRESSURE / ndi->inf.pre_inf_pa), ndi->inf.pre_inf_cfm) -
                        duct_blower_corrected_cfm((BASE_PRESSURE / ndi->inf.post_duct_seal_pa), ndi->inf.post_duct_seal_cfm));

      ndi->inf.post_duct_50_cfm = DEFAULT_CFM_POST_SEALED_DUCT_LEAKAGE_50;
      break;

    case BLOWER_SUBTRACT:  // Blower-Door Subtraction Method

      ASSERT(ndi->inf.pre_inf_pa, sprintf(msg, "Need non zero pressure for cfm correction"));
      ndi->inf.pre_duct_50_cfm = limit_to_zero(
          (blower_door_corrected_cfm((BASE_PRESSURE / ndi->inf.pre_inf_pa), ndi->inf.pre_inf_cfm) -
           duct_blower_corrected_cfm((ndi->inf.pre_duct_seal_close_pa / BASE_PRESSURE), ndi->inf.pre_duct_seal_close_cfm)) *
          EXPC(0.041317 * (BASE_PRESSURE - ndi->inf.pre_duct_seal_close_diff_pa)));

      ASSERT(ndi->inf.post_duct_seal_pa, sprintf(msg, "Need non zero pressure for cfm correction"));
      ndi->inf.post_duct_50_cfm = limit_to_zero(
          (blower_door_corrected_cfm((BASE_PRESSURE / ndi->inf.post_duct_seal_pa), ndi->inf.post_duct_seal_cfm) -
           duct_blower_corrected_cfm((ndi->inf.post_duct_seal_close_pa / BASE_PRESSURE), ndi->inf.post_duct_seal_close_cfm)) *
          EXPC(0.041317 * (BASE_PRESSURE - ndi->inf.post_duct_seal_close_diff_pa)));
      break;

    case DUCT_BLOWER:  // Duct-Blower Pressure Tests

      ASSERT(ndi->inf.pre_duct_seal_out_duct_pa, sprintf(msg, "Need non zero pressure for cfm correction"));
      ndi->inf.pre_duct_50_cfm =
          duct_blower_corrected_cfm((BASE_PRESSURE / ndi->inf.pre_duct_seal_out_duct_pa), ndi->inf.pre_duct_seal_out_cfm);

      ASSERT(ndi->inf.post_duct_seal_out_duct_pa, sprintf(msg, "Need non zero pressure for cfm correction"));
      ndi->inf.post_duct_50_cfm =
          duct_blower_corrected_cfm((BASE_PRESSURE / ndi->inf.post_duct_seal_out_duct_pa), ndi->inf.post_duct_seal_out_cfm);

      break;

    // we no longer support the PRESSURE_PAN method for NEAT.  NOTE, it is supported for MHEA
    case PRESSURE_PAN:
      ASSERT(0, sprintf(msg, "NEAT does not support duct leakage calculations using Pressure Pan readings."));
      break;

    default:
      break;    // nothing

  } // switch
}

/***************************************************************************
 ** Function Name: duct_efficiency_NEAT
 **          Date:
 **     Author(s): Mike Gettings
 **
 **  DESCRIPTION: This routine computes duct distribution efficiencies
 **  given the leakage
 **  of the ducts measure from established techniques.
 **************************************************************************/

void duct_efficiency_NEAT(float Q50, float supply_pa, float return_pa, float *effht, float *effcl) {

  // Declarations

  // FILE *inputf             // used for parametric study only
  // int   nfread;
  // FILE *outputf2;
  // FILE *outputf;
  int return_reg_num;
  int i, m, nhtm, nclm;

  float arg, asBs, arBr, Qinf15, Qimb15, wout, wdum, win;
  float tom, tdb_max_cl, tdb_min_ht, rhatmax;

  int compute_ducts = 1;

  float tin_ht,        // Indoor air temperature, heating
      tin_cl,          // Indoor air temperature, cooling
      tdb_ht,          // Outdoor dry bulb temperature, heating
      tdb_cl,          // Outdoor dry bulb temperature, cooling
      RH,              // Seasonal (summer) relative humidity
      tground,         // Ground temperature (year-round)
      tsp,             // Supply pleum dry-bulb temperature, cooling
                       //    totarea,             Total living space floor area
      Fout_S,          // Fraction of supply duct outside conditioned space
      Fout_R,          // Fraction of return duct outside conditioned space
      Aout_S,          // Area of supply duct outside conditioned space
      Aout_R,          // Area of return duct outside conditioned space
      Atot_S,          // Total area of supply duct
      Atot_R,          // Total area of return duct
      br,              // Return duct surface area coefficient
      tamb_S_ht,       // Ambient temperature at supply duct location, heating
      tamb_R_ht,       // Ambient temperature at return duct location, heating
      tamb_S_cl,       // Ambient temperature at supply duct location, cooling
      tamb_R_cl,       // Ambient temperature at return duct location, cooling
      hin,             // Enthalpy of inside conditioned air
      hout,            // Outdoor seasonal enthalpy
      hamb_R,          // Enthalpy of ambient air for return
      ductR_S,         // Supply duct R-value  (h-sqft-F/Btu)
      ductR_R,         // Return duct R-value  (h-sqft-F/Btu)
      dtemax_ht,       // Maximum temperture change across heat exchanger
      Ecap_ht,         // Heating capacity, Btu/hr
      Ecap_cl,         // Cooling capacity, Btu/hr
      Qe_ht,           // Heating fan flow rate (CFM)
      Qe_htmin,        // Heating fan flow rate minimum based on floor area (CFM)
      Qe_htmax,        // Heating fan flow rate maximum based on floor area (CFM)
      Qe_cl,           // Cooling fan flow rate (CFM)
      Qs_ht,           // Supply duct leakage rate (CFM), heating (measured)
      Qs_cl,           // Supply duct leakage rate (CFM), cooling (measured)
      Qr_ht,           // Return duct leakage rate (CFM), heating (measured)
      Qr_cl,           // Return duct leakage rate (CFM), cooling (measured)
      Qnet_ht,         // Net inf., combined natural and imbalance, CFM, heating
      Qnet_cl,         // Net inf., combined natural and imbalance, CFM, cooling
      Qinf,            // Natural infiltrtion rate (CFM)
      Qimb,            // Supply/Return flow imbalance, heating or cooling
      Fcyc,            // Cyclic loss factor
      F_cycloss,       // Duct thermal mass factor
      DTe_ht,          // Temperature change across equipment, heating
      DTe_cl,          // Temperature change across equipment, cooling
      DTr_ht,          // Building-return duct ambient temperature difference, heating
      DTr_cl,          // Building-return duct ambient temperature difference, cooling
      DTs_ht,          // Building-supply duct ambient temperature difference
      Bs_ht,           // Conduction fraction, supply, heating
      Bs_cl,           // Conduction fraction, supply, cooling
      Br_ht,           // Conduction fraction, return, heating
      Br_cl,           // Conduction fraction, return, cooling
      as_ht,           // Duct leakage factor, supply, heating
      as_cl,           // Duct leakage factor, supply, cooling
      ar_ht,           // Duct leakage factor, return, heating
      ar_cl,           // Duct leakage factor, return, cooling
      F_regain_S,      // Thermal regain factor, supply
      F_regain_R,      // Thermal regain factor, return
      DE_seas_ht,      // Delivery effectiveness, heating
      DE_seas_cl,      // Delivery effectiveness, cooling
      F_load_ht,       // Air infiltration load, heating
      F_load_cl,       // Air infiltration load, cooling
      F_equip_ht,      // Equipment efficiency factor, heating
      F_equip_cl,      // Equipment efficiency factor, cooling
      DE_corr_ht,      // Delivery effectiveness, corrected, heating
      DE_corr_cl,      // Delivery effectiveness, corrected, cooling
      N_dist_effic_ht, // Seasonal distribution system efficiency, heating
      N_dist_effic_cl; // Seasonal distribution system efficiency, cooling
                       //      epsilon=0.0001f;  // Limit to which Qs_ht can approach Qe_ht

  // Set fixed values

  tin_ht = 68.0f; // Prescribed pg  9 152P Standard, May 99
  tin_cl = 78.0f; // Prescribed pg  9 152P Standard, May 99
  tsp = 55.0f;    // Prescribed pg 27 152P Standard, May 99
  dtemax_ht = 50.0f;
  // Fcyc = 0.02f; // non-metalic ducts,  removed MJF 12/18 TODO resolve why
  Fcyc = 0.05f; // sheet metal ducts
  ductR_R = ductR_S = 1.54f;

  /**** Determine supply and return duct surface area outside condtnd space ***/

  return_reg_num = 1;
  if (ndi->gnl.floor_area > 1800.)
    return_reg_num = 2;
  if (return_reg_num >= 5)
    br = 0.25;
  else
    br = 0.05f * (float)(return_reg_num);

  /**  Removed as inaccurate if duct insulation being considered on only
  *        portion of ducts  MBG 4/11
  *
  * if(GLBductarea>0.001f) {   // ducts specified as in unconditioned space
  *   Atot_S = MAX(0.27f*totarea,GLBductarea);
  *   Aout_S = GLBductarea;
  *   Fout_S = Aout_S/Atot_S;
  *   if(ndi->htg[PRIMARY].location == UNHEATED || ndi->htg[PRIMARY].location == UNINTENTIONALLY_HEATED) Fout_R = 0.7f;
  *   else                           Fout_R = 0.2f;
  *   Atot_R = br*totarea;
  *   Aout_R = Atot_R * Fout_R; }
  * else          // No duct declared outside conditioned space  *******/

  if (ndi->htg[PRIMARY].location == UNHEATED ||
      ndi->htg[PRIMARY].location == UNINTENTIONALLY_HEATED) { // Heating equipment not in conditioned space
    Atot_S = 0.27f * ndi->gnl.floor_area;
    Fout_S = 1.0f;
    Aout_S = Atot_S * Fout_S;
    Fout_R = 0.7f;
    Atot_R = br * ndi->gnl.floor_area;
    Aout_R = Atot_R * Fout_R;
  } else {                                       // Heating equipment in conditioned space
    if (ndi->inf.evaluate_duct_sealing == YES) { //   But ducts evaluated so must have some in unconditioned space
      Atot_S = 0.27f * ndi->gnl.floor_area;
      Fout_S = 0.5f;
      Aout_S = Atot_S * Fout_S;
      Fout_R = 0.0f;
      Atot_R = br * ndi->gnl.floor_area;
      Aout_R = Atot_R * Fout_R;
    } else
      compute_ducts = 0;
  }

  // Avoid unititalized variables in compiler warings for Aout_S intialization if
  // we are not going to compute ducts
  if (compute_ducts != 0) {
    // Divide leakage into supply and return components based on area fractions

    Qs_ht = Q50 * Aout_S / (Aout_S + Aout_R);
    Qr_ht = Q50 * Aout_R / (Aout_S + Aout_R);

    // Adjust leakages to operating pressures in supply and return plenums

    if (Qs_ht > 0.0f)
      //Qs_ht = (float)(Qs_ht * POWC((supply_pa / BASE_PRESSURE), 0.6f));
      Qs_ht = blower_door_corrected_cfm((supply_pa / BASE_PRESSURE), Qs_ht);
    else
      Qs_ht = 0.0f;
    Qs_cl = Qs_ht;

    if (Qr_ht > 0.0f)
      //Qr_ht = (float)(Qr_ht * POWC((return_pa / BASE_PRESSURE), 0.6f));
      Qr_ht = blower_door_corrected_cfm((return_pa / BASE_PRESSURE), Qr_ht);
    else
      Qr_ht = 0.0f;
    Qr_cl = Qr_ht;

    // Read in values used for parametric study

    //  inputf = fopen("ductef.inp","r");

    //  outputf = fopen("duct.out","a");

    //  fprintf(outputf,"Total area  area outside  Fan flow      Sup leaks       Ret leaks     Dist Eff\n");
    //  fprintf(outputf," Sup   Ret   Sup   Ret    Htg  Clg     Htg     Clg     Htg     Clg    Htg  Clg\n");

    //  fprintf(outputf2,"    dry bulb             equip size  duct    leakage     dist effic\n");
    //  fprintf(outputf2,"    ht    cl   RH   area  ht    cl    R      ht    cl     ht      cl\n\n");

    //  for(;(nfread=fscanf(inputf,"%f %f %f %f %f %f %f %f %f %f %f",
    //    &tdb_ht, &tdb_cl, &htdestemp, &cldestemp, &RH, &totarea, &Ecap_ht, &Ecap_cl,
    //    &ductR_S, &Qs_ht, &Qs_cl))==11;) [if used would have squiggly bracket here]

    /*  Find the average heating and cooling outdoor ambient temperatures and RH.
    *  Cooling is average of monthly temperautures above 65 F, heating those below.
    *  If no monthly temperatures are above or below 65 to form seasonal average,
    *  use the maximum or minumum monthly temperatures as the average. */

    tdb_ht = tdb_cl = 0.0f;
    nhtm = nclm = 0;
    RH = 0.0f;
    tdb_min_ht = 100.0f;
    tdb_max_cl = -100.0f;
    for (m = 1; m <= MONTHS; m++) {
      tom = cwd->avg_drybulb_temp[m];
      if (tom < 65.0f) {
        if (m == DECEMBER || m < APRIL) {
          tdb_ht += tom;
          nhtm++;
        }
      }
      if (tom < tdb_min_ht)
        tdb_min_ht = tom;
      if (tom > 65.0f) {
        tdb_cl += tom;
        RH += relative_humidity(tom, cwd->avg_wetbulb_temp[m], cwd->altitude);
        nclm++;
      }
      if (tom > tdb_max_cl) {
        tdb_max_cl = tom;
        rhatmax = relative_humidity(tdb_max_cl, cwd->avg_wetbulb_temp[m], cwd->altitude);
      }
    }

    if (nhtm > 0)
      tdb_ht /= (float)nhtm;
    else
      tdb_ht = tdb_min_ht;
    if (nclm > 0) {
      tdb_cl /= (float)nclm;
      RH /= (float)nclm;
    } else {
      tdb_cl = tdb_max_cl;
      RH = rhatmax;
    }

    // Derived values

    tground = (cwd->heating_design_temp + cwd->cooling_design_temp) / 2.0f;
    Qinf = 8.0f * 0.35f * ndi->gnl.floor_area / 60; // Based on 0.35 ACH infiltration
    if (ndi->htg[PRIMARY].duct_location == SUBSPACE) {    // Ducts in uninsulated basement
      tamb_R_ht = tamb_S_ht = (5.0f * tground + 2.0f * tdb_ht + 3.0f * tin_ht) / 10.0f;
      tamb_R_cl = tamb_S_cl = (5.0f * tground + 2.0f * tdb_cl + 3.0f * tin_cl) / 10.0f;
    } else { // Ducts in vented attic
      tamb_R_ht = tamb_S_ht = tdb_ht + 2.0f;
      tamb_R_cl = tamb_S_cl = tdb_cl + 9.0f;
    }
    if (Fout_R == 0.0) {
      tamb_R_cl = tin_cl;
      tamb_R_ht = tin_ht;
    }

    if (tamb_R_cl < tin_cl)
      tamb_R_cl = (tdb_cl + tamb_R_cl) / 2.0f;
    if (tamb_R_ht > tin_ht)
      tamb_R_ht = (tdb_ht + tamb_R_ht) / 2.0f;

    F_equip_ht = 1.0f; // All single capacity furnaces - Table 6.7 pg.28
    F_equip_cl = 1.0f; // Thermostatic control A/C - Table 6.7 pg.28
    F_cycloss = Fcyc * (Fout_S + Fout_R) / 2.0f;

    if (ndi->htg[PRIMARY].duct_location == SUBSPACE) {
      F_regain_S = F_regain_R = 0.5f;
    } // Uninsulated basement
    else {
      F_regain_S = F_regain_R = 0.1f;
    } // Vented attic
  }   // on compute_ducts

  /*******************************/
  /***  Heating Computations  ****/
  /*******************************/

  if ((ndi->htg[PRIMARY].system_type != HE_FORCED_AIR_FURNACE && ndi->htg[PRIMARY].system_type != HE_HEAT_PUMP) || compute_ducts == 0) {
    // No forced air system or no ducts in unconditioned spaces
    N_dist_effic_ht = 1.0f;
    // Ecap_ht = 0.0f;    // dead assignment removal MJF 12/2018
    // Qe_ht = 0.0f;      // dead assignment removal MJF 12/2018
  }

  else {

    Ecap_ht = ndi->htg[PRIMARY].output_capacity * 1000.0f;
    Qe_ht = 0.85f * Ecap_ht / (60.0f * dtemax_ht * RHOCAIR); // (H-2, pg.86 for furnaces)

    // Compute limits to fan flow rate and restrain rate used to these limits (MBG 4/06)

    Qe_htmin = 0.5f * (float)ndi->gnl.floor_area;
    Qe_htmax = 1.0f * (float)ndi->gnl.floor_area;
    if (Qe_ht < Qe_htmin)
      Qe_ht = Qe_htmin;
    if (Qe_ht > Qe_htmax)
      Qe_ht = Qe_htmax;

    // Qs_ht = 0.17f*Fout_S*Qe_ht;  // Fout's must be less than or equal to 0.5
    // Qr_ht = 0.17f*Fout_R*Qe_ht;

    // Prevent Qs_ht and Qr_ht from becoming greater than 0.95 of Qe_ht

    if (Qs_ht >= 0.95f * Qe_ht)
      Qs_ht = 0.95f * Qe_ht; // Leakage greater than supply or
    if (Qr_ht >= 0.95f * Qe_ht)
      Qr_ht = 0.95f * Qe_ht; //  return flow rates

    ASSERT((60.0f * Qe_ht * RHOCAIR * ductR_S) != 0, sprintf(msg, "Need non zero factor"));
    ASSERT((60.0f * Qe_ht * RHOCAIR * ductR_R) != 0, sprintf(msg, "Need non zero factor"));

    arg = -Aout_S / (60.0f * Qe_ht * RHOCAIR * ductR_S);
    Bs_ht = (float)(exp((float)(arg)));
    arg = -Aout_R / (60.0f * Qe_ht * RHOCAIR * ductR_R);
    Br_ht = (float)(exp((float)(arg)));

    ASSERT(Qe_ht, sprintf(msg, "Need non zero factor"));
    as_ht = (Qe_ht - Qs_ht) / Qe_ht;
    ar_ht = (Qe_ht - Qr_ht) / Qe_ht;

    DTe_ht = Ecap_ht / (60.0f * Qe_ht * RHOCAIR);
    DTs_ht = tin_ht - tamb_S_ht;
    DTr_ht = tin_ht - tamb_R_ht;

    /***** Compute the imbalance flow, Qnet ******/

    Qimb = (float)(fabs((float)(Qs_ht - Qr_ht)));
    Qinf15 = (float)(POWC(Qinf, 1.5f));
    if (Qimb < 1.0e-3)
      Qnet_ht = (float)(POWC(Qinf15, 0.67f));
    else {
      Qimb15 = (float)(POWC(Qimb, 1.5f));
      if (Qs_ht > Qr_ht)
        Qnet_ht = (float)(POWC((Qinf15 + Qimb15), 0.67f));
      else {
        if (Qimb >= Qinf)
          Qnet_ht = 0.0f;
        else
          Qnet_ht = (float)(POWC((Qinf15 - Qimb15), 0.67f));
      }
    }

    /***** Compute delivery effectiveness and air infiltration load, F_load *****/

    asBs = as_ht * Bs_ht;
    arBr = ar_ht * Br_ht;

    ASSERT(DTe_ht, sprintf(msg, "Need non zero factor"));
    ASSERT(Ecap_ht, sprintf(msg, "Need non zero factor"));

    DE_seas_ht = asBs - asBs * (1.0f - arBr) * DTr_ht / DTe_ht - as_ht * (1.0f - Bs_ht) * DTs_ht / DTe_ht;
    ASSERT(DE_seas_ht, sprintf(msg, "Need non zero factor"));
    F_load_ht = 1.0f - (60.0f * RHOCAIR * (tin_ht - tdb_ht) * (Qnet_ht - Qinf)) / Ecap_ht / DE_seas_ht; // (6-40) p.30

    /***** Compute the corrected delivery effectiveness and distribution efficiency ******/

    DE_corr_ht = DE_seas_ht + F_regain_S * (1.0f - DE_seas_ht) -
                 (F_regain_S - F_regain_R - Br_ht * (ar_ht * F_regain_S - F_regain_R)) * DTr_ht / DTe_ht; // (6-34) p.30

    N_dist_effic_ht = DE_corr_ht * F_equip_ht * F_load_ht * (1.0f - F_cycloss); // (6-44) p.31
  }

  /*******************************/
  /***  Cooling Computations  ****/
  /*******************************/

  Ecap_cl = 0.0f;
  for (i = 0; i < ndi->num_clg; i++) {
    if (ndi->clg[i].system_type == CET_CENTRAL || ndi->clg[i].system_type == CET_HEATPUMP)
      Ecap_cl = ndi->clg[i].size * 1000.0f;   // convert from kBtu/h to Btu/h
    break;
  }

  if (Ecap_cl < 0.001f || compute_ducts == 0) {
    // No central A/C or no ducts in unconditioned spaces
    N_dist_effic_cl = 1.0f;
    // Qe_cl = 0.0f;    // dead assign removal MJF 12/2018
  } else {

    Qe_cl = 340.0f * Ecap_cl / BTUH_PER_TON; // Default value

    arg = -Aout_S / (60.0f * Qe_cl * RHOCAIR * ductR_S);
    Bs_cl = (float)(exp((float)(arg)));
    arg = -Aout_R / (60.0f * Qe_cl * RHOCAIR * ductR_R);
    Br_cl = (float)(exp((float)(arg)));

    as_cl = (Qe_cl - Qs_cl) / Qe_cl;
    ar_cl = (Qe_cl - Qr_cl) / Qe_cl;

    DTe_cl = -Ecap_cl / (60.0f * Qe_cl * RHOCAIR);
    DTr_cl = tin_cl - tamb_R_cl;

    /****** Compute the cooling season return enthalpy from psychometrics *****/

    hout = enthalpy(tdb_cl, RH, tdb_cl, &wout);
    win = 0.004f + 0.4f * wout; // Per e-mail of 5/22/00 from Iain Walker
    hin = 0.240f * tin_cl + win * (1061.0f + 0.444f * tin_cl);
    if (ndi->htg[PRIMARY].duct_location == SUBSPACE || Fout_R == 0.0)
      hamb_R = hin;
    else {
      hamb_R = enthalpy(tdb_cl, RH, tamb_R_cl, &wdum);
      if (hamb_R < hin)
        hamb_R = (hamb_R + hout) / 2.0f;
    }

    /***** Compute the imbalance flow, Qnet ******/

    Qimb = (float)(fabs((float)(Qs_cl - Qr_cl)));
    if (Qimb < 1.0e-3)
      Qnet_cl = (float)(POWC(Qinf15, 0.67f));
    else {
      Qimb15 = (float)(POWC(Qimb, 1.5f));
      if (Qs_ht > Qr_ht)
        Qnet_cl = (float)(POWC((Qinf15 + Qimb15), 0.67f));
      else {
        if (Qimb >= Qinf)
          Qnet_cl = 0.0f;
        else
          Qnet_cl = (float)(POWC((Qinf15 - Qimb15), 0.67f));
      }
    }

    /***** Compute delivery effectiveness and air infiltration load, F_load *****/

    DE_seas_cl =
        as_cl -
        60.0f * as_cl * Qe_cl * RHO / Ecap_cl * ((1.0f - ar_cl) * (hamb_R - hin) + ar_cl * CAIR * (Br_cl - 1.0f) * DTr_cl +
                                                 CAIR * (Bs_cl - 1.0f) * (tsp - tamb_S_cl));
    F_load_cl = 1.0f + (60.0f * RHO * (hin - hout) * (Qnet_cl - Qinf)) / Ecap_cl / DE_seas_cl; // (6-40) p.30

    /***** Compute the corrected delivery effectiveness and distribution efficiency ******/

    DE_corr_cl = DE_seas_cl + F_regain_S * (1.0f - DE_seas_cl) -
                 (F_regain_S - F_regain_R - Br_cl * (ar_cl * F_regain_S - F_regain_R)) * DTr_cl / DTe_cl; // (6-34) p.30

    N_dist_effic_cl = DE_corr_cl * F_equip_cl * F_load_cl * (1.0f - F_cycloss); // (6-44) p.31
  }
  /**************************************************/
  /******  Print interim and final results  *********/
  /**************************************************/
  //  fprintf(outputf,"\n%7.0f%7.0f%7.2f%7.2f%6.0f%6.0f%6.0f%6.0f%7.0f%3d  %c%7.3f%7.3f",
  //  Atot_S,Atot_R,Fout_S,Fout_R,Qs_ht,Qr_ht,Qe_ht,Qe_cl,Qinf,return_reg_num,duct_loc,
  //  N_dist_effic_ht,N_dist_effic_cl);
  //
  //  fprintf(outputf2,"\n%6.0f%6.0f%6.2f%6.0f%6.0f%6.0f%5.1f%6.0f%6.0f%8.3f%8.3f",
  //  tdb_ht,tdb_cl,RH,totarea,Ecap_ht,Ecap_cl,ductR_S,Qs_ht,Qs_cl,N_dist_effic_ht,N_dist_effic_cl);
  /*
   *  fprintf(stderr, "\n%8.3f  Total supply duct area (Sqft)   ",Atot_S);
   *  fprintf(stderr, "\n%8.3f  Total return duct area (Sqft)   ",Atot_R);
   *  fprintf(stderr, "\n%8.3f  Supply duct area outside conditioned space (Sqft)",Aout_S);
   *  fprintf(stderr, "\n%8.3f  Return duct area outside conditioned space (Sqft)",Aout_R);
   *  fprintf(stderr, "\n%8.3f  Heating fan flow rate (CFM)     ",Qe_ht);
   *  fprintf(stderr, "\n%8.3f  Cooling fan flow rate (CFM)     ",Qe_cl);
   *  fprintf(stderr, "\n%8.3f  Supply duct leakage rate (CFM)(heating)    ",Qs_ht);
   *  fprintf(stderr, "\n%8.3f  Supply duct leakage rate (CFM)(cooling)    ",Qs_cl);
   *  fprintf(stderr, "\n%8.3f  Return duct leakage rate (CFM)(heating)    ",Qr_ht);
   *  fprintf(stderr, "\n%8.3f  Return duct leakage rate (CFM)(cooling)    ",Qr_cl);
   *  fprintf(stderr, "\n%8.3f  Seasonal distribution efficiency (heating) ",N_dist_effic_ht);
   *  fprintf(stderr, "\n%8.3f  Seasonal distribution efficiency (cooling) ",N_dist_effic_cl);
  */
  //  fclose(inputf);
  //  fclose(outputf);
  //  fclose(outputf2);

  if (N_dist_effic_ht > 1.0)
    N_dist_effic_ht = 1.0f;
  if (N_dist_effic_cl > 1.0)
    N_dist_effic_cl = 1.0f;
  if (N_dist_effic_ht < 0.0)
    N_dist_effic_ht = 0.0f;
  if (N_dist_effic_cl < 0.0)
    N_dist_effic_cl = 0.0f;
  *effht = 0.5f * (N_dist_effic_ht + 1.0f); //  Map efficiencies into range
  *effcl = 0.5f * (N_dist_effic_cl + 1.0f); //    of 0.5 to 1.0
  return;
}
