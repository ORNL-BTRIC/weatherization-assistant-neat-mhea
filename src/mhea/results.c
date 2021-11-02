/*******************  MODULE NAME:  RESULTS.C  ***************************/
/**       DATE: 1/02/94                                                 **/
/**          BY:    SLF                                                 **/
/** DESCRIPTION:    Contains instructions for writing results output    **/
/**                 files in mhea_results() function called by Run()    **/
/**                 in MENUS.C.                                         **/
/**  REVISIONS:  03/10/94 SLF Added code to create output data file     **/
/**                             for graphing.                           **/
/**                  10/12/94 SLF Added WriteInputs().                  **/
/**                  11/17/94 SLF Reordered mor indices.         **/
/**                   8/17/95 NLW Corrected results indexing.           **/
/**                   8/18/95 NLW Added version labels to output.       **/
/**                   8/29/95 NLW 2nd indexing correction.              **/
/**                 3/26/97 NLW Add Heating System Sizing Output        **/
/**                 5/28/97 NLW Added Other Cost flag to cost results   **/
/**                             SIR and input summary file              **/
/**                 6/30/97 MJF Added Comments to end of file           **/
/**                 7/18/97 NLW Added title and project name to reports **/
/**                 7/18/97 NLW Added spending limit warning to output  **/
/**                 7/29/97 NLW Added stepwall to input summary         **/
/**                 7/29/97 NLW Corrected flgCostsIncl text in          **/
/**                             input summary                           **/
/**                 7/29/97 NLW Shortened fibreglass door label         **/
/*************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "wa_engine.h"

static void add_manual_j_heat_load(char *type, float pre, float post);

// populate the mor (MHEA Output Report) structure from our mir (MHEA Intermediate Results) structure
void mhea_results(int iflgBillAdj) {
  FILE *fp = NULL;
  // char sOutputFile[50];
  char buffer[STRING_LEN];
  int i;
  BCR_RES *res;      // just a local pointer to make coding easier
  int BellyMeas = 0; // flag indicating that a belly insul. mesure is to be done

  //int spending_flag = 0; // Flag indicating whether spending limit has been reported.

  float fDuctRatio1 = 0.0; // Computed duct loss fraction - base case
  float fDuctRatio2 = 0.0; // Computed duct loss fraction - retrofit case
  float fDuctDelta1 = 0.0; // Used in duct loss fraction calculation
  float fDuctDelta2 = 0.0; // Used in duct loss fraction calculation
                           // float acost = 0.0;        // keeps track of accumulated costs

  //spending_flag = 0;
  fDuctRatio1 = 0.0;
  fDuctRatio2 = 0.0;

  ASSERT(mor, sprintf(msg, "The main mor structure is missing"));

  /***********************/
  /* Unified report file */
  /***********************/

  /**************************/
  /* Measure Annual Savings */
  /**************************/

  // STRCPY(sOutputFile, GLBoutputDirectoryMHEA);
  // STRCAT(sOutputFile, GLBprojectNameMHEA);
  // STRCAT(sOutputFile, ".rep");
  // if (!(OutFile = fopen(sOutputFile, "w")))
  //   return (usr_msg(MAIN_MSG, NO_OUTPUT_RESULTS, sOutputFile));

  if (strcmp(cmds.mhea_compare_file_path, NO_OUTPUT) != 0) {
    fp = fopen(cmds.mhea_compare_file_path, "w");
    ASSERT(fp, sprintf(msg, "Couldn't open MHEA report output file: %s code:%d:%s", cmds.mhea_compare_file_path, errno, strerror(errno)));
  }

  if (fp) {
    
    if(cmds.regression_test)
      fprintf(fp, "%s\n", "MHEA ");
    else
      fprintf(fp, "%s\n", "MHEA " WA_VERSION);

    fprintf(fp, "MHEA Retrofit Measure Savings Report ");
    fprintf(fp, "      Audit Identifier: %s \n\n", cmds.run_identifier);

    // fprintf(fp, " Client Name: %s \n", mdi->gnl.client_name);
    // fprintf(fp, "     Address: %s \n"
    //                 "              %s  %s \n",
    //        mdi->gnl.address, mdi->gnl.city_state, mdi->gnl.zip_code);
    fprintf(fp, "Weather Site: %s, %s \n", mdi->wth.city, mdi->wth.state);
    // fprintf(fp, "  Audit Date: %s\n", mdi->gnl.audit_date);

    // fprintf(fp, "     Auditor: %s \n", mdi->gnl.auditor);
    fprintf(fp, "  Job Number: %ld \n", mdi->gnl.audit_number);
    if (iflgBillAdj)
      fprintf(fp, "        Note: Results have been adjusted using BILLING DATA. \n");
  }

 // now populate output results

  for (i = 0; i < mir->Rndx; i++) {
    int cnt;

    res = &mir->Results[i]; // for simpler coding, faster too

    // our composite results struture

    cnt = mor->num_measure;
    mor->measure[cnt].index = cnt + 1; // number to order the list
    res->measure_index = mor->measure[cnt].index;    // for future references as FK
    mor->measure[cnt].measure_id = res->measure_id;
    mor->measure[cnt].audit_section_id = res->audit_section_id;
    mor->measure[cnt].component_id = res->component_id;  // non zero for retro_itemized(). Pass through only
    STRCPY(mor->measure[cnt].measure, res->sName);
    STRCPY(mor->measure[cnt].components, res->sComponents);
    mor->measure[cnt].lifetime = res->fLife;

    mor->measure[cnt].heating_mmbtu = res->fEnerSavHtg; // annual MMBtu heating savings
    mor->measure[cnt].heating_sav = res->fCostSavHtg;  // annual dollar savings for heating
    mor->measure[cnt].cooling_kwh = res->fEnerSavClg * KWH_PER_MMBTU;  // annual kwh cooling savings
    mor->measure[cnt].cooling_sav = res->fCostSavClg;  // annual dollar savings for cooling
    mor->measure[cnt].baseload_kwh = res->fEnerSavBas * KWH_PER_MMBTU; // annual kwh baseload savings
    mor->measure[cnt].baseload_sav = res->fCostSavBas; // annual dollar savings for baseloads
    mor->measure[cnt].total_mmbtu = res->fEnerSavTot;   // total annual heating and cooling MMBtu

    mor->measure[cnt].savings = res->fCostAnnSavTot; // total annual heating and cooling dollar savings
    mor->measure[cnt].cost = res->fInitCost;         // total initial costs
    mor->measure[cnt].sir = res->fBCR;               // computed savings to investment ratio
    mor->measure[cnt].qtym = res->fQuant;            // The quantity of the material itself in units (new 7/04)
    mor->measure[cnt].qtyl = res->fQuant;            // The quantity for labor cost calculation (new 7/04)
    mor->measure[cnt].costum = res->fUnitMaterial;
    mor->measure[cnt].costul = res->fUnitLabor;

    mor->measure[cnt].qtyi = 1; // Extra itemized cost LUMPED in MHEA
    if (res->measure_id == M_CMS_REPLACE_WINDOWS || 
        res->measure_id == M_CMS_REPLACE_WINDOWS_ADD ||
        res->measure_id == M_CMS_PLASTIC_STORM_WINDOWS || 
        res->measure_id == M_CMS_PLASTIC_STORM_WINDOWS_ADD ||
        res->measure_id == M_CMS_GLASS_STORM_WINDOWS || 
        res->measure_id == M_CMS_GLASS_STORM_WINDOWS_ADD ||
        res->measure_id == M_CMS_WINDOW_SEALING || 
        res->measure_id == M_CMS_WINDOW_SEALING_ADD )
      mor->measure[cnt].qtyi = res->fReportQuant;
    // Use number of windows/lights for quantity

    mor->measure[cnt].costi1 = res->costi1;

    mor->measure[cnt].costi2 = res->costi2;
    STRCPY(mor->measure[cnt].desci2, res->desci2);
    mor->measure[cnt].typei2 = res->typei2;

    mor->measure[cnt].costi3 = res->costi3;
    STRCPY(mor->measure[cnt].desci3, res->desci3);
    mor->measure[cnt].typei3 = res->typei3;

    mor->num_measure++;
  }

  if (fp) {
    fprintf(fp, "--------------------------------------------------------------------------------- \n");
    fprintf(fp, "                                        Annual Energy Savings             \n");
    fprintf(fp, "                      ----------------------------------------------------------- \n");
    fprintf(fp, "Recommended               Heating       Cooling       BaseLoad         Total      \n");
    fprintf(fp, "Measures               (MMBtu)  ($)   (kWh)   ($)   (kWh)   ($)    (MMBtu) ($)    \n");
    fprintf(fp, "--------------------------------------------------------------------------------- \n");
  }
  for (i = 0; i < mir->Rndx; i++) {

    res = &mir->Results[i]; // for simpler coding, faster too

    // some itemized costs with zero savings may be in results and we don't want to report
    // those results in this section on annyual energy savings.  Other results with zero
    // savings should be reported #257
    if ((res->measure_priority == MPS_ITEMIZED_NO_SAVE_SIR || res->measure_priority == MPS_ITEMIZED_NO_SIR) && res->fEnerSavTot == 0.0f) 
      continue;

    STRNCPY(buffer, res->sName, 21); // truncate name
    // buffer[21] = '\0';               // with temp string

    if (fp)
      fprintf(fp, "%21s   %5.1f  %4.0f   %5.0f  %4.0f   %5.0f  %4.0f     %5.1f %4.0f \n", buffer, res->fEnerSavHtg,
              res->fCostSavHtg, res->fEnerSavClg * KWH_PER_MMBTU, res->fCostSavClg, res->fEnerSavBas * KWH_PER_MMBTU, res->fCostSavBas,
              res->fEnerSavTot, res->fCostAnnSavTot);

    if (iflgBillAdj) {
      int cnt = mor->num_an_asav; // making coding easier

      mor->an_asav[cnt].index = cnt + 1;
      mor->an_asav[cnt].measure_index = res->measure_index;   // FK

      STRCPY(mor->an_asav[cnt].measure, res->sName);
      STRCPY(mor->an_asav[cnt].components, res->sComponents);
      mor->an_asav[cnt].heating_mmbtu = res->fEnerSavHtg;
      mor->an_asav[cnt].heating_sav = res->fCostSavHtg;
      mor->an_asav[cnt].cooling_kwh = res->fEnerSavClg * KWH_PER_MMBTU;
      mor->an_asav[cnt].cooling_sav = res->fCostSavClg;
      mor->an_asav[cnt].baseload_kwh = res->fEnerSavBas * KWH_PER_MMBTU;
      mor->an_asav[cnt].baseload_sav = res->fCostSavBas;
      mor->an_asav[cnt].total_mmbtu = res->fEnerSavTot;

      mor->num_an_asav++;
    } else {
      int cnt = mor->num_an_sav; // making coding easier

      mor->an_sav[cnt].index = cnt + 1;
      mor->an_sav[cnt].measure_index = res->measure_index;   // FK

      STRCPY(mor->an_sav[cnt].measure, res->sName);
      STRCPY(mor->an_sav[cnt].components, res->sComponents);
      mor->an_sav[cnt].heating_mmbtu = res->fEnerSavHtg;
      mor->an_sav[cnt].heating_sav = res->fCostSavHtg;
      mor->an_sav[cnt].cooling_kwh = res->fEnerSavClg * KWH_PER_MMBTU;
      mor->an_sav[cnt].cooling_sav = res->fCostSavClg;
      mor->an_sav[cnt].baseload_kwh = res->fEnerSavBas * KWH_PER_MMBTU;
      mor->an_sav[cnt].baseload_sav = res->fCostSavBas;
      mor->an_sav[cnt].total_mmbtu = res->fEnerSavTot;

      mor->num_an_sav++;
    }

    // set our belly measure flag if this is a belly blow mesure (used
    // later to possible print out messages about belly measure details)

    if (res->measure_id == M_CMS_BELLY_CELLULOSE_LOOSE_INSL || res->measure_id == M_CMS_BELLY_CELLULOSE_LOOSE_INSL_ADD ||
        res->measure_id == M_CMS_BELLY_FIBERGLASS_LOOSE_INSL || res->measure_id == M_CMS_BELLY_FIBERGLASS_LOOSE_INSL_ADD)
      BellyMeas = 1;
  }

  if (!iflgBillAdj) { // only output once the first time through for unadjusted results

    /*******************************/
    /* Pre/Post Annual Consumption */
    /*******************************/

    mor->pre_heat = mir->fBasecase_Heating / 1e6f;                    // pre retrofit heating MMBtu
    mor->pre_cool = mir->fBasecase_Cooling / 1e6f * KWH_PER_MMBTU;    // pre retrofit cooling kWh
    mor->pre_base = mir->fBasecase_Baseload / 1e6f * KWH_PER_MMBTU;   // pre retrofit base loads kWh
    mor->post_heat = mir->fFinal_Heating / 1e6f;                      // post retrofit heating MMBtu
    mor->post_cool = mir->fFinal_Cooling / 1e6f * KWH_PER_MMBTU;      // post retorfit cooling kWh
    mor->post_base = mir->fFinal_Baseload / 1e6f * KWH_PER_MMBTU;     // post retrofit base loads kWh

    if (fp) {
      fprintf(fp, "-------------------------------------------------------- \n\n");
      fprintf(fp, "                            Annual Consumption                \n");
      fprintf(fp, "                        Heating  Cooling  BaseLoad \n");
      fprintf(fp, "                        (MMBtu)  (kWh)     (kWh) \n");
      fprintf(fp, "                        -------------------------------- \n");
      fprintf(fp, "Pre-Weatherization      %6.1f      %5.0f      %5.0f\n", mor->pre_heat, mor->pre_cool, mor->pre_base);
      fprintf(fp, "Post-Weatherization     %6.1f      %5.0f      %5.0f\n", mor->post_heat, mor->post_cool, mor->post_base);
      fprintf(fp, "------------------------------------------------------------------ \n\n");
    }

    /*******************/
    /* Misc. Messages  */
    /*******************/

    if (mir->flgPreHighHTGLoad == TRUE) {
      STRCPY(mir->sMsg, "Pre-weatherization heating load may not be met in all months.");
      if (fp)
        fprintf(fp, " \n %s \n", mir->sMsg);
      add_mhea_message(mir->sMsg);
    }
    if (mir->flgPreHighCLGLoad == TRUE) {
      STRCPY(mir->sMsg, "Pre-weatherization cooling load may not be met in all months.");
      if (fp)
        fprintf(fp, " \n %s \n", mir->sMsg);
      add_mhea_message(mir->sMsg);
    }
    if (mir->flgPreHighHTGLoad == TRUE || mir->flgPreHighCLGLoad == TRUE) {
      STRCPY(mir->sMsg, "\0");
      for (i = 0; i < 12; i++) {
        if (mir->iPreHighLoadMonths[i] == TRUE) {
          sprintf(buffer, " %d", i + 1);
          STRCAT(mir->sMsg, buffer);
        }
      }
      sprintf(buffer, "Loads may not be met in month(s): %s", mir->sMsg);
      if (fp)
        fprintf(fp, "\n [%s ] \n", buffer);
      add_mhea_message(buffer);
    }

    if (mir->flgPostHighHTGLoad == TRUE) {
      STRCPY(mir->sMsg, "Post-weatherization heating load may not be met in all months.");
      if (fp)
        fprintf(fp, " \n %s \n", mir->sMsg);
      add_mhea_message(mir->sMsg);
    }
    if (mir->flgPostHighCLGLoad == TRUE) {
      STRCPY(mir->sMsg, "Post-weatherization cooling load may not be met in all months.");
      if (fp)
        fprintf(fp, " \n %s \n", mir->sMsg);
      add_mhea_message(mir->sMsg);
    }
    if (mir->flgPostHighHTGLoad == TRUE || mir->flgPostHighCLGLoad == TRUE) {
      STRCPY(mir->sMsg, "\0");
      for (i = 0; i < 12; i++) {
        if (mir->iPostHighLoadMonths[i] == TRUE) {
          sprintf(buffer, " %d", i + 1);
          STRCAT(mir->sMsg, buffer);
        }
      }
      sprintf(buffer, "Loads may not be met in month(s): %s", mir->sMsg);
      if (fp)
        fprintf(fp, "\n [%s ] \n", buffer);
      add_mhea_message(buffer);
    }

    // messages for restricting blown type insulation in the floor/belly spaces

    if (BellyMeas == TRUE) {
      if (mir->flgLimitBellyInsul == TRUE) {
        add_mhea_message("The depth of added loose insulation in the BELLY has been limited to 8 in.");
        add_mhea_message("You may want to consider physically restricting the belly depth as part of the insulation measure.");
        add_mhea_message("Insure the belly perimeter cavities are filled with insulation.");
      }

      if (mir->flgLimitBellyInsulAdd == TRUE) {
        add_mhea_message("The depth of added loose insulation in the ADDITION has been limited to 8 in.");
        add_mhea_message("Consider adding wrap material or use batt type insulation in the Addition.");
      }
    }

    /*******************/
    /* Manual J output */
    /*******************/

    if (fp) {
      fprintf(fp, " ---------------------------------------------------\n");
      fprintf(fp, " Modified Manual-J Heating System Sizing Estimate ** \n");
      fprintf(fp, " ---------------------------------------------------\n\n");
      fprintf(fp, "                Pre-Weatherization  Post-Weatherization\n"
                  "                   (Btu / Hour)         (Btu / Hour)\n"
                  "                ------------------  --------------------\n\n");
      fprintf(fp, "    Wall            %8.0f          %8.0f \n",   mir->Htg_Sizing[PRE_RETROFIT].fWal_loss, mir->Htg_Sizing[POST_RETROFIT].fWal_loss);
      fprintf(fp, "    Floor           %8.0f          %8.0f \n",   mir->Htg_Sizing[PRE_RETROFIT].fFlr_loss, mir->Htg_Sizing[POST_RETROFIT].fFlr_loss);
      fprintf(fp, "    Roof            %8.0f          %8.0f \n",   mir->Htg_Sizing[PRE_RETROFIT].fRof_loss, mir->Htg_Sizing[POST_RETROFIT].fRof_loss);
      fprintf(fp, "    Windows         %8.0f          %8.0f \n",   mir->Htg_Sizing[PRE_RETROFIT].fWin_loss, mir->Htg_Sizing[POST_RETROFIT].fWin_loss);
      fprintf(fp, "    Doors           %8.0f          %8.0f \n",   mir->Htg_Sizing[PRE_RETROFIT].fDor_loss, mir->Htg_Sizing[POST_RETROFIT].fDor_loss);
      fprintf(fp, "    Infiltration    %8.0f          %8.0f \n",   mir->Htg_Sizing[PRE_RETROFIT].fInf_loss, mir->Htg_Sizing[POST_RETROFIT].fInf_loss);
      fprintf(fp, "    Duct Loss       %8.0f          %8.0f \n\n", mir->Htg_Sizing[PRE_RETROFIT].fDuc_loss, mir->Htg_Sizing[POST_RETROFIT].fDuc_loss);
      fprintf(fp, "    Total           %8.0f          %8.0f \n\n", mir->Htg_Sizing[PRE_RETROFIT].fTot_loss, mir->Htg_Sizing[POST_RETROFIT].fTot_loss);
    }

    add_manual_j_heat_load("Wall",          mir->Htg_Sizing[PRE_RETROFIT].fWal_loss, mir->Htg_Sizing[POST_RETROFIT].fWal_loss);
    add_manual_j_heat_load("Floor",         mir->Htg_Sizing[PRE_RETROFIT].fFlr_loss, mir->Htg_Sizing[POST_RETROFIT].fFlr_loss);
    add_manual_j_heat_load("Roof",          mir->Htg_Sizing[PRE_RETROFIT].fRof_loss, mir->Htg_Sizing[POST_RETROFIT].fRof_loss);
    add_manual_j_heat_load("Windows",       mir->Htg_Sizing[PRE_RETROFIT].fWin_loss, mir->Htg_Sizing[POST_RETROFIT].fWin_loss);
    add_manual_j_heat_load("Doors",         mir->Htg_Sizing[PRE_RETROFIT].fDor_loss, mir->Htg_Sizing[POST_RETROFIT].fDor_loss);
    add_manual_j_heat_load("Infiltration",  mir->Htg_Sizing[PRE_RETROFIT].fInf_loss, mir->Htg_Sizing[POST_RETROFIT].fInf_loss);
    add_manual_j_heat_load("Duct Loss",     mir->Htg_Sizing[PRE_RETROFIT].fDuc_loss, mir->Htg_Sizing[POST_RETROFIT].fDuc_loss);
    add_manual_j_heat_load("Total",         mir->Htg_Sizing[PRE_RETROFIT].fTot_loss, mir->Htg_Sizing[POST_RETROFIT].fTot_loss);

    if (fp) {
      fprintf(fp, "    (Sizing Calculations for this analysis\n");
      fprintf(fp, "         based on the following parameters:\n");
      fprintf(fp, "         o    70 degree F indoor design temperature,\n");
      fprintf(fp, "         o%6.0f degree F outdoor design temperature,\n", cwd->heating_design_temp);
    }
    sprintf(buffer, "ManualJ sizing based on 70F indoor and %3.0fF outdoor temp", cwd->heating_design_temp);
    add_mhea_message(buffer);

    // 5/28/97,NLW: Added computed duct loss fraction to output report
    // 7/21/97,NLW: Prevent divide-by-zero

    fDuctDelta1 = (mir->Htg_Sizing[PRE_RETROFIT].fTot_loss  - mir->Htg_Sizing[PRE_RETROFIT].fDuc_loss);
    fDuctDelta2 = (mir->Htg_Sizing[POST_RETROFIT].fTot_loss - mir->Htg_Sizing[POST_RETROFIT].fDuc_loss);
    if (fDuctDelta1 != 0.0)
      fDuctRatio1 = mir->Htg_Sizing[PRE_RETROFIT].fDuc_loss / fDuctDelta1 * 100.0f;
    if (fDuctDelta2 != 0.0)
      fDuctRatio2 = mir->Htg_Sizing[POST_RETROFIT].fDuc_loss / fDuctDelta2 * 100.0f;

    sprintf(buffer, "%3.0f Base case duct loss fraction", fDuctRatio1);
    if (fp)
      fprintf(fp, "         o   %s,\n", buffer);
    add_mhea_message(buffer);

    sprintf(buffer, "%3.0f Retrofit case duct loss fraction", fDuctRatio2);
    if (fp)
      fprintf(fp, "         o   %s )\n\n", buffer);
    add_mhea_message(buffer);

    if (fp) {
      fprintf(fp, "    ** IMPORTANT: This sizing estimate is provided as a\n");
      fprintf(fp, "       general guideline and should be reviewed by a\n");
      fprintf(fp, "       qualified heating contractor.\n\n");
      if (!cmds.regression_test){
        fprintf(fp, "---------------------------------------------- MHEA " WA_VERSION " ---- \n");
      } else {
        fprintf(fp, "---------------------------------------------- MHEA ---- \n");
      }

    }
    add_mhea_message("Sizing estimate are general guidelines only");
    add_mhea_message("Sizing estimate should be review by qualified heating contractor");
    add_mhea_message("(+) in the Materials list indicates there are more related User Defined Materials");
  }

  /**************/
  /* SIR output */
  /**************/

  if (fp) {
    if (iflgBillAdj)
      fprintf(fp, "        Note: Results have been adjusted using billing data. \n");
    fprintf(fp, "-------------------------------------------------------------------- \n");
    fprintf(fp, "                             Measure               Cumulative        \n");
    fprintf(fp, "                      ----------------------  ---------------------- \n");
    fprintf(fp, "Recommended           Savings   Costs    SIR  Savings   Costs    SIR \n");
    fprintf(fp, "Measures              ($/year)   ($)          ($/year)   ($)         \n");
    fprintf(fp, "-------------------------------------------------------------------- \n");
  }

  for (i = 0; i < mir->Rndx; i++) {
    res = &mir->Results[i];            // for simpler coding, faster too
    enum MEASURE_PACKAGE_SORT_PRIORITY priority = res->measure_priority;

    STRNCPY(buffer, res->sName, 21); // truncate name
    // buffer[21] = '\0';               // with temp string

    if (fp)
      fprintf(fp, "%21s   %4.0f    %4.0f   %5.1f    %4.0f    %4.0f   %5.1f \n", buffer, res->fCostAnnSavTot, res->fInitCost,
              res->fBCR, res->fTotAnnualSav, res->fTotInitCost, res->fTotSIR);

    if (iflgBillAdj) {
      int cnt = mor->num_asir; // making coding easier

      mor->asir[cnt].index = cnt + 1;
      mor->asir[cnt].measure_index = res->measure_index;   // FK

      mor->asir[cnt].group = measure_package_group(priority);
      STRCPY(mor->asir[cnt].measure, res->sName);
      STRCPY(mor->asir[cnt].components, res->sComponents);
      mor->asir[cnt].savings = res->fCostAnnSavTot;
      mor->asir[cnt].cost = res->fInitCost;
      mor->asir[cnt].sir = res->fBCR;
      mor->asir[cnt].csav = res->fTotAnnualSav;
      mor->asir[cnt].ccost = res->fTotInitCost;
      mor->asir[cnt].csir = res->fTotSIR;
      mor->num_asir++;
    } else {
      int cnt = mor->num_sir; // making coding easier

      mor->sir[cnt].index = cnt + 1;
      mor->sir[cnt].measure_index = res->measure_index;   // FK

      mor->sir[cnt].group = measure_package_group(priority);
      STRCPY(mor->sir[cnt].measure, res->sName);
      STRCPY(mor->sir[cnt].components, res->sComponents);
      mor->sir[cnt].savings = res->fCostAnnSavTot;
      mor->sir[cnt].cost = res->fInitCost;
      mor->sir[cnt].sir = res->fBCR;
      mor->sir[cnt].csav = res->fTotAnnualSav;
      mor->sir[cnt].ccost = res->fTotInitCost;
      mor->sir[cnt].csir = res->fTotSIR;
      mor->num_sir++;
    }
  }

  if (!iflgBillAdj) // check if spending limit exceeded
  {
    if (mir->Results[i - 1].fTotInitCost > mdi->key.spending_limit) {
      sprintf(buffer, "Cumulative Expenditure Exceeds Limit of %5.0f Dollars", mdi->key.spending_limit);
      if (fp)
        fprintf(fp, " \nWARNING: %s \n\n", buffer);
      add_mhea_message(buffer);
    }
  }

  /*********************/
  /* Materials         */
  /*********************/

  if (fp) {
    if (iflgBillAdj)
      fprintf(fp, "        Note: Results have been adjusted using billing data. \n");

    fprintf(fp, "---------------------------------------------------- \n");
    fprintf(fp, "                                                     \n");
    fprintf(fp, "Material                             Quantity  Units  \n");
    fprintf(fp, "----------------------------------------------------- \n");
  }

  for (i = 0; i < mir->Rndx; i++) {

    res = &mir->Results[i];                      // for simpler coding, faster too
    STRNCPY(buffer, res->sReportMaterial, 36); // truncate name
    // buffer[36] = '\0';                         // with temp string

    if (strlen(buffer)) // there is a material description
    {
      if (fp)
        fprintf(fp, "%36s   %5.1f  %6s\n", buffer, res->fReportQuant, res->sReportUnits);

      if (iflgBillAdj) {
        int cnt = mor->num_amaterial; // making coding easier

        mor->amaterial[cnt].index = cnt + 1;
        mor->amaterial[cnt].material_id = res->material_id;
        mor->amaterial[cnt].measure_index = res->measure_index;   // FK

        STRCPY(mor->amaterial[cnt].material, res->sReportMaterial);
        // STRCPY(mor->amaterial[cnt].type, retrofit_materials[i].type);
        mor->amaterial[cnt].quantity = res->fReportQuant;
        STRCPY(mor->amaterial[cnt].units, res->sReportUnits);

        mor->num_amaterial++;
      } else {
        int cnt = mor->num_material; // making coding easier

        mor->material[cnt].index = cnt + 1;
        mor->material[cnt].material_id = res->material_id;
        mor->material[cnt].measure_index = res->measure_index;   // FK

        STRCPY(mor->material[cnt].material, res->sReportMaterial);
        // STRCPY(mor->material[cnt].type, retrofit_materials[i].type);
        mor->material[cnt].quantity = res->fReportQuant;
        STRCPY(mor->material[cnt].units, res->sReportUnits);
        
        mor->num_material++;
      }
    }
  }

  mor->num_used_fuel = used_fuel_results(mor->used_fuel); // show the details for the fuels used

  if (fp)
    fclose(fp);

  //return (OK);
  return;
}

// simply adds a string message to our output struct and increments the
// message counter

void add_mhea_message(char *msg) {
  if (strlen(msg) < MESSAGE_LEN) 
    STRCPY(mor->message[mor->num_message++], msg);
  return;
}

static void add_manual_j_heat_load(char *type, float pre, float post) {
  int cnt = mor->num_manj;

  mor->manj[cnt].index = cnt;
  STRCPY(mor->manj[cnt].heatcool, "heat"); // until we implement cooling manualJ
  STRCPY(mor->manj[cnt].type, type);
  STRCPY(mor->manj[cnt].name, "");          // unused #342
  mor->manj[cnt].area_vol   = 0.0;          // unused #342
  mor->manj[cnt].pre_load = pre;
  mor->manj[cnt].post_load = post;
  mor->num_manj++;
  return;
}
