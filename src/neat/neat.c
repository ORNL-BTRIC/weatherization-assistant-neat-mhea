/***************************************************************************
* MODULE:       neat.c            CREATED:     Nov 5, 2018 (migration MJF)
*
* AUTHOR:       Mike Gettings    Oak Ridge National Laboratory
*               Mark Fishbaugher (mark@fishbaugher.com)
*
* MDESC:        Contains the main neat_run()function, previously named
*               load37.c for some reason lost to the dimm recesses of
*               human memory.  With the migration to *nix and JSON, it's
*               time for an update module name
****************************************************************************/
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "wa_engine.h"

static void sort_neat_package_measures(float *, int, int *);
static void recombine_window_measures();
static void measure_diag_print(char *prefix, int i, int s);
static void measure_diag_print_all(char *title, int sir_sort);
static void measure_diag_print_header(char *title);
static void measure_diag_print_line(int i, int s, char *suffix);
static void prevent_sill_insulation_dropout( void);
static void cumulative_interactive_effects(void);
static void heating_system_priority_adjustments(void);

#define UNSORTED 0
#define SORTED 1

// pre-duct parameters used in the duct sealing calculations

// **********************************************************************
//  Function Name: run_neat
//      Date: January 28, 2000
//      Author(s): Mark Fishbaugher
//  
//   DESCRIPTION: Entry point for the NEAT analysis 
//  **********************************************************************
void run_neat(void) {

  FILE *measfile, *comparefile;

  ASSERT(ndi, sprintf(msg, "You must have NEAT dwelling information structure to run engine"));
  ASSERT(nir, sprintf(msg, "You must have NEAT intermediate result structure to run engine"));
  ASSERT(nor, sprintf(msg, "You must have NEAT output result structure to run engine"));

  initialize_neat_measure_exclusion();

  initialize_billing();

  initialize_fuel_cost_data(ndi->fcs, ndi->fer, 1.0f + (ndi->key.real_discount_rate / 100.0f));

  translate_parms();

  translate_ndi();

  translate_neat_bil();

  read_weather_file(&ndi->wth);

  // Apply climate adjustment factor to replacement HP and central AC efficiencies

  adjust_seer(cwd->summer_hp_design_temp, &ndi->key.new_central_ac_seer);
  adjust_seer(cwd->summer_hp_design_temp, &ndi->key.new_heatpump_cooling_seer);

  neat_preparation();

  // No tune up for electric furnace, just clear up some confusion about the define here
  // should be no functional difference
  if (ndi->htg[PRIMARY].fuel_type == ELECTRIC && ndi->htg[PRIMARY].retrofit_option == ES_NO_REP_EVAL_TUNEUP) {
    ndi->htg[PRIMARY].retrofit_option = ES_EVALUATE_NONE;
  }

  // Insure heating sys. replacement efficiency is set to default or setup
  // value if no value has been entered on the input form
  if (ndi->htg[PRIMARY].fuel_type == ELECTRIC && ndi->htg[PRIMARY].retrofit_option == ES_ELECRES_REP_REQUIRED &&
      ndi->htg[PRIMARY].system_type != HE_HEAT_PUMP) {
    ndi->htg[PRIMARY].retrofit_afue_standard_eff = 100.0f;
    ndi->htg[PRIMARY].retrofit_fuel_type = ELECTRIC;
  }

  // ratio of the roof area to the ceiling area
  // farc = (float)sqrt(1.0f + ndi->uas[0].pitch * ndi->uas[0].pitch);  // constant MJF 5/19

  for (int i = 0; i < MAXECMS; i++)
    nir->associated_winner_ecm_index[i] = NOT_APPLICABLE;

  // initialize UA sums and our array of nighttime thermostat temps
  nir->ua_total = 0.0;
  nir->ua_walls = 0.0;
  nir->ua_windows = 0.0;
  nir->ua_doors = 0.0;
  nir->ua_attics = 0.0;
  nir->ua_foundations = 0.0;

  for (int m = 0; m <= MONTHS; m++)
    nir->night_setback_temperature[m] = ndi->key.nighttime_heating_setpoint;

  //  Wall parameters
  for (int nc = 0; nc < ndi->num_wal; nc++) {
    float annual_outside_film_coeff;
    //if (ndi->wal[nc].exposure == EX_BUFFERED || ndi->wal[nc].exposure == EX_ATTIC) {
    if (ndi->wal[nc].exposure == EX_BUFFERED) {
      annual_outside_film_coeff = 1.46f;
    } else {
      annual_outside_film_coeff = ndi->key.annual_outside_film_coeff;
    }

    ASSERT(annual_outside_film_coeff, sprintf(msg, "Outside film coefficient should be non zero"));
    ASSERT((1. + ndi->wal[nc].u_frame), sprintf(msg, "Need non zero uvalue"));
    ASSERT((1. + ndi->wal[nc].u_cavity), sprintf(msg, "Need non zero uvalue"));

    ndi->wal[nc].u_frame = ndi->wal[nc].u_frame / (1.0f + ndi->wal[nc].u_frame / annual_outside_film_coeff);
    ndi->wal[nc].u_cavity = ndi->wal[nc].u_cavity / (1.0f + ndi->wal[nc].u_cavity / annual_outside_film_coeff);
  }

  // Ceiling parameters
  for (int nc = 0; nc < ndi->num_uas; nc++) {
    ndi->uas[nc].ventilation_cuft_per_hr = ATTIC_CFM_PER_SQFT * ndi->uas[nc].area * 60.0f;

    if (ndi->uas[nc].attic_type == UAS_KNEEWALL) { /* for kneewalls */
      float temp = ndi->uas[nc].ins_depth;
      if (temp > 3.5f)
        temp = 3.5f;
      ndi->uas[nc].joist_u_value = 1.0f / (1.81f + FRAMING_R_PER_IN * temp); // 1.81 = 0.68+0.45+0.68
      ndi->uas[nc].ventilation_cuft_per_hr *= 3.0f;
      continue;
    }

    if (ndi->uas[nc].ins_depth > 5.5f) { // Add R-value of ins. over joists
      float tempp = BASE_CEILING_R + 5.5f * FRAMING_R_PER_IN;
      float temp = ndi->uas[nc].ins_depth - 5.5f; // Assumes 5.5" joist height
      ASSERT((tempp + temp * attic_ins_exist_rpi(nc)), sprintf(msg, "Need non zero r per inch"));
      ndi->uas[nc].joist_u_value = 1.0f / (tempp + temp * attic_ins_exist_rpi(nc));
    } else { // Use effective joist depth
      ASSERT((BASE_CEILING_R + ndi->uas[nc].ins_depth * FRAMING_R_PER_IN), sprintf(msg, "Need non zero r per inch"));
      ndi->uas[nc].joist_u_value = 1.0f / (BASE_CEILING_R + ndi->uas[nc].ins_depth * FRAMING_R_PER_IN);
    }

  }

  //  Subspace parameters
  for (int nc = 0; nc < ndi->num_fnd; nc++) {
    if (ndi->fnd[nc].space_type != UNINSULATED_SLAB && ndi->fnd[nc].space_type != INSULATED_SLAB &&
        ndi->fnd[nc].space_type != EXPOSED_FLOOR_CLOSED) {

      ASSERT(ndi->key.annual_outside_film_coeff, sprintf(msg, "Outside film coefficient should be non zero"));

      ASSERT((1.0f + ndi->fnd[nc].above_grade_wall_u_value / ndi->key.annual_outside_film_coeff),
             sprintf(msg, "Need non zero uvalue"));
      ASSERT((1.0f + ndi->fnd[nc].below_grade_wall_u_value / ndi->key.annual_outside_film_coeff),
             sprintf(msg, "Need non zero uvalue"));

      ndi->fnd[nc].above_grade_wall_u_value = ndi->fnd[nc].above_grade_wall_u_value /
                                              (1.0f + ndi->fnd[nc].above_grade_wall_u_value / ndi->key.annual_outside_film_coeff);
      ndi->fnd[nc].below_grade_wall_u_value = ndi->fnd[nc].below_grade_wall_u_value /
                                              (1.0f + ndi->fnd[nc].below_grade_wall_u_value / ndi->key.annual_outside_film_coeff);
    }
  }

  if (ndi->gnl.avg_no_occupants < 0.0001) { /* for versions 4.3 and before */
    if (ndi->gnl.floor_area > 1000.0f)
      ndi->key.base_free_heat_from_internals += 224.0f * (ndi->gnl.floor_area - 1000.0f) / 400.0f;
  } else if (ndi->gnl.avg_no_occupants > 2.0f)
    ndi->key.base_free_heat_from_internals += 224.0f * (ndi->gnl.avg_no_occupants - 2.0f);
  else
    ndi->key.base_free_heat_from_internals += (ndi->gnl.avg_no_occupants * 276.0f - 552.0f);

  if (ndi->htg[PRIMARY].percent_heat_supplied > .1)
    ndi->key.nighttime_heating_setback *= ndi->htg[PRIMARY].percent_heat_supplied;

  cwd->avg_solar_horizontal[TOTAL] = 0.0;

  for (int j = 1; j <= MONTHS; j++)
    cwd->avg_solar_horizontal[TOTAL] += cwd->avg_solar_horizontal[j];

  // Weather data degree hour bins

  if (cmds.debug_level & D_WEATHER_DATA_ECHO) {
    fprintf(stderr, "\nWeather data echo\nmonth/tbal");
    fprintf(stderr, " %11.9g", cwd->balance_point_temp[DH_BIN_40]);
    for (int i = 1; i <= DH_BIN_75; i++)
      fprintf(stderr, "   %11.9g", cwd->balance_point_temp[i]);
    fprintf(stderr, "\n  Daytime heating degree hours\n");
    for (int j = 1; j <= MONTHS; j++) {
      fprintf(stderr, "  %2d    ", j);
      for (int i = 0; i <= DH_BIN_75; i++)
        fprintf(stderr, "   %11.9g", cwd->degree_hours[DAY][HEATING][i][j]);
      fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n Total  ");
    for (int i = 0; i <= DH_BIN_75; i++)
      fprintf(stderr, "   %11.9g", cwd->degree_hours[DAY][HEATING][i][TOTAL]);
    fprintf(stderr, "\n");

    fprintf(stderr, "\n  Nighttime heating degree hours\n");
    for (int j = 1; j <= MONTHS; j++) {
      fprintf(stderr, "  %2d    ", j);
      for (int i = 0; i <= DH_BIN_75; i++)
        fprintf(stderr, "   %11.9g", cwd->degree_hours[NIGHT][HEATING][i][j]);
      fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n Total  ");
    for (int i = 0; i <= DH_BIN_75; i++)
      fprintf(stderr, "   %11.9g", cwd->degree_hours[NIGHT][HEATING][i][TOTAL]);
    fprintf(stderr, "\n Total  ");
    for (int i = 0; i <= DH_BIN_75; i++)
      fprintf(stderr, "   %11.9g", cwd->degree_hours[DAY][HEATING][i][TOTAL] + cwd->degree_hours[NIGHT][HEATING][i][TOTAL]);
    fprintf(stderr, "\n Total  ");
    for (int i = 0; i <= DH_BIN_75; i++)
      fprintf(stderr, "   %11.9g",
              (cwd->degree_hours[DAY][HEATING][i][TOTAL] + cwd->degree_hours[NIGHT][HEATING][i][TOTAL]) / 24.);
    fprintf(stderr, "\n");

    fprintf(stderr, "\n  Daytime cooling degree hours\n");
    for (int j = 1; j <= MONTHS; j++) {
      fprintf(stderr, "  %2d    ", j);
      for (int i = 0; i <= DH_BIN_75; i++)
        fprintf(stderr, "   %11.9g", cwd->degree_hours[DAY][COOLING][i][j]);
      fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n Total  ");
    for (int i = 0; i <= DH_BIN_75; i++)
      fprintf(stderr, "   %11.9g", cwd->degree_hours[DAY][COOLING][i][TOTAL]);
    fprintf(stderr, "\n");

    fprintf(stderr, "\n  Nighttime cooling degree hours\n");
    for (int j = 1; j <= MONTHS; j++) {
      fprintf(stderr, "  %2d    ", j);
      for (int i = 0; i <= DH_BIN_75; i++)
        fprintf(stderr, "   %11.9g", cwd->degree_hours[NIGHT][COOLING][i][j]);
      fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n Total  ");
    for (int i = 0; i <= DH_BIN_75; i++)
      fprintf(stderr, "   %11.9g", cwd->degree_hours[NIGHT][COOLING][i][TOTAL]);
    fprintf(stderr, "\n Total  ");
    for (int i = 0; i <= DH_BIN_75; i++)
      fprintf(stderr, "   %11.9g", cwd->degree_hours[DAY][COOLING][i][TOTAL] + cwd->degree_hours[NIGHT][COOLING][i][TOTAL]);
    fprintf(stderr, "\n Total  ");
    for (int i = 0; i <= DH_BIN_75; i++)
      fprintf(stderr, "   %11.9g",
              (cwd->degree_hours[DAY][COOLING][i][TOTAL] + cwd->degree_hours[NIGHT][COOLING][i][TOTAL]) / 24.);
    fprintf(stderr, "\n");

    fprintf(stderr, "\nMonthly total solar radiation on horizontal surface\
    (Btu/sqft)\n");
    for (int j = 1; j <= MONTHS; j++)
      fprintf(stderr, " %11.9g", cwd->avg_solar_horizontal[j]);
    fprintf(stderr, " %11.9g", cwd->avg_solar_horizontal[TOTAL]);
    fprintf(stderr, "\n");
  }

  // Compute cfm at 50 pascals, effective leakage area, and infiltration at time of blower-door test

  ASSERT(ndi->inf.post_inf_pa != 0, sprintf(msg, "Need non zero pressure"));

  //  pa_house[0] != 0 Can be 0 as long as ndi->inf.pre_inf_cfm > .01. Input requires this. (MBG 7/23/99)

  nir->infiltration_treatment = infiltration_reduction_treatment(ndi->inf);

  nir->whole_house_cfm_50_post = blower_door_corrected_cfm((BASE_PRESSURE / ndi->inf.post_inf_pa), ndi->inf.post_inf_cfm);
  if (ndi->inf.pre_inf_cfm > .01) {                     // non blank pre retrofit blower door cfm
    nir->whole_house_cfm_50_pre = blower_door_corrected_cfm((BASE_PRESSURE / ndi->inf.post_duct_seal_pa), ndi->inf.post_duct_seal_cfm);
    ndi->cms[N_CMS_INFILTRATION_REDUCTION].active = TRUE;
  } else { 
    ndi->cms[N_CMS_INFILTRATION_REDUCTION].active = FALSE;
    nir->whole_house_cfm_50_pre = ndi->gnl.floor_area * 8.0f / 60.0f * 20.0f;

    int house_leakiness = 2; // TODO, make this an input or key parameter? Temporary local till address linkage, 1->tight, =2->medium, =3->loose

    switch (house_leakiness) {
    case 1:
      nir->whole_house_cfm_50_pre *= 0.5f;
      break; // Constants are Air Changes / hour
    case 2:
      nir->whole_house_cfm_50_pre *= 0.8f;
      break; // Convert to cfm at 50 Pa
    case 3:
      nir->whole_house_cfm_50_pre *= 1.2f;
      break;
    default:
      nir->whole_house_cfm_50_pre *= 0.8f;
    }
  }

  monthly_infiltration(nir->whole_house_cfm_50_pre, 
    nir->whole_house_cfm_50_post, 
    ndi->key.daytime_heating_setpoint, 
    ndi->gnl.no_cond_stories,
    nir->whole_house_cfm);

  nir->ua_infiltration = nir->whole_house_cfm[POST_RETROFIT][AVG] * 60.0f * RHOCAIR;

  // #242 Constrain window and door infiltration to a percentage of the whole house value

  nir->window_cfm_adjustment = window_constrain_cfm_factor(nir->whole_house_cfm, nir->wn_cfm_tot, nir->dr_cfm_tot);
  if (nir->window_cfm_adjustment < 1.0f) {
    char msg[512];
    sprintf(msg, "Window infiltration constrained to %0.0f %% of whole house total reduced by an average of %0.1f %%", MAX_WINDOW_DOOR_PERCENT, nir->window_cfm_adjustment * 100);
    add_neat_message(msg);
    if (cmds.debug_level & D_INFILTRATION_DETAIL) {
      fprintf(stderr, "\nAverage adjusted monthly WINDOW cfm = %7.2f", nir->wn_cfm_tot[AVG]);
      fprintf(stderr, "\nPercent reduction in average WINDOW leakage = ");
      fprintf(stderr, "%7.1f\n", nir->window_cfm_adjustment * 100.);
    }
  }
  window_constrain_cfm(nir->whole_house_cfm, nir->wn_leak_cfm, nir->wn_cfm_tot, nir->dr_cfm_tot, ndi->num_win, ndi->gnl.floor_area);
  for (int m = 1; m <= MONTHS; m++) {
    nir->save_win_cfm_tot[m] = nir->wn_cfm_tot[m];
  }

  // Doors as well
  
  nir->door_cfm_adjustment = door_constrain_cfm_factor(nir->whole_house_cfm, nir->wn_cfm_tot, nir->dr_cfm_tot);
  if (nir->door_cfm_adjustment < 1.0f) {
    char msg[512];
    sprintf(msg, "Door infiltration constrained to %0.0f %% of whole house total reduced by an average of %0.1f %%", MAX_WINDOW_DOOR_PERCENT, nir->door_cfm_adjustment * 100);
    add_neat_message(msg);
    if (cmds.debug_level & D_INFILTRATION_DETAIL) {
      fprintf(stderr, "\nAverage adjusted monthly DOOR cfm = %7.2f", nir->wn_cfm_tot[AVG]);
      fprintf(stderr, "\nPercent reduction in average DOOR leakage = ");
      fprintf(stderr, "%7.1f\n", nir->door_cfm_adjustment * 100.);
    }
  }
  door_constrain_cfm(nir->whole_house_cfm, nir->wn_leak_cfm, nir->wn_cfm_tot, nir->dr_cfm_tot, ndi->num_win, ndi->gnl.floor_area);
  for (int m = 1; m <= MONTHS; m++) {
    nir->save_dor_cfm_tot[m] = nir->dr_cfm_tot[m];
  }

  sizing_heating(PRE_RETROFIT);
  sizing_cooling(PRE_RETROFIT);

  // Assign the primary heating system output capacity IF missing or type is HEAT pump (units of kBtu/hr)
  if (ndi->htg[PRIMARY].output_capacity < .001f || ndi->htg[PRIMARY].system_type == HE_HEAT_PUMP) {
    ndi->htg[PRIMARY].output_capacity = nir->htmt[0] / 1000.0f; // Heatpumps 
    char msg[512];
    sprintf(msg, "Assigned primary heating system output: %7.1f kBtu", ndi->htg[PRIMARY].output_capacity);
    add_neat_message(msg);
  }

  // Determine primary system seasonal duct efficiency
  // Note that non unity duct efficiency is calculated only if duct sealing is evaluated

  {
    float ducteff_ht, ducteff_cl;
    duct_efficiency_NEAT(ndi->inf.pre_duct_50_cfm, ndi->inf.pre_duct_seal_supply_pa, ndi->inf.pre_duct_seal_return_pa, &ducteff_ht,
               &ducteff_cl);

    ndi->inf.pre_duct_seal_efficiency = 1.0f;
    ndi->inf.post_duct_seal_efficiency = 1.0f;

    if (ndi->inf.evaluate_duct_sealing == YES && fabs(ducteff_ht - 1.0f) > 0.001) {               // Evaluate ducts
      ndi->inf.pre_duct_seal_efficiency = (float)(ducteff_ht / HTG_DUCT_ADJ);                     // Constant nomally = 0.8
      if (ndi->inf.post_duct_seal_supply_pa > 0.0f && ndi->inf.post_duct_seal_return_pa > 0.0f) { // Post-duct sealing data entered
        duct_efficiency_NEAT(ndi->inf.post_duct_50_cfm, ndi->inf.post_duct_seal_supply_pa, ndi->inf.post_duct_seal_return_pa, &ducteff_ht,
                   &ducteff_cl);
        ndi->inf.post_duct_seal_efficiency = (float)(ducteff_ht / HTG_DUCT_ADJ);
        //nir->implement[N_CMS_DUCT_SEALING] = TRUE;
        ndi->cms[N_CMS_DUCT_SEALING].active = TRUE;

        if (cmds.debug_level & D_INFILTRATION_DETAIL) {
          fprintf(stderr, "\npre_duct_50_cfm:%.4f post_duct_50_cfm:%.4f ducteff_ht:%.4f", ndi->inf.pre_duct_50_cfm,
          ndi->inf.post_duct_50_cfm, ducteff_ht);
        }
      }
    }
  }

  {
    float tot_heat_supplied = 0.0f;
    for (int i = 0; i < ndi->num_htg; i++) {
      tot_heat_supplied += ndi->htg[i].percent_heat_supplied;
      float tempeff = ndi->htg[i].steady_state_eff;
      if (tempeff < 1.0f)
        tempeff = DEFAULT_STANDARD_FURNACE_SS_EFFICIENCY; /* Use default efficiency */
      if (ndi->htg[i].fuel_type < 0)
        ndi->htg[i].fuel_type = 1; /* Default fuel type, MJF 11/97 */
      tempeff /= 100.;

      if (ndi->htg[i].system_type == HE_HEAT_PUMP) {
        float temphspf, year;
        temphspf = ndi->htg[i].hspf;
        year = ndi->htg[i].year_manufactured;

        // #241 use lookup table for HSPF by year
        if (temphspf < 0.9f) {

                    // if (temphspf < 0.9f) {
          //   if (year <= 1970.f)
          //     temphspf = 5.0f;
          //   else if (year < 2008)
          //     temphspf = 5.5f + 2.2f / 32.f * (year - 1976.f);
          //   else
          //     temphspf = 7.7f;
          // }

          temphspf = standard_central_heatpump_hspf(year);
          adjust_hspf(cwd->winter_hp_design_temp, &temphspf);
        } else {
          adjust_hspf(cwd->winter_hp_design_temp, &temphspf);
        }
        ndi->htg[i].delivered_eff = temphspf / 3.413f;
      }

      else if (ndi->htg[i].fuel_type == ELECTRIC) {
        // tempeff = 1.0f;
        if (ndi->htg[i].location == HEATED)
          ndi->htg[i].delivered_eff = 1.0f;
        else
          ndi->htg[i].delivered_eff = SEASONAL_TO_SS_RATIO;
      } 
      else {
        ndi->htg[i].delivered_eff = tempeff * SEASONAL_TO_SS_RATIO;
      }
      if (cmds.debug_level & D_NORMAL) {
        fprintf(stderr, "\nDelivered heating efficiency: %0.3f was used for heating system: %s", ndi->htg[i].delivered_eff, ndi->htg[i].code);
      }
    }
  }

  // Allow ducts only on system 0 for present
  nir->primary_heating_system_seasonal_efficiency_pre_ductseal = ndi->htg[PRIMARY].delivered_eff * ndi->inf.pre_duct_seal_efficiency;
  // Continute use of above term until allow ducts on multiple systems
  ndi->htg[PRIMARY].delivered_eff *= ndi->inf.post_duct_seal_efficiency;
  // Adjust from equipment to delivered Eff

  nir->sysht_seaseff                                   = ndi->htg[PRIMARY].percent_heat_supplied / ndi->htg[PRIMARY].delivered_eff;
  nir->heating_system_seasonal_efficiency_pre_ductseal = ndi->htg[PRIMARY].percent_heat_supplied / nir->primary_heating_system_seasonal_efficiency_pre_ductseal;
  for (int i = 1; i < ndi->num_htg; i++) {
    nir->sysht_seaseff                                   += ndi->htg[i].percent_heat_supplied / ndi->htg[i].delivered_eff;
    nir->heating_system_seasonal_efficiency_pre_ductseal += ndi->htg[i].percent_heat_supplied / ndi->htg[i].delivered_eff;
  }
  nir->sysht_seaseff = 1.0f / nir->sysht_seaseff;
  nir->heating_system_seasonal_efficiency_pre_ductseal = 1.0f / nir->heating_system_seasonal_efficiency_pre_ductseal;

  if (cmds.debug_level & D_NORMAL) {
    fprintf(stderr, "\nAggregate heating delivered efficiency: %0.3f", nir->sysht_seaseff);
  }

  // Determine the seer of each A/C, the composite A/C seer, and the
  // fraction of cooling load handled by each conditioner
  // Assume the same duct delivery efficiency for cooling as for heating

  ndi->clgs.avg_seer = 0.0;
  nir->cooling_seer_pre_ductseal = 0.0;
  float totareaac = 0.0f;

  for (int nc = 0; nc < ndi->num_clg; nc++) {

    totareaac += ndi->clg[nc].area_cooled;

    // TODO resolve this code which was removed MJF 12/18
    // Version 9 inputs an efficiency and efficiency units instead of seer.
    // Make appropriate conversion.

    // if (ndi->clg[nc].efficiency_units == COP)
    //   ndi->clg[nc].seer = ndi->clg[nc].efficiency * 3.413f; // Convert COP to EER

    // else if (ndi->clg[nc].efficiency_units == EER || ndi->clg[nc].efficiency_units == COP)
    //   ndi->clg[nc].seer = 1.05f * ndi->clg[nc].seer - 0.3f; // Convert EER to SEER
    // 
    //  using average of conversion for continuous and intermittent fan operation.

    if (ndi->clg[nc].year_manufactured == 0.0f && 
      ndi->clg[nc].seer <= 0.0f &&
      ndi->clg[nc].system_type != CET_EVAPORATIVE) {    // Added with Issue #138, another code change from MBG for 8910
      ndi->clg[nc].seer = DEFAULT_COOLING_SEER;
      continue;
    }
    if (ndi->clg[nc].system_type == CET_EVAPORATIVE) {
      ndi->clg[nc].seer = nir->evaporative_cooler_eer;
      continue;

    } // Evaporative coolors

    // #241 use lookkup table of efficiency standards for cooling systems by year of manufacture
    // if seer is not provided directly.  NEAT does not gather eer directly on cooling form as of 12/20

    if (ndi->clg[nc].seer < 0.9f) {
      switch (ndi->clg[nc].system_type) {
      case CET_CENTRAL:
        ndi->clg[nc].seer = standard_central_ac_seer(ndi->clg[nc].year_manufactured);
        break;
      case CET_HEATPUMP:
        ndi->clg[nc].seer = standard_central_heatpump_seer(ndi->clg[nc].year_manufactured);
      break;
      case CET_WINDOW:
        ndi->clg[nc].seer = standard_room_heat_pump_seer(ndi->clg[nc].year_manufactured);
        // we do not adjust the standard window unit seer values
      default:
        ndi->clg[nc].seer = DEFAULT_COOLING_SEER;
      break;
      }
    }

    switch (ndi->clg[nc].system_type) {
    case CET_CENTRAL:
    case CET_HEATPUMP:
      adjust_seer(cwd->summer_hp_design_temp, &ndi->clg[nc].seer);
      break;
    default:
      // no adjustment
      break;
    }

    if (cmds.debug_level & D_NORMAL) {
      fprintf(stderr, "\nSEER: %0.3f was used for cooling system: %s", ndi->clg[nc].seer, ndi->clg[nc].code);
    }

    // if (ndi->clg[nc].system_type == CET_CENTRAL || ndi->clg[nc].system_type == CET_HEATPUMP) {
    //   if (ndi->clg[nc].seer < 0.9f) {

    //     if (ndi->clg[nc].year_manufactured <= 1970)
    //       ndi->clg[nc].seer = 6.0f;
    //     else if (ndi->clg[nc].year_manufactured < 1997)
    //       ndi->clg[nc].seer = 9.5f + 2.5f / 14.f * (ndi->clg[nc].year_manufactured - 1990.f);
    //     else if (ndi->clg[nc].year_manufactured < 2003)
    //       ndi->clg[nc].seer = 10.75f + 0.45f / 6.f * (ndi->clg[nc].year_manufactured - 1997.f);
    //     else if (ndi->clg[nc].year_manufactured < 2008)
    //       ndi->clg[nc].seer = 11.20f + 1.80f / 5.f * (ndi->clg[nc].year_manufactured - 2003.f);
    //     else
    //       ndi->clg[nc].seer = 13.0f; // standards discusion at https://en.m.wikipedia.org/wiki/Seasonal_energy_efficiency_ratio
    //   }

    //   // Adjust SEER for climate
    //   adjust_seer(cwd->summer_hp_design_temp, &ndi->clg[nc].seer);
    // }

    // // Unit is Window AC with no specified SEER

    // else if (ndi->clg[nc].system_type == CET_WINDOW && ndi->clg[nc].seer < 0.9f) {

    //   if (ndi->clg[nc].year_manufactured <= 1972)
    //     ndi->clg[nc].seer = 6.0f;
    //   else if (ndi->clg[nc].year_manufactured < 1995)
    //     ndi->clg[nc].seer = 6.0f + 3.0f / 23.0f * (ndi->clg[nc].year_manufactured - 1972.f);
    //   else if (ndi->clg[nc].year_manufactured < 1999)
    //     ndi->clg[nc].seer = 9.0f;
    //   else if (ndi->clg[nc].year_manufactured < 2002)
    //     ndi->clg[nc].seer = 9.0f + 0.75f / 3.0f * (ndi->clg[nc].year_manufactured - 1999.f);
    //   else
    //     ndi->clg[nc].seer = 9.75;

    //   // For window units, convert EER to an SEER.

    //   ndi->clg[nc].seer = 0.9f * ndi->clg[nc].seer + 0.1f; // Fan runs continuously
    //   //   ndi->clg[nc].seer = 1.2f*ndi->clg[nc].seer - 0.7f;    Fan runs only when cooling
    // }

  }

  for (int nc = 0; nc < ndi->num_clg; nc++) {

    ASSERT(totareaac, sprintf(msg, "Need total area cooled non zero"));
    ASSERT(ndi->gnl.floor_area, sprintf(msg, "Need non zero total floor area"));
    ASSERT(totareaac <= ndi->gnl.floor_area, sprintf(msg, "Area cooled needs to be less than or equal to the home floor area"));

    ASSERT(ndi->clg[nc].seer, sprintf(msg, "Need non zero eff"));

    ndi->clg[nc].area_cooled_ratio = ndi->clg[nc].area_cooled / totareaac;
    ndi->clgs.fraction_cooled = totareaac / ndi->gnl.floor_area;
    ASSERT(ndi->clgs.fraction_cooled <= 1.0f,
           sprintf(msg, "Total area cooled %f exceeds 100 percent", ndi->clgs.fraction_cooled));
    if (ndi->clg[nc].system_type == CET_CENTRAL) {
      nir->cooling_seer_pre_ductseal += ndi->clg[nc].area_cooled_ratio / (ndi->clg[nc].seer * ndi->inf.pre_duct_seal_efficiency);
      ndi->clgs.avg_seer += ndi->clg[nc].area_cooled_ratio / (ndi->clg[nc].seer * ndi->inf.post_duct_seal_efficiency);
    } else {
      nir->cooling_seer_pre_ductseal += ndi->clg[nc].area_cooled_ratio / ndi->clg[nc].seer;
      ndi->clgs.avg_seer += ndi->clg[nc].area_cooled_ratio / ndi->clg[nc].seer;
    }
  }

  if (ndi->num_clg != 0) {
    ASSERT(ndi->clgs.avg_seer, sprintf(msg, "Need non zero average seer"));
    nir->cooling_seer_pre_ductseal = 1.0f / nir->cooling_seer_pre_ductseal;
    ndi->clgs.avg_seer = 1.0f / ndi->clgs.avg_seer;
  } 

  // impute_cooling always NO and the key.imput_cooling_seer removed MJF #71
  // else if (ndi->gnl.impute_cooling == YES) { /* No A/C-impute cooling savings */
  //   ndi->clgs.avg_seer = ndi->key.impute_cooling_seer;
  //   ndi->clgs.fraction_cooled = 1.0;
  // }

  if (ndi->clgs.avg_seer < 1.0f)
    ndi->clgs.avg_seer = DEFAULT_COOLING_SEER;
  if (nir->cooling_seer_pre_ductseal < 1.0f)
    nir->cooling_seer_pre_ductseal = DEFAULT_COOLING_SEER;

  if (cmds.debug_level & D_NORMAL) {
    fprintf(stderr, "\nAggregate cooling SEER: %7.3f", ndi->clgs.avg_seer);
  }

  neat_energy_use("BASE CASE", PRE_RETROFIT);

  if (ndi->htg[PRIMARY].retrofit_option == ES_TUNEUP_PERFORMED_EVAL_REP || ndi->htg[PRIMARY].retrofit_option == ES_STDEFF_REP_REQUIRED ||
      ndi->htg[PRIMARY].retrofit_option == ES_HIEFF_REP_REQUIRED) {
    //nir->implement[N_CMS_FURNACE_TUNE_UP] = FALSE;
    ndi->cms[N_CMS_FURNACE_TUNE_UP].active = FALSE;

  }

  if (ndi->htg[PRIMARY].retrofit_option == ES_STDEFF_REP_REQUIRED) {
    //nir->implement[N_CMS_HIGH_EFFICIENCY_BOILER] = FALSE;
    ndi->cms[N_CMS_HIGH_EFFICIENCY_BOILER].active = FALSE;
    //nir->implement[N_CMS_HIGH_EFFICIENCY_FURNACE] = FALSE;
    ndi->cms[N_CMS_HIGH_EFFICIENCY_FURNACE].active = FALSE;
  }
  // Turn off hi-eff replacement measures

  //  Save base case parameters

  savebase();

billing_adjust_loop_back:     //  <<<<<<<<<<<<<<=======================   loop back point for billing adjustment

  nir->heatload[LAST_GOOD_MEASURE] = nir->heatload[PRE_RETROFIT_POST_INFIL];
  nir->heatengy[LAST_GOOD_MEASURE] = nir->heatengy[PRE_RETROFIT_POST_INFIL];
  nir->coolload[LAST_GOOD_MEASURE] = nir->coolload[PRE_RETROFIT_POST_INFIL];
  nir->coolengy[LAST_GOOD_MEASURE] = nir->coolengy[PRE_RETROFIT_POST_INFIL];

  //  Restore base case parameters if adjusting for fuel billing analysis

  if (nir->adjflg > 0) {
    if (cmds.debug_level & D_NORMAL)
      fprintf(stderr, "\n\n **** BILLING DATA ADJUSTMENTS MADE **** \n\n");
    for (int i = 0; i < N_MAX_RMC; i++){
      ndi->rmc[i].quant = 0.0;
    }
    for (int i = 0; i < MAXECMS; i++) {
      for (int m = 0; m < MONTHS + 1; m++) {
        nir->dfreht[i][m] = 0.;
        nir->dua[i][m] = 0.0f;
      }
      nir->measure_priority[i] = MPS_SIR;
      nir->associated_winner_ecm_index[i] = NOT_APPLICABLE;
    }

    // make a clean sweep of the ecm and ecmm structures, MJF 5/06
    memset(nir->ecm, 0, MAXECMS * sizeof(struct measure));
    memset(nir->ecmm, 0, 4 * MAXECMS * sizeof(struct measure_material));

    restorebase();
  }

  // Apply measures individually without interaction

  first_pass_measures();     // FIRST PASS measures   <<<<<<<<=============

  measure_diag_print_all("Un-interacted values immediately after call to first_pass_measures()", UNSORTED);

  heating_system_priority_adjustments();

  measure_diag_print_all("Un-interacted values immediately after priority adjustment to first_pass_measures()", UNSORTED);

  // Rank measures according to decreasing B/C

  int save_priority[MAXECMS];
  for (int i = 0; i < nir->nms; i++) {
    nir->sorted_measure_index[i] = i;         // goes into sort in unsorted state
    save_priority[i] = nir->measure_priority[i];     // save value so we can restore later.
    nir->measure_priority[i] = MPS_SIR;       // primary sort index, SIR basis only
    nir->measure_sir[i] = nir->ecm[i].sir;    // secondary sort
    //nir->measure_priority[i] = abs(nir->measure_priority[i]);
  }

  sort_neat_package_measures(nir->measure_sir, nir->nms, nir->sorted_measure_index);    // FIRST pass package sorting

  for (int i = 0; i < nir->nms; i++) {
    nir->index_by_sir[i] = nir->sorted_measure_index[i];      // passed array now sorted
    nir->measure_priority[i] = save_priority[i];                     // recover from saved values, for next sort
  }

  measure_diag_print_all("Un-interacted values after FIRST pass ranking of measures", SORTED);

  cumulative_interactive_effects();   // SECOND PACKAGE CONSTRUCTION PASS through the measures in decreasing SIR order

  prevent_sill_insulation_dropout();

  recombine_window_measures();

  // Take off trialing comma from component list for measures.

  for (int i = 0; i < nir->nms; i++) {
    char *fpntr;
    fpntr = strrchr(nir->ecm[i].components, COMMA_CHAR);
    if (fpntr)
      *fpntr = '\0';
  }

  //  Perform sizing calculations (Manual J) on post-retrofit house 

  sizing_heating(POST_RETROFIT);
  sizing_cooling(POST_RETROFIT);

  measure_diag_print_all("Measures list BEFORE sorting SECOND pass interacted measures, but after recombining window measures", SORTED);

  // #170 Ranking adjustments
  for (int j = 0; j < nir->nms; j++) {
    if ((nir->measure_required[j] == YES && nir->measure_priority[j] > MPS_SIR) ||
         nir->ecm[j].cms_measure_num == N_CMS_DUCT_SEALING || 
         nir->ecm[j].cms_measure_num == N_CMS_INFILTRATION_REDUCTION)
      nir->measure_priority[j] = MPS_SIR;
  }

  for (int j = 0; j < nir->nms; j++) {
    nir->measure_sir[j] = nir->ecm[j].sir;
    nir->sorted_measure_index[j] = j;
    save_priority[j] = nir->measure_priority[j];     // save for later recover
  } 

  // FINAL Sorting
  sort_neat_package_measures(nir->measure_sir, nir->nms, nir->sorted_measure_index);

  for (int j = 0; j < nir->nms; j++) {
    nir->ecm[j].sir = nir->measure_sir[j];
    nir->index_by_sir[j] = nir->sorted_measure_index[j];
    nir->measure_priority[j] = save_priority[j];     // recover value from prior to first pass
  } 

  measure_diag_print_all("Interacted Measures list after SECOND pass sorting", SORTED);

  // open our [optional] output measure report file

  if (strcmp(cmds.neat_measure_file_path, NO_OUTPUT) != 0) {
    measfile = fopen(cmds.neat_measure_file_path, "w");
    ASSERT(measfile, sprintf(msg, "Failed to open the measure report file: %s code:%d:%s", cmds.neat_measure_file_path, errno, strerror(errno)));
  } else {
    measfile = NULL;
  }

  // Print measures data in order of decreasing B/C following interaction

  if (nir->adjflg == 0) {

    // #14 some extra messages regarding blower door driven infiltration reduction if applicable
    if (nir->infiltration_treatment == INF_FULL_MEASURE) {
      add_neat_message("NEAT assumes that infiltration reduction will be performed in parallel to measures "
                       "selected by the audit and according to guidelines chosen by the auditor.  NEAT can "
                       "evaluate the cost-effectiveness of infiltration reduction efforts, but it will not direct the work.");
      add_neat_message("The audit strongly suggests, but does not necessarily require, the use of existing "
                       "infiltration reduction procedures using a blower-door. The blower-door establishes if "
                       "infiltration reduction is necessary, then helps locate leaks and monitor progress in their elimination.");
    }

    report_header(measfile, nir->adjflg);
    neat_results(measfile);
    report_materials(measfile);
    report_energy(measfile);
    report_measure_materials(); // fill in our cost detail materials
  }

  //  Manage billing computations for autoexecute
  //  Notice the branch back for post billing adjustment calculations

  if (nir->adjflg == 0) {
    manage_neat_billing_adjustments(&nir->adjflg);
    if (nir->adjflg > 0)
      goto billing_adjust_loop_back;
  }

  // Print results after final billing adjustment

  if (nir->adjflg > 0) {

    if (nor->num_measure > 0)
      nor->num_measure = 0; // throw away unadjusted results */
    if (nor->num_measure_material > 0)
      nor->num_measure_material = 0; // throw away unadjusted results

    // make sure we really toss the whole structure by clearing
    // the unadjusted measure results, MJF 5/06

    memset(nor->measure, 0, MAXECMS * sizeof(NEAT_MEASURE));
    memset(nor->mmaterial, 0, 4 * MAXECMS * sizeof(NEAT_MEASURE_MATERIAL));

    if (measfile)
      fprintf(measfile, "\n\n\n");

    report_header(measfile, nir->adjflg);
    neat_results(measfile);
    report_materials(measfile);
    report_energy(measfile);
    report_measure_materials(); // fill in our cost detail materials
  }

  nor->num_used_fuel = used_fuel_results(nor->used_fuel); // show the details for the fuels used

  if (measfile)
    fclose(measfile); // - Eliminates null pointer error on exit, see 7/27/94

  // Print *.dat file with billing comparison and sizing results.

  // open our [optional]output measure report file

  if (strcmp(cmds.neat_compare_file_path, NO_OUTPUT) != 0) {
    comparefile = fopen(cmds.neat_compare_file_path, "w");
    ASSERT(comparefile, sprintf(msg, "Failed to open the compare report file: %s code:%d:%s", cmds.neat_compare_file_path, errno, strerror(errno)));
  } else {
    comparefile = NULL;
  }

  report_header(comparefile, 0);
  if (nir->billing_record_count[PRE_HEATING] * nir->blapflg[PRE_HEATING] > 0) {
    int iunit;
    if (nir->bill_consumption_units[PRE_HEATING] == BILL_UNITS_MISSING) {
      if (ndi->htg[PRIMARY].fuel_type != ELECTRIC)
        iunit = BILL_UNITS_THERMS;
      else
        iunit = BILL_UNITS_KWH;
    } else
      iunit = nir->bill_consumption_units[PRE_HEATING];
    neat_billing_adjustment(nir->bladj, PRE_HEATING, iunit, FIRST_BILL_ADJUST, nir->blapflg, comparefile);
    if (comparefile)
      fprintf(comparefile, "\n\n");
  }
  if (nir->billing_record_count[PRE_COOLING] * nir->blapflg[PRE_COOLING] > 0) {
    neat_billing_adjustment(nir->bladj, PRE_COOLING, BILL_UNITS_KWH, FIRST_BILL_ADJUST, nir->blapflg, comparefile);
    if (comparefile)
      fprintf(comparefile, "\n\n");
  }

  report_manual_j(comparefile);

  if (comparefile)
    fclose(comparefile);

  return; // all done, success
}

// Recombine measures of the same window treatment but on different components
static void recombine_window_measures() {

  int window_measures[] = {
    N_CMS_STORM_WINDOWS,
    N_CMS_LOW_E_WINDOWS,
    N_CMS_WINDOW_SEALING,
    N_CMS_WINDOW_REPLACEMENT
  };
  int window_measure_count = sizeof(window_measures)/sizeof(window_measures[0]);
  int all_win_measure_count = window_measure_count * 3;    // three MPS groups for all window measures
  int win_ecm_index[all_win_measure_count];

  for (int i = 0; i < all_win_measure_count; i++)
    win_ecm_index[i] = NOT_APPLICABLE;

  for (int jl = 0; jl < nir->nms; jl++) {
    int il = nir->index_by_sir[jl];
    int cms_measure_num = nir->ecm[il].cms_measure_num;

    if ((nir-> measure_priority[il] == MPS_SIR && nir->ecm[il].sir >= ndi->key.minimum_acceptable_sir) ||
      nir-> measure_priority[il] == MPS_ENVELOPE_REQUIRED ||
      nir-> measure_priority[il] == MPS_ENVELOPE_REQUIRED_NO_SIR) {

      for (int i = 0; i < window_measure_count; i++) {          // Cycle through the window treatment measures
        if (cms_measure_num == window_measures[i]) {
          int j;
          if (nir->measure_priority[il] == MPS_ENVELOPE_REQUIRED)
            j = i + window_measure_count;
          else if (nir->measure_priority[il] == MPS_ENVELOPE_REQUIRED_NO_SIR)
            j = i + (window_measure_count * 2);
          else
            j = i;

          if (win_ecm_index[j] < 0)
            win_ecm_index[j] = il;
          else {
            int ecm_index = win_ecm_index[j];
            nir->clengysav[ecm_index] += nir->clengysav[il];
            nir->clengysav[il] = 0.0f;

            nir->htengysav[ecm_index] += nir->htengysav[il];
            nir->htengysav[il] = 0.0f;

            nir->ecm[ecm_index].ensav += nir->ecm[il].ensav;
            nir->ecm[il].ensav = 0.0f;

            nir->cldlsav[ecm_index] += nir->cldlsav[il];
            nir->cldlsav[il] = 0.0f;

            nir->htdlsav[ecm_index] += nir->htdlsav[il];
            nir->htdlsav[il] = 0.0f;

            nir->ecm[ecm_index].dlsav += nir->ecm[il].dlsav;
            nir->ecm[il].dlsav = 0.0f;

            nir->ecm[ecm_index].cost += nir->ecm[il].cost;
            nir->ecm[il].cost = 0.0f;

            nir->dslfsav[ecm_index] += nir->dslfsav[il];
            nir->dslfsav[il] = 0.0f;

            nir->npv[ecm_index] += nir->npv[il];
            nir->npv[il] = 0.0f;

            nir->ecm[il].sir = 0.0f;

            nir->measure_priority[il] = MPS_NPV;
            nir->measure_required[il] = FALSE;

            STRCAT(nir->ecm[ecm_index].components, nir->ecm[il].components);

            //  Add parameter needed for the measures tab of audit

            nir->ecm[ecm_index].qtym += nir->ecm[il].qtym;
            nir->ecm[il].qtym = 0.0f;
            nir->ecm[ecm_index].qtyl += nir->ecm[il].qtyl;
            nir->ecm[il].qtyl = 0.0f;
            nir->ecm[ecm_index].qtyi += nir->ecm[il].qtyi;
            nir->ecm[il].qtyi = 0.0f;
            nir->ecm[ecm_index].costi2 += nir->ecm[il].costi2;
            nir->ecm[il].costi2 = 0.0f;
          }
        }
      }
    }
  }
  for (int i = 0; i < all_win_measure_count; i++) { // Cycle through the window treatment measures
    if (nir->ecm[win_ecm_index[i]].cost > 0.0f)
      nir->ecm[win_ecm_index[i]].sir = nir->dslfsav[win_ecm_index[i]] / nir->ecm[win_ecm_index[i]].cost;
  }
  return;
}


//This routine performs the whole building energy calculation, given the component descriptions.

void neat_energy_use(char *run_title, int phase) {

  float tdb, tsa; 
  float areaattic;
  int largest_attic;
  float temp, uaflr;
  float uwb, ug, areabg, areaag, tia;
  float heattemp, cooltemp, tempsa;
  float fSolarStorage;
  float uvalcl;
  int energy_array_index;

  switch(phase){
    case PRE_RETROFIT:
      energy_array_index = PRE_RETROFIT_POST_INFIL;
      break;
    case POST_RETROFIT:
      energy_array_index = CURRENT_MEASURE;
      break;
    default:
      ASSERT(FALSE, sprintf(msg, "Unrecognized phase:%d in energy calc call", phase));
  }

  nor->energy_calc_counter++; // gitlab #47

  if (cmds.debug_level & D_NEAT_ENERGY_DETAIL_ALL || (phase == PRE_RETROFIT && (cmds.debug_level & D_NEAT_ENERGY_DETAIL_BASE))) {
    fprintf(stderr, "\n\n------------------START of NEAT_ENERGY_USE:%02d %s -------------------\n", nor->energy_calc_counter, run_title);
  }

  nir->ua_total = 0.0;
  nir->ua_walls = 0.0;
  nir->ua_windows = 0.0;
  nir->ua_doors = 0.0;
  nir->ua_attics = 0.0;
  nir->ua_foundations = 0.0;

  for (int i = SOLAR_NORTH; i < SOLAR_DIFFUSE; i++){
    nir->solar_aperture_direct[HEATING][i] = 0.0;
    nir->solar_aperture_direct[COOLING][i] = 0.0;
    nir->solar_aperture_diffuse[HEATING][i] = 0.0;
    nir->solar_aperture_diffuse[COOLING][i] = 0.0;
  }

  for (int m = 0; m <= MONTHS; m++)
    nir->totsol[m] = 0.;

  ASSERT(ndi->key.annual_outside_film_coeff, sprintf(msg, "Need non zero outside film coeff"));

  // Walls
  for (int nc = 0; nc < ndi->num_wal; nc++) {
    int orientation = ndi->wal[nc].solar_orient;
    // #314
    // if (ndi->wal[nc].exposure == EX_ATTIC) {
    //   continue; // kneewalls handled as ceiling
    // }
    ndi->wal[nc].ua_value = (ndi->wal[nc].u_frame * FRAMING_FACTOR_WALL + ndi->wal[nc].u_cavity * (1.0f - FRAMING_FACTOR_WALL)) * ndi->wal[nc].area;
  
    if (ndi->wal[nc].exposure == EX_BUFFERED) {
      ndi->wal[nc].ua_value *= BUFFERED_TO_EXPOSED_WALL_DT_RATIO;
    } else {
      nir->solar_aperture_direct[HEATING][orientation] += ndi->wal[nc].ua_value * WALL_ABSORPTIVITY / ndi->key.annual_outside_film_coeff;
      nir->solar_aperture_diffuse[HEATING][orientation] = nir->solar_aperture_direct[HEATING][orientation];

      nir->solar_aperture_direct[COOLING][orientation] += ndi->wal[nc].ua_value * WALL_ABSORPTIVITY / ndi->key.annual_outside_film_coeff;
      nir->solar_aperture_diffuse[COOLING][orientation] = nir->solar_aperture_direct[COOLING][orientation];
    }

    nir->ua_walls += ndi->wal[nc].ua_value;
    if (cmds.debug_level & D_NEAT_ENERGY_DETAIL_ALL || (phase == PRE_RETROFIT && (cmds.debug_level & D_NEAT_ENERGY_DETAIL_BASE))) {
      fprintf(stderr, "\nWall[%02d]       UA: %7.3f %s", nc, ndi->wal[nc].ua_value, ndi->wal[nc].code);
    }

  }
  nir->ua_total += nir->ua_walls;
  if (cmds.debug_level & D_NEAT_ENERGY_DETAIL_ALL || (phase == PRE_RETROFIT && (cmds.debug_level & D_NEAT_ENERGY_DETAIL_BASE))) 
    fprintf(stderr, "\nWall TOT       UA: %7.3f", nir->ua_walls);

  // Windows
  for (int nc = 0; nc < ndi->num_win; nc++) {
    int orientation = ndi->win[nc].solar_orient;
    float ua_window = ndi->win[nc].u_value * ndi->win[nc].area_gross;
    nir->ua_windows += ua_window;
    if (cmds.debug_level & D_NEAT_ENERGY_DETAIL_ALL || (phase == PRE_RETROFIT && (cmds.debug_level & D_NEAT_ENERGY_DETAIL_BASE))) {
      fprintf(stderr, "\nWindow[%02d]     UA: %7.3f %s",nc , ua_window, ndi->win[nc].code);
    }

    // All window areas changed to be gross window areas 2/09
    // Added wn_sunscrn factor 3/28/11 to correct handling of sunscreen measures

    nir->solar_aperture_diffuse[HEATING][orientation] += ndi->win[nc].shgc_winter * ndi->win[nc].area_gross * nir->wn_sunscrn[HEATING][nc];
    nir->solar_aperture_diffuse[COOLING][orientation] += ndi->win[nc].shgc_summer * ndi->win[nc].area_gross * nir->wn_sunscrn[COOLING][nc];

    nir->solar_aperture_direct[HEATING][orientation] += ndi->win[nc].shgc_winter * ndi->win[nc].shade_factor_winter * ndi->win[nc].area_gross * nir->wn_sunscrn[HEATING][nc];
    nir->solar_aperture_direct[COOLING][orientation] += ndi->win[nc].shgc_summer * ndi->win[nc].shade_factor_summer * ndi->win[nc].area_gross * nir->wn_sunscrn[COOLING][nc];
  }
  nir->ua_total += nir->ua_windows;
  if (cmds.debug_level & D_NEAT_ENERGY_DETAIL_ALL || (phase == PRE_RETROFIT && (cmds.debug_level & D_NEAT_ENERGY_DETAIL_BASE))) 
    fprintf(stderr, "\nWindow TOT     UA: %7.3f", nir->ua_windows);

  // Doors
  for (int nc = 0; nc < ndi->num_dor; nc++) {
    float ua_door = ndi->dor[nc].u_value * ndi->dor[nc].area;
    nir->ua_doors += ua_door;
    if (cmds.debug_level & D_NEAT_ENERGY_DETAIL_ALL || (phase == PRE_RETROFIT && (cmds.debug_level & D_NEAT_ENERGY_DETAIL_BASE))) {
      fprintf(stderr, "\nDoor[%02d]       UA: %7.3f %s",nc , ua_door, ndi->dor[nc].code);
    }

    float temp_solar_aperature = ndi->dor[nc].u_value * ndi->dor[nc].area * WALL_ABSORPTIVITY / ndi->key.annual_outside_film_coeff;
    nir->solar_aperture_direct[HEATING][ndi->dor[nc].solar_orient] += temp_solar_aperature;
    nir->solar_aperture_direct[COOLING][ndi->dor[nc].solar_orient] += temp_solar_aperature;
    nir->solar_aperture_diffuse[HEATING][ndi->dor[nc].solar_orient] += temp_solar_aperature;
    nir->solar_aperture_diffuse[COOLING][ndi->dor[nc].solar_orient] += temp_solar_aperature;
  }
  nir->ua_total += nir->ua_doors;
  if (cmds.debug_level & D_NEAT_ENERGY_DETAIL_ALL || (phase == PRE_RETROFIT && (cmds.debug_level & D_NEAT_ENERGY_DETAIL_BASE))) 
    fprintf(stderr, "\nDoor TOT       UA: %7.3f", nir->ua_doors);

  /* Determine which attic, if any, will have ducts assumed present */
  /* Use the largest attic space not listed as cathedral, else assume ambient */

  largest_attic = NOT_APPLICABLE;
  areaattic = 0.0;
  for (int nc = 0; nc < ndi->num_uas; nc++) {
    if (ndi->uas[nc].attic_type == UAS_FLOORED || 
        ndi->uas[nc].attic_type == UAS_UNFLOORED ||
        ndi->uas[nc].attic_type == UAS_CEILING_JOIST) {
      if (ndi->uas[nc].area > areaattic) {
        largest_attic = nc;
        areaattic = ndi->uas[nc].area;
      }
    }
  }

  // Attics
  for (int nc = 0; nc < ndi->num_uas; nc++) {
    float rcceilc, rcinfc, rcroofc, rcsumc, rcgable;
    float farc1, farc2, uaknj;

    // Gable heat loss
    rcgable = attic_rcgable(nc);

    if (ndi->uas[nc].attic_type == UAS_KNEEWALL) {
      uaknj = KNEEWALL_JOIST_U_VALUE * 3.0f * ndi->uas[nc].area;
      uvalcl = FRAMING_FACTOR_WALL * ndi->uas[nc].joist_u_value + (1.0f - FRAMING_FACTOR_WALL) * ndi->uas[nc].u_value;
    } else {
      uaknj = 0.0;
      uvalcl = FRAMING_FACTOR_CEILING * ndi->uas[nc].joist_u_value + (1.0f - FRAMING_FACTOR_CEILING) * ndi->uas[nc].u_value;
    }

    // roof surface area to attic floor area ratios
    farc1 = 1.0f;
    farc2 = STICK_BUILT_ROOF_TO_ATTIC_AREA_RATIO;
    if (ndi->uas[nc].attic_type == UAS_KNEEWALL)
      farc2 = 3.1623f;
    if (ndi->uas[nc].attic_type == UAS_CATHEDRAL || ndi->uas[nc].attic_type == UAS_ROOF_RAFTER)
      farc1 = STICK_BUILT_ROOF_TO_ATTIC_AREA_RATIO;
    rcceilc = uvalcl * ndi->uas[nc].area * farc1;

    rcroofc = ndi->uas[nc].frame_u_value * farc2 * ndi->uas[nc].area;
    rcinfc = RHOCAIR * ndi->uas[nc].ventilation_cuft_per_hr;
    rcsumc = rcceilc + rcroofc + rcinfc + uaknj + rcgable;

    ASSERT(rcsumc, sprintf(msg, "Need non zero r value"));
    ndi->uas[nc].ua_value = rcceilc * (rcinfc + rcroofc + rcgable) / rcsumc;
    nir->ua_attics += ndi->uas[nc].ua_value;
    if (cmds.debug_level & D_NEAT_ENERGY_DETAIL_ALL || (phase == PRE_RETROFIT && (cmds.debug_level & D_NEAT_ENERGY_DETAIL_BASE))) {
      fprintf(stderr, "\nAttic[%02d]:rcceilc: %7.3f %s", nc, rcceilc, ndi->uas[nc].code);
      fprintf(stderr, "\nAttic[%02d]:rcroofc: %7.3f", nc, rcroofc);
      fprintf(stderr, "\nAttic[%02d]: rcinfc: %7.3f", nc, rcinfc);
      fprintf(stderr, "\nAttic[%02d]:  uaknj: %7.3f", nc, uaknj);
      fprintf(stderr, "\nAttic[%02d]:rcgable: %7.3f", nc, rcgable);
      fprintf(stderr, "\nAttic[%02d]:     UA: %7.3f", nc, ndi->uas[nc].ua_value);
    }

    if (nc == largest_attic && phase == PRE_RETROFIT) {
      tia = (ndi->key.daytime_cooling_setpoint + ndi->key.nighttime_cooling_setpoint + ndi->key.daytime_heating_setpoint +
             ndi->key.nighttime_heating_setpoint) / 4.0f;
      for (int m = 1; m <= MONTHS; m++) {
        tdb = (cwd->avg_nighttime_temp[m] + cwd->avg_daytime_temp[m]) / 2.0f;
        tsa = tdb + ndi->uas[nc].roof_absorptance / ndi->key.annual_outside_film_coeff * cwd->solar_load[m][SOLAR_HORIZONTAL_TOTAL];
        nir->tattic[m] = (rcinfc * tdb + rcroofc * tsa + rcceilc * tia) / rcsumc;
      }
    }

    tempsa = rcceilc * rcroofc * ndi->uas[nc].roof_absorptance / ndi->key.annual_outside_film_coeff / rcsumc;
    nir->solar_aperture_direct[HEATING][ndi->uas[nc].solar_orient] += tempsa;
    nir->solar_aperture_direct[COOLING][ndi->uas[nc].solar_orient] += tempsa;

    nir->solar_aperture_diffuse[HEATING][ndi->uas[nc].solar_orient] += tempsa;
    nir->solar_aperture_diffuse[COOLING][ndi->uas[nc].solar_orient] += tempsa;
  }
  nir->ua_total += nir->ua_attics;
  if (cmds.debug_level & D_NEAT_ENERGY_DETAIL_ALL || (phase == PRE_RETROFIT && (cmds.debug_level & D_NEAT_ENERGY_DETAIL_BASE))) 
    fprintf(stderr, "\nAttic TOT      UA: %7.3f", nir->ua_attics);

  if (phase == PRE_RETROFIT) {
    if (cwd->avg_drybulb_temp_55 > 0) {
      nir->heat_dumped_to_unint_cond_space = 0.07f * (nir->ua_total + nir->whole_house_cfm[POST_RETROFIT][TOTAL] * 60.0f * RHOCAIR) * cwd->hdd65 * 24.0f * 0.7f / 8760 / cwd->avg_drybulb_temp_55;
    } else
      nir->heat_dumped_to_unint_cond_space = 0.0f;
  }

  //Foundation spaces

  // we only want to set tucsbsp to the outdoor air
  // temperature on the base case pass, 4/23/01 debug
  if (phase == PRE_RETROFIT)
    for (int m = 1; m <= MONTHS; m++)
      nir->tucsbsp[m] = cwd->avg_drybulb_temp[m];

  nir->dblcs = 0.0;

  for (int nc = 0; nc < ndi->num_fnd; nc++) {
    float uvalbg;
    float uvalag;
    double F2Slope[] = {4.18394e-5, 1.54311e-5};
    float F2Intcpt[] = {1.01058f, 0.45907f};
    float F2, flruval;
    float ua_vent = 0.0;      // UAv for ventilated foundation

    switch (ndi->fnd[nc].space_type) {
    case UNINSULATED_SLAB:
    case INSULATED_SLAB:

      if (ndi->fnd[nc].space_type == UNINSULATED_SLAB)
        F2 = (float)F2Slope[0] * cwd->hdd65 + F2Intcpt[0];
      else  // INSULATED_SLAB
        F2 = (float)F2Slope[1] * cwd->hdd65 + F2Intcpt[1];

      ASSERT(ndi->fnd[nc].area != 0, sprintf(msg, "Need non zero area, foundation: %s", ndi->fnd[nc].code));
      flruval = F2 * ndi->fnd[nc].perim_length / ndi->fnd[nc].area;
      ASSERT(ndi->fnd[nc].floor_u_value != 0, sprintf(msg, "Need non zero u value, foundation: %s", ndi->fnd[nc].code));
      ASSERT(flruval != 0, sprintf(msg, "Need non zero u value"));

      flruval = 1.0f / (1.0f / flruval + 1.0f / ndi->fnd[nc].floor_u_value);
      ndi->fnd[nc].ua_value_basement_effective = flruval * ndi->fnd[nc].area;

      break;

    case  EXPOSED_FLOOR_CLOSED:
    case  EXPOSED_FLOOR_UNCLOSED:
      ndi->fnd[nc].ua_value_basement_effective = ndi->fnd[nc].area * ndi->fnd[nc].floor_u_value;
      break;
      
    case VENTED_NON_CONTITIONED:
      ua_vent = RHOCAIR * FLOOR_CFM_PER_SQFT * ndi->fnd[nc].area * 60.0;
      // no break fall through

    default:    //  CONDITIONED, NON_CONDITIONED, VENTED_NON_CONTITIONED, UNINTENTIONALLY_CONDITIONED,

      if (ndi->fnd[nc].below_grade_wall_height < 1.e-2)
        uwb = ndi->fnd[nc].u_value_basement_wall_total = 0.;
      else {
        //float csoil  = 0.86f;   // (Btu/hr-ft-F) = value used by Shipp ASHRAE SP-38
        float csoil = 1.00f;
        temp = 1 + PI * ndi->fnd[nc].below_grade_wall_height / 2.0f / csoil * ndi->fnd[nc].below_grade_wall_u_value;
        ASSERT(ndi->fnd[nc].below_grade_wall_height, sprintf(msg, "Need non zero below_grade_wall_height, foundation: %s", ndi->fnd[nc].code));
        uwb = ndi->fnd[nc].u_value_basement_wall_total =
            2.0f * csoil / PI / ndi->fnd[nc].below_grade_wall_height * (float)log(temp);
      }
      // through the ground path u value
      ug = .038f;
      if (ndi->fnd[nc].wall_height < 5.)
        ug = .078f;

      ASSERT(ndi->fnd[nc].wall_height != 0, sprintf(msg, "Need non zero wall_height, foundation: %s", ndi->fnd[nc].code));
      areabg = ndi->fnd[nc].wall_area * ndi->fnd[nc].below_grade_wall_height / ndi->fnd[nc].wall_height;
      areaag = ndi->fnd[nc].wall_area - areabg;

      uvalbg = uwb;
      uvalag = ndi->fnd[nc].above_grade_wall_u_value;

      // #232
      // if (phase == PRE_RETROFIT) {  // recomputed with added_r in second pass
      //   ndi->fnd[nc].ua_value_basement_sill = foundation_sill_ua_value(nc);
      // }
      ndi->fnd[nc].ua_value_basement =
          (areaag * uvalag) + (areabg * uvalbg) + (ndi->fnd[nc].area * ug) + ndi->fnd[nc].ua_value_basement_sill;

      if (ndi->fnd[nc].space_type == CONDITIONED) {
        ndi->fnd[nc].ua_value_basement_effective = ndi->fnd[nc].ua_value_basement;
      } else {        // NON_CONDITIONED, VENTED_NON_CONTITIONED, UNINTENTIONALLY_CONDITIONED

        // Issue #224
        if (ndi->fnd[nc].space_type == UNINTENTIONALLY_CONDITIONED) {
          ndi->fnd[nc].equipment_waste_heat = nir->heat_dumped_to_unint_cond_space;
        } else {
          ndi->fnd[nc].equipment_waste_heat = 0.0;
        }

        uaflr = ndi->fnd[nc].area * ndi->fnd[nc].floor_u_value;

        // #230 added ua_vent term
        ASSERT((uaflr + ndi->fnd[nc].ua_value_basement), sprintf(msg, "Need non zero ua value, foundation: %s", ndi->fnd[nc].code));
        ndi->fnd[nc].ua_value_basement_effective = uaflr * (ndi->fnd[nc].ua_value_basement + ua_vent - ndi->fnd[nc].equipment_waste_heat) /
                                                   (uaflr + ndi->fnd[nc].ua_value_basement + ua_vent);

        // these foundation type effect the building load coefficient
        nir->dblcs += uaflr * ndi->fnd[nc].ua_value_basement / (uaflr + ndi->fnd[nc].ua_value_basement) -
                 ndi->fnd[nc].ua_value_basement_effective;
        // clang-format off
        if (ndi->fnds.subspace_with_ductwork != NOT_APPLICABLE &&
          nc == ndi->fnds.subspace_with_ductwork && 
          phase == PRE_RETROFIT) {
          tia = (ndi->key.daytime_heating_setpoint + ndi->key.nighttime_heating_setpoint) / 2.0f;
          for (int m = 1; m < MONTHS + 1; m++) {
            nir->tucsbsp[m] = (tia * (uaflr + ndi->fnd[nc].equipment_waste_heat) +
                          cwd->avg_drybulb_temp[m] * (ndi->fnd[nc].ua_value_basement - 
                          ndi->fnd[nc].equipment_waste_heat)) /
                         (uaflr + ndi->fnd[nc].ua_value_basement);
          }
        }
        // clang-format on
      }
      break;
    }

    //fprintf(stderr, "\nFoundation:%s UAeffective:%8.3f", ndi->fnd[nc].code, ndi->fnd[nc].ua_value_basement_effective);
    nir->ua_foundations += ndi->fnd[nc].ua_value_basement_effective;
    if (cmds.debug_level & D_NEAT_ENERGY_DETAIL_ALL || (phase == PRE_RETROFIT && (cmds.debug_level & D_NEAT_ENERGY_DETAIL_BASE))) 
      fprintf(stderr, "\nFoundation[%02d] UA: %7.3f",nc , ndi->fnd[nc].ua_value_basement_effective);

    //if (cmds.debug_level & D_NORMAL)
    //  fprintf(stderr, "\nnc:%d ndi->fnd[nc].ua_value_basement_effective:%.4g", nc, ndi->fnd[nc].ua_value_basement_effective);
  }
  nir->ua_total += nir->ua_foundations;
  if (cmds.debug_level & D_NEAT_ENERGY_DETAIL_ALL || (phase == PRE_RETROFIT && (cmds.debug_level & D_NEAT_ENERGY_DETAIL_BASE))) {
    fprintf(stderr, "\nFoundation TOT UA: %7.3f", nir->ua_foundations);
    fprintf(stderr, "\nTotal Cond     UA: %7.3f", nir->ua_total);
    fprintf(stderr, "\nEffective Inf. UA: %7.3f", nir->ua_infiltration);
    fprintf(stderr, "\nTOTAL Cond+Inf UA: %7.3f", nir->ua_total + nir->ua_infiltration);
    if (largest_attic != NOT_APPLICABLE)
      fprintf(stderr, "\nLargest Attic: %s %7.3f sqft", ndi->uas[largest_attic].code, ndi->uas[largest_attic].area);
  }

  // Compute monthly latent load from infiltration
  if (phase == PRE_RETROFIT) {
    for (int m = 1; m <= MONTHS; m++) {
      nir->latentload[PRE_RETROFIT][m] = get_latent_infil_load(cwd->avg_drybulb_temp[m], cwd->avg_wetbulb_temp[m], nir->whole_house_cfm[PRE_RETROFIT][m]) * 24.0f * cwd->days_in_month[m];
      nir->latentload[POST_RETROFIT][m] = get_latent_infil_load(cwd->avg_drybulb_temp[m], cwd->avg_wetbulb_temp[m], nir->whole_house_cfm[POST_RETROFIT][m]) * 24.0f * cwd->days_in_month[m];
    }
  }
  for (int m = 1; m <= MONTHS; m++) {
    nir->wn_lat_load[m] = get_latent_infil_load(cwd->avg_drybulb_temp[m], cwd->avg_wetbulb_temp[m], nir->wn_cfm_tot[m]) * 24.0f * cwd->days_in_month[m];
    nir->dr_lat_load[m] = get_latent_infil_load(cwd->avg_drybulb_temp[m], cwd->avg_wetbulb_temp[m], nir->dr_cfm_tot[m]) * 24.0f * cwd->days_in_month[m];
  }

  // Compute monthly building load coefficients

  for (int loop = PRE_RETROFIT; loop <= POST_RETROFIT; loop++) {

    //temp1 = temp2 = temp3 = 0.0f;
    //tempa1 = tempa2 = tempa3 = 0.0f;
    if (loop == PRE_RETROFIT && phase == POST_RETROFIT)
      continue; /* Compute pre-retrofit for base case only */

    nir->total_annual_free_heat = 0.0;
    nir->building_load_coeff[AVG] = 0.0;
    nir->totsol[TOTAL] = 0.0;
    nir->teffn[AVG] = 0.0;
    nir->heatload[energy_array_index] = 0.0;
    nir->coolload[energy_array_index] = 0.0;

    nir->balance_point_temp[NIGHT][HEATING][AVG] = 0.0;
    nir->balance_point_temp[DAY][HEATING][AVG] = 0.0;
    nir->balance_point_temp[NIGHT][COOLING][AVG] = 0.0;
    nir->balance_point_temp[DAY][COOLING][AVG] = 0.0;

    if (loop == PRE_RETROFIT && 
        (nir->infiltration_treatment == INF_NO_COST_COMPUTE_SAVINGS_ONLY ||
         nir->infiltration_treatment == INF_FULL_MEASURE)) {      // base case with pre-ret. infilt. specified
      for (int m = 1; m <= MONTHS; m++) {
        nir->building_load_coeff[m] = nir->ua_total + nir->whole_house_cfm[PRE_RETROFIT][m] * 60.0f * RHOCAIR; // Pre-retrofit
        nir->building_load_coeff[AVG] += nir->building_load_coeff[m];
      }
    } else {
      for (int m = 1; m <= MONTHS; m++) {
        //tempa1 += nir->whole_house_cfm[POST_RETROFIT][m] * 60.0f * RHOCAIR;
        //tempa2 += nir->wn_cfm_tot[m] * 60.0f * RHOCAIR;
        nir->building_load_coeff[m] = nir->ua_total + nir->whole_house_cfm[POST_RETROFIT][m] * 60.0f * RHOCAIR; // Post-retrofit

        if (nir->wn_cfm_tot[m] != nir->save_win_cfm_tot[m]) { // Retrofit has changed window leakage
          nir->building_load_coeff[m] -= (nir->save_win_cfm_tot[m] - nir->wn_cfm_tot[m]) * 60.0f * RHOCAIR;
        }

        if (nir->dr_cfm_tot[m] != nir->save_dor_cfm_tot[m]) { // Retrofit has changed door leakage
          nir->building_load_coeff[m] -= (nir->save_dor_cfm_tot[m] - nir->dr_cfm_tot[m]) * 60.0f * RHOCAIR;
        }

        nir->building_load_coeff[AVG] += nir->building_load_coeff[m];
      }
    }
    nir->building_load_coeff[AVG] /= 12.0;

    for (int m = 1; m <= MONTHS; m++) {
      nir->totsol[m] = 0.;

      for (int i = SOLAR_NORTH; i <= SOLAR_HORIZONTAL_TOTAL; i++) {
        if (m > MARCH && m < OCTOBER)
          nir->totsol[m] += cwd->solar_load[m][SOLAR_DIFFUSE] * nir->solar_aperture_diffuse[COOLING][i] + cwd->solar_load[m][i] * nir->solar_aperture_direct[COOLING][i];
        else
          nir->totsol[m] += cwd->solar_load[m][SOLAR_DIFFUSE] * nir->solar_aperture_diffuse[HEATING][i] + cwd->solar_load[m][i] * nir->solar_aperture_direct[HEATING][i];
      }

      /*******************************************************/
      /*  Solar Storage Factor  (unitless) (taken from MHEA  */
      /*******************************************************/

      neat_solar_storage(cwd->avg_daytime_temp[m], nir->totsol[m], ndi->key.base_free_heat_from_internals, 0.0, nir->building_load_coeff[m],
                       &fSolarStorage);

      // if (cmds.debug_level & D_NORMAL)
      //   fprintf(stderr, "\nm:%d fSolarStorage:%.4g totsol:%.4g ndi->key.base_free_heat_from_internals:%.4g", m, fSolarStorage,
      //           nir->totsol[m], ndi->key.base_free_heat_from_internals);

      nir->internal_gain_night[m] = 2.0f * fSolarStorage * nir->totsol[m] + ndi->key.base_free_heat_from_internals;
      nir->internal_gain_day[m] = 2.0f * (1.0f - fSolarStorage) * nir->totsol[m] + ndi->key.base_free_heat_from_internals;

      ASSERT(nir->building_load_coeff[m], sprintf(msg, "Need non zero factor"));
      ASSERT((nir->building_load_coeff[m] - nir->dblcs), sprintf(msg, "Need non zero factor"));

      nir->teffn[m] = cwd->avg_nighttime_temp[m] + nir->internal_gain_night[m] / nir->building_load_coeff[m];
      nir->teff[m] = cwd->avg_drybulb_temp[m] + nir->internal_gain_day[m] / nir->building_load_coeff[m];

      // if (cmds.debug_level & D_NORMAL)
      //   fprintf(stderr, "\nm:%d night_setback_temperature:%.4g freeheat:%.4g blc:%.4g", m, nir->night_setback_temperature[m], nir->internal_gain_night[m], nir->building_load_coeff[m]);

      nir->balance_point_temp[NIGHT][HEATING][m] = nir->night_setback_temperature[m] - nir->internal_gain_night[m] / nir->building_load_coeff[m];
      nir->balance_point_temp[DAY][HEATING][m] = ndi->key.daytime_heating_setpoint - nir->internal_gain_day[m] / nir->building_load_coeff[m];
      nir->balance_point_temp[NIGHT][COOLING][m] = ndi->key.nighttime_cooling_setpoint - nir->internal_gain_night[m] / (nir->building_load_coeff[m] - nir->dblcs);
      nir->balance_point_temp[DAY][COOLING][m] = ndi->key.daytime_cooling_setpoint - nir->internal_gain_day[m] / (nir->building_load_coeff[m] - nir->dblcs);
      nir->totsol[TOTAL] += nir->totsol[m]; 
      nir->total_annual_free_heat += nir->internal_gain_day[m];
      nir->teffn[AVG] += nir->teffn[m];
      nir->balance_point_temp[NIGHT][HEATING][AVG] += nir->balance_point_temp[NIGHT][HEATING][m];
      nir->balance_point_temp[DAY][HEATING][AVG] += nir->balance_point_temp[DAY][HEATING][m];
      nir->balance_point_temp[NIGHT][COOLING][AVG] += nir->balance_point_temp[NIGHT][COOLING][m];
      nir->balance_point_temp[DAY][COOLING][AVG] += nir->balance_point_temp[DAY][COOLING][m];
    }

    nir->balance_point_temp[NIGHT][HEATING][AVG] /= 12.0;
    nir->balance_point_temp[NIGHT][COOLING][AVG] /= 12.0;
    nir->balance_point_temp[DAY][HEATING][AVG] /= 12.0;
    nir->balance_point_temp[DAY][COOLING][AVG] /= 12.0;
    nir->teffn[AVG] /= 12.0;

    adjusted_monthly_degree_hours(nir->balance_point_temp, nir->degree_hours);

    // if (cmds.debug_level & D_NORMAL) {
    //   if (loop == PRE_RETROFIT)
    //     fprintf(stderr, "\n\n----PRE-RETROFIT-----");
    //   else
    //     fprintf(stderr, "\n\n----POST-RETROFIT-----");
    // }

    for (int m = 1; m <= MONTHS; m++) {
      heattemp = nir->degree_hours[NIGHT][m] * nir->building_load_coeff[m];
      //temp1 += (nir->degree_hours[DAY][m] * (nir->building_load_coeff[m] - nir->dblcs));
      //temp2 += nir->latentload[loop][m];
      //temp3 += nir->wn_lat_load[m];

      cooltemp = (nir->degree_hours[DAY][m] * (nir->building_load_coeff[m] - nir->dblcs) + nir->latentload[loop][m] + nir->wn_lat_load[m] + nir->dr_lat_load[m]);

      ASSERT(nir->sysht_seaseff, sprintf(msg, "Need non zero eff"));
      ASSERT(ndi->clgs.avg_seer, sprintf(msg, "Need non zero eff"));

      if (loop == POST_RETROFIT && phase == PRE_RETROFIT) {
        nir->htgmengy[1][m] = heattemp / 1.e6f / nir->sysht_seaseff;
        nir->clgmengy[1][m] = cooltemp / 1.e6f / ndi->clgs.avg_seer * 3.413f * ndi->clgs.fraction_cooled;
      } else {
        nir->htgmengy[phase][m] = heattemp / 1.e6f / nir->sysht_seaseff;
        nir->clgmengy[phase][m] = cooltemp / 1.e6f / ndi->clgs.avg_seer * 3.413f * ndi->clgs.fraction_cooled;
      }

      nir->heatload[energy_array_index] += heattemp;
      nir->coolload[energy_array_index] += cooltemp;
    }

    nir->heatload[energy_array_index] = nir->heatload[energy_array_index] / 1.e6f - nir->rdbrdldh;
    nir->coolload[energy_array_index] = nir->coolload[energy_array_index] / 1.e6f - nir->rdbrdldc;
    nir->heatengy[energy_array_index] = nir->heatload[energy_array_index] / nir->sysht_seaseff;
    nir->coolengy[energy_array_index] = nir->coolload[energy_array_index] / ndi->clgs.avg_seer * 3.413f * ndi->clgs.fraction_cooled;

    // if (cmds.debug_level & D_NORMAL)
    //   fprintf(stderr, "\n2xnn:%d heatload:%.4g nir->sysht_seaseff:%.4g", 2 * nn, nir->heatload[energy_array_index], nir->sysht_seaseff);
    // if (cmds.debug_level & D_NORMAL)
    //   fprintf(stderr, "\n2xnn:%d coolload:%.4g avg_seer:%.4g fraction_cooled:%.4g", 2 * nn, nir->coolload[energy_array_index], ndi->clgs.avg_seer,
    //           ndi->clgs.fraction_cooled);

    if (phase == PRE_RETROFIT && loop == PRE_RETROFIT) {
      nir->heatload[PRE_RETROFIT_PRE_INFIL] = nir->heatload[energy_array_index];
      nir->coolload[PRE_RETROFIT_PRE_INFIL] = nir->coolload[energy_array_index];
      nir->heatengy[PRE_RETROFIT_PRE_INFIL] = nir->heatengy[energy_array_index];
      nir->coolengy[PRE_RETROFIT_PRE_INFIL] = nir->coolengy[energy_array_index];
    }
    if (phase == PRE_RETROFIT && loop == POST_RETROFIT) {
      ASSERT(nir->cooling_seer_pre_ductseal, sprintf(msg, "Need non zero eff"));
      ASSERT(nir->heating_system_seasonal_efficiency_pre_ductseal, sprintf(msg, "Need non zero eff"));
      nir->coolengy[PRE_RETROFIT_POST_DUCT_SEAL] = nir->coolload[energy_array_index] / nir->cooling_seer_pre_ductseal * 3.413f * ndi->clgs.fraction_cooled;
      nir->heatengy[PRE_RETROFIT_POST_DUCT_SEAL] = nir->heatload[energy_array_index] / nir->heating_system_seasonal_efficiency_pre_ductseal;
    }
  }

  if (cmds.debug_level & D_NEAT_ENERGY_DETAIL_ALL || (phase == PRE_RETROFIT && (cmds.debug_level & D_NEAT_ENERGY_DETAIL_BASE))) {
    fprintf(stderr, "\nHeating/cooling solar apertures for cardinal directions\n");
    for (int i = 0; i <= 4; i++)
      fprintf(stderr, "%8.2f", nir->solar_aperture_direct[HEATING][i]);
    fprintf(stderr, "\n");
    for (int i = 0; i <= 4; i++)
      fprintf(stderr, "%8.2f", nir->solar_aperture_direct[COOLING][i]);
    fprintf(stderr, "\n");

    fprintf(stderr, "\n       Month  Solar incident on surfs  Free heat  Total   Teff   Tbal\n");
    fprintf(stderr, "              North East  South  West  from solr free ht\n\n");
    for (int m = 1; m <= MONTHS; m++) {
      // clang-format off
      fprintf(stderr, "%9d%7.2f%7.2f%7.2f%7.2f%8.0f%9.0f%8.1f%7.1f\n", 
        m, 
        cwd->solar_load[m][SOLAR_NORTH], 
        cwd->solar_load[m][SOLAR_EAST],
        cwd->solar_load[m][SOLAR_SOUTH], 
        cwd->solar_load[m][SOLAR_WEST], 
        nir->totsol[m], 
        nir->internal_gain_day[m], 
        nir->teff[m], 
        nir->balance_point_temp[0][0][m]);
    }
    fprintf(stderr, "\n Tot/Avg %7.2f%7.2f%7.2f%7.2f%8.0f%9.0f%15.1f\n", 
      cwd->solar_load[0][SOLAR_NORTH], 
      cwd->solar_load[0][SOLAR_EAST],
      cwd->solar_load[0][SOLAR_SOUTH], 
      cwd->solar_load[0][SOLAR_WEST], 
      nir->totsol[0], 
      nir->total_annual_free_heat, 
      nir->balance_point_temp[0][0][0]);

    fprintf(stderr, "\nMonthly average effective outdoor temperatures\n");
    for (int j = 1; j <= MONTHS; j++)
      fprintf(stderr, "%7.1f", nir->teff[j]);
    fprintf(stderr, "\n");
    fprintf(stderr, "Given the following average outdoor temperatures\n");
    for (int j = 1; j <= MONTHS; j++)
      fprintf(stderr, "%7.1f", cwd->avg_drybulb_temp[j]);
    fprintf(stderr, "\n");
    fprintf(stderr, "\nAverage infiltration (cfm)\n");
    for (int j = 1; j <= MONTHS; j++)
      fprintf(stderr, "%7.0f", nir->whole_house_cfm[POST_RETROFIT][j]);
    fprintf(stderr, "%10.0f\n", nir->whole_house_cfm[POST_RETROFIT][TOTAL]);
    fprintf(stderr, "\nBuilding load coefficient\n");
    for (int j = 1; j <= MONTHS; j++)
      fprintf(stderr, "%7.1f", nir->building_load_coeff[j]);
    fprintf(stderr, "%10.1f\n", nir->building_load_coeff[AVG]);
    fprintf(stderr, "\nMonthly balance point temperatures\n");
    for (int j = 1; j <= MONTHS; j++)
      fprintf(stderr, "%7.1f", nir->balance_point_temp[0][0][j]);
    fprintf(stderr, "%10.1f\n", nir->balance_point_temp[0][0][0]);

    fprintf(stderr, "\nDegree-hours at computed balance temperature\n");

    for (int j = 1; j <= MONTHS; j++) {
      fprintf(stderr, "%6.0f", nir->degree_hours[HEATING][j]);
      nir->degree_hours[HEATING][TOTAL] += nir->degree_hours[HEATING][j];
    }
    fprintf(stderr, "%10.0f%6.0f\n", nir->degree_hours[HEATING][TOTAL], nir->degree_hours[HEATING][TOTAL] / 24.);

    for (int j = 1; j <= MONTHS; j++){
      fprintf(stderr, "%6.0f", nir->degree_hours[COOLING][j]); 
      nir->degree_hours[COOLING][TOTAL] += nir->degree_hours[COOLING][j];
    }
    fprintf(stderr, "%10.0f%6.0f\n", nir->degree_hours[COOLING][TOTAL], nir->degree_hours[COOLING][TOTAL] / 24.);

    fprintf(stderr, "\nAnnual heating load   = %.2f MMBtu", nir->heatload[PRE_RETROFIT_POST_INFIL]);
    fprintf(stderr, "\nAnnual cooling load   = %.2f MMBtu", nir->coolload[PRE_RETROFIT_POST_INFIL]);
    fprintf(stderr, "\nAnnual heating energy = %.2f MMBtu", nir->heatengy[PRE_RETROFIT_POST_INFIL]);
    fprintf(stderr, "\nAnnual cooling energy = %.2f MMBtu", nir->coolengy[PRE_RETROFIT_POST_INFIL]);

    fprintf(stderr, "\n------------------END of NEAT_ENERGY_USE-------------------\n");
  }

  return;
}

/*************
 *************
 Compute the latent load (Btu/hr) of infiltration given cfm, drybulb
 temperature, tdb (F), and the wetbulb temperature, twb. */

#define HYWEX(t) (c1 / t + c2 + c3 * t + c4 * t * t + c5 * t * t * t + c6 * log(t))

float get_latent_infil_load(float tdb, float twb, float cfm) {
  float wetbt, pwsw, ws, patm, w, load, fraction_met;
  float drybtin, tin = 75, rhumin = .50, pwsdin, pwin, win, loadin, altadj;
  double c1 = -1.044039708e4, c2 = -1.12946496e1, c3 = -2.7022355e-2, c4 = 1.289036e-5, c5 = -2.478068e-9, c6 = 6.5459673;
  double conv = 459.67;
  /* fprintf(stderr, "                                    Out        In");
   * fprintf(stderr, "\nTemperature (F)            = %10.0f%10.0f",tdb,tin);
   * fprintf(stderr, "\nRelative Humidity (%%)      = %20.0f",rhumin*100);
   * fprintf(stderr, "\nWet Bulb Temperature (F)   = %10.0f",twb);  */

  altadj = (float)exp(-0.0000368f * (double)cwd->altitude);
  wetbt = twb + (float)conv;

  ASSERT(wetbt, sprintf(msg, "Need non zero temperature"));
  pwsw = (float)HYWEX((double)wetbt);
  pwsw = (float)exp(pwsw);
  patm = 14.696f * altadj;

  ASSERT((patm - pwsw), sprintf(msg, "Need non zero pressure"));
  ws = 0.62198f * pwsw / (patm - pwsw);

  ASSERT((1093 + 0.444 * tdb - twb), sprintf(msg, "Need non zero temperature"));
  w = ((1093 - 0.556f * twb) * ws - 0.24f * (tdb - twb)) / (1093 + 0.444f * tdb - twb);

  drybtin = tin + (float)conv;

  ASSERT(drybtin, sprintf(msg, "Need non zero temperature"));
  pwsdin = (float)HYWEX((double)drybtin);
  pwsdin = (float)exp(pwsdin);
  pwin = rhumin * pwsdin;

  ASSERT((patm - pwin), sprintf(msg, "Need non zero pressure"));
  win = 0.62198f * pwin / (patm - pwin);

  /* fprintf(stderr, "\nHumidity ratios            = %10.4f%10.4f",w,win);
   * fprintf(stderr, "\n  lbs H2O / lbs dry air "); */
  load = 4840.0f * cfm * w * altadj;
  loadin = 4840.0f * cfm * win * altadj;

  /* fprintf(stderr, "\nLatent loads (Btu/hr)      = %10.0f%10.0f\n",load,loadin); */

  load = load - loadin;
  if (load < 0)
    load = 0;
  fraction_met = 4.33f * ((float)POWC(cwd->cdd65, 0.372)) / 100.0f;
  if (fraction_met > 1.0)
    fraction_met = 1.0;
  load = load * fraction_met;
  return load;
}

/*******************  FUNCTION NAME:  getSolarStorage2  *******************/
/**         DATE:  November 1993                                                        **/
/**           BY:    SLF                                                                    **/
/**  DESCRIPTION:    Calculates the fraction of the solar gain received **/
/**                  over 24 hours that is released during the night        **/
/**                  (Beta) (see p. 49 of Sheila Hayter's notes).           **/
/**         Revisions: 8/11/95, Norm Weaver (NW): transmismod update and solar            **/
/**             storage factor correction                                           **/
/*************************************************************************/
void neat_solar_storage(float fTempOutDay, float fSolarGainTotal, float fFreeHeatDay, float fSkyRadLossDay, float fBLC,
                      float *fSolarStorage) {
  float fTempAdjE;  /* Adjusted temp. (Tm), English units */
  double fTempAdjM; /* Adjusted temp. (Tm), Metric units */
                    //  float fSolarStorageM = 0.22;    /* 1st approx. of solar storage factor */
                    // NW, 8/11/95: lower storage factor for mobile homes
  //  float fSolarStorageM = 0.10f;    /* 1st approx. of solar storage factor */
  float fSolarStorageM = 0.22f; /* 1st approx. of solar storage factor */
                                // For NEAT use the traditional value for light construction

  fTempAdjE = fTempOutDay + ((fSolarGainTotal + fFreeHeatDay - fSkyRadLossDay) / (fBLC));

  fTempAdjM = (double)((fTempAdjE - 32.0f) * 5.0f / 9.0f);

  //if (cmds.debug_level & D_NORMAL)
  //  fprintf(stderr, "\nfTempOutDay:%.4g fSolarGainTotal:%.4g fFreeHeatDay:%.4g fSkyRadLossDay:%.4g fBLC:%.4g", fTempOutDay,
  //          fSolarGainTotal, fFreeHeatDay, fSkyRadLossDay, fBLC);

  ASSERT(fSolarStorageM, sprintf(msg, "Need non zero factor"));
  if (fTempAdjM > 21.0)
    *fSolarStorage = fSolarStorageM * (float)pow((1.64 - (fTempAdjM / 38.7)), (0.44 / fSolarStorageM));
  else /* fTempAdjM <= 21.0 */
    *fSolarStorage = fSolarStorageM * (float)pow((0.56 + (fTempAdjM / 38.7)), (0.44 / fSolarStorageM));

} // End neat_solar_storage() function

/*******************  FUNCTION NAME: SortResults         *****************/
/**         DATE:  9/21/00                                              **/
/**           BY:  MJF                                                  **/
/**  DESCRIPTION:  Order the kbtrcp array and the nir->measure_priority **/
/**                by FIRST decreasing measure_priority THEN by         **/
/*                 decreasing btc (benefit to cost)                     **/
/*************************************************************************/
static void sort_neat_package_measures(float *l_btc, int l_nmst, int *sorted_measure_index) {
  int flgSwap = 1;      // did we swap in the loop
  int kbtctemp, mstemp; // temp values for swapping
  int i;

  if (l_nmst < 2) // nothing to do - changed from 2 to 1, 8/09
    return;       // since prevented proper sorting with two measures.

  // we need to guarantee that any existing
  // retrofits with negative BCR do NOT get
  // sorted past the end list of results otherwise
  // we get a null GLBResult record sorted into the list
  // of retrofits (fixed MJF 8/02 and 4/05)

  nir->measure_priority[l_nmst] = MPS_BOTTOM_MARK;
  l_btc[l_nmst] = MPS_BOTTOM_MARK;

  while (flgSwap) // our simple bubble sort
  {
    flgSwap = 0;
    for (i = 0; i < l_nmst; i++) {

      // sorting first on measure_priority field

      if (nir->measure_priority[i + 1] != // secondary sort key involved
          nir->measure_priority[i]) {

        if (nir->measure_priority[i + 1] > // out of order
            nir->measure_priority[i]) {
          kbtctemp = sorted_measure_index[i + 1];
          sorted_measure_index[i + 1] = sorted_measure_index[i];
          sorted_measure_index[i] = kbtctemp;
          mstemp = nir->measure_priority[i + 1];
          nir->measure_priority[i + 1] = nir->measure_priority[i];
          nir->measure_priority[i] = mstemp;
          flgSwap = 1;
        }
        continue;
      }

      // default sorting on BCR within each measure_priority group

      if (l_btc[sorted_measure_index[i + 1]] > // default sort key
          l_btc[sorted_measure_index[i]]) {
        kbtctemp = sorted_measure_index[i + 1];
        sorted_measure_index[i + 1] = sorted_measure_index[i];
        sorted_measure_index[i] = kbtctemp;
        flgSwap = 1;
      }
    }
  }
}

// Standardize our diagnostics output

static void measure_diag_print_all(char *title, int sir_sort) {
  if (cmds.debug_level & D_NORMAL) {
    measure_diag_print_header(title);
    for (int i = 0; i < nir->nms; i++) {
      fprintf(stderr, "\n");
      if (sir_sort == SORTED)
        measure_diag_print_line(nir->index_by_sir[i], i, "");
      else
        measure_diag_print_line(i, nir->index_by_sir[i], "");
    }
    fprintf(stderr, "\n\n");
  }
}

static void measure_diag_print_header(char *title) {
  fprintf(stderr, "\n\n%s", title);
  fprintf(stderr, "\n i  s  m  p  r                                               htengysav  htdlsav clengysav  cldlsav   dlsav     cost     sir       npv");
}

static void measure_diag_print_line(int i, int s, char *suffix) {

  char comp_group[LRG_STR + 1];
  if (nir->ecm[i].comp_group_num) 
    sprintf(comp_group, "%-3.3s%02d", comp_group_name_short(nir->ecm[i].comp_group_type), nir->ecm[i].comp_group_num);
  else
    comp_group[0] = '\0';

  fprintf(stderr, "%2d %2d %2d %2d %2d  %-24.24s    %-5.5s %-10.10s %9.3f%9.2f%9.3f%9.2f%9.2f%9.2f%9.2f%9.2f %-5.5s", 
    i, s, 
    nir->ecm[i].cms_measure_num, 
    nir->measure_priority[i],
    nir->measure_required[i],
    nir->measure_name[nir->ecm[i].cms_measure_num], 
    comp_group,
    nir->ecm[i].components,
    nir->htengysav[i], 
    nir->htdlsav[i], 
    nir->clengysav[i], 
    nir->cldlsav[i],
    nir->ecm[i].dlsav, 
    nir->ecm[i].cost, 
    nir->ecm[i].sir, 
    nir->npv[i],
    suffix);
}

static void measure_diag_print(char *suffix, int i, int s) {
  if (cmds.debug_level & D_NORMAL) {
    //fprintf(stderr, "\n%5s ", suffix);
    fprintf(stderr, "\n");
    measure_diag_print_line(i, s, suffix);
  }
}

  //  The following code added to attempt preventing sill insulation from
  //  dropping out because of mutually exclusive floor insulation, even when
  //  latter is eliminated by foundation insulation being recommended.
static void prevent_sill_insulation_dropout( void){
  int check[NEAT_MAX_FND], ic;

  ic = 0;
  for (int j = 0; j < nir->nms; j++) {
    int i = nir->index_by_sir[j];
    if (nir->ecm[i].cms_measure_num == N_CMS_FOUNDATION_WALL_INSULATION) {
      if ((nir->measure_priority[i] == MPS_SIR &&  nir->ecm[i].sir >= ndi->key.minimum_acceptable_sir) || 
        nir->measure_priority[i] == MPS_DUCT_SEAL ||
        nir->measure_priority[i] == MPS_INFILTRATION_REDUCTION ||
        nir->measure_priority[i] == MPS_EQUIP_REQUIRED ||
        nir->measure_priority[i] == MPS_ENVELOPE_REQUIRED ||
        nir->measure_priority[i] == MPS_ENVELOPE_REQUIRED_NO_SIR ||
        nir->measure_priority[i] == MPS_EQUIP_REQUIRED_NO_SIR) {
        check[ic] = i;
        ic++;
        if (ic == NEAT_MAX_FND)
          break;
      }
    }
  }
  for (int j = 0; j < nir->nms; j++) {
    int i = nir->index_by_sir[j];
    if (nir->ecm[i].cms_measure_num == N_CMS_SILLBOX_INSULATION){
      for (int k = 0; k < ic; k++) {
        int l = check[k];
        if (components_in_common(nir->ecm[l].components, nir->ecm[i].components)) {
          if (nir->ecm[i].sir >= ndi->key.minimum_acceptable_sir) {
            nir->measure_priority[i] = MPS_SIR;    // second_pass_measure_interaction(il)
          }
        }
      }
    }
  }
}

  // here is the main loop through the ordered first pass measures  CUMULATIVE Pass  <<<<<<===========

static void cumulative_interactive_effects(void) {

  float sir = 0.0;
  float delta_cooling_load;
  float delta_heating_load;
  float dollars_saved;
  float effsav, preffsav, htensav, clensav, htnpvsav, ducteffsav;
  float effclsav, peracsav;
  int ftypesav;

  nir->heatengy[LAST_GOOD_MEASURE] = nir->heatengy[PRE_RETROFIT_POST_INFIL];
  nir->heatload[LAST_GOOD_MEASURE] = nir->heatload[PRE_RETROFIT_POST_INFIL];
  nir->coolengy[LAST_GOOD_MEASURE] = nir->coolengy[PRE_RETROFIT_POST_INFIL];
  nir->coolload[LAST_GOOD_MEASURE] = nir->coolload[PRE_RETROFIT_POST_INFIL];

  if (cmds.debug_level & D_NORMAL) {
    fprintf(stderr, "\n    Interacted values of:\n");
    fprintf(stderr, "      htload   clload   htengy   clengy\n");
    fprintf(stderr, "   %9.2f%9.2f%9.2f%9.2f\n",  nir->heatload[PRE_RETROFIT_POST_INFIL], nir->coolload[PRE_RETROFIT_POST_INFIL], nir->heatengy[PRE_RETROFIT_POST_INFIL], nir->coolengy[PRE_RETROFIT_POST_INFIL]);

    measure_diag_print_header("SECOND pass as accumulated\n");
  }

  for (int jl = 0; jl < nir->nms; jl++) {                     // for each first pass measure unordered
    int il = nir->index_by_sir[jl];                           // get the sir order index
    int cms_measure_num = nir->ecm[il].cms_measure_num;

    ASSERT(nir->ecm[il].index == il, sprintf(msg, "The element %d does not match ecm[].index", il));

    switch (nir->meas_type[cms_measure_num]) {
      case CMT_HEATING_ENVELOPE:
      case CMT_COOLING_ENVELOPE:
      case CMT_WINDOWS_DOORS: {

        // adjust dfreht values for attic insulation measures IF white roof measure has already been implemented.

        if (cms_measure_num == N_CMS_ATTIC_INSULATION_R19 ||
          cms_measure_num == N_CMS_ATTIC_INSULATION_R30 ||
          cms_measure_num == N_CMS_ATTIC_INSULATION_R38 ||
          cms_measure_num == N_CMS_ATTIC_INSULATION_R49 || 
          cms_measure_num == N_CMS_FILL_CEILING_CAVITY) // Do adjustment only for attic ins measures
        {
          for (int j = 0; j < jl; j++) {        // Loop through only those measures already evaluated
            int i = nir->index_by_sir[j];
            int m = nir->ecm[i].cms_measure_num;
            if (m == N_CMS_WHITE_ROOF_COATING && nir->measure_priority[i] == MPS_SIR)  { // Do adjustment only if white roof implemented
              for (int l = 1; l <= MONTHS; l++)
                nir->dfreht[il][l] *= WHITE_ROOF_ABSORPTIVITY / ROOF_ABSORPTIVITY;
              break;
            }
          }
        } else if (cms_measure_num == N_CMS_WHITE_ROOF_COATING) {
          white_roof_coating(1, il);
        }

        annual_energy_load_change(nir->dua[il], nir->dfreht[il], &delta_heating_load, &delta_cooling_load);

        if (nir->meas_type[cms_measure_num] == CMT_WINDOWS_DOORS)    // Correction for latent load of window and door leakage
          delta_cooling_load += nir->ecm[il].value2;                // value2 should be 0.0 for insulation measures

        ASSERT((nir->sysht_seaseff * nir->bladj[PRE_HEATING]) != 0, sprintf(msg, "Need non zero eff"));
        ASSERT(ndi->clgs.avg_seer, sprintf(msg, "Need non zero average SEER"));

        nir->htengysav[il] = delta_heating_load / nir->sysht_seaseff * nir->bladj[PRE_HEATING];
        nir->clengysav[il] = delta_cooling_load / ndi->clgs.avg_seer * 3.413f * nir->bladj[PRE_COOLING] * ndi->clgs.fraction_cooled;
        nir->ecm[il].ensav = nir->htengysav[il] + nir->clengysav[il];
        nir->htdlsav[il] = nir->htengysav[il] * CompFuelCost();
        nir->cldlsav[il] = nir->clengysav[il] * fuel_cost(ELECTRIC) * 1000.0f / 3.413f;
        dollars_saved = nir->htdlsav[il] + nir->cldlsav[il];
        nir->htlcs[il] = DCompFuelCost(nir->mslife[il]);
        nir->cllcs[il] = pw_fuel_cost(ELECTRIC, nir->mslife[il]);
        nir->dslfsav[il] = nir->htengysav[il] * nir->htlcs[il] + nir->clengysav[il] * nir->cllcs[il];
        sir = calculate_sir(nir->dslfsav[il], nir->ecm[il].cost);
        /*      nir->npv[il] = nir->dslfsav[il]-nir->ecm[il].cost; */
        break;
      }
      case CMT_HEATING_SYSTEM_UPDATE: {
        effsav = nir->sysht_seaseff;
        preffsav = ndi->htg[PRIMARY].delivered_eff;
        htensav = nir->heatengy[LAST_GOOD_MEASURE];
        ducteffsav = ndi->inf.post_duct_seal_efficiency;
        nir->htlcs[il] = pw_fuel_cost(ndi->htg[PRIMARY].fuel_type, nir->mslife[il]);
        htnpvsav = nir->npv[il];
        second_pass_measure_interaction(il);
        nir->htengysav[il] = (htensav - nir->heatengy[LAST_GOOD_MEASURE]) * nir->bladj[PRE_HEATING];
        nir->htdlsav[il] = dollars_saved = nir->htengysav[il] * fuel_cost_per_mmbtu(ndi->htg[PRIMARY].fuel_type);
        nir->npv[il] = htnpvsav;
        nir->heatengy[LAST_GOOD_MEASURE] = htensav;
        ndi->htg[PRIMARY].delivered_eff = preffsav;
        nir->sysht_seaseff = effsav;
        ndi->inf.post_duct_seal_efficiency = ducteffsav;
        nir->dslfsav[il] = nir->htengysav[il] * nir->htlcs[il];
        sir = calculate_sir(nir->dslfsav[il], nir->ecm[il].cost);
        nir->clengysav[il] = nir->cldlsav[il] = nir->cllcs[il] = 0.;
        break;
      }
      // O Magnum Mysterium / Robert Shaw Festival & Chamber Singers - Victoria, Tomás Luis de (inspiration for the day)
      case CMT_HEAT_PUMP_REPLACE:
      case CMT_HEATING_SYSTEM_REPLACE: {
        effsav = nir->sysht_seaseff;
        effclsav = ndi->clgs.avg_seer;
        peracsav = ndi->clgs.fraction_cooled;
        preffsav = ndi->htg[PRIMARY].delivered_eff;
        htensav = nir->heatengy[LAST_GOOD_MEASURE];
        clensav = nir->coolengy[LAST_GOOD_MEASURE];
        ducteffsav = ndi->inf.post_duct_seal_efficiency;
        htnpvsav = nir->npv[il];
        ftypesav = ndi->htg[PRIMARY].fuel_type;
        second_pass_measure_interaction(il);
        nir->htengysav[il] = (htensav - nir->heatengy[LAST_GOOD_MEASURE]) * nir->bladj[PRE_HEATING];
        dollars_saved = nir->htdlsav[il];
        nir->npv[il] = htnpvsav;
        nir->heatengy[LAST_GOOD_MEASURE] = htensav;
        nir->coolengy[LAST_GOOD_MEASURE] = clensav;
        ndi->htg[PRIMARY].delivered_eff = preffsav;
        nir->sysht_seaseff = effsav;
        ndi->clgs.avg_seer = effclsav;
        ndi->clgs.fraction_cooled = peracsav;
        ndi->inf.post_duct_seal_efficiency = ducteffsav;
        ndi->htg[PRIMARY].fuel_type = ftypesav;
        sir = calculate_sir(nir->dslfsav[il], nir->ecm[il].cost);
        if (nir->meas_type[cms_measure_num] == CMT_HEATING_SYSTEM_REPLACE) {
          nir->clengysav[il] = nir->cldlsav[il] = nir->cllcs[il] = 0.;
        }
        break;
      }
      case CMT_COOLING_SYSTEM: {
        nir->clengysav[il] = nir->ecm[il].value * nir->coolload[LAST_GOOD_MEASURE] * nir->bladj[PRE_COOLING];
        nir->cldlsav[il] = nir->clengysav[il] * fuel_cost(ELECTRIC) * 1000.0f / 3.413f;
        nir->cllcs[il] = pw_fuel_cost(ELECTRIC, nir->mslife[il]);
        // TODO I think this if has a dead else
        if (nir->meas_type[cms_measure_num] == CMT_COOLING_SYSTEM) {
          dollars_saved = nir->cldlsav[il];
          nir->dslfsav[il] = nir->clengysav[il] * nir->cllcs[il];
          nir->htengysav[il] = nir->htdlsav[il] = nir->htlcs[il] = 0.;
        } else {
          dollars_saved += nir->cldlsav[il];
        }
        sir = calculate_sir(nir->dslfsav[il], nir->ecm[il].cost);
        break;
      }
      case CMT_SMART_THERMOSTAT: {
        /* Values of htengysav and htdlsav set in smart_thermostat() */
        int save_nms = nir->nms;
        nir->nms = il;
        smart_thermostat(0);
        nir->nms = save_nms;
        /*      nir->htengysav[il] *= nir->bladj[PRE_HEATING];  Multiplication done in smrtherm */
        dollars_saved = nir->ecm[il].dlsav; /* *nir->bladj[PRE_HEATING];   "  */
                                /*      nir->htlcs[il] = DCompFuelCost(retrofit_materials[n].life);  OOPS, n is not set MJF 4/99 */
        nir->htlcs[il] = DCompFuelCost(nir->mslife[il]);
        sir = nir->ecm[il].sir;
        nir->clengysav[il] = nir->cldlsav[il] = nir->cllcs[il] = 0.;
        break;
      }
      case CMT_DUCT: {
        int save_nms = nir->nms;
        nir->nms = il;
        duct_insulation();
        nir->nms = save_nms;
        /*      nir->htengysav[il] *= nir->bladj[PRE_HEATING];  Multiplication done in ductins() */
        dollars_saved = nir->ecm[il].dlsav; /*  *nir->bladj[PRE_HEATING];   "  */
        nir->htlcs[il] = pw_fuel_cost(ndi->htg[PRIMARY].fuel_type, nir->mslife[il]);
        nir->cllcs[il] = pw_fuel_cost(ELECTRIC, nir->mslife[il]);
        nir->dslfsav[il] = nir->htengysav[il] * nir->htlcs[il] + nir->clengysav[il] * nir->cllcs[il];
        sir = calculate_sir(nir->dslfsav[il], nir->ecm[il].cost);
        break;
      }
      case CMT_WATER_HEATER: {
        sir = nir->ecm[il].sir;
        dollars_saved = nir->ecm[il].dlsav;
        break;
      }
      case CMT_BASELOAD:
      case CMT_INFILTRATION_REDUCTION: 
      case CMT_OTHER:  {
        // sir = nir->ecm[il].sir;    // static analysis says this is DEAD,,, should it be?  MJF 12/2018 TODO
        dollars_saved = nir->ecm[il].dlsav;
        measure_diag_print("CONT1", il, jl);
        continue; // TODO, figure out why... note that the nir->ecm[].sir is NOT filled in below... should it be?
      default:
        ASSERT(FALSE, sprintf(msg, "Unhandled conservation measure type: %d", nir->meas_type[cms_measure_num]));
      }
    }   // end of switch statement   ======<<<<<<<<<<

    nir->ecm[il].sir = sir;
    nir->ecm[il].lifetime = nir->ecm[il].lifetime == 0 ? nir->mslife[il] : nir->ecm[il].lifetime;
    nir->ecm[il].dlsav = dollars_saved;

    if (nir->measure_priority[il] == MPS_NPV || nir->measure_priority[il] == MPS_DUCT_SEAL){
      measure_diag_print("CONT2", il, jl);
      continue;     // next measure
    }

    // Our selection of the next best measure based on NPV if a measure falls below the min SIR

    if (sir < ndi->key.minimum_acceptable_sir && 
      (nir->measure_priority[il] == MPS_SIR ||
       nir->measure_priority[il] == MPS_NPV)) {     // did not meet minimum SIR and should have to be considered
      nir->measure_priority[il] = MPS_NPV;          // so implement a previously successful mutually exclusive measure based on highest NPV
      for (int k = jl - 1; k >= 0; k--) {           // looking through ALL PRIOR measures with higher NPV, if there is one
        int test_ecm_index = nir->index_by_sir[k];                   
        if (nir->associated_winner_ecm_index[test_ecm_index] == il) {      // found prior 
          second_pass_measure_interaction(test_ecm_index);              // so add that one to the package
          measure_diag_print("PRIOR", test_ecm_index, k);
          nir->measure_priority[test_ecm_index] = MPS_SIR;
        }
      }
    } else {  // current measure il is the winner on SIR
      int found_alternate_flag = FALSE;
      int test_ecm_index = 0;
      int k;
      for (k = jl + 1; k < nir->nms; k++) {     // so we look FORWARD through the other measures for mutually exclusive measures
         test_ecm_index = nir->index_by_sir[k];
        //km = nir->ecm[l].cms_measure_num;

        // Check for mutually exclusive measures on the same component
        // demote sort priority for the one with lesser NPV

        if (!mutually_exclusive_measures(cms_measure_num, nir->ecm[test_ecm_index].cms_measure_num))
          continue;
        else if (!components_in_common(nir->ecm[test_ecm_index].components, nir->ecm[il].components))
          continue;
        else if ((abs(nir->measure_priority[il]) >= abs(nir->measure_priority[test_ecm_index]) && nir->npv[il] > nir->npv[test_ecm_index]) || 
           abs(nir->measure_priority[il]) > MPS_SIR) {
           nir->measure_priority[test_ecm_index] = MPS_NPV;  // mututally exclusive on same components get NPV priority
          continue;
        } else {  // We found a better option in test_ecm_index vs il, so don't implement il
          nir->associated_winner_ecm_index[il] = test_ecm_index;  // rather tag the better measure index in il
          nir->measure_priority[il] = MPS_NPV;  // but it still gets NPV priority like the others
          nir->measure_required[il] = FALSE;    // clear any required flag
          found_alternate_flag = TRUE;  // flag it to skip
          break;
        }
      } //. next measure in search for higher NPV forward direction

      if (found_alternate_flag == FALSE) {
        second_pass_measure_interaction(il);
        measure_diag_print("IMPL", il, jl);
      } else {
        measure_diag_print("SKIP", il, jl);
      }
    }
  } // next first pass measure in CUMULATIVE Pass
}

static void heating_system_priority_adjustments(void) {

  /* Apply results of Furnace Tuneup/Replacement option */

  for (int i = 0; i < nir->nms; i++) { /* check for applicability of mand. measures */
    if (nir->ecm[i].cms_measure_num == N_CMS_FURNACE_TUNE_UP) {
      if (ndi->htg[PRIMARY].retrofit_option == ES_TUNEUP_REQUIRED) {
        if (ndi->cms[N_CMS_FURNACE_TUNE_UP].active) {
          nir->measure_required[i] = YES;
          nir->measure_priority[i] = MPS_EQUIP_REQUIRED; // Tuneup measure on, tag as required
          if (ndi->htg[PRIMARY].inc_sir != YES)
            nir->measure_priority[i] = MPS_EQUIP_REQUIRED_NO_SIR;
        } else {
          add_neat_message("Tuneup measure declared required but not available for system described.");
        }
      }
    } //   Else error message

    else if (nir->ecm[i].cms_measure_num == N_CMS_REPLACE_HEATING_SYSTEM) {
      if (ndi->htg[PRIMARY].retrofit_option == ES_STDEFF_REP_REQUIRED || ndi->htg[PRIMARY].retrofit_option == ES_ELECRES_REP_REQUIRED) {
        if (ndi->cms[N_CMS_REPLACE_HEATING_SYSTEM].active) {
          nir->measure_required[i] = YES;
          nir->measure_priority[i] = MPS_EQUIP_REQUIRED; // Replacement measure on, tag as required
          if (ndi->htg[PRIMARY].inc_sir != YES)
            nir->measure_priority[i] = MPS_EQUIP_REQUIRED_NO_SIR;
        } else {
          add_neat_message("Heating system replacement declared required but not available for system described.");
        }
      }
    } //   Else error message

    else if (nir->ecm[i].cms_measure_num == N_CMS_HIGH_EFFICIENCY_FURNACE) {
      if (ndi->htg[PRIMARY].retrofit_option == ES_HIEFF_REP_REQUIRED) {
        if (ndi->htg[PRIMARY].system_type == HE_STEAM_BOILER || ndi->htg[PRIMARY].system_type == HE_HOT_WATER_BOILER) {
          ndi->htg[PRIMARY].retrofit_option = ES_STDEFF_REP_REQUIRED;
          add_neat_message("High efficiency replacement boiler selected but not available.");
        }
        // Existing system not compatible with High Eff. Replacement
        else if (ndi->cms[N_CMS_HIGH_EFFICIENCY_FURNACE].active) {
          nir->measure_required[i] = YES;
          nir->measure_priority[i] = MPS_EQUIP_REQUIRED; // High Eff. Rep measure on, tag as required
          if (ndi->htg[PRIMARY].inc_sir != YES)
            nir->measure_priority[i] = MPS_EQUIP_REQUIRED_NO_SIR;
        } else {
            add_neat_message("System replacement declared required but not available for system described.");
        }
      }
    } //   Else error message

    else if (nir->ecm[i].cms_measure_num == N_CMS_HIGH_EFFICIENCY_BOILER) { // High Eff. Boiler Replacement measure
      if (ndi->htg[PRIMARY].retrofit_option == ES_HIEFF_REP_REQUIRED) {
        if (ndi->htg[PRIMARY].system_type == HE_GRAVITY_FURNACE || ndi->htg[PRIMARY].system_type == HE_FORCED_AIR_FURNACE) {
          ndi->htg[PRIMARY].retrofit_option = ES_STDEFF_REP_REQUIRED;
          add_neat_message("High efficiency replacement boiler selected but not available.");
        }
        // Existing system not compatible with High Eff. Replacement
        else if (ndi->cms[N_CMS_HIGH_EFFICIENCY_BOILER].active) {
          nir->measure_required[i] = YES;
          nir->measure_priority[i] = MPS_EQUIP_REQUIRED; // High Eff. Rep measure on, tag as required
          if (ndi->htg[PRIMARY].inc_sir != YES)
            nir->measure_priority[i] = MPS_EQUIP_REQUIRED_NO_SIR;
        } else {
          add_neat_message("System replacement declared required but not available for system described.");
        }
      }
    } // end of final elseif
  } // end of loop through first pass measures
}


