/******************  MODULE NAME:  RETROFIT.C  ***************************/
/**         DATE:  1/2/93                                               **/
/**           BY:    NLW,SLF; ERG                                       **/
/**  DESCRIPTION:    Functions for computing the result of -            **/
/**                         Bubble Sort (of retrofit measures)          **/
/**                         BCR Calculation                             **/
/**                         First Pass through Retrofit Measures        **/
/**                         Cumulative Pass through Retrofit Measures   **/
/**  Revisions:                                                         **/
/**                 7/12/95 NLW - Corrected air-seal error in firstpass **/
/**                 7/13/95 NLW - Corrected air-seal error  in cumulpass**/
/**                 7/21/95 NLW - Corrected "no-HTG" error in mhea_measure_sir    **/
/**                 8/30/95 NLW - Corrected k index in cumulpass        **/
/**                 9/ 1/95 NLW - Update Repl-HTG BCR in cumulpass      **/
/**                 9/ 6/95 NLW - Corrected air-seal error in cumulpass **/
/**                                                                     **/
/**  MW3 revisions by MJF 4/97                                          **/
/**  7/23/97, NLW: Adjust mapping for MDI scheme in GETBCR              **/
/**  7/24/97, NLW: Initialize retrofit enable flag array                **/
/**  11/4/97, NLW: Correct cumulative pass logic for failed measures    **/
/**  2020  MJF Rewrite for single wa engine                             **/
/*************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <math.h>

#include "wa_engine.h"

FILE *measure_file;

// some local functions

static void sort_mhea_package_measures(int sortabs);
static void diagnostic_results_header(void);
static void diagnostic_results_line(int i);

/*******************  FUNCTION NAME: SortResults         *****************/
/**         DATE:  9/21/00                                              **/
/**           BY:  MJF                                                  **/
/**  DESCRIPTION:  Simple bubble sort algorithm to order the entries    **/
/**                in the mor array.  Note how entries with      **/
/**                flgRequired Set are sorted to the top also in BCR    **/
/**                order                                                **/
/**     MODIFIED:  10/08 - MBG - Allows sorting on absolute value of    **/
/**                sorting parameter, flgInclCost, in order to allow    **/
/**                different order for measure evaluation and reporting **/
/*************************************************************************/
static void sort_mhea_package_measures(int sortabs) {
  int flgSwap = TRUE; // did we swap in the loop
  BCR_RES temp;    // temp structure for swapping
  int i;

  if (mir->Rndx < 2) // base zero counter = 1 means [0] is the only measure
    return;

  // we need to guarantee that any existing
  // retrofits with negative BCR do NOT get
  // sorted past the end list of results otherwise
  // we get a null GLBResult record sorted into the list
  // of retrofits (fixed MJF 8/02 and 4/05)
  // remember that mir->Rndx is base zero so this is one PAST the end of 
  // valid array entries.
  if (sortabs)
    mir->Results[mir->Rndx].measure_priority = 0;
  else 
    mir->Results[mir->Rndx].measure_priority = MPS_BOTTOM_MARK; 
  mir->Results[mir->Rndx].fBCR = MPS_BOTTOM_MARK;

  while (flgSwap) // our simple bubble sort
  {
    flgSwap = FALSE;
    for (i = 0; i < mir->Rndx; i++) {

      // sorting first based on flgInclCost field

      if (mir->Results[i + 1].measure_priority != // secondary sort key involved
          mir->Results[i].measure_priority) {
        if (sortabs) {                               // Sort on absolute value of measure_priority
          if (abs(mir->Results[i + 1].measure_priority) > // out of order
              abs(mir->Results[i].measure_priority)) {
            memcpy(&temp, &mir->Results[i + 1], sizeof(BCR_RES));
            memcpy(&mir->Results[i + 1], &mir->Results[i], sizeof(BCR_RES));
            memcpy(&mir->Results[i], &temp, sizeof(BCR_RES));
            flgSwap = TRUE;
          }
        } else {
          if (mir->Results[i + 1].measure_priority > // out of order
              mir->Results[i].measure_priority) {
            memcpy(&temp, &mir->Results[i + 1], sizeof(BCR_RES));
            memcpy(&mir->Results[i + 1], &mir->Results[i], sizeof(BCR_RES));
            memcpy(&mir->Results[i], &temp, sizeof(BCR_RES));
            flgSwap = TRUE;
          }
        }
        continue;
      }

      // second level sorting on fBCR

      if (mir->Results[i + 1].fBCR > // default sort key
          mir->Results[i].fBCR) {
        memcpy(&temp, &mir->Results[i + 1], sizeof(BCR_RES));
        memcpy(&mir->Results[i + 1], &mir->Results[i], sizeof(BCR_RES));
        memcpy(&mir->Results[i], &temp, sizeof(BCR_RES));
        flgSwap = TRUE;
      }
    }
  }
}

/*******************  FUNCTION NAME: mhea_measure_sir              *****************/
/**         DATE:  1/1/93                                               **/
/**           BY:  NW, SLF, 9/17/01 MJF rewrite to use GLBResult        **/
/**  DESCRIPTION:  computes benefit to cost ratio from heating          **/
/**                and cooling energy savings (passed as parameters in  **/
/**                units of MMBtu) and stores results in the GLBResult   **/
/**                array.                                               **/
/*************************************************************************/
void mhea_measure_sir(int Rndx) // index into mor
{
  int iMat;             // material index
  BCR_RES *res;         // just a local pointer to make coding easier
  float life= 0;        // lifetime of the measure
  float PreHtgPWFactor; // our pre retrofit present worth factors by fuel type
  float PreClgPWFactor;
  float PreBasPWFactor;
  float PstHtgPWFactor; // our post retrofit present worth factors by fuel type
  float PstClgPWFactor;
  float PstBasPWFactor;
  float frac; // fractional part of fLife
  float fTempQuant;
  int yr; // whole number year part of fLife

  if (mir->Results[Rndx].fBCR != 0.0) // already computed
    return;                           // dont recompute

  res = &mir->Results[Rndx];             // assign a temp pointer
  iMat = res->material_id;               // which material

  if (res->fLife > 0)
    life = res->fLife; // override

  if (iMat >= 0 && iMat < MHEA_MAX_RMC)  // do assignments only if we have a valid material id
  {
    if (res->fInitCost == 0.0) { // don't compute if already set
      fTempQuant = res->fReportQuant;
      if (fTempQuant < 1.e-3)
        fTempQuant = 1.0f;
      res->fInitCost = // initial cost
          (mdi->rmc[iMat].material + mdi->rmc[iMat].labor) * res->fQuant + mdi->rmc[iMat].extra * fTempQuant +
          res->fAddCost;

      positive_initial_cost_required(res->fInitCost, res->sName);

      if (cmds.debug_level & D_MHEA_SIR_DETAIL) {
        fprintf(stderr, "\nMaterial: %d", iMat);        
        fprintf(stderr, "\nRndx: %d", Rndx);
        fprintf(stderr, "\nRetrofit Name: %s", res->sName);
        fprintf(stderr, "\nUnit Material Cost: %8.3f", mdi->rmc[iMat].material);
        fprintf(stderr, "\nUnit Labor Cost: %8.3f", mdi->rmc[iMat].labor);
        fprintf(stderr, "\nUnit Quantity: %8.3f", res->fQuant);
        fprintf(stderr, "\nUnit Extra Cost: %8.3f", mdi->rmc[iMat].extra);
        fprintf(stderr, "\nUnit Extra Quantity: %8.3f", fTempQuant);
        fprintf(stderr, "\nAdded: %8.3f", res->fAddCost);
        fprintf(stderr, "\nTotal: %8.3f", res->fInitCost);
      }

    }

    // some other fields needed for measure table carry through

    //res->measure_id = mdi->rmc[iMat].measure_id; // carry forward the ID from the database
    res->fUnitMaterial = mdi->rmc[iMat].material;
    res->fUnitLabor = mdi->rmc[iMat].labor;
    res->costi1 = mdi->rmc[iMat].extra;

    // now lets fill in the material name if it is blank in the
    // results structure, also grab the units string

    if (strlen(res->sMaterial) == 0)
      STRCPY(res->sMaterial, mdi->rmc[iMat].retro_name);

    if (strlen(res->sUnits) == 0)
      STRCPY(res->sUnits, mdi->rmc[iMat].units);

    if (res->fLife <= 0)
      life = mdi->rmc[iMat].life; // note that .life is a float
  }

  ASSERT( life && life <= MAXMLIFE, sprintf(msg, "Lifetimne range error of %d years for measure: %s", MAXMLIFE, res->sMaterial));
  res->fLife = life;      // #85 MJF 8/2019 save the lifetime used in the mor structure

  // simple annual energy savings calculations in units of MMBtu
  // Note that the results structure stores everything in units
  // of BTU or MMBtu

  res->fEnerSavHtg = (res->fEnerPreHtg - res->fEnerPstHtg) / (float)1.0e6;
  res->fEnerSavClg = (res->fEnerPreClg - res->fEnerPstClg) / (float)1.0e6;
  res->fEnerSavBas = (res->fEnerPreBas - res->fEnerPstBas) / (float)1.0e6;

  // total PW of dollar savings over life
  res->fCostSavTot = 0;

  // fuel neutral (ie before and after fuels need not be the same)
  // calculation of the annual energy savings in dollars

  // ------HEATING------
  if (res->fEnerSavHtg) {
    ASSERT(res->PreHtgFuel && res->PstHtgFuel, sprintf(msg, "Must have pre and post heating fuel types for %s", res->sMaterial));

    res->fCostSavHtg =
      (res->fEnerPreHtg/(float)1.0e6 *
       fuel_cost_per_mmbtu(res->PreHtgFuel)) -
      (res->fEnerPstHtg/(float)1.0e6 *
       fuel_cost_per_mmbtu(res->PstHtgFuel));

    if (life == (int)(life)) {
      yr = (int)(life);
      PreHtgPWFactor = upw_fuel_factor(res->PreHtgFuel, yr); // no interpolation
      PstHtgPWFactor = upw_fuel_factor(res->PstHtgFuel, yr);
    } else {
      frac = life - (int)(life);
      yr = (int)(life);
      PreHtgPWFactor = upw_fuel_factor(res->PreHtgFuel, yr) +
                       frac * (upw_fuel_factor(res->PreHtgFuel, yr + 1) - upw_fuel_factor(res->PreHtgFuel, yr));
      PstHtgPWFactor = upw_fuel_factor(res->PstHtgFuel, yr) +
                       (frac * (upw_fuel_factor(res->PstHtgFuel, yr + 1) - upw_fuel_factor(res->PstHtgFuel, yr)));
    }

    res->fCostSavTot +=
     ((res->fEnerPreHtg/(float)1.0e6 *
       fuel_cost_per_mmbtu(res->PreHtgFuel) *
       PreHtgPWFactor) -
      (res->fEnerPstHtg/(float)1.0e6 *
       fuel_cost_per_mmbtu(res->PstHtgFuel) *
       PstHtgPWFactor));

  }  else {
    res->fCostSavHtg = 0.0;
  }

  // Fuel switching emphirical savings factor  #90  (not github or gitlab  MJF)
  if(res->PreHtgFuel != res->PstHtgFuel) {
     res->fEnerSavHtg *= MHEASAVINGSADJ; 
     res->fCostSavHtg *= MHEASAVINGSADJ; 
     res->fCostSavTot *= MHEASAVINGSADJ; 
   }

  // ------COOLING------
  if (res->fEnerSavClg) {
    ASSERT(res->PreClgFuel && res->PstClgFuel, sprintf(msg, "Must have pre and post cooling fuel types for %s", res->sMaterial));

    res->fCostSavClg =
      (res->fEnerPreClg / (float)1.0e6 * fuel_cost_per_mmbtu(res->PreClgFuel)) -
      (res->fEnerPstClg / (float)1.0e6 * fuel_cost_per_mmbtu(res->PstClgFuel));

    if (life == (int)(life)) {
      yr = (int)(life);
      PreClgPWFactor = upw_fuel_factor(res->PreClgFuel, yr); // no interpolation
      PstClgPWFactor = upw_fuel_factor(res->PstClgFuel, yr);
    } else {
      frac = life - (int)(life);
      yr = (int)(life);
      PreClgPWFactor = upw_fuel_factor(res->PreClgFuel, yr) +
                       frac * (upw_fuel_factor(res->PreClgFuel, yr + 1) - upw_fuel_factor(res->PreClgFuel, yr));
      PstClgPWFactor = upw_fuel_factor(res->PstClgFuel, yr) +
                       (frac * (upw_fuel_factor(res->PstClgFuel, yr + 1) - upw_fuel_factor(res->PstClgFuel, yr)));
    }

    res->fCostSavTot +=
     ((res->fEnerPreClg/(float)1.0e6 *
       fuel_cost_per_mmbtu(res->PreClgFuel) *
       PreClgPWFactor) -
      (res->fEnerPstClg/(float)1.0e6 *
       fuel_cost_per_mmbtu(res->PstClgFuel) *
       PstClgPWFactor));

  }  else {
    res->fCostSavClg = 0.0;
  }

  // ------BASELOADS------
  if (res->fEnerSavBas) {
    ASSERT(res->PreBasFuel && res->PstBasFuel, sprintf(msg, "Must have pre and post baseload fuel types for %s", res->sMaterial));

    res->fCostSavBas = // baseload
      (res->fEnerPreBas / (float)1.0e6 * fuel_cost_per_mmbtu(res->PreBasFuel)) -
      (res->fEnerPstBas / (float)1.0e6 * fuel_cost_per_mmbtu(res->PstBasFuel));

    if (life == (int)(life)) {
      yr = (int)(life);
      PreBasPWFactor = upw_fuel_factor(res->PreBasFuel, yr); // no interpolation
      PstBasPWFactor = upw_fuel_factor(res->PstBasFuel, yr);
    } else {
      frac = life - (int)(life);
      yr = (int)(life);
      PreBasPWFactor = upw_fuel_factor(res->PreBasFuel, yr) +
                       frac * (upw_fuel_factor(res->PreBasFuel, yr + 1) - upw_fuel_factor(res->PreBasFuel, yr));
      PstBasPWFactor = upw_fuel_factor(res->PstBasFuel, yr) +
                       (frac * (upw_fuel_factor(res->PstBasFuel, yr + 1) - upw_fuel_factor(res->PstBasFuel, yr)));
    }

    res->fCostSavTot +=
     ((res->fEnerPreBas/(float)1.0e6 *             // baseload
       fuel_cost_per_mmbtu(res->PreBasFuel) *
       PreBasPWFactor) -
      (res->fEnerPstBas/(float)1.0e6 *
       fuel_cost_per_mmbtu(res->PstBasFuel) *
       PstBasPWFactor));

  }  else {
    res->fCostSavBas = 0.0;
  }

  // total annual MMBtu savings
  res->fEnerSavTot = res->fEnerSavHtg + res->fEnerSavClg + res->fEnerSavBas;

// total annual $ savings
  res->fCostAnnSavTot = res->fCostSavHtg + res->fCostSavClg + res->fCostSavBas;

  // fill in the fields for materials reporting

  if (strlen(res->sReportMaterial) == 0)
    STRCPY(res->sReportMaterial, res->sMaterial);

  if (strlen(res->sReportUnits) == 0)
    STRCPY(res->sReportUnits, res->sUnits);

  if (res->fReportQuant == 0.0)
    res->fReportQuant = res->fQuant;

  // SIR calculation with protection

  res->fBCR = calculate_sir(res->fCostSavTot, res->fInitCost);

  return;
}

/*******************  FUNCTION NAME: first_pass_retrofits  *****************/
/**         DATE:  1/1/93                                               **/
/**           BY:  NW, SLF                                              **/
/**  DESCRIPTION:  MJF rewrite 9/01                                     **/
/*************************************************************************/
void first_pass_retrofits(void) {

  int iRetroNumber;      // index of measure
  //float fHtgAnnual;      // Annual heating Btus
  //float fClgAnnual;      // Annual cooling Btus
  //SIZING HtgDsgnLoad[2]; // Scratch structure for unused sizing results
  MDI *original = NULL;  // as the name implies -- our untouched MDI struct
                         // NULL value is signal it is an unallocated ptr
  int lastRndx;          // The value of mir->Rndx prior to calling measure, measures POST increment mir->Rndx IF the measure is applied

  if (strcmp(cmds.mhea_measure_file_path, NO_OUTPUT) != 0) {
    measure_file = fopen(cmds.mhea_measure_file_path, "w");
    ASSERT(measure_file, sprintf(msg, "Failed to open the MHEA measure report file: %s code:%d:%s", cmds.mhea_measure_file_path, errno, strerror(errno)));

    if(cmds.regression_test)
      fprintf(measure_file, "%s\n", "MHEA ");
    else
      fprintf(measure_file, "%s\n", "MHEA " WA_VERSION);

    fprintf(measure_file, "\nFirst Pass Retrofits\n");
    fprintf(measure_file, "    Pre    Post   HtgSvg   SIR  Reqrd  InclC  Component    Measure\n");

  } else {
    measure_file = NULL;
  }

  // GKA Here's where the copying game begins.

  ASSERT(mdi, sprintf(msg, "The main mdi structure is missing"));
  ASSERT(mir, sprintf(msg, "The main mir structure is missing"));

  copy_mdi(&original, mdi); // Save original mdi structure, allocating memory for original

  // Initial Ranking of Retrofits, first blank our structures
  // and reset our GLB BCR results counter

  memset(mir->Results, 0, (MAXECMS) * sizeof(BCR_RES));
  for (int i = 0; i < MAXECMS; i++) {
      mir->Results[i].measure_priority = MPS_SIR;
  }
  mir->Rndx = 0;

  diagnostic_results_header();

  // the unordered list of measures ` 
  for (iRetroNumber = 0; iRetroNumber < MHEA_MAX_CMS; iRetroNumber++) {

    /********************
    Skip calculations if this retrofit is not enabled
    ********************/

    if (mdi->cms[iRetroNumber].active == YES) {

      // get a fresh copy of the mdi structure and reset
      // all our retrofit flags to zero so no retrofits are
      // skipped due to interactions/exclusions

      copy_mdi(&mdi, original);
      memset(mir->flgRetrofits, 0, MHEA_MAX_CMS * sizeof(int));

      // Reset the base house consumptions on each measure, just to make it explicit
      mir->fPre_Heating = mir->fBasecase_Heating;
      mir->fPre_Cooling = mir->fBasecase_Cooling;

      lastRndx = mir->Rndx;
      ASSERT(Measure_Function[iRetroNumber], sprintf(msg, "Must have non null measure function pointer item %d", iRetroNumber));
      (*Measure_Function[iRetroNumber])(); // call our retro function
                                           // increments mir->Rndx by one OR MORE if implemented measure

      // note that the following will be skipped if the measure routine
      // computes its own BCR figure (it should also fill in the energy
      // savings values as well)  particularly for base load measures


      // note also that the index to the most recent record in
      // mir->Results[] is mir->Rndx-1 since this index is post incremented
      // and thus points to the 'next' retrofit to be added.
      // GKA This sounds fishy.
      if (mir->flgRetrofits[iRetroNumber]) {               // we did this retrofit
        int energy_has_run = FALSE;
        for (int i = lastRndx; i < mir->Rndx; i++) {  // may have incremented mir->Rndx by more than one

          if (mir->Results[i].fBCR == 0.0) {  // no BCR results from Measure_Function, so we compute them now

            if (energy_has_run == FALSE) { 
              mhea_energy_use();
              energy_has_run = TRUE;
            }

            //mir->Results[i].fEnerPreHtg = mir->fHtgBase;
            mir->Results[i].fEnerPreHtg = mir->fPre_Heating;
            //mir->Results[i].fEnerPstHtg = fHtgAnnual;
            mir->Results[i].fEnerPstHtg = mir->fHeating_Energy;
            //mir->Results[i].fEnerPreClg = mir->fClgBase;
            mir->Results[i].fEnerPreClg = mir->fPre_Cooling;
            //mir->Results[i].fEnerPstClg = fClgAnnual;
            mir->Results[i].fEnerPstClg = mir->fCooling_Energy;

            mhea_measure_sir(i); // assign the BCR
          }

          // // MJF 3/2020 #157 #170, if a required measure has a BCR > min, then we want it to 
          // // appear with the rest of the regular measures in BCR order.
          // if (mir->Results[i].measure_required == TRUE && mir->Results[i].fBCR > mdi->key.minimum_acceptable_sir)
          //   mir->Results[i].measure_priority = MPS_SIR;

          diagnostic_results_line(i);
        }
      }

      // Following block of code used only for new measures debug output, MBG 4/07

      if (measure_file) {
        if (mir->flgRetrofits[iRetroNumber]) { // we did this retrofit

          fprintf(measure_file, "\n%8.2f%8.2f%7.2f%7.2f%5d %7d  %9.8s   %13s", 
            mir->Results[lastRndx].fEnerPreHtg / 1.e6,
            mir->Results[lastRndx].fEnerPstHtg / 1.e6, 
           (mir->Results[lastRndx].fEnerPreHtg - mir->Results[lastRndx].fEnerPstHtg) / 1.e6,
            mir->Results[lastRndx].fBCR, 
            mir->Results[lastRndx].measure_required, 
            mir->Results[lastRndx].measure_priority, 
            mir->Results[lastRndx].sComponents,
            mir->Results[lastRndx].sName);

        }
      }

    } // end of if clause for enabled retrofit

  } // End for( iRetroNumber = 0;...

  // Issue #201 all measures applied in SIR order w/o respect to required or include in sir (priority)
  for (int i = 0; i < MAXECMS; i++)
    mir->Results[i].measure_priority = MPS_SIR;

  sort_mhea_package_measures(1);    // only sorts by SIR given the universaly MPS_SIR setting above

  copy_mdi(&mdi, original);
  free(original);

  return;
}

/*******************  FUNCTION NAME: cumulative_retrofits  ****************/
/**         DATE:  1/2/93                                               **/
/**           BY:    NW, SLF                                            **/
/**  DESCRIPTION:    Performs cumulative BCR calculations and computes  **/
/**                  interacted values for savings.  The mir->Results[]   **/
/**                  array is assumed to be complete and sorted on entry**/
/*************************************************************************/
void cumulative_retrofits(void) {
  BCR_RES *LResults;       // local copy of mor on entry
  BCR_RES *res;            // just a local pointer to make coding easier to follow
  int lRndx;
  MDI *original = NULL;    // as the name implies -- our untouched MDI struct
  MDI *retrofit = NULL;    // MDI structure with implemented measures
  //float   fHtgAnnual;      // Annual heating Btus
  //float   fClgAnnual;      // Annual cooling Btus
  int iflgMeasure[MHEA_MAX_CMS];
                           // flag to indicate muliple instances of measure
                           // contributing to infiltration reduction adjustment
  int i,ndx;

  int lastRndx;           // The value of mir->Rndx prior to calling measure, measures POST increment mir->Rndx IF the measure is applied

  //int flgHeatAssigned = 0;    // is the post heating consumption assigned
  //int flgCoolAssigned = 0;    // is the post cooling consumption assigned
  float fSaveBaseHtg;         // save our base consumption on entry
  float fSaveBaseClg;
  float fInfMeasEnrgyTotHtg = 0.;
  float fInfMeasEnrgyTotClg = 0.;

  ASSERT(mdi, sprintf(msg, "The main mdi structure is missing"));
  ASSERT(mir, sprintf(msg, "The main mir structure is missing"));

  //fSaveBaseHtg = mir->fHtgBase;
  fSaveBaseHtg = mir->fPre_Heating;
  //fSaveBaseClg = mir->fClgBase;
  fSaveBaseClg = mir->fPre_Cooling;

  if (measure_file) {
    fprintf(measure_file,"\n\nCumulative Retrofits\n");  // New measure debug output, MBG 4/07
    fprintf(measure_file,"    Pre    Post   HtgSvg   SIR  Reqrd  InclC  Component    Measure\n");
  }

  // retain a copy of our mdi struct

  copy_mdi(&original, mdi);
  copy_mdi(&retrofit, mdi);

  ASSERT((LResults = (BCR_RES *)calloc(MAXECMS, sizeof(BCR_RES))), sprintf(msg, "Out of memory"));
  memcpy(LResults, mir->Results, (MAXECMS)*sizeof(BCR_RES));
  
  // prepare for second pass generating mir->results from scratch exclusively in SIR order (#201)
  memset(mir->Results, 0, (MAXECMS) * sizeof(BCR_RES));
  for (int i = 0; i < MAXECMS; i++)
    mir->Results[i].measure_priority = MPS_SIR;

  // keep a local copy of mir->Rndx then reset it to zero

  lRndx = mir->Rndx;
  mir->Rndx = 0;

  // Blank our retrofit flag array

  memset(mir->flgRetrofits, 0, MHEA_MAX_CMS * sizeof(int));

  diagnostic_results_header();

  // loop through our copy of the MHEA Output Results (mor) in BCR order from FIRST PASS

  for (i=0; i<lRndx; i++) {
    res = &LResults[i];         // assign a temp pointer

    strcpy(mir->sComponents, res->sComponents);   // put list of measure components in our global 
                                                  // string for possible use by the measure function

    // call our retrofit routine. 
    // all routines including baseload measures are called
    // in decresing order of SIR here

    lastRndx = mir->Rndx;                       // save on the way in
    ASSERT(res->measure_id >= 0 && res->measure_id < MHEA_MAX_CMS, sprintf(msg, "Out of range measure function pointer: %d", res->measure_id));
    if (measure_file) {
      fprintf(measure_file,"\nCalling Measure_Function: %d/%d for %s", i, res->measure_id, res->sName);
    }

    ASSERT(Measure_Function[res->measure_id], sprintf(msg, "Must have non null measure function pointer"));
    (*Measure_Function[res->measure_id])();   // call our retro function, sets mir->Rndx if measure is applied, resets .priority

    if (mir->Rndx == lastRndx)    // previous call did not increment this
        continue;                 // if the measure is not implemented, just continue outer loop

    // we only go on this execution path if the call to the retrofit
    // function resulted in a change to mir->Rndx (some retrofit action done)

    if (mir->Results[lastRndx].fBCR == 0.0) { // no BCR results, so compute them

      // this is NOT a base load measure, so do the interacted energy savings 
      // Set flag indicating mhea_measure_sir() can be called for measure. MBG 5/08

      mir->Results[lastRndx].flgGetBCR = 1;

      mhea_energy_use();

      mir->Results[lastRndx].fEnerPreHtg = mir->fPre_Heating;
      mir->Results[lastRndx].fEnerPstHtg = mir->fHeating_Energy * mir->fAdj_Htg;
      mir->Results[lastRndx].fEnerPreClg = mir->fPre_Cooling;
      mir->Results[lastRndx].fEnerPstClg = mir->fCooling_Energy * mir->fAdj_Clg;

      // we are doing delta version the base -- so reasign the base

      // #334 HUGE bug fix see below.  This rebase of energy
      // should ONLY be done if we are above minimum SIR  MJF 1/2021
      //mir->fPre_Heating = mir->Results[lastRndx].fEnerPstHtg;
      //mir->fPre_Cooling = mir->Results[lastRndx].fEnerPstClg;

      mhea_measure_sir(lastRndx);                // assign the BCR
    }

    // MJF 3/2020 #157, if a required measure has a BCR > min, then we want it to 
    // appear with the rest of the regular measures in BCR order.
    // if (mir->Results[lastRndx].measure_required == TRUE && 
    //   mir->Results[lastRndx].fBCR > mdi->key.minimum_acceptable_sir &&
    //   mir->Results[lastRndx].measure_priority > MPS_SIR)
    //   mir->Results[lastRndx].measure_priority = MPS_SIR;

    // #257 We want the required meaures marked for inclusion in SIR to
    // appear in SIR order regardless of the existing SIR value
    if (mir->Results[lastRndx].measure_required == TRUE && 
      mir->Results[lastRndx].measure_priority > MPS_SIR)
      mir->Results[lastRndx].measure_priority = MPS_SIR;

    // if measure not cost-effective and not required, reset the
    // mdi array so that sizing results don't reflect the measure
    // otherwise increment the retrofit mdi structure with any changes
    // made by the Measure_Function()

    if (mir->Results[lastRndx].fBCR * MHEASAVINGSADJ < mdi->key.minimum_acceptable_sir &&
        mir->Results[lastRndx].measure_required == FALSE) {
      mir->flgRetrofits[mir->Results[lastRndx].measure_id] = NO;   // turn off the flag for the measure
      copy_mdi(&mdi, retrofit);     // do NOT accumulate to the retrofit, step back to prior 
    } else {
      // we are doing delta version the base -- so reasign the base energy, leave the flag on and rebase
      // #334 HUGE bug fix.  Wow..  MJF 1/2021
      mir->fPre_Heating = mir->Results[lastRndx].fEnerPstHtg;
      mir->fPre_Cooling = mir->Results[lastRndx].fEnerPstClg;
      copy_mdi(&retrofit, mdi);     // DO accumulate mdi changes to the retrofit or package audit
    }

    if (measure_file) {
      fprintf(measure_file,"\n%8.2f%8.2f%7.2f%7.2f%5d %7d  %9.8s   %13s",
          mir->Results[lastRndx].fEnerPreHtg/1.e6,
          mir->Results[lastRndx].fEnerPstHtg/1.e6,
         (mir->Results[lastRndx].fEnerPreHtg - mir->Results[lastRndx].fEnerPstHtg)/1.e6,
          mir->Results[lastRndx].fBCR,
          mir->Results[lastRndx].measure_required,
          mir->Results[lastRndx].measure_priority,
          mir->Results[lastRndx].sComponents,
          mir->Results[lastRndx].sName);
    }
    diagnostic_results_line(lastRndx);

  } //  lRndx loop

  retro_itemized();    // lets add our itemized costs and measures

  sort_mhea_package_measures(1);      // some SIRs may have changed, so re-sort

  // Alter post-retrofit energy to create result of multiplying
  // savings by MHEASAVINGSADJ per MHEA Field Test / Steering 
  // Committee recommendations.  MBG 4/08, 7/08.
  for (i=0; i<mir->Rndx; i++) {

    // skip those measures that do not call mhea_measure_sir (things like base load measures)
    if(mir->Results[i].flgGetBCR == 0) 
      continue; 

    if(mir->Results[i].PreHtgFuel == mir->Results[i].PstHtgFuel) {
     // Don't adjust post energies if fuel-switching
       mir->Results[i].fEnerPstHtg = 
             (1.0f-MHEASAVINGSADJ) * mir->Results[i].fEnerPreHtg +
                MHEASAVINGSADJ     * mir->Results[i].fEnerPstHtg; 
    }

    mir->Results[i].fBCR = 0.0f;    // Zero out so mhea_measure_sir sees need to 
    mhea_measure_sir( i ); 
  }                // recompute BCR

  // Eliminate measures no longer cost-effective after application of adjustment factor.  MBG 7/08
  // Reset mir->flgRetrofits if rejected.  MBG 11/15/11 Version 89
  // #279 earlier version has some memcpy shifting and re-indexing the for() loop that did NOT
  // run the same across platforms.  So, instead we make another copy of the whole results set into LResults, blank
  // the mir->Results stucture, then selectively copy back from LResults to mir_Results for those measures that
  // are to be implemented

  memcpy(LResults, mir->Results, (MAXECMS) * sizeof(BCR_RES));
  memset(mir->Results, 0, (MAXECMS) * sizeof(BCR_RES));
  int implemented_count = 0;

  for (i=0; i<mir->Rndx; i++) {

    res = &LResults[i];         // assign a temp pointer

    // if the INDIVIDUAL SIR is less than the threshold and
    // the measure is not required, then exlcude that measure

    if (res->fBCR < mdi->key.minimum_acceptable_sir &&
       res->measure_required == FALSE) {
      // do nothing, the measure is not copied forwared
    } else {
      memcpy(&mir->Results[implemented_count], &LResults[i], (1) * sizeof(BCR_RES));
      implemented_count++;
    }
  }
  mir->Rndx = implemented_count;

  // Call mhea_energy_use() final time to base sizing estimates on retrofitted house

  mhea_energy_use();

  // Determine amount needed to adjust general infiltration 
  // savings due to all implemented measures that reduce air leakage.

  for (i=0; i<MHEA_MAX_CMS; i++) 
    iflgMeasure[i]=0;

  for (i=0; i<mir->Rndx; i++) {   
    ndx = mir->Results[i].measure_id;
    if(iflgMeasure[ndx]) continue;
    iflgMeasure[ndx]=1;
    fInfMeasEnrgyTotHtg += mir->fInfMeasEnrgyHtg[ndx] * MHEASAVINGSADJ;
    fInfMeasEnrgyTotClg += mir->fInfMeasEnrgyClg[ndx];
  }

  // Loop through applied measures again, locate infiltration reduction,
  // and reduce its savings by increasing its associated post-retrofit
  // energy consumption. Then re-compute BCRs.

  for (i=0; i<mir->Rndx; i++) {

    if(mir->Results[i].measure_id == M_CMS_GENERAL_AIR_SEALING &&
       fInfMeasEnrgyTotHtg > 0.0f) { 
      mir->Results[i].fEnerPstHtg += fInfMeasEnrgyTotHtg;
      mir->Results[i].fEnerPstClg += fInfMeasEnrgyTotClg;
      // Don't go into negative energy savings territory for heating or cooling
      if(mir->Results[i].fEnerPstHtg > mir->Results[i].fEnerPreHtg)
         mir->Results[i].fEnerPstHtg = mir->Results[i].fEnerPreHtg;
      if(mir->Results[i].fEnerPstClg > mir->Results[i].fEnerPreClg)
         mir->Results[i].fEnerPstClg = mir->Results[i].fEnerPreClg;
      mir->Results[i].fBCR = 0.0f;    // Zero out so mhea_measure_sir sees need to recompute BCR
      mhea_measure_sir( i ); 
    }

    // For non-baseload fuel switching measures (heating system replacement)
    // alter post-retrofit energy to create result of multiplying
    // savings by MHEASAVINGSADJ). Do NOT call mhea_measure_sir().

    else if(mir->Results[i].PreHtgFuel != mir->Results[i].PstHtgFuel &&
            mir->Results[i].flgGetBCR != 0) {
      mir->Results[i].fEnerPstHtg = 
        (1.0f-MHEASAVINGSADJ) * mir->Results[i].fEnerPreHtg +
        MHEASAVINGSADJ * mir->Results[i].fEnerPstHtg; 
    } 
  }

  // We need to subtract the energy savings for each APPLIED 
  // measure in the GLBResult list at this point (ie. after we have
  // done the final sort on SIR and discarded measures that do not
  // meet the minimum SIR requirement.

  mir->fFinal_Heating = fSaveBaseHtg;    // start with base consumption
  mir->fFinal_Cooling = fSaveBaseClg;

  // Adjust our Infiltration reducation and Duct Seal measures to normal set of energy savings measures
  // if they are in the list of implemented measures

  for (i=0; i<mir->Rndx; i++) {
    if (mir->Results[i].measure_id == M_CMS_SEAL_DUCTS) {
      mir->Results[i].measure_priority = MPS_SIR;
      mir->Results[i].measure_required = TRUE;
    }
    if (mir->Results[i].measure_id == M_CMS_GENERAL_AIR_SEALING) {
      mir->Results[i].measure_priority = MPS_SIR;
      mir->Results[i].measure_required = TRUE;
    }
  }

  // Call SortResults ======FINAL====== time to put measures in reporting order.

  sort_mhea_package_measures(0);

  // we have added an item to mor then
  // cummulative statistics stored in the mor array as well
  // note special start condition for our accumulators

  for (i=0; i<mir->Rndx; i++) {
    if (i == 0)                 // first time through
        {
        mir->Results[i].fTotAnnualSav = mir->Results[i].fCostAnnSavTot;
        mir->Results[i].fTotInitCost  = mir->Results[i].fInitCost;
        mir->Results[i].fTotPWSav     = mir->Results[i].fCostSavTot;
        }
    else
        {
        mir->Results[i].fTotAnnualSav =
            mir->Results[i-1].fTotAnnualSav +
            mir->Results[i].fCostAnnSavTot;
        mir->Results[i].fTotInitCost =
            mir->Results[i-1].fTotInitCost +
            mir->Results[i].fInitCost;
        mir->Results[i].fTotPWSav =
            mir->Results[i-1].fTotPWSav +
            mir->Results[i].fCostSavTot;
        } 

    // now for our cummulative SIR
    // extra check to make sure our cummulative initial costs != 0

    if (mir->Results[i].measure_priority >= MPS_SIR &&
        mir->Results[i].fTotInitCost > 0.0)                         
        mir->Results[i].fTotSIR =
            mir->Results[i].fTotPWSav /
            mir->Results[i].fTotInitCost;
    else
        mir->Results[i].fTotSIR = 0.0;

  }

  if (measure_file) {
    fprintf(measure_file,"\n\nFinal Results for Applied Measures \n");
    fprintf(measure_file,"   Pre   Post  HtgSvg  SIR Reqrd InclC Component   Measure\n");
  }

  if (cmds.debug_level & D_NORMAL)
    fprintf(stderr, "\n\nFINAL");
  diagnostic_results_header();

  for (i=0; i<mir->Rndx; i++) {       // loop and subtract for each measure
    mir->fFinal_Heating -= (mir->Results[i].fEnerPreHtg - mir->Results[i].fEnerPstHtg);
    mir->fFinal_Cooling -= (mir->Results[i].fEnerPreClg - mir->Results[i].fEnerPstClg);

    if (measure_file) {
      fprintf(measure_file,"\n%7.1f%7.1f%6.1f%6.1f%4d %6d  %8.8s   %12s",
        mir->Results[i].fEnerPreHtg/1.e6,
        mir->Results[i].fEnerPstHtg/1.e6,
        (mir->Results[i].fEnerPreHtg - mir->Results[i].fEnerPstHtg)/1.e6,
        mir->Results[i].fBCR,
        mir->Results[i].measure_required,
        mir->Results[i].measure_priority,
        mir->Results[i].sComponents,
        mir->Results[i].sName);
    }
    diagnostic_results_line(i);
  }

  if (cmds.debug_level & D_NORMAL) {
    fprintf(stderr, "\n\nAnnual Heating kBtu: %8.1f  Annual Cooling kBtu: %8.1f", mir->fFinal_Heating/1000, mir->fFinal_Cooling/1000);
  }

  copy_mdi(&mdi, original);       // back to original MDI structure

  free(original);
  free(retrofit);     // TODO some day echo this as the as-built JSON
  free(LResults);

  if (measure_file)
    fclose(measure_file);

  return;
}

static void diagnostic_results_header(void){
  if (cmds.debug_level & D_NORMAL) {
    fprintf(stderr, "\n%s", " i  mat p r                                                  esav     dsav    icost life    sir");
  }
}

static void diagnostic_results_line(int i) {

  if (cmds.debug_level & D_NORMAL) {
    BCR_RES *res;             // just a local pointer to make coding easier
    res = &mir->Results[i];   // assign a temp pointer

    fprintf(stderr, "\n%2d %3d %2d %1d  %-30.30s  %-10.10s  %8.1f %8.2f %8.2f %4.0f %6.2f", 
      i, 
      res->material_id,
      res->measure_priority,
      res->measure_required,
      res->sName,
      res->sComponents,
      res->fEnerSavTot,
      res->fCostSavTot,
      res->fInitCost,
      res->fLife,
      res->fBCR);
  }

}
