/***************************************************************************
* MODULE:       report.c            CREATED:    January 12, 2001
*
* AUTHOR:       Mike Gettings       Oak Ridge National Laboratory
*               Mark Fishbaugher    Fishbaugher and Associates
*
* MDESC:        report routines called from neat.c
****************************************************************************/
#include <assert.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "wa_engine.h"

static void update_fill_ceiling_cavity_measure_name(int ecm_index);

static void update_measure_material_type(int ecm_index, char *mat_type);
static void update_measure_material_type_using_code(int ecm_index, char *code, char *mat_type);

//static void measure_material_cost_detail(int i);

static void report_measure_diag_print_header(char *title);
static void report_measure_diag_print_line(char *prefix, int mcnt);
static int get_string_length_without_padding(char *string);

/*******************
 *******************
 This routine prints the header material for the report */

static int wlinsn[AW_TYPE6 + 1] =   // material numbers for added wall insl types
   {N_MAT_NONE,
    N_MAT_NONE, 
    N_MAT_WALL_INSULATION_CELL,     // AW_CELLULOSE_BLOWN etc..
    N_MAT_WALL_INSULATION_UT2, 
    N_MAT_WALL_INSULATION_UT3, 
    N_MAT_WALL_INSULATION_UT4, 
    N_MAT_WALL_INSULATION_UT5, 
    N_MAT_WALL_INSULATION_UT6};     // possible wall insulation material numbers

void report_header(FILE *fp, int l_adjflg) {
  time_t ltime;

  if (!fp)
    return; // optionally no report header if null file pointer

  time(&ltime);
  if (cmds.regression_test) {
    fprintf(fp, "%s\n", "NEAT ");
  } else {
    fprintf(fp, "%s\n", "NEAT " WA_VERSION);
    fprintf(fp, "\nExecuted: %s", ctime(&ltime));
  } 
  // fprintf(fp, "House Description: %s\nBuilding Identifier: %-8s", house_desc, GLBprojectName);
  fprintf(fp, "\nAudit Type: %s", ndi->gnl.audit_type);
  fprintf(fp, "\nAudit ID: %ld", ndi->gnl.audit_id);
  fprintf(fp, "\nAudit Number: %ld", ndi->gnl.audit_number);
  fprintf(fp, "\nClimate: %s", cwd->city_name);

  // fprintf(fp, "Auditor: %-8s     Audit Date: %s\n", auditor, auddate);

  // MJF 11/13/97 made this upper case with a little more white space and
  // a line of dashes to highlight this section of the report -- just so it
  // is more obvious.

  if (l_adjflg > 0) {
    fprintf(fp, "\n\nRESULTS FOLLOWING APPLICATION OF BILLING DATA ADJUSTMENT FACTORS\n");
    fprintf(fp, "----------------------------------------------------------------\n");
  }
}

static void update_fill_ceiling_cavity_measure_name(int ecm_index){
  //int nc;
  int cms_measure_num = nir->ecm[ecm_index].cms_measure_num;

  if (cms_measure_num == N_CMS_FILL_CEILING_CAVITY) {
    if (nir->ecm[ecm_index].comp_group_type == MG_ATTIC_SPECIFIED_REQUIRED ||
      nir->ecm[ecm_index].comp_group_type == MG_ATTIC_COLLAR_BEAM_SPECIFIED_REQUIRED ||
      nir->ecm[ecm_index].comp_group_type == MG_ATTIC_ROOF_RAFTER_SPECIFIED_REQUIRED ||
      nir->ecm[ecm_index].comp_group_type == MG_ATTIC_OUTER_CEILING_JOIST_SPECIFIED_REQUIRED) {
      char name[MEASURENAME_LEN + 1];
      if (ndi->uas[nir->ecm[ecm_index].dwelling_component_index].various_r_values_in_measure == TRUE)   // #349
        sprintf(name, "User-Spec Ceiling Various R-values");
      else
        sprintf(name, "User-Spec Ceiling R-%2.0f", ndi->uas[nir->ecm[ecm_index].dwelling_component_index].added_R);
      STRCPY(nir->measure_name[cms_measure_num], name);
    } else {
      STRCPY(nir->measure_name[cms_measure_num],"Fill Ceiling Cavity");
    }
  }

  return;
}

void neat_results(FILE *fp) {

  if (fp) {
    fprintf(fp, "\n\n   Recommended");
    skipc(fp, 30);
    fprintf(fp, "    M e a s u r e       Cumulative\n");
    fprintf(fp, "     Measure             Component          Savings  Cost");
    fprintf(fp, "   SIR    Cost    SIR\n");
    skipc(fp, 43);
    fprintf(fp, "  ($/yr)   ($)          ($)\n\n");
  }

  float accost;
  float accsav;
  float accbc;
  accost = accsav = accbc = 0.;       // start package accumulators for cost, savings, and SIR

  report_measure_diag_print_header("FINAL as reported\n");

  // itemized costs that ARE to be included in the SIR but
  // have no energy savings.  Itemized costs WITH energy savings are
  // treated together with the main body of measures and are have an ecm_index tag for the ecm[] with savings
  // #350 belt and suspenders

  for (int j = 0; j < ndi->num_itc; j++) {
    if (ndi->itc[j].inc_sir == YES && ndi->itc[j].savings < .001 && ndi->itc[j].ecm_index == NOT_APPLICABLE) {

      int mcnt = nor->num_measure;
      nor->measure[mcnt].index = mcnt + 1;                      // number to order the list, base 1
      nor->measure[mcnt].measure_id = N_CMS_ITEMIZED_COST;
      nor->measure[mcnt].audit_section_id = N_ITEMIZED_COST;
      nor->measure[mcnt].component_id = ndi->itc[j].component_id;
      STRCPY(nor->measure[mcnt].measure, ndi->itc[j].measure);  // name of measure
      nor->measure[mcnt].savings = 0;                           // total annual heating and cooling dollar savings
      nor->measure[mcnt].required = TRUE;                       // itemized records are required by definition
      nor->measure[mcnt].cost = ndi->itc[j].cost;               // total initial costs
      nor->measure[mcnt].sir = 0;                               // computed savings to investment ratio
      nor->measure[mcnt].lifetime = 0;                          // measure lifetime in years use for sir
      nor->measure[mcnt].qtym = 0;                              // The quantity of the material itself in units (new 7/04)
      nor->measure[mcnt].qtyl = 0;                              // The quantity for labor cost calculation (new 7/04)
      nor->measure[mcnt].qtyi = 0;                              // Quantity of any associated Item Cost (new 7/04)
      nor->measure[mcnt].costum = 0;
      nor->measure[mcnt].costul = 0;
      nor->measure[mcnt].costi1 = 0;

      //Issue #21
      nor->measure[mcnt].costi2 = ndi->itc[j].cost;
      nor->measure[mcnt].typei2 = MT_NONE;

      if (strlen(ndi->itc[j].material))  // copy our material description
        STRCPY(nor->measure[mcnt].desci2, ndi->itc[j].material);
      else
        STRCPY(nor->measure[mcnt].desci2, "Misc Material");

      ndi->itc[j].measure_index = nor->measure[mcnt].index;   // associate this measure via the itc
      report_measure_diag_print_line("ITC1", mcnt);
      nor->num_measure++;   // post incremented, so it becomes the count of implemented measures

      accost += ndi->itc[j].cost;

      if (fp)
        fprintf(fp, "%-51.51s%6.0f     - %7.0f     -\n", ndi->itc[j].measure, ndi->itc[j].cost, accost);

      if (nir->adjflg) {
        int cnt = nor->num_asir;
        nor->asir[cnt].index = cnt + 1;
        nor->asir[cnt].measure_index = nor->measure[mcnt].index;
        nor->asir[cnt].group =  GROUP_INCIDENTAL_REPAIRS;
        STRCPY(nor->asir[cnt].measure, ndi->itc[j].measure);
        nor->asir[cnt].required = TRUE;
        nor->asir[cnt].cost = ndi->itc[j].cost;
        nor->asir[cnt].ccost = accost;
        nor->num_asir++;
      } else {
        int cnt = nor->num_sir;
        nor->sir[cnt].index = cnt + 1;
        nor->sir[cnt].measure_index = nor->measure[mcnt].index;
        nor->sir[cnt].group =  GROUP_INCIDENTAL_REPAIRS;
        STRCPY(nor->sir[cnt].measure, ndi->itc[j].measure);
        nor->sir[cnt].required = TRUE;
        nor->sir[cnt].cost = ndi->itc[j].cost;
        nor->sir[cnt].ccost = accost;
        nor->num_sir++;
      }

    }
  }

  if (nir->infiltration_treatment == INF_INCIDENTAL_COST) {

    int mcnt = nor->num_measure;
    nor->measure[mcnt].index = mcnt + 1;                             // number to order the list
    nor->measure[mcnt].measure_id = N_CMS_INFILTRATION_REDUCTION;   // needs to come from lib-The fixed measure number (new 7/04)
    nor->measure[mcnt].audit_section_id = N_DUCTS_AND_INFILTRATION;
    //nor->measure[mcnt].material_id = -2;                  // not used-The fixed material reference number
    STRCPY(nor->measure[mcnt].measure, nir->measure_name[N_CMS_INFILTRATION_REDUCTION]);        // name of measure
    nor->measure[mcnt].required = TRUE;
    nor->measure[mcnt].savings = 0;                       // total annual heating and cooling dollar savings
    nor->measure[mcnt].cost = ndi->inf.air_leak_red_cost; // total initial costs
    nor->measure[mcnt].sir = 0;                           // computed savings to investment ratio
    nor->measure[mcnt].lifetime = 0;                      // in years
    nor->measure[mcnt].qtym = 0;                          // The quantity of the material itself in units (new 7/04)
    nor->measure[mcnt].qtyl = 0;                          // The quantity for labor cost calculation (new 7/04)
    nor->measure[mcnt].qtyi = 0;                          // Quantity of any associated Item Cost (new 7/04)
    nor->measure[mcnt].costum = 0;
    nor->measure[mcnt].costul = 0;
    nor->measure[mcnt].costi1 = 0;

    nor->measure[mcnt].costi2 = ndi->inf.air_leak_red_cost;
    //nor->measure[mcnt].typei2 = MT_INFILTRATION_REDUCTION;
    nor->measure[mcnt].typei2 = MT_MISCELLANEOUS_SUPPLIES;
    STRCPY(nor->measure[mcnt].desci2, "Infiltration Reduction");

    report_measure_diag_print_line("INFI", mcnt);
    nor->num_measure++;

    accost += ndi->inf.air_leak_red_cost;

    if (fp)
      fprintf(fp, "%-51.51s%6.0f     - %7.0f     -\n", nir->measure_name[N_CMS_INFILTRATION_REDUCTION], ndi->inf.air_leak_red_cost, accost);

    // optional structure output

    if (nir->adjflg) {
      int cnt = nor->num_asir;
      nor->asir[cnt].index = cnt + 1;
      nor->asir[cnt].measure_index = nor->measure[mcnt].index;
      nor->asir[cnt].group = GROUP_INCIDENTAL_REPAIRS;
      STRCPY(nor->asir[cnt].measure, nir->measure_name[N_CMS_INFILTRATION_REDUCTION]);
      nor->asir[cnt].required = TRUE;
      nor->asir[cnt].cost = ndi->inf.air_leak_red_cost;
      nor->asir[cnt].ccost = accost;
      nor->num_asir++;
    } else {
      int cnt = nor->num_sir;
      nor->sir[cnt].index = cnt + 1;
      nor->sir[cnt].measure_index = nor->measure[mcnt].index;
      nor->sir[cnt].group = GROUP_INCIDENTAL_REPAIRS;
      STRCPY(nor->sir[cnt].measure, nir->measure_name[N_CMS_INFILTRATION_REDUCTION]);
      nor->asir[cnt].required = TRUE;
      nor->sir[cnt].cost = ndi->inf.air_leak_red_cost;
      nor->sir[cnt].ccost = accost;
      nor->num_sir++;
    }

  }

  // economics and measures for the ====== MAIN SET ======== of recommended measures by Benefit to Cost ratio (SIR)

  for (int j = 0; j < nir->nms; j++) {
    int i = nir->index_by_sir[j];
    enum MEASURE_PACKAGE_SORT_PRIORITY priority = nir->measure_priority[i];
    float sir = nir->ecm[i].sir;
    int required = nir->measure_required[i];
    int cms_measure_num = nir->ecm[i].cms_measure_num;

    if (cms_measure_num == N_CMS_INFILTRATION_REDUCTION && nir->infiltration_treatment == INF_NO_COST_COMPUTE_SAVINGS_ONLY)
      continue; // Infiltration without cost, otherwise treat infiltration reduction as normal measure

    if (include_measure_in_package(NEAT_FLG, cms_measure_num, priority, required, sir)) {

      update_fill_ceiling_cavity_measure_name(i);

      int mcnt = nor->num_measure;
      nor->measure[mcnt].index = mcnt + 1;                        // number to order the list, base 1, json PK 
      nor->measure[mcnt].measure_id = cms_measure_num;
      nor->measure[mcnt].audit_section_id = nir->ecm[i].audit_section_id;
      nor->measure[mcnt].component_id = nir->ecm[i].component_id;
      STRCPY(nor->measure[mcnt].measure, nir->measure_name[cms_measure_num]);       // name of measure
      if (nir->ecm[i].comp_group_num) 
        sprintf(nor->measure[mcnt].comp_group, "%s%02d", comp_group_name(nir->ecm[i].comp_group_type), nir->ecm[i].comp_group_num);
      STRCPY(nor->measure[mcnt].components, nir->ecm[i].components);  // list of components
      nor->measure[mcnt].required = required;
      nor->measure[mcnt].savings = nir->ecm[i].dlsav;           // total annual heating and cooling dollar savings
      nor->measure[mcnt].cost = nir->ecm[i].cost;               // total initial costs
      nor->measure[mcnt].sir = nir->ecm[i].sir;                // computed savings to investment ratio
      nor->measure[mcnt].lifetime = nir->ecm[i].lifetime;
      nor->measure[mcnt].qtym = nir->ecm[i].qtym; // The quantity of the material itself in units (new 7/04)
      nor->measure[mcnt].qtyl = nir->ecm[i].qtyl; // The quantity for labor cost calculation (new 7/04)
      nor->measure[mcnt].qtyi = nir->ecm[i].qtyi; // Quantity of any associated Item Cost (new 7/04)
      nor->measure[mcnt].costum = nir->ecm[i].costum;
      nor->measure[mcnt].costul = nir->ecm[i].costul;
      nor->measure[mcnt].costi1 = nir->ecm[i].costi1;

      nor->measure[mcnt].costi2 = nir->ecm[i].costi2;
      nor->measure[mcnt].typei2 = nir->ecm[i].typei2;
      STRCPY(nor->measure[mcnt].desci2, nir->ecm[i].desci2);
      nor->measure[mcnt].costi3 = nir->ecm[i].costi3;
      nor->measure[mcnt].typei3 = nir->ecm[i].typei3;
      STRCPY(nor->measure[mcnt].desci3, nir->ecm[i].desci3);

      nor->measure[mcnt].heating_mmbtu = nir->htengysav[i];
      nor->measure[mcnt].heating_sav = nir->htdlsav[i];
      nor->measure[mcnt].cooling_kwh = (float)(nir->clengysav[i] * KWH_PER_MMBTU);
      nor->measure[mcnt].cooling_sav = nir->cldlsav[i];
      nor->measure[mcnt].baseload_kwh = (float)(nir->blengysav[i] * KWH_PER_MMBTU);
      nor->measure[mcnt].baseload_sav = nir->bldlsav[i];
      nor->measure[mcnt].total_mmbtu = nir->ecm[i].ensav;

      nir->ecm[i].measure_index = nor->measure[mcnt].index;   // copy base 1 index back into ecm[] for later use and SIGNAL that ecm[i] was implemented
      if (nir->ecm[i].rmc_index != NOT_APPLICABLE)
        ndi->rmc[nir->ecm[i].rmc_index].measure_index = nor->measure[mcnt].index;    // and into materials
      for (int ecmm_index = 0; ecmm_index < nir->nmsm; ecmm_index++) {               // and tag the matching measure materials
        if (nir->ecmm[ecmm_index].ecm_index == i)
          nir->ecmm[ecmm_index].measure_index = nor->measure[mcnt].index;
      }

      report_measure_diag_print_line("MAIN", mcnt);
      nor->num_measure++;

      accost += nir->ecm[i].cost;
      accsav += nir->ecm[i].sir * nir->ecm[i].cost;

      ASSERT(accost, sprintf(msg, "Need non zero cost"));
      accbc = accsav / accost;

      // our report file output

      if (fp)
        fprintf(fp, "%-23s%-24.24s%7.0f%7.0f%7.1f%7.0f%7.1f\n", nir->measure_name[cms_measure_num], nir->ecm[i].components, nir->ecm[i].dlsav, nir->ecm[i].cost, nir->ecm[i].sir,
                accost, accbc);

      if (nir->adjflg) {
        int cnt = nor->num_asir;
        nor->asir[cnt].index = cnt + 1;
        nor->asir[cnt].measure_index = nor->measure[mcnt].index;
        nor->asir[cnt].group = measure_package_group(priority);
        STRCPY(nor->asir[cnt].measure, nir->measure_name[cms_measure_num]);
        if (nir->ecm[i].comp_group_num) 
          sprintf(nor->asir[cnt].comp_group, "%s%02d", comp_group_name(nir->ecm[i].comp_group_type), nir->ecm[i].comp_group_num);
        STRCPY(nor->asir[cnt].components, nir->ecm[i].components);
        nor->asir[cnt].required = required;
        nor->asir[cnt].savings = nir->ecm[i].dlsav;
        nor->asir[cnt].cost = nir->ecm[i].cost;
        nor->asir[cnt].sir = nir->ecm[i].sir;
        nor->asir[cnt].ccost = accost;
        nor->asir[cnt].csir = accbc;
        nor->num_asir++;
      } else {
        int cnt = nor->num_sir;
        nor->sir[cnt].index = cnt + 1;
        nor->sir[cnt].measure_index = nor->measure[mcnt].index;
        nor->sir[cnt].group = measure_package_group(priority);
        STRCPY(nor->sir[cnt].measure, nir->measure_name[cms_measure_num]);
        if (nir->ecm[i].comp_group_num) 
          sprintf(nor->sir[cnt].comp_group, "%s%02d", comp_group_name(nir->ecm[i].comp_group_type), nir->ecm[i].comp_group_num);
        STRCPY(nor->sir[cnt].components, nir->ecm[i].components);
        nor->sir[cnt].required = required;
        nor->sir[cnt].savings = nir->ecm[i].dlsav;
        nor->sir[cnt].cost = nir->ecm[i].cost;
        nor->sir[cnt].sir = nir->ecm[i].sir;
        nor->sir[cnt].ccost = accost;
        nor->sir[cnt].csir = accbc;
        nor->num_sir++;
      }

      int nc;
      if (fp && (nc = get_string_length_without_padding(nir->ecm[i].components)) > 20) {
        int nlc = nc / 20;
        for (int l = 2; l <= nlc; l++) {
          skipc(fp, 23);
          for (int n = 20 * (l - 1); n <= 20 * l - 1; n++)
            fprintf(fp, "%c", nir->ecm[i].components[n]);
          fprintf(fp, "\n");
        }
        skipc(fp, 23);
        for (int n = 20 * nlc; n <= nc; n++)
          fprintf(fp, "%c", nir->ecm[i].components[n]);
        fprintf(fp, "\n");
      }
    } 
  }  // next ecm measure to consider

  // Mandatory measure not included in SIR

  for (int j = 0; j < nir->nms; j++) {
    int i = nir->index_by_sir[j];
    int cms_measure_num = nir->ecm[i].cms_measure_num;

    // btctemp = ndi->key.minimum_acceptable_sir;   // dead assign removal MJF 12/2018

    if (nir->measure_priority[i] < MPS_NPV) {

      int mcnt = nor->num_measure;
      nor->measure[mcnt].index = mcnt + 1; // number to order the list
      nor->measure[mcnt].measure_id = cms_measure_num;
      nor->measure[mcnt].audit_section_id = nir->ecm[i].audit_section_id;
      //nor->measure[mcnt].material_id = nir->ecm[i].material_id; // not used-The fixed material reference number
      STRCPY(nor->measure[mcnt].measure, nir->measure_name[cms_measure_num]);       // name of measure
      if (nir->ecm[i].comp_group_num) 
        sprintf(nor->measure[mcnt].comp_group, "%s%02d", comp_group_name(nir->ecm[i].comp_group_type), nir->ecm[i].comp_group_num);
      STRCPY(nor->measure[mcnt].components, nir->ecm[i].components);  // name of measure
      nor->measure[mcnt].required = nir->measure_required[i];
      nor->measure[mcnt].savings = nir->ecm[i].dlsav;           // total annual heating and cooling dollar savings
      nor->measure[mcnt].cost = nir->ecm[i].cost;               // total initial costs
      nor->measure[mcnt].sir = nir->ecm[i].sir;                // computed savings to investment ratio
      nor->measure[mcnt].lifetime = nir->ecm[i].lifetime;
      nor->measure[mcnt].qtym = nir->ecm[i].qtym; // The quantity of the material itself in units (new 7/04)
      nor->measure[mcnt].qtyl = nir->ecm[i].qtyl; // The quantity for labor cost calculation (new 7/04)
      nor->measure[mcnt].qtyi = nir->ecm[i].qtyi; // Quantity of any associated Item Cost (new 7/04)
      nor->measure[mcnt].costum = nir->ecm[i].costum;
      nor->measure[mcnt].costul = nir->ecm[i].costul;
      nor->measure[mcnt].costi1 = nir->ecm[i].costi1;

      nor->measure[mcnt].costi2 = nir->ecm[i].costi2;
      nor->measure[mcnt].typei2 = nir->ecm[i].typei2;
      STRCPY(nor->measure[mcnt].desci2, nir->ecm[i].desci2);
      nor->measure[mcnt].costi3 = nir->ecm[i].costi3;
      nor->measure[mcnt].typei3 = nir->ecm[i].typei3;
      STRCPY(nor->measure[mcnt].desci3, nir->ecm[i].desci3);

      nor->measure[mcnt].heating_mmbtu = nir->htengysav[i];
      nor->measure[mcnt].heating_sav = nir->htdlsav[i];
      nor->measure[mcnt].cooling_kwh = (float)(nir->clengysav[i] * KWH_PER_MMBTU);
      nor->measure[mcnt].cooling_sav = nir->cldlsav[i];
      nor->measure[mcnt].baseload_kwh = (float)(nir->blengysav[i] * KWH_PER_MMBTU);
      nor->measure[mcnt].baseload_sav = nir->bldlsav[i];
      nor->measure[mcnt].total_mmbtu = nir->ecm[i].ensav;

      nir->ecm[i].measure_index = nor->measure[mcnt].index;       // copy base 1 index back into ecm[] for later use
      if (nir->ecm[i].rmc_index != NOT_APPLICABLE) {
        if (ndi->rmc[nir->ecm[i].rmc_index].measure_index == 0) {   // not previously assigned
          ndi->rmc[nir->ecm[i].rmc_index].measure_index = nor->measure[mcnt].index;    // and into materials #239
        }
      }
      for (int ecmm_index = 0; ecmm_index < nir->nmsm; ecmm_index++) {    // and tag the matching measure materials
        if (nir->ecmm[ecmm_index].ecm_index == i)
          nir->ecmm[ecmm_index].measure_index = nor->measure[mcnt].index;
      }

      report_measure_diag_print_line("MAND", mcnt);
      nor->num_measure++;

      accost += nir->ecm[i].cost;
      //accsav += nir->ecm[i].sir * nir->ecm[i].cost;
      //ASSERT(accost, sprintf(msg, "Need non zero cost"));
      // GKA        accbc = accsav/accost;

      update_fill_ceiling_cavity_measure_name(i);

      if (fp)
        fprintf(fp, "%-23s%-24.24s%7.0f%7.0f%7.1f%7.0f%7.1f\n", nir->measure_name[cms_measure_num], nir->ecm[i].components, nir->ecm[i].dlsav, nir->ecm[i].cost, nir->ecm[i].sir,
                accost, accbc);

      if (nir->adjflg) {
        int cnt = nor->num_asir;
        nor->asir[cnt].index = cnt + 1;
        nor->asir[cnt].measure_index = nor->measure[mcnt].index;
        nor->asir[cnt].group = GROUP_HEALTH_AND_SAFETY_MEASURES;
        STRCPY(nor->asir[cnt].measure, nir->measure_name[cms_measure_num]);
        if (nir->ecm[i].comp_group_num) 
          sprintf(nor->asir[cnt].comp_group, "%s%02d", comp_group_name(nir->ecm[i].comp_group_type), nir->ecm[i].comp_group_num);
        STRCPY(nor->asir[cnt].components, nir->ecm[i].components);
        nor->asir[cnt].required = nir->measure_required[i];
        nor->asir[cnt].savings = nir->ecm[i].dlsav;
        nor->asir[cnt].cost = nir->ecm[i].cost;
        nor->asir[cnt].sir = nir->ecm[i].sir;
        nor->asir[cnt].ccost = accost;
        nor->asir[cnt].csir = 0.0; // GKA We do not want accbc to be in the cumulative SIR
        nor->num_asir++;
      } else {
        int cnt = nor->num_sir;
        nor->sir[cnt].index = cnt + 1;
        nor->sir[cnt].measure_index = nor->measure[mcnt].index;
        nor->sir[cnt].group = GROUP_HEALTH_AND_SAFETY_MEASURES;
        STRCPY(nor->sir[cnt].measure, nir->measure_name[cms_measure_num]);
        if (nir->ecm[i].comp_group_num) 
          sprintf(nor->sir[cnt].comp_group, "%s%02d", comp_group_name(nir->ecm[i].comp_group_type), nir->ecm[i].comp_group_num);
        STRCPY(nor->sir[cnt].components, nir->ecm[i].components);
        nor->sir[cnt].required = nir->measure_required[i];
        nor->sir[cnt].savings = nir->ecm[i].dlsav;
        nor->sir[cnt].cost = nir->ecm[i].cost;
        nor->sir[cnt].sir = nir->ecm[i].sir;
        nor->sir[cnt].ccost = accost;
        nor->sir[cnt].csir = 0.0; // GKA We do not want accbc to be in the cumulative SIR
        nor->num_sir++;
      }

      int nc;
      if (fp && (nc = get_string_length_without_padding(nir->ecm[i].components)) > 20) {
        int nlc = nc / 20;
        for (int l = 2; l <= nlc; l++) {
          skipc(fp, 23);
          for (int n = 20 * (l - 1); n <= 20 * l - 1; n++)
            fprintf(fp, "%c", nir->ecm[i].components[n]);
          fprintf(fp, "\n");
        }
        skipc(fp, 23);
        for (int n = 20 * nlc; n <= nc; n++)
          fprintf(fp, "%c", nir->ecm[i].components[n]);
        fprintf(fp, "\n");
      }
    }
  }

  // itemized cost items NOT included in the
  // cummulative SIR calculation but accumulates project cost
  // #350 and not already included as a savings measure

  for (int j = 0; j < ndi->num_itc; j++) {
    if (ndi->itc[j].inc_sir != YES && ndi->itc[j].ecm_index == NOT_APPLICABLE) {

      // our composite results struture

      int mcnt = nor->num_measure;
      nor->measure[mcnt].index = mcnt + 1; // number to order the list
      nor->measure[mcnt].measure_id = N_CMS_ITEMIZED_COST;
      nor->measure[mcnt].audit_section_id = N_ITEMIZED_COST;
      nor->measure[mcnt].component_id = ndi->itc[j].component_id;
      //nor->measure[mcnt].material_id = -2;                     // signal to front-end not to include in cost
      STRCPY(nor->measure[mcnt].measure, ndi->itc[j].measure); // name of measure
      nor->measure[mcnt].required = TRUE;
      nor->measure[mcnt].savings = 0;                          // total annual heating and cooling dollar savings
      nor->measure[mcnt].cost = ndi->itc[j].cost;              // total initial costs
      nor->measure[mcnt].sir = 0;                              // computed savings to investment ratio
      nor->measure[mcnt].lifetime = 0;
      nor->measure[mcnt].qtym = 1; // The quantity of the material itself in units (new 7/04)
      nor->measure[mcnt].qtyl = 1; // The quantity for labor cost calculation (new 7/04)
      nor->measure[mcnt].qtyi = 1; // Quantity of any associated Item Cost (new 7/04)
      nor->measure[mcnt].costum = ndi->itc[j].cost;
      nor->measure[mcnt].costul = 0;
      nor->measure[mcnt].costi1 = 0;

      // Issue #21
      //if (ndi->itc[j].measure_id == 0) { // lumped cost for no referenced measure
        nor->measure[mcnt].costi2 = ndi->itc[j].cost;
        //nor->measure[mcnt].typei2 = MT_ITEMIZED_MEASURE_COST;
        nor->measure[mcnt].typei2 = MT_NONE;
        if (strlen(ndi->itc[j].material))  // copy our material description
          STRCPY(nor->measure[mcnt].desci2, ndi->itc[j].material);
        else
          STRCPY(nor->measure[mcnt].desci2, "Itemized Material");
      //}
      ndi->itc[j].measure_index = nor->measure[mcnt].index;
      report_measure_diag_print_line("ITC2", mcnt);
      nor->num_measure++;

      accost += ndi->itc[j].cost;

      // report output

      if (fp)
        fprintf(fp, "%-51.51s%6.0f     - %7.0f     -\n", ndi->itc[j].measure, ndi->itc[j].cost, accost);

      if (nir->adjflg) {
        int cnt = nor->num_asir;
        nor->asir[cnt].index = cnt + 1;
        nor->asir[cnt].measure_index = nor->measure[mcnt].index;
        nor->asir[cnt].group = GROUP_HEALTH_AND_SAFETY_MEASURES;
        STRCPY(nor->asir[cnt].measure, ndi->itc[j].measure);
        nor->asir[cnt].required = TRUE;
        nor->asir[cnt].cost = ndi->itc[j].cost;
        nor->asir[cnt].ccost = accost;
        nor->num_asir++;
      } else {
        int cnt = nor->num_sir;
        nor->sir[cnt].index = cnt + 1;
        nor->sir[cnt].measure_index = nor->measure[mcnt].index;
        nor->sir[cnt].group = GROUP_HEALTH_AND_SAFETY_MEASURES;
        STRCPY(nor->sir[cnt].measure, ndi->itc[j].measure);
        nor->sir[cnt].required = TRUE;
        nor->sir[cnt].cost = ndi->itc[j].cost;
        nor->sir[cnt].ccost = accost;
        nor->num_sir++;
      }

    }
  }

  // if (ndi->num_clg == 0 && ndi->gnl.impute_cooling == YES) {

  //   if (fp) {
  //     fprintf(fp, "\n\n");
  //     fprintf(fp, "Savings for this house were determined assuming it was air-conditioned,\n");
  //     fprintf(fp, "though no cooling equipment existed. Implementing some envelope measures\n");
  //     fprintf(fp, "under these circumstances may increase the discomfort of the occupants,\n");
  //     fprintf(fp, "particularly during the spring and fall.  Caution should be used when\n");
  //     fprintf(fp, "installing such measures, especially in homes with high internal loads.\n");
  //     fprintf(fp, "The use of natural ventilation and/or fans may help increase comfort.");
  //     if (wallinsflg == 1)
  //       fprintf(fp, "\n\n");
  //     fprintf(fp, "The effect of wall insulation was degraded to help prevent occupant\n");
  //     fprintf(fp, "discomfort with only minimal savings.\n\n");
  //   }

  //   if (nir->adjflg == 0) // copy only once
  //   {
  //     cnt = nor->num_message;
  //     nor->message[cnt].index = cnt + 1;
  //     STRCPY(nor->message[cnt].msg, "WARNING: Read warning messages about cooling savings calculations.");
  //     nor->num_message++;
  //   }
  // }

  // ====================== Annual Energy Savings Section====================

  if (fp) {
    fprintf(fp, "\n   Recommended");
    skipc(fp, 33);
    fprintf(fp, "  A n n u a l  S a v i n g s \n");
    fprintf(fp, "     Measure             Component");
    skipc(fp, 14);
    fprintf(fp, "Heating       Cooling    BaseLoad   Total\n");
    skipc(fp, 45);
    fprintf(fp, "(MMBtu)   ($)   (kWh)  ($)   (kWh)  ($)   (MMBtu)\n\n");
  }

  for (int j = 0; j < nir->nms; j++) {
    int i = nir->index_by_sir[j];
    enum MEASURE_PACKAGE_SORT_PRIORITY priority = nir->measure_priority[i];
    float sir = nir->ecm[i].sir;
    int required = nir->measure_required[i];
    int cms_measure_num = nir->ecm[i].cms_measure_num;

    if (include_measure_in_package(NEAT_FLG, cms_measure_num, priority, required, sir)) {

      update_fill_ceiling_cavity_measure_name(i);

      if (fp)
        fprintf(fp, "%-23s%-24.24s%8.1f%6.0f%7.0f%6.0f%7.0f%6.0f%7.1f\n", nir->measure_name[cms_measure_num], nir->ecm[i].components, nir->htengysav[i], nir->htdlsav[i],
                nir->clengysav[i] * KWH_PER_MMBTU, nir->cldlsav[i], nir->blengysav[i] * KWH_PER_MMBTU, nir->bldlsav[i], nir->ecm[i].ensav);

      if (nir->adjflg) {
        int cnt = nor->num_an_asav;
        nor->an_asav[cnt].index = cnt + 1;
        nor->an_asav[cnt].measure_index = nir->ecm[i].measure_index;
        STRCPY(nor->an_asav[cnt].measure, nir->measure_name[cms_measure_num]);
        if (nir->ecm[i].comp_group_num) 
          sprintf(nor->an_asav[cnt].comp_group, "%s%02d", comp_group_name(nir->ecm[i].comp_group_type), nir->ecm[i].comp_group_num);
        STRCPY(nor->an_asav[cnt].components, nir->ecm[i].components);
        nor->an_asav[cnt].required = required;
        nor->an_asav[cnt].heating_mmbtu = nir->htengysav[i];
        nor->an_asav[cnt].heating_sav = nir->htdlsav[i];
        nor->an_asav[cnt].cooling_kwh = nir->clengysav[i] * KWH_PER_MMBTU;
        nor->an_asav[cnt].cooling_sav = nir->cldlsav[i];
        nor->an_asav[cnt].baseload_kwh = nir->blengysav[i] * KWH_PER_MMBTU;
        nor->an_asav[cnt].baseload_sav = nir->bldlsav[i];
        nor->an_asav[cnt].total_mmbtu = nir->ecm[i].ensav;
        nor->num_an_asav++;
      } else {
        int cnt = nor->num_an_sav;
        nor->an_sav[cnt].index = cnt + 1;
        nor->an_sav[cnt].measure_index = nir->ecm[i].measure_index;
        STRCPY(nor->an_sav[cnt].measure, nir->measure_name[cms_measure_num]);
        if (nir->ecm[i].comp_group_num) 
          sprintf(nor->an_sav[cnt].comp_group, "%s%02d", comp_group_name(nir->ecm[i].comp_group_type), nir->ecm[i].comp_group_num);
        STRCPY(nor->an_sav[cnt].components, nir->ecm[i].components);
        nor->an_sav[cnt].required = required;
        nor->an_sav[cnt].heating_mmbtu = nir->htengysav[i];
        nor->an_sav[cnt].heating_sav = nir->htdlsav[i];
        nor->an_sav[cnt].cooling_kwh = nir->clengysav[i] * KWH_PER_MMBTU;
        nor->an_sav[cnt].cooling_sav = nir->cldlsav[i];
        nor->an_sav[cnt].baseload_kwh = nir->blengysav[i] * KWH_PER_MMBTU;
        nor->an_sav[cnt].baseload_sav = nir->bldlsav[i];
        nor->an_sav[cnt].total_mmbtu = nir->ecm[i].ensav;
        nor->num_an_sav++;
      }

      // TODO, clean up this mystery

      int nc;
      if (fp && (nc = get_string_length_without_padding(nir->ecm[i].components)) > 20) {
        int nlc = nc / 20;
        for (int l = 2; l <= nlc; l++) {
          skipc(fp, 23);
          for (int n = 20 * (l - 1); n <= 20 * l - 1; n++)
            fprintf(fp, "%c", nir->ecm[i].components[n]);
          fprintf(fp, "\n");
        }
        skipc(fp, 23);
        for (int n = 20 * nlc; n <= nc - 1; n++)
          fprintf(fp, "%c", nir->ecm[i].components[n]);
        fprintf(fp, "\n");
      }

    }
  }

  // =============== Append Annual Energy and Cost Savings report section ===================
  // Print required evelope and equipment measures not to be inclued in whole house SIR but may have energy savings to contribute

  for (int j = 0; j < nir->nms; j++) {
    int i = nir->index_by_sir[j];      // item index from measure ordering

    //if (nir->measure_priority[i] < MPS_NPV) {
    if (nir->measure_priority[i] == MPS_ENVELOPE_REQUIRED_NO_SIR ||
      nir->measure_priority[i] == MPS_EQUIP_REQUIRED_NO_SIR) {

      int cms_measure_num = nir->ecm[i].cms_measure_num;
      if (fp)
        fprintf(fp, "%-23s%-24.24s%8.1f%6.0f%7.0f%6.0f%7.0f%6.0f%7.1f\n", nir->measure_name[cms_measure_num], nir->ecm[i].components, nir->htengysav[i], nir->htdlsav[i],
                nir->clengysav[i] * KWH_PER_MMBTU, nir->cldlsav[i], nir->blengysav[i] * KWH_PER_MMBTU, nir->bldlsav[i], nir->ecm[i].ensav);

      // echo the same material to the nor data structure

      if (nir->adjflg) {
        int cnt = nor->num_an_asav;
        nor->an_asav[cnt].index = cnt + 1;
        nor->an_asav[cnt].measure_index = nir->ecm[i].measure_index;
        STRCPY(nor->an_asav[cnt].measure, nir->measure_name[cms_measure_num]);
        if (nir->ecm[i].comp_group_num) 
          sprintf(nor->an_asav[cnt].comp_group, "%s%02d", comp_group_name(nir->ecm[i].comp_group_type), nir->ecm[i].comp_group_num);
        STRCPY(nor->an_asav[cnt].components, nir->ecm[i].components);
        nor->an_asav[cnt].required = nir->measure_required[i];
        nor->an_asav[cnt].heating_mmbtu = nir->htengysav[i];
        nor->an_asav[cnt].heating_sav = nir->htdlsav[i];
        nor->an_asav[cnt].cooling_kwh = nir->clengysav[i] * KWH_PER_MMBTU;
        nor->an_asav[cnt].cooling_sav = nir->cldlsav[i];
        nor->an_asav[cnt].baseload_kwh = nir->blengysav[i] * KWH_PER_MMBTU;
        nor->an_asav[cnt].baseload_sav = nir->bldlsav[i];
        nor->an_asav[cnt].total_mmbtu = nir->ecm[i].ensav;
        nor->num_an_asav++;
      } else {
        int cnt = nor->num_an_sav;
        nor->an_sav[cnt].index = cnt + 1;
        nor->an_sav[cnt].measure_index = nir->ecm[i].measure_index;
        STRCPY(nor->an_sav[cnt].measure, nir->measure_name[cms_measure_num]);
        if (nir->ecm[i].comp_group_num) 
          sprintf(nor->an_sav[cnt].comp_group, "%s%02d", comp_group_name(nir->ecm[i].comp_group_type), nir->ecm[i].comp_group_num);
        STRCPY(nor->an_sav[cnt].components, nir->ecm[i].components);
        nor->an_sav[cnt].required = nir->measure_required[i];
        nor->an_sav[cnt].heating_mmbtu = nir->htengysav[i];
        nor->an_sav[cnt].heating_sav = nir->htdlsav[i];
        nor->an_sav[cnt].cooling_kwh = nir->clengysav[i] * KWH_PER_MMBTU;
        nor->an_sav[cnt].cooling_sav = nir->cldlsav[i];
        nor->an_sav[cnt].baseload_kwh = nir->blengysav[i] * KWH_PER_MMBTU;
        nor->an_sav[cnt].baseload_sav = nir->bldlsav[i];
        nor->an_sav[cnt].total_mmbtu = nir->ecm[i].ensav;
        nor->num_an_sav++;
      }

    }
  }

}

static void report_measure_diag_print_header(char *title) {
  if (cmds.debug_level & D_NORMAL) {
    fprintf(stderr, "\n\n%s", title);
    fprintf(stderr, "\n      i  m  a  r                                                              dlsav     cost  life   sir");
  }
}

static void report_measure_diag_print_line(char *prefix, int mcnt) {
  if (cmds.debug_level & D_NORMAL) {
    fprintf(stderr, "\n%4s %2d %2d %2d %2d %-24.24s %-20.20s %-10.10s %9.3f%9.3f%6d%6.2f", 
      prefix,
      nor->measure[mcnt].index,
      nor->measure[mcnt].measure_id,
      nor->measure[mcnt].audit_section_id,
      nor->measure[mcnt].required,
      nor->measure[mcnt].measure,
      nor->measure[mcnt].comp_group,
      nor->measure[mcnt].components,
      nor->measure[mcnt].savings,
      nor->measure[mcnt].cost,
      nor->measure[mcnt].lifetime,
      nor->measure[mcnt].sir);
  }
}

/**************************************
 This routine prints the material list for the report */

void report_materials(FILE *fp) {
  int i, j, nc;
  int cms_measure_num, furnflg = FALSE, imat, istud;
  float wall_type_area[SS_TWOXEIGHT + 1][AW_TYPE6 + 1];             // aggregates area of walls having the same stud depth and added ins type
  char *blanks = "                        ";
  char temp_type[TYPE_LEN + 1];
  int rmc_index;

  // zero the areas
  for (istud = SS_TWOXTWO; istud <= SS_TWOXEIGHT; istud++) {
    for (imat = AW_CELLULOSE_BLOWN; imat <= AW_TYPE6; imat++)
      wall_type_area[istud][imat] = 0.0f;
  }

  // useful for diagnostics output
  if (cmds.debug_level & D_NORMAL) {
    fprintf(stderr, "\n\nECM Array on entry to report_materials\n");
    fprintf(stderr, "\n i Index   Mdx rmci\n");
    for (int i = 0; i < nir->nms; i++)
      fprintf(stderr, "%2d%5d%7d%4d %s\n", i,
        nir->ecm[i].index,
        nir->ecm[i].measure_index,
        nir->ecm[i].rmc_index,
        nir->measure_name[nir->ecm[i].cms_measure_num]);

    fprintf(stderr, "\nMaterials Array on entry to report_materials\n");
    fprintf(stderr, "\n  i Mdx     qty\n");
    for (int i = 0; i < NMAT; i++) {
      if (ndi->rmc[i].measure_index || ndi->rmc[i].quant)
        fprintf(stderr, "%3d%4d %7.3f %s\n", i,
          ndi->rmc[i].measure_index,
          ndi->rmc[i].quant,
          ndi->rmc[i].material);
    }
  }

  if (fp) {
    fprintf(fp, "\n                        Material List\n\n");
    fprintf(fp, "Material Name        Type                               Quantity\n\n");
  }

  for (j = 0; j < nir->nms; j++) {
    cms_measure_num = nir->ecm[j].cms_measure_num;

    if (nir->ecm[j].measure_index == 0) continue;       // measure not implemented

    /* Wall Insulation - find walls of similar depth and retrofit ins type */

    // if ((cms_measure_num == N_CMS_WALL_INSULATION && nir->measure_priority[j] != MPS_NPV &&  nir->ecm[j].sir >= ndi->key.minimum_acceptable_sir) || 
    //   (cms_measure_num == N_CMS_WALL_INSULATION && nir->measure_priority[j] == MPS_INFILTRATION_REDUCTION)) {

    // if (cms_measure_num == N_CMS_WALL_INSULATION && 
    //   nir->measure_priority[j] != MPS_NPV && 
    //   nir->ecm[j].sir >= ndi->key.minimum_acceptable_sir) {

    if (cms_measure_num == N_CMS_WALL_INSULATION ) {

      for (nc = 0; nc < ndi->num_wal; nc++) {
        if (ndi->wal[nc].measure_number != nir->ecm[j].comp_group_num)
          continue;

        imat = ndi->wal[nc].added_insulation;
        istud = ndi->wal[nc].stud_size;
        wall_type_area[istud][imat] += ndi->wal[nc].area;

        // this duplicates the temp type string construction code below
        // just for the call to ts_costdetail for each component
        // so the type strings in the ecmm array can be updated correctly
        // for each compent detail.

        rmc_index = wlinsn[imat];
        if (strcmp(ndi->ins_wall[imat].units, "R/in") == 0) {
          STRCPY(temp_type, ndi->rmc[rmc_index].retrofit_type);
          STRCAT(temp_type, " - ");
          STRCAT(temp_type, wall_stud_size_name(istud));
          STRCAT(temp_type, " Filled");
        } else if (strcmp(ndi->ins_wall[imat].units, "R") == 0) {
          STRCPY(temp_type, ndi->rmc[rmc_index].retrofit_type);
        } else {
          ASSERT(FALSE, sprintf(msg, "Did not find R or R/in:%s", ndi->ins_wall[imat].units));
        }
        update_measure_material_type_using_code(j, ndi->wal[nc].code, temp_type); // substitute type string
      }
    }
  }

  /* Create the material records for those materials and wall depths present */

  for (istud = SS_TWOXTWO; istud <= SS_TWOXEIGHT; istud++) {
    for (imat = AW_CELLULOSE_BLOWN; imat <= AW_TYPE6; imat++) {

      if (wall_type_area[istud][imat] < 0.01)     // previously accumulated for implemented measure
        continue;

      rmc_index = wlinsn[imat];
      if (strcmp(ndi->ins_wall[imat].units, "R/in") == 0) {
        STRCPY(temp_type, ndi->rmc[rmc_index].retrofit_type);
        STRCAT(temp_type, " - ");
        STRCAT(temp_type, wall_stud_size_name(istud));
        STRCAT(temp_type, " Filled");
        if (fp)
          fprintf(fp, "%-21.21s%-30.30s%10.0f  %s\n", ndi->rmc[rmc_index].material, temp_type, wall_type_area[istud][imat],
                  ndi->rmc[rmc_index].units);
      } else if (strcmp(ndi->ins_wall[imat].units, "R") == 0) {
        STRCPY(temp_type, ndi->rmc[rmc_index].retrofit_type);
        if (fp)
          fprintf(fp, "%-21.21s%-30.30s%10.0f  %s\n", ndi->rmc[rmc_index].material, ndi->rmc[rmc_index].retrofit_type,
                  wall_type_area[istud][imat], ndi->rmc[rmc_index].units);
      } else {
          ASSERT(FALSE, sprintf(msg, "Did not find R or R/in:%s", ndi->ins_wall[imat].units));
      }

      // output to data structure

      if (nir->adjflg) {
        int cnt = nor->num_amaterial;
        nor->amaterial[cnt].index = cnt + 1;
        nor->amaterial[cnt].measure_index = ndi->rmc[rmc_index].measure_index;
        //nor->amaterial[cnt].measure_index = nir->ecm[j].measure_index;
        nor->amaterial[cnt].material_id = rmc_index;
        STRCPY(nor->amaterial[cnt].material, ndi->rmc[rmc_index].material);
        STRCPY(nor->amaterial[cnt].type, temp_type);
        nor->amaterial[cnt].quantity = wall_type_area[istud][imat];
        STRCPY(nor->amaterial[cnt].units, ndi->rmc[rmc_index].units);
        nor->num_amaterial++;
      } else {
        int cnt = nor->num_material;
        nor->material[cnt].index = cnt + 1;
        nor->material[cnt].measure_index = ndi->rmc[rmc_index].measure_index;
        //nor->material[cnt].measure_index = nir->ecm[j].measure_index;
        nor->material[cnt].material_id = rmc_index;        
        STRCPY(nor->material[cnt].material, ndi->rmc[rmc_index].material);
        STRCPY(nor->material[cnt].type, temp_type);
        nor->material[cnt].quantity = wall_type_area[istud][imat];
        STRCPY(nor->material[cnt].units, ndi->rmc[rmc_index].units);
        nor->num_material++;
      }

    }
  }

  // Look through the COMPLETE set of retrofit material cost (rmc) records looking for non zero quantity
  // then report those items in the materials list

  for (rmc_index = 0; rmc_index < NMAT; rmc_index++) {
    //i = nir->material_translate[k]; // mt is our material xlation numbers YUCK!
    if (ndi->rmc[rmc_index].quant > 0 && ndi->rmc[rmc_index].measure_index != 0) {    // some quantity and participating in a measure
      char material_name[MATERIAL_LEN];  // so we avoid strlwr() effects on the results array
      //strcpy(material_name,ndi->rmc[i].material);
      STRCPY(material_name,ndi->rmc[rmc_index].material);
      strlwr(material_name);
      if (strncmp(material_name, "wall ins", 8) == 0)   // we have already done walls above
        continue;
      if ((strncmp(material_name, "low e", 5)) == 0 ||
          (strncmp(material_name, "window repl", 11)) == 0 ||
          (strncmp(material_name, "storm", 5)) == 0)
        STRCPY(ndi->rmc[rmc_index].units, "Each");

      if (fp)
        fprintf(fp, "%-21.21s%-30.30s%10.0f  %s\n", ndi->rmc[rmc_index].material, ndi->rmc[rmc_index].retrofit_type,
                ndi->rmc[rmc_index].quant, ndi->rmc[rmc_index].units);

      // output to data structure

      if (nir->adjflg) {
        int cnt = nor->num_amaterial;
        nor->amaterial[cnt].index = cnt + 1;
        nor->amaterial[cnt].measure_index = ndi->rmc[rmc_index].measure_index;
        nor->amaterial[cnt].material_id = rmc_index;
        STRCPY(nor->amaterial[cnt].material, ndi->rmc[rmc_index].material);
        STRCPY(nor->amaterial[cnt].type, ndi->rmc[rmc_index].retrofit_type);
        nor->amaterial[cnt].quantity = ndi->rmc[rmc_index].quant;
        STRCPY(nor->amaterial[cnt].units, ndi->rmc[rmc_index].units);
        nor->num_amaterial++;
      } else {
        int cnt = nor->num_material;
        nor->material[cnt].index = cnt + 1;
        nor->material[cnt].measure_index = ndi->rmc[rmc_index].measure_index;
        nor->material[cnt].material_id = rmc_index;
        STRCPY(nor->material[cnt].material, ndi->rmc[rmc_index].material);
        STRCPY(nor->material[cnt].type, ndi->rmc[rmc_index].retrofit_type);
        nor->material[cnt].quantity = ndi->rmc[rmc_index].quant;
        STRCPY(nor->material[cnt].units, ndi->rmc[rmc_index].units);
        nor->num_material++;
      }
    }
  }

  // some special processing for specific materials

  for (int ecm_index = 0; ecm_index < nir->nms; ecm_index++) {
    cms_measure_num = nir->ecm[ecm_index].cms_measure_num;

    if (nir->ecm[ecm_index].measure_index == 0) continue;       // measure not implemented

    /* Fill Attic Cavity */

    // if (cms_measure_num == N_CMS_FILL_CEILING_CAVITY && 
    //   ((nir->measure_priority[ecm_index] == MPS_SIR &&  nir->ecm[ecm_index].sir >= ndi->key.minimum_acceptable_sir) || 
    //   nir->measure_priority[ecm_index] == MPS_ENVELOPE_REQUIRED)) {

    if (cms_measure_num == N_CMS_FILL_CEILING_CAVITY) {

      char temp_material_name[MATERIAL_LEN + 1];
      char temp_type[MATERIAL_LEN + 1];

      /*      rmc_index=nir->ecm[ecm_index].intgr; */
      for (nc = 0; nc < ndi->num_uas; nc++) {

        // does not participate in component grouping type
        if (ndi->uas[nc].comp_group_type != nir->ecm[ecm_index].comp_group_type)
          continue;

        // does not participate in component grouping number
        if (ndi->uas[nc].comp_group_num != nir->ecm[ecm_index].comp_group_num)
          continue;

        //rmc_index = attic_add_insulation_material(ndi->uas[nc].added_insulation, cms_measure_num);
        rmc_index = nir->ecm[ecm_index].rmc_index;

        ASSERT(strlen(ndi->rmc[rmc_index].retrofit_type) + 7 < TYPE_LEN, sprintf(msg, "String retrofit_type too long: %ld", strlen(ndi->rmc[rmc_index].retrofit_type)));

        fill_ceiling_cavity_names(temp_material_name, temp_type, rmc_index, nc);

        if (fp)
          fprintf(fp, "%-21.21s%-30.30s%10.0f  %s\n", ndi->rmc[rmc_index].material, temp_type, ndi->uas[nc].area,
                  ndi->rmc[rmc_index].units);

        //update_measure_material_type2(nir->ecm[ecm_index].measure_index, ndi->uas[nc].code, temp_type); // substitute type string
        //update_measure_material_type(ecm_index, temp_type);

        // output to data structure

        if (nir->adjflg) {
          int cnt = nor->num_amaterial;
          nor->amaterial[cnt].index = cnt + 1;
          nor->amaterial[cnt].measure_index = ndi->rmc[rmc_index].measure_index;
          nor->amaterial[cnt].material_id = rmc_index;
          STRCPY(nor->amaterial[cnt].material, temp_material_name);
          STRCPY(nor->amaterial[cnt].type, temp_type);
          nor->amaterial[cnt].quantity = ndi->uas[nc].area;
          STRCPY(nor->amaterial[cnt].units, ndi->rmc[rmc_index].units);
          nor->num_amaterial++;
        } else {
          int cnt = nor->num_material;
          nor->material[cnt].index = cnt + 1;
          nor->material[cnt].measure_index = ndi->rmc[rmc_index].measure_index;
          nor->material[cnt].material_id = rmc_index;
          STRCPY(nor->material[cnt].material, temp_material_name);
          STRCPY(nor->material[cnt].type, temp_type);
          nor->material[cnt].quantity = ndi->uas[nc].area;
          STRCPY(nor->material[cnt].units, ndi->rmc[rmc_index].units);
          nor->num_material++;
        }

      }
    }

    /* Air Conditioners */

    // else if (cms_measure_num == N_CMS_REPLACE_AC && 
    //   ((nir->measure_priority[ecm_index] == MPS_SIR && nir->ecm[ecm_index].sir >= ndi->key.minimum_acceptable_sir) || 
    //     nir->measure_priority[ecm_index] == MPS_EQUIP_REQUIRED ||
    //     nir->measure_priority[ecm_index] == MPS_EQUIP_REQUIRED_NO_SIR)) {

    else if (cms_measure_num == N_CMS_REPLACE_AC) {

      switch (ndi->clg[nir->ecm[ecm_index].dwelling_component_index].system_type) {
      case CET_CENTRAL:

        // The ac replacement might be any of these materials, find the one with non zero measure_index
        rmc_index = N_MAT_REPLACE_AC_CENTRAL_2T_24K;
        if (ndi->rmc[rmc_index].measure_index) break;
        rmc_index = N_MAT_REPLACE_AC_CENTRAL_3T_36K;
        if (ndi->rmc[rmc_index].measure_index) break;
        rmc_index = N_MAT_REPLACE_AC_CENTRAL_4T_48K;
        if (ndi->rmc[rmc_index].measure_index) break;
        ASSERT(FALSE, sprintf(msg, "Did not find central AC measure material"));

      case CET_WINDOW:

        // The ac replacement might be any of these materials, find the one with non zero measure_index
        rmc_index = N_MAT_REPLACE_AC_WIN_5K;
        if (ndi->rmc[rmc_index].measure_index) break;
        rmc_index = N_MAT_REPLACE_AC_WIN_15K;
        if (ndi->rmc[rmc_index].measure_index) break;
        rmc_index = N_MAT_REPLACE_AC_WIN_25K;
        if (ndi->rmc[rmc_index].measure_index) break;
        ASSERT(FALSE, sprintf(msg, "Did not find window AC measure material"));

      default:
        ASSERT(FALSE, sprintf(msg, "Unknown AC replacement type:%d", ndi->clg[nir->ecm[ecm_index].dwelling_component_index].system_type));
      }

      if (fp)
        fprintf(fp, "%-51.51s%-3.0f kBtu%24.24s1  Each\n", 
          ndi->rmc[rmc_index].material, 
          ndi->clg[nir->ecm[ecm_index].dwelling_component_index].size, blanks);

      // output to data structure
      {
        char temp_type[40];
        //sprintf(temp_type, "%-3.0f kBtu",ac_size[nir->ecm[ecm_index].dwelling_component_index]);
        //sprintf(temp_type, "       ");
        //sprintf(temp_type, "");
        temp_type[0] = NULL_CHAR;
        // 10/07 decision not to output size in material type designation

        //update_measure_material_type2(nir->ecm[ecm_index].measure_index, "", temp_type); // substitute type string
        update_measure_material_type(ecm_index, temp_type);

        if (nir->adjflg) {
          int cnt = nor->num_amaterial;
          nor->amaterial[cnt].index = cnt + 1;
          nor->amaterial[cnt].measure_index = ndi->rmc[rmc_index].measure_index;
          nor->amaterial[cnt].material_id = rmc_index;
          STRCPY(nor->amaterial[cnt].material, ndi->rmc[rmc_index].material);
          STRCPY(nor->amaterial[cnt].type, temp_type);
          nor->amaterial[cnt].quantity = 1;
          STRCPY(nor->amaterial[cnt].units, "Each");
          nor->num_amaterial++;
        } else {
          int cnt = nor->num_material; // making coding easier
          nor->material[cnt].index = cnt + 1;
          nor->material[cnt].measure_index = ndi->rmc[rmc_index].measure_index;
          nor->material[cnt].material_id = rmc_index;
          STRCPY(nor->material[cnt].material, ndi->rmc[rmc_index].material);
          STRCPY(nor->material[cnt].type, temp_type);
          nor->material[cnt].quantity = 1;
          STRCPY(nor->material[cnt].units, "Each");
          nor->num_material++;
        }
      }

    }

    /* Furnace replacements */

    else if (cms_measure_num == N_CMS_REPLACE_HEATING_SYSTEM || 
      cms_measure_num == N_CMS_HIGH_EFFICIENCY_FURNACE || 
      cms_measure_num == N_CMS_HIGH_EFFICIENCY_BOILER) {

      // if ((nir->measure_priority[ecm_index] == MPS_SIR && nir->ecm[ecm_index].sir >= ndi->key.minimum_acceptable_sir) || 
      //   nir->measure_priority[ecm_index] == MPS_EQUIP_REQUIRED ||
      //   nir->measure_priority[ecm_index] == MPS_EQUIP_REQUIRED_NO_SIR) {

        furnflg = TRUE;

        if (cms_measure_num == N_CMS_REPLACE_HEATING_SYSTEM)
          rmc_index = N_MAT_STANDARD_EFFICIENCY_FURNACE;
        else
          rmc_index = N_MAT_HIGH_EFFICIENCY_FURNACE;
        if (ndi->htg[PRIMARY].system_type == HE_STEAM_BOILER || ndi->htg[PRIMARY].system_type == HE_HOT_WATER_BOILER) {
          if (cms_measure_num == N_CMS_REPLACE_HEATING_SYSTEM)
            rmc_index = N_MAT_BOILER;
          else
            rmc_index = N_MAT_HIGH_EFFICIENCY_BOILER;
        } else if (ndi->htg[PRIMARY].system_type == HE_UNVENTED_SPACE_HEATER || ndi->htg[PRIMARY].system_type == HE_VENTED_SPACE_HEATER) {

          rmc_index = NOT_APPLICABLE;
          if (ndi->rmc[N_MAT_SPACE_HEATER_GAS_8K].measure_index)   rmc_index = N_MAT_SPACE_HEATER_GAS_8K;
          if (ndi->rmc[N_MAT_SPACE_HEATER_GAS_55K].measure_index)  rmc_index = N_MAT_SPACE_HEATER_GAS_55K;
          if (ndi->rmc[N_MAT_SPACE_HEATER_OIL_40K].measure_index)  rmc_index = N_MAT_SPACE_HEATER_OIL_40K;
          if (ndi->rmc[N_MAT_SPACE_HEATER_OIL_75K].measure_index)  rmc_index = N_MAT_SPACE_HEATER_OIL_75K;
          if (ndi->rmc[N_MAT_SPACE_HEATER_KER_10K].measure_index)  rmc_index = N_MAT_SPACE_HEATER_KER_10K;
          if (ndi->rmc[N_MAT_SPACE_HEATER_KER_40K].measure_index)  rmc_index = N_MAT_SPACE_HEATER_KER_40K;
          ASSERT(rmc_index != NOT_APPLICABLE, sprintf(msg, "Did not find space heater measure material"));

        }

        if (fp)
          fprintf(fp, "%-51.51s%-3.0f kBtu  Existing%14.14s1  Each\n", ndi->rmc[rmc_index].material, ndi->htg[PRIMARY].output_capacity,
                  blanks);

        // output to data structure

        {
          char temp_type[4 * TYPE_LEN];
          char pre_fuel[TYPE_LEN + 1];
          char post_fuel[TYPE_LEN + 1];
          //char *replaced;

          switch (ndi->htg[PRIMARY].fuel_type) {
          case NATURAL_GAS:
            STRCPY(pre_fuel, "NG");
            break;
          case OIL:
            STRCPY(pre_fuel, "Oil");
            break;
          case ELECTRIC:
            STRCPY(pre_fuel, "Elec");
            break;
          case PROPANE:
            STRCPY(pre_fuel, "Prop");
            break;
          case WOOD:
            STRCPY(pre_fuel, "Wood");
            break;
          case COAL:
            STRCPY(pre_fuel, "Coal");
            break;
          case KEROSENE:
            STRCPY(pre_fuel, "Kero");
            break;
          case OTHER:
            STRCPY(pre_fuel, "Othr");
            break;
          default:
            break;
          }

          switch (ndi->htg[PRIMARY].retrofit_fuel_type) {
          case NATURAL_GAS:
            STRCPY(post_fuel, "NG");
            break;
          case OIL:
            STRCPY(post_fuel, "Oil");
            break;
          case ELECTRIC:
            STRCPY(post_fuel, "Elec");
            break;
          case PROPANE:
            STRCPY(post_fuel, "Prop");
            break;
          case WOOD:
            STRCPY(post_fuel, "Wood");
            break;
          case COAL:
            STRCPY(post_fuel, "Coal");
            break;
          case KEROSENE:
            STRCPY(post_fuel, "Kero");
            break;
          case OTHER:
            STRCPY(post_fuel, "Othr");
            break;
          default:
            break;
          }

          sprintf(temp_type, "%-3.0f kBtu/h %s Existing, %-3.0f-%3.0f kBtu/h %s Post", ndi->htg[PRIMARY].output_capacity, pre_fuel,
                  nir->htmt[POST_RETROFIT] / 1000., 1.4 * nir->htmt[POST_RETROFIT] / 1000., post_fuel);

          //update_measure_material_type2(nir->ecm[ecm_index].measure_index, "", temp_type); // substitute type string
          update_measure_material_type(ecm_index, temp_type);

          // remove the (not used) -- which refers to the costs from the
          // setup library -- from the material description

          ASSERT(rmc_index < NMAT, sprintf(msg, "Array out of range"));

          // replaced = replace_str(ndi->rmc[rmc_index].material, " (not used)", " ");
          // STRCPY(ndi->rmc[rmc_index].material, replaced);
          // free(replaced);

          if (nir->adjflg) {
            int cnt = nor->num_amaterial; // making coding easier
            nor->amaterial[cnt].index = cnt + 1;
            nor->amaterial[cnt].measure_index = ndi->rmc[rmc_index].measure_index;
            nor->amaterial[cnt].material_id = rmc_index;
            STRCPY(nor->amaterial[cnt].material, ndi->rmc[rmc_index].material);
            STRCPY(nor->amaterial[cnt].type, temp_type);
            nor->amaterial[cnt].quantity = 1;
            STRCPY(nor->amaterial[cnt].units, "Each");
            nor->num_amaterial++;
          } else {
            int cnt = nor->num_material; // making coding easier
            nor->material[cnt].index = cnt + 1;
            nor->material[cnt].measure_index = ndi->rmc[rmc_index].measure_index;
            nor->material[cnt].material_id = rmc_index;
            STRCPY(nor->material[cnt].material, ndi->rmc[rmc_index].material);
            STRCPY(nor->material[cnt].type, temp_type);
            nor->material[cnt].quantity = 1;
            STRCPY(nor->material[cnt].units, "Each");
            nor->num_material++;
          }
        }

        if (fp)
          fprintf(fp, "%-21.21s%-3.0f kBtu  Post-ret. sized%14.14s\n", blanks, nir->htmt[POST_RETROFIT] / 1000., blanks);
      // }
    }

    /* Heat Pumps */

    // else if (cms_measure_num == N_CMS_INSTALL_OR_REPLACE_HEATPUMP) {
    //   if ((nir->measure_priority[ecm_index] == MPS_SIR && nir->ecm[ecm_index].sir >= ndi->key.minimum_acceptable_sir) || 
    //     nir->measure_priority[ecm_index] == MPS_EQUIP_REQUIRED ||
    //     nir->measure_priority[ecm_index] == MPS_EQUIP_REQUIRED_NO_SIR) {


    else if (cms_measure_num == N_CMS_INSTALL_OR_REPLACE_HEATPUMP) {
      // if ((nir->measure_priority[ecm_index] == MPS_SIR && nir->ecm[ecm_index].sir >= ndi->key.minimum_acceptable_sir) || 
      //   nir->measure_priority[ecm_index] == MPS_EQUIP_REQUIRED ||
      //   nir->measure_priority[ecm_index] == MPS_EQUIP_REQUIRED_NO_SIR) {

        rmc_index = NOT_APPLICABLE;
        if (ndi->rmc[N_MAT_HEATPUMP_2T_24K].measure_index)  rmc_index = N_MAT_HEATPUMP_2T_24K;
        if (ndi->rmc[N_MAT_HEATPUMP_3T_36K].measure_index)  rmc_index = N_MAT_HEATPUMP_3T_36K;
        if (ndi->rmc[N_MAT_HEATPUMP_4T_48K].measure_index)  rmc_index = N_MAT_HEATPUMP_4T_48K;
          ASSERT(rmc_index != NOT_APPLICABLE, sprintf(msg, "Did not find heat pump measure material"));

        if (fp)
          fprintf(fp, "%-51.51s%-3.0f kBtu%24.24s1  Each\n", ndi->rmc[rmc_index].material, ndi->clgs.size_replaced_by_heatpump,
                  blanks);

        // output to data structure

        {
          char temp_type[TYPE_LEN + 1];
          if (ndi->clg[nir->ecm[ecm_index].dwelling_component_index].size < 0.1)
            sprintf(temp_type, "Consult Manual J Data for sizing");
          else
            sprintf(temp_type, "%-3.0f kBtu/h Existing, Consult Manual J Data", ndi->clgs.size_replaced_by_heatpump);

          //update_measure_material_type2(nir->ecm[ecm_index].measure_index, "", temp_type); // substitute type string
                                                       //    update_measure_material_type1(57, "", temp_type);     // substitute type string
          update_measure_material_type(ecm_index, temp_type);


          if (nir->adjflg) {
            int cnt = nor->num_amaterial; // making coding easier
            nor->amaterial[cnt].index = cnt + 1;
            nor->amaterial[cnt].measure_index = ndi->rmc[rmc_index].measure_index;
            nor->amaterial[cnt].material_id = rmc_index;
            STRCPY(nor->amaterial[cnt].material, ndi->rmc[rmc_index].material);
            STRCPY(nor->amaterial[cnt].type, temp_type);
            nor->amaterial[cnt].quantity = 1;
            STRCPY(nor->amaterial[cnt].units, "Each");
            nor->num_amaterial++;
          } else {
            int cnt = nor->num_material; // making coding easier
            nor->material[cnt].index = cnt + 1;
            nor->material[cnt].measure_index = ndi->rmc[rmc_index].measure_index;
            nor->material[cnt].material_id = rmc_index;
            STRCPY(nor->material[cnt].material, ndi->rmc[rmc_index].material);
            STRCPY(nor->material[cnt].type, temp_type);
            nor->material[cnt].quantity = 1;
            STRCPY(nor->material[cnt].units, "Each");
            nor->num_material++;
          }
        //}
      }
    }

  } // end of nir->nmst loop

  // Here is where we print our balance of materials using
  // the entries copied here from library details,  MJF 3/01

  for (i = 0; i <= nir->nother_materials; i++) {
    rmc_index = NMAT + i;
    if (ndi->rmc[rmc_index].quant > 0) {

      if (fp)
        fprintf(fp, "%-21.21s%-30.30s%12.0f  %s\n", ndi->rmc[rmc_index].material, ndi->rmc[rmc_index].retrofit_type, ndi->rmc[rmc_index].quant,
                ndi->rmc[rmc_index].units);

      if (nir->adjflg) {
        int cnt = nor->num_amaterial; // making coding easier
        nor->amaterial[cnt].index = cnt + 1;
        nor->amaterial[cnt].measure_index = ndi->rmc[rmc_index].measure_index;
        nor->amaterial[cnt].material_id = rmc_index;
        STRCPY(nor->amaterial[cnt].material, ndi->rmc[rmc_index].material);
        STRCPY(nor->amaterial[cnt].type, ndi->rmc[rmc_index].retrofit_type);
        nor->amaterial[cnt].quantity = ndi->rmc[rmc_index].quant;
        STRCPY(nor->amaterial[cnt].units, ndi->rmc[rmc_index].units);
        nor->num_amaterial++;
      } else {
        int cnt = nor->num_material; // making coding easier
        nor->material[cnt].index = cnt + 1;
        nor->material[cnt].measure_index = ndi->rmc[rmc_index].measure_index;
        nor->material[cnt].material_id = rmc_index;
        STRCPY(nor->material[cnt].material, ndi->rmc[rmc_index].material);
        STRCPY(nor->material[cnt].type, ndi->rmc[rmc_index].retrofit_type);
        nor->material[cnt].quantity = ndi->rmc[rmc_index].quant;
        STRCPY(nor->material[cnt].units, ndi->rmc[rmc_index].units);
        nor->num_material++;
      }

    }
  }

  /* Materialsl for Itemized costs and user-defined measures */

  for (j = 0; j < ndi->num_itc; j++)
    if (ndi->itc[j].material[0] != '\0' && nir->pntitcretrofit_materials[j] > 0) {

      if (fp)
        fprintf(fp, "%-51.51s\n", ndi->itc[j].material);

      if (nir->adjflg) {
        int cnt = nor->num_amaterial; // making coding easier
        nor->amaterial[cnt].index = cnt + 1;
        if (ndi->itc[j].ecm_index != NOT_APPLICABLE)
          nor->amaterial[cnt].measure_index = nir->ecm[ndi->itc[j].ecm_index].measure_index;
        else 
          nor->amaterial[cnt].measure_index = ndi->itc[j].measure_index;
        nor->amaterial[cnt].material_id = 200 + j;
        STRCPY(nor->amaterial[cnt].material, ndi->itc[j].material);
        nor->amaterial[cnt].quantity = 1;
        STRCPY(nor->amaterial[cnt].units, "Each");
        nor->num_amaterial++;
      } else {
        int cnt = nor->num_material; // making coding easier
        nor->material[cnt].index = cnt + 1;
        if (ndi->itc[j].ecm_index != NOT_APPLICABLE)
          nor->material[cnt].measure_index = nir->ecm[ndi->itc[j].ecm_index].measure_index;
        else 
          nor->material[cnt].measure_index = ndi->itc[j].measure_index;
        nor->material[cnt].material_id = 200 + j;
        STRCPY(nor->material[cnt].material, ndi->itc[j].material);
        nor->material[cnt].quantity = 1;
        STRCPY(nor->material[cnt].units, "Each");
        nor->num_material++;
      }

    }

  if (furnflg == TRUE) {
    char *msg = "NOTE: Read cautions in NEAT User's Manual related to sizing results.";
    add_neat_message(msg);
    if (fp)
      fprintf(fp, "\n %s", msg);
  }

}

// Return a specific type string for the fill insulation material #319
void fill_ceiling_cavity_names(char *material_name, char *type_name, int rmc_index, int nc) {
  if (ndi->uas[nc].added_R > 1.0f) {
    sprintf(type_name, "%s R-%02.0f", ndi->rmc[rmc_index].retrofit_type, round(ndi->uas[nc].added_R));
  } else {
    sprintf(type_name, "%s %02.0f in.", ndi->rmc[rmc_index].retrofit_type, round(ndi->uas[nc].max_depth));
  }

  // The type_name specifies the insulation level, so we strip off the insulation level in the material
  // description since that was used only for interpoating the cost #319
  int name_len = strlen(ndi->rmc[rmc_index].material) - strlen(" R11");
  strncpy(material_name, ndi->rmc[rmc_index].material, name_len);
  material_name[name_len] = '\0';
  return;
}

/*******************
 *******************
 This routine prints the annual loads, consumptions, and sizing results */

void report_energy(FILE *filetmp) {

  if (nir->adjflg == 0) {
    if (filetmp) {
      fprintf(filetmp, "\n\n\n");
      fprintf(filetmp, "                           Pre-retofit       Post-retrofit\n");
      fprintf(filetmp, "                        Heating  Cooling   Heating  Cooling\n\n");
      fprintf(filetmp, " Annual load (MMBtu/yr)%8.0f%9.0f%10.0f%9.0f\n", nir->heatload[PRE_RETROFIT_PRE_INFIL], nir->coolload[PRE_RETROFIT_PRE_INFIL], nir->heatload[LAST_GOOD_MEASURE], nir->coolload[LAST_GOOD_MEASURE]);
      fprintf(filetmp, " Annual Energy (MMBtu/yr)%6.0f%9.0f%10.0f%9.0f\n", nir->heatengy[PRE_RETROFIT_PRE_INFIL], nir->coolengy[PRE_RETROFIT_PRE_INFIL], nir->heatengy[LAST_GOOD_MEASURE], nir->coolengy[LAST_GOOD_MEASURE]);
      fprintf(filetmp, " Heat loss/gain (kBtu/hr)%6.0f%9.0f%10.0f%9.0f\n", (nir->htmt[PRE_RETROFIT] - nir->htduct[PRE_RETROFIT]) / 1000., nir->clmt[PRE_RETROFIT] / 1000.,
              (nir->htmt[POST_RETROFIT] - nir->htduct[POST_RETROFIT]) / 1000., nir->clmt[POST_RETROFIT] / 1000.);
      fprintf(filetmp, " Output (kBtu/hr or Ton)%6.0f%9.1f%10.0f%9.1f\n", nir->htmt[PRE_RETROFIT] / 1000., nir->clmt[PRE_RETROFIT] / BTUH_PER_TON, nir->htmt[POST_RETROFIT] / 1000.,
              nir->clmt[POST_RETROFIT] / BTUH_PER_TON);
      fprintf(filetmp, "\n  Design building heat loss and output required intended only as guides");
      fprintf(filetmp, "\n   to sizing equipment.  See NEAT User's Manual for further details.");
      fprintf(filetmp, "\n NOTE: Read cautions in NEAT User's Manual related to sizing results.");
      fprintf(filetmp, "\n NOTE: (+) in the Materials list indicates there are more related User Defined Materials.");
    }

    // structure output

    nor->an_load[ANNUAL_THERMAL_LOAD_MMBTU].index = 1;
    STRCPY(nor->an_load[ANNUAL_THERMAL_LOAD_MMBTU].name, "Annual Load (MMBtu/yr)");
    nor->an_load[ANNUAL_THERMAL_LOAD_MMBTU].pre_heat = nir->heatload[PRE_RETROFIT_PRE_INFIL];
    nor->an_load[ANNUAL_THERMAL_LOAD_MMBTU].pre_cool = nir->coolload[PRE_RETROFIT_PRE_INFIL];
    nor->an_load[ANNUAL_THERMAL_LOAD_MMBTU].post_heat = nir->heatload[LAST_GOOD_MEASURE];
    nor->an_load[ANNUAL_THERMAL_LOAD_MMBTU].post_cool = nir->coolload[LAST_GOOD_MEASURE];

    nor->an_load[ANNUAL_SYSTEM_ENERGY_MMBTU].index = 2;
    STRCPY(nor->an_load[ANNUAL_SYSTEM_ENERGY_MMBTU].name, "Annual Energy (MMBtu/yr)");
    nor->an_load[ANNUAL_SYSTEM_ENERGY_MMBTU].pre_heat = nir->heatengy[PRE_RETROFIT_PRE_INFIL] + nir->heatengy[PRE_RETROFIT_POST_DUCT_SEAL] - nir->heatengy[PRE_RETROFIT_POST_INFIL];
    nor->an_load[ANNUAL_SYSTEM_ENERGY_MMBTU].pre_cool = nir->coolengy[PRE_RETROFIT_PRE_INFIL];
    nor->an_load[ANNUAL_SYSTEM_ENERGY_MMBTU].post_heat = nir->heatengy[LAST_GOOD_MEASURE];
    nor->an_load[ANNUAL_SYSTEM_ENERGY_MMBTU].post_cool = nir->coolengy[LAST_GOOD_MEASURE];

    nor->an_load[DESIGN_DAY_HEAT_LOSS_GAIN_PEAK_BTUH].index = 3;
    STRCPY(nor->an_load[DESIGN_DAY_HEAT_LOSS_GAIN_PEAK_BTUH].name, "Heat Loss/Gain (kBtu/hr)");
    nor->an_load[DESIGN_DAY_HEAT_LOSS_GAIN_PEAK_BTUH].pre_heat = (nir->htmt[PRE_RETROFIT] - nir->htduct[PRE_RETROFIT]) / 1000.0f;
    nor->an_load[DESIGN_DAY_HEAT_LOSS_GAIN_PEAK_BTUH].pre_cool = nir->clmt[PRE_RETROFIT] / 1000.0f;
    nor->an_load[DESIGN_DAY_HEAT_LOSS_GAIN_PEAK_BTUH].post_heat = (nir->htmt[POST_RETROFIT] - nir->htduct[POST_RETROFIT]) / 1000.0f;
    nor->an_load[DESIGN_DAY_HEAT_LOSS_GAIN_PEAK_BTUH].post_cool = nir->clmt[POST_RETROFIT] / 1000.0f;

    nor->an_load[DESIGN_DAY_OUTPUT_REQUIRED_BTUH].index = NEAT_MAX_ANLOAD;
    STRCPY(nor->an_load[DESIGN_DAY_OUTPUT_REQUIRED_BTUH].name, "Output Required (kBtu/hr)(ton)");
    nor->an_load[DESIGN_DAY_OUTPUT_REQUIRED_BTUH].pre_heat = nir->htmt[PRE_RETROFIT] / 1000.0f;
    nor->an_load[DESIGN_DAY_OUTPUT_REQUIRED_BTUH].pre_cool = nir->clmt[PRE_RETROFIT] / BTUH_PER_TON;
    nor->an_load[DESIGN_DAY_OUTPUT_REQUIRED_BTUH].post_heat = nir->htmt[POST_RETROFIT] / 1000.0f;
    nor->an_load[DESIGN_DAY_OUTPUT_REQUIRED_BTUH].post_cool = nir->clmt[POST_RETROFIT] / BTUH_PER_TON;

    add_neat_message("NOTE: Heat loss and Output required are only guides to sizing equipment.");
    add_neat_message("NOTE: See NEAT User's Manual for further sizing details.");
    add_neat_message("NOTE: Read cautions in NEAT User's Manual related to sizing results.");  
    add_neat_message("NOTE: (+) in the Materials list indicates there are more related User Defined Materials.");

  }
}

// output the measure material cost detail records for implemented
// measures using the ecm and ecmm arrays. Assumes that the nir->ecmm[ecmm_index].measure_index
// has been filled in with base 1 measure index values for implemented measures prior to this call
// Refactored 12/9/2020 #319

void report_measure_materials(void) {

  if (cmds.debug_level & D_NORMAL) {
    fprintf(stderr, "\nECMM Array on entry to report_measure_materials\n");
    fprintf(stderr, "\n ecm_index Mdx rmc           comp                  desc                                type            matid\n");
  }

  for (int ecmm_index = 0; ecmm_index < nir->nmsm; ecmm_index++) {
    if (nir->ecmm[ecmm_index].measure_index) {

      int mcnt = nor->num_measure_material;        // mmaterial[] JSON output array counter

      if (cmds.debug_level & D_NORMAL) {
        fprintf(stderr, "%3d %2d %2d %30s %30s %30s %2d\n",
          nir->ecmm[ecmm_index].ecm_index,
          nir->ecmm[ecmm_index].measure_index,
          nir->ecmm[ecmm_index].rmc_index,
          nir->ecmm[ecmm_index].components,
          nir->ecmm[ecmm_index].desc,
          nir->ecmm[ecmm_index].type,
          nir->ecmm[ecmm_index].material_type_id);
      }

      nor->mmaterial[mcnt].index = mcnt + 1;                                          // base 1 index for json output
      nor->mmaterial[mcnt].measure_index = nir->ecmm[ecmm_index].measure_index;       // PK of matching nor->measure[] record
      nor->mmaterial[mcnt].material_id = nir->ecmm[ecmm_index].rmc_index;
      nor->mmaterial[mcnt].qty_est = nir->ecmm[ecmm_index].qty_est;                   // quantity of the material itself, estimated
      nor->mmaterial[mcnt].unit_cost_est = nir->ecmm[ecmm_index].unit_cost_est;       // unit cost, estimated
      nor->mmaterial[mcnt].material_type_id = nir->ecmm[ecmm_index].material_type_id; // fixed material reference number

      if (strlen(nir->ecmm[ecmm_index].components) > 0)
        STRCPY(nor->mmaterial[mcnt].components, nir->ecmm[ecmm_index].components);

      if (strlen(nir->ecmm[ecmm_index].desc) > 0)
        STRCAT(nor->mmaterial[mcnt].description, nir->ecmm[ecmm_index].desc);

      if (strlen(nir->ecmm[ecmm_index].type) > 0) {
        STRCAT(nor->mmaterial[mcnt].description, " - ");
        STRCAT(nor->mmaterial[mcnt].description, nir->ecmm[ecmm_index].type);
      }

      if (strlen(nir->ecmm[ecmm_index].units) > 0)
        STRCPY(nor->mmaterial[mcnt].units, nir->ecmm[ecmm_index].units); // description of units
      if (strlen(nir->ecmm[ecmm_index].comment) > 0)
        STRCPY(nor->mmaterial[mcnt].comment, nir->ecmm[ecmm_index].comment); // additional words of wisdom

      nor->num_measure_material++; // next record, see mcnt above

      // we have mmaterial cost detail records and they
      // substitute for the aggregated values in the measure[] record
      // so we zero these aggregated values in the JSON output

      int index = nir->ecmm[ecmm_index].measure_index - 1;    // it's base 0 in the output array, but reported base 1
      nor->measure[index].qtym = 0;
      nor->measure[index].qtyl = 0;
      nor->measure[index].qtyi = 0;
      nor->measure[index].costum = 0;
      nor->measure[index].costul = 0;
      nor->measure[index].costi1 = 0;
      nor->measure[index].costi2 = 0;
      nor->measure[index].typei2 = MT_NONE;
      nor->measure[index].desci2[0] = '\0';
      nor->measure[index].costi3 = 0;
      nor->measure[index].typei3 = MT_NONE;
      nor->measure[index].desci3[0] = '\0';

    }
  }

  return;
}

void update_measure_material_type(int ecm_index, char *mat_type) {
  for (int ecmm_index = 0; ecmm_index < nir->nmsm; ecmm_index++) {
    if (nir->ecmm[ecmm_index].ecm_index == ecm_index) {
      STRCPY(nir->ecmm[ecmm_index].type, mat_type);
    }
  }
  return;
}

// #265 Needed to restrict the material type update further
void update_measure_material_type_using_code(int ecm_index, char *code, char *mat_type) {
  for (int ecmm_index = 0; ecmm_index < nir->nmsm; ecmm_index++) {
    if (nir->ecmm[ecmm_index].ecm_index == ecm_index) {
      if(strlen(code)) {
        if (strstr(nir->ecmm[ecmm_index].components, code)) {
          STRCPY(nir->ecmm[ecmm_index].type, mat_type);
        }
      } else {
        ASSERT(FALSE, sprintf(msg, "Need code to update measure material type for ecmm[%d]", ecmm_index));
      }
    }
  }
  return;
}

// simply adds a string message to our output struct and increments the
// message counter

void add_neat_message(char *msg) {
  if (strlen(msg) < MESSAGE_LEN) 
    STRCPY(nor->message[nor->num_message++], msg);
  return;
}

/*************************
 *************************
 *  Determine the length of a string excluding any blank padding  */
int get_string_length_without_padding(char *string) {
  char blank = ' ';
  int next, nchrs;

  nchrs = 0;
  for (next = strlen(string); next >= 0; next--) {
    if (string[next] != blank && string[next] != 0) {
      nchrs = next + 1;
      string[nchrs] = '\0';
      break;
    }
  }
  return (nchrs);
}
