/***************************************************************************
* MODULE:       json_bld.c            CREATED:     November 2, 2018
*
* AUTHOR:       Mark Fishbaugher (mark@fishbaugher.com)
*
* MDESC:        The JSON input handling for reading the NEAT dwelling data structure
****************************************************************************/
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "wa_engine.h"

/// Reads the fuel escalation rates from the sys/fuel_escalation JSON files.  Read the fuel escalation data from
/// the external file given the 'rer' (referenced escalation rate) information in the passed NDI structure.  It
/// uses the year and region to form a filename following a naming convention, then cJSON to read that escalation
/// factor information into the NDI 'fer' structure for later use in the engine.

static void get_referenced_escalation_rates(NDI *top, cJSON *jschema) {
  cJSON *jleaf = NULL;
  cJSON *jleaf2 = NULL;
  cJSON *jbranch = NULL;
  cJSON *jbranch2 = NULL;

  cJSON *jtree = NULL;
  char filepath[PATH_LEN];
  int fuel_id = 0;
  char *region_states[REGIONS];
  char match_state[10]; // space for vertical bars

  // look up the region from the top->rer.state if not given the region directly, MJF 1/2019

  if (top->rer.region == 0) {
    if (top->rer.state[0] == '\0' && top->wth.state[0] != '\0') {
      STRCPY(top->rer.state, top->wth.state);
    }
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

/// Reads NDI JSON input and assigns to the NDI struct. Assigns the JSON data to our NDI struct doing data checks
/// as well, both range and repeat (number of record) MIN and MAX checks.
/// Some notes on this implementation:
/// 1) This mirrors information in common/dimm.h common/struct.h neat/struct_n.h commoon/enum.h and neat/enum_n.h,
///    any changes there need to be mirrored here. The order of input follows the
///    definition of NDI in neat/struct_n.h
/// 2) Range check values are taken from the current NEAT Web GUI.  Open the associated
///    form in the current UI and call showMinMaxForCurrentForm() in the browder console to display range values.
/// 3) IMPORTANT, the assignments only run IF there is a matching JSON element within the named
///    structure, OTHERWISE, the top->xxx.yyyyyy remains in the state left by the calloc() call (all set to zero bytes)
///    So, default values are zero and null strings
/// 4) This code is MOSTLY order independent both the ordering of sections and the ordering of elements
///    within a section. The exceptions are where the J_ Macros are not used or local index variables are declared.  Those
///    sections are order dependent

void neat_json_read(NDI *top, cJSON *jtree, cJSON *jschema) {
  cJSON *jleaf = NULL;
  cJSON *jleaf2 = NULL;
  cJSON *jbranch = NULL;
  cJSON *jbranch2 = NULL;

  ASSERT(top, sprintf(msg, "Must have the NEAT NDI memory allocated"));
  ASSERT(jtree, sprintf(msg, "Must have the JSON structure filled in"));

  cJSON_ArrayForEach(jbranch, jtree) {

    J_SEC_BEG(audit);
      J_STR_ASSIGN(audit, gnl, audit_type);
      J_LNG_ASSIGN(audit, gnl, audit_id);
      J_LNG_ASSIGN(audit, gnl, audit_number);
      J_BOO_ASSIGN(audit, gnl, do_billing_adjust);
      J_FLT_ASSIGN(audit, gnl, no_cond_stories);
      J_FLT_ASSIGN(audit, gnl, floor_area);
      J_FLT_ASSIGN(audit, gnl, avg_no_occupants);
    J_SEC_END();

    J_SEC_BEG(weather_location);
      J_STR_ASSIGN(weather_location, wth, state);
      J_STR_ASSIGN(weather_location, wth, city);
      J_STR_ASSIGN(weather_location, wth, file);
    J_SEC_END();

    JI_ARR_BEG(walls, top->num_wal);
      JI_STR_ASSIGN(walls, wal, code);
      JI_ENU_ASSIGN(walls, wal, stud_size);
      JI_ENU_ASSIGN(walls, wal, orient);
      JI_ENU_ASSIGN(walls, wal, exposure);
      JI_ENU_ASSIGN(walls, wal, ext_type);
      JI_ENU_ASSIGN(walls, wal, wall_type);
      JI_FLT_ASSIGN(walls, wal, area);
      JI_ENU_ASSIGN(walls, wal, exist_insulation);
      JI_FLT_ASSIGN(walls, wal, exist_r);
      JI_ENU_ASSIGN(walls, wal, added_insulation);
      JI_FLT_ASSIGN(walls, wal, add_cost);
      JI_INT_ASSIGN(walls, wal, measure_number);
    JI_ARR_END();

    JI_ARR_BEG(windows, top->num_win);
      JI_STR_ASSIGN(windows, win,  code);
      JI_STR_ASSIGN(windows, win,  wall);
      JI_ENU_ASSIGN(windows, win,  frame_type);
      JI_ENU_ASSIGN(windows, win,  window_type);
      JI_ENU_ASSIGN(windows, win,  glazing_type);
      JI_ENU_ASSIGN(windows, win,  int_shading);
      JI_ENU_ASSIGN(windows, win,  ext_shading);
      JI_FLT_ASSIGN(windows, win,  shade);
      JI_ENU_ASSIGN(windows, win,  leak);
      JI_FLT_ASSIGN(windows, win,  width);
      JI_FLT_ASSIGN(windows, win,  height);
      JI_FLT_ASSIGN(windows, win,  number);
      JI_ENU_ASSIGN(windows, win,  retrofit_option);
      JI_BOO_ASSIGN(windows, win,  inc_sir);
      JI_FLT_ASSIGN(windows, win,  cost_seal);
      JI_FLT_ASSIGN(windows, win,  cost_replace);
      JI_FLT_ASSIGN(windows, win,  cost_add_storm);
      JI_FLT_ASSIGN(windows, win,  cost_low_e);
    JI_ARR_END();

    JI_ARR_BEG(doors, top->num_dor);
      JI_STR_ASSIGN(doors, dor, code);
      JI_STR_ASSIGN(doors, dor, wall);
      JI_FLT_ASSIGN(doors, dor, number);
      JI_ENU_ASSIGN(doors, dor, door_type);
      JI_FLT_ASSIGN(doors, dor, area);
      JI_ENU_ASSIGN(doors, dor, condition);
      JI_ENU_ASSIGN(doors, dor, leakiness);
      JI_BOO_ASSIGN(doors, dor, replace);
      JI_BOO_ASSIGN(doors, dor, inc_sir);
      JI_FLT_ASSIGN(doors, dor, cost);
    JI_ARR_END();

    JI_ARR_BEG(unfinished_attics, top->num_atc);
      JI_STR_ASSIGN(unfinished_attics, atc, code);
      JI_ENU_ASSIGN(unfinished_attics, atc, attic_type);
      JI_FLT_ASSIGN(unfinished_attics, atc, joist_sp);
      JI_FLT_ASSIGN(unfinished_attics, atc, area);
      JI_ENU_ASSIGN(unfinished_attics, atc, exist_insulation);
      JI_FLT_ASSIGN(unfinished_attics, atc, ins_depth);
      JI_ENU_ASSIGN(unfinished_attics, atc, added_insulation);
      JI_FLT_ASSIGN(unfinished_attics, atc, max_depth);
      JI_FLT_ASSIGN(unfinished_attics, atc, added_R);
      JI_FLT_ASSIGN(unfinished_attics, atc, cost);
      JI_INT_ASSIGN(unfinished_attics, atc, measure_number);
      JI_ENU_ASSIGN(unfinished_attics, atc, roof_color);
    JI_ARR_END();

    JI_ARR_BEG(finished_attics, top->num_fat);
      JI_STR_ASSIGN(finished_attics, fat, code);
      JI_ENU_ASSIGN(finished_attics, fat, acode);
      JI_ENU_ASSIGN(finished_attics, fat, floor_type);
      JI_FLT_ASSIGN(finished_attics, fat, area);
      JI_ENU_ASSIGN(finished_attics, fat, exist_insulation);
      JI_FLT_ASSIGN(finished_attics, fat, ins_depth);
      JI_ENU_ASSIGN(finished_attics, fat, added_insulation);
      JI_ENU_ASSIGN(finished_attics, fat, added_kneewall);
      JI_FLT_ASSIGN(finished_attics, fat, max_depth);
      JI_FLT_ASSIGN(finished_attics, fat, added_R);
      JI_FLT_ASSIGN(finished_attics, fat, cost);
      JI_INT_ASSIGN(finished_attics, fat, measure_number);
      JI_ENU_ASSIGN(finished_attics, fat, roof_color);
    JI_ARR_END();

    JI_ARR_BEG(foundations, top->num_fnd);
      JI_STR_ASSIGN(foundations, fnd, code);
      JI_ENU_ASSIGN(foundations, fnd, space_type);
      JI_FLT_ASSIGN(foundations, fnd, area);
      JI_FLT_ASSIGN(foundations, fnd, flr_ins_r);
      JI_FLT_ASSIGN(foundations, fnd, perim_length);
      //JI_FLT_ASSIGN(foundations, fnd, sill_perim);    #23
      JI_FLT_ASSIGN(foundations, fnd, joist_height);
      JI_FLT_ASSIGN(foundations, fnd, sill_perimeter);
      JI_FLT_ASSIGN(foundations, fnd, sill_r);
      JI_FLT_ASSIGN(foundations, fnd, wall_height);
      JI_FLT_ASSIGN(foundations, fnd, wall_exp);
      JI_FLT_ASSIGN(foundations, fnd, wall_ins_r);
      JI_ENU_ASSIGN(foundations, fnd, added_floor);
      JI_ENU_ASSIGN(foundations, fnd, added_sill);
      JI_ENU_ASSIGN(foundations, fnd, added_found);
      JI_FLT_ASSIGN(foundations, fnd, floor_cost);
      JI_FLT_ASSIGN(foundations, fnd, sill_cost);
      JI_FLT_ASSIGN(foundations, fnd, wall_cost);
      JI_INT_ASSIGN(foundations, fnd, measure_number);
    JI_ARR_END();

    JI_ARR_BEG(heating, top->num_htg);
      JI_STR_ASSIGN(heating, htg, code);
      JI_ENU_ASSIGN(heating, htg, system_type);
      JI_ENU_ASSIGN(heating, htg, fuel_type);
      JI_ENU_ASSIGN(heating, htg, location);
      JI_FLT_ASSIGN(heating, htg, percent_heat_supplied);
      JI_BOO_ASSIGN(heating, htg, primary);
      JI_BOO_ASSIGN(heating, htg, replace_system);

      JI_ENU_ASSIGN(heating, htg, duct_location);
      JI_ENU_ASSIGN(heating, htg, duct_type1);
      JI_FLT_ASSIGN(heating, htg, duct_length1);
      JI_FLT_ASSIGN(heating, htg, duct_width1);
      JI_FLT_ASSIGN(heating, htg, duct_height1);
      JI_FLT_ASSIGN(heating, htg, duct_diameter1);
      JI_ENU_ASSIGN(heating, htg, duct_type2);
      JI_FLT_ASSIGN(heating, htg, duct_length2);
      JI_FLT_ASSIGN(heating, htg, duct_width2);
      JI_FLT_ASSIGN(heating, htg, duct_height2);
      JI_FLT_ASSIGN(heating, htg, duct_diameter2);
      JI_ENU_ASSIGN(heating, htg, duct_type3);
      JI_FLT_ASSIGN(heating, htg, duct_length3);
      JI_FLT_ASSIGN(heating, htg, duct_width3);
      JI_FLT_ASSIGN(heating, htg, duct_height3);
      JI_FLT_ASSIGN(heating, htg, duct_diameter3);

      JI_FLT_ASSIGN(heating, htg, hspf);
      JI_FLT_ASSIGN(heating, htg, year_manufactured);

      JI_ENU_ASSIGN(heating, htg, input_units);
      JI_FLT_ASSIGN(heating, htg, output_capacity);
      JI_FLT_ASSIGN(heating, htg, steady_state_eff);

      JI_ENU_ASSIGN(heating, htg, condition);
      JI_BOO_ASSIGN(heating, htg, smart_thermostat);
      JI_BOO_ASSIGN(heating, htg, vent_damper_present);
      JI_BOO_ASSIGN(heating, htg, vent_damper_recommended);
      JI_BOO_ASSIGN(heating, htg, pilot_light_present);
      JI_BOO_ASSIGN(heating, htg, pilot_light_on_summer);
      JI_BOO_ASSIGN(heating, htg, intermittent_ignition);
      JI_BOO_ASSIGN(heating, htg, retention_head_burner);
      JI_BOO_ASSIGN(heating, htg, retention_head_recommended);
      JI_BOO_ASSIGN(heating, htg, power_burner);

      JI_ENU_ASSIGN(heating, htg, retrofit_option);
      JI_BOO_ASSIGN(heating, htg, inc_sir);
      JI_FLT_ASSIGN(heating, htg, retrofit_hspf);  // added MJF 7/2019
      JI_FLT_ASSIGN(heating, htg, retrofit_afue_standard_eff);
      JI_FLT_ASSIGN(heating, htg, retrofit_afue_high_eff);
      JI_ENU_ASSIGN(heating, htg, retrofit_fuel_type);
      JI_FLT_ASSIGN(heating, htg, labor_cost_standard_eff);
      JI_FLT_ASSIGN(heating, htg, material_cost_standard_eff);
      JI_FLT_ASSIGN(heating, htg, labor_cost_high_eff);
      JI_FLT_ASSIGN(heating, htg, material_cost_high_eff);
    JI_ARR_END();

    JI_ARR_BEG(cooling, top->num_clg);
      JI_STR_ASSIGN(cooling, clg, code);
      JI_ENU_ASSIGN(cooling, clg, system_type);
      JI_FLT_ASSIGN(cooling, clg, size);
      JI_FLT_ASSIGN(cooling, clg, area_cooled);
      JI_FLT_ASSIGN(cooling, clg, seer);
      JI_FLT_ASSIGN(cooling, clg, year_manufactured);
      JI_BOO_ASSIGN(cooling, clg, replace);
      JI_BOO_ASSIGN(cooling, clg, tune_up);
      JI_BOO_ASSIGN(cooling, clg, inc_sir);
    JI_ARR_END();

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
      J_FLT_ASSIGN(ducts_and_infiltration, inf, pre_duct_seal_return_pa);
      J_FLT_ASSIGN(ducts_and_infiltration, inf, post_duct_seal_supply_pa);
      J_FLT_ASSIGN(ducts_and_infiltration, inf, post_duct_seal_return_pa);
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
      J_FLT_ASSIGN(water_heating, dwh, replace_input);
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
      top->rmc[id].material_id = id;
      // BOO_ASSIGN(rmc, &top->rmc[id].idefined,  idefined);
      // INT_ASSIGN(rmc, &top->rmc[id].itype, itype);
      // INT_ASSIGN(rmc, &top->rmc[id].inum, inum);
      STR_ASSIGN(measure_costs, top->rmc[id].material, material);
      STR_ASSIGN(measure_costs, top->rmc[id].retrofit_type, retrofit_type);
      FLT_ASSIGN(measure_costs, &top->rmc[id].life, life);
      STR_ASSIGN(measure_costs, top->rmc[id].units, units);
      FLT_ASSIGN(measure_costs, &top->rmc[id].matcost, material_cost);
      FLT_ASSIGN(measure_costs, &top->rmc[id].labcost, labor_cost);
      FLT_ASSIGN(measure_costs, &top->rmc[id].itemcost, other_cost);
    JI_ARR_END();
    }

    {
    int usage_id = 0;
    int index = 0;
    char usage[100];
    JI_ARR_BEG(neat_insulation_types, top->num_ins); {
      STR_ASSIGN(neat_insulation_types, usage, usage);
      INT_ASSIGN(neat_insulation_types, &usage_id, usage_id); // NOTE Order dependent JSON, usage_id and index must come first
      INT_ASSIGN(neat_insulation_types, &index, index);       // index 1 is the NONE option, that is why we have [index + 1] below
      switch (usage_id) {
      case 1: {
        STR_ASSIGN(neat_insulation_types, top->ins_attic[index + 1].insul_name, name);
        STR_ASSIGN(neat_insulation_types, top->ins_attic[index + 1].units, units);
        FLT_ASSIGN(neat_insulation_types, &top->ins_attic[index + 1].value, value); // r/in
        break;
      }
      case 2: {
        STR_ASSIGN(neat_insulation_types, top->ins_kneewall[index + 1].insul_name, name);
        STR_ASSIGN(neat_insulation_types, top->ins_kneewall[index + 1].units, units);
        FLT_ASSIGN(neat_insulation_types, &top->ins_kneewall[index + 1].value, value); // r value
        break;
      }
      case 3: {
        STR_ASSIGN(neat_insulation_types, top->ins_wall[index + 1].insul_name, name);
        STR_ASSIGN(neat_insulation_types, top->ins_wall[index + 1].units, units);
        FLT_ASSIGN(neat_insulation_types, &top->ins_wall[index + 1].value, value); // user r or r/in
        break;
      }
      case 4: {
        STR_ASSIGN(neat_insulation_types, top->ins_floor[index + 1].insul_name, name);
        STR_ASSIGN(neat_insulation_types, top->ins_floor[index + 1].units, units);
        FLT_ASSIGN(neat_insulation_types, &top->ins_floor[index + 1].value, value); // r/in
        break;
      }
      case 5: {
        STR_ASSIGN(neat_insulation_types, top->ins_sill[index + 1].insul_name, name);
        STR_ASSIGN(neat_insulation_types, top->ins_sill[index + 1].units, units);
        FLT_ASSIGN(neat_insulation_types, &top->ins_sill[index + 1].value, value); // r
        break;
      }
      case 6: {
        STR_ASSIGN(neat_insulation_types, top->ins_foundation[index + 1].insul_name, name);
        STR_ASSIGN(neat_insulation_types, top->ins_foundation[index + 1].units, units);
        FLT_ASSIGN(neat_insulation_types, &top->ins_foundation[index + 1].value, value); // r
        break;
      }
      }
    }
    JI_ARR_END();
    }

    J_SEC_BEG(key_parameters);
      J_FLT_ASSIGN(key_parameters, key, real_discount_rate);
      J_FLT_ASSIGN(key_parameters, key, minimum_acceptable_sir);
      J_FLT_ASSIGN(key_parameters, key, daytime_heating_setpoint);
      J_FLT_ASSIGN(key_parameters, key, nighttime_heating_setpoint);
      J_FLT_ASSIGN(key_parameters, key, daytime_cooling_setpoint);
      J_FLT_ASSIGN(key_parameters, key, nighttime_cooling_setpoint);
      J_FLT_ASSIGN(key_parameters, key, nighttime_heating_setback);
      J_FLT_ASSIGN(key_parameters, key, annual_outside_film_coeff);
      J_FLT_ASSIGN(key_parameters, key, base_free_heat_from_internals);
      J_FLT_ASSIGN(key_parameters, key, r_value_uninsulated_other_wall);
      J_FLT_ASSIGN(key_parameters, key, r_value_exterior_siding_other);
      J_FLT_ASSIGN(key_parameters, key, new_window_ac_seer);
      J_FLT_ASSIGN(key_parameters, key, new_central_ac_seer);
      J_FLT_ASSIGN(key_parameters, key, new_heatpump_cooling_seer);
      //J_FLT_ASSIGN(key_parameters, key, impute_cooling_seer);
      J_FLT_ASSIGN(key_parameters, key, new_showerhead_gpm);
      J_FLT_ASSIGN(key_parameters, key, new_dwh_blanket_r_value);
      J_FLT_ASSIGN(key_parameters, key, single_defrost_kwh);
      J_FLT_ASSIGN(key_parameters, key, new_duct_insulation_r_value);
      J_FLT_ASSIGN(key_parameters, key, new_standard_window_u_value);
      J_FLT_ASSIGN(key_parameters, key, new_standard_window_shgc);
      J_FLT_ASSIGN(key_parameters, key, new_lowe_window_u_value);
      J_FLT_ASSIGN(key_parameters, key, new_lowe_window_shgc);
      J_FLT_ASSIGN(key_parameters, key, storm_window_inside_emittance);
      J_FLT_ASSIGN(key_parameters, key, storm_window_shgc);
      J_FLT_ASSIGN(key_parameters, key, window_film_shgc);
      J_FLT_ASSIGN(key_parameters, key, window_film_emittance);
    J_SEC_END();

  }  // next section in the json

  // read in any referenced fuel escalation rates

  if (top->num_fer == 0) {
    ASSERT(top->wth.state[0] || top->rer.region || top->rer.state[0],
           sprintf(msg, "You must enter either the region or state for the referenced fuel cost escalation rates"));
    ASSERT(top->rer.year, sprintf(msg, "You must enter the year for the referenced fuel cost escalation rates"));
    get_referenced_escalation_rates(top, jschema);
  }

  //Handy bit of code here to produce the measure and material ID definitions in def_n.h
  // for (int i = 0; i < top->num_cms; i++) {
  //   fprintf(stderr, "\n#define N_CMS_%s %d", replace_char(strupr(top->cms[i].measure_name), ' ', '_'), top->cms[i].id);
  // }
  // for (int i = 0; i < top->num_rmc; i++) {
  //   fprintf(stderr, "\n#define N_MAT_%s %d", replace_char(strupr(top->rmc[i].material), ' ', '_'), top->rmc[i].material_id);
  // }

  // At this point, we should have all data in NDI and can do sematic biz logic checks

  ndi_check(top);

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

// Echos the NDI input structure back to JSON
void neat_json_echo_write(NDI *top) {
  cJSON *jroot = NULL; // the JSON tree echo of our NDI structure
  cJSON *jarray = NULL;
  cJSON *jarray2 = NULL;
  cJSON *jitem = NULL;
  int i, j;

  ASSERT(top, sprintf(msg, "Must have the NEAT input structure filled out"));

  // clang-format off

  jroot = cJSON_CreateObject();
  
  cJSON_AddItemToObject(jroot, "audit", jitem = cJSON_CreateObject());
  cJSON_AddStringToObject(jitem, "audit_type",              top->gnl.audit_type);
  cJSON_AddNumberToObject(jitem, "audit_id",                top->gnl.audit_id);
  cJSON_AddNumberToObject(jitem, "audit_number",            top->gnl.audit_number);
  WA_AddBoolToObject(     jitem, "do_billing_adjust",       top->gnl.do_billing_adjust);
  cJSON_AddNumberToObject(jitem, "no_cond_stories",         WA_DBL_FMT(top->gnl.no_cond_stories, 2));
  cJSON_AddNumberToObject(jitem, "floor_area",              WA_DBL_FMT(top->gnl.floor_area, 2));
  cJSON_AddNumberToObject(jitem, "avg_no_occupants",        WA_DBL_FMT(top->gnl.avg_no_occupants, 2));

  cJSON_AddItemToObject(jroot, "walls", jarray = cJSON_CreateArray());
  for (i = 0; i < top->num_wal; i++) {
    cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
    cJSON_AddStringToObject(jitem, "code",              top->wal[i].code);
    WA_AddNumToObjectNoZero(jitem, "stud_size",         top->wal[i].stud_size);
    cJSON_AddNumberToObject(jitem, "orient",            top->wal[i].orient);
    cJSON_AddNumberToObject(jitem, "exposure",          top->wal[i].exposure);
    cJSON_AddNumberToObject(jitem, "ext_type",          top->wal[i].ext_type);
    cJSON_AddNumberToObject(jitem, "wall_type",         top->wal[i].wall_type);
    cJSON_AddNumberToObject(jitem, "area",              WA_DBL_FMT(top->wal[i].area,2));
    cJSON_AddNumberToObject(jitem, "exist_insulation",  top->wal[i].exist_insulation);
    WA_AddNumToObjectNoZero(jitem, "exist_r",           WA_DBL_FMT(top->wal[i].exist_r, 2));
    cJSON_AddNumberToObject(jitem, "added_insulation",  top->wal[i].added_insulation);
    cJSON_AddNumberToObject(jitem, "add_cost",          WA_DBL_FMT(top->wal[i].add_cost, 2));
    cJSON_AddNumberToObject(jitem, "measure_number",    top->wal[i].measure_number);
  }

  if (top->num_win) {
    cJSON_AddItemToObject(jroot, "windows", jarray = cJSON_CreateArray());
    for (i = 0; i < top->num_win; i++) {
      cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
      cJSON_AddStringToObject(jitem, "code",              top->win[i].code);
      cJSON_AddStringToObject(jitem, "wall",              top->win[i].wall);
      cJSON_AddNumberToObject(jitem, "frame_type",        top->win[i].frame_type);
      WA_AddNumToObjectNoZero(jitem, "window_type",       top->win[i].window_type);
      cJSON_AddNumberToObject(jitem, "glazing_type",      top->win[i].glazing_type);
      WA_AddNumToObjectNoZero(jitem, "int_shading",       top->win[i].int_shading);
      //cJSON_AddNumberToObject(jitem, "ext_shading",       top->win[i].ext_shading);  MHEA, rather than NEAT, possible future item
      cJSON_AddNumberToObject(jitem, "shade",             WA_DBL_FMT(top->win[i].shade, 2));
      cJSON_AddNumberToObject(jitem, "leak",              top->win[i].leak);
      cJSON_AddNumberToObject(jitem, "width",             WA_DBL_FMT(top->win[i].width, 2));
      cJSON_AddNumberToObject(jitem, "height",            WA_DBL_FMT(top->win[i].height, 2));
      cJSON_AddNumberToObject(jitem, "number",            WA_DBL_FMT(top->win[i].number, 2));
      cJSON_AddNumberToObject(jitem, "retrofit_option",   top->win[i].retrofit_option);
      WA_AddBoolToObject(     jitem, "inc_sir",           top->win[i].inc_sir);
      cJSON_AddNumberToObject(jitem, "cost_seal",   WA_DBL_FMT(top->win[i].cost_seal, 2));
      cJSON_AddNumberToObject(jitem, "cost_replace",      WA_DBL_FMT(top->win[i].cost_replace, 2));
      cJSON_AddNumberToObject(jitem, "cost_add_storm",    WA_DBL_FMT(top->win[i].cost_add_storm, 2));
      cJSON_AddNumberToObject(jitem, "cost_low_e",        WA_DBL_FMT(top->win[i].cost_low_e, 2)); 
    }
  }

  if (top->num_dor) {
    cJSON_AddItemToObject(jroot,   "doors",     jarray = cJSON_CreateArray());
    for (i = 0; i < top->num_dor; i++) {
      cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
      cJSON_AddStringToObject(jitem, "code",              top->dor[i].code);
      cJSON_AddStringToObject(jitem, "wall",              top->dor[i].wall);
      cJSON_AddNumberToObject(jitem, "number",            WA_DBL_FMT(top->dor[i].number, 2));
      cJSON_AddNumberToObject(jitem, "door_type",         top->dor[i].door_type);
      cJSON_AddNumberToObject(jitem, "area",              WA_DBL_FMT(top->dor[i].area, 2));
      cJSON_AddNumberToObject(jitem, "condition",         top->dor[i].condition);
      cJSON_AddNumberToObject(jitem, "leakiness",         top->dor[i].leakiness);
      WA_AddBoolToObject(     jitem, "replace",           top->dor[i].replace);
      WA_AddBoolToObject(     jitem, "inc_sir",           top->dor[i].inc_sir);
      cJSON_AddNumberToObject(jitem, "cost",              WA_DBL_FMT(top->dor[i].cost, 2));
    }
  }

  if (top->num_atc) {
    cJSON_AddItemToObject(jroot,   "unfinished_attics",     jarray = cJSON_CreateArray());
    for (i = 0; i < top->num_atc; i++) {
      cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
      cJSON_AddStringToObject(jitem, "code",              top->atc[i].code);
      cJSON_AddNumberToObject(jitem, "attic_type",        top->atc[i].attic_type);
      WA_AddNumToObjectNoZero(jitem, "joist_sp",          WA_DBL_FMT(top->atc[i].joist_sp, 2));
      cJSON_AddNumberToObject(jitem, "area",              WA_DBL_FMT(top->atc[i].area, 2));
      cJSON_AddNumberToObject(jitem, "exist_insulation",  top->atc[i].exist_insulation);
      WA_AddNumToObjectNoZero(jitem, "ins_depth",         WA_DBL_FMT(top->atc[i].ins_depth, 2));
      cJSON_AddNumberToObject(jitem, "added_insulation",  top->atc[i].added_insulation);
      WA_AddNumToObjectNoZero(jitem, "max_depth",         WA_DBL_FMT(top->atc[i].max_depth, 2));
      WA_AddNumToObjectNoZero(jitem, "added_R",           WA_DBL_FMT(top->atc[i].added_R, 2));
      WA_AddNumToObjectNoZero(jitem, "cost",              WA_DBL_FMT(top->atc[i].cost, 2));
      cJSON_AddNumberToObject(jitem, "measure_number",    top->atc[i].measure_number);
      cJSON_AddNumberToObject(jitem, "roof_color",        top->atc[i].roof_color);
    }
  }

  if (top->num_fat) {
    cJSON_AddItemToObject(jroot,   "finished_attics",     jarray = cJSON_CreateArray());
    for (i = 0; i < top->num_fat; i++) {
      cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
      cJSON_AddStringToObject(jitem, "code",              top->fat[i].code);
      cJSON_AddNumberToObject(jitem, "acode",             top->fat[i].acode);
      WA_AddNumToObjectNoZero(jitem, "floor_type",        top->fat[i].floor_type);
      cJSON_AddNumberToObject(jitem, "area",              WA_DBL_FMT(top->fat[i].area, 2));
      cJSON_AddNumberToObject(jitem, "exist_insulation",  top->fat[i].exist_insulation);
      WA_AddNumToObjectNoZero(jitem, "ins_depth",         WA_DBL_FMT(top->fat[i].ins_depth, 2));
      WA_AddNumToObjectNoZero(jitem, "added_insulation",  top->fat[i].added_insulation);
      WA_AddNumToObjectNoZero(jitem, "added_kneewall",    top->fat[i].added_kneewall);
      WA_AddNumToObjectNoZero(jitem, "max_depth",         WA_DBL_FMT(top->fat[i].max_depth, 2));
      WA_AddNumToObjectNoZero(jitem, "added_R",           WA_DBL_FMT(top->fat[i].added_R, 2));
      WA_AddNumToObjectNoZero(jitem, "cost",              WA_DBL_FMT(top->fat[i].cost, 2));
      WA_AddNumToObjectNoZero(jitem, "measure_number",    top->fat[i].measure_number);
      WA_AddNumToObjectNoZero(jitem, "roof_color",        top->fat[i].roof_color);
    }
  }

  if (top->num_fnd) {
    cJSON_AddItemToObject(jroot,   "foundations",     jarray = cJSON_CreateArray());
    for (i = 0; i < top->num_fnd; i++) {
      cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
      cJSON_AddStringToObject(jitem, "code",              top->fnd[i].code);
      cJSON_AddNumberToObject(jitem, "space_type",        top->fnd[i].space_type);
      cJSON_AddNumberToObject(jitem, "area",              WA_DBL_FMT(top->fnd[i].area, 2));
      WA_AddNumToObjectNoZero(jitem, "perim_length",      WA_DBL_FMT(top->fnd[i].perim_length, 2));
      cJSON_AddNumberToObject(jitem, "flr_ins_r",         WA_DBL_FMT(top->fnd[i].flr_ins_r, 2));
      //cJSON_AddNumberToObject(jitem, "sill_perim",        WA_DBL_FMT(top->fnd[i].sill_perim, 2));   #232
      WA_AddNumToObjectNoZero(jitem, "joist_height",      WA_DBL_FMT(top->fnd[i].joist_height, 2 ));
      cJSON_AddNumberToObject(jitem, "sill_perimeter",    WA_DBL_FMT(top->fnd[i].sill_perimeter, 2));
      cJSON_AddNumberToObject(jitem, "sill_r",            WA_DBL_FMT(top->fnd[i].sill_r, 2));
      WA_AddNumToObjectNoZero(jitem, "wall_height",       WA_DBL_FMT(top->fnd[i].wall_height, 2));
      WA_AddNumToObjectNoZero(jitem, "wall_exp",          WA_DBL_FMT(top->fnd[i].wall_exp, 2));
      cJSON_AddNumberToObject(jitem, "wall_ins_r",        WA_DBL_FMT(top->fnd[i].wall_ins_r, 2));
      WA_AddNumToObjectNoZero(jitem, "added_sill",        top->fnd[i].added_sill);
      WA_AddNumToObjectNoZero(jitem, "added_floor",       top->fnd[i].added_floor);
      WA_AddNumToObjectNoZero(jitem, "added_found",       top->fnd[i].added_found);
      cJSON_AddNumberToObject(jitem, "floor_cost",        WA_DBL_FMT(top->fnd[i].floor_cost, 2));
      WA_AddNumToObjectNoZero(jitem, "sill_cost",         WA_DBL_FMT(top->fnd[i].sill_cost, 2));
      WA_AddNumToObjectNoZero(jitem, "wall_cost",         WA_DBL_FMT(top->fnd[i].wall_cost, 2));
      cJSON_AddNumberToObject(jitem, "measure_number",    top->fnd[i].measure_number);
    }
  }

if (top->num_clg) {
    cJSON_AddItemToObject(jroot,   "cooling",     jarray = cJSON_CreateArray());
    for (i = 0; i < top->num_clg; i++) {
      cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
      cJSON_AddStringToObject(jitem, "code",                          top->clg[i].code);
      cJSON_AddNumberToObject(jitem, "system_type",                   top->clg[i].system_type);
      cJSON_AddNumberToObject(jitem, "area_cooled",                   WA_DBL_FMT(top->clg[i].area_cooled, 2));
      WA_AddNumToObjectNoZero(jitem, "size",                          WA_DBL_FMT(top->clg[i].size, 2));
      WA_AddNumToObjectNoZero(jitem, "seer",                          WA_DBL_FMT(top->clg[i].seer, 2));
      WA_AddNumToObjectNoZero(jitem, "year_manufactured",             WA_DBL_FMT(top->clg[i].year_manufactured, 2));
      WA_AddBoolToObject(     jitem, "replace",                       top->clg[i].replace);
      WA_AddBoolToObject(     jitem, "tune_up",                       top->clg[i].tune_up);
      WA_AddBoolToObject(     jitem, "inc_sir",                       top->clg[i].inc_sir);

      // cJSON_AddNumberToObject(jitem, "efficiency",                    top->clg[i].efficiency);
      // cJSON_AddNumberToObject(jitem, "efficiency_units",              top->clg[i].efficiency_units);
      // WA_AddBoolToObject(     jitem, "primary",                       top->clg[i].primary);
      // cJSON_AddNumberToObject(jitem, "replacement_seer",              top->clg[i].replacement_seer);
      // cJSON_AddNumberToObject(jitem, "tuneup_seer",                   top->clg[i].tuneup_seer);
      // cJSON_AddNumberToObject(jitem, "replacement_labor_cost",        top->clg[i].replacement_labor_cost);
      // cJSON_AddNumberToObject(jitem, "tuneup_labor_cost",             top->clg[i].tuneup_labor_cost);
      // cJSON_AddNumberToObject(jitem, "replacement_material_cost",     top->clg[i].replacement_material_cost);
      // cJSON_AddNumberToObject(jitem, "tuneup_material_cost",          top->clg[i].tuneup_material_cost);
      // cJSON_AddNumberToObject(jitem, "replacement_other_cost",        top->clg[i].replacement_other_cost);
      // cJSON_AddNumberToObject(jitem, "tuneup_other_cost",             top->clg[i].tuneup_other_cost);
      // cJSON_AddNumberToObject(jitem, "replacement_equipment_option",  top->clg[i].replacement_equipment_option);
    }
  }

  cJSON_AddItemToObject(jroot,   "heating",     jarray = cJSON_CreateArray());
  for (i = 0; i < top->num_htg; i++) {
    cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
    cJSON_AddStringToObject(jitem, "code",                          top->htg[i].code);
    cJSON_AddNumberToObject(jitem, "system_type",                   top->htg[i].system_type);
    cJSON_AddNumberToObject(jitem, "fuel_type",                     top->htg[i].fuel_type);
    cJSON_AddNumberToObject(jitem, "location",                      top->htg[i].location);
    WA_AddBoolToObject(     jitem, "primary",                       top->htg[i].primary);
    WA_AddBoolToObject(     jitem, "replace_system",                top->htg[i].replace_system);
    cJSON_AddNumberToObject(jitem, "percent_heat_supplied",         WA_DBL_FMT(top->htg[i].percent_heat_supplied, 2));
    WA_AddNumToObjectNoZero(jitem, "hspf",                          WA_DBL_FMT(top->htg[i].hspf, 2));
    WA_AddNumToObjectNoZero(jitem, "year_manufactured",             WA_DBL_FMT(top->htg[i].year_manufactured, 2));
    WA_AddNumToObjectNoZero(jitem, "input_units",                   top->htg[i].input_units);
    WA_AddNumToObjectNoZero(jitem, "output_capacity",               WA_DBL_FMT(top->htg[i].output_capacity, 2));
    WA_AddNumToObjectNoZero(jitem, "steady_state_eff",              WA_DBL_FMT(top->htg[i].steady_state_eff, 2));
    WA_AddNumToObjectNoZero(jitem, "condition",                     top->htg[i].condition);
    WA_AddBoolToObject(     jitem, "smart_thermostat",              top->htg[i].smart_thermostat);
    WA_AddBoolToObject(     jitem, "vent_damper_present",           top->htg[i].vent_damper_present);
    WA_AddBoolToObject(     jitem, "vent_damper_recommended",       top->htg[i].vent_damper_recommended);
    WA_AddBoolToObject(     jitem, "pilot_light_present",           top->htg[i].pilot_light_present);
    WA_AddBoolToObject(     jitem, "pilot_light_on_summer",         top->htg[i].pilot_light_on_summer);
    WA_AddBoolToObject(     jitem, "intermittent_ignition",         top->htg[i].intermittent_ignition);
    WA_AddBoolToObject(     jitem, "retention_head_burner",         top->htg[i].retention_head_burner);
    WA_AddBoolToObject(     jitem, "retention_head_recommended",    top->htg[i].retention_head_recommended);
    WA_AddBoolToObject(     jitem, "power_burner",                  top->htg[i].power_burner);
    WA_AddNumToObjectNoZero(jitem, "retrofit_option",               top->htg[i].retrofit_option);
    WA_AddBoolToObject(     jitem, "inc_sir",                       top->htg[i].inc_sir);
    WA_AddNumToObjectNoZero(jitem, "retrofit_hspf",                 WA_DBL_FMT(top->htg[i].retrofit_hspf, 2)); // added MJF 7/2019
    WA_AddNumToObjectNoZero(jitem, "retrofit_afue_standard_eff",    WA_DBL_FMT(top->htg[i].retrofit_afue_standard_eff, 2));
    WA_AddNumToObjectNoZero(jitem, "retrofit_afue_high_eff",        WA_DBL_FMT(top->htg[i].retrofit_afue_high_eff, 2));
    WA_AddNumToObjectNoZero(jitem, "retrofit_fuel_type",            top->htg[i].retrofit_fuel_type);
    cJSON_AddNumberToObject(jitem, "labor_cost_standard_eff",       WA_DBL_FMT(top->htg[i].labor_cost_standard_eff, 2));
    cJSON_AddNumberToObject(jitem, "material_cost_standard_eff",    WA_DBL_FMT(top->htg[i].material_cost_standard_eff, 2));
    cJSON_AddNumberToObject(jitem, "labor_cost_high_eff",           WA_DBL_FMT(top->htg[i].labor_cost_high_eff, 2));
    cJSON_AddNumberToObject(jitem, "material_cost_high_eff",        WA_DBL_FMT(top->htg[i].material_cost_high_eff, 2));
    cJSON_AddNumberToObject(jitem, "duct_location",                 top->htg[i].duct_location);
    cJSON_AddNumberToObject(jitem, "duct_type1",                    top->htg[i].duct_type1);
    WA_AddNumToObjectNoZero(jitem, "duct_length1",                  WA_DBL_FMT(top->htg[i].duct_length1, 2));
    WA_AddNumToObjectNoZero(jitem, "duct_width1",                   WA_DBL_FMT(top->htg[i].duct_width1, 2));
    WA_AddNumToObjectNoZero(jitem, "duct_height1",                  WA_DBL_FMT(top->htg[i].duct_height1, 2));
    WA_AddNumToObjectNoZero(jitem, "duct_diameter1",                WA_DBL_FMT(top->htg[i].duct_diameter1, 2));
    cJSON_AddNumberToObject(jitem, "duct_type2",                    top->htg[i].duct_type2);
    WA_AddNumToObjectNoZero(jitem, "duct_length2",                  WA_DBL_FMT(top->htg[i].duct_length2, 2));
    WA_AddNumToObjectNoZero(jitem, "duct_width2",                   WA_DBL_FMT(top->htg[i].duct_width2, 2));
    WA_AddNumToObjectNoZero(jitem, "duct_height2",                  WA_DBL_FMT(top->htg[i].duct_height2, 2));
    WA_AddNumToObjectNoZero(jitem, "duct_diameter2",                WA_DBL_FMT(top->htg[i].duct_diameter2, 2));
    cJSON_AddNumberToObject(jitem, "duct_type3",                    top->htg[i].duct_type3);
    WA_AddNumToObjectNoZero(jitem, "duct_length3",                  WA_DBL_FMT(top->htg[i].duct_length3, 2));
    WA_AddNumToObjectNoZero(jitem, "duct_width3",                   WA_DBL_FMT(top->htg[i].duct_width3, 2));
    WA_AddNumToObjectNoZero(jitem, "duct_height3",                  WA_DBL_FMT(top->htg[i].duct_height3, 2));
    WA_AddNumToObjectNoZero(jitem, "duct_diameter3",                WA_DBL_FMT(top->htg[i].duct_diameter3, 2));
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
  WA_AddNumToObjectNoZero(jitem, "pre_duct_seal_return_pa",               WA_DBL_FMT(top->inf.pre_duct_seal_return_pa, 2));
  WA_AddNumToObjectNoZero(jitem, "post_duct_seal_supply_pa",              WA_DBL_FMT(top->inf.post_duct_seal_supply_pa, 2));
  WA_AddNumToObjectNoZero(jitem, "post_duct_seal_return_pa",              WA_DBL_FMT(top->inf.post_duct_seal_return_pa, 2));
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
    cJSON_AddNumberToObject(jitem, "replace_type",                   top->dwh.replace_type);
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

  if (top->ref.location_id) {
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
      cJSON_AddStringToObject(jitem, "fuel_name",                top->fer[i].fuelname);
      cJSON_AddItemToObject(  jitem, "rate",                    jarray2 = cJSON_CreateArray());
      for (j = 0; j < NUM_FUEL_RATES; j++) {
        cJSON_AddItemToArray(jarray2, cJSON_CreateNumber(WA_DBL_FMT(top->fer[i].rates[j], 2)));
      }
    }
  }

  cJSON_AddItemToObject(jroot,   "measure_active_flags",     jarray = cJSON_CreateArray());
  for (i = 0; i < NEAT_MAX_CMS; i++) {
    if (strlen(top->cms[i].measure_name)) {
      cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
      cJSON_AddNumberToObject(jitem, "id",          i);
      WA_AddBoolToObject(     jitem, "active",       top->cms[i].active);
      cJSON_AddStringToObject(jitem, "measure_name", top->cms[i].measure_name);
    }
  }

  cJSON_AddItemToObject(jroot,   "measure_costs",     jarray = cJSON_CreateArray());
  for (i = 0; i < NEAT_MAX_RMC; i++) {      // traverse whole possible array including blanks
    if (strlen(top->rmc[i].material)) {     // there are some blank (never read in) entries
      cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
      cJSON_AddNumberToObject(jitem, "id",                  top->rmc[i].material_id);
      // WA_AddBoolToObject(     jitem, "idefined",            top->rmc[i].idefined);
      // cJSON_AddNumberToObject(jitem, "itype",               top->rmc[i].itype);
      // cJSON_AddNumberToObject(jitem, "inum",                top->rmc[i].inum);
      cJSON_AddStringToObject(jitem, "material",            top->rmc[i].material);
      cJSON_AddStringToObject(jitem, "retrofit_type",       top->rmc[i].retrofit_type);
      cJSON_AddNumberToObject(jitem, "life",                WA_DBL_FMT(top->rmc[i].life, 2));
      cJSON_AddStringToObject(jitem, "units",               top->rmc[i].units);
      cJSON_AddNumberToObject(jitem, "material_cost",       WA_DBL_FMT(top->rmc[i].matcost, 3));
      cJSON_AddNumberToObject(jitem, "labor_cost",          WA_DBL_FMT(top->rmc[i].labcost, 3));
      cJSON_AddNumberToObject(jitem, "other_cost",          WA_DBL_FMT(top->rmc[i].itemcost, 3));
    }
  }

  cJSON_AddItemToObject(jroot,   "neat_insulation_types",     jarray = cJSON_CreateArray());

  for (i = 2; i < NEAT_MAX_TOT_INS; i++) {
    cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
    cJSON_AddNumberToObject(jitem, "usage_id",                 1);
    cJSON_AddStringToObject(jitem, "usage",                    "Attic");
    cJSON_AddNumberToObject(jitem, "index",                    i-1);
    cJSON_AddStringToObject(jitem, "name",                     top->ins_attic[i].insul_name);
    cJSON_AddStringToObject(jitem, "units",                    top->ins_attic[i].units);
    cJSON_AddNumberToObject(jitem, "value",                    WA_DBL_FMT(top->ins_attic[i].value,2));
  }

  for (i = 2; i < NEAT_MAX_TOT_INS; i++) {
    cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
    cJSON_AddNumberToObject(jitem, "usage_id",                 2);
    cJSON_AddStringToObject(jitem, "usage",                    "Knee Wall");
    cJSON_AddNumberToObject(jitem, "index",                    i-1);
    cJSON_AddStringToObject(jitem, "name",                     top->ins_kneewall[i].insul_name);
    cJSON_AddStringToObject(jitem, "units",                    top->ins_kneewall[i].units);
    cJSON_AddNumberToObject(jitem, "value",                    WA_DBL_FMT(top->ins_kneewall[i].value,2));
  }

  for (i = 2; i < NEAT_MAX_TOT_INS; i++) {
    cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
    cJSON_AddNumberToObject(jitem, "usage_id",                 3);
    cJSON_AddStringToObject(jitem, "usage",                    "Wall");
    cJSON_AddNumberToObject(jitem, "index",                    i-1);
    cJSON_AddStringToObject(jitem, "name",                     top->ins_wall[i].insul_name);
    cJSON_AddStringToObject(jitem, "units",                    top->ins_wall[i].units);
    cJSON_AddNumberToObject(jitem, "value",                    WA_DBL_FMT(top->ins_wall[i].value,2));
  }

  for (i = 2; i < NEAT_MAX_TOT_INS; i++) {
    cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
    cJSON_AddNumberToObject(jitem, "usage_id",                 4);
    cJSON_AddStringToObject(jitem, "usage",                    "Floor");
    cJSON_AddNumberToObject(jitem, "index",                    i-1);
    cJSON_AddStringToObject(jitem, "name",                     top->ins_floor[i].insul_name);
    cJSON_AddStringToObject(jitem, "units",                    top->ins_floor[i].units);
    cJSON_AddNumberToObject(jitem, "value",                    WA_DBL_FMT(top->ins_floor[i].value,2));
  }

  for (i = 2; i < NEAT_MAX_TOT_INS; i++) {
    cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
    cJSON_AddNumberToObject(jitem, "usage_id",                 5);
    cJSON_AddStringToObject(jitem, "usage",                    "Sill");
    cJSON_AddNumberToObject(jitem, "index",                    i-1);
    cJSON_AddStringToObject(jitem, "name",                     top->ins_sill[i].insul_name);
    cJSON_AddStringToObject(jitem, "units",                    top->ins_sill[i].units);
    cJSON_AddNumberToObject(jitem, "value",                    WA_DBL_FMT(top->ins_sill[i].value, 2));
  }

  for (i = 2; i < NEAT_MAX_TOT_INS; i++) {
    cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
    cJSON_AddNumberToObject(jitem, "usage_id",                 6);
    cJSON_AddStringToObject(jitem, "usage",                    "Foundation Wall");
    cJSON_AddNumberToObject(jitem, "index",                    i-1);
    cJSON_AddStringToObject(jitem, "name",                     top->ins_foundation[i].insul_name);
    cJSON_AddStringToObject(jitem, "units",                    top->ins_foundation[i].units);
    cJSON_AddNumberToObject(jitem, "value",                    WA_DBL_FMT(top->ins_foundation[i].value, 2));
  }
  
  cJSON_AddItemToObject(jroot,   "key_parameters",     jitem = cJSON_CreateObject());
  cJSON_AddNumberToObject(jitem, "real_discount_rate",              WA_DBL_FMT(top->key.real_discount_rate, 2));
  cJSON_AddNumberToObject(jitem, "minimum_acceptable_sir",          WA_DBL_FMT(top->key.minimum_acceptable_sir, 2));
  cJSON_AddNumberToObject(jitem, "daytime_heating_setpoint",        WA_DBL_FMT(top->key.daytime_heating_setpoint,2 ));
  cJSON_AddNumberToObject(jitem, "nighttime_heating_setpoint",      WA_DBL_FMT(top->key.nighttime_heating_setpoint, 2));
  cJSON_AddNumberToObject(jitem, "daytime_cooling_setpoint",        WA_DBL_FMT(top->key.daytime_cooling_setpoint, 2));
  cJSON_AddNumberToObject(jitem, "nighttime_cooling_setpoint",      WA_DBL_FMT(top->key.nighttime_cooling_setpoint,2));
  cJSON_AddNumberToObject(jitem, "nighttime_heating_setback",       WA_DBL_FMT(top->key.nighttime_heating_setback,2));
  cJSON_AddNumberToObject(jitem, "annual_outside_film_coeff",       WA_DBL_FMT(top->key.annual_outside_film_coeff,2));
  cJSON_AddNumberToObject(jitem, "base_free_heat_from_internals",   WA_DBL_FMT(top->key.base_free_heat_from_internals,2));
  cJSON_AddNumberToObject(jitem, "r_value_uninsulated_other_wall",  WA_DBL_FMT(top->key.r_value_uninsulated_other_wall,2));
  cJSON_AddNumberToObject(jitem, "r_value_exterior_siding_other",   WA_DBL_FMT(top->key.r_value_exterior_siding_other,2));
  cJSON_AddNumberToObject(jitem, "new_window_ac_seer",              WA_DBL_FMT(top->key.new_window_ac_seer,2));
  cJSON_AddNumberToObject(jitem, "new_central_ac_seer",             WA_DBL_FMT(top->key.new_central_ac_seer,2));
  cJSON_AddNumberToObject(jitem, "new_heatpump_cooling_seer",       WA_DBL_FMT(top->key.new_heatpump_cooling_seer,2));
  //cJSON_AddNumberToObject(jitem, "impute_cooling_seer",             WA_DBL_FMT(top->key.impute_cooling_seer,2));
  cJSON_AddNumberToObject(jitem, "new_showerhead_gpm",              WA_DBL_FMT(top->key.new_showerhead_gpm, 2));
  cJSON_AddNumberToObject(jitem, "new_dwh_blanket_r_value",         WA_DBL_FMT(top->key.new_dwh_blanket_r_value,2));
  cJSON_AddNumberToObject(jitem, "single_defrost_kwh",              WA_DBL_FMT(top->key.single_defrost_kwh,2));
  cJSON_AddNumberToObject(jitem, "new_duct_insulation_r_value",     WA_DBL_FMT(top->key.new_duct_insulation_r_value,2));
  cJSON_AddNumberToObject(jitem, "new_standard_window_u_value",     WA_DBL_FMT(top->key.new_standard_window_u_value,2));
  cJSON_AddNumberToObject(jitem, "new_standard_window_shgc",        WA_DBL_FMT(top->key.new_standard_window_shgc,2));
  cJSON_AddNumberToObject(jitem, "new_lowe_window_u_value",         WA_DBL_FMT(top->key.new_lowe_window_u_value,2));
  cJSON_AddNumberToObject(jitem, "new_lowe_window_shgc",            WA_DBL_FMT(top->key.new_lowe_window_shgc,2));
  cJSON_AddNumberToObject(jitem, "storm_window_inside_emittance",   WA_DBL_FMT(top->key.storm_window_inside_emittance,2));
  cJSON_AddNumberToObject(jitem, "storm_window_shgc",               WA_DBL_FMT(top->key.storm_window_shgc,2));
  cJSON_AddNumberToObject(jitem, "window_film_shgc",                WA_DBL_FMT(top->key.window_film_shgc,2));
  cJSON_AddNumberToObject(jitem, "window_film_emittance",           WA_DBL_FMT(top->key.window_film_emittance,2));
  // cJSON_AddNumberToObject(jitem, "window_film_delta_r",             top->key.window_film_delta_r);  // computed field

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

// Takes both the NDI and the NOR and create a JSON output file.
// The name of the results output file is taken from the wa_cmds command line
// parameters.
void neat_json_result_write(NDI *top, NOR *res) {
  cJSON *jroot = NULL; // the JSON tree echo of our NOR structure
  cJSON *jarray = NULL;
  cJSON *jitem = NULL;
  int dd_act_present;       // boolean indicating if there are any non zero entries for degree day actual figures #231
  int i;

  ASSERT(top, sprintf(msg, "Must have the NEAT input structure filled out"));
  ASSERT(res, sprintf(msg, "Must have the NEAT results structure filled out"));

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

  cJSON_AddNumberToObject(jroot,    "no_cond_stories",    top->gnl.no_cond_stories);
  cJSON_AddNumberToObject(jroot,    "floor_area",         top->gnl.floor_area);

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
  cJSON_AddNumberToObject(jroot,    "energy_delta_counter",  res->energy_delta_counter);

  // back to regularly scheduled res structure
  cJSON_AddNumberToObject(jroot, "num_measure",  res->num_measure);
  cJSON_AddItemToObject(jroot,   "measures",     jarray = cJSON_CreateArray());
  for (i = 0; i < res->num_measure; i++) {
    cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
    cJSON_AddNumberToObject(jitem, "index",           res->measure[i].index);
    cJSON_AddNumberToObject(jitem, "measure_id",      res->measure[i].measure_id);
    cJSON_AddNumberToObject(jitem, "component_id",    res->measure[i].component_id);
    cJSON_AddNumberToObject(jitem, "audit_section_id",res->measure[i].audit_section_id);
    cJSON_AddStringToObject(jitem, "measure",         res->measure[i].measure);
    WA_AddStrToObjectNoNull(jitem, "comp_group",      res->measure[i].comp_group);
    cJSON_AddStringToObject(jitem, "components",      res->measure[i].components);
    cJSON_AddNumberToObject(jitem, "heating_mmbtu",    WA_DBL_FMT(res->measure[i].heating_mmbtu, 3));
    cJSON_AddNumberToObject(jitem, "heating_sav",     WA_DBL_FMT(res->measure[i].heating_sav, 2));
    cJSON_AddNumberToObject(jitem, "cooling_kwh",     WA_DBL_FMT(res->measure[i].cooling_kwh, 1));
    cJSON_AddNumberToObject(jitem, "cooling_sav",     WA_DBL_FMT(res->measure[i].cooling_sav, 2));
    cJSON_AddNumberToObject(jitem, "baseload_kwh",    WA_DBL_FMT(res->measure[i].baseload_kwh, 1));
    cJSON_AddNumberToObject(jitem, "baseload_sav",    WA_DBL_FMT(res->measure[i].baseload_sav, 2));
    cJSON_AddNumberToObject(jitem, "total_mmbtu",      WA_DBL_FMT(res->measure[i].total_mmbtu, 3));
    cJSON_AddNumberToObject(jitem, "savings",         WA_DBL_FMT(res->measure[i].savings, 2));
    cJSON_AddNumberToObject(jitem, "cost",            WA_DBL_FMT(res->measure[i].cost, 2));
    cJSON_AddNumberToObject(jitem, "sir",             WA_DBL_FMT(res->measure[i].sir, 3));
    WA_AddNumToObjectNoZero(jitem, "lifetime",        res->measure[i].lifetime);
    cJSON_AddNumberToObject(jitem, "qtym",            WA_DBL_FMT(res->measure[i].qtym, 3));
    cJSON_AddNumberToObject(jitem, "qtyl",            WA_DBL_FMT(res->measure[i].qtyl, 3));
    cJSON_AddNumberToObject(jitem, "qtyi",            WA_DBL_FMT(res->measure[i].qtyi, 3));
    cJSON_AddNumberToObject(jitem, "costum",          WA_DBL_FMT(res->measure[i].costum, 2));
    cJSON_AddNumberToObject(jitem, "costul",          WA_DBL_FMT(res->measure[i].costul, 3));
    cJSON_AddNumberToObject(jitem, "costi1",          WA_DBL_FMT(res->measure[i].costi1, 2));
    cJSON_AddNumberToObject(jitem, "costi2",          WA_DBL_FMT(res->measure[i].costi2, 2));
    cJSON_AddStringToObject(jitem, "desci2",          res->measure[i].desci2);
    cJSON_AddNumberToObject(jitem, "typei2",          res->measure[i].typei2);
    cJSON_AddNumberToObject(jitem, "costi3",          WA_DBL_FMT(res->measure[i].costi3, 2));
    cJSON_AddStringToObject(jitem, "desci3",          res->measure[i].desci3);
    cJSON_AddNumberToObject(jitem, "typei3",          res->measure[i].typei3);
  }

  cJSON_AddNumberToObject(jroot, "num_measure_material", res->num_measure_material);
  cJSON_AddItemToObject(jroot,   "mmaterial", jarray = cJSON_CreateArray());
  for (i = 0; i < res->num_measure_material; i++) {
    cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
    cJSON_AddNumberToObject(jitem, "index",             res->mmaterial[i].index);
    cJSON_AddNumberToObject(jitem, "measure_index",     res->mmaterial[i].measure_index);
    cJSON_AddNumberToObject(jitem, "material_id",       res->mmaterial[i].material_id);
    WA_AddStrToObjectNoNull(jitem, "comp_group",        res->mmaterial[i].comp_group);
    cJSON_AddStringToObject(jitem, "components",        res->mmaterial[i].components);
    cJSON_AddStringToObject(jitem, "description",       res->mmaterial[i].description);
    cJSON_AddNumberToObject(jitem, "material_type_id",  res->mmaterial[i].material_type_id);
    cJSON_AddStringToObject(jitem, "units",             res->mmaterial[i].units);
    cJSON_AddNumberToObject(jitem, "qty_est",           WA_DBL_FMT(res->mmaterial[i].qty_est, 3));
    cJSON_AddNumberToObject(jitem, "unit_cost_est",     WA_DBL_FMT(res->mmaterial[i].unit_cost_est, 3));
    cJSON_AddStringToObject(jitem, "comment",           res->mmaterial[i].comment);
  }

  cJSON_AddNumberToObject(jroot, "num_an_sav", res->num_an_sav);
  cJSON_AddItemToObject(jroot,   "an_sav", jarray = cJSON_CreateArray());
  for (i = 0; i < res->num_an_sav; i++) {
    cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
    cJSON_AddNumberToObject(jitem, "index",         res->an_sav[i].index);
    cJSON_AddNumberToObject(jitem, "measure_index", res->an_sav[i].measure_index);
    cJSON_AddStringToObject(jitem, "measure",       res->an_sav[i].measure);
    WA_AddStrToObjectNoNull(jitem, "comp_group",    res->an_sav[i].comp_group);
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
    WA_AddStrToObjectNoNull(jitem, "comp_group",    res->an_asav[i].comp_group);
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
    cJSON_AddNumberToObject(jitem, "group",         res->sir[i].group);
    cJSON_AddStringToObject(jitem, "measure",       res->sir[i].measure);
    WA_AddStrToObjectNoNull(jitem, "comp_group",    res->sir[i].comp_group);
    cJSON_AddStringToObject(jitem, "components",    res->sir[i].components);
    cJSON_AddNumberToObject(jitem, "savings",       WA_DBL_FMT(res->sir[i].savings, 2));
    cJSON_AddNumberToObject(jitem, "cost",          WA_DBL_FMT(res->sir[i].cost, 2));
    cJSON_AddNumberToObject(jitem, "sir",           WA_DBL_FMT(res->sir[i].sir, 3));
    cJSON_AddNumberToObject(jitem, "ccost",         WA_DBL_FMT(res->sir[i].ccost, 2));
    cJSON_AddNumberToObject(jitem, "csir",          WA_DBL_FMT(res->sir[i].csir, 3));
  }

  cJSON_AddNumberToObject(jroot, "num_asir", res->num_asir);
  cJSON_AddItemToObject(jroot, "asir", jarray = cJSON_CreateArray());
  for (i = 0; i < res->num_asir; i++) {
    cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
    cJSON_AddNumberToObject(jitem, "index",         res->asir[i].index);
    cJSON_AddNumberToObject(jitem, "measure_index", res->asir[i].measure_index);
    cJSON_AddNumberToObject(jitem, "group",         res->asir[i].group);
    cJSON_AddStringToObject(jitem, "measure",       res->asir[i].measure);
    WA_AddStrToObjectNoNull(jitem, "comp_group",    res->asir[i].comp_group);
    cJSON_AddStringToObject(jitem, "components",    res->asir[i].components);
    cJSON_AddNumberToObject(jitem, "savings",       WA_DBL_FMT(res->asir[i].savings, 2));
    cJSON_AddNumberToObject(jitem, "cost",          WA_DBL_FMT(res->asir[i].cost, 2));
    cJSON_AddNumberToObject(jitem, "sir",           WA_DBL_FMT(res->asir[i].sir, 3));
    cJSON_AddNumberToObject(jitem, "ccost",         WA_DBL_FMT(res->asir[i].ccost, 2));
    cJSON_AddNumberToObject(jitem, "csir",          WA_DBL_FMT(res->asir[i].csir, 3));
  }

  cJSON_AddNumberToObject(jroot, "num_material", res->num_material);
  cJSON_AddItemToObject(jroot, "material", jarray = cJSON_CreateArray());
  for (i = 0; i < res->num_material; i++) {
    cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
    cJSON_AddNumberToObject(jitem, "index",         res->material[i].index);
    cJSON_AddNumberToObject(jitem, "measure_index", res->material[i].measure_index);
    cJSON_AddNumberToObject(jitem, "material_id",   res->material[i].material_id);
    cJSON_AddStringToObject(jitem, "material",      res->material[i].material);
    cJSON_AddStringToObject(jitem, "type",          res->material[i].type);
    cJSON_AddNumberToObject(jitem, "quantity",      WA_DBL_FMT(res->material[i].quantity, 3));
    cJSON_AddStringToObject(jitem, "units",         res->material[i].units);
    cJSON_AddNumberToObject(jitem, "qtymat",        WA_DBL_FMT(res->material[i].qtymat, 3));
    cJSON_AddNumberToObject(jitem, "qtyhrs",        WA_DBL_FMT(res->material[i].qtyhrs, 3));
    cJSON_AddNumberToObject(jitem, "qtyeach",       WA_DBL_FMT(res->material[i].qtyeach, 3));
  }

  cJSON_AddNumberToObject(jroot, "num_amaterial", res->num_amaterial);
  cJSON_AddItemToObject(jroot, "amaterial", jarray = cJSON_CreateArray());
  for (i = 0; i < res->num_amaterial; i++) {
    cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
    cJSON_AddNumberToObject(jitem, "index",         res->amaterial[i].index);
    cJSON_AddNumberToObject(jitem, "measure_index", res->amaterial[i].measure_index);
    cJSON_AddNumberToObject(jitem, "material_id",   res->amaterial[i].material_id);
    cJSON_AddStringToObject(jitem, "material",      res->amaterial[i].material);
    cJSON_AddStringToObject(jitem, "type",          res->amaterial[i].type);
    cJSON_AddNumberToObject(jitem, "quantity",      WA_DBL_FMT(res->amaterial[i].quantity, 3));
    cJSON_AddStringToObject(jitem, "units",         res->amaterial[i].units);
    cJSON_AddNumberToObject(jitem, "qtymat",        WA_DBL_FMT(res->amaterial[i].qtymat, 3));
    cJSON_AddNumberToObject(jitem, "qtyhrs",        WA_DBL_FMT(res->amaterial[i].qtyhrs, 3));
    cJSON_AddNumberToObject(jitem, "qtyeach",       WA_DBL_FMT(res->amaterial[i].qtyeach, 3));
  }

  cJSON_AddNumberToObject(jroot, "num_an_load", NEAT_MAX_ANLOAD); // always 4, not in RESULT structure, but echoed here
  cJSON_AddItemToObject(jroot, "an_load", jarray = cJSON_CreateArray());
  for (i = 0; i < NEAT_MAX_ANLOAD; i++) {
    cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
    cJSON_AddNumberToObject(jitem, "index",     res->an_load[i].index);
    cJSON_AddStringToObject(jitem, "name",      res->an_load[i].name);
    cJSON_AddNumberToObject(jitem, "pre_heat",  WA_DBL_FMT(res->an_load[i].pre_heat, 3));
    cJSON_AddNumberToObject(jitem, "pre_cool",  WA_DBL_FMT(res->an_load[i].pre_cool, 3));
    cJSON_AddNumberToObject(jitem, "post_heat", WA_DBL_FMT(res->an_load[i].post_heat, 3));
    cJSON_AddNumberToObject(jitem, "post_cool", WA_DBL_FMT(res->an_load[i].post_cool, 3));
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

