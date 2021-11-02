  /***************************************************************************
* MODULE:       json.c            CREATED:     October 22, 2018
*
* AUTHOR:       Mark Fishbaugher (mark@fishbaugher.com)
*
* MDESC:        The JSON input handling for reading the MHEA MDI structure
****************************************************************************/
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "wa_engine.h"

/// Reads the fuel escalation rates from the sys/fuel_escalation JSON files.  Read the fuel escalation data from
/// the external file given the 'rer' (referenced escalation rate) information in the passed MDI structure.  It
/// uses the year and region to form a filename following a naming convention, then cJSON to read that escalation
/// factor information into the MDI 'fer' structure for later use in the engine.

static void get_referenced_escalation_rates(MDI *top, cJSON *jschema) {
  cJSON *jleaf = NULL;
  cJSON *jleaf2 = NULL;
  cJSON *jbranch = NULL;
  cJSON *jbranch2 = NULL;
  // cJSON *bld_jbranch = NULL;

  cJSON *jtree = NULL;
  char filepath[PATH_LEN];
  int fuel_id = 0;
  char *region_states[REGIONS];
  char match_state[10]; // space for vertical bars

  // look up the region from the top->rer.state if not given the region directly, MJF 1/2019

  if (top->rer.region == 0) {
    if (top->rer.state[0] == '\0' && top->wth.state[0] != '\0')
      STRCPY(top->rer.state, top->wth.state);
    region_states[0] = "|CT|MN|MA|NH|NJ|NY|PA|RI|VT|";
    region_states[1] = "|IL|IN|IA|KS|MI|MN|MO|NE|ND|OH|SD|WI|";
    region_states[2] = "|AL|AR|DE|DC|FL|GA|KY|LA|MD|MS|NC|OK|SC|TN|TX|VA|WV|";
    region_states[3] = "|AK|AZ|CA|CO|HI|ID|MT|NV|NM|OR|UT|WA|WY|";
    region_states[4] = "|US|USA|ANY|";
    STRCPY(match_state, "|");
    STRCAT(match_state, top->rer.state);
    STRCAT(match_state, "|");
    if (top->rer.region == 0 && top->rer.state[0] != '\0') {
      for (int i = 0; i < REGIONS; i++) {
        if (strstr(region_states[i], strupr(match_state))) {
          top->rer.region = i + 1;
          break;
        }
      }
    }
  }
  ASSERT(top->rer.region,
         sprintf(msg, "No fuel cost escalation rates for state: %s region: %d", top->rer.state, top->rer.region));

  sprintf(filepath, ESCALATION_DIR "%4d_%02d.json", top->rer.year, top->rer.region);
  if (cmds.debug_level & D_NORMAL)
    fprintf(stderr, "\nReading referenced fuel cost escalation rates from file: %s", filepath);

  jtree = parse_json_file(filepath);

  ASSERT(jtree, sprintf(msg, "Problem parsing the referenced escalation rate file:%s", filepath));

  char *section_name = "fuel_escalation_rates";
  cJSON_ArrayForEach(jbranch, jtree) {
    if (strcmp(strlwr(jbranch->string), section_name) != 0)
      continue; // allow other non 'bld' elements like comments etc
    JI_ARR_BEG(fuel_escalation_rates, top->num_fer);
      ENU_ASSIGN(fer, &fuel_id, fuel_type_id);
      STR_ASSIGN(fer, top->fer[fuel_id - 1].fuelname, fuel_name);
      char *field_name = "rate";
      if (strcmp(strlwr(jleaf->string), field_name) == 0) {
        int year = 0;
        cJSON_ArrayForEach(jleaf2, jleaf) {
          top->fer[fuel_id - 1].rates[year] = (float)jleaf2->valuedouble;
          year++;
        }
        continue; // because we found the rate section
      }
    JI_ARR_END();
  }

  // clean up
  if (jleaf)
    cJSON_Delete(jleaf);
  if (jleaf2)
    cJSON_Delete(jleaf2);
  if (jbranch)
    cJSON_Delete(jbranch);
  if (jbranch2)
    cJSON_Delete(jbranch2);

  return;
}

/// Reads MDI JSON input and assigns to the MDI struct. Assigns the JSON data to our MDI struct doing data checks
/// as well, both range and repeat (number of record) MIN and MAX checks.
/// Some notes on this implementation:
/// 1) This mirrors information in dimmensions, structures, and enumberations for MHEA
///    any changes there need to be mirrored here. The order of input follows the
///    definition of MDI in mhea/dwelling.h
/// 2) Range check values are taken from the current NEAT Web GUI.  Open the associated
///    form in the current UI and call showMinMaxForCurrentForm() in the browder console to display range values.
/// 3) IMPORTANT, the assignments only run IF there is a matching JSON element within the named
///    structure, OTHERWISE, the top->xxx.yyyyyy remains in the state left by the calloc() call (all set to zero bytes)
///    So, default values are zero and null strings
/// 4) This code is MOSTLY order independent both the ordering of sections and the ordering of elements
///    within a section. The exceptions are where the J_ Macros are not used or local index variables are declared.  Those
///    sections are order dependent

void mhea_json_read(MDI *top, cJSON *jtree, cJSON *jschema) {
  cJSON *jleaf = NULL;
  cJSON *jleaf2 = NULL;
  cJSON *jbranch = NULL;
  cJSON *jbranch2 = NULL;

  ASSERT(top, sprintf(msg, "Must have the MHEA MDI memory allocated"));
  ASSERT(jtree, sprintf(msg, "Must have the JSON structure filled in"));
  
  cJSON_ArrayForEach(jbranch, jtree) {

    J_SEC_BEG(audit);
      J_STR_ASSIGN(audit, gnl, audit_type);
      J_LNG_ASSIGN(audit, gnl, audit_id);
      J_LNG_ASSIGN(audit, gnl, audit_number);
      J_FLT_ASSIGN(audit, gnl, avg_no_occupants);
      J_FLT_ASSIGN(audit, gnl, length);
      J_FLT_ASSIGN(audit, gnl, width);
      J_FLT_ASSIGN(audit, gnl, height);
      J_BOO_ASSIGN(audit, gnl, do_billing_adjust);
      J_BOO_ASSIGN(audit, gnl, water_heater_closet);
      J_ENU_ASSIGN(audit, gnl, wind_shielding);
      J_ENU_ASSIGN(audit, gnl, leakiness);
    J_SEC_END();

    J_SEC_BEG(weather_location);
      J_STR_ASSIGN(weather_location, wth, state);
      J_STR_ASSIGN(weather_location, wth, city);
      J_STR_ASSIGN(weather_location, wth, file);
    J_SEC_END();

  // MAIN HOME SHELL 

    J_SEC_BEG(walls);
      J_ENU_ASSIGN(walls, wal, stud_size);
      J_ENU_ASSIGN(walls, wal, home_orientation);
      J_ENU_ASSIGN(walls, wal, wall_vent);
      J_FLT_ASSIGN(walls, wal, batt_insl);
      J_FLT_ASSIGN(walls, wal, loose_insl);
      J_FLT_ASSIGN(walls, wal, foam_insl);
      J_FLT_ASSIGN(walls, wal, uninsulatable_area);
      J_FLT_ASSIGN(walls, wal, add_cost);
      J_FLT_ASSIGN(walls, wal, porch_length);
      J_FLT_ASSIGN(walls, wal, porch_width);
      J_ENU_ASSIGN(walls, wal, porch_orientation);
    J_SEC_END();

    JI_ARR_BEG(windows, top->num_win);
      JI_STR_ASSIGN(windows, win, code);
      JI_ENU_ASSIGN(windows, win, window_type);
      JI_ENU_ASSIGN(windows, win, frame_type);
      JI_ENU_ASSIGN(windows, win, glazing_type);
      JI_ENU_ASSIGN(windows, win, int_shading);
      JI_ENU_ASSIGN(windows, win, ext_shading);
      JI_ENU_ASSIGN(windows, win, leak);
      JI_FLT_ASSIGN(windows, win, width);
      JI_FLT_ASSIGN(windows, win, height);
      JI_FLT_ASSIGN(windows, win, num_n);
      JI_FLT_ASSIGN(windows, win, num_s);
      JI_FLT_ASSIGN(windows, win, num_e);
      JI_FLT_ASSIGN(windows, win, num_w);
      JI_ENU_ASSIGN(windows, win, retrofit_option);
      JI_BOO_ASSIGN(windows, win, inc_sir);
      JI_FLT_ASSIGN(windows, win, cost_seal);
      JI_FLT_ASSIGN(windows, win, cost_replace);
      JI_FLT_ASSIGN(windows, win, cost_add_glass_storm);
      JI_FLT_ASSIGN(windows, win, cost_add_plastic_storm);
    JI_ARR_END();

    JI_ARR_BEG(doors, top->num_dor);
      JI_STR_ASSIGN(doors, dor, code);
      JI_ENU_ASSIGN(doors, dor, door_type);
      JI_ENU_ASSIGN(doors, dor, leakiness);
      JI_BOO_ASSIGN(doors, dor, storm);
      JI_FLT_ASSIGN(doors, dor, width);
      JI_FLT_ASSIGN(doors, dor, height);
      JI_FLT_ASSIGN(doors, dor, num_n);
      JI_FLT_ASSIGN(doors, dor, num_s);
      JI_FLT_ASSIGN(doors, dor, num_e);
      JI_FLT_ASSIGN(doors, dor, num_w);
      JI_BOO_ASSIGN(doors, dor, replace);
      JI_BOO_ASSIGN(doors, dor, inc_sir);
      JI_FLT_ASSIGN(doors, dor, cost_replace);
    JI_ARR_END();

    J_SEC_BEG(ceiling);
      J_ENU_ASSIGN(ceiling, rof, roof_type);
      J_ENU_ASSIGN(ceiling, rof, roof_color);
      J_ENU_ASSIGN(ceiling, rof, joist_size);
      J_FLT_ASSIGN(ceiling, rof, ceiling_height);
      J_FLT_ASSIGN(ceiling, rof, pitch_add_insl);
      J_FLT_ASSIGN(ceiling, rof, mineral_insl);
      J_FLT_ASSIGN(ceiling, rof, loose_insl);
      J_FLT_ASSIGN(ceiling, rof, rigid_insl);
      J_FLT_ASSIGN(ceiling, rof, cathedral_ceiling);
      J_ENU_ASSIGN(ceiling, rof, step_wall);
      J_FLT_ASSIGN(ceiling, rof, add_cost);
    J_SEC_END();

    J_SEC_BEG(floor);
      J_BOO_ASSIGN(floor, flr, skirt);
      J_ENU_ASSIGN(floor, flr, wing_joist_size);
      J_FLT_ASSIGN(floor, flr, wing_mineral_insl);
      J_FLT_ASSIGN(floor, flr, wing_loose_insl);
      J_ENU_ASSIGN(floor, flr, wing_insl_location);
      J_ENU_ASSIGN(floor, flr, belly_joist_size);
      J_FLT_ASSIGN(floor, flr, belly_mineral_insl);
      J_FLT_ASSIGN(floor, flr, belly_loose_insl);
      J_ENU_ASSIGN(floor, flr, belly_insl_location);
      J_ENU_ASSIGN(floor, flr, belly_condition);
      J_ENU_ASSIGN(floor, flr, belly_cavity);
      J_FLT_ASSIGN(floor, flr, belly_depth);
      J_FLT_ASSIGN(floor, flr, add_cost);
    J_SEC_END();

  // ANY HOME ADDITIONS SHELL INFORMAITON

    J_SEC_BEG(walls_addition);
      J_ENU_ASSIGN(walls_addition, awl, stud_size);
      J_ENU_ASSIGN(walls_addition, awl, orientation);
      J_ENU_ASSIGN(walls_addition, awl, wall_vent);
      J_FLT_ASSIGN(walls_addition, awl, batt_insl);
      J_FLT_ASSIGN(walls_addition, awl, loose_insl);
      J_FLT_ASSIGN(walls_addition, awl, foam_insl);
      J_ENU_ASSIGN(walls_addition, awl, wall_config);
      J_FLT_ASSIGN(walls_addition, awl, height_max);
      J_FLT_ASSIGN(walls_addition, awl, height_min);
      J_FLT_ASSIGN(walls_addition, awl, add_cost);
    J_SEC_END();

    JI_ARR_BEG(windows_addition, top->num_awn);
      JI_STR_ASSIGN(windows_addition, awn, code);
      JI_ENU_ASSIGN(windows_addition, awn, window_type);
      JI_ENU_ASSIGN(windows_addition, awn, frame_type);
      JI_ENU_ASSIGN(windows_addition, awn, glazing_type);
      JI_ENU_ASSIGN(windows_addition, awn, int_shading);
      JI_ENU_ASSIGN(windows_addition, awn, ext_shading);
      JI_ENU_ASSIGN(windows_addition, awn, leak);
      JI_FLT_ASSIGN(windows_addition, awn, width);
      JI_FLT_ASSIGN(windows_addition, awn, height);
      JI_FLT_ASSIGN(windows_addition, awn, num_n);
      JI_FLT_ASSIGN(windows_addition, awn, num_s);
      JI_FLT_ASSIGN(windows_addition, awn, num_e);
      JI_FLT_ASSIGN(windows_addition, awn, num_w);
      JI_ENU_ASSIGN(windows_addition, awn, retrofit_option);
      JI_BOO_ASSIGN(windows_addition, awn, inc_sir);
      JI_FLT_ASSIGN(windows_addition, awn, cost_seal);
      JI_FLT_ASSIGN(windows_addition, awn, cost_replace);
      JI_FLT_ASSIGN(windows_addition, awn, cost_add_glass_storm);
      JI_FLT_ASSIGN(windows_addition, awn, cost_add_plastic_storm);
    JI_ARR_END();

    JI_ARR_BEG(doors_addition, top->num_adr);
      JI_STR_ASSIGN(doors_addition, adr, code);
      JI_ENU_ASSIGN(doors_addition, adr, door_type);
      JI_ENU_ASSIGN(doors_addition, adr, leakiness);
      JI_BOO_ASSIGN(doors_addition, adr, storm);
      JI_FLT_ASSIGN(doors_addition, adr, width);
      JI_FLT_ASSIGN(doors_addition, adr, height);
      JI_FLT_ASSIGN(doors_addition, adr, num_n);
      JI_FLT_ASSIGN(doors_addition, adr, num_s);
      JI_FLT_ASSIGN(doors_addition, adr, num_e);
      JI_FLT_ASSIGN(doors_addition, adr, num_w);
      JI_BOO_ASSIGN(doors_addition, adr, replace);
      JI_BOO_ASSIGN(doors_addition, adr, inc_sir);
      JI_FLT_ASSIGN(doors_addition, adr, cost_replace);
    JI_ARR_END();

    J_SEC_BEG(ceiling_addition);
      J_ENU_ASSIGN(addition_ceiling, arf, roof_color);
      J_ENU_ASSIGN(addition_ceiling, arf, joist_size);
      J_FLT_ASSIGN(addition_ceiling, arf, mineral_insl);
      J_FLT_ASSIGN(addition_ceiling, arf, loose_insl);
      J_FLT_ASSIGN(addition_ceiling, arf, rigid_insl);
      J_FLT_ASSIGN(addition_ceiling, arf, add_cost);
    J_SEC_END();

    J_SEC_BEG(floor_addition);
      J_ENU_ASSIGN(floor_addition, afl, add_floor_type);
      J_ENU_ASSIGN(floor_addition, afl, joist_size);
      J_ENU_ASSIGN(floor_addition, afl, insl_location);
      J_FLT_ASSIGN(floor_addition, afl, length);
      J_FLT_ASSIGN(floor_addition, afl, width);
      J_FLT_ASSIGN(floor_addition, afl, mineral_insl);
      J_FLT_ASSIGN(floor_addition, afl, loose_insl);
      J_FLT_ASSIGN(floor_addition, afl, avail_insl);
    J_SEC_END();

    J_SEC_BEG(heating_primary);
      J_ENU_ASSIGN(heating_primary, htg, equip_type);
      J_ENU_ASSIGN(heating_primary, htg, fuel_type);
      J_FLT_ASSIGN(heating_primary, htg, capacity);
      J_FLT_ASSIGN(heating_primary, htg, efficiency_percent);
      J_FLT_ASSIGN(heating_primary, htg, efficiency_hspf);
      J_FLT_ASSIGN(heating_primary, htg, efficiency_cop);
      J_ENU_ASSIGN(heating_primary, htg, eff_units);
      J_ENU_ASSIGN(heating_primary, htg, duct_location);
      J_ENU_ASSIGN(heating_primary, htg, duct_insl);
      J_FLT_ASSIGN(heating_primary, htg, percent_heated);
      J_BOO_ASSIGN(heating_primary, htg, smart_thermostat);
      J_BOO_ASSIGN(heating_primary, htg, tuneup);
      J_BOO_ASSIGN(heating_primary, htg, inc_sir);
    J_SEC_END();

    J_SEC_BEG(heating_secondary);
      J_ENU_ASSIGN(heating_secondary, ht2, equip_type);
      J_ENU_ASSIGN(heating_secondary, ht2, fuel_type);
      J_FLT_ASSIGN(heating_secondary, ht2, capacity);
      J_FLT_ASSIGN(heating_secondary, ht2, efficiency_percent);
      J_FLT_ASSIGN(heating_secondary, ht2, efficiency_hspf);
      J_FLT_ASSIGN(heating_secondary, ht2, efficiency_cop);
      J_ENU_ASSIGN(heating_secondary, ht2, eff_units);
      //J_ENU_ASSIGN(heating_secondary, ht2, duct_location);
      //J_ENU_ASSIGN(heating_secondary, ht2, duct_insl);
    J_SEC_END();

    J_SEC_BEG(heating_replacement);
      J_ENU_ASSIGN(heating_replacement, htr, equip_type);
      J_ENU_ASSIGN(heating_replacement, htr, fuel_type);
      J_FLT_ASSIGN(heating_replacement, htr, capacity);
      J_FLT_ASSIGN(heating_replacement, htr, efficiency_percent);
      J_FLT_ASSIGN(heating_replacement, htr, efficiency_hspf);
      J_FLT_ASSIGN(heating_replacement, htr, efficiency_cop);
      J_ENU_ASSIGN(heating_replacement, htr, eff_units);
      J_ENU_ASSIGN(heating_replacement, htr, duct_location);
      J_ENU_ASSIGN(heating_replacement, htr, duct_insl);
      J_BOO_ASSIGN(heating_replacement, htr, replacement);
      J_BOO_ASSIGN(heating_replacement, htr, incl_costs);
      J_FLT_ASSIGN(heating_replacement, htr, labor_cost);
      J_FLT_ASSIGN(heating_replacement, htr, material_cost);
    J_SEC_END();

    J_SEC_BEG(cooling_primary);
      J_ENU_ASSIGN(cooling_primary, clg, equip_type);
      J_FLT_ASSIGN(cooling_primary, clg, capacity);
      J_FLT_ASSIGN(cooling_primary, clg, efficiency_eer);
      J_FLT_ASSIGN(cooling_primary, clg, efficiency_seer);
      J_FLT_ASSIGN(cooling_primary, clg, efficiency_cop);
      J_ENU_ASSIGN(cooling_primary, clg, eff_units);
      J_ENU_ASSIGN(cooling_primary, clg, duct_location);
      J_ENU_ASSIGN(cooling_primary, clg, duct_insl);
      J_FLT_ASSIGN(cooling_primary, clg, percent_area_room_ac);
      J_BOO_ASSIGN(cooling_primary, clg, tuneup);
      J_BOO_ASSIGN(cooling_primary, clg, inc_sir);
    J_SEC_END();

    J_SEC_BEG(cooling_secondary);
      J_ENU_ASSIGN(cooling_secondary, cl2, equip_type);
      J_FLT_ASSIGN(cooling_secondary, cl2, capacity);
      J_FLT_ASSIGN(cooling_secondary, cl2, efficiency_eer);
      J_FLT_ASSIGN(cooling_secondary, cl2, efficiency_seer);
      J_FLT_ASSIGN(cooling_secondary, cl2, efficiency_cop);
      J_ENU_ASSIGN(cooling_secondary, cl2, eff_units);
      J_FLT_ASSIGN(cooling_secondary, cl2, percent_area_room_ac);
    J_SEC_END();

    J_SEC_BEG(cooling_replacement);
      J_ENU_ASSIGN(cooling_replacement, clr, equip_type);
      J_FLT_ASSIGN(cooling_replacement, clr, capacity);
      J_FLT_ASSIGN(cooling_replacement, clr, efficiency_eer);
      J_FLT_ASSIGN(cooling_replacement, clr, efficiency_seer);
      J_FLT_ASSIGN(cooling_replacement, clr, efficiency_cop);
      J_ENU_ASSIGN(cooling_replacement, clr, eff_units);
      J_ENU_ASSIGN(cooling_replacement, clr, clg_duct_location);
      J_ENU_ASSIGN(cooling_replacement, clr, clg_duct_insl);
      J_BOO_ASSIGN(cooling_replacement, clr, replacement);
      J_BOO_ASSIGN(cooling_replacement, clr, incl_costs);
      J_FLT_ASSIGN(cooling_replacement, clr, labor_cost);
      J_FLT_ASSIGN(cooling_replacement, clr, material_cost);
    J_SEC_END();

    J_SEC_BEG(ducts_and_infiltration);
      J_BOO_ASSIGN(ducts_and_infiltration, inf, evaluate_duct_sealing);
      J_ENU_ASSIGN(ducts_and_infiltration, inf, duct_seal_method);
      J_FLT_ASSIGN(ducts_and_infiltration, inf, air_leak_red_cost);
      J_FLT_ASSIGN(ducts_and_infiltration, inf, duct_seal_cost);
      J_FLT_ASSIGN(ducts_and_infiltration, inf, pre_inf_cfm);
      J_FLT_ASSIGN(ducts_and_infiltration, inf, pre_inf_pa);
      J_FLT_ASSIGN(ducts_and_infiltration, inf, post_inf_cfm);
      J_FLT_ASSIGN(ducts_and_infiltration, inf, post_inf_pa);
      J_FLT_ASSIGN(ducts_and_infiltration, inf, pre_duct_seal_supply_pa);
      //J_FLT_ASSIGN(ducts_and_infiltration, inf, pre_duct_seal_return_pa);
      J_FLT_ASSIGN(ducts_and_infiltration, inf, post_duct_seal_supply_pa);
      //J_FLT_ASSIGN(ducts_and_infiltration, inf, post_duct_seal_return_pa);
      J_FLT_ASSIGN(ducts_and_infiltration, inf, post_duct_seal_cfm);
      J_FLT_ASSIGN(ducts_and_infiltration, inf, post_duct_seal_pa);
      J_FLT_ASSIGN(ducts_and_infiltration, inf, pre_duct_seal_close_cfm);
      J_FLT_ASSIGN(ducts_and_infiltration, inf, pre_duct_seal_close_pa);
      J_FLT_ASSIGN(ducts_and_infiltration, inf, pre_duct_seal_close_diff_pa);
      J_FLT_ASSIGN(ducts_and_infiltration, inf, post_duct_seal_close_cfm);
      J_FLT_ASSIGN(ducts_and_infiltration, inf, post_duct_seal_close_pa);
      J_FLT_ASSIGN(ducts_and_infiltration, inf, post_duct_seal_close_diff_pa);
      J_FLT_ASSIGN(ducts_and_infiltration, inf, pre_duct_seal_tot_cfm);
      J_FLT_ASSIGN(ducts_and_infiltration, inf, pre_duct_seal_tot_duct_pa);
      J_FLT_ASSIGN(ducts_and_infiltration, inf, pre_duct_seal_out_cfm);
      J_FLT_ASSIGN(ducts_and_infiltration, inf, pre_duct_seal_out_duct_pa);
      J_FLT_ASSIGN(ducts_and_infiltration, inf, post_duct_seal_tot_cfm);
      J_FLT_ASSIGN(ducts_and_infiltration, inf, post_duct_seal_tot_duct_pa);
      J_FLT_ASSIGN(ducts_and_infiltration, inf, post_duct_seal_out_cfm);
      J_FLT_ASSIGN(ducts_and_infiltration, inf, post_duct_seal_out_duct_pa);
    J_SEC_END();

    J_SEC_BEG(water_heating);
      J_ENU_ASSIGN(water_heating, dwh, exist_tank_location_id);
      J_ENU_ASSIGN(water_heating, dwh, exist_fuel_type_id);
      J_ENU_ASSIGN(water_heating, dwh, exist_type);
      J_FLT_ASSIGN(water_heating, dwh, exist_gal);
      J_FLT_ASSIGN(water_heating, dwh, exist_uniform_energy_factor);
      J_FLT_ASSIGN(water_heating, dwh, exist_energy_factor);
      J_FLT_ASSIGN(water_heating, dwh, exist_recovery_efficiency);
      J_ENU_ASSIGN(water_heating, dwh, exist_input_units_id);
      J_FLT_ASSIGN(water_heating, dwh, exist_input);
      J_FLT_ASSIGN(water_heating, dwh, exist_rvalue);
      J_FLT_ASSIGN(water_heating, dwh, exist_insul_thick);
      J_ENU_ASSIGN(water_heating, dwh, exist_insul_type_id);
      J_BOO_ASSIGN(water_heating, dwh, exist_pipe_insul);
      J_BOO_ASSIGN(water_heating, dwh, exist_tank_wrap);
      J_INT_ASSIGN(water_heating, dwh, shower_heads);
      J_FLT_ASSIGN(water_heating, dwh, shower_usage_per_day);
      J_FLT_ASSIGN(water_heating, dwh, shower_gpm);
      J_STR_ASSIGN(water_heating, dwh, replace_manufacturer);
      J_STR_ASSIGN(water_heating, dwh, replace_model);
      J_ENU_ASSIGN(water_heating, dwh, replace_fuel_type_id);
      J_ENU_ASSIGN(water_heating, dwh, replace_type);
      J_FLT_ASSIGN(water_heating, dwh, replace_gal);
      J_FLT_ASSIGN(water_heating, dwh, replace_uniform_energy_factor);
      J_FLT_ASSIGN(water_heating, dwh, replace_recovery_efficiency);
      J_ENU_ASSIGN(water_heating, dwh, replace_input_units_id);
      J_FLT_ASSIGN(water_heating, dwh, replace_input); // range ok for kBTU or KW input units
      J_INT_ASSIGN(water_heating, dwh, replace_life);
      J_FLT_ASSIGN(water_heating, dwh, replace_install_cost);
      J_FLT_ASSIGN(water_heating, dwh, replace_added_cost);
      J_BOO_ASSIGN(water_heating, dwh, replace);
      J_BOO_ASSIGN(water_heating, dwh, inc_sir);
    J_SEC_END();

    J_SEC_BEG(refrigerators);
      J_ENU_ASSIGN(refrigerators, ref, location_id);
      J_ENU_ASSIGN(refrigerators, ref, label_year_id);
      J_ENU_ASSIGN(refrigerators, ref, door_seal_condition_id);
      J_FLT_ASSIGN(refrigerators, ref, label_kwh_per_year);
      J_FLT_ASSIGN(refrigerators, ref, meter_energy_reading);
      J_FLT_ASSIGN(refrigerators, ref, meter_energy_interval);
      J_BOO_ASSIGN(refrigerators, ref, meter_manual_defrost);
      J_BOO_ASSIGN(refrigerators, ref, meter_includes_defrost);
      J_FLT_ASSIGN(refrigerators, ref, meter_temperature);
      J_STR_ASSIGN(refrigerators, ref, replace_manufacturer);
      J_STR_ASSIGN(refrigerators, ref, replace_model);
      J_FLT_ASSIGN(refrigerators, ref, replace_kwh_per_year);
      J_INT_ASSIGN(refrigerators, ref, replace_life);
      J_FLT_ASSIGN(refrigerators, ref, replace_install_cost);
      J_FLT_ASSIGN(refrigerators, ref, replace_added_cost);
    J_SEC_END();

    JI_ARR_BEG(lighting, top->num_ltg);
      JI_STR_ASSIGN(lighting, ltg, code);
      JI_ENU_ASSIGN(lighting, ltg, exist_lamp_type);
      JI_INT_ASSIGN(lighting, ltg, exist_lamp_count);
      JI_FLT_ASSIGN(lighting, ltg, exist_lamp_watts);
      JI_FLT_ASSIGN(lighting, ltg, exist_hours_per_day);
      JI_ENU_ASSIGN(lighting, ltg, new_lamp_type);
      JI_INT_ASSIGN(lighting, ltg, new_lamp_count);
      JI_FLT_ASSIGN(lighting, ltg, new_lamp_watts);
      JI_FLT_ASSIGN(lighting, ltg, new_hours_per_day);
      JI_FLT_ASSIGN(lighting, ltg, new_lifetime_hrs);
      JI_FLT_ASSIGN(lighting, ltg, install_cost_per_lamp);
      JI_FLT_ASSIGN(lighting, ltg, added_cost_per_lamp);
      JI_FLT_ASSIGN(lighting, ltg, added_cost);
    JI_ARR_END();

    JI_ARR_BEG(itemized_costs, top->num_itc);
      JI_STR_ASSIGN(itemized_costs, itc, measure);
      JI_INT_ASSIGN(itemized_costs, itc, component_id);
      JI_FLT_ASSIGN(itemized_costs, itc, cost);
      JI_BOO_ASSIGN(itemized_costs, itc, inc_sir);
      JI_STR_ASSIGN(itemized_costs, itc, material);
      JI_ENU_ASSIGN(itemized_costs, itc, units);
      JI_FLT_ASSIGN(itemized_costs, itc, savings);
      JI_ENU_ASSIGN(itemized_costs, itc, fuel);
      JI_FLT_ASSIGN(itemized_costs, itc, life);
    JI_ARR_END();

    J_SEC_BEG(utility_bills_pre_retrofit_heating);
      J_ENU_ASSIGN(utility_bills_pre_retrofit_heating, ubh, usage_units);
      J_INT_ASSIGN(utility_bills_pre_retrofit_heating, ubh, period_days);
      J_FLT_ASSIGN(utility_bills_pre_retrofit_heating, ubh, base_temp);
      J_FLT_ASSIGN(utility_bills_pre_retrofit_heating, ubh, base_load);
    J_SEC_END();

    JI_ARR_BEG(utility_bills_pre_retrofit_heating_data, top->num_urh);
      JI_INT_ASSIGN(utility_bills_pre_retrofit_heating_data, urh, year);
      JI_INT_ASSIGN(utility_bills_pre_retrofit_heating_data, urh, month);
      JI_INT_ASSIGN(utility_bills_pre_retrofit_heating_data, urh, day);
      JI_FLT_ASSIGN(utility_bills_pre_retrofit_heating_data, urh, usage);
      JI_FLT_ASSIGN(utility_bills_pre_retrofit_heating_data, urh, degree_days);
    JI_ARR_END();

    J_SEC_BEG(utility_bills_pre_retrofit_cooling);
      //J_ENU_ASSIGN(utility_bills_pre_retrofit_cooling, ubc, usage_units);
      J_INT_ASSIGN(utility_bills_pre_retrofit_cooling, ubc, period_days);
      J_FLT_ASSIGN(utility_bills_pre_retrofit_cooling, ubc, base_temp);
      J_FLT_ASSIGN(utility_bills_pre_retrofit_cooling, ubc, base_load);
    J_SEC_END();

    JI_ARR_BEG(utility_bills_pre_retrofit_cooling_data, top->num_urc);
      JI_INT_ASSIGN(utility_bills_pre_retrofit_cooling_data, urc, year);
      JI_INT_ASSIGN(utility_bills_pre_retrofit_cooling_data, urc, month);
      JI_INT_ASSIGN(utility_bills_pre_retrofit_cooling_data, urc, day);
      JI_FLT_ASSIGN(utility_bills_pre_retrofit_cooling_data, urc, usage);
      JI_FLT_ASSIGN(utility_bills_pre_retrofit_cooling_data, urc, degree_days);
    JI_ARR_END();

    J_SEC_BEG(fuel_costs);
      J_FLT_ASSIGN(fuel_costs, fcs, natural_gas);
      J_FLT_ASSIGN(fuel_costs, fcs, oil);
      J_FLT_ASSIGN(fuel_costs, fcs, electric);
      J_FLT_ASSIGN(fuel_costs, fcs, propane);
      J_FLT_ASSIGN(fuel_costs, fcs, wood);
      J_FLT_ASSIGN(fuel_costs, fcs, coal);
      J_FLT_ASSIGN(fuel_costs, fcs, kerosene);
      J_FLT_ASSIGN(fuel_costs, fcs, other);
      J_FLT_ASSIGN(fuel_costs, fcs, natural_gas_heat);
      J_FLT_ASSIGN(fuel_costs, fcs, oil_heat);
      J_FLT_ASSIGN(fuel_costs, fcs, electric_heat);
      J_FLT_ASSIGN(fuel_costs, fcs, propane_heat);
      J_FLT_ASSIGN(fuel_costs, fcs, wood_heat);
      J_FLT_ASSIGN(fuel_costs, fcs, coal_heat);
      J_FLT_ASSIGN(fuel_costs, fcs, kerosene_heat);
      J_FLT_ASSIGN(fuel_costs, fcs, other_heat);
    J_SEC_END();


    { 
    int fuel_id = 0;
    JI_ARR_BEG(fuel_escalation_rates, top->num_fer);
      ENU_ASSIGN(fuel_escalation_rates, &fuel_id, fuel_type_id);  // used as index (base 0) into top->fer[x]
      STR_ASSIGN(fuel_escalation_rates, top->fer[fuel_id - 1].fuelname, fuel_name);
      char *field_name = "rate";
      if (strcmp(strlwr(jleaf->string), field_name) == 0) {
        int year = 0;
        cJSON_ArrayForEach(jleaf2, jleaf) {
          top->fer[fuel_id - 1].rates[year] = (float)jleaf2->valuedouble;
          year++;
        }
        continue; // because we found the rate section
      }
    JI_ARR_END();
    }

    J_SEC_BEG(fuel_escalation_rates_by_reference);
      J_INT_ASSIGN(fuel_escalation_rates_by_reference, rer, year);
      J_STR_ASSIGN(fuel_escalation_rates_by_reference, rer, state);
      J_INT_ASSIGN(fuel_escalation_rates_by_reference, rer, region);
    J_SEC_END()

    {
    int id = 0;
    JI_ARR_BEG(measure_active_flags, top->num_cms); 
      INT_ASSIGN(measure_active_flags, &id, id);
      top->cms[id].id = id;
      BOO_ASSIGN(measure_active_flags, &top->cms[id].active, active);
      STR_ASSIGN(measure_active_flags, top->cms[id].measure_name, measure_name);
    JI_ARR_END();
    }

    {
    int id = 0;
    JI_ARR_BEG(measure_costs, top->num_rmc);
      INT_ASSIGN(measure_costs, &id, id);
      top->rmc[id].measure_id = id;
      STR_ASSIGN(measure_costs, top->rmc[id].retro_name, retro_name);
      //STR_ASSIGN(measure_costs, top->rmc[id].retrofit_type, retrofit_type);
      FLT_ASSIGN(measure_costs, &top->rmc[id].life, life);
      STR_ASSIGN(measure_costs, top->rmc[id].units, units);
      FLT_ASSIGN(measure_costs, &top->rmc[id].material, material);
      FLT_ASSIGN(measure_costs, &top->rmc[id].labor, labor);
      FLT_ASSIGN(measure_costs, &top->rmc[id].extra, extra);
    JI_ARR_END();
    }

    J_SEC_BEG(key_parameters);
      J_FLT_ASSIGN(key_parameters, key, real_discount_rate);
      J_FLT_ASSIGN(key_parameters, key, minimum_acceptable_sir);
      J_FLT_ASSIGN(key_parameters, key, spending_limit);

      J_FLT_ASSIGN(key_parameters, key, heating_setpoint_day);
      J_FLT_ASSIGN(key_parameters, key, heating_setpoint_night);
      J_FLT_ASSIGN(key_parameters, key, cooling_setpoint_day);
      J_FLT_ASSIGN(key_parameters, key, cooling_setpoint_night);
      J_FLT_ASSIGN(key_parameters, key, thermostat_setback_amount);
      J_FLT_ASSIGN(key_parameters, key, length_of_night_thermostat_setback);

      J_FLT_ASSIGN(key_parameters, key, bag_size_for_loose_fiberglass_insulation);
      J_FLT_ASSIGN(key_parameters, key, density_of_loose_fiberglass_insulation);
      J_FLT_ASSIGN(key_parameters, key, bag_size_for_loose_cellulose_insulation);
      J_FLT_ASSIGN(key_parameters, key, density_of_loose_cellulose_insulation);
      J_FLT_ASSIGN(key_parameters, key, interior_wall_r_value_winter);
      J_FLT_ASSIGN(key_parameters, key, interior_wall_r_value_summer);
      J_FLT_ASSIGN(key_parameters, key, interior_ceiling_r_value_winter);
      J_FLT_ASSIGN(key_parameters, key, interior_ceiling_r_value_summer);
      J_FLT_ASSIGN(key_parameters, key, interior_floor_r_value_winter);
      J_FLT_ASSIGN(key_parameters, key, interior_floor_r_value_summer);
      J_FLT_ASSIGN(key_parameters, key, outside_wall_r_value_winter);
      J_FLT_ASSIGN(key_parameters, key, outside_wall_r_value_summer);
      J_FLT_ASSIGN(key_parameters, key, loose_insulation_r_value_per_inch);
      J_FLT_ASSIGN(key_parameters, key, batt_blanket_insulation_r_value_per_inch);
      J_FLT_ASSIGN(key_parameters, key, rigid_insulation_r_value_per_inch);
      J_FLT_ASSIGN(key_parameters, key, foamcore_insulation_r_value_per_inch);

      J_FLT_ASSIGN(key_parameters, key, home_leakiness_loose);
      J_FLT_ASSIGN(key_parameters, key, home_leakiness_medium);
      J_FLT_ASSIGN(key_parameters, key, home_leakiness_tight);
      J_FLT_ASSIGN(key_parameters, key, free_heat_from_interior_sources_day);
      J_FLT_ASSIGN(key_parameters, key, free_heat_from_interior_sources_night);
      J_FLT_ASSIGN(key_parameters, key, duct_sealing_distribution_loss_reduction);
      J_FLT_ASSIGN(key_parameters, key, duct_insulation_dist_loss_reduction);
      J_FLT_ASSIGN(key_parameters, key, evaporative_cooler_actual_saturating_eff);
      J_FLT_ASSIGN(key_parameters, key, saturating_eff_for_evaporative_tune_up);
      J_FLT_ASSIGN(key_parameters, key, saturating_eff_for_evaporative_rplcmnt);
      J_FLT_ASSIGN(key_parameters, key, cooling_system_fan_power);

      J_FLT_ASSIGN(key_parameters, key, door_u_value_wood_with_hollow_core);
      J_FLT_ASSIGN(key_parameters, key, door_u_value_wood_with_solid_core);
      J_FLT_ASSIGN(key_parameters, key, door_u_value_standard_mfg_home_door);
      J_FLT_ASSIGN(key_parameters, key, u_value_of_replacement_door);

      J_FLT_ASSIGN(key_parameters, key, window_u_value_1_glazing_winter);
      J_FLT_ASSIGN(key_parameters, key, window_u_value_1_glazing_summer);
      J_FLT_ASSIGN(key_parameters, key, window_u_value_2_glazing_winter);
      J_FLT_ASSIGN(key_parameters, key, window_u_value_2_glazing_summer);
      J_FLT_ASSIGN(key_parameters, key, window_u_value_1_glass_storm_winter);
      J_FLT_ASSIGN(key_parameters, key, window_u_value_1_glass_storm_summer);
      J_FLT_ASSIGN(key_parameters, key, window_u_value_2_glass_storm_winter);
      J_FLT_ASSIGN(key_parameters, key, window_u_value_2_glass_storm_summer);
      J_FLT_ASSIGN(key_parameters, key, window_u_value_1_plastic_storm_winter);
      J_FLT_ASSIGN(key_parameters, key, window_u_value_1_plastic_storm_summer);
      J_FLT_ASSIGN(key_parameters, key, window_u_value_2_plastic_storm_winter);
      J_FLT_ASSIGN(key_parameters, key, window_u_value_2_plastic_storm_summer);
      J_FLT_ASSIGN(key_parameters, key, skylight_u_value_1_glazing_winter);
      J_FLT_ASSIGN(key_parameters, key, skylight_u_value_1_glazing_summer);
      J_FLT_ASSIGN(key_parameters, key, skylight_u_value_2_glazing_winter);
      J_FLT_ASSIGN(key_parameters, key, skylight_u_value_2_glazing_summer);
      J_FLT_ASSIGN(key_parameters, key, skylight_u_value_1_glass_storm_winter);
      J_FLT_ASSIGN(key_parameters, key, skylight_u_value_1_glass_storm_summer);
      J_FLT_ASSIGN(key_parameters, key, skylight_u_value_2_glass_storm_winter);
      J_FLT_ASSIGN(key_parameters, key, skylight_u_value_2_glass_storm_summer);
      J_FLT_ASSIGN(key_parameters, key, skylight_u_value_1_plstc_storm_winter);
      J_FLT_ASSIGN(key_parameters, key, skylight_u_value_1_plstc_storm_summer);
      J_FLT_ASSIGN(key_parameters, key, skylight_u_value_2_plstc_storm_winter);
      J_FLT_ASSIGN(key_parameters, key, skylight_u_value_2_plstc_storm_summer);
      J_FLT_ASSIGN(key_parameters, key, window_shading_r_value_drapes);
      J_FLT_ASSIGN(key_parameters, key, window_shading_r_value_blinds_shades);
      J_FLT_ASSIGN(key_parameters, key, window_shading_r_value_drapes_shades);
      J_FLT_ASSIGN(key_parameters, key, sun_screen_solar_trans_reduction_summer);
      J_FLT_ASSIGN(key_parameters, key, sun_screen_solar_trans_reduction_winter);
      J_FLT_ASSIGN(key_parameters, key, ratio_of_awning_depth_to_window_height);

      J_FLT_ASSIGN(key_parameters, key, low_flow_shower_head_flow_rate);
      J_FLT_ASSIGN(key_parameters, key, water_heater_wrap_added_r_value);
      J_FLT_ASSIGN(key_parameters, key, refrigerator_defrost_cycle_energy);

    J_SEC_END();

  }  // next section in the json

// read in any referenced fuel esalation rates

  if (top->num_fer == 0) {
    ASSERT(top->wth.state[0] || top->rer.region || top->rer.state[0],
           sprintf(msg, "You must enter either the region or state for the referenced fuel cost escalation rates"));
    ASSERT(top->rer.year, sprintf(msg, "You must enter the year for the referenced fuel cost escalation rates"));
    get_referenced_escalation_rates(top, jschema);
  }

  // All sematic data validation above and beyond the ajv-cli structural checks presumed passed by the time we get here

  mdi_check(top);

  // clean up
  if (jleaf)
    cJSON_Delete(jleaf);
  if (jleaf2)
    cJSON_Delete(jleaf2);
  if (jbranch)
    cJSON_Delete(jbranch);
  if (jbranch2)
    cJSON_Delete(jbranch2);
  return;
}

/// Writes the JSON representation of the typedef struct MDI to the out_file handle.  The
/// calling function needs to open the file handle and this function will close it

void mhea_json_echo_write(MDI *top) {
  cJSON *jroot = NULL; // the JSON tree echo of our MDI structure
  cJSON *jarray = NULL;
  cJSON *jarray2 = NULL;
  cJSON *jitem = NULL;
  int i, j;

  // clang-format off

  jroot = cJSON_CreateObject();

  cJSON_AddItemToObject(jroot, "audit", jitem = cJSON_CreateObject());
  cJSON_AddStringToObject(jitem, "audit_type",              top->gnl.audit_type);
  cJSON_AddNumberToObject(jitem, "audit_id",                top->gnl.audit_id);
  cJSON_AddNumberToObject(jitem, "audit_number",            top->gnl.audit_number);
  cJSON_AddNumberToObject(jitem, "avg_no_occupants",        WA_DBL_FMT(top->gnl.avg_no_occupants, 2));
  cJSON_AddNumberToObject(jitem, "length",                  WA_DBL_FMT(top->gnl.length, 2));
  cJSON_AddNumberToObject(jitem, "width",                   WA_DBL_FMT(top->gnl.width, 2));
  cJSON_AddNumberToObject(jitem, "height",                  WA_DBL_FMT(top->gnl.height, 2));
  cJSON_AddNumberToObject(jitem, "wind_shielding",          top->gnl.wind_shielding);
  cJSON_AddNumberToObject(jitem, "leakiness",               top->gnl.leakiness);
  WA_AddBoolToObject(     jitem, "water_heater_closet",     top->gnl.water_heater_closet);
  WA_AddBoolToObject(     jitem, "do_billing_adjust",       top->gnl.do_billing_adjust);

  // MAIN HOME SHELL 

  cJSON_AddItemToObject(jroot, "walls", jitem = cJSON_CreateObject());
  cJSON_AddNumberToObject(jitem, "stud_size",               top->wal.stud_size);
  cJSON_AddNumberToObject(jitem, "home_orientation",        top->wal.home_orientation);
  cJSON_AddNumberToObject(jitem, "wall_vent",               top->wal.wall_vent);
  cJSON_AddNumberToObject(jitem, "batt_insl",               WA_DBL_FMT(top->wal.batt_insl, 2));
  cJSON_AddNumberToObject(jitem, "loose_insl",              WA_DBL_FMT(top->wal.loose_insl, 2));
  cJSON_AddNumberToObject(jitem, "foam_insl",               WA_DBL_FMT(top->wal.foam_insl, 2));
  cJSON_AddNumberToObject(jitem, "uninsulatable_area",      WA_DBL_FMT(top->wal.uninsulatable_area, 2));
  cJSON_AddNumberToObject(jitem, "porch_length",            WA_DBL_FMT(top->wal.porch_length, 2));
  cJSON_AddNumberToObject(jitem, "porch_width",             WA_DBL_FMT(top->wal.porch_width, 2));
  if (top->wal.porch_length !=0 && top->wal.porch_width != 0) cJSON_AddNumberToObject(jitem, "porch_orientation",       top->wal.porch_orientation);
  cJSON_AddNumberToObject(jitem, "add_cost",                WA_DBL_FMT(top->wal.add_cost, 2));

  if (top->num_win) {
    cJSON_AddItemToObject(jroot,   "windows",     jarray = cJSON_CreateArray());
    for (i = 0; i < top->num_win; i++) {
      cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
      cJSON_AddStringToObject(jitem, "code",                    top->win[i].code);
      cJSON_AddNumberToObject(jitem, "window_type",             top->win[i].window_type);
      cJSON_AddNumberToObject(jitem, "frame_type",              top->win[i].frame_type);
      cJSON_AddNumberToObject(jitem, "glazing_type",            top->win[i].glazing_type);
      cJSON_AddNumberToObject(jitem, "int_shading",             top->win[i].int_shading);
      cJSON_AddNumberToObject(jitem, "ext_shading",             top->win[i].ext_shading);
      cJSON_AddNumberToObject(jitem, "leak",                    top->win[i].leak);
      cJSON_AddNumberToObject(jitem, "width",                   WA_DBL_FMT(top->win[i].width, 2));
      cJSON_AddNumberToObject(jitem, "height",                  WA_DBL_FMT(top->win[i].height, 2));
      cJSON_AddNumberToObject(jitem, "num_n",                   WA_DBL_FMT(top->win[i].num_n, 2));
      cJSON_AddNumberToObject(jitem, "num_s",                   WA_DBL_FMT(top->win[i].num_s, 2));
      cJSON_AddNumberToObject(jitem, "num_e",                   WA_DBL_FMT(top->win[i].num_e, 2));
      cJSON_AddNumberToObject(jitem, "num_w",                   WA_DBL_FMT(top->win[i].num_w, 2));
      cJSON_AddNumberToObject(jitem, "retrofit_option",         top->win[i].retrofit_option);
      WA_AddBoolToObject(     jitem, "inc_sir",                 top->win[i].inc_sir);
      cJSON_AddNumberToObject(jitem, "cost_seal",         WA_DBL_FMT(top->win[i].cost_seal, 2));
      cJSON_AddNumberToObject(jitem, "cost_replace",            WA_DBL_FMT(top->win[i].cost_replace, 2));
      cJSON_AddNumberToObject(jitem, "cost_add_glass_storm",    WA_DBL_FMT(top->win[i].cost_add_glass_storm, 2));
      cJSON_AddNumberToObject(jitem, "cost_add_plastic_storm",  WA_DBL_FMT(top->win[i].cost_add_plastic_storm, 2));
    }
  }

  if (top->num_dor) {
    cJSON_AddItemToObject(jroot,   "doors",     jarray = cJSON_CreateArray());
    for (i = 0; i < top->num_dor; i++) {
      cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
      cJSON_AddStringToObject(jitem, "code",                    top->dor[i].code);
      cJSON_AddNumberToObject(jitem, "door_type",               top->dor[i].door_type);
      cJSON_AddNumberToObject(jitem, "leakiness",               top->dor[i].leakiness);
      WA_AddBoolToObject(     jitem, "storm",                   top->dor[i].storm);
      cJSON_AddNumberToObject(jitem, "width",                   WA_DBL_FMT(top->dor[i].width, 2));
      cJSON_AddNumberToObject(jitem, "height",                  WA_DBL_FMT(top->dor[i].height, 2));
      cJSON_AddNumberToObject(jitem, "num_n",                   WA_DBL_FMT(top->dor[i].num_n, 2));
      cJSON_AddNumberToObject(jitem, "num_s",                   WA_DBL_FMT(top->dor[i].num_s, 2));
      cJSON_AddNumberToObject(jitem, "num_e",                   WA_DBL_FMT(top->dor[i].num_e, 2));
      cJSON_AddNumberToObject(jitem, "num_w",                   WA_DBL_FMT(top->dor[i].num_w, 2));
      WA_AddBoolToObject(     jitem, "replace",                 top->dor[i].replace);
      WA_AddBoolToObject(     jitem, "inc_sir",                 top->dor[i].inc_sir);
      cJSON_AddNumberToObject(jitem, "cost_replace",            WA_DBL_FMT(top->dor[i].cost_replace, 2));
    }
  }

  cJSON_AddItemToObject(jroot, "ceiling", jitem = cJSON_CreateObject());
  cJSON_AddNumberToObject(jitem, "roof_type",               top->rof.roof_type);
  cJSON_AddNumberToObject(jitem, "roof_color",              top->rof.roof_color);
  if (top->rof.roof_type == RT_FLAT)      cJSON_AddNumberToObject(jitem, "joist_size",              top->rof.joist_size);
  if (top->rof.roof_type == RT_BOWSTRING) cJSON_AddNumberToObject(jitem, "ceiling_height",          WA_DBL_FMT(top->rof.ceiling_height, 2));
  if (top->rof.roof_type == RT_PITCHED)   cJSON_AddNumberToObject(jitem, "pitch_add_insl",          WA_DBL_FMT(top->rof.pitch_add_insl, 2));
  cJSON_AddNumberToObject(jitem, "mineral_insl",            WA_DBL_FMT(top->rof.mineral_insl, 2));
  cJSON_AddNumberToObject(jitem, "loose_insl",              WA_DBL_FMT(top->rof.loose_insl, 2));
  cJSON_AddNumberToObject(jitem, "rigid_insl",              WA_DBL_FMT(top->rof.rigid_insl, 2));
  cJSON_AddNumberToObject(jitem, "cathedral_ceiling",       WA_DBL_FMT(top->rof.cathedral_ceiling, 2));
  if (top->rof.cathedral_ceiling != 0) cJSON_AddNumberToObject(jitem, "step_wall",                  top->rof.step_wall);
  cJSON_AddNumberToObject(jitem, "add_cost",                WA_DBL_FMT(top->rof.add_cost, 2));


  cJSON_AddItemToObject(jroot, "floor", jitem = cJSON_CreateObject());
  WA_AddBoolToObject(     jitem, "skirt",                          top->flr.skirt);
  cJSON_AddNumberToObject(jitem, "wing_joist_size",                top->flr.wing_joist_size);
  cJSON_AddNumberToObject(jitem, "wing_mineral_insl",              WA_DBL_FMT(top->flr.wing_mineral_insl, 2));
  cJSON_AddNumberToObject(jitem, "wing_loose_insl",                WA_DBL_FMT(top->flr.wing_loose_insl, 2));
  cJSON_AddNumberToObject(jitem, "wing_insl_location",             top->flr.wing_insl_location);
  cJSON_AddNumberToObject(jitem, "belly_joist_size",               top->flr.belly_joist_size);
  cJSON_AddNumberToObject(jitem, "belly_mineral_insl",             WA_DBL_FMT(top->flr.belly_mineral_insl, 2));
  cJSON_AddNumberToObject(jitem, "belly_loose_insl",               WA_DBL_FMT(top->flr.belly_loose_insl, 2));
  cJSON_AddNumberToObject(jitem, "belly_insl_location",            top->flr.belly_insl_location);
  cJSON_AddNumberToObject(jitem, "belly_condition",                top->flr.belly_condition);
  cJSON_AddNumberToObject(jitem, "belly_cavity",                   top->flr.belly_cavity);
  cJSON_AddNumberToObject(jitem, "belly_depth",                    WA_DBL_FMT(top->flr.belly_depth, 2));
  cJSON_AddNumberToObject(jitem, "add_cost",                       WA_DBL_FMT(top->flr.add_cost, 2));

  // ANY HOME ADDITIONS SHELL INFORMAITON

  if(top->awl.stud_size) {     // proxy for non-null rest of struct
    cJSON_AddItemToObject(jroot, "walls_addition", jitem = cJSON_CreateObject());
    cJSON_AddNumberToObject(jitem, "stud_size",               top->awl.stud_size);
    cJSON_AddNumberToObject(jitem, "orientation",             top->awl.orientation);
    cJSON_AddNumberToObject(jitem, "wall_vent",               top->awl.wall_vent);
    cJSON_AddNumberToObject(jitem, "wall_config",             top->awl.wall_config);
    cJSON_AddNumberToObject(jitem, "height_max",              WA_DBL_FMT(top->awl.height_max, 2));
    cJSON_AddNumberToObject(jitem, "height_min",              WA_DBL_FMT(top->awl.height_min, 2));
    cJSON_AddNumberToObject(jitem, "batt_insl",               WA_DBL_FMT(top->awl.batt_insl, 2));
    cJSON_AddNumberToObject(jitem, "loose_insl",              WA_DBL_FMT(top->awl.loose_insl, 2));
    cJSON_AddNumberToObject(jitem, "foam_insl",               WA_DBL_FMT(top->awl.foam_insl, 2));
    cJSON_AddNumberToObject(jitem, "add_cost",                WA_DBL_FMT(top->awl.add_cost, 2));
  }

  if (top->num_awn) {
    cJSON_AddItemToObject(jroot,   "windows_addition",     jarray = cJSON_CreateArray());
    for (i = 0; i < top->num_awn; i++) {
      cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
      cJSON_AddStringToObject(jitem, "code",                    top->awn[i].code);
      cJSON_AddNumberToObject(jitem, "window_type",             top->awn[i].window_type);
      WA_AddNumToObjectNoZero(jitem, "frame_type",              top->awn[i].frame_type);
      cJSON_AddNumberToObject(jitem, "glazing_type",            top->awn[i].glazing_type);
      cJSON_AddNumberToObject(jitem, "int_shading",             top->awn[i].int_shading);
      cJSON_AddNumberToObject(jitem, "ext_shading",             top->awn[i].ext_shading);
      cJSON_AddNumberToObject(jitem, "leak",                    top->awn[i].leak);
      cJSON_AddNumberToObject(jitem, "width",                   WA_DBL_FMT(top->awn[i].width, 2));
      cJSON_AddNumberToObject(jitem, "height",                  WA_DBL_FMT(top->awn[i].height, 2));
      cJSON_AddNumberToObject(jitem, "num_n",                   WA_DBL_FMT(top->awn[i].num_n, 2));
      cJSON_AddNumberToObject(jitem, "num_s",                   WA_DBL_FMT(top->awn[i].num_s, 2));
      cJSON_AddNumberToObject(jitem, "num_e",                   WA_DBL_FMT(top->awn[i].num_e, 2));
      cJSON_AddNumberToObject(jitem, "num_w",                   WA_DBL_FMT(top->awn[i].num_w, 2));
      cJSON_AddNumberToObject(jitem, "retrofit_option",         top->awn[i].retrofit_option);
      WA_AddBoolToObject(     jitem, "inc_sir",                 top->awn[i].inc_sir);
      cJSON_AddNumberToObject(jitem, "cost_seal",         WA_DBL_FMT(top->awn[i].cost_seal, 2));
      cJSON_AddNumberToObject(jitem, "cost_replace",            WA_DBL_FMT(top->awn[i].cost_replace, 2));
      cJSON_AddNumberToObject(jitem, "cost_add_glass_storm",    WA_DBL_FMT(top->awn[i].cost_add_glass_storm, 2));
      cJSON_AddNumberToObject(jitem, "cost_add_plastic_storm",  WA_DBL_FMT(top->awn[i].cost_add_plastic_storm, 2));
    }
  }

  if (top->num_adr) {
    cJSON_AddItemToObject(jroot,   "doors_addition",     jarray = cJSON_CreateArray());
    for (i = 0; i < top->num_adr; i++) {
      cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
      cJSON_AddStringToObject(jitem, "code",                    top->adr[i].code);
      cJSON_AddNumberToObject(jitem, "door_type",               top->adr[i].door_type);
      cJSON_AddNumberToObject(jitem, "leakiness",               top->adr[i].leakiness);
      WA_AddBoolToObject(     jitem, "storm",                   top->adr[i].storm);
      cJSON_AddNumberToObject(jitem, "width",                   WA_DBL_FMT(top->adr[i].width, 2));
      cJSON_AddNumberToObject(jitem, "height",                  WA_DBL_FMT(top->adr[i].height, 2));
      cJSON_AddNumberToObject(jitem, "num_n",                   WA_DBL_FMT(top->adr[i].num_n, 2));
      cJSON_AddNumberToObject(jitem, "num_s",                   WA_DBL_FMT(top->adr[i].num_s, 2));
      cJSON_AddNumberToObject(jitem, "num_e",                   WA_DBL_FMT(top->adr[i].num_e, 2));
      cJSON_AddNumberToObject(jitem, "num_w",                   WA_DBL_FMT(top->adr[i].num_w, 2));
      WA_AddBoolToObject(     jitem, "replace",                 top->adr[i].replace);
      WA_AddBoolToObject(     jitem, "inc_sir",                 top->adr[i].inc_sir);
      cJSON_AddNumberToObject(jitem, "cost_replace",            WA_DBL_FMT(top->adr[i].cost_replace, 2));
    }
  }

  if(top->arf.joist_size) {     // proxy for non-null rest of struct
    cJSON_AddItemToObject(jroot, "ceiling_addition", jitem = cJSON_CreateObject());
    cJSON_AddNumberToObject(jitem, "roof_color",              top->arf.roof_color);
    cJSON_AddNumberToObject(jitem, "joist_size",              top->arf.joist_size);
    cJSON_AddNumberToObject(jitem, "mineral_insl",            WA_DBL_FMT(top->arf.mineral_insl, 2));
    cJSON_AddNumberToObject(jitem, "loose_insl",              WA_DBL_FMT(top->arf.loose_insl, 2));
    cJSON_AddNumberToObject(jitem, "rigid_insl",              WA_DBL_FMT(top->arf.rigid_insl, 2));
    cJSON_AddNumberToObject(jitem, "add_cost",                WA_DBL_FMT(top->arf.add_cost, 2));
  }

  if(top->afl.joist_size) {     // proxy for non-null rest of struct
    cJSON_AddItemToObject(jroot, "floor_addition", jitem = cJSON_CreateObject());
    cJSON_AddNumberToObject(jitem, "add_floor_type",              top->afl.add_floor_type);
    cJSON_AddNumberToObject(jitem, "joist_size",                  top->afl.joist_size);
    cJSON_AddNumberToObject(jitem, "insl_location",               top->afl.insl_location);
    cJSON_AddNumberToObject(jitem, "length",                      WA_DBL_FMT(top->afl.length, 2));
    cJSON_AddNumberToObject(jitem, "width",                       WA_DBL_FMT(top->afl.width, 2));
    cJSON_AddNumberToObject(jitem, "mineral_insl",                WA_DBL_FMT(top->afl.mineral_insl, 2));
    cJSON_AddNumberToObject(jitem, "loose_insl",                  WA_DBL_FMT(top->afl.loose_insl, 2));
    cJSON_AddNumberToObject(jitem, "avail_insl",                  WA_DBL_FMT(top->afl.avail_insl, 2));
  }

  cJSON_AddItemToObject(jroot, "heating_primary", jitem = cJSON_CreateObject());
  cJSON_AddNumberToObject(jitem, "equip_type",                    top->htg.equip_type);
  cJSON_AddNumberToObject(jitem, "fuel_type",                     top->htg.fuel_type);
  // #370
  //if (top->htg.fuel_type != WOOD && top->htg.fuel_type != COAL) {
    cJSON_AddNumberToObject(jitem, "capacity",                      WA_DBL_FMT(top->htg.capacity, 2));
  //}
  cJSON_AddNumberToObject(jitem, "eff_units",                     top->htg.eff_units);
  if (top->htg.eff_units) {
    if (top->htg.eff_units == HE_SSTATE || top->htg.eff_units == HE_AFUE)
    cJSON_AddNumberToObject(jitem, "efficiency_percent",          WA_DBL_FMT(top->htg.efficiency_percent, 2));
    if (top->htg.eff_units == HE_HSPF)
    cJSON_AddNumberToObject(jitem, "efficiency_hspf",             WA_DBL_FMT(top->htg.efficiency_hspf, 2));
    if (top->htg.eff_units == HE_COP)
    cJSON_AddNumberToObject(jitem, "efficiency_cop",              WA_DBL_FMT(top->htg.efficiency_cop, 2));
  }
  WA_AddNumToObjectNoZero(jitem, "duct_location",                 top->htg.duct_location);
  WA_AddNumToObjectNoZero(jitem, "duct_insl",                     top->htg.duct_insl);
  cJSON_AddNumberToObject(jitem, "percent_heated",                WA_DBL_FMT(top->htg.percent_heated, 2));
  WA_AddBoolToObject(     jitem, "smart_thermostat",              top->htg.smart_thermostat);
  WA_AddBoolToObject(     jitem, "tuneup",                        top->htg.tuneup);
  WA_AddBoolToObject(     jitem, "inc_sir",                       top->htg.inc_sir);

  if (top->ht2.equip_type && top->ht2.equip_type != ET_NONE) {
    cJSON_AddItemToObject(jroot, "heating_secondary", jitem = cJSON_CreateObject());
    cJSON_AddNumberToObject(jitem, "equip_type",                    top->ht2.equip_type);
    cJSON_AddNumberToObject(jitem, "fuel_type",                     top->ht2.fuel_type);
    // #370
    //if (top->ht2.fuel_type != WOOD && top->ht2.fuel_type != COAL) {
      cJSON_AddNumberToObject(jitem, "capacity",                      WA_DBL_FMT(top->ht2.capacity, 2));
    //}
    cJSON_AddNumberToObject(jitem, "eff_units",                     top->ht2.eff_units);
    if (top->ht2.eff_units) {
      if (top->ht2.eff_units == HE_SSTATE || top->ht2.eff_units == HE_AFUE)
      cJSON_AddNumberToObject(jitem, "efficiency_percent",          WA_DBL_FMT(top->ht2.efficiency_percent, 2));
      if (top->ht2.eff_units == HE_HSPF)
      cJSON_AddNumberToObject(jitem, "efficiency_hspf",             WA_DBL_FMT(top->ht2.efficiency_hspf, 2));
      if (top->ht2.eff_units == HE_COP)
      cJSON_AddNumberToObject(jitem, "efficiency_cop",              WA_DBL_FMT(top->ht2.efficiency_cop, 2));
    }
  }

  if (top->htr.equip_type && top->htr.equip_type != ET_NONE) {
    cJSON_AddItemToObject(jroot, "heating_replacement", jitem = cJSON_CreateObject());
    cJSON_AddNumberToObject(jitem, "equip_type",                    top->htr.equip_type);
    cJSON_AddNumberToObject(jitem, "fuel_type",                     top->htr.fuel_type);
    cJSON_AddNumberToObject(jitem, "capacity",                      WA_DBL_FMT(top->htr.capacity, 2));
    cJSON_AddNumberToObject(jitem, "eff_units",                     top->htr.eff_units);
    if (top->htr.eff_units) {
      if (top->htr.eff_units == HE_SSTATE || top->htr.eff_units == HE_AFUE)
      cJSON_AddNumberToObject(jitem, "efficiency_percent",          WA_DBL_FMT(top->htr.efficiency_percent, 2));
      if (top->htr.eff_units == HE_HSPF)
      cJSON_AddNumberToObject(jitem, "efficiency_hspf",             WA_DBL_FMT(top->htr.efficiency_hspf, 2));
      if (top->htr.eff_units == HE_COP)
      cJSON_AddNumberToObject(jitem, "efficiency_cop",              WA_DBL_FMT(top->htr.efficiency_cop, 2));
    }
    WA_AddNumToObjectNoZero(jitem, "duct_location",                 top->htr.duct_location);
    WA_AddNumToObjectNoZero(jitem, "duct_insl",                     top->htr.duct_insl);
    WA_AddBoolToObject(     jitem, "replacement",                   top->htr.replacement);
    WA_AddBoolToObject(     jitem, "incl_costs",                    top->htr.incl_costs);
    cJSON_AddNumberToObject(jitem, "labor_cost",                    WA_DBL_FMT(top->htr.labor_cost, 2));
    cJSON_AddNumberToObject(jitem, "material_cost",                 WA_DBL_FMT(top->htr.material_cost, 2));
  }

  if (top->clg.equip_type && top->clg.equip_type != CE_NONE) {
    cJSON_AddItemToObject(jroot, "cooling_primary", jitem = cJSON_CreateObject());
    cJSON_AddNumberToObject(jitem, "equip_type",                    top->clg.equip_type);
    if (top->clg.equip_type != CE_EVAPORATIVE && top->clg.equip_type != CE_NONE) {
      cJSON_AddNumberToObject(jitem, "capacity",                      WA_DBL_FMT(top->clg.capacity, 2));
      cJSON_AddNumberToObject(jitem, "eff_units",                     top->clg.eff_units);
      if (top->clg.eff_units) {
        if (top->clg.eff_units == CE_EER)
          cJSON_AddNumberToObject(jitem, "efficiency_eer",            WA_DBL_FMT(top->clg.efficiency_eer, 2));
        if (top->clg.eff_units == CE_SEER)
          cJSON_AddNumberToObject(jitem, "efficiency_seer",           WA_DBL_FMT(top->clg.efficiency_seer, 2));
        if (top->clg.eff_units == CE_COP)
          cJSON_AddNumberToObject(jitem, "efficiency_cop",            WA_DBL_FMT(top->clg.efficiency_cop, 2));
      }
      WA_AddNumToObjectNoZero(jitem, "duct_location",                 top->clg.duct_location);
      WA_AddNumToObjectNoZero(jitem, "duct_insl",                     top->clg.duct_insl);
      cJSON_AddNumberToObject(jitem, "percent_area_room_ac",          WA_DBL_FMT(top->clg.percent_area_room_ac, 2));
      WA_AddBoolToObject(     jitem, "tuneup",                        top->clg.tuneup);
      WA_AddBoolToObject(     jitem, "inc_sir",                       top->clg.inc_sir);
    }
  }

  if (top->cl2.equip_type && top->cl2.equip_type != CE_NONE) {
    cJSON_AddItemToObject(jroot, "cooling_secondary", jitem = cJSON_CreateObject());
    cJSON_AddNumberToObject(jitem, "equip_type",                    top->cl2.equip_type);
    if (top->cl2.equip_type != CE_EVAPORATIVE && top->cl2.equip_type != CE_NONE) {
      cJSON_AddNumberToObject(jitem, "capacity",                      WA_DBL_FMT(top->cl2.capacity, 2));
      cJSON_AddNumberToObject(jitem, "eff_units",                     top->cl2.eff_units);
      if (top->cl2.eff_units) {
        if (top->cl2.eff_units == CE_EER)
          cJSON_AddNumberToObject(jitem, "efficiency_eer",            WA_DBL_FMT(top->cl2.efficiency_eer, 2));
        if (top->cl2.eff_units == CE_SEER)
          cJSON_AddNumberToObject(jitem, "efficiency_seer",           WA_DBL_FMT(top->cl2.efficiency_seer, 2));
        if (top->cl2.eff_units == CE_COP)
          cJSON_AddNumberToObject(jitem, "efficiency_cop",            WA_DBL_FMT(top->cl2.efficiency_cop, 2));
      }
      cJSON_AddNumberToObject(jitem, "percent_area_room_ac",          WA_DBL_FMT(top->cl2.percent_area_room_ac, 2));
    }
  }

  if (top->clr.equip_type && top->clr.equip_type != CE_NONE) {
    cJSON_AddItemToObject(jroot, "cooling_replacement", jitem = cJSON_CreateObject());
    cJSON_AddNumberToObject(jitem, "equip_type",                    top->clr.equip_type);
    if (top->clr.equip_type != CE_EVAPORATIVE && top->clr.equip_type != CE_NONE) {
      cJSON_AddNumberToObject(jitem, "capacity",                      WA_DBL_FMT(top->clr.capacity, 2));
      cJSON_AddNumberToObject(jitem, "eff_units",                     top->clr.eff_units);
      if (top->clr.eff_units) {
        if (top->clr.eff_units == CE_EER)
          cJSON_AddNumberToObject(jitem, "efficiency_eer",            WA_DBL_FMT(top->clr.efficiency_eer, 2));
        if (top->clr.eff_units == CE_SEER)
          cJSON_AddNumberToObject(jitem, "efficiency_seer",           WA_DBL_FMT(top->clr.efficiency_seer, 2));
        if (top->clr.eff_units == CE_COP)
          cJSON_AddNumberToObject(jitem, "efficiency_cop",            WA_DBL_FMT(top->clr.efficiency_cop, 2));
      }
      WA_AddNumToObjectNoZero(jitem, "clg_duct_location",             top->clr.clg_duct_location);
      WA_AddNumToObjectNoZero(jitem, "clg_duct_insl",                 top->clr.clg_duct_insl);
      WA_AddBoolToObject(     jitem, "replacement",                   top->clr.replacement);
      WA_AddBoolToObject(     jitem, "incl_costs",                    top->clr.incl_costs);
      cJSON_AddNumberToObject(jitem, "labor_cost",                    WA_DBL_FMT(top->clr.labor_cost, 2));
      cJSON_AddNumberToObject(jitem, "material_cost",                 WA_DBL_FMT(top->clr.material_cost, 2));
    }
  }

  cJSON_AddItemToObject(jroot, "ducts_and_infiltration", jitem = cJSON_CreateObject());
  WA_AddBoolToObject(     jitem, "evaluate_duct_sealing",                 top->inf.evaluate_duct_sealing);
  cJSON_AddNumberToObject(jitem, "duct_seal_method",                      top->inf.duct_seal_method);
  WA_AddNumToObjectNoZero(jitem, "air_leak_red_cost",                     WA_DBL_FMT(top->inf.air_leak_red_cost, 2));
  WA_AddNumToObjectNoZero(jitem, "duct_seal_cost",                        WA_DBL_FMT(top->inf.duct_seal_cost, 2));
  WA_AddNumToObjectNoZero(jitem, "pre_inf_cfm",                           WA_DBL_FMT(top->inf.pre_inf_cfm, 2));
  WA_AddNumToObjectNoZero(jitem, "pre_inf_pa",                            WA_DBL_FMT(top->inf.pre_inf_pa, 2));
  cJSON_AddNumberToObject(jitem, "post_inf_cfm",                          WA_DBL_FMT(top->inf.post_inf_cfm, 2));
  cJSON_AddNumberToObject(jitem, "post_inf_pa",                           WA_DBL_FMT(top->inf.post_inf_pa, 2));
  WA_AddNumToObjectNoZero(jitem, "pre_duct_seal_supply_pa",               WA_DBL_FMT(top->inf.pre_duct_seal_supply_pa, 2));
  //WA_AddNumToObjectNoZero(jitem, "pre_duct_seal_return_pa",               WA_DBL_FMT(top->inf.pre_duct_seal_return_pa, 2));
  WA_AddNumToObjectNoZero(jitem, "post_duct_seal_supply_pa",              WA_DBL_FMT(top->inf.post_duct_seal_supply_pa, 2));
  //WA_AddNumToObjectNoZero(jitem, "post_duct_seal_return_pa",              WA_DBL_FMT(top->inf.post_duct_seal_return_pa, 2));
  WA_AddNumToObjectNoZero(jitem, "post_duct_seal_cfm",                    WA_DBL_FMT(top->inf.post_duct_seal_cfm, 2));
  WA_AddNumToObjectNoZero(jitem, "post_duct_seal_pa",                     WA_DBL_FMT(top->inf.post_duct_seal_pa, 2));
  WA_AddNumToObjectNoZero(jitem, "pre_duct_seal_close_cfm",               WA_DBL_FMT(top->inf.pre_duct_seal_close_cfm, 2));
  WA_AddNumToObjectNoZero(jitem, "pre_duct_seal_close_pa",                WA_DBL_FMT(top->inf.pre_duct_seal_close_pa, 2));
  WA_AddNumToObjectNoZero(jitem, "pre_duct_seal_close_diff_pa",           WA_DBL_FMT(top->inf.pre_duct_seal_close_diff_pa, 2));
  WA_AddNumToObjectNoZero(jitem, "post_duct_seal_close_cfm",              WA_DBL_FMT(top->inf.post_duct_seal_close_cfm, 2));
  WA_AddNumToObjectNoZero(jitem, "post_duct_seal_close_pa",               WA_DBL_FMT(top->inf.post_duct_seal_close_pa, 2));
  WA_AddNumToObjectNoZero(jitem, "post_duct_seal_close_diff_pa",          WA_DBL_FMT(top->inf.post_duct_seal_close_diff_pa, 2));
  WA_AddNumToObjectNoZero(jitem, "pre_duct_seal_tot_cfm",                 WA_DBL_FMT(top->inf.pre_duct_seal_tot_cfm, 2));
  WA_AddNumToObjectNoZero(jitem, "pre_duct_seal_tot_duct_pa",             WA_DBL_FMT(top->inf.pre_duct_seal_tot_duct_pa, 2));
  WA_AddNumToObjectNoZero(jitem, "pre_duct_seal_out_cfm",                 WA_DBL_FMT(top->inf.pre_duct_seal_out_cfm, 2));
  WA_AddNumToObjectNoZero(jitem, "pre_duct_seal_out_duct_pa",             WA_DBL_FMT(top->inf.pre_duct_seal_out_duct_pa, 2));
  WA_AddNumToObjectNoZero(jitem, "post_duct_seal_tot_cfm",                WA_DBL_FMT(top->inf.post_duct_seal_tot_cfm, 2));
  WA_AddNumToObjectNoZero(jitem, "post_duct_seal_tot_duct_pa",            WA_DBL_FMT(top->inf.post_duct_seal_tot_duct_pa, 2));
  WA_AddNumToObjectNoZero(jitem, "post_duct_seal_out_cfm",                WA_DBL_FMT(top->inf.post_duct_seal_out_cfm, 2));
  WA_AddNumToObjectNoZero(jitem, "post_duct_seal_out_duct_pa",            WA_DBL_FMT(top->inf.post_duct_seal_out_duct_pa, 2));
 
  if (top->dwh.exist_fuel_type_id) {
    cJSON_AddItemToObject(jroot,   "water_heating",     jitem = cJSON_CreateObject());
    cJSON_AddNumberToObject(jitem, "exist_tank_location_id",         top->dwh.exist_tank_location_id);
    cJSON_AddNumberToObject(jitem, "exist_fuel_type_id",             top->dwh.exist_fuel_type_id);
    cJSON_AddNumberToObject(jitem, "exist_type",                     top->dwh.exist_type);
    WA_AddNumToObjectNoZero(jitem, "exist_gal",                      WA_DBL_FMT(top->dwh.exist_gal, 2));
    WA_AddNumToObjectNoZero(jitem, "exist_uniform_energy_factor",    WA_DBL_FMT(top->dwh.exist_uniform_energy_factor, 2));
    WA_AddNumToObjectNoZero(jitem, "exist_energy_factor",            WA_DBL_FMT(top->dwh.exist_energy_factor, 2));
    WA_AddNumToObjectNoZero(jitem, "exist_recovery_efficiency",      WA_DBL_FMT(top->dwh.exist_recovery_efficiency, 2));
    WA_AddNumToObjectNoZero(jitem, "exist_input_units_id",           top->dwh.exist_input_units_id);
    WA_AddNumToObjectNoZero(jitem, "exist_input",                    WA_DBL_FMT(top->dwh.exist_input, 2));
    WA_AddNumToObjectNoZero(jitem, "exist_insul_type_id",            top->dwh.exist_insul_type_id);
    WA_AddNumToObjectNoZero(jitem, "exist_insul_thick",              WA_DBL_FMT(top->dwh.exist_insul_thick, 2));
    WA_AddNumToObjectNoZero(jitem, "exist_rvalue",                   WA_DBL_FMT(top->dwh.exist_rvalue, 2));
    WA_AddBoolToObject(     jitem, "exist_pipe_insul",               top->dwh.exist_pipe_insul);
    WA_AddBoolToObject(     jitem, "exist_tank_wrap",                top->dwh.exist_tank_wrap);
    WA_AddNumToObjectNoZero(jitem, "shower_heads",                   top->dwh.shower_heads);
    WA_AddNumToObjectNoZero(jitem, "shower_usage_per_day",           WA_DBL_FMT(top->dwh.shower_usage_per_day, 2));
    WA_AddNumToObjectNoZero(jitem, "shower_gpm",                     WA_DBL_FMT(top->dwh.shower_gpm, 2));
    cJSON_AddStringToObject(jitem, "replace_manufacturer",           top->dwh.replace_manufacturer);
    cJSON_AddStringToObject(jitem, "replace_model",                  top->dwh.replace_model);
    WA_AddNumToObjectNoZero(jitem, "replace_fuel_type_id",           top->dwh.replace_fuel_type_id);
    WA_AddNumToObjectNoZero(jitem, "replace_type",                   top->dwh.replace_type);
    WA_AddNumToObjectNoZero(jitem, "replace_gal",                    WA_DBL_FMT(top->dwh.replace_gal, 2));
    WA_AddNumToObjectNoZero(jitem, "replace_uniform_energy_factor",  WA_DBL_FMT(top->dwh.replace_uniform_energy_factor, 2));
    WA_AddNumToObjectNoZero(jitem, "replace_recovery_efficiency",    WA_DBL_FMT(top->dwh.replace_recovery_efficiency, 2));
    WA_AddNumToObjectNoZero(jitem, "replace_input_units_id",         top->dwh.replace_input_units_id);
    WA_AddNumToObjectNoZero(jitem, "replace_input",                  WA_DBL_FMT(top->dwh.replace_input, 2));
    WA_AddNumToObjectNoZero(jitem, "replace_life",                   top->dwh.replace_life);
    WA_AddNumToObjectNoZero(jitem, "replace_install_cost",           WA_DBL_FMT(top->dwh.replace_install_cost, 2));
    cJSON_AddNumberToObject(jitem, "replace_added_cost",             WA_DBL_FMT(top->dwh.replace_added_cost, 2));
    WA_AddBoolToObject(     jitem, "replace",                        top->dwh.replace);
    WA_AddBoolToObject(     jitem, "inc_sir",                        top->dwh.inc_sir);
  }

  if (top->ref.label_year_id) {
    cJSON_AddItemToObject(jroot,   "refrigerators",     jitem = cJSON_CreateObject());
    cJSON_AddNumberToObject(jitem, "location_id",                  top->ref.location_id);
    WA_AddNumToObjectNoZero(jitem, "label_year_id",                top->ref.label_year_id);
    WA_AddNumToObjectNoZero(jitem, "door_seal_condition_id",       top->ref.door_seal_condition_id);
    WA_AddNumToObjectNoZero(jitem, "label_kwh_per_year",           WA_DBL_FMT(top->ref.label_kwh_per_year, 2));
    WA_AddNumToObjectNoZero(jitem, "meter_energy_reading",         WA_DBL_FMT(top->ref.meter_energy_reading, 2));
    WA_AddNumToObjectNoZero(jitem, "meter_energy_interval",        WA_DBL_FMT(top->ref.meter_energy_interval, 2));
    WA_AddBoolToObject(     jitem, "meter_manual_defrost",         top->ref.meter_manual_defrost);
    WA_AddBoolToObject(     jitem, "meter_includes_defrost",       top->ref.meter_includes_defrost);
    WA_AddNumToObjectNoZero(jitem, "meter_temperature",            WA_DBL_FMT(top->ref.meter_temperature, 2));
    cJSON_AddStringToObject(jitem, "replace_manufacturer",         top->ref.replace_manufacturer);
    cJSON_AddStringToObject(jitem, "replace_model",                top->ref.replace_model);
    cJSON_AddNumberToObject(jitem, "replace_kwh_per_year",         WA_DBL_FMT(top->ref.replace_kwh_per_year, 2));
    cJSON_AddNumberToObject(jitem, "replace_life",                 top->ref.replace_life);
    cJSON_AddNumberToObject(jitem, "replace_install_cost",         WA_DBL_FMT(top->ref.replace_install_cost, 2));
    cJSON_AddNumberToObject(jitem, "replace_added_cost",           WA_DBL_FMT(top->ref.replace_added_cost, 2));
  }

  if (top->num_ltg) {
    cJSON_AddItemToObject(jroot,   "lighting",     jarray = cJSON_CreateArray());
    for (i = 0; i < top->num_ltg; i++) {
      cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
      cJSON_AddStringToObject(jitem, "code",                      top->ltg[i].code);
      cJSON_AddNumberToObject(jitem, "exist_lamp_type",           top->ltg[i].exist_lamp_type);
      cJSON_AddNumberToObject(jitem, "exist_lamp_count",          top->ltg[i].exist_lamp_count);
      cJSON_AddNumberToObject(jitem, "exist_lamp_watts",          WA_DBL_FMT(top->ltg[i].exist_lamp_watts, 2));
      cJSON_AddNumberToObject(jitem, "exist_hours_per_day",       WA_DBL_FMT(top->ltg[i].exist_hours_per_day, 2));
      cJSON_AddNumberToObject(jitem, "new_lamp_type",             top->ltg[i].new_lamp_type);
      cJSON_AddNumberToObject(jitem, "new_lamp_count",            top->ltg[i].new_lamp_count);
      cJSON_AddNumberToObject(jitem, "new_lamp_watts",            WA_DBL_FMT(top->ltg[i].new_lamp_watts, 2));
      cJSON_AddNumberToObject(jitem, "new_hours_per_day",         WA_DBL_FMT(top->ltg[i].new_hours_per_day, 2));
      cJSON_AddNumberToObject(jitem, "new_lifetime_hrs",          WA_DBL_FMT(top->ltg[i].new_lifetime_hrs, 2));
      cJSON_AddNumberToObject(jitem, "install_cost_per_lamp",     WA_DBL_FMT(top->ltg[i].install_cost_per_lamp, 2));
      cJSON_AddNumberToObject(jitem, "added_cost_per_lamp",       WA_DBL_FMT(top->ltg[i].added_cost_per_lamp, 2));
      cJSON_AddNumberToObject(jitem, "added_cost",                WA_DBL_FMT(top->ltg[i].added_cost, 2));
    }
  }

  if (top->num_itc) {
    cJSON_AddItemToObject(jroot,   "itemized_costs",     jarray = cJSON_CreateArray());
    for (i = 0; i < top->num_itc; i++) {
      cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
      cJSON_AddStringToObject(jitem, "measure",                      top->itc[i].measure);
      if (top->itc[i].component_id) cJSON_AddNumberToObject(jitem,    "component_id",     top->itc[i].component_id);
      cJSON_AddNumberToObject(jitem, "cost",                         WA_DBL_FMT(top->itc[i].cost, 2));
      WA_AddBoolToObject(     jitem, "inc_sir",                      top->itc[i].inc_sir);
      cJSON_AddStringToObject(jitem, "material",                     top->itc[i].material);
      WA_AddNumToObjectNoZero(jitem, "units",                        top->itc[i].units);
      cJSON_AddNumberToObject(jitem, "savings",                      WA_DBL_FMT(top->itc[i].savings, 2));
      WA_AddNumToObjectNoZero(jitem, "fuel",                         top->itc[i].fuel);
      WA_AddNumToObjectNoZero(jitem, "life",                         WA_DBL_FMT(top->itc[i].life, 2));
    }
  }

  if (top->ubh.usage_units) {
    cJSON_AddItemToObject(jroot,   "utility_bills_pre_retrofit_heating",     jitem = cJSON_CreateObject());
    cJSON_AddNumberToObject(jitem, "usage_units",                   top->ubh.usage_units);
    cJSON_AddNumberToObject(jitem, "period_days",                   top->ubh.period_days);
    WA_AddNumToObjectNoZero(jitem, "base_temp",                     WA_DBL_FMT(top->ubh.base_temp, 2));
    WA_AddNumToObjectNoZero(jitem, "base_load",                     WA_DBL_FMT(top->ubh.base_load, 2));

    if (top->num_urh) {
      cJSON_AddItemToObject(jroot,   "utility_bills_pre_retrofit_heating_data",     jarray = cJSON_CreateArray());
      for (i = 0; i < top->num_urh; i++) {
        cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
        cJSON_AddNumberToObject(jitem, "year",                    top->urh[i].year);
        cJSON_AddNumberToObject(jitem, "month",                   top->urh[i].month);
        cJSON_AddNumberToObject(jitem, "day",                     top->urh[i].day);
        cJSON_AddNumberToObject(jitem, "usage",                   WA_DBL_FMT(top->urh[i].usage, 2));
        cJSON_AddNumberToObject(jitem, "degree_days",             WA_DBL_FMT(top->urh[i].degree_days, 2));
      }
    }
  }

  if (top->ubc.period_days) {
    cJSON_AddItemToObject(jroot,   "utility_bills_pre_retrofit_cooling",     jitem = cJSON_CreateObject());
    //cJSON_AddNumberToObject(jitem, "usage_units",                   top->ubc.usage_units);
    cJSON_AddNumberToObject(jitem, "period_days",                   top->ubc.period_days);
    WA_AddNumToObjectNoZero(jitem, "base_temp",                     WA_DBL_FMT(top->ubc.base_temp, 2));
    WA_AddNumToObjectNoZero(jitem, "base_load",                     WA_DBL_FMT(top->ubc.base_load, 2));

    if (top->num_urc) {
      cJSON_AddItemToObject(jroot,   "utility_bills_pre_retrofit_cooling_data",     jarray = cJSON_CreateArray());
      for (i = 0; i < top->num_urc; i++) {
        cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
        cJSON_AddNumberToObject(jitem, "year",                    top->urc[i].year);
        cJSON_AddNumberToObject(jitem, "month",                   top->urc[i].month);
        cJSON_AddNumberToObject(jitem, "day",                     top->urc[i].day);
        cJSON_AddNumberToObject(jitem, "usage",                   WA_DBL_FMT(top->urc[i].usage, 2));
        cJSON_AddNumberToObject(jitem, "degree_days",             WA_DBL_FMT(top->urc[i].degree_days, 2));
      }
    }
  }

  cJSON_AddItemToObject(jroot, "weather_location", jitem = cJSON_CreateObject());
  cJSON_AddStringToObject(jitem, "state",                   top->wth.state);
  cJSON_AddStringToObject(jitem, "city",                    top->wth.city);
  cJSON_AddStringToObject(jitem, "file",                    top->wth.file);

  cJSON_AddItemToObject(jroot, "fuel_costs", jitem = cJSON_CreateObject());
  WA_AddNumToObjectNoZero(jitem, "natural_gas",                   WA_DBL_FMT(top->fcs.natural_gas, 4));
  WA_AddNumToObjectNoZero(jitem, "oil",                           WA_DBL_FMT(top->fcs.oil, 4));
  WA_AddNumToObjectNoZero(jitem, "electric",                      WA_DBL_FMT(top->fcs.electric, 4));
  WA_AddNumToObjectNoZero(jitem, "propane",                       WA_DBL_FMT(top->fcs.propane, 4));
  WA_AddNumToObjectNoZero(jitem, "wood",                          WA_DBL_FMT(top->fcs.wood, 4));
  WA_AddNumToObjectNoZero(jitem, "coal",                          WA_DBL_FMT(top->fcs.coal, 4));
  WA_AddNumToObjectNoZero(jitem, "kerosene",                      WA_DBL_FMT(top->fcs.kerosene, 4));
  WA_AddNumToObjectNoZero(jitem, "other",                         WA_DBL_FMT(top->fcs.other, 4));
  WA_AddNumToObjectNoZero(jitem, "natural_gas_heat",              WA_DBL_FMT(top->fcs.natural_gas_heat, 4));
  WA_AddNumToObjectNoZero(jitem, "oil_heat",                      WA_DBL_FMT(top->fcs.oil_heat, 4));
  WA_AddNumToObjectNoZero(jitem, "electric_heat",                 WA_DBL_FMT(top->fcs.electric_heat, 6));
  WA_AddNumToObjectNoZero(jitem, "propane_heat",                  WA_DBL_FMT(top->fcs.propane_heat, 4));
  WA_AddNumToObjectNoZero(jitem, "wood_heat",                     WA_DBL_FMT(top->fcs.wood_heat, 4));
  WA_AddNumToObjectNoZero(jitem, "coal_heat",                     WA_DBL_FMT(top->fcs.coal_heat, 4));
  WA_AddNumToObjectNoZero(jitem, "kerosene_heat",                 WA_DBL_FMT(top->fcs.kerosene_heat, 4));
  WA_AddNumToObjectNoZero(jitem, "other_heat",                    WA_DBL_FMT(top->fcs.other_heat, 4));

  if (top->rer.year) {
    cJSON_AddItemToObject(jroot, "fuel_escalation_rates_by_reference", jitem = cJSON_CreateObject());
    cJSON_AddNumberToObject(jitem, "year",                   top->rer.year);
    if (strlen(top->rer.state)) {cJSON_AddStringToObject(jitem, "state",                  top->rer.state);  }
    if (top->rer.region)        {cJSON_AddNumberToObject(jitem, "region",                 top->rer.region); }
  } else {
    cJSON_AddItemToObject(jroot,   "fuel_escalation_rates",     jarray = cJSON_CreateArray());
    for (i = 0; i < top->num_fer; i++) {
      cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
      cJSON_AddNumberToObject(jitem, "fuel_type_id",              i + 1);
      if (strlen(top->fer[i].fuelname)) cJSON_AddStringToObject(jitem, "fuel_name",        top->fer[i].fuelname);
      cJSON_AddItemToObject(  jitem, "rate",                    jarray2 = cJSON_CreateArray());
      for (j = 0; j < NUM_FUEL_RATES; j++) {
        cJSON_AddItemToArray(jarray2, cJSON_CreateNumber(WA_DBL_FMT(top->fer[i].rates[j], 2)));
      }
    }
  }

  cJSON_AddItemToObject(jroot,   "measure_active_flags",     jarray = cJSON_CreateArray());
  for (i = 0; i < MHEA_MAX_CMS; i++) {
    if (strlen(top->cms[i].measure_name)) {
      cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
      cJSON_AddNumberToObject(jitem, "id",          i);
      WA_AddBoolToObject(     jitem, "active",       top->cms[i].active);
      cJSON_AddStringToObject(jitem, "measure_name", top->cms[i].measure_name);
    }
  }

  cJSON_AddItemToObject(jroot,   "measure_costs",     jarray = cJSON_CreateArray());
  for (i = 0; i < MHEA_MAX_RMC; i++) {        // traverse whole array
    if (strlen(top->rmc[i].retro_name)) {     // there are some blank (never read in) entries
      cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
      cJSON_AddNumberToObject(jitem, "id",                  top->rmc[i].measure_id);
      cJSON_AddStringToObject(jitem, "retro_name",          top->rmc[i].retro_name);
      cJSON_AddNumberToObject(jitem, "life",                WA_DBL_FMT(top->rmc[i].life, 2));
      cJSON_AddStringToObject(jitem, "units",               top->rmc[i].units);
      cJSON_AddNumberToObject(jitem, "material",            WA_DBL_FMT(top->rmc[i].material, 2));
      cJSON_AddNumberToObject(jitem, "labor",               WA_DBL_FMT(top->rmc[i].labor, 2));
      cJSON_AddNumberToObject(jitem, "extra",               WA_DBL_FMT(top->rmc[i].extra, 2));
    }
  }

  cJSON_AddItemToObject(jroot,   "key_parameters",     jitem = cJSON_CreateObject());

  cJSON_AddNumberToObject(jitem, "real_discount_rate",                          WA_DBL_FMT(top->key.real_discount_rate, 2));
  cJSON_AddNumberToObject(jitem, "minimum_acceptable_sir",                      WA_DBL_FMT(top->key.minimum_acceptable_sir, 2));
  cJSON_AddNumberToObject(jitem, "spending_limit",                              WA_DBL_FMT(top->key.spending_limit, 2));

  cJSON_AddNumberToObject(jitem, "heating_setpoint_day",                        WA_DBL_FMT(top->key.heating_setpoint_day, 2));
  cJSON_AddNumberToObject(jitem, "heating_setpoint_night",                      WA_DBL_FMT(top->key.heating_setpoint_night, 2));
  cJSON_AddNumberToObject(jitem, "cooling_setpoint_day",                        WA_DBL_FMT(top->key.cooling_setpoint_day, 2));
  cJSON_AddNumberToObject(jitem, "cooling_setpoint_night",                      WA_DBL_FMT(top->key.cooling_setpoint_night, 2));
  cJSON_AddNumberToObject(jitem, "thermostat_setback_amount",                   WA_DBL_FMT(top->key.thermostat_setback_amount, 2));
  cJSON_AddNumberToObject(jitem, "length_of_night_thermostat_setback",          WA_DBL_FMT(top->key.length_of_night_thermostat_setback, 2));

  cJSON_AddNumberToObject(jitem, "bag_size_for_loose_fiberglass_insulation",    WA_DBL_FMT(top->key.bag_size_for_loose_fiberglass_insulation, 2));
  cJSON_AddNumberToObject(jitem, "density_of_loose_fiberglass_insulation",      WA_DBL_FMT(top->key.density_of_loose_fiberglass_insulation, 2));
  cJSON_AddNumberToObject(jitem, "bag_size_for_loose_cellulose_insulation",     WA_DBL_FMT(top->key.bag_size_for_loose_cellulose_insulation, 2));
  cJSON_AddNumberToObject(jitem, "density_of_loose_cellulose_insulation",       WA_DBL_FMT(top->key.density_of_loose_cellulose_insulation, 2));
  cJSON_AddNumberToObject(jitem, "interior_ceiling_r_value_winter",             WA_DBL_FMT(top->key.interior_ceiling_r_value_winter, 2));
  cJSON_AddNumberToObject(jitem, "interior_ceiling_r_value_summer",             WA_DBL_FMT(top->key.interior_ceiling_r_value_summer, 2));
  cJSON_AddNumberToObject(jitem, "interior_floor_r_value_winter",               WA_DBL_FMT(top->key.interior_floor_r_value_winter, 2));
  cJSON_AddNumberToObject(jitem, "interior_floor_r_value_summer",               WA_DBL_FMT(top->key.interior_floor_r_value_summer, 2));
  cJSON_AddNumberToObject(jitem, "interior_wall_r_value_winter",                WA_DBL_FMT(top->key.interior_wall_r_value_winter, 2));
  cJSON_AddNumberToObject(jitem, "interior_wall_r_value_summer",                WA_DBL_FMT(top->key.interior_wall_r_value_summer, 2));
  cJSON_AddNumberToObject(jitem, "outside_wall_r_value_winter",                 WA_DBL_FMT(top->key.outside_wall_r_value_winter, 2));
  cJSON_AddNumberToObject(jitem, "outside_wall_r_value_summer",                 WA_DBL_FMT(top->key.outside_wall_r_value_summer, 2));
  cJSON_AddNumberToObject(jitem, "loose_insulation_r_value_per_inch",           WA_DBL_FMT(top->key.loose_insulation_r_value_per_inch, 2));
  cJSON_AddNumberToObject(jitem, "batt_blanket_insulation_r_value_per_inch",    WA_DBL_FMT(top->key.batt_blanket_insulation_r_value_per_inch, 2));
  cJSON_AddNumberToObject(jitem, "rigid_insulation_r_value_per_inch",           WA_DBL_FMT(top->key.rigid_insulation_r_value_per_inch, 2));
  cJSON_AddNumberToObject(jitem, "foamcore_insulation_r_value_per_inch",        WA_DBL_FMT(top->key.foamcore_insulation_r_value_per_inch, 2));

  cJSON_AddNumberToObject(jitem, "home_leakiness_loose",                        WA_DBL_FMT(top->key.home_leakiness_loose, 2));
  cJSON_AddNumberToObject(jitem, "home_leakiness_medium",                       WA_DBL_FMT(top->key.home_leakiness_medium, 2));
  cJSON_AddNumberToObject(jitem, "home_leakiness_tight",                        WA_DBL_FMT(top->key.home_leakiness_tight, 2));
  cJSON_AddNumberToObject(jitem, "free_heat_from_interior_sources_day",         WA_DBL_FMT(top->key.free_heat_from_interior_sources_day, 2));
  cJSON_AddNumberToObject(jitem, "free_heat_from_interior_sources_night",       WA_DBL_FMT(top->key.free_heat_from_interior_sources_night, 2));
  cJSON_AddNumberToObject(jitem, "duct_sealing_distribution_loss_reduction",    WA_DBL_FMT(top->key.duct_sealing_distribution_loss_reduction, 2));
  cJSON_AddNumberToObject(jitem, "duct_insulation_dist_loss_reduction",         WA_DBL_FMT(top->key.duct_insulation_dist_loss_reduction, 2));
  cJSON_AddNumberToObject(jitem, "evaporative_cooler_actual_saturating_eff",    WA_DBL_FMT(top->key.evaporative_cooler_actual_saturating_eff, 2));
  cJSON_AddNumberToObject(jitem, "saturating_eff_for_evaporative_tune_up",      WA_DBL_FMT(top->key.saturating_eff_for_evaporative_tune_up, 2));
  cJSON_AddNumberToObject(jitem, "saturating_eff_for_evaporative_rplcmnt",      WA_DBL_FMT(top->key.saturating_eff_for_evaporative_rplcmnt, 2));
  cJSON_AddNumberToObject(jitem, "cooling_system_fan_power",                    WA_DBL_FMT(top->key.cooling_system_fan_power, 2));

  cJSON_AddNumberToObject(jitem, "door_u_value_wood_with_solid_core",           WA_DBL_FMT(top->key.door_u_value_wood_with_solid_core, 2));
  cJSON_AddNumberToObject(jitem, "door_u_value_wood_with_hollow_core",          WA_DBL_FMT(top->key.door_u_value_wood_with_hollow_core, 2));
  cJSON_AddNumberToObject(jitem, "door_u_value_standard_mfg_home_door",         WA_DBL_FMT(top->key.door_u_value_standard_mfg_home_door, 2));
  cJSON_AddNumberToObject(jitem, "u_value_of_replacement_door",                 WA_DBL_FMT(top->key.u_value_of_replacement_door, 2));

  cJSON_AddNumberToObject(jitem, "window_u_value_1_glazing_summer",             WA_DBL_FMT(top->key.window_u_value_1_glazing_summer, 2));
  cJSON_AddNumberToObject(jitem, "window_u_value_1_glazing_winter",             WA_DBL_FMT(top->key.window_u_value_1_glazing_winter, 2));
  cJSON_AddNumberToObject(jitem, "window_u_value_2_glazing_summer",             WA_DBL_FMT(top->key.window_u_value_2_glazing_summer, 2));
  cJSON_AddNumberToObject(jitem, "window_u_value_2_glazing_winter",             WA_DBL_FMT(top->key.window_u_value_2_glazing_winter, 2));
  cJSON_AddNumberToObject(jitem, "window_u_value_1_glass_storm_summer",         WA_DBL_FMT(top->key.window_u_value_1_glass_storm_summer, 2));
  cJSON_AddNumberToObject(jitem, "window_u_value_1_glass_storm_winter",         WA_DBL_FMT(top->key.window_u_value_1_glass_storm_winter, 2));
  cJSON_AddNumberToObject(jitem, "window_u_value_2_glass_storm_summer",         WA_DBL_FMT(top->key.window_u_value_2_glass_storm_summer, 2));
  cJSON_AddNumberToObject(jitem, "window_u_value_2_glass_storm_winter",         WA_DBL_FMT(top->key.window_u_value_2_glass_storm_winter, 2));
  cJSON_AddNumberToObject(jitem, "window_u_value_1_plastic_storm_summer",       WA_DBL_FMT(top->key.window_u_value_1_plastic_storm_summer, 2));
  cJSON_AddNumberToObject(jitem, "window_u_value_1_plastic_storm_winter",       WA_DBL_FMT(top->key.window_u_value_1_plastic_storm_winter, 2));
  cJSON_AddNumberToObject(jitem, "window_u_value_2_plastic_storm_summer",       WA_DBL_FMT(top->key.window_u_value_2_plastic_storm_summer, 2));
  cJSON_AddNumberToObject(jitem, "window_u_value_2_plastic_storm_winter",       WA_DBL_FMT(top->key.window_u_value_2_plastic_storm_winter, 2));
  cJSON_AddNumberToObject(jitem, "skylight_u_value_1_glazing_summer",           WA_DBL_FMT(top->key.skylight_u_value_1_glazing_summer, 2));
  cJSON_AddNumberToObject(jitem, "skylight_u_value_1_glazing_winter",           WA_DBL_FMT(top->key.skylight_u_value_1_glazing_winter, 2));
  cJSON_AddNumberToObject(jitem, "skylight_u_value_2_glazing_summer",           WA_DBL_FMT(top->key.skylight_u_value_2_glazing_summer, 2));
  cJSON_AddNumberToObject(jitem, "skylight_u_value_2_glazing_winter",           WA_DBL_FMT(top->key.skylight_u_value_2_glazing_winter, 2));
  cJSON_AddNumberToObject(jitem, "skylight_u_value_1_glass_storm_summer",       WA_DBL_FMT(top->key.skylight_u_value_1_glass_storm_summer, 2));
  cJSON_AddNumberToObject(jitem, "skylight_u_value_1_glass_storm_winter",       WA_DBL_FMT(top->key.skylight_u_value_1_glass_storm_winter, 2));
  cJSON_AddNumberToObject(jitem, "skylight_u_value_2_glass_storm_summer",       WA_DBL_FMT(top->key.skylight_u_value_2_glass_storm_summer, 2));
  cJSON_AddNumberToObject(jitem, "skylight_u_value_2_glass_storm_winter",       WA_DBL_FMT(top->key.skylight_u_value_2_glass_storm_winter, 2));
  cJSON_AddNumberToObject(jitem, "skylight_u_value_1_plstc_storm_summer",       WA_DBL_FMT(top->key.skylight_u_value_1_plstc_storm_summer, 2));
  cJSON_AddNumberToObject(jitem, "skylight_u_value_1_plstc_storm_winter",       WA_DBL_FMT(top->key.skylight_u_value_1_plstc_storm_winter, 2));
  cJSON_AddNumberToObject(jitem, "skylight_u_value_2_plstc_storm_summer",       WA_DBL_FMT(top->key.skylight_u_value_2_plstc_storm_summer, 2));
  cJSON_AddNumberToObject(jitem, "skylight_u_value_2_plstc_storm_winter",       WA_DBL_FMT(top->key.skylight_u_value_2_plstc_storm_winter, 2));
  cJSON_AddNumberToObject(jitem, "window_shading_r_value_drapes",               WA_DBL_FMT(top->key.window_shading_r_value_drapes, 2));
  cJSON_AddNumberToObject(jitem, "window_shading_r_value_blinds_shades",        WA_DBL_FMT(top->key.window_shading_r_value_blinds_shades, 2));
  cJSON_AddNumberToObject(jitem, "window_shading_r_value_drapes_shades",        WA_DBL_FMT(top->key.window_shading_r_value_drapes_shades, 2));
  cJSON_AddNumberToObject(jitem, "sun_screen_solar_trans_reduction_summer",     WA_DBL_FMT(top->key.sun_screen_solar_trans_reduction_summer, 2));
  cJSON_AddNumberToObject(jitem, "sun_screen_solar_trans_reduction_winter",     WA_DBL_FMT(top->key.sun_screen_solar_trans_reduction_winter, 2));
  cJSON_AddNumberToObject(jitem, "ratio_of_awning_depth_to_window_height",      WA_DBL_FMT(top->key.ratio_of_awning_depth_to_window_height, 2));

  cJSON_AddNumberToObject(jitem, "low_flow_shower_head_flow_rate",              WA_DBL_FMT(top->key.low_flow_shower_head_flow_rate, 2));
  cJSON_AddNumberToObject(jitem, "water_heater_wrap_added_r_value",             WA_DBL_FMT(top->key.water_heater_wrap_added_r_value, 2));
  cJSON_AddNumberToObject(jitem, "refrigerator_defrost_cycle_energy",           WA_DBL_FMT(top->key.refrigerator_defrost_cycle_energy, 2));

  //clang-format on

  char *output = NULL;
  if (cmds.format_json_output) {
    output = cJSON_Print(jroot);             // allocates the formatted JSON output string and returns it
  } else {
    output = cJSON_PrintUnformatted(jroot);  // allocates the un-formatted JSON output string and returns it
  }

  write_json_echo_to_file(output);

  if (output) free(output);             // done with monster output string
  if (jroot)                      // pretty sure this cleans up the sub cJSON objects
    cJSON_Delete(jroot); 

}

// Writes the JSON representation of the typedef struct MOR to the output_file_path.  The
// calling function needs to open the file handle and this function will close it
void mhea_json_result_write(MDI *top, MOR *res) {
  cJSON *jroot = NULL; // the JSON tree echo of our NEAT_RESULTS structure
  //cJSON *jdata = NULL;
  cJSON *jarray = NULL;
  cJSON *jitem = NULL;
  int dd_act_present;       // boolean indicating if there are any non zero entries for degree day actual figures #231
  int i;

  ASSERT(res, sprintf(msg, "Must have the MHEA results structure filled out"));

  // if (strcmp(cmds.output_file_path, STD_OUTPUT) == 0) {
  //   out_file = stdout;
  // } else {
  //   out_file = fopen(cmds.output_file_path, "wb");
  //   ASSERT(out_file, sprintf(msg, "Failed to open the json results output file: %s code:%d:%s", cmds.output_file_path, errno, strerror(errno)));
  // }

  jroot = cJSON_CreateObject();

  // Only return a top level JSON object that can be packaged however the calling process decides #121 
  //cJSON_AddFalseToObject(jroot, "success");
  //cJSON_AddItemToObject(jroot, "data", jdata = cJSON_CreateObject());

  // here is a batch of run meta data to tag onto the top of the JSON
  // clang-format off
  
  if (strlen(cmds.run_identifier)) cJSON_AddStringToObject(jroot,    "run_identifier",     cmds.run_identifier);
  cJSON_AddStringToObject(jroot,    "audit_type",         top->gnl.audit_type);
  cJSON_AddNumberToObject(jroot,    "audit_id",           top->gnl.audit_id);
  cJSON_AddNumberToObject(jroot,    "audit_number",       top->gnl.audit_number);

  cJSON_AddNumberToObject(jroot,    "length",    top->gnl.length);
  cJSON_AddNumberToObject(jroot,    "width",     top->gnl.width);

  if (!cmds.regression_test) {
    time_t rawtime;
    struct tm *info;
    char time_buffer[80];
    time(&rawtime);
    info = localtime(&rawtime);
    strftime(time_buffer, 80, "%c", info);
    cJSON_AddStringToObject(jroot,    "run_timestamp",      time_buffer);
    cJSON_AddStringToObject(jroot,    "run_version",        WA_VERSION);
  }
  cJSON_AddNumberToObject(jroot,    "energy_calc_counter",   res->energy_calc_counter);

  cJSON_AddNumberToObject(jroot,    "pre_heat",   WA_DBL_FMT(res->pre_heat,1));
  cJSON_AddNumberToObject(jroot,    "pre_cool",   WA_DBL_FMT(res->pre_cool,1));
  cJSON_AddNumberToObject(jroot,    "pre_base",   WA_DBL_FMT(res->pre_base,1));

  cJSON_AddNumberToObject(jroot,    "post_heat",  WA_DBL_FMT(res->post_heat,1));
  cJSON_AddNumberToObject(jroot,    "post_cool",  WA_DBL_FMT(res->post_cool,1));
  cJSON_AddNumberToObject(jroot,    "post_base",  WA_DBL_FMT(res->post_base,1));

  // back to regularly scheduled res structure
  cJSON_AddNumberToObject(jroot, "num_measure",  res->num_measure);
  cJSON_AddItemToObject(jroot,   "measures",     jarray = cJSON_CreateArray());
  for (i = 0; i < res->num_measure; i++) {
    cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
    cJSON_AddNumberToObject(jitem, "index",         res->measure[i].index);
    cJSON_AddNumberToObject(jitem, "measure_id",    res->measure[i].measure_id);
    cJSON_AddNumberToObject(jitem, "component_id",  res->measure[i].component_id);
    cJSON_AddNumberToObject(jitem, "audit_section_id",    res->measure[i].audit_section_id);
    cJSON_AddStringToObject(jitem, "measure",       res->measure[i].measure);
    cJSON_AddStringToObject(jitem, "components",    res->measure[i].components);
    cJSON_AddNumberToObject(jitem, "heating_mmbtu",  WA_DBL_FMT(res->measure[i].heating_mmbtu, 3));
    cJSON_AddNumberToObject(jitem, "heating_sav",   WA_DBL_FMT(res->measure[i].heating_sav, 2));
    cJSON_AddNumberToObject(jitem, "cooling_kwh",   WA_DBL_FMT(res->measure[i].cooling_kwh, 1));
    cJSON_AddNumberToObject(jitem, "cooling_sav",   WA_DBL_FMT(res->measure[i].cooling_sav, 2));
    cJSON_AddNumberToObject(jitem, "baseload_kwh",  WA_DBL_FMT(res->measure[i].baseload_kwh, 1));
    cJSON_AddNumberToObject(jitem, "baseload_sav",  WA_DBL_FMT(res->measure[i].baseload_sav, 2));
    cJSON_AddNumberToObject(jitem, "total_mmbtu",    WA_DBL_FMT(res->measure[i].total_mmbtu, 3));
    cJSON_AddNumberToObject(jitem, "savings",       WA_DBL_FMT(res->measure[i].savings, 2));
    cJSON_AddNumberToObject(jitem, "cost",          WA_DBL_FMT(res->measure[i].cost, 2));
    cJSON_AddNumberToObject(jitem, "sir",           WA_DBL_FMT(res->measure[i].sir, 3));
    cJSON_AddNumberToObject(jitem, "lifetime",      res->measure[i].lifetime);
    cJSON_AddNumberToObject(jitem, "qtym",          WA_DBL_FMT(res->measure[i].qtym, 3));
    cJSON_AddNumberToObject(jitem, "qtyl",          WA_DBL_FMT(res->measure[i].qtyl, 3));
    cJSON_AddNumberToObject(jitem, "qtyi",          WA_DBL_FMT(res->measure[i].qtyi, 3));
    cJSON_AddNumberToObject(jitem, "costum",        WA_DBL_FMT(res->measure[i].costum, 2));
    cJSON_AddNumberToObject(jitem, "costul",        WA_DBL_FMT(res->measure[i].costul, 3));
    cJSON_AddNumberToObject(jitem, "costi1",        WA_DBL_FMT(res->measure[i].costi1, 2));
    cJSON_AddNumberToObject(jitem, "costi2",        WA_DBL_FMT(res->measure[i].costi2, 2));
    cJSON_AddStringToObject(jitem, "desci2",        res->measure[i].desci2);
    cJSON_AddNumberToObject(jitem, "typei2",        res->measure[i].typei2);
    cJSON_AddNumberToObject(jitem, "costi3",        WA_DBL_FMT(res->measure[i].costi3, 2));
    cJSON_AddStringToObject(jitem, "desci3",        res->measure[i].desci3);
    cJSON_AddNumberToObject(jitem, "typei3",        res->measure[i].typei3);
  }

  cJSON_AddNumberToObject(jroot, "num_an_sav", res->num_an_sav);
  cJSON_AddItemToObject(jroot,   "an_sav", jarray = cJSON_CreateArray());
  for (i = 0; i < res->num_an_sav; i++) {
    cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
    cJSON_AddNumberToObject(jitem, "index",         res->an_sav[i].index);
    cJSON_AddNumberToObject(jitem, "measure_index", res->an_sav[i].measure_index);
    cJSON_AddStringToObject(jitem, "measure",       res->an_sav[i].measure);
    cJSON_AddStringToObject(jitem, "components",    res->an_sav[i].components);
    cJSON_AddNumberToObject(jitem, "heating_mmbtu",  WA_DBL_FMT(res->an_sav[i].heating_mmbtu, 3));
    cJSON_AddNumberToObject(jitem, "heating_sav",   WA_DBL_FMT(res->an_sav[i].heating_sav, 2));
    cJSON_AddNumberToObject(jitem, "cooling_kwh",   WA_DBL_FMT(res->an_sav[i].cooling_kwh, 1));
    cJSON_AddNumberToObject(jitem, "cooling_sav",   WA_DBL_FMT(res->an_sav[i].cooling_sav, 2));
    cJSON_AddNumberToObject(jitem, "baseload_kwh",  WA_DBL_FMT(res->an_sav[i].baseload_kwh, 1));
    cJSON_AddNumberToObject(jitem, "baseload_sav",  WA_DBL_FMT(res->an_sav[i].baseload_sav, 2));
    cJSON_AddNumberToObject(jitem, "total_mmbtu",    WA_DBL_FMT(res->an_sav[i].total_mmbtu, 3));
  }

  cJSON_AddNumberToObject(jroot, "num_an_asav", res->num_an_asav);
  cJSON_AddItemToObject(jroot, "an_asav", jarray = cJSON_CreateArray());
  for (i = 0; i < res->num_an_asav; i++) {
    cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
    cJSON_AddNumberToObject(jitem, "index",         res->an_asav[i].index);
    cJSON_AddNumberToObject(jitem, "measure_index", res->an_asav[i].measure_index);
    cJSON_AddStringToObject(jitem, "measure",       res->an_asav[i].measure);
    cJSON_AddStringToObject(jitem, "components",    res->an_asav[i].components);
    cJSON_AddNumberToObject(jitem, "heating_mmbtu",  WA_DBL_FMT(res->an_asav[i].heating_mmbtu, 3));
    cJSON_AddNumberToObject(jitem, "heating_sav",   WA_DBL_FMT(res->an_asav[i].heating_sav, 2));
    cJSON_AddNumberToObject(jitem, "cooling_kwh",   WA_DBL_FMT(res->an_asav[i].cooling_kwh, 1));
    cJSON_AddNumberToObject(jitem, "cooling_sav",   WA_DBL_FMT(res->an_asav[i].cooling_sav, 2));
    cJSON_AddNumberToObject(jitem, "baseload_kwh",  WA_DBL_FMT(res->an_asav[i].baseload_kwh, 1));
    cJSON_AddNumberToObject(jitem, "baseload_sav",  WA_DBL_FMT(res->an_asav[i].baseload_sav, 2));
    cJSON_AddNumberToObject(jitem, "total_mmbtu",    WA_DBL_FMT(res->an_asav[i].total_mmbtu, 3));
  }

  cJSON_AddNumberToObject(jroot, "num_sir", res->num_sir);
  cJSON_AddItemToObject(jroot, "sir", jarray = cJSON_CreateArray());
  for (i = 0; i < res->num_sir; i++) {
    cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
    cJSON_AddNumberToObject(jitem, "index",         res->sir[i].index);
    cJSON_AddNumberToObject(jitem, "measure_index", res->sir[i].measure_index);
    cJSON_AddNumberToObject(jitem, "group",       res->sir[i].group);
    cJSON_AddStringToObject(jitem, "measure",     res->sir[i].measure);
    cJSON_AddStringToObject(jitem, "components",  res->sir[i].components);
    cJSON_AddNumberToObject(jitem, "savings",     WA_DBL_FMT(res->sir[i].savings, 2));
    cJSON_AddNumberToObject(jitem, "cost",        WA_DBL_FMT(res->sir[i].cost, 2));
    cJSON_AddNumberToObject(jitem, "sir",         WA_DBL_FMT(res->sir[i].sir, 3));
    cJSON_AddNumberToObject(jitem, "ccost",       WA_DBL_FMT(res->sir[i].ccost, 2));
    cJSON_AddNumberToObject(jitem, "csir",        WA_DBL_FMT(res->sir[i].csir, 3));
  }

  cJSON_AddNumberToObject(jroot, "num_asir", res->num_asir);
  cJSON_AddItemToObject(jroot, "asir", jarray = cJSON_CreateArray());
  for (i = 0; i < res->num_asir; i++) {
    cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
    cJSON_AddNumberToObject(jitem, "index",         res->asir[i].index);
    cJSON_AddNumberToObject(jitem, "measure_index", res->asir[i].measure_index);
    cJSON_AddNumberToObject(jitem, "group",       res->asir[i].group);
    cJSON_AddStringToObject(jitem, "measure",     res->asir[i].measure);
    cJSON_AddStringToObject(jitem, "components",  res->asir[i].components);
    cJSON_AddNumberToObject(jitem, "savings",     WA_DBL_FMT(res->asir[i].savings, 2));
    cJSON_AddNumberToObject(jitem, "cost",        WA_DBL_FMT(res->asir[i].cost, 2));
    cJSON_AddNumberToObject(jitem, "sir",         WA_DBL_FMT(res->asir[i].sir, 3));
    cJSON_AddNumberToObject(jitem, "ccost",       WA_DBL_FMT(res->asir[i].ccost, 2));
    cJSON_AddNumberToObject(jitem, "csir",        WA_DBL_FMT(res->asir[i].csir, 3));
  }

  cJSON_AddNumberToObject(jroot, "num_material", res->num_material);
  cJSON_AddItemToObject(jroot, "material", jarray = cJSON_CreateArray());
  for (i = 0; i < res->num_material; i++) {
    cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
    cJSON_AddNumberToObject(jitem, "index",         res->material[i].index);
    cJSON_AddNumberToObject(jitem, "measure_index", res->material[i].measure_index);
    cJSON_AddNumberToObject(jitem, "material_id",   res->material[i].material_id);
    cJSON_AddStringToObject(jitem, "material",  res->material[i].material);
    cJSON_AddStringToObject(jitem, "type",      res->material[i].type);
    cJSON_AddNumberToObject(jitem, "quantity",  WA_DBL_FMT(res->material[i].quantity, 3));
    cJSON_AddStringToObject(jitem, "units",     res->material[i].units);
  }

  cJSON_AddNumberToObject(jroot, "num_amaterial", res->num_amaterial);
  cJSON_AddItemToObject(jroot, "amaterial", jarray = cJSON_CreateArray());
  for (i = 0; i < res->num_amaterial; i++) {
    cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
    cJSON_AddNumberToObject(jitem, "index",         res->amaterial[i].index);
    cJSON_AddNumberToObject(jitem, "measure_index", res->amaterial[i].measure_index);
    cJSON_AddNumberToObject(jitem, "material_id",   res->amaterial[i].material_id);    
    cJSON_AddStringToObject(jitem, "material",  res->amaterial[i].material);
    cJSON_AddStringToObject(jitem, "type",      res->amaterial[i].type);
    cJSON_AddNumberToObject(jitem, "quantity",  WA_DBL_FMT(res->amaterial[i].quantity, 3));
    cJSON_AddStringToObject(jitem, "units",     res->amaterial[i].units);
  }

  cJSON_AddNumberToObject(jroot, "num_message", res->num_message);
  cJSON_AddItemToObject(jroot, "message", jarray = cJSON_CreateArray());
  for (i = 0; i < res->num_message; i++) {
    cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
    cJSON_AddNumberToObject(jitem, "index", i + 1);
    cJSON_AddStringToObject(jitem, "msg",   res->message[i]);
  }

  // cJSON_AddNumberToObject(jroot, "num_comment", res->num_comment); // always 4, not in RESULT structure, but echoed here
  // cJSON_AddItemToObject(jroot, "comment", jarray = cJSON_CreateArray());
  // for (i = 0; i < res->num_comment; i++) {
  //   cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
  //   cJSON_AddNumberToObject(jitem, "index", res->comment[i].index);
  //   cJSON_AddStringToObject(jitem, "code",  res->comment[i].code);
  //   cJSON_AddStringToObject(jitem, "type",  res->comment[i].type);
  //   cJSON_AddStringToObject(jitem, "msg",   res->comment[i].msg);
  // }

  cJSON_AddNumberToObject(jroot, "num_manj", res->num_manj);
  cJSON_AddItemToObject(jroot, "manj", jarray = cJSON_CreateArray());
  for (i = 0; i < res->num_manj; i++) {
    cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
    cJSON_AddNumberToObject(jitem, "index",       res->manj[i].index);
    cJSON_AddStringToObject(jitem, "heatcool",    res->manj[i].heatcool);
    cJSON_AddStringToObject(jitem, "type",        res->manj[i].type);
    cJSON_AddStringToObject(jitem, "name",        res->manj[i].name);
    cJSON_AddNumberToObject(jitem, "area_vol",    WA_DBL_FMT(res->manj[i].area_vol, 3));
    cJSON_AddNumberToObject(jitem, "pre_load",    WA_DBL_FMT(res->manj[i].pre_load, 3));
    cJSON_AddNumberToObject(jitem, "post_load",   WA_DBL_FMT(res->manj[i].post_load, 3));
  }

  // #231
  dd_act_present = FALSE;
  for (i = 0; i < res->num_heat_comp; i++) { 
    if (res->heat_comp[i].dd_act) {
      dd_act_present = TRUE; 
      break;
    } 
  }
  cJSON_AddStringToObject(jroot, "heat_comp_units", res->heat_comp_units);
  cJSON_AddNumberToObject(jroot, "heat_dd_base",    res->heat_dd_base);
  cJSON_AddNumberToObject(jroot, "num_heat_comp",   res->num_heat_comp);
  cJSON_AddItemToObject(jroot, "heat_comp", jarray = cJSON_CreateArray());
  for (i = 0; i < res->num_heat_comp; i++) {
    cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
    cJSON_AddNumberToObject(jitem, "index",         res->heat_comp[i].index);
    cJSON_AddNumberToObject(jitem, "year",          res->heat_comp[i].year);
    cJSON_AddNumberToObject(jitem, "month",         res->heat_comp[i].month);
    cJSON_AddNumberToObject(jitem, "day",           res->heat_comp[i].day);
    cJSON_AddNumberToObject(jitem, "period_days",   res->heat_comp[i].period_days);
    cJSON_AddNumberToObject(jitem, "consump_act",   res->heat_comp[i].consump_act);
    cJSON_AddNumberToObject(jitem, "consump_pred",  res->heat_comp[i].consump_pred);
    if (dd_act_present)
      cJSON_AddNumberToObject(jitem, "dd_act",      res->heat_comp[i].dd_act);
    else
      cJSON_AddNullToObject(jitem, "dd_act");    
    cJSON_AddNumberToObject(jitem, "dd_pred",       res->heat_comp[i].dd_pred);
  }

  // #231
  dd_act_present = FALSE;
  for (i = 0; i < res->num_cool_comp; i++) { 
    if (res->cool_comp[i].dd_act) {
      dd_act_present = TRUE; 
      break;
    } 
  }
  cJSON_AddStringToObject(jroot, "cool_comp_units", res->cool_comp_units);
  cJSON_AddNumberToObject(jroot, "cool_dd_base",    res->cool_dd_base);
  cJSON_AddNumberToObject(jroot, "num_cool_comp",   res->num_cool_comp);
  cJSON_AddItemToObject(jroot, "cool_comp", jarray = cJSON_CreateArray());
  for (i = 0; i < res->num_cool_comp; i++) {
    cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
    cJSON_AddNumberToObject(jitem, "index",         res->cool_comp[i].index);
    cJSON_AddNumberToObject(jitem, "year",          res->cool_comp[i].year);
    cJSON_AddNumberToObject(jitem, "month",         res->cool_comp[i].month);
    cJSON_AddNumberToObject(jitem, "day",           res->cool_comp[i].day);
    cJSON_AddNumberToObject(jitem, "period_days",   res->cool_comp[i].period_days);
    cJSON_AddNumberToObject(jitem, "consump_act",   res->cool_comp[i].consump_act);
    cJSON_AddNumberToObject(jitem, "consump_pred",  res->cool_comp[i].consump_pred);
    if (dd_act_present)
      cJSON_AddNumberToObject(jitem, "dd_act",      res->cool_comp[i].dd_act);
    else
      cJSON_AddNullToObject(jitem, "dd_act");
    cJSON_AddNumberToObject(jitem, "dd_pred",       res->cool_comp[i].dd_pred);
  }

  cJSON_AddNumberToObject(jroot, "num_used_fuel", res->num_used_fuel); 
  cJSON_AddItemToObject(jroot, "used_fuel", jarray = cJSON_CreateArray());
  for (i = 0; i < res->num_used_fuel; i++) {
    cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
    cJSON_AddStringToObject(jitem, "fuel_name",             res->used_fuel[i].fuel_name);
    cJSON_AddNumberToObject(jitem, "fuel_cost",             WA_DBL_FMT(res->used_fuel[i].fuel_cost, 4));
    cJSON_AddStringToObject(jitem, "fuel_cost_units",       res->used_fuel[i].fuel_cost_units);
    cJSON_AddNumberToObject(jitem, "fuel_cost_per_mmbtu",    WA_DBL_FMT(res->used_fuel[i].fuel_cost_per_mmbtu, 4));
  }
  
  //clang-format on

  char *output = NULL;
  if (cmds.format_json_output) {
    output = cJSON_Print(jroot); // allocates the formatted JSON output string and returns it
  } else {
    output = cJSON_PrintUnformatted(jroot); // allocates the un-formatted JSON output string and returns it
  }

  write_results_to_file(output);

  if (output) free(output);             // done with monster output string
  if (jroot) cJSON_Delete(jroot);       // pretty sure this cleans up the sub cJSON objects

}
