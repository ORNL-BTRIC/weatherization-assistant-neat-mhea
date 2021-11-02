/***************************************************************************
* MODULE:       mhea.c            CREATED:    October 7, 1998
*
* AUTHOR:       Mark Fishbaugher, Fishbaugher and Associates  1998
*
* MDESC:        MHEA executable engine.  This program is just the analysis
*           portion of the MHEA program.  It takes the name of an input
*           MDI file and creates the output report files.
*
*           It returns 0 for OK, and non-zero for an error condition.
*           Other command line flags are:
*           -s for silent operation
*           -d for detailed calc debug output (formerly norm_DEBUG output)
*
*           -i iname       where iname is the MDI input file full path name
*           -o odir        where odir is the output directory path [default = output\]
*           -w wdir        where wdir is the weather directory     [default = weather\]
*
* HISTORY:
*
*           Completely refactored for use in combined NEAT/MHEA executable for
*           the new RestFul API used in the web interface  MJF 2020
****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wa_engine.h"

static void fill_static_global_arrays(void);
static void adjust_r_value_per_inch_for_compression(void);
static void adjust_free_heat_for_occupancy(void);

/***************************************************************************
 ** Function Name: run_mhea
 **          Date: January 26, 2000
 **     Author(s): Mark Fishbaugher
 **
 **  DESCRIPTION:  Heart of the MHEA analysis engine
 **************************************************************************/
void run_mhea(void) {

  fill_static_global_arrays();

  // must have BOTH of these allocated to run correctly

  ASSERT(mdi, sprintf(msg, "You must have MHEA dwelling information structure to run engine"));
  ASSERT(mir, sprintf(msg, "You must have MHEA intermediate result structure to run engine"));
  ASSERT(mor, sprintf(msg, "You must have MHEA output result structure to run engine"));

  read_weather_file(&mdi->wth);

  initialize_fuel_cost_data(mdi->fcs, mdi->fer, 1.0f + (mdi->key.real_discount_rate / 100.0f));

  analysis_initialize();

  adjust_r_value_per_inch_for_compression();
 
  duct_leakage_MHEA();

  adjust_free_heat_for_occupancy();

  monthly_infiltration(mir->fCfm_house[M_PRE_RETROFIT],  mir->fCfm_house[M_POST_RETROFIT], mdi->key.heating_setpoint_day, 1.0, mir->whole_house_cfm);

  mir->window_cfm_adjustment = window_constrain_cfm_factor(mir->whole_house_cfm, mir->wn_cfm_tot, mir->dr_cfm_tot);
  if (mir->window_cfm_adjustment < 1.0f) {
    char msg[512];
    sprintf(msg, "Window infiltration constrained to %0.0f %% of whole house total reduced by an average of %0.1f %%", MAX_WINDOW_DOOR_PERCENT, mir->window_cfm_adjustment * 100);
    add_neat_message(msg);
    if (cmds.debug_level & D_INFILTRATION_DETAIL) {
      fprintf(stderr, "\nAverage adjusted monthly WINDOW cfm = %7.2f", mir->wn_cfm_tot[AVG]);
      fprintf(stderr, "\nPercent reduction in average WINDOW leakage = ");
      fprintf(stderr, "%7.1f\n", mir->window_cfm_adjustment * 100.);
    }
  }
  mir->door_cfm_adjustment = door_constrain_cfm_factor(mir->whole_house_cfm, mir->wn_cfm_tot, mir->dr_cfm_tot);
  if (mir->door_cfm_adjustment < 1.0f) {
    char msg[512];
    sprintf(msg, "Door infiltration constrained to %0.0f %% of whole house total reduced by an average of %0.1f %%", MAX_WINDOW_DOOR_PERCENT, mir->door_cfm_adjustment * 100);
    add_neat_message(msg);
    if (cmds.debug_level & D_INFILTRATION_DETAIL) {
      fprintf(stderr, "\nAverage adjusted monthly DOOR cfm = %7.2f", mir->wn_cfm_tot[AVG]);
      fprintf(stderr, "\nPercent reduction in average DOOR leakage = ");
      fprintf(stderr, "%7.1f\n", mir->door_cfm_adjustment * 100.);
    }
  }
  
  mir->flgWhichPass = BASE_CASE;

  mir->fAdj_Htg = 1.0; // First pass has utility billing adjustment factors
  mir->fAdj_Clg = 1.0;

  if (cmds.debug_level & D_NORMAL) {
    fprintf(stderr, "\n\nBASE_CASE");
  }

  mhea_energy_use();    // first call to establish untouched dwelling base case

  if (cmds.debug_level & D_NORMAL) {
    fprintf(stderr, "\n\nAnnual Heating kBtu: %8.1f  Annual Cooling kBtu: %8.1f", mir->fHeating_Energy/1000, mir->fCooling_Energy/1000);
  }

  // starting point for energy fBasecase and current calculated for each measure
  mir->fBasecase_Heating = mir->fPre_Heating = mir->fHeating_Energy;
  mir->fBasecase_Cooling = mir->fPre_Cooling = mir->fCooling_Energy;

  mir->flgWhichPass = FIRST_PASS; /* For First Pass Retrofit Calculations */

  if (cmds.debug_level & D_NORMAL)
    fprintf(stderr, "\n\nFIRST_PASS");

  first_pass_retrofits();

  mir->flgWhichPass = CUMULATIVE; /* For Cumulative Pass Retrofit Calculations */

  if (cmds.debug_level & D_NORMAL)
    fprintf(stderr, "\n\nCUMULATIVE");

  cumulative_retrofits();

  get_base_load();

  mhea_results(FALSE);

  if (mdi->gnl.do_billing_adjust == YES) {      // adjustment run is optional
    manage_mhea_billing_adjustments();

    // only proceed with the adjustment runs if we return from the
    // above call with non-unity factors (ie. there is billing data)

    if (mir->fAdj_Htg != 1.0f || mir->fAdj_Clg != 1.0f) {

      // Reset globals storing measures' infiltration reduction

      for (int i = 0; i < MHEA_MAX_CMS; i++) {
        mir->fInfMeasEnrgyHtg[i] = 0.0f;
        mir->fInfMeasEnrgyClg[i] = 0.0f;
      }

      // Reset the moving base house consumptions
      mir->fPre_Heating = mir->fBasecase_Heating;
      mir->fPre_Cooling = mir->fBasecase_Cooling;

      // all of the following is just a duplicate of the code above
      // w/o the billing adjustments to the base loads

      mir->flgWhichPass = FIRST_PASS; /* For First Pass Retrofit Calculations */

      if (cmds.debug_level & D_NORMAL)
        fprintf(stderr, "\n\n----mir->flgWhichPass(Adj) = FIRST_PASS w ADJUST----\n");

      // let's use the un-adjusted heating and cooling figures on our
      // first pass. Shouldn't affect the ordering of measures.
      // Otherwise would need to feed fAdj_* values into routine.

      first_pass_retrofits();

      mir->flgWhichPass = CUMULATIVE; /* For Cumulative Pass Retrofit Calculations */

      if (cmds.debug_level & D_NORMAL)
        fprintf(stderr, "\n\n----mir->flgWhichPass = CUMULATIVE w ADJUST----\n");

      // Apply billing adjustment to base energy use
      mir->fPre_Heating = mir->fHeating_Energy * mir->fAdj_Htg;
      mir->fPre_Cooling = mir->fCooling_Energy * mir->fAdj_Clg;

      cumulative_retrofits();

      if (mor->num_measure > 0) // get rid of any unadjusted results
        mor->num_measure = 0;

      /* make sure we really toss the whole structure by clearing */
      /* the unadjusted measure results, MJF 5/06                 */
      memset(mor->measure, 0, MAXECMS * sizeof(MHEA_MEASURE));

      mhea_results(TRUE);

    }
  } // end 'with billing adjustment' runs

  return;
}


 /***********************************************************************/
  /* Set R/inch of compressed loose fiberglass and cellulose insulations */
  /* to correspond to user-supplied densities.  Relations are curve fits */
  /* to 1992 Energy Design Update values.                                */
  /***********************************************************************/

static void adjust_r_value_per_inch_for_compression(void) {
  float fDen = mdi->key.density_of_loose_fiberglass_insulation;

  if (fDen <= 2.5f)
    mir->fRinFGCompressed = 0.975f + 3.2f * fDen - 0.7f * fDen * fDen;
  else if (fDen <= 3.0f)
    mir->fRinFGCompressed = 4.6f;
  else
    mir->fRinFGCompressed = (float)(-0.2 * fDen + 5.2);

  if (mir->fRinFGCompressed < 0.0f)
    mir->fRinFGCompressed = 0.0f;

  fDen = mdi->key.density_of_loose_cellulose_insulation;

  mir->fRinCompCelInsul = (float)(-0.066f * fDen + 3.903);

  if (mir->fRinCompCelInsul < 0.0f)
    mir->fRinCompCelInsul = 0.0f;
}


  // Adjust interior gain values for number of occupants
  // The default interior gain values are for 2 people so we want
  // to adjust the numbers comming out of the parameter set rather
  // than a simpler calculation. Change 4/08 to be consistent with NEAT

static void adjust_free_heat_for_occupancy(void) {
  if (mdi->gnl.avg_no_occupants < 2.0) {
    mdi->key.free_heat_from_interior_sources_night -= 276.0f;
    mdi->key.free_heat_from_interior_sources_day -= 276.0f;
  } else if (mdi->gnl.avg_no_occupants > 2.0) {
    mdi->key.free_heat_from_interior_sources_night += (mdi->gnl.avg_no_occupants - 2.0f) * 224.0f;
    mdi->key.free_heat_from_interior_sources_day += (mdi->gnl.avg_no_occupants - 2.0f) * 224.0f;
  }
}

static void fill_static_global_arrays() {

  mir->fRinFGCompressed = R_PER_INCH_FG_COMPRESSED;
  
  mir->fDensExistCelInsul = 1.6f;     // Density of existing (uncompressed) cellulose insulation
  mir->fRinExistCelInsul = 3.8f;      // R's/inch of existing (uncompressed) cellulose insulation
  mir->fDensExistBatInsul = 1.0f;     // Density of existing (uncompressed) fiberglass batt insulation
  mir->fRinCompCelInsul = 3.7f;       // R's/inch of compressed cellulose insulation
  mir->fRinCompBatInsul = 4.2f;       // R's/inch of compressed fiberglass batt insulation
  mir->fRinBellyLFGInsul = 3.0f;      // R's/inch of retrofitted loose FG insulation in belly
  mir->fRinBellyCelInsul = 3.75f;     // R's/inch of retrofitted cellulose insulation in belly
  mir->fDensBellyLFGInsul = 0.75f;    // Density of retrofitted loose FG insulation in belly
  mir->fDensBellyCelInsul = 2.0f;     // Density of retrofitted cellulose insulation in belly

  // our list of function pointers in FIRST PASS execution order

  // first clear them so subsequent assignments do not have to be contiguous #271
  for (int i = 0; i < MHEA_MAX_CMS; i++) Measure_Function[i] = 0;

  Measure_Function[M_CMS_REPLACE_HEATING_SYSTEM]            = retro_replace_heating;
  Measure_Function[M_CMS_SEAL_DUCTS]                        = retro_seal_ducts;
  Measure_Function[M_CMS_GENERAL_AIR_SEALING]               = retro_air_seal;
  Measure_Function[M_CMS_WALL_FIBERGLASS_BATT_INSL]         = retro_insulate_wall_batt;
  Measure_Function[M_CMS_WALL_FIBERGLASS_BATT_INSL_ADD]     = retro_insulate_wall_batt_add;
  Measure_Function[M_CMS_WALL_CELLULOSE_LOOSE_INSL]         = retro_insulate_wall_cellulose;
  Measure_Function[M_CMS_WALL_CELLULOSE_LOOSE_INSL_ADD]     = retro_insulate_wall_cellulose_add;
  Measure_Function[M_CMS_WALL_FIBERGLASS_LOOSE_INSL]        = retro_insulate_wall_fiberglass;
  Measure_Function[M_CMS_WALL_FIBERGLASS_LOOSE_INSL_ADD]    = retro_insulate_wall_fiberglass_add;
  Measure_Function[M_CMS_BELLY_CELLULOSE_LOOSE_INSL]        = retro_insulate_belly_cellulose;
  Measure_Function[M_CMS_BELLY_CELLULOSE_LOOSE_INSL_ADD]    = retro_insulate_belly_cellulose_add;
  Measure_Function[M_CMS_BELLY_FIBERGLASS_LOOSE_INSL]       = retro_insulate_belly_fiberglass;
  Measure_Function[M_CMS_BELLY_FIBERGLASS_LOOSE_INSL_ADD]   = retro_insulate_belly_fiberglass_add;
  Measure_Function[M_CMS_ROOF_CELLULOSE_LOOSE_INSL]         = retro_insulate_roof_cellulose;
  Measure_Function[M_CMS_ROOF_CELLULOSE_LOOSE_INSL_ADD]     = retro_insulate_roof_cellulose_add;
  Measure_Function[M_CMS_ROOF_FIBERGLASS_LOOSE_INSL]        = retro_insulate_roof_fiberglass;
  Measure_Function[M_CMS_ROOF_FIBERGLASS_LOOSE_INSL_ADD]    = retro_insulate_roof_fiberglass_add;
  Measure_Function[M_CMS_ADD_SKIRTING]                      = retro_skirt;
  Measure_Function[M_CMS_ADD_SKIRTING_ADD]                  = retro_skirt_add;
  Measure_Function[M_CMS_WHITE_ROOF_COATING]                = retro_white_coat;
  Measure_Function[M_CMS_WHITE_ROOF_COATING_ADD]            = retro_white_coat_add;

  //Required vs optional door replacements now handled by retro_door and retro_door_add #245
  //Measure_Function[M_CMS_REPLACE_DOORS_REQUIRED]            = retro_door_required;

  Measure_Function[M_CMS_REPLACE_DOORS]                     = retro_door;
  Measure_Function[M_CMS_REPLACE_DOORS_ADD]                 = retro_door_add;
  Measure_Function[M_CMS_STORM_DOORS]                       = retro_storm_door;
  Measure_Function[M_CMS_STORM_DOORS_ADD]                   = retro_storm_door_add;
  Measure_Function[M_CMS_REPLACE_WINDOWS]                   = retro_window;
  Measure_Function[M_CMS_REPLACE_WINDOWS_ADD]               = retro_window_add;
  Measure_Function[M_CMS_PLASTIC_STORM_WINDOWS]             = retro_plastic_storm;
  Measure_Function[M_CMS_PLASTIC_STORM_WINDOWS_ADD]         = retro_plastic_storm_add;
  Measure_Function[M_CMS_GLASS_STORM_WINDOWS]               = retro_glass_storm;
  Measure_Function[M_CMS_GLASS_STORM_WINDOWS_ADD]           = retro_glass_storm_add;
  Measure_Function[M_CMS_ADD_AWNINGMHEAS]                   = retro_awning;
  Measure_Function[M_CMS_ADD_AWNINGMHEAS_ADD]               = retro_awning_add;
  Measure_Function[M_CMS_ADD_SHADE_SCREENS]                 = retro_shade;
  Measure_Function[M_CMS_ADD_SHADE_SCREENS_ADD]             = retro_shade_add;
  Measure_Function[M_CMS_SETBACK_THERMOSTAT]                = retro_smart_thermostat;
  Measure_Function[M_CMS_TUNE_HEATING_SYSTEM]               = retro_tune_heating;
  Measure_Function[M_CMS_EVAPORATIVE_COOLING]               = retro_evaporative_cooling;
  Measure_Function[M_CMS_TUNE_COOLING_SYSTEM]               = retro_tune_cooling;
  Measure_Function[M_CMS_REPLACE_DX_COOLING_EQUIP]          = retro_replace_cooling;
  Measure_Function[M_CMS_LIGHTING_RETROFITS]                = retro_replace_lighting;
  Measure_Function[M_CMS_REFRIGERATOR_REPLACEMENT]          = retro_refrigerator;
  Measure_Function[M_CMS_WATER_HEATER_TANK_INS]             = retro_water_heater_insulation;
  Measure_Function[M_CMS_WATER_HEATER_PIPE_INS]             = retro_water_heater_pipe_insulation;
  Measure_Function[M_CMS_LOW_FLOW_SHOWERHEADS]              = retro_show_heads;
  Measure_Function[M_CMS_WATER_HEATER_REPLACEMENT]          = retro_water_heater;
  Measure_Function[M_CMS_WINDOW_SEALING]                    = retro_window_sealing;
  Measure_Function[M_CMS_WINDOW_SEALING_ADD]                = retro_window_sealing_add;
}
