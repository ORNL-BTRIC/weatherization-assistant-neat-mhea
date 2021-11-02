/***************************************************************************
* MODULE:       ecms.c            CREATED:      February 24, 1999
*
* AUTHOR:       Mike Gettings       Oak Ridge National Laboratory
*           Mark Fishbaugher    Fishbaugher and Associates
*
* MDESC:
****************************************************************************/
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "wa_engine.h"

// our local function prototypes

static void measure_economics_using_rmc(int ecm_index, int rmc_index);
static void measure_economics_hvac(int nm, int rmc_index);
static void measure_economics_baseline(int ecm_index, int life, int fuel);
//static void measure_economics_with_fuel_switch(int ecm_index, int life, int fuel[POST_RETROFIT + 1], float consumption[POST_RETROFIT + 1]);

static void infiltration_reduction(void);
static void duct_sealing(void);
static void itemized_cost_measures(void);

static void wall_insulation(void);
static int wall_skip_insulation(int nc,  int comp_group_num);

static void attic_insulation(float radded, int cms_measure_num);
static int attic_skip_insulation(int nc, enum MEASURE_COMPONENT_GROUP_TYPE comp_group_type, int comp_group_num, int cms_measure_num, float *radded);
static int attic_insulation_material(int msn, int cn, int flg, float *pcost, float radd);

static void sill_insulation(void);
static int sill_skip_insulation(int nc, int comp_group_num);

static void foundation_wall_insulation(void);
static int foundation_wall_skip_insulation(int nc, int comp_group_num);

static void floor_insulation(float rflr, int cms_measure_num);
static int floor_skip_insulation(int nc, int cms_measure_num, int comp_group_num, float added_r);

static void window_storm(void);
static void window_low_e(void);
static void window_shade(void);
static void window_screen(int cms_measure_num);
static void window_sealing(void);
static void window_replacement(void);

static void door_replacement(void);

static void heating_vent_damper(void);
static void heating_intermittent_ignition_device(void);
static void heating_vent_damper_electric(void);
static void heating_vent_damper_and_intermittent_ignition_device(void);
static void heating_flame_retention_burner(void);
static void heating_tuneup(void);
static void heating_replacement_standard_efficiency(void);
static void heating_replacement_high(int cms_measure_num);
static void heating_system_energy_savings(float load_fraction_met, float repeff);
static void heating_system_dollar_savings(float load_fraction_met, float repeff, int rmc_index);

static void cooling_replacement(void);
static void cooling_replacement_evaporative(void);
static void cooling_tuneup(void);
static void heatpump_replacement(void);
static float cooling_replacement_material_and_cost(float kbtu, enum CLGEQUIPTYPE type, int *nn);

static void lighting_replacement(void);

static void water_heater_tank_insulation(void);
static void water_heater_pipe_insulation(void);
static void water_heater_shower_heads(void);
static void water_heater_replacement(void);

static void refrigerator_replacement(void);

static void increment_measure_counter(void);
static int heating_repacement_material_standard_efficiency(void);
static int heating_repacement_material_high_efficiency(void);

static void add_material(char *comp, float studdepth, int wmeas, int rmc_index, char *desc, char *type, enum MATERIAL_TYPE mat_type,
                 char *units, float qty, float ucost, char *comment);

static void add_component_code(char *code);
static void add_component_code_once(char *code);
static float get_ceiling_joist_u_value(int nc, float radded);
static void non_zero_measure_cost(float cost, int cms_measure_num);

static void additional_cost(float cost);

/******  variables for this module only ******/

static float vnttsavf;        // thermal vent damper energy savings factor
static float vntesavf;        // thermal vent damper energy savings factor
static float iidsavs;         // intermittent ignition device summer savings
static float iidsavw;         // ditto for winter savings
static float iidsv;           // iid savings set either to iidsavs or iidsavw

// Routine for computing the energy savings due to applicable measures
// Modified 9/9/99 to use Fixed measure numbers in case stmts rather
// than execution measure numbers

void first_pass_measures(void) {
  int je;             // order of execution
  int jm;             // fixed measure number

  int measure_execution_order[] = {
    N_CMS_ATTIC_INSULATION_R11,
    N_CMS_ATTIC_INSULATION_R19,
    N_CMS_ATTIC_INSULATION_R30,
    N_CMS_ATTIC_INSULATION_R38,
    N_CMS_ATTIC_INSULATION_R49,
    N_CMS_FILL_CEILING_CAVITY,
    N_CMS_WALL_INSULATION,
    N_CMS_SILLBOX_INSULATION,
    N_CMS_FOUNDATION_WALL_INSULATION,
    N_CMS_FLOOR_INSULATION_R11,
    N_CMS_FLOOR_INSULATION_R19,
    N_CMS_FLOOR_INSULATION_R30,
    N_CMS_FLOOR_INSULATION_R38,
    N_CMS_FILL_FLOOR_CAVITY,
    N_CMS_WHITE_ROOF_COATING,
    N_CMS_KNEEWALL_INSULATION,
    N_CMS_DUCT_INSULATION,
    N_CMS_WINDOW_SEALING,
    N_CMS_STORM_WINDOWS,
    N_CMS_WINDOW_REPLACEMENT,
    N_CMS_LOW_E_WINDOWS,
    N_CMS_WINDOW_SHADING_AWNING,
    N_CMS_SUN_SCREEN_FABRIC,
    N_CMS_SUN_SCREEN_LOUVERED,
    N_CMS_WINDOW_FILM,
    N_CMS_DOOR_REPLACEMENT,
    N_CMS_THERMAL_VENT_DAMPER,
    N_CMS_ELECTRIC_VENT_DAMPER,
    N_CMS_IID,
    N_CMS_ELECTRIC_VENT_DAMPER_AND_IID,
    N_CMS_FLAME_RETENTION_BURNER,
    N_CMS_FURNACE_TUNE_UP,
    N_CMS_REPLACE_HEATING_SYSTEM,
    N_CMS_HIGH_EFFICIENCY_FURNACE,
    N_CMS_HIGH_EFFICIENCY_BOILER,
    N_CMS_SMART_THERMOSTAT,
    N_CMS_TUNE_UP_AC,
    N_CMS_REPLACE_AC,
    N_CMS_EVAPORATIVE_COOLER,
    N_CMS_INSTALL_OR_REPLACE_HEATPUMP,
    N_CMS_INFILTRATION_REDUCTION,
    N_CMS_LIGHTING_RETROFITS,
    N_CMS_REFRIGERATOR_REPLACEMENT,
    N_CMS_WATER_HEATER_TANK_INSULATION,
    N_CMS_WATER_HEATER_PIPE_INSULATION,
    N_CMS_LOW_FLOW_SHOWERHEADS,
    N_CMS_DUCT_SEALING,
    N_CMS_WATER_HEATER_REPLACEMENT
  };

  nir->nms = 0;  // zero the measures counter
  nir->nmsm = 0; // zero the measure-material counter

  // loop through measures to implement in execution order
  for (je = 0; je < (sizeof(measure_execution_order)/sizeof(measure_execution_order[0])); je++) {
    jm = measure_execution_order[je];       // jm is fixed measure number, je is execution order
    //if (nir->implement[jm]) {               // the implement flag is set so evaluate the measure
    if (ndi->cms[jm].active) {               // the implement flag is set so evaluate the measure
      switch (jm) {

      //  Thermal Envelope Measures

      case N_CMS_KNEEWALL_INSULATION:
        attic_insulation(13.0, jm);
        break;
      case N_CMS_ATTIC_INSULATION_R11:
        attic_insulation(11.0, jm);
        break;
      case N_CMS_ATTIC_INSULATION_R19:
        attic_insulation(19.0, jm);
        break;
      case N_CMS_ATTIC_INSULATION_R30:
        attic_insulation(30.0, jm);
        break;
      case N_CMS_ATTIC_INSULATION_R38:
        attic_insulation(38.0, jm);
        break;
      case N_CMS_ATTIC_INSULATION_R49:
        attic_insulation(49.0, jm);
        break;
      case N_CMS_FILL_CEILING_CAVITY:
        attic_insulation(50.0, jm);
        break;

      case N_CMS_WALL_INSULATION:
        wall_insulation();
        break;

      case N_CMS_SILLBOX_INSULATION:
        sill_insulation();
        break;

      case N_CMS_FOUNDATION_WALL_INSULATION:
        foundation_wall_insulation();
        break;

      case N_CMS_FLOOR_INSULATION_R11:
        floor_insulation(11.0, jm);
        break;
      case N_CMS_FLOOR_INSULATION_R19:
        floor_insulation(19.0, jm);
        break;
      case N_CMS_FLOOR_INSULATION_R30:
        floor_insulation(30.0, jm);
        break;
      case N_CMS_FLOOR_INSULATION_R38:
        floor_insulation(38.0, jm);
        break;
      case N_CMS_FILL_FLOOR_CAVITY:
        floor_insulation(50.0, jm);
        break;

      // Cooling Envelope & Window Measures

      case N_CMS_WHITE_ROOF_COATING:
        white_roof_coating(0, nir->nms);
        break;
      case N_CMS_STORM_WINDOWS:
        window_storm();
        break;
      case N_CMS_LOW_E_WINDOWS:
        window_low_e();
        break;
      case N_CMS_WINDOW_SHADING_AWNING:
        window_shade();
        break;
      case N_CMS_SUN_SCREEN_FABRIC:
        window_screen(jm);
        break;
      case N_CMS_SUN_SCREEN_LOUVERED:
        window_screen(jm);
        break;
      case N_CMS_WINDOW_FILM:
        window_screen(jm);
        break;
      case N_CMS_WINDOW_SEALING:
        window_sealing();
        break;
      case N_CMS_WINDOW_REPLACEMENT:
        window_replacement();
        break;
      case N_CMS_DOOR_REPLACEMENT:
        door_replacement();
        break;

      // Heating Mechanical System Measures

      case N_CMS_THERMAL_VENT_DAMPER:
        heating_vent_damper();
        break;
      case N_CMS_IID:
        heating_intermittent_ignition_device();
        break;
      case N_CMS_ELECTRIC_VENT_DAMPER:
        heating_vent_damper_electric();
        break;
      case N_CMS_ELECTRIC_VENT_DAMPER_AND_IID:
        heating_vent_damper_and_intermittent_ignition_device();
        break;
      case N_CMS_FLAME_RETENTION_BURNER:
        heating_flame_retention_burner();
        break;
      case N_CMS_FURNACE_TUNE_UP:
        heating_tuneup();
        break;
      case N_CMS_REPLACE_HEATING_SYSTEM:
        heating_replacement_standard_efficiency();
        break;
      case N_CMS_HIGH_EFFICIENCY_BOILER:
      case N_CMS_HIGH_EFFICIENCY_FURNACE:
        heating_replacement_high(jm);
        break;
      case N_CMS_SMART_THERMOSTAT:
        smart_thermostat(0);
        break;

      // Cooling Mechanical System Measures

      case N_CMS_TUNE_UP_AC:
        cooling_tuneup();
        break;
      case N_CMS_REPLACE_AC:
        cooling_replacement();
        break;
      case N_CMS_EVAPORATIVE_COOLER:
        cooling_replacement_evaporative();
        break;
      case N_CMS_INSTALL_OR_REPLACE_HEATPUMP:
        heatpump_replacement();
        break;

      // Infiltration and Duct Measures

      case N_CMS_INFILTRATION_REDUCTION:
        infiltration_reduction();
        break;
      case N_CMS_DUCT_INSULATION:
        duct_insulation();
        break;
      case N_CMS_DUCT_SEALING:
        duct_sealing();
        break;

      //  Base Load Measures

      case N_CMS_LIGHTING_RETROFITS:
        lighting_replacement();
        break;
      case N_CMS_WATER_HEATER_TANK_INSULATION:
        water_heater_tank_insulation();
        break;
      case N_CMS_WATER_HEATER_PIPE_INSULATION:
        water_heater_pipe_insulation();
        break;
      case N_CMS_LOW_FLOW_SHOWERHEADS:
        water_heater_shower_heads();
        break;
      case N_CMS_REFRIGERATOR_REPLACEMENT:
        refrigerator_replacement();
        break;
      case N_CMS_WATER_HEATER_REPLACEMENT:
        water_heater_replacement();
        break;
      default:
        break;
      }

      ASSERT(nir->nms < MAXECMS, sprintf(msg, "Measure limit: %d reached. Building description must be simplified or give more components the same measure number", MAXECMS));
      ASSERT(nir->nmsm < (4 * MAXECMS), sprintf(msg, "Measure limit: %d reached. Building description must be simplified or give more components the same measure number", (4 * MAXECMS)));

    }
  }

  itemized_cost_measures();

  return;
}

// This routine implements a specific measure lying above the min SIR, changing
// the dwelling information (audit) to construct the as-retrofited dwelling
// description

void second_pass_measure_interaction(int il) {
  float temp, temph, leakage_savings_factor, radded, inv_seaseff_post, areasill, filmSHGC;
  float farc1;
  int cms_measure_num;
  int rmc_index;

  nir->heatengy[CURRENT_MEASURE] = nir->heatengy[LAST_GOOD_MEASURE];
  nir->heatload[CURRENT_MEASURE] = nir->heatload[LAST_GOOD_MEASURE];
  nir->coolengy[CURRENT_MEASURE] = nir->coolengy[LAST_GOOD_MEASURE];
  nir->coolload[CURRENT_MEASURE] = nir->coolload[LAST_GOOD_MEASURE];

  cms_measure_num = nir->ecm[il].cms_measure_num;

  // Three basic groupings of measures, attics, floors, then everything else

  if (cms_measure_num == N_CMS_ATTIC_INSULATION_R11 ||
      cms_measure_num == N_CMS_ATTIC_INSULATION_R19 ||
      cms_measure_num == N_CMS_ATTIC_INSULATION_R30 ||
      cms_measure_num == N_CMS_ATTIC_INSULATION_R38 ||
      cms_measure_num == N_CMS_ATTIC_INSULATION_R49 ||
      cms_measure_num == N_CMS_KNEEWALL_INSULATION || 
      cms_measure_num == N_CMS_FILL_CEILING_CAVITY) {
    switch (cms_measure_num) {
    case N_CMS_KNEEWALL_INSULATION:
      radded = 13.0;
      break;
    case N_CMS_ATTIC_INSULATION_R11:
      radded = 11.0;
      break;
    case N_CMS_ATTIC_INSULATION_R19:
      radded = 19.0;
      break;
    case N_CMS_ATTIC_INSULATION_R30:
      radded = 30.0;
      break;
    case N_CMS_ATTIC_INSULATION_R38:
      radded = 38.0;
      break;
    case N_CMS_ATTIC_INSULATION_R49:
      radded = 49.0;
      break;
    case N_CMS_FILL_CEILING_CAVITY:
      radded = 50.0;
      break;
    }

    for (int nc = 0; nc < ndi->num_uas; nc++) {

      if (attic_skip_insulation(nc, nir->ecm[il].comp_group_type, nir->ecm[il].comp_group_num, cms_measure_num, &radded))
        continue;

      ndi->uas[nc].r_value += radded; /* for sizing,  this is where the audit gets modified */

      ASSERT(ndi->uas[nc].u_value, sprintf(msg, "Need non zero U Value"));
      ndi->uas[nc].u_value = 1.0f / (1.0f / ndi->uas[nc].u_value + radded);
      if (ndi->uas[nc].ins_depth < 5.5f)
        temp += ndi->uas[nc].ins_depth - 5.5f;
      if (temp < 0.)
        temp = 0.;

      // only kneewalls don't have floor joists
      if (ndi->uas[nc].attic_type != UAS_KNEEWALL) {
        ndi->uas[nc].joist_u_value = get_ceiling_joist_u_value(nc, radded);
      }

      rmc_index = attic_insulation_material(cms_measure_num, nc, 0, &temp, radded);
      if (cms_measure_num != N_CMS_FILL_CEILING_CAVITY)
        ndi->rmc[rmc_index].quant += ndi->uas[nc].area;
    }
    neat_energy_use("Attic Change", POST_RETROFIT);

  } else if (
    cms_measure_num == N_CMS_FLOOR_INSULATION_R11 ||
    cms_measure_num == N_CMS_FLOOR_INSULATION_R19 ||
    cms_measure_num == N_CMS_FLOOR_INSULATION_R30 ||
    cms_measure_num == N_CMS_FLOOR_INSULATION_R38 ||
    cms_measure_num == N_CMS_FILL_FLOOR_CAVITY) {
    
    switch (cms_measure_num) {
    case N_CMS_FLOOR_INSULATION_R11:
      radded = 11.;
      break;
    case N_CMS_FLOOR_INSULATION_R19:
      radded = 19.;
      break;
    case N_CMS_FLOOR_INSULATION_R30:
      radded = 30.;
      break;
    case N_CMS_FLOOR_INSULATION_R38:
      radded = 38.;
      break;
    }

    for (int nc = 0; nc < ndi->num_fnd; nc++) {

      if (floor_skip_insulation(nc, cms_measure_num, nir->ecm[il].comp_group_num, radded))
        continue;

      ndi->fnd[nc].floor_cavity_r += nir->ecm[il].value;
      ndi->fnd[nc].floor_framing_r += nir->ecm[il].value2;
      ndi->fnd[nc].flr_ins_r += nir->ecm[il].value; /* for sizing */

      ASSERT(ndi->fnd[nc].floor_framing_r, sprintf(msg, "Need non zero floor frame R value"));
      ASSERT(ndi->fnd[nc].floor_cavity_r, sprintf(msg, "Need non zero floor cavity R value"));
      ndi->fnd[nc].floor_u_value =
          FLOOR_FRAMING_RATIO / ndi->fnd[nc].floor_framing_r + (1.0f - FLOOR_FRAMING_RATIO) / ndi->fnd[nc].floor_cavity_r;
      rmc_index = floor_add_insulation_material(ndi->fnd[nc].added_floor, cms_measure_num);
      ndi->rmc[rmc_index].quant += ndi->fnd[nc].area;

    }
    neat_energy_use("Foundation Change", POST_RETROFIT);

  } else {   // all other measures

    switch (cms_measure_num) {
    case N_CMS_WALL_INSULATION:

      for (int nc = 0; nc < ndi->num_wal; nc++) {

        if (wall_skip_insulation(nc, nir->ecm[il].comp_group_num))
          continue;

        ASSERT(ndi->wal[nc].u_cavity, sprintf(msg, "Need non zero wall cavity u value"));
        ndi->wal[nc].u_cavity = 1.0f / (1.0f / ndi->wal[nc].u_cavity + nir->ecm[il].value);
        ndi->wal[nc].exist_r += nir->ecm[il].value; /* for sizing */
        rmc_index = wall_add_insulation_material(ndi->wal[nc].added_insulation);
        ASSERT(rmc_index < NMAT, sprintf(msg, "Array out of range"));
        ndi->rmc[rmc_index].quant += ndi->wal[nc].area;
      }
      neat_energy_use("Wall Change", POST_RETROFIT);
      break;

    case N_CMS_FOUNDATION_WALL_INSULATION:

      for (int nc = 0; nc < ndi->num_fnd; nc++) {

        if (foundation_wall_skip_insulation(nc, nir->ecm[il].comp_group_num))
          continue;

        ASSERT(ndi->fnd[nc].above_grade_wall_u_value, sprintf(msg, "Need non zero foundation u value a"));
        ASSERT(ndi->fnd[nc].below_grade_wall_u_value, sprintf(msg, "Need non zero foundation u value a"));
        ndi->fnd[nc].above_grade_wall_u_value = 1.0f / (1.0f / ndi->fnd[nc].above_grade_wall_u_value + nir->ecm[il].value);
        ndi->fnd[nc].below_grade_wall_u_value = 1.0f / (1.0f / ndi->fnd[nc].below_grade_wall_u_value + nir->ecm[il].value);
        ndi->fnd[nc].wall_ins_r += nir->ecm[il].value; /* for sizing */
        /*         usillins = 1./(1./SILL_U_VALUE + nir->ecm[il].value);  NOTE SILL_U_VALUEINS No a defined CONSTANT MJF 4/99*/
        //         n=41;
        rmc_index = foundation_wall_add_insulation_material(ndi->fnd[nc].added_found);
        ASSERT(rmc_index < NMAT, sprintf(msg, "Array out of range"));
        ndi->rmc[rmc_index].quant += ndi->fnd[nc].wall_area;
      }
      neat_energy_use("Foundation Wall Change", POST_RETROFIT);
      break;

    case N_CMS_SILLBOX_INSULATION:

      for (int nc = 0; nc < ndi->num_fnd; nc++) {

        if (sill_skip_insulation(nc, nir->ecm[il].comp_group_num))
          continue;

        //areasill = ndi->fnd[nc].ua_value_basement_sill / SILL_U_VALUE;    // #232
        areasill = ndi->fnd[nc].sill_perimeter * ndi->fnd[nc].joist_height / 12.0f;
        ASSERT(ndi->ins_sill[ndi->fnd[nc].added_sill].value > 0.0f, sprintf(msg, "Missing insulation value for foundation sill: %s", ndi->fnd[nc].code));
        radded = ndi->ins_sill[ndi->fnd[nc].added_sill].value;
        //         ndi->fnd[nc].ua_value_basement_sill *= SILL_U_VALUEINS/SILL_U_VALUE;
        ndi->fnd[nc].ua_value_basement_sill = 1.0f / (1.0f / SILL_U_VALUE + radded) * areasill;
        rmc_index = foundation_sill_add_insulation_material(ndi->fnd[nc].added_sill);
        ASSERT(rmc_index < NMAT, sprintf(msg, "Array out of range"));
        ndi->rmc[rmc_index].quant += areasill;
      }
      neat_energy_use("Sillbox Change", POST_RETROFIT);
      break;

    case N_CMS_STORM_WINDOWS:
      for (int nc = 0; nc < ndi->num_win; nc++) {
        if (nir->ecm[il].dwelling_component_index != nc)
          continue;
        if (ndi->win[nc].retrofit_option == WS_SEAL || ndi->win[nc].retrofit_option == WS_REPLACE ||
            ndi->win[nc].retrofit_option == WS_EVALUATE_NONE)
          continue;
        // Storms not requested since they are already installed
        if (ndi->win[nc].glazing_type == SINGLE_W_WOOD_STORM || ndi->win[nc].glazing_type == SINGLE_W_METAL_STORM)
          continue;
        ASSERT(ndi->win[nc].u_value, sprintf(msg, "Need non zero window u value"));
        ndi->win[nc].u_value = 1.0f / (nir->ecm[il].value + 1.0f / ndi->win[nc].u_value);
        leakage_savings_factor = window_storm_leak_coef(ndi->win[nc].leak_coef) / ndi->win[nc].leak_coef;
        ndi->win[nc].leak_coef *= leakage_savings_factor;
        for (int m = 1; m <= MONTHS; m++) {
          nir->wn_cfm_tot[m] -= nir->wn_leak_cfm[nc][m] * (1.0f - leakage_savings_factor);
          nir->wn_leak_cfm[nc][m] *= leakage_savings_factor;
        }
        //        ndi->win[nc].shgc_winter *= STORMSHGC, ndi->win[nc].shgc_summer *= STORMSHGC;
        ndi->win[nc].shgc_winter *= ndi->key.storm_window_shgc;
        ndi->win[nc].shgc_summer *= ndi->key.storm_window_shgc;
        if (ndi->win[nc].glazing_type == DOUBLE_GLAZED || ndi->win[nc].glazing_type == DOUBLE_GLAZED_LOWE) {
          //          ndi->win[nc].shgc_winter -= .075f, ndi->win[nc].shgc_summer -= .075f;
          ndi->win[nc].glazing_type = DOUBLE_GLAZED_W_STORM;
        } /* for sizing */
        else {
          //          ndi->win[nc].shgc_winter -= .093f, ndi->win[nc].shgc_summer -= .093f;
          ndi->win[nc].glazing_type = SINGLE_W_METAL_STORM;
        } /* for sizing */
        ndi->rmc[N_MAT_STORM_WINDOWS].quant += (int)(ndi->win[nc].number);
      }
      neat_energy_use("Storm Window", POST_RETROFIT);
      break;

    case N_CMS_WINDOW_SEALING:
      for (int nc = 0; nc < ndi->num_win; nc++) {
        if (nir->ecm[il].dwelling_component_index != nc)
          continue;

        leakage_savings_factor = window_sealing_leak_coef(ndi->win[nc].leak_coef) / ndi->win[nc].leak_coef;

        if (ndi->win[nc].retrofit_option == WS_REPLACE || ndi->win[nc].retrofit_option == WS_ADD_STORM ||
            ndi->win[nc].retrofit_option == WS_EVALUATE_NONE)
          continue;
        // Measure not requested

        // Window sealing effects are independent of the glazing type, only depends on the leakiness.
        ndi->win[nc].leak_coef *= leakage_savings_factor;
        for (int m = 1; m <= MONTHS; m++) {
          nir->wn_cfm_tot[m] -= nir->wn_leak_cfm[nc][m] * (1.0f - leakage_savings_factor);
          nir->wn_leak_cfm[nc][m] *= leakage_savings_factor;
        }
        ndi->rmc[N_MAT_WINDOW_SEALING].quant += (int)(ndi->win[nc].number);
      }
      neat_energy_use("Window Sealing", POST_RETROFIT);
      break;

    case N_CMS_WINDOW_REPLACEMENT:
      for (int nc = 0; nc < ndi->num_win; nc++) {
        if (nir->ecm[il].dwelling_component_index != nc)
          continue;
        if (ndi->win[nc].retrofit_option == WS_SEAL || ndi->win[nc].retrofit_option == 4 ||
            ndi->win[nc].retrofit_option == WS_EVALUATE_NONE)
          continue;
        // Measure not requested

        // Mandatory replacements
        // have an effect on windows other than just single pane or
        // single pane with storm window types,  MJF

        ndi->win[nc].u_value = ndi->key.new_standard_window_u_value;
        ndi->win[nc].shgc_winter = ndi->win[nc].shgc_summer = ndi->key.new_standard_window_shgc;
        leakage_savings_factor = WINDOW_MEC_LEAKAGE_COEF / ndi->win[nc].leak_coef;
        ndi->win[nc].leak_coef *= leakage_savings_factor;
        for (int m = 1; m <= MONTHS; m++) {
          nir->wn_cfm_tot[m] -= nir->wn_leak_cfm[nc][m] * (1.0f - leakage_savings_factor);
          nir->wn_leak_cfm[nc][m] *= leakage_savings_factor;
        }
        ndi->win[nc].glazing_type = DOUBLE_GLAZED;
        ndi->rmc[N_MAT_WINDOW_REPLACEMENT].quant += (int)(ndi->win[nc].number);
      }
      neat_energy_use("Window Replace", POST_RETROFIT);
      break;

    case N_CMS_DOOR_REPLACEMENT:
      for (int nc = 0; nc < ndi->num_dor; nc++) {
        if (nir->ecm[il].dwelling_component_index != nc)
          continue;
        ndi->dor[nc].door_type = WOOD_SOLID_CORE; /* for sizing */
        // ndi->dor[nc].u_value = door_u_value(ndi->dor[nc].door_type, DR_NONE, ASHRAE);
        ndi->dor[nc].u_value = 0.4f;
        ASSERT(ndi->dor[nc].leak_coef, sprintf(msg, "Need non zero door leakage coeff"));
        leakage_savings_factor = NEW_DOOR_LEAKAGE_COEF / ndi->dor[nc].leak_coef;
        ndi->dor[nc].leak_coef *= leakage_savings_factor;
        for (int m = 1; m <= MONTHS; m++) {
          nir->dr_cfm_tot[m] -= nir->dr_leak_cfm[nc][m] * (1.0f - leakage_savings_factor);
          nir->dr_leak_cfm[nc][m] *= leakage_savings_factor;
        }
        ndi->rmc[N_MAT_DOOR_REPLACEMENT].quant += (int)(ndi->dor[nc].number);
      }
      neat_energy_use("Door Change", POST_RETROFIT);
      break;

    case N_CMS_LOW_E_WINDOWS:
      for (int nc = 0; nc < ndi->num_win; nc++) {
        if (nir->ecm[il].dwelling_component_index != nc)
          continue;
        if (ndi->win[nc].retrofit_option == WS_SEAL || ndi->win[nc].retrofit_option == 4 ||
            ndi->win[nc].retrofit_option == WS_EVALUATE_NONE)
          continue;
        // Measure not requested

        // required replacements
        // have an effect on windows other than just single pane or
        // single pane with storm window types,  For Version 8604. Had been
        // done previously for normal window replacement.

        ndi->win[nc].u_value = ndi->key.new_lowe_window_u_value;
        ndi->win[nc].shgc_winter = ndi->win[nc].shgc_summer = ndi->key.new_lowe_window_shgc;
        leakage_savings_factor = WINDOW_MEC_LEAKAGE_COEF / ndi->win[nc].leak_coef;
        ndi->win[nc].leak_coef *= leakage_savings_factor;
        for (int m = 1; m <= MONTHS; m++) {
          nir->wn_cfm_tot[m] -= nir->wn_leak_cfm[nc][m] * (1.0f - leakage_savings_factor);
          nir->wn_leak_cfm[nc][m] *= leakage_savings_factor;
        }
        ndi->win[nc].glazing_type = DOUBLE_GLAZED_LOWE;
        ndi->rmc[N_MAT_LOW_E_WINDOWS].quant += (int)(ndi->win[nc].number);
      }    
      neat_energy_use("Low-e Window Change", POST_RETROFIT);
      break;

    // Cooling Envelope Measures

    case N_CMS_WINDOW_SHADING_AWNING:
      for (int nc = 0; nc < ndi->num_win; nc++) {
        if (window_not_suitable_for_shading_retrofit(nc))
          continue;
        ndi->win[nc].shade_factor_winter *= 0.8f;
        ndi->win[nc].shade_factor_summer = 0.1f;
        ndi->rmc[N_MAT_WINDOW_SHADING_AWNING].quant += ndi->win[nc].width / 12 * ndi->win[nc].number;
      }
      neat_energy_use("Window Shading", POST_RETROFIT);
      break;

    case N_CMS_SUN_SCREEN_FABRIC:
      for (int nc = 0; nc < ndi->num_win; nc++) {
        if (window_not_suitable_for_shading_retrofit(nc))
          continue;
        nir->wn_sunscrn[COOLING][nc] = window_treatment_shgc(cms_measure_num);
        if (nir->screens_removed_for_winter == NO) {
          nir->wn_sunscrn[HEATING][nc] = window_treatment_shgc(cms_measure_num);
        }
        ndi->rmc[N_MAT_SUN_SCREEN_FABRIC].quant += ndi->win[nc].area_gross;
      }
      neat_energy_use("Sun Screen Fabric", POST_RETROFIT);
      break;
    case N_CMS_SUN_SCREEN_LOUVERED:
      for (int nc = 0; nc < ndi->num_win; nc++) {
        if (window_not_suitable_for_shading_retrofit(nc))
          continue;
        nir->wn_sunscrn[COOLING][nc] = window_treatment_shgc(cms_measure_num);
        if (nir->screens_removed_for_winter == NO) {
          nir->wn_sunscrn[HEATING][nc] = window_treatment_shgc(cms_measure_num);
        }
        ndi->rmc[N_MAT_SUN_SCREEN_LOUVERED].quant += ndi->win[nc].area_gross;
      }
      neat_energy_use("Sun Screen Louvered", POST_RETROFIT);
      break;

    case N_CMS_WINDOW_FILM:
      for (int nc = 0; nc < ndi->num_win; nc++) {
        if (window_not_suitable_for_shading_retrofit(nc))
          continue;
        ndi->win[nc].shgc_summer = window_treatment_shgc(cms_measure_num);
        filmSHGC = ndi->key.window_film_shgc;
        if (ndi->win[nc].glazing_type != SINGLE && ndi->win[nc].glazing_type != SINGLE_W_BAD_STORM) {
          ndi->win[nc].glazing_type = DOUBLE_GLAZED_LOWE; // For sizing seen as double pane low-e
          filmSHGC = 1.1f * ndi->key.window_film_shgc;
        }
        // Film industry said SHGC 10% greater for 2-pane wdws. 10/09
        else
          ndi->win[nc].glazing_type = DOUBLE_GLAZED; // For sizing see as double pane
        ndi->win[nc].shgc_summer = ndi->win[nc].shgc_winter = filmSHGC;
        if (ndi->key.window_film_delta_r > 0.0f)
          ndi->win[nc].u_value /= (1.0f + ndi->win[nc].u_value * ndi->key.window_film_delta_r);
        // Issue #138 Getting change for 8.9.1.0, MJF 1/20 based on industry feedback that
        // film material is sized from gross rather than net glazing area
        // ndi->rmc[N_MAT_WINDOW_FILM].quant += ndi->win[nc].area_glazing;
        ndi->rmc[N_MAT_WINDOW_FILM].quant += ndi->win[nc].area_gross;
      }
      neat_energy_use("Sun Screen Film", POST_RETROFIT);
      break;

    //Heating Mechanical System Measures

    case N_CMS_THERMAL_VENT_DAMPER:
      ASSERT(ndi->htg[PRIMARY].delivered_eff, sprintf(msg, "Need non zero heating system eff"));
      nir->heatengy[CURRENT_MEASURE] = nir->heatengy[LAST_GOOD_MEASURE] - nir->heatload[LAST_GOOD_MEASURE] * ndi->htg[PRIMARY].percent_heat_supplied * vnttsavf / ndi->htg[PRIMARY].delivered_eff;
      ASSERT((1.0f - vnttsavf), sprintf(msg, "Need rational vent damper savings"));
      ndi->htg[PRIMARY].delivered_eff /= (1.0f - vnttsavf);
      ASSERT(nir->heatengy[CURRENT_MEASURE], sprintf(msg, "Need non zero heating energy"));
      nir->sysht_seaseff = nir->heatload[LAST_GOOD_MEASURE] / nir->heatengy[CURRENT_MEASURE];
      ndi->rmc[N_MAT_THERMAL_VENT_DAMPER].quant = 1;
      break;

    case N_CMS_ELECTRIC_VENT_DAMPER:
      ASSERT(ndi->htg[PRIMARY].delivered_eff, sprintf(msg, "Need non zero heating system eff"));
      nir->heatengy[CURRENT_MEASURE] = nir->heatengy[LAST_GOOD_MEASURE] - nir->heatload[LAST_GOOD_MEASURE] * ndi->htg[PRIMARY].percent_heat_supplied * vntesavf / ndi->htg[PRIMARY].delivered_eff;
      ASSERT((1.0f - vntesavf), sprintf(msg, "Need rational vent damper savings"));
      ndi->htg[PRIMARY].delivered_eff /= (1.0f - vntesavf);
      ASSERT(nir->heatengy[CURRENT_MEASURE], sprintf(msg, "Need non zero heating energy"));
      nir->sysht_seaseff = nir->heatload[LAST_GOOD_MEASURE] / nir->heatengy[CURRENT_MEASURE];
      ndi->rmc[N_MAT_ELECTRIC_VENT_DAMPER].quant = 1;
      break;

    case N_CMS_IID:
      ASSERT(ndi->htg[PRIMARY].delivered_eff, sprintf(msg, "Need non zero heating system eff"));
      ASSERT((ndi->htg[PRIMARY].percent_heat_supplied * nir->heatload[LAST_GOOD_MEASURE]) != 0, sprintf(msg, "IID error"));
      ndi->htg[PRIMARY].delivered_eff =
          1.0f / (1.0f / ndi->htg[PRIMARY].delivered_eff - nir->ecm[il].ensav / (ndi->htg[PRIMARY].percent_heat_supplied * nir->heatload[LAST_GOOD_MEASURE]));
      nir->heatengy[CURRENT_MEASURE] = nir->heatengy[LAST_GOOD_MEASURE] - iidsv;
      ASSERT(nir->heatengy[CURRENT_MEASURE], sprintf(msg, "Need non zero heating energy"));
      nir->sysht_seaseff = nir->heatload[LAST_GOOD_MEASURE] / nir->heatengy[CURRENT_MEASURE];
      ndi->rmc[N_MAT_IID].quant = 1;
      break;

    case N_CMS_ELECTRIC_VENT_DAMPER_AND_IID:
      ASSERT(ndi->htg[PRIMARY].delivered_eff, sprintf(msg, "Need non zero heating system eff"));
      temp = nir->heatload[LAST_GOOD_MEASURE] * ndi->htg[PRIMARY].percent_heat_supplied * vntesavf / ndi->htg[PRIMARY].delivered_eff;
      temph = nir->ecm[il].value + MAX(temp, iidsavw) + 0.5f * MIN(temp, iidsavw);
      nir->heatengy[CURRENT_MEASURE] = nir->heatengy[LAST_GOOD_MEASURE] - temph;
      ASSERT(nir->heatengy[CURRENT_MEASURE], sprintf(msg, "Need non zero heating energy"));
      nir->sysht_seaseff = nir->heatload[LAST_GOOD_MEASURE] / nir->heatengy[CURRENT_MEASURE];
      ASSERT((ndi->htg[PRIMARY].percent_heat_supplied * nir->heatload[LAST_GOOD_MEASURE]) != 0, sprintf(msg, "Electric Vent Damper error"));
      ndi->htg[PRIMARY].delivered_eff =
          1.0f / (1.0f / ndi->htg[PRIMARY].delivered_eff - temph / (ndi->htg[PRIMARY].percent_heat_supplied * nir->heatload[LAST_GOOD_MEASURE]));
      ndi->rmc[N_MAT_ELECTRIC_VENT_DAMPER_AND_IID].quant = 1;
      break;

    case N_CMS_FLAME_RETENTION_BURNER:

      temp = ndi->htg[PRIMARY].delivered_eff;
      ASSERT((1.0f - nir->ecm[il].value), sprintf(msg, "Need non zero savings value"));
      ndi->htg[PRIMARY].delivered_eff = 0.8f * SEASONAL_TO_SS_RATIO * ndi->inf.post_duct_seal_efficiency / (1.0f - nir->ecm[il].value);
      ASSERT(ndi->htg[PRIMARY].delivered_eff, sprintf(msg, "Need non zero heating system eff"));
      nir->heatengy[CURRENT_MEASURE] =
          nir->heatengy[LAST_GOOD_MEASURE] - nir->heatload[LAST_GOOD_MEASURE] * ndi->htg[PRIMARY].percent_heat_supplied * (1.0f / temp - 1.0f / ndi->htg[PRIMARY].delivered_eff);
      ASSERT(nir->heatengy[CURRENT_MEASURE], sprintf(msg, "Need non zero heating energy"));
      nir->sysht_seaseff = nir->heatload[LAST_GOOD_MEASURE] / nir->heatengy[CURRENT_MEASURE];
      ndi->rmc[N_MAT_FLAME_RETENTION_BURNER].quant = 1;
      break;

    case N_CMS_FURNACE_TUNE_UP:

      ASSERT(ndi->inf.post_duct_seal_efficiency, sprintf(msg, "Tuneup savings error"));
      temp = nir->heatload[LAST_GOOD_MEASURE] / ndi->htg[PRIMARY].delivered_eff * ndi->htg[PRIMARY].percent_heat_supplied * nir->ecm[il].value /
             (ndi->htg[PRIMARY].delivered_eff / SEASONAL_TO_SS_RATIO / ndi->inf.post_duct_seal_efficiency + nir->ecm[il].value);
      nir->heatengy[CURRENT_MEASURE] = nir->heatengy[LAST_GOOD_MEASURE] - temp;
      nir->sysht_seaseff = nir->heatload[LAST_GOOD_MEASURE] / nir->heatengy[CURRENT_MEASURE];
      ndi->htg[PRIMARY].delivered_eff =
          1.0f / (1.0f / ndi->htg[PRIMARY].delivered_eff - temp / nir->heatload[LAST_GOOD_MEASURE] / ndi->htg[PRIMARY].percent_heat_supplied);
      break;

    case N_CMS_REPLACE_HEATING_SYSTEM:
    case N_CMS_HIGH_EFFICIENCY_FURNACE:
    case N_CMS_HIGH_EFFICIENCY_BOILER:

      if (cms_measure_num == N_CMS_REPLACE_HEATING_SYSTEM) { /* Standard efficiency replacement */
        inv_seaseff_post = nir->ecm[il].value;
        ndi->htg[PRIMARY].delivered_eff = ndi->htg[PRIMARY].retrofit_afue_standard_eff / 100.0f;
      }

      else { /* High Eff. Furn. Rep. */
        inv_seaseff_post = nir->ecm[il].value2;
        ndi->htg[PRIMARY].delivered_eff = ndi->htg[PRIMARY].retrofit_afue_high_eff / 100.0f;
      }

      ASSERT(nir->sysht_seaseff, sprintf(msg, "Need non zero heating system efficiency"));

      temp = (1.0f / nir->sysht_seaseff - inv_seaseff_post) * nir->heatload[LAST_GOOD_MEASURE];
      nir->heatengy[CURRENT_MEASURE] = nir->heatengy[LAST_GOOD_MEASURE] - temp;

      nir->heatengy[CURRENT_MEASURE] = nir->heatengy[LAST_GOOD_MEASURE] - temp;
      nir->htdlsav[il] = nir->heatengy[LAST_GOOD_MEASURE] * fuel_cost_per_mmbtu(ndi->htg[PRIMARY].fuel_type) -
                    nir->heatengy[CURRENT_MEASURE] * fuel_cost_per_mmbtu(ndi->htg[PRIMARY].retrofit_fuel_type);
      nir->htdlsav[il] *= nir->bladj[PRE_HEATING];

      nir->dslfsav[il] = nir->heatengy[LAST_GOOD_MEASURE] * pw_fuel_cost(ndi->htg[PRIMARY].fuel_type, nir->mslife[il]) -
                    nir->heatengy[CURRENT_MEASURE] * pw_fuel_cost(ndi->htg[PRIMARY].retrofit_fuel_type, nir->mslife[il]);

      nir->dslfsav[il] *= nir->bladj[PRE_HEATING];
      ndi->htg[PRIMARY].fuel_type = ndi->htg[PRIMARY].retrofit_fuel_type;
      ASSERT(nir->heatengy[CURRENT_MEASURE], sprintf(msg, "Need non zero heating energy"));
      nir->sysht_seaseff = nir->heatload[LAST_GOOD_MEASURE] / nir->heatengy[CURRENT_MEASURE];
      break;

    case N_CMS_SMART_THERMOSTAT:
      smart_thermostat(1); 
      ndi->rmc[N_MAT_SMART_THERMOSTAT].quant = 1;
      break;

    // Cooling Mechanical System Measures

    case N_CMS_REPLACE_AC: {

      int nc = nir->ecm[il].dwelling_component_index;

      switch (ndi->clg[nir->ecm[il].dwelling_component_index].system_type) {
      case CET_CENTRAL:
        ndi->clg[nc].seer = ndi->key.new_central_ac_seer;
        break;
      case CET_WINDOW:
        ndi->clg[nc].seer = ndi->key.new_window_ac_seer;
        break;
      case CET_HEATPUMP:
        ndi->clg[nc].seer = ndi->key.new_heatpump_cooling_seer;
        break;
      default:
        ASSERT(FALSE, sprintf(msg, "Unknown AC replacement type:%d", ndi->clg[nir->ecm[il].dwelling_component_index].system_type));
      }
      nir->coolengy[CURRENT_MEASURE] = nir->coolengy[LAST_GOOD_MEASURE] - nir->coolload[LAST_GOOD_MEASURE] * nir->ecm[il].value;
      ndi->clgs.avg_seer = nir->coolload[LAST_GOOD_MEASURE] / nir->coolengy[CURRENT_MEASURE] * 3.413f * ndi->clgs.fraction_cooled;
      break; }

    case N_CMS_TUNE_UP_AC: {

      int nc = nir->ecm[il].dwelling_component_index;

      ndi->clg[nc].seer = nir->ecm[il].value2;
      nir->coolengy[CURRENT_MEASURE] = nir->coolengy[LAST_GOOD_MEASURE] - nir->coolload[LAST_GOOD_MEASURE] * nir->ecm[il].value;
      ndi->clgs.avg_seer = nir->coolload[LAST_GOOD_MEASURE] / nir->coolengy[CURRENT_MEASURE] * 3.413f;
      break; }

    case N_CMS_EVAPORATIVE_COOLER:

      nir->coolengy[CURRENT_MEASURE] = nir->coolengy[LAST_GOOD_MEASURE] - nir->coolload[LAST_GOOD_MEASURE] * nir->ecm[il].value;
      ndi->clgs.avg_seer = nir->coolload[LAST_GOOD_MEASURE] / nir->coolengy[CURRENT_MEASURE] * 3.413f;
      ndi->rmc[N_MAT_EVAPORATIVE_COOLER].quant = 1;
      break;

    case N_CMS_INSTALL_OR_REPLACE_HEATPUMP: {

      int nc = nir->ecm[il].dwelling_component_index;

      ndi->clg[nc].seer = ndi->key.new_heatpump_cooling_seer;
      nir->clengysav[il] = nir->coolload[LAST_GOOD_MEASURE] * nir->ecm[il].value;
      nir->coolengy[CURRENT_MEASURE] = nir->coolengy[LAST_GOOD_MEASURE] - nir->clengysav[il];
      if (ndi->clgs.fraction_cooled < .0001)
        ndi->clgs.fraction_cooled = 1.0f;
      // MBG 10/01  If new installation assume cools entire house
      // MBG 6/10 seer calc moved here after the perac limit statement
      ndi->clgs.avg_seer = nir->coolload[LAST_GOOD_MEASURE] / nir->coolengy[CURRENT_MEASURE] * 3.413f * ndi->clgs.fraction_cooled;

      inv_seaseff_post = nir->ecm[il].value2;

      float save_hspf = ndi->htg[PRIMARY].retrofit_hspf;
      adjust_hspf(cwd->winter_hp_design_temp, &save_hspf);
      ndi->htg[PRIMARY].delivered_eff = save_hspf / 3.413f;
      temp = (1.0f / nir->sysht_seaseff - inv_seaseff_post) * nir->heatload[LAST_GOOD_MEASURE];
      nir->heatengy[CURRENT_MEASURE] = nir->heatengy[LAST_GOOD_MEASURE] - temp;
      nir->htdlsav[il] = nir->heatengy[LAST_GOOD_MEASURE] * fuel_cost_per_mmbtu(ndi->htg[PRIMARY].fuel_type) - nir->heatengy[CURRENT_MEASURE] * fuel_cost_per_mmbtu(ELECTRIC);
      nir->htdlsav[il] *= nir->bladj[PRE_HEATING];
      // clang-format off
      nir->dslfsav[il] =
          (nir->heatengy[LAST_GOOD_MEASURE] * pw_fuel_cost(ndi->htg[PRIMARY].fuel_type, nir->mslife[il]) - 
           nir->heatengy[CURRENT_MEASURE] * pw_fuel_cost(ELECTRIC, nir->mslife[il])) * nir->bladj[PRE_HEATING] +
           nir->clengysav[il] * pw_fuel_cost(ELECTRIC, nir->mslife[il]) * nir->bladj[PRE_COOLING];
      // clang-format on
      ASSERT(nir->heatengy[CURRENT_MEASURE], sprintf(msg, "Need non zero heating energy"));
      nir->sysht_seaseff = nir->heatload[LAST_GOOD_MEASURE] / nir->heatengy[CURRENT_MEASURE];
      //        ndi->htg[PRIMARY].delivered_eff = 1.0f/(1.0f/ndi->htg[PRIMARY].delivered_eff-temp/nir->heatload[LAST_GOOD_MEASURE]/
      //          ndi->htg[PRIMARY].percent_heat_supplied);
      break; }

    case N_CMS_DUCT_INSULATION:

      ndi->rmc[N_MAT_DUCT_INSULATION].quant = nir->ductarea;
      return;

    case N_CMS_LIGHTING_RETROFITS:
      return;

    case N_CMS_WATER_HEATER_TANK_INSULATION:
      ndi->rmc[N_MAT_WATER_HEATER_TANK_INSULATION].quant = 1;
      return;

    case N_CMS_WHITE_ROOF_COATING:
      for (int nc = 0; nc < ndi->num_uas; nc++) {
        farc1 = STICK_BUILT_ROOF_TO_ATTIC_AREA_RATIO;
        if (ndi->uas[nc].attic_type == UAS_KNEEWALL)
          continue;
        //if (ndi->uas[nc].attic_type == UAS_CATHEDRAL)
        if (ndi->uas[nc].attic_type == UAS_CATHEDRAL || ndi->uas[nc].attic_type == UAS_ROOF_RAFTER)
          farc1 = 1.0f;
        ndi->uas[nc].roof_absorptance = WHITE_ROOF_ABSORPTIVITY;
        ndi->rmc[N_MAT_WHITE_ROOF_COATING].quant += ndi->uas[nc].area * farc1;
      }
      neat_energy_use("White Roof", POST_RETROFIT);
      break;
    default:
      return;   // only the above non-return measures will continue below
    }  // end of switch
  } // end of balance of measures else

  nir->htengysav[il] = (nir->heatengy[LAST_GOOD_MEASURE] - nir->heatengy[CURRENT_MEASURE]) * nir->bladj[PRE_HEATING];
  nir->clengysav[il] = (nir->coolengy[LAST_GOOD_MEASURE] - nir->coolengy[CURRENT_MEASURE]) * nir->bladj[PRE_COOLING];

  nir->heatengy[LAST_GOOD_MEASURE] = nir->heatengy[CURRENT_MEASURE];
  nir->heatload[LAST_GOOD_MEASURE] = nir->heatload[CURRENT_MEASURE];
  nir->coolengy[LAST_GOOD_MEASURE] = nir->coolengy[CURRENT_MEASURE];
  nir->coolload[LAST_GOOD_MEASURE] = nir->coolload[CURRENT_MEASURE];
  
  nir->ecm[il].ensav = nir->htengysav[il] + nir->clengysav[il];
  nir->cldlsav[il] = nir->clengysav[il] * fuel_cost(ELECTRIC) * 1000.0f / 3.413f;

  int itype = nir->meas_type[cms_measure_num];

  if (itype != CMT_HEATING_SYSTEM_REPLACE && itype != CMT_HEAT_PUMP_REPLACE) {    // Measures with possible fuel switching

    if (itype == CMT_HEATING_ENVELOPE || 
        itype == CMT_COOLING_ENVELOPE || 
        itype == CMT_SMART_THERMOSTAT || 
        itype == CMT_WINDOWS_DOORS) { /* Saves composite fuel */
      nir->htdlsav[il] = nir->htengysav[il] * CompFuelCost();
    } else if (itype == CMT_HEATING_SYSTEM_UPDATE) {      /* Saves fuel of primary heating system */
      nir->htdlsav[il] = nir->htengysav[il] * fuel_cost_per_mmbtu(ndi->htg[PRIMARY].fuel_type);
    }
    nir->dslfsav[il] = nir->htengysav[il] * nir->htlcs[il] + nir->clengysav[il] * nir->cllcs[il];
  }

  nir->ecm[il].dlsav = nir->htdlsav[il] + nir->cldlsav[il];
  nir->npv[il] = nir->dslfsav[il] - nir->ecm[il].cost;
  nir->ecm[il].sir = calculate_sir(nir->dslfsav[il], nir->ecm[il].cost);
  nir->ecm[il].lifetime = nir->mslife[il];

  return;
}

// Annual energy change (dhtld and dclld) from change in monthly UA [duam] and monthly free heat [dfheat]
// dhtld & dclld computed and returned in units of annual MMBtu (10**6 Btu)

int annual_energy_load_change(float duam[], float dfheat[], float *dhtld, float *dclld) {
  int m;
  float temp1 = 0, temp2 = 0, temp3 = 0;
  float blct[MONTHS + 1];
  float tbalt[DAY + 1][COOLING + 1][MONTHS + 1];
  float drs[COOLING + 1][MONTHS + 1];
  float hld, cld, fSolarStorage, totsolt, freeheatt[2];

  nor->energy_delta_counter++; // gitlab #47

  hld = cld = 0.;
  for (m = 1; m <= MONTHS; m++) {
    blct[m] = nir->building_load_coeff[m] - duam[m];

    // Apply the solar storage factor

    totsolt = nir->totsol[m] - dfheat[m];

    neat_solar_storage(cwd->avg_daytime_temp[m], totsolt, ndi->key.base_free_heat_from_internals, 0.0, blct[m], &fSolarStorage);

    freeheatt[NIGHT] = 2.0f * fSolarStorage * totsolt + ndi->key.base_free_heat_from_internals;
    freeheatt[DAY] = 2.0f * (1.0f - fSolarStorage) * totsolt + ndi->key.base_free_heat_from_internals;

    tbalt[NIGHT][HEATING][m] = nir->night_setback_temperature[m] - freeheatt[NIGHT] / blct[m];
    tbalt[DAY][HEATING][m] = ndi->key.daytime_heating_setpoint - freeheatt[DAY] / blct[m];
    tbalt[NIGHT][COOLING][m] = ndi->key.nighttime_cooling_setpoint - freeheatt[NIGHT] / (blct[m] - nir->dblcs);
    tbalt[DAY][COOLING][m] = ndi->key.daytime_cooling_setpoint - freeheatt[DAY] / (blct[m] - nir->dblcs);
  }

  adjusted_monthly_degree_hours(tbalt, drs);

  for (m = 1; m <= MONTHS; m++) {
    temp1 += drs[COOLING][m] * (blct[m] - nir->dblcs) * ndi->clgs.fraction_cooled;
    temp2 += nir->latentload[POST_RETROFIT][m] * ndi->clgs.fraction_cooled;
    temp3 += nir->wn_lat_load[m] * ndi->clgs.fraction_cooled;
    hld += drs[HEATING][m] * blct[m];
    cld += drs[COOLING][m] * (blct[m] - nir->dblcs) + nir->latentload[POST_RETROFIT][m] + nir->wn_lat_load[m] + nir->dr_lat_load[m];
  }
  *dhtld = nir->heatload[LAST_GOOD_MEASURE] - hld * 1.0e-6f;
  *dclld = nir->coolload[LAST_GOOD_MEASURE] - cld * 1.0e-6f;
  return (0);
}

static void measure_economics_using_rmc(int ecm_index, int rmc_index) {

  ASSERT(rmc_index < NMAT, sprintf(msg, "Material array index out of range: %d", rmc_index));
  nir->mslife[ecm_index] = ndi->rmc[rmc_index].life;
  nir->htengysav[ecm_index] *= nir->bladj[PRE_HEATING];
  nir->clengysav[ecm_index] *= nir->bladj[PRE_COOLING];
  nir->cldlsav[ecm_index] = nir->clengysav[ecm_index] * fuel_cost(ELECTRIC) * 1000.0f / 3.413f;
  nir->htdlsav[ecm_index] = nir->htengysav[ecm_index] * CompFuelCost();
  nir->dslfsav[ecm_index] = nir->htengysav[ecm_index] * DCompFuelCost(ndi->rmc[rmc_index].life) +
      nir->clengysav[ecm_index] * pw_fuel_cost(ELECTRIC, ndi->rmc[rmc_index].life);

  // to this we add the baseload savings (MJF 1/01)
  nir->ecm[ecm_index].ensav = nir->htengysav[ecm_index] + nir->clengysav[ecm_index] + nir->blengysav[ecm_index];
  nir->ecm[ecm_index].dlsav = nir->htdlsav[ecm_index] + nir->cldlsav[ecm_index] + nir->bldlsav[ecm_index];
  nir->ecm[ecm_index].lifetime = ndi->rmc[rmc_index].life;

  // point our ecm[] to associated rmc[] and  vice versa
  nir->ecm[ecm_index].rmc_index = rmc_index;
  ndi->rmc[rmc_index].ecm_index = ecm_index;

  // finally the SIR and NPV
  nir->ecm[ecm_index].sir = calculate_sir(nir->dslfsav[ecm_index], nir->ecm[ecm_index].cost);
  nir->npv[ecm_index] = nir->dslfsav[ecm_index] - nir->ecm[ecm_index].cost;
  return;
}

static void measure_economics_hvac(int ecm_index, int rmc_index) {

  ASSERT(rmc_index < NMAT, sprintf(msg, "Material array index out of range"));
  nir->mslife[ecm_index] = ndi->rmc[rmc_index].life;
  nir->htengysav[ecm_index] *= nir->bladj[PRE_HEATING];
  nir->clengysav[ecm_index] *= nir->bladj[PRE_COOLING];

  nir->cldlsav[ecm_index] = nir->clengysav[ecm_index] * fuel_cost(ELECTRIC) * 1000.0f / 3.413f;
  nir->htdlsav[ecm_index] = nir->htengysav[ecm_index] * fuel_cost_per_mmbtu(ndi->htg[PRIMARY].fuel_type);
  nir->dslfsav[ecm_index] = nir->htengysav[ecm_index] * pw_fuel_cost(ndi->htg[PRIMARY].fuel_type, ndi->rmc[rmc_index].life) +
                nir->clengysav[ecm_index] * pw_fuel_cost(ELECTRIC, ndi->rmc[rmc_index].life);

  // to this we add the baseload savings (MJF 1/01)
  nir->ecm[ecm_index].ensav = nir->htengysav[ecm_index] + nir->clengysav[ecm_index] + nir->blengysav[ecm_index];
  nir->ecm[ecm_index].dlsav = nir->htdlsav[ecm_index] + nir->cldlsav[ecm_index] + nir->bldlsav[ecm_index];
  nir->ecm[ecm_index].lifetime = ndi->rmc[rmc_index].life;

  // point our ecm[] to associated rmc[] and  vice versa
  nir->ecm[ecm_index].rmc_index = rmc_index;
  ndi->rmc[rmc_index].ecm_index = ecm_index;

  nir->ecm[ecm_index].sir = calculate_sir(nir->dslfsav[ecm_index], nir->ecm[ecm_index].cost);
  nir->npv[ecm_index] = nir->dslfsav[ecm_index] - nir->ecm[ecm_index].cost;
  return;
}

static void measure_economics_baseline(int ecm_index, int life, int fuel) {

  nir->htlcs[ecm_index] = pw_fuel_cost(fuel, life);
  nir->dslfsav[ecm_index] = nir->htlcs[ecm_index] * nir->ecm[ecm_index].ensav;
  nir->ecm[ecm_index].lifetime = life;

  nir->ecm[ecm_index].sir = calculate_sir(nir->dslfsav[ecm_index], nir->ecm[ecm_index].cost);
  nir->npv[ecm_index] = nir->dslfsav[ecm_index] - nir->ecm[ecm_index].cost;
  return;
}

// Wall insulation - Blown-in Cellulose

static void wall_insulation(void) {
  float ucav, uanew, duaw, dsaw;
  float dhld,           // Change in heating load (MMBtu)
        dcld;           // Change in cooling load (MMBtu) */
  float studsizefactor; // Accounts for cavity depth being other than 3.5 in.
  float fDepthExistIns; // Depth of existing insulation in cavity
  int rmc_index;
  int implement_flag;

  for (int comp_group_num = 1; comp_group_num < NEAT_MAX_MEASURE_NUMBER; comp_group_num++) {

    implement_flag = FALSE;
    for (int nc = 0; nc < ndi->num_wal; nc++) { // nc is base 0 window number

      if (wall_skip_insulation(nc, comp_group_num))
        continue;

      implement_flag = TRUE;

      int add_blown_insulation;
      if (strcmp(ndi->ins_wall[ndi->wal[nc].added_insulation].units, "R/in") == 0)
        add_blown_insulation = TRUE;
      else
        add_blown_insulation = FALSE;

      if (add_blown_insulation) { 
        studsizefactor = wall_stud_depth(nc) / 3.5f;

        if (ndi->wal[nc].exist_insulation == EW_POLYSTYRENE_BOARD) {   // Existing insulation board
          ASSERT(ndi->ins_wall[ndi->wal[nc].added_insulation].value > 0.0f, sprintf(msg, "Missing insulation value for wall: %s", ndi->wal[nc].code));
          nir->ecm[nir->nms].value = wall_stud_depth(nc) * ndi->ins_wall[ndi->wal[nc].added_insulation].value -
                           get_air_space_r_value(ndi->wal[nc].wall_air_space_thickness);
          if (nir->ecm[nir->nms].value < 0.0f)
            nir->ecm[nir->nms].value = 0.0f;
        } else {
          fDepthExistIns = ndi->wal[nc].exist_r / EXIST_BATT_INS_R_PER_INCH;
          ASSERT(ndi->ins_wall[ndi->wal[nc].added_insulation].value > 0.0f, sprintf(msg, "Missing insulation value for wall: %s", ndi->wal[nc].code));
          nir->ecm[nir->nms].value = (wall_stud_depth(nc) - fDepthExistIns) * ndi->ins_wall[ndi->wal[nc].added_insulation].value -
                           get_air_space_r_value(ndi->wal[nc].wall_air_space_thickness);
          if (nir->ecm[nir->nms].value < 0.0f)
            nir->ecm[nir->nms].value = 0.0f;
        }

      } else {    // added r value insulation

        studsizefactor = 1.0f;
        ASSERT(ndi->ins_wall[ndi->wal[nc].added_insulation].value > 0.0f, sprintf(msg, "Missing insulation value for wall: %s", ndi->wal[nc].code));
        nir->ecm[nir->nms].value = ndi->ins_wall[ndi->wal[nc].added_insulation].value;

      }

      ucav = 1.0f / (1.0f / ndi->wal[nc].u_cavity + nir->ecm[nir->nms].value);
      nir->ecm[nir->nms].comp_group_num = ndi->wal[nc].comp_group_num;
      nir->ecm[nir->nms].comp_group_type = MG_WALL; 
      nir->ecm[nir->nms].dwelling_component_index = nc;

      uanew = ndi->wal[nc].u_frame * FRAMING_FACTOR_WALL + ucav * (1.0f - FRAMING_FACTOR_WALL);
      uanew *= ndi->wal[nc].area;

      if (ndi->wal[nc].exposure == EX_BUFFERED) {
        uanew *= BUFFERED_TO_EXPOSED_WALL_DT_RATIO;
      }
      duaw = (ndi->wal[nc].ua_value - uanew);
      duaw = (ndi->wal[nc].ua_value - uanew);

      if (ndi->wal[nc].exposure == EX_BUFFERED) {
        dsaw = 0;
      } else {
        dsaw = duaw * WALL_ABSORPTIVITY / ndi->key.annual_outside_film_coeff;
      }

      for (int m = 1; m <= MONTHS; m++) {
        nir->dfreht[nir->nms][m] += dsaw * (cwd->solar_load[m][ndi->wal[nc].solar_orient] + cwd->solar_load[m][SOLAR_DIFFUSE]);
        nir->dua[nir->nms][m] += duaw;
      }

      add_component_code(ndi->wal[nc].code);

      rmc_index = wall_add_insulation_material(ndi->wal[nc].added_insulation);

      ASSERT(rmc_index < NMAT, sprintf(msg, "Array out of range"));
      nir->ecm[nir->nms].cost += (ndi->rmc[rmc_index].matcost + ndi->rmc[rmc_index].labcost) * studsizefactor * ndi->wal[nc].area +
                       ndi->rmc[rmc_index].itemcost + ndi->wal[nc].add_cost;

      nir->ecm[nir->nms].costum = ndi->rmc[rmc_index].matcost * studsizefactor;
      nir->ecm[nir->nms].costul = ndi->rmc[rmc_index].labcost * studsizefactor;
      nir->ecm[nir->nms].costi1 = ndi->rmc[rmc_index].itemcost;
      nir->ecm[nir->nms].qtym += ndi->wal[nc].area;
      nir->ecm[nir->nms].qtyl += ndi->wal[nc].area;
      nir->ecm[nir->nms].qtyi += 1;

      // For wall measures, the cost for the measure is reported in the ecmm structure (measure_materials or mmaterial)
      // so costi2 = 0 here
      //nir->ecm[nir->nms].typei2 = MT_ADDED_COST_FROM_AUDIT_FORM;
      nir->ecm[nir->nms].typei2 = MT_MISCELLANEOUS_SUPPLIES;
      STRCPY(nir->ecm[nir->nms].desci2, "Additional Cost");

      // new materials cost detail MJF 7/07
      add_material(ndi->wal[nc].code, wall_stud_depth(nc), comp_group_num, rmc_index, ndi->rmc[rmc_index].material, ndi->rmc[rmc_index].retrofit_type,
                  MT_INSULATION, ndi->rmc[rmc_index].units, ndi->wal[nc].area, nir->ecm[nir->nms].costum, "");
      add_material(ndi->wal[nc].code, wall_stud_depth(nc), comp_group_num, rmc_index, ndi->rmc[rmc_index].material, ndi->rmc[rmc_index].retrofit_type,
                  MT_LABOR, ndi->rmc[rmc_index].units, ndi->wal[nc].area, nir->ecm[nir->nms].costul, "");
      add_material(ndi->wal[nc].code, wall_stud_depth(nc), comp_group_num, rmc_index, ndi->rmc[rmc_index].material, ndi->rmc[rmc_index].retrofit_type,
                  MT_OTHER, "Each", 1, nir->ecm[nir->nms].costi1, "");
      // add_material(ndi->wal[nc].code, wall_stud_depth(nc), comp_group_num, N_MAT_NONE, "Additional Cost", "", 
      //             MT_ADDED_COST_FROM_AUDIT_FORM, "Each", 1, ndi->wal[nc].add_cost, "");
      //add_material(ndi->wal[nc].code, wall_stud_depth(nc), comp_group_num, N_MAT_NONE, "Additional Cost", "", 
      //            MT_MISCELLANEOUS_SUPPLIES, "Each", 1, ndi->wal[nc].add_cost, "");
      add_material(ndi->wal[nc].code, wall_stud_depth(nc), comp_group_num, N_MAT_NONE, ndi->rmc[rmc_index].material, ndi->rmc[rmc_index].retrofit_type, 
                  MT_MISCELLANEOUS_SUPPLIES, "Each", 1, ndi->wal[nc].add_cost, "");
    }
    if (implement_flag) {
      annual_energy_load_change(nir->dua[nir->nms], nir->dfreht[nir->nms], &dhld, &dcld);
      nir->ecm[nir->nms].cms_measure_num = N_CMS_WALL_INSULATION;
      nir->ecm[nir->nms].audit_section_id = N_WALL;
      nir->htengysav[nir->nms] = dhld / nir->sysht_seaseff;
      nir->clengysav[nir->nms] = dcld / ndi->clgs.avg_seer * 3.413f * ndi->clgs.fraction_cooled; // MBG 10/07

      non_zero_measure_cost(nir->ecm[nir->nms].cost, nir->ecm[nir->nms].cms_measure_num);
      measure_economics_using_rmc(nir->nms, rmc_index);
      increment_measure_counter();
    }
  }
}

static int wall_skip_insulation(int nc,  int comp_group_num) {
  if (ndi->wal[nc].comp_group_num != comp_group_num)
    return TRUE;

  return FALSE;
}

// Ceiling/attic Insulation

static void attic_insulation(float radded, int cms_measure_num) {
  float uclc,       /*  New ceiling cavity path u-value */
      uclj,         /*  New ceiling joist path u-value */
      uceil,        /*  New ceiling u-value */
      uanew,        /*  New roof/ceiling ua-value */
      duac,         /*  Difference in roof/ceiling ua-value */
      dsac,         /*  Differnece in solar aperatures */
      dhld,         /*  Change in heating load (MMBtu) */
      dcld,         /*  Change in cooling load (MMBtu) */
      farc1, farc2, /*  Sloped/Horizontal area ratios for ceil & roof */
      frfac;        /*  Framing factor */
  float rcceilc, rcinfc, rcroofc, rcsumc, rcceilco, rcsumco, uaocj, rcgable, cost;
  int rmc_index;
  float unlikely_r_value = -999.0f;

  if (ndi->num_uas == 0)
    return;

  // Note the assumption that MG_ATTIC_RESTRICTED -> MG_ATTIC_OUTER_CEILING_JOIST_SPECIFIED_REQUIRED enumerations are contiguous  MJF #349
  for (enum MEASURE_COMPONENT_GROUP_TYPE comp_group_type = MG_ATTIC_RESTRICTED; comp_group_type <= MG_ATTIC_OUTER_CEILING_JOIST_SPECIFIED_REQUIRED; comp_group_type++) {

    for (int comp_group_num = 1; comp_group_num < NEAT_MAX_MEASURE_NUMBER; comp_group_num++) {

      int implement_flag = FALSE;               // do we have at least one uas[] implemented for this comp_group_num
      float max_area = 0;                       // area in sqft of the larges uas[]
      float save_radded = unlikely_r_value;     // used to test for uniform radded for a comp_group_num, unlikely initial value
      int various_r_values_in_measure = FALSE;

      for (int nc = 0; nc < ndi->num_uas; nc++) {

        if (attic_skip_insulation(nc, comp_group_type, comp_group_num, cms_measure_num, &radded))
          continue;

        implement_flag = TRUE;

        if (cms_measure_num == N_CMS_FILL_CEILING_CAVITY) {

          if (save_radded == unlikely_r_value)    // once for each comp_group_num
            save_radded = radded;

          if (save_radded != radded) 
            various_r_values_in_measure = TRUE;

          if (ndi->uas[nc].added_R > 1.0f) {
            nir->measure_required[nir->nms] = YES;
            nir->measure_priority[nir->nms] = MPS_ENVELOPE_REQUIRED;
          } else {
            nir->measure_required[nir->nms] = NO;
            nir->measure_priority[nir->nms] = MPS_SIR;
          }
        }

        ASSERT(ndi->uas[nc].u_value, sprintf(msg, "Need non zero u value"));
        uclc = 1.0f / (1.0f / ndi->uas[nc].u_value + radded);
        uclj = ndi->uas[nc].joist_u_value;

        /* Compute new joist path U-Value including insulation over joists */

        if (ndi->uas[nc].attic_type != UAS_KNEEWALL) {
          uclj = get_ceiling_joist_u_value(nc, radded);
        }

        nir->ecm[nir->nms].comp_group_num = comp_group_num;
        nir->ecm[nir->nms].comp_group_type = ndi->uas[nc].comp_group_type;
        nir->ecm[nir->nms].dwelling_component_index = nc;
        ndi->uas[nc].various_r_values_in_measure = various_r_values_in_measure;   // #349

        farc1 = 1.0;
        farc2 = STICK_BUILT_ROOF_TO_ATTIC_AREA_RATIO;
        frfac = FRAMING_FACTOR_CEILING; /* sloped or horizontal roof */
        uaocj = 0.;

        // Include gable heat lost
        rcgable = attic_rcgable(nc);

        if (ndi->uas[nc].attic_type == UAS_KNEEWALL) {
          farc2 = 3.1623f;
          frfac = FRAMING_FACTOR_WALL;
          uaocj = KNEEWALL_JOIST_U_VALUE * 3.0f * ndi->uas[nc].area;
        }
        //if (ndi->uas[nc].attic_type == UAS_CATHEDRAL) {
        if (ndi->uas[nc].attic_type == UAS_CATHEDRAL || ndi->uas[nc].attic_type == UAS_ROOF_RAFTER) {
          farc1 = STICK_BUILT_ROOF_TO_ATTIC_AREA_RATIO;
        }

        uceil = frfac * uclj + (1.0f - frfac) * uclc;
        rcceilco = (frfac * ndi->uas[nc].joist_u_value + (1.0f - frfac) * ndi->uas[nc].u_value) * ndi->uas[nc].area * farc1;
        rcceilc = uceil * ndi->uas[nc].area * farc1;
        rcroofc = ndi->uas[nc].frame_u_value * farc2 * ndi->uas[nc].area;
        rcinfc = RHOCAIR * ndi->uas[nc].ventilation_cuft_per_hr;
        rcsumc = rcceilc + rcroofc + rcinfc + uaocj + rcgable;
        rcsumco = rcceilco + rcroofc + rcinfc + uaocj + rcgable;

        ASSERT(rcsumc, sprintf(msg, "Need non zero r value"));
        ASSERT(rcsumco, sprintf(msg, "Need non zero r value"));
        ASSERT(ndi->key.annual_outside_film_coeff, sprintf(msg, "Need non zero film coeff"));
        uanew = rcceilc * (rcinfc + rcroofc + rcgable) / rcsumc;
        duac = ndi->uas[nc].ua_value - uanew;
        dsac =
            rcroofc * ndi->uas[nc].roof_absorptance / ndi->key.annual_outside_film_coeff * (rcceilco / rcsumco - rcceilc / rcsumc);
        for (int m = 1; m <= MONTHS; m++) {
          nir->dfreht[nir->nms][m] += dsac * (cwd->solar_load[m][ndi->uas[nc].solar_orient] + cwd->solar_load[m][SOLAR_DIFFUSE]);
          nir->dua[nir->nms][m] += duac;
        }
        add_component_code(ndi->uas[nc].code);
        nir->ecm[nir->nms].cms_measure_num = cms_measure_num;

        // assign audit_section_id to the component with largest area (lions share)
        if(ndi->uas[nc].area >= max_area) {
          max_area = ndi->uas[nc].area;
          nir->ecm[nir->nms].audit_section_id = ndi->uas[nc].audit_section_id;
        }

        rmc_index = attic_insulation_material(cms_measure_num, nc, 1, &cost, radded);

        nir->ecm[nir->nms].cost += cost;
        nir->ecm[nir->nms].qtym += ndi->uas[nc].area;
        nir->ecm[nir->nms].qtyl += ndi->uas[nc].area;
        nir->ecm[nir->nms].qtyi += 1;

        additional_cost(ndi->uas[nc].cost);

        // Update the material the type for the mmaterial output if we are filling the cavity.  We
        // want to alter the material and retrofit type
        char temp_material_name[MATERIAL_LEN + 1];
        char temp_type[MATERIAL_LEN + 1];

        if (cms_measure_num == N_CMS_FILL_CEILING_CAVITY) {
          fill_ceiling_cavity_names(temp_material_name, temp_type, rmc_index, nc);
        } else {
          STRCPY(temp_material_name, ndi->rmc[rmc_index].material);
          STRCPY(temp_type,ndi->rmc[rmc_index].retrofit_type);
        }

        // new materials cost detail MJF 7/07
        //nir->ecm[nir->nms].index = nir->nms;
        add_material(ndi->uas[nc].code, 0, 0, rmc_index, temp_material_name, temp_type, 
                    MT_INSULATION, ndi->rmc[rmc_index].units, ndi->uas[nc].area, nir->ecm[nir->nms].costum, "");
        add_material(ndi->uas[nc].code, 0, 0, rmc_index, temp_material_name, temp_type, 
                    MT_LABOR, ndi->rmc[rmc_index].units, ndi->uas[nc].area, nir->ecm[nir->nms].costul, "");
        add_material(ndi->uas[nc].code, 0, 0, rmc_index, temp_material_name, temp_type, 
                    MT_OTHER, "Each", 1, nir->ecm[nir->nms].costi1, "");
        // add_material(ndi->uas[nc].code, 0, 0, N_MAT_NONE, "Added Misc Cost", "", 
        //             MT_MISCELLANEOUS_SUPPLIES, "Each", 1, ndi->uas[nc].cost, "");
        add_material(ndi->uas[nc].code, 0, 0, N_MAT_NONE, temp_material_name, temp_type, 
                    MT_MISCELLANEOUS_SUPPLIES, "Each", 1, ndi->uas[nc].cost, "");
      }   // next attic space
 
      // did any space get implemented into a measure
      if (implement_flag) {
        annual_energy_load_change(nir->dua[nir->nms], nir->dfreht[nir->nms], &dhld, &dcld);
        ASSERT(nir->sysht_seaseff, sprintf(msg, "Need non zero seasonal eff"));
        nir->htengysav[nir->nms] = dhld / nir->sysht_seaseff;
        nir->clengysav[nir->nms] = dcld / ndi->clgs.avg_seer * 3.413f * ndi->clgs.fraction_cooled; // MBG 10/07

        non_zero_measure_cost(cost, cms_measure_num);
        measure_economics_using_rmc(nir->nms, rmc_index);
        increment_measure_counter();
      }

    }   // next comp_group_num for consideration
  }   // next comp_group_type for consideration
}

static int attic_skip_insulation(int nc, enum MEASURE_COMPONENT_GROUP_TYPE comp_group_type, int comp_group_num, int cms_measure_num, float *radded) {

  // does not participate in component grouping type
  if (ndi->uas[nc].comp_group_type != comp_group_type)
    return TRUE;

  // does not participate in component grouping number
  if (ndi->uas[nc].comp_group_num != comp_group_num)
    return TRUE;
    
  if (ndi->uas[nc].attic_type == UAS_KNEEWALL) {

    // no added insulation specified
    if (ndi->uas[nc].added_kneewall == AK_NONE)
      return TRUE;

    // not evaluating the kneewalls measure
    if (cms_measure_num != N_CMS_KNEEWALL_INSULATION)
      return TRUE;

    // already have 2 or more inches of insulation
    if (ndi->uas[nc].ins_depth > 2.0f)
      return TRUE;

    *radded = ndi->ins_kneewall[ndi->uas[nc].added_kneewall].value;

  } else  {

    // no added insulation specified
    if (ndi->uas[nc].added_insulation == AA_NONE)
      return TRUE;

    // kneewalls measure not considered here
    if (cms_measure_num == N_CMS_KNEEWALL_INSULATION)
      return TRUE;

    // only in the fill ceiling cavity case do we alter the passed radded parameter
    if (cms_measure_num == N_CMS_FILL_CEILING_CAVITY) {
      if (ndi->uas[nc].added_R > 1.0f) {
        *radded = ndi->uas[nc].added_R;
      } else {
        *radded = ndi->uas[nc].max_depth * attic_ins_added_rpi(nc);
      }
    } else {
      // radded NOT reassigned from the value passed it
    }

    // #317 it is possible for max_depth to be equal to ins_depth, so no room for insulation and thus
    // the compute added R value will be zero, just skip instead of fail
    //ASSERT(*radded > 0.0f, sprintf(msg, "Missing R-value to add to attic: %s", ndi->uas[nc].code));
    if (*radded == 0.0f)
      return TRUE;

    // new insulation values exceeds the maximum allowed, silently do not consider it
    if (*radded + (attic_ins_exist_rpi(nc) * ndi->uas[nc].ins_depth) > MAX_CEILING_R_VALUE)
      return TRUE;

    ASSERT(attic_ins_added_rpi(nc), sprintf(msg, "Need non zero R per inch"));

    // There was insufficient space in attic to add insulation
    // within the specified available maximum depth, silently do not consider it
    
    if (cms_measure_num != N_CMS_FILL_CEILING_CAVITY && 
      ndi->uas[nc].max_depth < (*radded / attic_ins_added_rpi(nc)) && 
      ndi->uas[nc].added_R <= 1.0f) { 
      return TRUE; 
    }

  }

  return FALSE;   // don't skip
}

static int attic_insulation_material(int cms_measure_num, int nc, int compute_cost, float *pcost, float radd) {
  float prop;
  float cost_r11;
  int rmc_index1;
  int rmc_index2;
  int rmc_index_r11;

  if (ndi->uas[nc].attic_type == UAS_KNEEWALL){

    rmc_index1 = attic_kneewall_add_insulation_material(ndi->uas[nc].added_kneewall);

  } else if (cms_measure_num == N_CMS_FILL_CEILING_CAVITY) {

    // Setup to interpolate or extrapolate cost below R11 and above R49
    if (radd < 19.0) {
      rmc_index1 = attic_add_insulation_material(ndi->uas[nc].added_insulation, N_CMS_ATTIC_INSULATION_R11);
      rmc_index2 = attic_add_insulation_material(ndi->uas[nc].added_insulation, N_CMS_ATTIC_INSULATION_R19);
      prop = (19.0 - radd) / (19.0 - 11.0);
    } else if (radd >= 19.0 && radd < 30.0) {
      rmc_index1 = attic_add_insulation_material(ndi->uas[nc].added_insulation, N_CMS_ATTIC_INSULATION_R19);
      rmc_index2 = attic_add_insulation_material(ndi->uas[nc].added_insulation, N_CMS_ATTIC_INSULATION_R30);
      prop = (30.0 - radd) / (30.0 - 19.0);
    } else if (radd >= 30.0 && radd < 38.0) {
      rmc_index1 = attic_add_insulation_material(ndi->uas[nc].added_insulation, N_CMS_ATTIC_INSULATION_R30);
      rmc_index2 = attic_add_insulation_material(ndi->uas[nc].added_insulation, N_CMS_ATTIC_INSULATION_R38);
      prop = (38.0 - radd) / (38.0 - 30.0);
    } else if (radd >= 38.0) {
      rmc_index1 = attic_add_insulation_material(ndi->uas[nc].added_insulation, N_CMS_ATTIC_INSULATION_R38);
      rmc_index2 = attic_add_insulation_material(ndi->uas[nc].added_insulation, N_CMS_ATTIC_INSULATION_R49);
      prop = (49.0 - radd) / (49.0 - 38.0);
    } else {
      ASSERT(FALSE, sprintf(msg, "Did not compute fill cavity proportion"));
    }

    if (compute_cost == FALSE)
      return rmc_index1;

    float icostum; // to hold interpolated unit costs
    float icostul;
    float icosti1;

    icostum = ndi->rmc[rmc_index2].matcost  - prop * (ndi->rmc[rmc_index2].matcost  - ndi->rmc[rmc_index1].matcost);
    icostul = ndi->rmc[rmc_index2].labcost  - prop * (ndi->rmc[rmc_index2].labcost  - ndi->rmc[rmc_index1].labcost);
    icosti1 = ndi->rmc[rmc_index2].itemcost - prop * (ndi->rmc[rmc_index2].itemcost - ndi->rmc[rmc_index1].itemcost);

    // limit downside extrapolation to 1/2 th R11 cost
    rmc_index_r11 = attic_add_insulation_material(ndi->uas[nc].added_insulation, N_CMS_ATTIC_INSULATION_R11);
    cost_r11 = (ndi->rmc[rmc_index_r11].matcost + ndi->rmc[rmc_index_r11].labcost) * ndi->uas[nc].area + ndi->rmc[rmc_index_r11].itemcost;
    *pcost = (icostum + icostul) * ndi->uas[nc].area + icosti1 + ndi->uas[nc].cost;
    if (*pcost < cost_r11 / 2)
      *pcost = cost_r11 / 2;

    nir->ecm[nir->nms].costum = icostum;
    nir->ecm[nir->nms].costul = icostul;
    nir->ecm[nir->nms].costi1 = icosti1;
    return rmc_index1;

  } else {

    rmc_index1 = attic_add_insulation_material(ndi->uas[nc].added_insulation, cms_measure_num);

  }

  if (compute_cost == FALSE)
    return rmc_index1;

  *pcost = (ndi->rmc[rmc_index1].matcost + ndi->rmc[rmc_index1].labcost) * ndi->uas[nc].area +
           ndi->rmc[rmc_index1].itemcost + ndi->uas[nc].cost; /* Normal unfinished attic */

  nir->ecm[nir->nms].costum = ndi->rmc[rmc_index1].matcost;
  nir->ecm[nir->nms].costul = ndi->rmc[rmc_index1].labcost;
  nir->ecm[nir->nms].costi1 = ndi->rmc[rmc_index1].itemcost;
  return rmc_index1;
}

// Gable heat loss for certain attic types
float attic_rcgable(int nc) {
  if (ndi->uas[nc].attic_type == UAS_FLOORED || 
    ndi->uas[nc].attic_type == UAS_UNFLOORED ||
    ndi->uas[nc].attic_type == UAS_CEILING_JOIST) {
    return (GABLE_U_VALUE * STICK_BUILT_ROOF_PITCH * ndi->uas[nc].area / ATTIC_ASPECT_RATIO);
  }
  return 0.0f;
}

// Add white roof coating (absorptance = 0.3) to the existing roof

void white_roof_coating(int icall, int nmsi) {
  int implement_flag;
  float dtempsa, dhld, /*  Change in heating load (MMBtu) */
      dcld,            /*  Change in cooling load (MMBtu) */
      farc1;           /*  Sloped/Horizontal area ratios for ceil & roof */
  float uclj, uclc, uceil, uaocj, radded;
  float rcgable, rcceilc, rcroofc, rcinfc, rcsumc;
  float max_area = 0;
  int rmc_index;

  if (ndi->num_uas == 0)
    return;
  implement_flag = FALSE;

  if (icall == 0)
    nmsi = nir->nms;

  for (int m = 1; m <= MONTHS; m++) {
    nir->dfreht[nmsi][m] = 0.0f;
  }
  // zero out parameter due to multiple calls of coolroof routine

  for (int nc = 0; nc < ndi->num_uas; nc++) {

    if (ndi->uas[nc].attic_type == UAS_KNEEWALL)
      continue; // kneewall
    if (ndi->uas[nc].roof_color != RC_DARK)
      continue; // roof already reflective or shaded

    implement_flag = TRUE;

    radded = ndi->uas[nc].added_R;
    uclc = 1.0f / (1.0f / ndi->uas[nc].u_value + radded);
    uclj = ndi->uas[nc].joist_u_value; // or get_ceiling_joist_u_value(nc,radded)
    uceil = FRAMING_FACTOR_CEILING * uclj + (1.0f - FRAMING_FACTOR_CEILING) * uclc;
    uaocj = 0.0f;

    // Include gable heat lost
    rcgable = attic_rcgable(nc);

    farc1 = 1.0f;
    if (ndi->uas[nc].attic_type == UAS_CATHEDRAL || ndi->uas[nc].attic_type == UAS_ROOF_RAFTER)
      farc1 = STICK_BUILT_ROOF_TO_ATTIC_AREA_RATIO;

    // This use of farc-related terms is thought to be incorrect, but retained for
    // consistency with use in neat_energy_use() and attic_insulation(). See commentary of 1/18/11.

    rcceilc = uceil * ndi->uas[nc].area * farc1;
    rcroofc = ndi->uas[nc].frame_u_value * STICK_BUILT_ROOF_TO_ATTIC_AREA_RATIO * ndi->uas[nc].area;
    rcinfc = RHOCAIR * ndi->uas[nc].ventilation_cuft_per_hr;
    rcsumc = rcceilc + rcroofc + rcinfc + uaocj + rcgable;

    for (int m = 1; m <= MONTHS; m++) {
      dtempsa = rcceilc * rcroofc * (ndi->uas[nc].roof_absorptance - WHITE_ROOF_ABSORPTIVITY) / ndi->key.annual_outside_film_coeff / rcsumc;
      nir->dfreht[nmsi][m] += dtempsa * (cwd->solar_load[m][ndi->uas[nc].solar_orient] + cwd->solar_load[m][SOLAR_DIFFUSE]);
      nir->dua[nmsi][m] = 0.0f;
    }

    if (icall > 0)
      return;
    add_component_code(ndi->uas[nc].code);

    //  For material quantity and cost, modify use of farc-related terms to what
    //  is believed to be correct.  1/18/11.

    farc1 = STICK_BUILT_ROOF_TO_ATTIC_AREA_RATIO;
    if (ndi->uas[nc].attic_type == UAS_CATHEDRAL || ndi->uas[nc].attic_type == UAS_ROOF_RAFTER)
      farc1 = 1.0f;

    rmc_index = N_MAT_WHITE_ROOF_COATING;
    nir->ecm[nir->nms].dwelling_component_index = nc;
    nir->ecm[nir->nms].cost += (ndi->rmc[rmc_index].matcost + ndi->rmc[rmc_index].labcost) * ndi->uas[nc].area * farc1 +
                     ndi->rmc[rmc_index].itemcost;

    nir->ecm[nir->nms].costum = ndi->rmc[rmc_index].matcost;
    nir->ecm[nir->nms].costul = ndi->rmc[rmc_index].labcost;
    nir->ecm[nir->nms].costi1 = ndi->rmc[rmc_index].itemcost;
    nir->ecm[nir->nms].qtym += ndi->uas[nc].area * farc1;
    nir->ecm[nir->nms].qtyl += ndi->uas[nc].area * farc1;
    nir->ecm[nir->nms].qtyi += 1;

    // assign to section with lions share of the area
    if (ndi->uas[nc].area >= max_area) {
      max_area = ndi->uas[nc].area;
      nir->ecm[nir->nms].audit_section_id = ndi->uas[nc].audit_section_id;
    }

  }  // next uas[]

  if (implement_flag) {
    annual_energy_load_change(nir->dua[nir->nms], nir->dfreht[nir->nms], &dhld, &dcld);
    nir->htengysav[nir->nms] = dhld / nir->sysht_seaseff;
    nir->clengysav[nir->nms] = dcld / ndi->clgs.avg_seer * 3.413f * ndi->clgs.fraction_cooled;
    nir->ecm[nir->nms].cms_measure_num = N_CMS_WHITE_ROOF_COATING;

    non_zero_measure_cost(nir->ecm[nir->nms].cost, nir->ecm[nir->nms].cms_measure_num);
    measure_economics_using_rmc(nir->nms, rmc_index);
    increment_measure_counter();
  }
}

// Adding R-19 Batt insulation to sill

static void sill_insulation(void) {
  float duasill, duasbspc, areasill, uaflr, dhld, dcld, denm, ecmdua;
  float radded, usillins;
  int rmc_index;
  int implement_flag;

  if (ndi->num_fnd == 0)
    return;

  for (int comp_group_num = 1; comp_group_num < NEAT_MAX_MEASURE_NUMBER; comp_group_num++) {

    implement_flag = FALSE;
    ecmdua = 0.;
    for (int m = 1; m <= MONTHS; m++)
      nir->dfreht[nir->nms][m] = 0.0f;

    for (int nc = 0; nc < ndi->num_fnd; nc++) { // nc is base 0 window number

      if (sill_skip_insulation(nc, comp_group_num))
        continue;

      implement_flag = TRUE;

      nir->ecm[nir->nms].comp_group_num = comp_group_num;
      nir->ecm[nir->nms].comp_group_type = MG_FOUNDATION_SILL;
      nir->ecm[nir->nms].dwelling_component_index = nc;

      //areasill = ndi->fnd[nc].ua_value_basement_sill / SILL_U_VALUE;    // #232
      areasill = ndi->fnd[nc].sill_perimeter * ndi->fnd[nc].joist_height / 12.0f;

      ASSERT(ndi->ins_sill[ndi->fnd[nc].added_sill].value > 0.0f, sprintf(msg, "Missing insulation value for foundation sill: %s", ndi->fnd[nc].code));
      radded = ndi->ins_sill[ndi->fnd[nc].added_sill].value;
      usillins = 1.0f / (1.0f / SILL_U_VALUE + radded);
      duasill = (SILL_U_VALUE - usillins) * areasill;
      if (ndi->fnd[nc].space_type == CONDITIONED)
        duasbspc = duasill;
      else {
        uaflr = ndi->fnd[nc].area * ndi->fnd[nc].floor_u_value;
        denm = (uaflr + ndi->fnd[nc].ua_value_basement) * (uaflr + ndi->fnd[nc].ua_value_basement - duasill);
        duasbspc = uaflr * duasill * (uaflr + ndi->fnd[nc].equipment_waste_heat) / denm;
      }
      add_component_code(ndi->fnd[nc].code);
      ecmdua += duasbspc;

      rmc_index = foundation_sill_add_insulation_material(ndi->fnd[nc].added_sill);
      ASSERT(rmc_index < NMAT, sprintf(msg, "Array out of range"));
      nir->ecm[nir->nms].cost += (ndi->rmc[rmc_index].matcost + ndi->rmc[rmc_index].labcost) * areasill + ndi->rmc[rmc_index].itemcost + ndi->fnd[nc].sill_cost;

      nir->ecm[nir->nms].costum = ndi->rmc[rmc_index].matcost;
      nir->ecm[nir->nms].costul = ndi->rmc[rmc_index].labcost;
      nir->ecm[nir->nms].costi1 = ndi->rmc[rmc_index].itemcost;
      nir->ecm[nir->nms].qtym += areasill;
      nir->ecm[nir->nms].qtyl += areasill;
      nir->ecm[nir->nms].qtyi += 1;

      additional_cost(ndi->fnd[nc].sill_cost);

      // new materials cost detail MJF 7/07
      //nir->ecm[nir->nms].index = nir->nms;
      add_material(ndi->fnd[nc].code, 0, 0, rmc_index, ndi->rmc[rmc_index].material, ndi->rmc[rmc_index].retrofit_type, 
                  MT_INSULATION, ndi->rmc[rmc_index].units, areasill, nir->ecm[nir->nms].costum, "");
      add_material(ndi->fnd[nc].code, 0, 0, rmc_index, ndi->rmc[rmc_index].material, ndi->rmc[rmc_index].retrofit_type, 
                  MT_LABOR, ndi->rmc[rmc_index].units, areasill, nir->ecm[nir->nms].costul, "");
      add_material(ndi->fnd[nc].code, 0, 0, rmc_index, ndi->rmc[rmc_index].material, ndi->rmc[rmc_index].retrofit_type, 
                  MT_OTHER, "Each", 1, nir->ecm[nir->nms].costi1, "");
      // add_material(ndi->fnd[nc].code, 0, 0, N_MAT_NONE, "Added Misc Cost", "", 
      //             MT_MISCELLANEOUS_SUPPLIES, "Each", 1, ndi->fnd[nc].sill_cost, "");
      add_material(ndi->fnd[nc].code, 0, 0, N_MAT_NONE, ndi->rmc[rmc_index].material, ndi->rmc[rmc_index].retrofit_type, 
                  MT_MISCELLANEOUS_SUPPLIES, "Each", 1, ndi->fnd[nc].sill_cost, "");
    }

    if (implement_flag) {
      for (int m = 1; m <= MONTHS; m++)
        nir->dua[nir->nms][m] = ecmdua;
      annual_energy_load_change(nir->dua[nir->nms], nir->dfreht[nir->nms], &dhld, &dcld);
      nir->htengysav[nir->nms] = dhld / nir->sysht_seaseff;
      nir->clengysav[nir->nms] = dcld / ndi->clgs.avg_seer * 3.413f * ndi->clgs.fraction_cooled; // MBG 10/07
      nir->ecm[nir->nms].cms_measure_num = N_CMS_SILLBOX_INSULATION;
      nir->ecm[nir->nms].audit_section_id = N_FOUNDATION;

      non_zero_measure_cost(nir->ecm[nir->nms].cost, nir->ecm[nir->nms].cms_measure_num);
      measure_economics_using_rmc(nir->nms, rmc_index);
      increment_measure_counter();
    }
  }
}

static int sill_skip_insulation(int nc, int comp_group_num) {

  // does not participate in component grouping number
  if (ndi->fnd[nc].comp_group_num != comp_group_num)
    return TRUE;

  if (ndi->fnd[nc].added_sill == AS_NONE)
    return TRUE;

  // not a foundation type that bennefits from sill insulation
  if (ndi->fnd[nc].space_type != CONDITIONED && 
      ndi->fnd[nc].space_type != NON_CONDITIONED &&
      ndi->fnd[nc].space_type != UNINTENTIONALLY_CONDITIONED)
    return TRUE;

  // Can't ins. sill if height < 2 ft 
  if (ndi->fnd[nc].wall_height < 2.0f)
    return TRUE;

  // already well insulated
  if (ndi->fnd[nc].ua_value_basement_sill < 0.01f)
    return TRUE;

  return FALSE;
}

// Add foundation insulation to subspace walls

static void foundation_wall_insulation(void) {
  int implement_flag;
  float fbgr, duafla, duaflb, duabsmt, dhld, dcld, uaflr, uabsmtnew, uaeffn, ecmdua, fndinsr;
  int rmc_index;

  if (ndi->num_fnd == 0)
    return;

  for (int comp_group_num = 1; comp_group_num < NEAT_MAX_MEASURE_NUMBER; comp_group_num++) {

    implement_flag = FALSE;
    ecmdua = 0;
    for (int nc = 0; nc < ndi->num_fnd; nc++) {

      if (foundation_wall_skip_insulation(nc, comp_group_num))
        continue;

      implement_flag = TRUE;
      ASSERT(ndi->ins_foundation[ndi->fnd[nc].added_found].value > 0.0f, sprintf(msg, "Missing insulation value for foundation: %s", ndi->fnd[nc].code));
      fndinsr = ndi->ins_foundation[ndi->fnd[nc].added_found].value;

      nir->ecm[nir->nms].comp_group_num = comp_group_num;
      nir->ecm[nir->nms].comp_group_type = MG_FOUNDATION_WALL;
      nir->ecm[nir->nms].dwelling_component_index = nc;

      nir->ecm[nir->nms].value = fndinsr;
      fbgr = ndi->fnd[nc].below_grade_wall_height / ndi->fnd[nc].wall_height;
      duafla = ndi->fnd[nc].above_grade_wall_u_value * ndi->fnd[nc].wall_area * (1.0f - fbgr) *
               ndi->fnd[nc].above_grade_wall_u_value * fndinsr / (1.0f + ndi->fnd[nc].above_grade_wall_u_value * fndinsr);
      duaflb = ndi->fnd[nc].u_value_basement_wall_total * ndi->fnd[nc].wall_area * fbgr *
               ndi->fnd[nc].u_value_basement_wall_total * fndinsr / (1.0f + ndi->fnd[nc].u_value_basement_wall_total * fndinsr);
      /*    duasill=ndi->fnd[nc].ua_value_basement_sill*SILL_U_VALUEINS*fndinsr/(1.0f+SILL_U_VALUEINS*fndinsr); */
      /*    duabsmt = duafla+duaflb+duasill; */
      duabsmt = duafla + duaflb;
      /*    finsarea=ndi->fnd[nc].wall_area + ndi->fnd[nc].ua_value_basement_sill/SILL_U_VALUEINS; */
      if (ndi->fnd[nc].space_type == CONDITIONED)
        ecmdua += duabsmt;
      else {
        uabsmtnew = ndi->fnd[nc].ua_value_basement - duabsmt;
        uaflr = ndi->fnd[nc].area * ndi->fnd[nc].floor_u_value;
        uaeffn = uaflr * (uabsmtnew - ndi->fnd[nc].equipment_waste_heat) / (uaflr + uabsmtnew);
        ecmdua += ndi->fnd[nc].ua_value_basement_effective - uaeffn;
      }
      add_component_code(ndi->fnd[nc].code);

      rmc_index = foundation_wall_add_insulation_material(ndi->fnd[nc].added_found);
      ASSERT(rmc_index < NMAT, sprintf(msg, "Array out of range"));
      nir->ecm[nir->nms].cost += (ndi->rmc[rmc_index].matcost + ndi->rmc[rmc_index].labcost) * ndi->fnd[nc].wall_area +
                       ndi->rmc[rmc_index].itemcost + ndi->fnd[nc].wall_cost;

      nir->ecm[nir->nms].costum = ndi->rmc[rmc_index].matcost;
      nir->ecm[nir->nms].costul = ndi->rmc[rmc_index].labcost;
      nir->ecm[nir->nms].costi1 = ndi->rmc[rmc_index].itemcost;
      nir->ecm[nir->nms].qtym += ndi->fnd[nc].wall_area;
      nir->ecm[nir->nms].qtyl += ndi->fnd[nc].wall_area;
      nir->ecm[nir->nms].qtyi += 1;

      additional_cost(ndi->fnd[nc].wall_cost);

      // new materials cost detail MJF 7/07
      //nir->ecm[nir->nms].index = nir->nms;
      add_material(ndi->fnd[nc].code, 0, 0, rmc_index, ndi->rmc[rmc_index].material, ndi->rmc[rmc_index].retrofit_type, 
                  MT_INSULATION, ndi->rmc[rmc_index].units, ndi->fnd[nc].wall_area, nir->ecm[nir->nms].costum, "");
      add_material(ndi->fnd[nc].code, 0, 0, rmc_index, ndi->rmc[rmc_index].material, ndi->rmc[rmc_index].retrofit_type, 
                  MT_LABOR, ndi->rmc[rmc_index].units, ndi->fnd[nc].wall_area, nir->ecm[nir->nms].costul, "");
      add_material(ndi->fnd[nc].code, 0, 0, rmc_index, ndi->rmc[rmc_index].material, ndi->rmc[rmc_index].retrofit_type, 
                  MT_OTHER, "Each", 1, nir->ecm[nir->nms].costi1, "");
      // add_material(ndi->fnd[nc].code, 0, 0, N_MAT_NONE, "Added Misc Cost", "", 
      //             MT_MISCELLANEOUS_SUPPLIES, "Each", 1, ndi->fnd[nc].wall_cost, "");
      add_material(ndi->fnd[nc].code, 0, 0, N_MAT_NONE, ndi->rmc[rmc_index].material, ndi->rmc[rmc_index].retrofit_type, 
                  MT_MISCELLANEOUS_SUPPLIES, "Each", 1, ndi->fnd[nc].wall_cost, "");
    }

    if (implement_flag) {
      for (int m = 1; m <= MONTHS; m++)
        nir->dua[nir->nms][m] = ecmdua;
      annual_energy_load_change(nir->dua[nir->nms], nir->dfreht[nir->nms], &dhld, &dcld);
      nir->htengysav[nir->nms] = dhld / nir->sysht_seaseff;
      nir->clengysav[nir->nms] = dcld / ndi->clgs.avg_seer * 3.413f * ndi->clgs.fraction_cooled; // MBG 10/07
      nir->ecm[nir->nms].cms_measure_num = N_CMS_FOUNDATION_WALL_INSULATION;
      nir->ecm[nir->nms].audit_section_id = N_FOUNDATION;

      non_zero_measure_cost(nir->ecm[nir->nms].cost, nir->ecm[nir->nms].cms_measure_num);
      measure_economics_using_rmc(nir->nms, rmc_index);
      increment_measure_counter();
    }
  }
}

static int foundation_wall_skip_insulation(int nc, int comp_group_num) {

  // does not participate in component grouping number
  if (ndi->fnd[nc].comp_group_num != comp_group_num)
    return TRUE;

  if (ndi->fnd[nc].added_found == AF_NONE)
    return TRUE;

  if (ndi->fnd[nc].space_type == EXPOSED_FLOOR_CLOSED || 
    ndi->fnd[nc].space_type == EXPOSED_FLOOR_UNCLOSED ||
    ndi->fnd[nc].space_type == UNINSULATED_SLAB ||
    ndi->fnd[nc].space_type == INSULATED_SLAB || 
    ndi->fnd[nc].space_type == VENTED_NON_CONTITIONED)
    return TRUE;

  if (ndi->fnd[nc].wall_height < 2.0)
    return TRUE;

  return FALSE;
}

//Adding batt insulation to floors

static void floor_insulation(float added_r, int cms_measure_num) {
  float zeta, uaeffn, uaflrnew, dhld, dcld, ecmdua;
  int rmc_index;
  int implement_flag;

  if (ndi->num_fnd == 0)
    return;

  for (int comp_group_num = 1; comp_group_num < NEAT_MAX_MEASURE_NUMBER; comp_group_num++) {

    implement_flag = FALSE;
    ecmdua = 0.;

    for (int nc = 0; nc < ndi->num_fnd; nc++) {

      if (floor_skip_insulation(nc, cms_measure_num, comp_group_num, added_r))
        continue;

      implement_flag = TRUE;

      nir->ecm[nir->nms].comp_group_num = comp_group_num;
      nir->ecm[nir->nms].comp_group_type = MG_FOUNDATION_FLOOR;
      nir->ecm[nir->nms].dwelling_component_index = nc;

      add_component_code(ndi->fnd[nc].code);

      if (cms_measure_num == N_CMS_FILL_FLOOR_CAVITY) {

        switch (ndi->fnd[nc].added_floor) {
          case AL_CELLULOSE_BLOWN:
            rmc_index = N_MAT_FLOOR_FILL_CELLULOSE;
            break;
          case AL_FIBERGLASS_BLOWN:
            rmc_index = N_MAT_FLOOR_FILL_FIBERGLASS;
            break;
          case AL_TYPE4:
            rmc_index = N_MAT_FLOOR_FILL_UT4;
            break;
          case AL_TYPE5:
            rmc_index = N_MAT_FLOOR_FILL_UT5;
            break;
          case AL_TYPE6:
            rmc_index = N_MAT_FLOOR_FILL_UT6;
            break;
          default:
            ASSERT(FALSE, sprintf(msg, "Incorrect enclosed floor insulation type: %d for:%s", ndi->fnd[nc].added_floor, ndi->fnd[nc].code));
        }

        // compute the R value either directly or from joist dimension and R/inch for specified insulation type
        // assume that the whole cavity gets the R/in of the retrofit material since existing material is assumed
        // degraded or partial

        added_r = ndi->fnd[nc].joist_height * ndi->ins_floor[ndi->fnd[nc].added_floor].value;   // floor types are R/in

        if (ndi->fnd[nc].space_type == EXPOSED_FLOOR_CLOSED)
          added_r -= 1.0f; /* replaces R-1 air space in exp flrs */

      } else {
        // use the added_r passed in
        rmc_index = floor_add_insulation_material(ndi->fnd[nc].added_floor, cms_measure_num);
      }

      nir->ecm[nir->nms].value = added_r;

      //  Compute the difference in the framing path R-value caused by a change in
      //  the effective joist depth

      nir->ecm[nir->nms].value2 = added_r / (ndi->ins_floor[ndi->fnd[nc].added_floor].value) * FRAMING_R_PER_IN;

      uaflrnew = FLOOR_FRAMING_RATIO / (ndi->fnd[nc].floor_framing_r + nir->ecm[nir->nms].value2) +
                 (1.0f - FLOOR_FRAMING_RATIO) / (ndi->fnd[nc].floor_cavity_r + nir->ecm[nir->nms].value);
      uaflrnew *= ndi->fnd[nc].area;
      if (ndi->fnd[nc].space_type == EXPOSED_FLOOR_CLOSED ||
        ndi->fnd[nc].space_type == EXPOSED_FLOOR_UNCLOSED)
        zeta = -uaflrnew;   // no equipment waste heat
      else
        zeta = ndi->fnd[nc].equipment_waste_heat;
      uaeffn = uaflrnew * (ndi->fnd[nc].ua_value_basement - zeta) / (uaflrnew + ndi->fnd[nc].ua_value_basement);
      ecmdua += ndi->fnd[nc].ua_value_basement_effective - uaeffn;

      nir->ecm[nir->nms].cost += (ndi->rmc[rmc_index].matcost + ndi->rmc[rmc_index].labcost) * ndi->fnd[nc].area +
                       ndi->rmc[rmc_index].itemcost + ndi->fnd[nc].floor_cost;

      nir->ecm[nir->nms].costum = ndi->rmc[rmc_index].matcost;
      nir->ecm[nir->nms].costul = ndi->rmc[rmc_index].labcost;
      nir->ecm[nir->nms].costi1 = ndi->rmc[rmc_index].itemcost;

      nir->ecm[nir->nms].qtym += ndi->fnd[nc].area;
      nir->ecm[nir->nms].qtyl += ndi->fnd[nc].area;
      nir->ecm[nir->nms].qtyi += 1;

      additional_cost(ndi->fnd[nc].floor_cost);

      // new materials cost detail MJF 7/07
      //nir->ecm[nir->nms].index = nir->nms;
      add_material(ndi->fnd[nc].code, 0, 0, rmc_index, ndi->rmc[rmc_index].material, ndi->rmc[rmc_index].retrofit_type, 
                MT_INSULATION, ndi->rmc[rmc_index].units, ndi->fnd[nc].area, nir->ecm[nir->nms].costum, "");
      add_material(ndi->fnd[nc].code, 0, 0, rmc_index, ndi->rmc[rmc_index].material, ndi->rmc[rmc_index].retrofit_type, 
                MT_LABOR, ndi->rmc[rmc_index].units, ndi->fnd[nc].area, nir->ecm[nir->nms].costul, "");
      add_material(ndi->fnd[nc].code, 0, 0, rmc_index, ndi->rmc[rmc_index].material, ndi->rmc[rmc_index].retrofit_type, 
                MT_OTHER, "Each", 1, nir->ecm[nir->nms].costi1, "");
      // add_material(ndi->fnd[nc].code, 0, 0, N_MAT_NONE, "Added Misc Cost", "", 
      //           MT_MISCELLANEOUS_SUPPLIES, "Each", 1, ndi->fnd[nc].floor_cost, "");
      add_material(ndi->fnd[nc].code, 0, 0, N_MAT_NONE, ndi->rmc[rmc_index].material, ndi->rmc[rmc_index].retrofit_type, 
                MT_MISCELLANEOUS_SUPPLIES, "Each", 1, ndi->fnd[nc].floor_cost, "");
    }

    if (implement_flag) {
      for (int m = 1; m <= MONTHS; m++)
        nir->dua[nir->nms][m] = ecmdua;
      annual_energy_load_change(nir->dua[nir->nms], nir->dfreht[nir->nms], &dhld, &dcld);
      nir->htengysav[nir->nms] = dhld / nir->sysht_seaseff;
      nir->clengysav[nir->nms] = dcld / ndi->clgs.avg_seer * 3.413f * ndi->clgs.fraction_cooled; // MBG 10/07
      nir->ecm[nir->nms].cms_measure_num = cms_measure_num;
      nir->ecm[nir->nms].audit_section_id = N_FOUNDATION;

      non_zero_measure_cost(nir->ecm[nir->nms].cost, nir->ecm[nir->nms].cms_measure_num);
      measure_economics_using_rmc(nir->nms, rmc_index);
      increment_measure_counter();
    }
  }
}

static int floor_skip_insulation(int nc, int cms_measure_num, int comp_group_num, float added_r) {

  // not in right group number
  if (ndi->fnd[nc].comp_group_num != comp_group_num)
    return TRUE;

  // no added insulation called for
  if (ndi->fnd[nc].added_floor == AL_NONE)
    return TRUE;

  // must be one of the foundation types that will bennefit
  if (ndi->fnd[nc].space_type != NON_CONDITIONED &&
      ndi->fnd[nc].space_type != VENTED_NON_CONTITIONED &&
      ndi->fnd[nc].space_type != UNINTENTIONALLY_CONDITIONED &&
      ndi->fnd[nc].space_type != EXPOSED_FLOOR_CLOSED &&
      ndi->fnd[nc].space_type != EXPOSED_FLOOR_UNCLOSED)
    return TRUE;

  // the right measure and insulation type
  if (cms_measure_num == N_CMS_FILL_FLOOR_CAVITY &&
    !(ndi->fnd[nc].added_floor == AL_CELLULOSE_BLOWN || 
      ndi->fnd[nc].added_floor == AL_FIBERGLASS_BLOWN ||
      ndi->fnd[nc].added_floor == AL_TYPE4 ||
      ndi->fnd[nc].added_floor == AL_TYPE5 ||
      ndi->fnd[nc].added_floor == AL_TYPE6))
    return TRUE;

  if (ndi->fnd[nc].space_type == EXPOSED_FLOOR_CLOSED) {
    if (cms_measure_num != N_CMS_FILL_FLOOR_CAVITY)
      return TRUE;
  } else {
    // Maximum R-Value = 38 
    if (ndi->fnd[nc].flr_ins_r + added_r > 38.01)
      return TRUE; 

    //  Prevent added insulation from extending below the floor joists.  4/11 MBG
    //  Existing insulation depth (assuming fiberglass batts) + added insulation
    //  depth <= joist height.

    ASSERT(ndi->ins_floor[ndi->fnd[nc].added_floor].value > 0.0f, sprintf(msg, "Missing insulation value for floor: %s", ndi->fnd[nc].code));
    if (ndi->fnd[nc].flr_ins_r / ndi->ins_floor[AL_FIBERGLASS_BATT].value +
            added_r / (ndi->ins_floor[ndi->fnd[nc].added_floor].value) >
        ndi->fnd[nc].joist_height)
      return TRUE;   // not enough room for added_r plus existing r (assumed to be fiberglass batt)
  }
  return FALSE;
}

// Add storm windows to all windows without storms

static void window_storm(void) {
  float unew, duaw, leakage_savings_factor, Effemit, dhld, /*  Change in heating load (MMBtu)  */
      dcld,                               /*  Change in cooling load (MMBtu)  */
      lata,                               /*  Latent load of affected windows after retrofit */
      latb,                               /*  Latent load of affected windows before retrofit  */
      addedr;                             /*  Computed added R-Value due to storm  */
  /*  value2      Correction for dcld = latent load after
                   installation of the measure   */
  float dsaw_winter, dsaw_summer;
  int rmc_index;

  addedr = 0.96f;
  if (ndi->key.storm_window_inside_emittance > 0.0f) {
    ASSERT(ndi->key.storm_window_inside_emittance, sprintf(msg, "Need non zero emittance"));
    Effemit = 1.0f * (1.0f / 0.84f + 1.0f / ndi->key.storm_window_inside_emittance - 1.0f);
    ASSERT(Effemit, sprintf(msg, "Need non zero emittance"));
    Effemit = 1.0f / Effemit;
    addedr = 2.4f * (float)exp(-1.5f * Effemit + 0.1f);
  } // Curve fit to ASHREA data
  for (int nc = 0; nc < ndi->num_win; nc++) {
    lata = latb = 0.f;
    if (ndi->win[nc].retrofit_option != WS_EVALUATE_ALL && ndi->win[nc].retrofit_option != WS_ADD_STORM)
      continue;
    // Storms not requested because we have an existing good storm window
    if (ndi->win[nc].glazing_type == SINGLE_W_WOOD_STORM || ndi->win[nc].glazing_type == SINGLE_W_METAL_STORM)
      continue; /* Storm already on windw */
    // if (ndi->win[nc].retrofit_option == WS_ADD_STORM && wdwmand == 1) {
    if (ndi->win[nc].retrofit_option == WS_ADD_STORM) {
      nir->measure_required[nir->nms] = YES;
      nir->measure_priority[nir->nms] = MPS_ENVELOPE_REQUIRED; // Measure considered required
      if (ndi->win[nc].inc_sir != YES)
        nir->measure_priority[nir->nms] = MPS_ENVELOPE_REQUIRED_NO_SIR;
    }
    nir->ecm[nir->nms].dwelling_component_index = nc;
    unew = 1.0f / (addedr + 1.0f / ndi->win[nc].u_value);
    duaw = (ndi->win[nc].u_value - unew) * ndi->win[nc].area_gross;

    // Allow user to specify SHGC of storm window. Also convert to using SHGC for entire window, not just glazing
    dsaw_winter = ndi->win[nc].area_gross * ndi->win[nc].shgc_winter * (1.0f - ndi->key.storm_window_shgc);
    dsaw_summer = ndi->win[nc].area_gross * ndi->win[nc].shgc_summer * (1.0f - ndi->key.storm_window_shgc);

    leakage_savings_factor = window_storm_leak_coef(ndi->win[nc].leak_coef) / ndi->win[nc].leak_coef;

    for (int m = 1; m <= MONTHS; m++) {
      nir->dua[nir->nms][m] += duaw + 60.f * RHOCAIR * nir->wn_leak_cfm[nc][m] * (1.0f - leakage_savings_factor);
      lata += get_latent_infil_load(cwd->avg_drybulb_temp[m], cwd->avg_wetbulb_temp[m], leakage_savings_factor * nir->wn_leak_cfm[nc][m]) * 24.0f * cwd->days_in_month[m];
      latb += get_latent_infil_load(cwd->avg_drybulb_temp[m], cwd->avg_wetbulb_temp[m], nir->wn_leak_cfm[nc][m]) * 24.0f * cwd->days_in_month[m];
      if (m > MARCH && m < OCTOBER)
        nir->dfreht[nir->nms][m] += dsaw_summer * (cwd->solar_load[m][ndi->win[nc].solar_orient] * ndi->win[nc].shade_factor_summer +
                                         cwd->solar_load[m][SOLAR_DIFFUSE]);
      else
        nir->dfreht[nir->nms][m] += dsaw_winter * (cwd->solar_load[m][ndi->win[nc].solar_orient] * ndi->win[nc].shade_factor_winter +
                                         cwd->solar_load[m][SOLAR_DIFFUSE]);
    }
    add_component_code(ndi->win[nc].code);

    rmc_index = N_MAT_STORM_WINDOWS;
    nir->ecm[nir->nms].cost += (ndi->rmc[rmc_index].matcost + ndi->rmc[rmc_index].labcost) * ndi->win[nc].area_gross +
                     ndi->rmc[rmc_index].itemcost * ndi->win[nc].number;

    nir->ecm[nir->nms].costum = ndi->rmc[rmc_index].matcost;
    nir->ecm[nir->nms].costul = ndi->rmc[rmc_index].labcost;
    nir->ecm[nir->nms].costi1 = ndi->rmc[rmc_index].itemcost;
    nir->ecm[nir->nms].qtym += ndi->win[nc].area_gross;
    nir->ecm[nir->nms].qtyl += ndi->win[nc].area_gross;
    nir->ecm[nir->nms].qtyi += ndi->win[nc].number; // each window not area

    // Add added cost from input

    nir->ecm[nir->nms].cost += ndi->win[nc].cost_add_storm * ndi->win[nc].number;
    additional_cost(ndi->win[nc].cost_add_storm * ndi->win[nc].number);

    annual_energy_load_change(nir->dua[nir->nms], nir->dfreht[nir->nms], &dhld, &dcld);
    nir->ecm[nir->nms].value2 = (float)((latb - lata) / 1.e6 * ndi->clgs.fraction_cooled);
    nir->ecm[nir->nms].value = addedr;
    dcld += nir->ecm[nir->nms].value2;
    nir->htengysav[nir->nms] = dhld / nir->sysht_seaseff;
    nir->clengysav[nir->nms] = dcld / ndi->clgs.avg_seer * 3.413f * ndi->clgs.fraction_cooled; // MBG 10/07
    nir->ecm[nir->nms].cms_measure_num = N_CMS_STORM_WINDOWS;
    nir->ecm[nir->nms].audit_section_id = N_WINDOW;

    non_zero_measure_cost(nir->ecm[nir->nms].cost, nir->ecm[nir->nms].cms_measure_num);
    measure_economics_using_rmc(nir->nms, rmc_index);
    increment_measure_counter();
  }
}

// Install low-e windows to all single pane windows without storms, or
// windows having missing glazing or single with bad storm

static void window_low_e(void) {
  float unew, duaw, dsaw, leakage_savings_factor, dhld, dcld;
  float lata, /*  Latent load of affected windows after retrofit */
      latb;   /*  Latent load of affected windows before retrofit  */
  /*  value2      Correction for dcld = latent load after
                   installation of the measure   */
  int rmc_index;

  for (int nc = 0; nc < ndi->num_win; nc++) {
    lata = latb = 0.f;
    if (ndi->win[nc].retrofit_option != WS_EVALUATE_ALL && ndi->win[nc].retrofit_option != WS_REPLACE_LOWE)
      continue;
    // Replace not requested
    // if (ndi->win[nc].retrofit_option == WS_REPLACE_LOWE && wdwmand == 1) {
    if (ndi->win[nc].retrofit_option == WS_REPLACE_LOWE) {
      nir->measure_required[nir->nms] = YES;
      nir->measure_priority[nir->nms] = MPS_ENVELOPE_REQUIRED; // Measure considered required
      if (ndi->win[nc].inc_sir != YES)
        nir->measure_priority[nir->nms] = MPS_ENVELOPE_REQUIRED_NO_SIR;
    }
    nir->ecm[nir->nms].dwelling_component_index = nc;
    //    unew = 1.0f/LOWEWDWR;  2/12/09
    unew = ndi->key.new_lowe_window_u_value;
    duaw = (ndi->win[nc].u_value - unew) * ndi->win[nc].area_gross;
    //    dsaw = (ndi->win[nc].shgc_summer-sgfscreen[3])*ndi->win[nc].area_gross;
    // Changed to use of whole window area for solar aperture, 2/09
    dsaw = (ndi->win[nc].shgc_summer - ndi->key.new_lowe_window_shgc) * ndi->win[nc].area_gross;
    // Changed to use setup values for window characteristics  5/09

    leakage_savings_factor = WINDOW_MEC_LEAKAGE_COEF / ndi->win[nc].leak_coef;
    for (int m = 1; m <= MONTHS; m++) {
      if (m > MARCH && m < OCTOBER)
        nir->dfreht[nir->nms][m] += dsaw * (cwd->solar_load[m][ndi->win[nc].solar_orient] * ndi->win[nc].shade_factor_summer +
                                  cwd->solar_load[m][SOLAR_DIFFUSE]);
      else
        nir->dfreht[nir->nms][m] += dsaw * (cwd->solar_load[m][ndi->win[nc].solar_orient] * ndi->win[nc].shade_factor_winter +
                                  cwd->solar_load[m][SOLAR_DIFFUSE]);
      nir->dua[nir->nms][m] += duaw + 60.f * RHOCAIR * nir->wn_leak_cfm[nc][m] * (1.0f - leakage_savings_factor);
      lata += get_latent_infil_load(cwd->avg_drybulb_temp[m], cwd->avg_wetbulb_temp[m], leakage_savings_factor * nir->wn_leak_cfm[nc][m]) * 24.0f * cwd->days_in_month[m];
      latb += get_latent_infil_load(cwd->avg_drybulb_temp[m], cwd->avg_wetbulb_temp[m], nir->wn_leak_cfm[nc][m]) * 24.0f * cwd->days_in_month[m];
    }
    add_component_code(ndi->win[nc].code);

    rmc_index = N_MAT_LOW_E_WINDOWS;
    nir->ecm[nir->nms].cost += (ndi->rmc[rmc_index].matcost + ndi->rmc[rmc_index].labcost) * ndi->win[nc].area_gross +
                     ndi->rmc[rmc_index].itemcost * ndi->win[nc].number;

    nir->ecm[nir->nms].costum = ndi->rmc[rmc_index].matcost;
    nir->ecm[nir->nms].costul = ndi->rmc[rmc_index].labcost;
    nir->ecm[nir->nms].costi1 = ndi->rmc[rmc_index].itemcost;
    nir->ecm[nir->nms].qtym += ndi->win[nc].area_gross;
    nir->ecm[nir->nms].qtyl += ndi->win[nc].area_gross;
    nir->ecm[nir->nms].qtyi += ndi->win[nc].number; // each window

    // Add added cost from input

    nir->ecm[nir->nms].cost += ndi->win[nc].cost_low_e * ndi->win[nc].number;
    additional_cost(ndi->win[nc].cost_low_e * ndi->win[nc].number);

    annual_energy_load_change(nir->dua[nir->nms], nir->dfreht[nir->nms], &dhld, &dcld);
    nir->ecm[nir->nms].value2 = (float)((latb - lata) / 1.e6 * ndi->clgs.fraction_cooled);
    dcld += nir->ecm[nir->nms].value2;
    nir->htengysav[nir->nms] = dhld / nir->sysht_seaseff;
    nir->clengysav[nir->nms] = dcld / ndi->clgs.avg_seer * 3.413f * ndi->clgs.fraction_cooled; // MBG 10/07
    nir->ecm[nir->nms].cms_measure_num = N_CMS_LOW_E_WINDOWS;
    nir->ecm[nir->nms].audit_section_id = N_WINDOW;

    non_zero_measure_cost(nir->ecm[nir->nms].cost, nir->ecm[nir->nms].cms_measure_num);
    measure_economics_using_rmc(nir->nms, rmc_index);
    increment_measure_counter();
  }
}

// Add window shading to all windows not facing north, previously shaded
// less than 50% 

static void window_shade(void) {
  int implement_flag;
  float dsaws, dsaww, dhld, /*  Change in heating load (MMBtu) */
      dcld;                 /*  Change in cooling load (MMBtu) */
  int rmc_index;

  implement_flag = FALSE;
  for (int nc = 0; nc < ndi->num_win; nc++) {
    if (window_not_suitable_for_shading_retrofit(nc))
      continue;
    /* Window not north facing or with summer shade < 50%  */
    implement_flag = TRUE;
    //     dsaww = (ndi->win[nc].shade_factor_winter*0.2f)*ndi->win[nc].shgc_winter*ndi->win[nc].area_glazing;  Use whole wdw
    //     SHGC, 2/09
    //     dsaws = (ndi->win[nc].shade_factor_summer - 0.1f)*ndi->win[nc].shgc_summer*ndi->win[nc].area_glazing;
    dsaww = (ndi->win[nc].shade_factor_winter * 0.2f) * ndi->win[nc].shgc_winter * ndi->win[nc].area_gross;
    dsaws = (ndi->win[nc].shade_factor_summer - 0.1f) * ndi->win[nc].shgc_summer * ndi->win[nc].area_gross;

    for (int m = 1; m <= MONTHS; m++) {
      if (m > MARCH && m < OCTOBER)
        nir->dfreht[nir->nms][m] += dsaws * (cwd->solar_load[m][ndi->win[nc].solar_orient]);
      else
        nir->dfreht[nir->nms][m] += dsaww * (cwd->solar_load[m][ndi->win[nc].solar_orient]);
      nir->dua[nir->nms][m] = 0.0;
    }
    add_component_code(ndi->win[nc].code);

    rmc_index = N_MAT_WINDOW_SHADING_AWNING;
    nir->ecm[nir->nms].dwelling_component_index = nc;
    nir->ecm[nir->nms].cost += ((ndi->rmc[rmc_index].matcost + ndi->rmc[rmc_index].labcost) * ndi->win[nc].width / 12.0f +
                      ndi->rmc[rmc_index].itemcost) *
                     ndi->win[nc].number;

    nir->ecm[nir->nms].costum = ndi->rmc[rmc_index].matcost;
    nir->ecm[nir->nms].costul = ndi->rmc[rmc_index].labcost;
    nir->ecm[nir->nms].costi1 = ndi->rmc[rmc_index].itemcost;
    nir->ecm[nir->nms].qtym += (ndi->win[nc].width / 12.0f) * ndi->win[nc].number;
    nir->ecm[nir->nms].qtyl += (ndi->win[nc].width / 12.0f) * ndi->win[nc].number;
    nir->ecm[nir->nms].qtyi += ndi->win[nc].number; // each awning

  }

  if (implement_flag) {
    annual_energy_load_change(nir->dua[nir->nms], nir->dfreht[nir->nms], &dhld, &dcld);
    nir->htengysav[nir->nms] = dhld / nir->sysht_seaseff;
    nir->clengysav[nir->nms] = dcld / ndi->clgs.avg_seer * 3.413f * ndi->clgs.fraction_cooled; // MBG 10/07
    nir->ecm[nir->nms].cms_measure_num = N_CMS_WINDOW_SHADING_AWNING;
    nir->ecm[nir->nms].audit_section_id = N_WINDOW;

    non_zero_measure_cost(nir->ecm[nir->nms].cost, nir->ecm[nir->nms].cms_measure_num);
    measure_economics_using_rmc(nir->nms, rmc_index);
    increment_measure_counter();
  }
}

// Add sun screens to all windows not facing north, previously shaded
// less than 50%  Assume the screens are on only during cooling months.

static void window_screen(int cms_measure_num) {
  int implement_flag;
  float dsaws, dsaww, dhld, dcld, ecmdua = 0.0f, filmSHGC;
  int rmc_index;

  implement_flag = FALSE;
  for (int nc = 0; nc < ndi->num_win; nc++) {
    if (window_not_suitable_for_shading_retrofit(nc))
      continue;
    /* Window not north facing or with summer shade < 50%  */
    implement_flag = TRUE;
    dsaww = 0.;
    //        Changes to use whole window area and shading coefficients. 2/09
    //     dsaws = (ndi->win[nc].shgc_summer-sgfscreen[type])*ndi->win[nc].area_gross;
    dsaws = (nir->wn_sunscrn[COOLING][nc] - window_treatment_shgc(cms_measure_num)) * ndi->win[nc].shgc_summer * ndi->win[nc].area_gross;
    switch (cms_measure_num) {
    case N_CMS_SUN_SCREEN_FABRIC:
    case N_CMS_SUN_SCREEN_LOUVERED:
      if (nir->screens_removed_for_winter == NO) {
        //                 dsaww = (ndi->win[nc].shgc_winter-sgfscreen[type])*ndi->win[nc].area_gross;
        dsaww = (nir->wn_sunscrn[HEATING][nc] - window_treatment_shgc(cms_measure_num)) * ndi->win[nc].shgc_winter * ndi->win[nc].area_gross;
        //                 ecmdua = 0.1f * ndi->win[nc].u_value * ndi->win[nc].area_gross;
        ecmdua = 0.0f;
      }
      break;
    case N_CMS_WINDOW_FILM:
      filmSHGC = ndi->key.window_film_shgc;
      if (ndi->win[nc].glazing_type != SINGLE && ndi->win[nc].glazing_type != SINGLE_W_BAD_STORM) {
        filmSHGC = 1.1f * ndi->key.window_film_shgc;
      }
      // Film industry said SHGC 10% greater for 2-pane wdws. 10/09
      dsaww = (ndi->win[nc].shgc_winter - filmSHGC) * ndi->win[nc].area_gross;
      dsaws = (ndi->win[nc].shgc_summer - filmSHGC) * ndi->win[nc].area_gross;
      ecmdua = 0.0f;
      if (ndi->key.window_film_delta_r > 0.0f)
        ecmdua = ndi->win[nc].u_value * (1.0f - 1.0f / (1.0f + ndi->win[nc].u_value * ndi->key.window_film_delta_r));
      ecmdua *= ndi->win[nc].area_gross;
      break;
    default:
      ASSERT(FALSE, sprintf(msg, "Window film measure not found: %d", cms_measure_num));
    }

    for (int m = 1; m <= MONTHS; m++) {
      nir->dua[nir->nms][m] += ecmdua;
      if (m > MARCH && m < OCTOBER)
        nir->dfreht[nir->nms][m] += dsaws * (cwd->solar_load[m][ndi->win[nc].solar_orient] * ndi->win[nc].shade_factor_summer +
                                   cwd->solar_load[m][SOLAR_DIFFUSE]);
      else
        nir->dfreht[nir->nms][m] += dsaww * (cwd->solar_load[m][ndi->win[nc].solar_orient] * ndi->win[nc].shade_factor_winter +
                                   cwd->solar_load[m][SOLAR_DIFFUSE]);
    }
    add_component_code(ndi->win[nc].code);

    switch (cms_measure_num) {
    case N_CMS_SUN_SCREEN_FABRIC:
      rmc_index = N_MAT_SUN_SCREEN_FABRIC;
      break;
    case N_CMS_SUN_SCREEN_LOUVERED:
      rmc_index = N_MAT_SUN_SCREEN_LOUVERED;
      break;
    case N_CMS_WINDOW_FILM:
      rmc_index = N_MAT_WINDOW_FILM;
      break;
    }
    nir->ecm[nir->nms].dwelling_component_index = nc;
    nir->ecm[nir->nms].cost += (ndi->rmc[rmc_index].matcost + ndi->rmc[rmc_index].labcost) * ndi->win[nc].area_gross +
                     ndi->rmc[rmc_index].itemcost * ndi->win[nc].number;

    nir->ecm[nir->nms].qtym += ndi->win[nc].area_gross;
    nir->ecm[nir->nms].qtyl += ndi->win[nc].area_gross;

    nir->ecm[nir->nms].costum = ndi->rmc[rmc_index].matcost;
    nir->ecm[nir->nms].costul = ndi->rmc[rmc_index].labcost;
    nir->ecm[nir->nms].costi1 = ndi->rmc[rmc_index].itemcost;
    nir->ecm[nir->nms].qtyi += ndi->win[nc].number; // each awning
  }

  if (implement_flag) {
    annual_energy_load_change(nir->dua[nir->nms], nir->dfreht[nir->nms], &dhld, &dcld);
    nir->htengysav[nir->nms] = dhld / nir->sysht_seaseff;
    nir->clengysav[nir->nms] = dcld / ndi->clgs.avg_seer * 3.413f * ndi->clgs.fraction_cooled; // MBG 10/07
    nir->ecm[nir->nms].cms_measure_num = cms_measure_num;
    nir->ecm[nir->nms].audit_section_id = N_WINDOW;

    non_zero_measure_cost(nir->ecm[nir->nms].cost, nir->ecm[nir->nms].cms_measure_num);
    measure_economics_using_rmc(nir->nms, rmc_index);
    increment_measure_counter();
  }
}

// Thermal vent damper model

static void heating_vent_damper(void) {
  float tvtdpsav;
  int rmc_index;

  vnttsavf = 0.04f; /* Damper on other than boiler */
  if (ndi->htg[PRIMARY].system_type == HE_STEAM_BOILER || ndi->htg[PRIMARY].system_type == HE_HOT_WATER_BOILER)
    vnttsavf = 0.06f; /* Damper on boiler */
  if (ndi->htg[PRIMARY].location == UNINTENTIONALLY_HEATED)
    vnttsavf /= 2.; /* System in unintentionally heated space */

  tvtdpsav = nir->heatload[PRE_RETROFIT_POST_INFIL] * ndi->htg[PRIMARY].percent_heat_supplied / ndi->htg[PRIMARY].delivered_eff * vnttsavf;

  if (ndi->htg[PRIMARY].vent_damper_recommended != YES)
    return; /* Not recommended */
  if (ndi->htg[PRIMARY].system_type == HE_OTHER || ndi->htg[PRIMARY].system_type == HE_UNVENTED_SPACE_HEATER)
    return; /* Does not check for electric systems. System not other or unvented spaceheater */
  if (ndi->htg[PRIMARY].location == UNHEATED)
    return; /* System not in unconditioned space */
  if (ndi->htg[PRIMARY].fuel_type != NATURAL_GAS && ndi->htg[PRIMARY].fuel_type != PROPANE)
    return; /* Only gas and propane systems */
  if (ndi->htg[PRIMARY].vent_damper_present == YES)
    return; /* Vent damper already installed */
  if (ndi->htg[PRIMARY].power_burner == YES || ndi->htg[PRIMARY].retention_head_burner == YES)
    return; /* No gas power or flame ret head burners */

  add_component_code(ndi->htg[PRIMARY].code);

  nir->htengysav[nir->nms] = tvtdpsav;
  nir->clengysav[nir->nms] = 0.0;

  rmc_index = N_MAT_THERMAL_VENT_DAMPER;

  nir->ecm[nir->nms].cost = (ndi->rmc[rmc_index].matcost + ndi->rmc[rmc_index].labcost) + ndi->rmc[rmc_index].itemcost;

  nir->ecm[nir->nms].dwelling_component_index = PRIMARY;
  nir->ecm[nir->nms].costum = ndi->rmc[rmc_index].matcost;
  nir->ecm[nir->nms].costul = ndi->rmc[rmc_index].labcost;
  nir->ecm[nir->nms].costi1 = ndi->rmc[rmc_index].itemcost;
  nir->ecm[nir->nms].qtym = 1;
  nir->ecm[nir->nms].qtyl = 1;
  nir->ecm[nir->nms].qtyi = 1;
  nir->ecm[nir->nms].cms_measure_num = N_CMS_THERMAL_VENT_DAMPER;
  nir->ecm[nir->nms].audit_section_id = N_HEATING;

  measure_economics_hvac(nir->nms, rmc_index);
  increment_measure_counter();
  return;
}

// Intermittent Ignition Device (IID)

static void heating_intermittent_ignition_device(void) {
  float iidsavt, temp;
  int rmc_index;

  iidsavw = (float)(4.2392e-3 * POWC(cwd->hdd65, 0.74214)); /* IID winter savings  */
  iidsavt = (float)(14.5875 * POWC(cwd->hdd65, -0.12188));  /* IID total savings */
  iidsavs = iidsavt - iidsavw;                         /* IID summer savings */
  if (ndi->htg[PRIMARY].pilot_light_on_summer == YES)
    iidsv = iidsavt; /* Pilot on in summer */
  else
    iidsv = iidsavw; /* Pilot off in summer */

  if (ndi->htg[PRIMARY].pilot_light_present != YES || ndi->htg[PRIMARY].intermittent_ignition == YES)
    return; /* Must have pilot light not iid */
  if (ndi->htg[PRIMARY].fuel_type != NATURAL_GAS && ndi->htg[PRIMARY].fuel_type != PROPANE)
    return; /* Fuel must be gas or propane */
  if (ndi->htg[PRIMARY].system_type != HE_GRAVITY_FURNACE && ndi->htg[PRIMARY].system_type != HE_FORCED_AIR_FURNACE &&
      ndi->htg[PRIMARY].system_type != HE_STEAM_BOILER) /* Must be furnace or boiler */
    return;

  add_component_code(ndi->htg[PRIMARY].code);

  temp = 0.1f * nir->heatengy[LAST_GOOD_MEASURE];
  if (iidsv > temp) {
    iidsavw *= temp / iidsv;
    iidsavs *= temp / iidsv;
    iidsv = temp;
  }
  nir->htengysav[nir->nms] = iidsv;
  nir->clengysav[nir->nms] = 0.0;
  
  rmc_index = N_MAT_IID;
  nir->ecm[nir->nms].dwelling_component_index = PRIMARY;
  nir->ecm[nir->nms].cost = (ndi->rmc[rmc_index].matcost + ndi->rmc[rmc_index].labcost) + ndi->rmc[rmc_index].itemcost;
  nir->ecm[nir->nms].costum = ndi->rmc[rmc_index].matcost;
  nir->ecm[nir->nms].costul = ndi->rmc[rmc_index].labcost;
  nir->ecm[nir->nms].costi1 = ndi->rmc[rmc_index].itemcost;
  nir->ecm[nir->nms].qtym = 1;
  nir->ecm[nir->nms].qtyl = 1;
  nir->ecm[nir->nms].qtyi = 1;
  nir->ecm[nir->nms].cms_measure_num = N_CMS_IID;
  nir->ecm[nir->nms].audit_section_id = N_HEATING;

  non_zero_measure_cost(nir->ecm[nir->nms].cost, nir->ecm[nir->nms].cms_measure_num);
  measure_economics_hvac(nir->nms, rmc_index);
  increment_measure_counter();

  return;
}

//Electric vent damper model

static void heating_vent_damper_electric(void) {
  int rmc_index;

  vntesavf = 0.;
  if (ndi->htg[PRIMARY].fuel_type == NATURAL_GAS || ndi->htg[PRIMARY].fuel_type == PROPANE) { /* Gas or propane systems */
    vntesavf = 0.06f;                                                             /* Furnace or space heater */
    if (ndi->htg[PRIMARY].system_type == HE_STEAM_BOILER || ndi->htg[PRIMARY].system_type == HE_HOT_WATER_BOILER) {
      vntesavf = 0.09f;
    }
  }                                        /* Boiler */
  else if (ndi->htg[PRIMARY].fuel_type == OIL) { /* Oil systems */
    vntesavf = 0.04f;                      /* Furnace or space heater */
    if (ndi->htg[PRIMARY].system_type == HE_STEAM_BOILER || ndi->htg[PRIMARY].system_type == HE_HOT_WATER_BOILER) {
      vntesavf = 0.06f;
    }
  } /* Boiler */
  if (ndi->htg[PRIMARY].location == UNINTENTIONALLY_HEATED)
    vntesavf /= 2.; /* System in unintentionally heated space */

  if (ndi->htg[PRIMARY].vent_damper_recommended != YES)
    return; /* Not recommended */
  if (ndi->htg[PRIMARY].location == UNHEATED)
    return; /* System must be in conditioned space */
  if (ndi->htg[PRIMARY].power_burner == YES || ndi->htg[PRIMARY].retention_head_burner == YES)
    return; /* No gas power or flame ret head burners */
  if (ndi->htg[PRIMARY].fuel_type == ELECTRIC)
    return; /* Not on electric systems */
  if (ndi->htg[PRIMARY].vent_damper_present == YES)
    return; /* Vent damper already installed */
  if (ndi->htg[PRIMARY].system_type == HE_UNVENTED_SPACE_HEATER)
    return; /* System is unvented */
  if (ndi->htg[PRIMARY].system_type != HE_GRAVITY_FURNACE && ndi->htg[PRIMARY].system_type != HE_FORCED_AIR_FURNACE &&
      ndi->htg[PRIMARY].system_type != HE_STEAM_BOILER &&
      ndi->htg[PRIMARY].system_type != HE_HOT_WATER_BOILER) /* Must be furnace or boiler */
    return;

  if (ndi->htg[PRIMARY].pilot_light_present == YES)
    return; /* Not on system with pilot light */

  add_component_code(ndi->htg[PRIMARY].code);

  nir->htengysav[nir->nms] = nir->heatload[PRE_RETROFIT_POST_INFIL] * ndi->htg[PRIMARY].percent_heat_supplied / ndi->htg[PRIMARY].delivered_eff * vntesavf;
  nir->clengysav[nir->nms] = 0.0;

  rmc_index = N_MAT_ELECTRIC_VENT_DAMPER;
  nir->ecm[nir->nms].dwelling_component_index = PRIMARY;
  nir->ecm[nir->nms].cost = (ndi->rmc[rmc_index].matcost + ndi->rmc[rmc_index].labcost) + ndi->rmc[rmc_index].itemcost;
  nir->ecm[nir->nms].costum = ndi->rmc[rmc_index].matcost;
  nir->ecm[nir->nms].costul = ndi->rmc[rmc_index].labcost;
  nir->ecm[nir->nms].costi1 = ndi->rmc[rmc_index].itemcost;
  nir->ecm[nir->nms].qtym = 1;
  nir->ecm[nir->nms].qtyl = 1;
  nir->ecm[nir->nms].qtyi = 1;
  nir->ecm[nir->nms].cms_measure_num = N_CMS_ELECTRIC_VENT_DAMPER;
  nir->ecm[nir->nms].audit_section_id = N_HEATING;

  non_zero_measure_cost(nir->ecm[nir->nms].cost, nir->ecm[nir->nms].cms_measure_num);
  measure_economics_hvac(nir->nms, rmc_index);
  increment_measure_counter();

  return;
}

//Electric vent damper / IID combination 

static void heating_vent_damper_and_intermittent_ignition_device(void) {
  float vntsav, vntesavtemp;
  float iidsavt, temp;
  int rmc_index;

  if (ndi->htg[PRIMARY].vent_damper_recommended != YES)
    return; /* Not recommended */
  if (ndi->htg[PRIMARY].location == UNHEATED)
    return; /* System in unconditioned space */
  if (ndi->htg[PRIMARY].power_burner == YES || ndi->htg[PRIMARY].retention_head_burner == YES)
    return; /* No gas power or flame ret head burners */
  if (ndi->htg[PRIMARY].fuel_type == ELECTRIC)
    return; /* Not on electric systems */
  if (ndi->htg[PRIMARY].vent_damper_present == YES || ndi->htg[PRIMARY].intermittent_ignition == YES)
    return; /* Vent damper or IID already installed */
  if (ndi->htg[PRIMARY].system_type == HE_UNVENTED_SPACE_HEATER)
    return; /* System is unvented */
  if (ndi->htg[PRIMARY].pilot_light_present != YES)
    return; /* Must have pilot light */
  if (ndi->htg[PRIMARY].system_type != HE_GRAVITY_FURNACE && ndi->htg[PRIMARY].system_type != HE_FORCED_AIR_FURNACE &&
      ndi->htg[PRIMARY].system_type != HE_STEAM_BOILER &&
      ndi->htg[PRIMARY].system_type != HE_HOT_WATER_BOILER) /* Must be furnace or boiler */
    return;

  add_component_code(ndi->htg[PRIMARY].code);

  vntesavtemp = 0.;
  if (ndi->htg[PRIMARY].fuel_type == NATURAL_GAS || ndi->htg[PRIMARY].fuel_type == PROPANE) { /* Gas or propane systems */
    vntesavtemp = 0.06f;                                                          /* Furnace or space heater */
    if (ndi->htg[PRIMARY].system_type == HE_STEAM_BOILER || ndi->htg[PRIMARY].system_type == HE_HOT_WATER_BOILER)
      vntesavtemp = 0.09f;
  }                                        /* Boiler */
  else if (ndi->htg[PRIMARY].fuel_type == OIL) { /* Oil systems */
    vntesavtemp = 0.04f;                   /* Furnace or space heater */
    if (ndi->htg[PRIMARY].system_type == HE_STEAM_BOILER || ndi->htg[PRIMARY].system_type == HE_HOT_WATER_BOILER)
      vntesavtemp = 0.06f;
  } /* Boiler */
  if (ndi->htg[PRIMARY].location == UNINTENTIONALLY_HEATED)
    vntesavtemp /= 2.; /* System in unintentionally heated space */

  vntsav = nir->heatload[PRE_RETROFIT_POST_INFIL] * ndi->htg[PRIMARY].percent_heat_supplied / ndi->htg[PRIMARY].delivered_eff * vntesavtemp;

  iidsavw = (float)(4.2392e-3 * POWC(cwd->hdd65, 0.74214)); /* IID winter savings  */
  iidsavt = (float)(14.5875 * POWC(cwd->hdd65, -0.12188));  /* IID total savings */
  iidsavs = iidsavt - iidsavw;                         /* IID summer savings */
  temp = 0.1f * nir->heatengy[LAST_GOOD_MEASURE];
  if (iidsavt > temp) {
    iidsavw *= temp / iidsavt;
    iidsavs *= temp / iidsavt;
  }

  nir->ecm[nir->nms].value = 0;
  if (ndi->htg[PRIMARY].pilot_light_on_summer == YES)
    nir->ecm[nir->nms].value = iidsavs;

  nir->htengysav[nir->nms] = nir->ecm[nir->nms].value + MAX(vntsav, iidsavw) + 0.5f * MIN(vntsav, iidsavw);
  nir->clengysav[nir->nms] = 0.0;

  rmc_index = N_MAT_ELECTRIC_VENT_DAMPER_AND_IID;
  nir->ecm[nir->nms].dwelling_component_index = PRIMARY;
  nir->ecm[nir->nms].cost = (ndi->rmc[rmc_index].matcost + ndi->rmc[rmc_index].labcost) + ndi->rmc[rmc_index].itemcost;
  nir->ecm[nir->nms].costum = ndi->rmc[rmc_index].matcost;
  nir->ecm[nir->nms].costul = ndi->rmc[rmc_index].labcost;
  nir->ecm[nir->nms].costi1 = ndi->rmc[rmc_index].itemcost;
  nir->ecm[nir->nms].qtym = 1;
  nir->ecm[nir->nms].qtyl = 1;
  nir->ecm[nir->nms].qtyi = 1;
  nir->ecm[nir->nms].cms_measure_num = N_CMS_ELECTRIC_VENT_DAMPER_AND_IID;
  nir->ecm[nir->nms].audit_section_id = N_HEATING;

  non_zero_measure_cost(nir->ecm[nir->nms].cost, nir->ecm[nir->nms].cms_measure_num);
  measure_economics_hvac(nir->nms, rmc_index);
  increment_measure_counter();

  return;
}

// Flame retention head burner 

static void heating_flame_retention_burner(void) {
  int rmc_index;

  if (ndi->htg[PRIMARY].retention_head_recommended != YES)
    return;
  if (ndi->htg[PRIMARY].fuel_type != OIL)
    return; /* Must be oil system */
  if (ndi->htg[PRIMARY].system_type != HE_GRAVITY_FURNACE && ndi->htg[PRIMARY].system_type != HE_FORCED_AIR_FURNACE &&
      ndi->htg[PRIMARY].system_type != HE_STEAM_BOILER &&
      ndi->htg[PRIMARY].system_type != HE_HOT_WATER_BOILER) /* Must be furnace or boiler */
    return;
  if (ndi->htg[PRIMARY].retention_head_burner == YES)
    return; /* Not already installed */

  add_component_code(ndi->htg[PRIMARY].code);

  ASSERT(ndi->htg[PRIMARY].delivered_eff, sprintf(msg, "Need non zero eff"));
  ASSERT(ndi->inf.post_duct_seal_efficiency, sprintf(msg, "Need non zero efficiency"));
  nir->ecm[nir->nms].value = 0;
  if (ndi->htg[PRIMARY].vent_damper_present != YES)
    nir->ecm[nir->nms].value = vnttsavf;

  // TODO resolve htg
  // nir->htengysav[nir->nms] = nir->heatload[PRE_RETROFIT_POST_INFIL]*ndi->htg[PRIMARY].percent_heat_supplied*(1.0f/ndi->htg[PRIMARY].delivered_eff -
  //    (1.0f-nir->ecm[nir->nms].value)/0.8f/ndi->htg[PRIMARY].ss_to_seas_factor/ndi->inf.post_duct_seal_efficiency);

  nir->htengysav[nir->nms] = nir->heatload[PRE_RETROFIT_POST_INFIL] * ndi->htg[PRIMARY].percent_heat_supplied *
    (1.0f / ndi->htg[PRIMARY].delivered_eff -
    (1.0f - nir->ecm[nir->nms].value) / 0.8f / SEASONAL_TO_SS_RATIO / ndi->inf.post_duct_seal_efficiency);
  nir->clengysav[nir->nms] = 0.0;

  rmc_index = N_MAT_FLAME_RETENTION_BURNER;
  nir->ecm[nir->nms].dwelling_component_index = PRIMARY;
  nir->ecm[nir->nms].cost = (ndi->rmc[rmc_index].matcost + ndi->rmc[rmc_index].labcost) + ndi->rmc[rmc_index].itemcost;
  nir->ecm[nir->nms].costum = ndi->rmc[rmc_index].matcost;
  nir->ecm[nir->nms].costul = ndi->rmc[rmc_index].labcost;
  nir->ecm[nir->nms].costi1 = ndi->rmc[rmc_index].itemcost;
  nir->ecm[nir->nms].qtym = 1;
  nir->ecm[nir->nms].qtyl = 1;
  nir->ecm[nir->nms].qtyi = 1;
  nir->ecm[nir->nms].cms_measure_num = N_CMS_FLAME_RETENTION_BURNER;
  nir->ecm[nir->nms].audit_section_id = N_HEATING;

  non_zero_measure_cost(nir->ecm[nir->nms].cost, nir->ecm[nir->nms].cms_measure_num);
  measure_economics_hvac(nir->nms, rmc_index);
  increment_measure_counter();

  return;
}

// Tuneup Gas/Propane/Oil/Kersene-Fired Furnace/Boiler
// Treat Propane as Gas and Kerosene as oil.
// Expansion to propane and kerosene done MBG 10/23/03

static void heating_tuneup(void) {
  float prsys_sseff, delssef, e1, e2, c1, s1, c2;
  char convb = 'N';
  int rmc_index;

  if (ndi->htg[PRIMARY].retrofit_option == ES_EVALUATE_NONE)
    return;

  if (ndi->htg[PRIMARY].system_type != HE_GRAVITY_FURNACE && ndi->htg[PRIMARY].system_type != HE_FORCED_AIR_FURNACE &&
      ndi->htg[PRIMARY].system_type != HE_STEAM_BOILER &&
      ndi->htg[PRIMARY].system_type != HE_HOT_WATER_BOILER) /* Must be furnace or boiler */
    return;

  if (ndi->htg[PRIMARY].fuel_type == ELECTRIC || ndi->htg[PRIMARY].fuel_type == WOOD || ndi->htg[PRIMARY].fuel_type == COAL)
    return; /* Must be oil,gas,propane or kerosene system - Can't be electric,wood or coal */

  add_component_code(ndi->htg[PRIMARY].code);

  ASSERT(ndi->inf.post_duct_seal_efficiency, sprintf(msg, "Need non zero"));
  delssef = 0.;

  // TODO resolve htg
  // prsys_sseff = ndi->htg[PRIMARY].delivered_eff/ndi->htg[PRIMARY].ss_to_seas_factor/ndi->inf.post_duct_seal_efficiency;

  prsys_sseff = ndi->htg[PRIMARY].delivered_eff / SEASONAL_TO_SS_RATIO / ndi->inf.post_duct_seal_efficiency;

  e1 = 0.70f; // Gas or Propane
  e2 = 0.76f;
  c1 = 0.67f;
  s1 = 0.04f;

  if (ndi->htg[PRIMARY].fuel_type == OIL || ndi->htg[PRIMARY].fuel_type == KEROSENE) {
    e1 = .69f;
    c1 = 1.0f;
    s1 = .07f;
    // Oil or Kerosene
    if (convb == 'Y') {
      e1 = .65f;
      e2 = .72f;
    }
  }

  c2 = 0.;
  if (ndi->htg[PRIMARY].system_type == HE_GRAVITY_FURNACE)
    c2 = 0.75f;
  else if (ndi->htg[PRIMARY].system_type == HE_FORCED_AIR_FURNACE)
    c2 = 1.0f;
  
  if (prsys_sseff <= e1)
    delssef = s1;
  else if (prsys_sseff < e2)
    delssef = c1 * (e2 - prsys_sseff);

  // TODO resolve why the existing system condition only seems to play into
  // the tune up IF duct sealing is not evaluated.  Review with Mark T

  if (ndi->inf.evaluate_duct_sealing != YES) {
    switch (ndi->htg[PRIMARY].condition) {
    case POOR:
      delssef += 3.0f * .025f * c2;
      break;
    case FAIR:
      delssef += 2.0f * .025f * c2;
      break;
    case GOOD:
      delssef += 1.0f * .025f * c2;
      break;
    }
  }
  nir->ecm[nir->nms].value = delssef;

  nir->htengysav[nir->nms] =
      nir->heatload[PRE_RETROFIT_POST_INFIL] / ndi->htg[PRIMARY].delivered_eff * ndi->htg[PRIMARY].percent_heat_supplied * delssef / (prsys_sseff + delssef);
  nir->clengysav[nir->nms] = 0.0;

  rmc_index = N_MAT_FURNACE_TUNE_UP;
  nir->ecm[nir->nms].dwelling_component_index = PRIMARY;
  nir->ecm[nir->nms].cost = (ndi->rmc[rmc_index].matcost + ndi->rmc[rmc_index].labcost) + ndi->rmc[rmc_index].itemcost;
  nir->ecm[nir->nms].costum = ndi->rmc[rmc_index].matcost;
  nir->ecm[nir->nms].costul = ndi->rmc[rmc_index].labcost;
  nir->ecm[nir->nms].costi1 = ndi->rmc[rmc_index].itemcost;
  nir->ecm[nir->nms].qtym = 1;
  nir->ecm[nir->nms].qtyl = 1;
  nir->ecm[nir->nms].qtyi = 1;
  nir->ecm[nir->nms].cms_measure_num = N_CMS_FURNACE_TUNE_UP;
  nir->ecm[nir->nms].audit_section_id = N_HEATING;

  non_zero_measure_cost(nir->ecm[nir->nms].cost, nir->ecm[nir->nms].cms_measure_num);
  measure_economics_hvac(nir->nms, rmc_index);
  increment_measure_counter();

  return;
}

static void heating_replacement_standard_efficiency(void) {
  //float repeff, consumption[POST_RETROFIT + 1];
  float repeff;
  float htsys_repcstl, htsys_repcstm, load_fraction_met, inv_seaseff_post;
  //int nflg = 0, i, fuel[POST_RETROFIT + 1];
  int i;
  int rmc_index;

  if (ndi->htg[PRIMARY].retrofit_fuel_type == FUEL_NONE)            // required field, just in case
    ndi->htg[PRIMARY].retrofit_fuel_type = ndi->htg[PRIMARY].fuel_type;   // default to the pre-retrofit fuel

  // measure does not apply if
  if (ndi->htg[PRIMARY].retrofit_option == ES_TUNEUP_REQUIRED ||
    ndi->htg[PRIMARY].retrofit_option == ES_HIEFF_REP_REQUIRED ||
    ndi->htg[PRIMARY].retrofit_option == ES_NO_REP_EVAL_TUNEUP ||
    ndi->htg[PRIMARY].retrofit_option == ES_EVALUATE_HEATPUMP ||
    ndi->htg[PRIMARY].retrofit_option == ES_HEATPUMP_REQUIRED ||
    ndi->htg[PRIMARY].retrofit_option == ES_EVALUATE_NONE)
    return;

  /* Add load fractions from replaced secondary systems and
     form component string */

  add_component_code(ndi->htg[PRIMARY].code);
  load_fraction_met = ndi->htg[PRIMARY].percent_heat_supplied;
  for (i = 1; i < ndi->num_htg; i++) {
    if (ndi->htg[i].replace_system == YES) {
      load_fraction_met += ndi->htg[i].percent_heat_supplied;
      add_component_code(ndi->htg[i].code);
    }
  }

  nir->ecm[nir->nms].cms_measure_num = N_CMS_REPLACE_HEATING_SYSTEM;
  nir->ecm[nir->nms].audit_section_id = N_HEATING;
  nir->ecm[nir->nms].dwelling_component_index = PRIMARY;

  repeff = ndi->htg[PRIMARY].retrofit_afue_standard_eff / 100.0f;

  /*  REQUIRED ELECTRIC RESISTANCE REPLACEMENTS */

  if (ndi->htg[PRIMARY].fuel_type == ELECTRIC && ndi->htg[PRIMARY].retrofit_option == ES_ELECRES_REP_REQUIRED &&
      ndi->htg[PRIMARY].system_type != HE_HEAT_PUMP) {
    htsys_repcstl = ndi->htg[PRIMARY].labor_cost_standard_eff;
    htsys_repcstm = ndi->htg[PRIMARY].material_cost_standard_eff;
    nir->ecm[nir->nms].cost = htsys_repcstl + htsys_repcstm;
    nir->ecm[nir->nms].costum = htsys_repcstm;
    nir->ecm[nir->nms].costul = htsys_repcstl;
    nir->ecm[nir->nms].costi1 = 0;

    rmc_index = heating_repacement_material_standard_efficiency();

    nir->ecm[nir->nms].rmc_index = rmc_index;
    ndi->rmc[rmc_index].ecm_index = nir->nms;

    nir->htengysav[nir->nms] = 0.0f;
    nir->clengysav[nir->nms] = 0.0f;

    nir->ecm[nir->nms].qtym = 1;
    nir->ecm[nir->nms].qtyl = 0;
    nir->ecm[nir->nms].qtyi = 0;

    // new materials cost detail MJF 12/07
    add_material("", 0, 0, rmc_index, ndi->rmc[rmc_index].material, ndi->rmc[rmc_index].retrofit_type, 
              MT_HEATING_EQUIPMENT, ndi->rmc[rmc_index].units, 1, nir->ecm[nir->nms].costum, "");
    add_material("", 0, 0, rmc_index, ndi->rmc[rmc_index].material, ndi->rmc[rmc_index].retrofit_type, 
              MT_LABOR, ndi->rmc[rmc_index].units, 1, nir->ecm[nir->nms].costul, "");
    add_material("", 0, 0, rmc_index, ndi->rmc[rmc_index].material, ndi->rmc[rmc_index].retrofit_type, 
              MT_OTHER, "Each", 1, nir->ecm[nir->nms].costi1, "");

    inv_seaseff_post = load_fraction_met / repeff / ndi->inf.post_duct_seal_efficiency;
    // loop through non-primary systems
    for (i = 1; i < ndi->num_htg; i++) {
      if (ndi->htg[i].replace_system != YES) // Eliminate removed systems
        inv_seaseff_post += ndi->htg[i].percent_heat_supplied / ndi->htg[i].delivered_eff;
    }
    nir->ecm[nir->nms].value = inv_seaseff_post;

    nir->mslife[nir->nms] = ndi->rmc[rmc_index].life;
    nir->measure_priority[nir->nms] = MPS_EQUIP_REQUIRED; // Make required

    non_zero_measure_cost(nir->ecm[nir->nms].cost, nir->ecm[nir->nms].cms_measure_num);
    increment_measure_counter();
    return;
  }

  /*  FURNACES AND BOILERS */

  if (ndi->htg[PRIMARY].system_type == HE_GRAVITY_FURNACE || ndi->htg[PRIMARY].system_type == HE_FORCED_AIR_FURNACE ||
      ndi->htg[PRIMARY].system_type == HE_STEAM_BOILER || ndi->htg[PRIMARY].system_type == HE_HOT_WATER_BOILER) {

    if (ndi->htg[PRIMARY].retrofit_option == ES_HIEFF_REP_REQUIRED)
      return;
    /* Hi Eff Rep Mandatory */

    if (ndi->htg[PRIMARY].fuel_type != NATURAL_GAS && 
      ndi->htg[PRIMARY].fuel_type != OIL && 
      ndi->htg[PRIMARY].fuel_type != PROPANE &&
      ndi->htg[PRIMARY].fuel_type != KEROSENE)
      return; /* Must be oil, kerosene, gas, or propane system */

    // if ((ndi->htg[PRIMARY].system_type == HE_STEAM_BOILER || ndi->htg[PRIMARY].system_type == HE_HOT_WATER_BOILER) && (jm == 28))
    //   return; /* Boiler but evaluating furnace */

    // if ((ndi->htg[PRIMARY].system_type == HE_GRAVITY_FURNACE || ndi->htg[PRIMARY].system_type == HE_FORCED_AIR_FURNACE) && (jm == 46))
    //   return; /* Furnace but evaluating boiler */

    /* Set replacement costs from building description input */

    htsys_repcstl = ndi->htg[PRIMARY].labor_cost_standard_eff;
    htsys_repcstm = ndi->htg[PRIMARY].material_cost_standard_eff;
  }

  /*  SPACE HEATERS  */

  else if (ndi->htg[PRIMARY].system_type == HE_UNVENTED_SPACE_HEATER || ndi->htg[PRIMARY].system_type == HE_VENTED_SPACE_HEATER) {
    if (ndi->htg[PRIMARY].fuel_type == WOOD || 
      ndi->htg[PRIMARY].fuel_type == COAL || 
      ndi->htg[PRIMARY].fuel_type == ELECTRIC)
      return; /* Can't be wood, coal, or electric (no efficiency change)  */

    htsys_repcstl = ndi->htg[PRIMARY].labor_cost_standard_eff;
    htsys_repcstm = ndi->htg[PRIMARY].material_cost_standard_eff;

    if (htsys_repcstl <= .1 && htsys_repcstm <= .1) {

      int sh_type, rmc_index1, rmc_index2;
      float low_capacity_kbtuh[3], high_capacity_kbtuh[3], slope, mincost;

      switch (ndi->htg[PRIMARY].fuel_type) {
      case OIL:
        rmc_index1 = N_MAT_SPACE_HEATER_OIL_40K;
        rmc_index2 = N_MAT_SPACE_HEATER_OIL_75K;
        sh_type = 1;
        break;
      case PROPANE:
        rmc_index1 = N_MAT_SPACE_HEATER_GAS_8K;
        rmc_index2 = N_MAT_SPACE_HEATER_GAS_55K;
        sh_type = 0;
        break;
      case KEROSENE:
        rmc_index1 = N_MAT_SPACE_HEATER_KER_10K;
        rmc_index2 = N_MAT_SPACE_HEATER_KER_40K;
        sh_type = 2;
        break;
      default:
        ASSERT(FALSE, sprintf(msg, "Incorrect space heater fuel: %d", ndi->htg[PRIMARY].fuel_type));
      }

      // interpolate space heater replacement cost based on the 
      // rated capacities in the rmc[] data for fixed space heater 
      // capacities in kBtu/h

      low_capacity_kbtuh[0] = 8.;       // N_MAT_SPACE_HEATER_GAS_8K
      low_capacity_kbtuh[1] = 40.;      // N_MAT_SPACE_HEATER_OIL_40K
      low_capacity_kbtuh[2] = 10.;      // N_MAT_SPACE_HEATER_KER_10K
      
      high_capacity_kbtuh[0] = 55.;      // N_MAT_SPACE_HEATER_GAS_55K
      high_capacity_kbtuh[1] = 75.;      // N_MAT_SPACE_HEATER_OIL_75K
      high_capacity_kbtuh[2] = 40.;      // N_MAT_SPACE_HEATER_KER_40K

      mincost = ndi->rmc[rmc_index1].matcost;
      slope = (ndi->rmc[rmc_index2].matcost - mincost) / (high_capacity_kbtuh[sh_type] - low_capacity_kbtuh[sh_type]);
      htsys_repcstm = mincost + slope * (ndi->htg[PRIMARY].output_capacity - low_capacity_kbtuh[sh_type]);
      if (htsys_repcstm < mincost)
        htsys_repcstm = mincost;

      mincost = ndi->rmc[rmc_index1].labcost + ndi->rmc[rmc_index1].itemcost;
      slope = (ndi->rmc[rmc_index2].labcost + ndi->rmc[rmc_index2].itemcost - mincost) / (high_capacity_kbtuh[sh_type] - low_capacity_kbtuh[sh_type]);
      htsys_repcstl = mincost + slope * (ndi->htg[PRIMARY].output_capacity - low_capacity_kbtuh[sh_type]);
      if (htsys_repcstl < mincost)
        htsys_repcstl = mincost;
    }

   //fuelt = costhtsys(&htsys_repcstm, &htsys_repcstl, &nmat, nflg, 0);
   // ASSERT(nmat < NMAT, sprintf(msg, "Array out of range"));

    nir->ecm[nir->nms].cost = htsys_repcstl + htsys_repcstm;
  }

  else
    return;       // measure does not apply so return with nothing implemented

  /*  ALL OTHER HEATING SYSTEM replacements with standard efficiency */

  /* Compute annual energy savings */

  rmc_index = heating_repacement_material_standard_efficiency();

  ASSERT(repeff != 0.0, sprintf(msg, "Must have non zero efficiency"));
  ASSERT(ndi->inf.post_duct_seal_efficiency != 0.0, sprintf(msg, "Must have non zero duct efficiency"));
  inv_seaseff_post = load_fraction_met / repeff / ndi->inf.post_duct_seal_efficiency;

  for (i = 1; i < ndi->num_htg; i++) {
    if (ndi->htg[i].replace_system != YES) // Eliminate removed systems
      inv_seaseff_post += ndi->htg[i].percent_heat_supplied / ndi->htg[i].delivered_eff;
  }

  // #228 updated energy and economics for fuel switching
  heating_system_energy_savings(load_fraction_met, repeff);

  nir->clengysav[nir->nms] = 0.0f;
  nir->ecm[nir->nms].value = inv_seaseff_post;

  //  Compute locally defined consumptions for call to fuelswitch
  // consumption[PRE_RETROFIT] = nir->heatload[PRE_RETROFIT_POST_INFIL] * (1.0f / nir->sysht_seaseff);
  // consumption[POST_RETROFIT] = nir->heatload[PRE_RETROFIT_POST_INFIL] * inv_seaseff_post;

  if (htsys_repcstl > .1 || htsys_repcstm > .1) {
    nir->ecm[nir->nms].cost = htsys_repcstl + htsys_repcstm;
    nir->ecm[nir->nms].costum = htsys_repcstm;
    nir->ecm[nir->nms].costul = htsys_repcstl;
    nir->ecm[nir->nms].costi1 = 0;
  } else {
    nir->ecm[nir->nms].cost = (ndi->rmc[rmc_index].matcost + ndi->rmc[rmc_index].labcost) + ndi->rmc[rmc_index].itemcost;
    nir->ecm[nir->nms].costum = ndi->rmc[rmc_index].matcost;
    nir->ecm[nir->nms].costul = ndi->rmc[rmc_index].labcost;
    nir->ecm[nir->nms].costi1 = ndi->rmc[rmc_index].itemcost;
  }

  nir->ecm[nir->nms].qtym = 1;
  nir->ecm[nir->nms].qtyl = 1;
  nir->ecm[nir->nms].qtyi = 1;

  // new materials cost detail MJF 12/07
  add_material("", 0, 0, rmc_index, ndi->rmc[rmc_index].material, ndi->rmc[rmc_index].retrofit_type, 
            MT_HEATING_EQUIPMENT, ndi->rmc[rmc_index].units, 1, nir->ecm[nir->nms].costum, "");
  add_material("", 0, 0, rmc_index, ndi->rmc[rmc_index].material, ndi->rmc[rmc_index].retrofit_type, 
            MT_LABOR, ndi->rmc[rmc_index].units, 1, nir->ecm[nir->nms].costul, "");
  add_material("", 0, 0, rmc_index, ndi->rmc[rmc_index].material, ndi->rmc[rmc_index].retrofit_type, 
            MT_OTHER, "Each", 1, nir->ecm[nir->nms].costi1, "");

  // Populate local parameters for call to measure_economics_with_fuel_switch()

  // fuel[PRE_RETROFIT] = ndi->htg[PRIMARY].fuel_type;
  // fuel[POST_RETROFIT] = ndi->htg[PRIMARY].retrofit_fuel_type;

  nir->mslife[nir->nms] = ndi->rmc[rmc_index].life;

  // #228 with fuel switching
  heating_system_dollar_savings(load_fraction_met, repeff, rmc_index);

  nir->ecm[nir->nms].rmc_index = rmc_index;
  ndi->rmc[rmc_index].ecm_index = nir->nms;

  non_zero_measure_cost(nir->ecm[nir->nms].cost, nir->ecm[nir->nms].cms_measure_num);
  increment_measure_counter();

  return;
}

// Replace Heating System - High efficiency

static void heating_replacement_high(int cms_measure_num) {
  //float repeff, consumption[POST_RETROFIT + 1];
  float repeff;
  float htsys_repcstl, htsys_repcstm, load_fraction_met, inv_seaseff_post;
  //int i, fuel[POST_RETROFIT + 1];
  int i;
  int rmc_index;

  if (ndi->htg[PRIMARY].retrofit_fuel_type == 0)                  // required field, just in case
    ndi->htg[PRIMARY].retrofit_fuel_type = ndi->htg[PRIMARY].fuel_type; // default to the pre-retrofit fuel

  // measure does not apply if
  if (ndi->htg[PRIMARY].retrofit_option == ES_TUNEUP_REQUIRED ||
    ndi->htg[PRIMARY].retrofit_option == ES_STDEFF_REP_REQUIRED ||
    ndi->htg[PRIMARY].retrofit_option == ES_NO_REP_EVAL_TUNEUP ||
    ndi->htg[PRIMARY].retrofit_option == ES_EVALUATE_HEATPUMP ||
    ndi->htg[PRIMARY].retrofit_option == ES_HEATPUMP_REQUIRED ||
    ndi->htg[PRIMARY].retrofit_option == ES_EVALUATE_NONE)
    return;

  rmc_index = heating_repacement_material_high_efficiency();

  nir->ecm[nir->nms].rmc_index = rmc_index;
  ndi->rmc[rmc_index].ecm_index = nir->nms;

  nir->ecm[nir->nms].audit_section_id = N_HEATING;
  nir->ecm[nir->nms].dwelling_component_index = PRIMARY;

  /*  FURNACES & BOILERS  */

  switch(rmc_index){
  case N_MAT_HIGH_EFFICIENCY_BOILER:
    nir->ecm[nir->nms].cms_measure_num = N_CMS_HIGH_EFFICIENCY_BOILER;
    break;
  case N_MAT_HIGH_EFFICIENCY_FURNACE:
    nir->ecm[nir->nms].cms_measure_num = N_CMS_HIGH_EFFICIENCY_FURNACE;
    break;
  default:
    ASSERT(FALSE, sprintf(msg, "Incorrent high efficiency heating system material:%d",rmc_index));
  }

  if ((ndi->htg[PRIMARY].system_type == HE_GRAVITY_FURNACE || ndi->htg[PRIMARY].system_type == HE_FORCED_AIR_FURNACE ||
       ndi->htg[PRIMARY].system_type == HE_STEAM_BOILER || ndi->htg[PRIMARY].system_type == HE_HOT_WATER_BOILER)) {       // Furnace or boiler

    if (ndi->htg[PRIMARY].fuel_type != NATURAL_GAS && ndi->htg[PRIMARY].fuel_type != OIL && ndi->htg[PRIMARY].fuel_type != PROPANE &&
        ndi->htg[PRIMARY].fuel_type != KEROSENE)
      return; /* Must be oil, kerosene, gas, or propane system */

    if ((ndi->htg[PRIMARY].system_type == HE_STEAM_BOILER || ndi->htg[PRIMARY].system_type == HE_HOT_WATER_BOILER) && 
      (cms_measure_num == N_CMS_HIGH_EFFICIENCY_FURNACE))
      return; /* Boiler but evaluating furnace */

    if ((ndi->htg[PRIMARY].system_type == HE_GRAVITY_FURNACE || ndi->htg[PRIMARY].system_type == HE_FORCED_AIR_FURNACE) && 
      (cms_measure_num == N_CMS_HIGH_EFFICIENCY_BOILER))
      return; /* Furnace but evaluating boiler */

    /* Set replacement efficiency from input */

    repeff = ndi->htg[PRIMARY].retrofit_afue_high_eff / 100.0f; // Hi Eff Rep

    /* Set replacement costs from building description input */

    htsys_repcstl = ndi->htg[PRIMARY].labor_cost_high_eff;
    htsys_repcstm = ndi->htg[PRIMARY].material_cost_high_eff;

    /* Add load fractions from replaced secondary systems and
       form component string */

    add_component_code(ndi->htg[PRIMARY].code);
    load_fraction_met = ndi->htg[PRIMARY].percent_heat_supplied;
    for (i = 1; i < ndi->num_htg; i++) {
      if (ndi->htg[i].replace_system == YES) {
        load_fraction_met += ndi->htg[i].percent_heat_supplied;
        add_component_code(ndi->htg[i].code);
      }
    }

    /* Compute annual energy savings */

    ASSERT(repeff != 0.0, sprintf(msg, "Must have non zero efficiency"));
    ASSERT(ndi->inf.post_duct_seal_efficiency != 0.0, sprintf(msg, "Must have non zero duct efficiency"));
    inv_seaseff_post = load_fraction_met / repeff / ndi->inf.post_duct_seal_efficiency;

    for (i = 1; i < ndi->num_htg; i++) {        // not primary system i = 0
      if (ndi->htg[i].replace_system != YES)    // eliminate removed non-primary systems
        inv_seaseff_post += ndi->htg[i].percent_heat_supplied / ndi->htg[i].delivered_eff;
    }

    // #228 updated energy and economics for fuel switching
    heating_system_energy_savings(load_fraction_met, repeff);

    nir->clengysav[nir->nms] = 0.0f;
    nir->ecm[nir->nms].value2 = inv_seaseff_post;

    //  Compute locally defined consumptions for call to fuelswitch
    // consumption[PRE_RETROFIT] = nir->heatload[PRE_RETROFIT_POST_INFIL] * (1.0f / nir->sysht_seaseff);
    // consumption[POST_RETROFIT] = nir->heatload[PRE_RETROFIT_POST_INFIL] * inv_seaseff_post;
  }

  else
    return;   // no applicable systems to repace

  /*  ALL HEATING SYSTEMS */

  if (htsys_repcstl > .1 || htsys_repcstm > .1) {
    nir->ecm[nir->nms].cost = htsys_repcstl + htsys_repcstm;
    nir->ecm[nir->nms].costum = htsys_repcstm;
    nir->ecm[nir->nms].costul = htsys_repcstl;
    nir->ecm[nir->nms].costi1 = 0;
  } else {
    nir->ecm[nir->nms].cost = (ndi->rmc[rmc_index].matcost + ndi->rmc[rmc_index].labcost) + ndi->rmc[rmc_index].itemcost;
    nir->ecm[nir->nms].costum = ndi->rmc[rmc_index].matcost;
    nir->ecm[nir->nms].costul = ndi->rmc[rmc_index].labcost;
    nir->ecm[nir->nms].costi1 = ndi->rmc[rmc_index].itemcost;
  }

  nir->ecm[nir->nms].qtym = 1;
  nir->ecm[nir->nms].qtyl = 1;
  nir->ecm[nir->nms].qtyi = 1;

  // new materials cost detail MJF 12/07
  add_material("", 0, 0, rmc_index, ndi->rmc[rmc_index].material, ndi->rmc[rmc_index].retrofit_type, 
            MT_HEATING_EQUIPMENT, ndi->rmc[rmc_index].units, 1, nir->ecm[nir->nms].costum, "");
  add_material("", 0, 0, rmc_index, ndi->rmc[rmc_index].material, ndi->rmc[rmc_index].retrofit_type, 
            MT_LABOR, ndi->rmc[rmc_index].units, 1, nir->ecm[nir->nms].costul, "");
  add_material("", 0, 0, rmc_index, ndi->rmc[rmc_index].material, ndi->rmc[rmc_index].retrofit_type, 
            MT_OTHER, "Each", 1, nir->ecm[nir->nms].costi1, "");

  // Populate local parameters for call to measure_economics_with_fuel_switch()

  // fuel[PRE_RETROFIT] = ndi->htg[PRIMARY].fuel_type;
  // fuel[POST_RETROFIT] = ndi->htg[PRIMARY].retrofit_fuel_type;
  // fuel[POST_RETROFIT] = ndi->htg[PRIMARY].retrofit_fuel_type;

  nir->mslife[nir->nms] = ndi->rmc[rmc_index].life;

  // #228 with fuel switching
  heating_system_dollar_savings(load_fraction_met, repeff, rmc_index);

  non_zero_measure_cost(nir->ecm[nir->nms].cost, nir->ecm[nir->nms].cms_measure_num);
  increment_measure_counter();

  return;
}

// #228 updated energy and economics for fuel switching
static void heating_system_energy_savings(float load_fraction_met, float repeff){

  float energy_pre = 0.0;
  float energy_post = 0.0;

  if (cmds.debug_level & D_NEAT_HTG_DETAIL) {
    fprintf(stderr, "\n\nLoad:%8.3f\n", nir->heatload[PRE_RETROFIT_POST_INFIL]);
  }

  // PRE
  for (int i = 0; i < ndi->num_htg; i++) {

    if (cmds.debug_level & D_NEAT_HTG_DETAIL) {
      fprintf(stderr, "System Pre:%s Eff:%8.3f PercentSup:%8.3f Energy:%8.3f\n", 
        ndi->htg[i].code,
        ndi->htg[i].delivered_eff,
        ndi->htg[i].percent_heat_supplied,
        (nir->heatload[PRE_RETROFIT_POST_INFIL] * ndi->htg[i].percent_heat_supplied) / ndi->htg[i].delivered_eff);
    }
    energy_pre += (nir->heatload[PRE_RETROFIT_POST_INFIL] * ndi->htg[i].percent_heat_supplied) / ndi->htg[i].delivered_eff;   // mmbtu
  }

  // POST
  for (int i = 0; i < ndi->num_htg; i++) {
    if (i == 0) {
      if (cmds.debug_level & D_NEAT_HTG_DETAIL) {
        fprintf(stderr, "System Post:%s Eff:%8.3f PercentSup:%8.3f Energy:%8.3f\n", 
          ndi->htg[i].code,
          repeff,
          load_fraction_met,
          (nir->heatload[PRE_RETROFIT_POST_INFIL] * load_fraction_met) / repeff);
      }
      energy_post += (nir->heatload[PRE_RETROFIT_POST_INFIL] * load_fraction_met) / repeff;
    } else if (ndi->htg[i].replace_system != YES) {
      if (cmds.debug_level & D_NEAT_HTG_DETAIL) {
        fprintf(stderr, "System Post:%s Eff:%8.3f PercentSup:%8.3f Energy:%8.3f\n", 
          ndi->htg[i].code,
          ndi->htg[i].delivered_eff,
          ndi->htg[i].percent_heat_supplied,
          (nir->heatload[PRE_RETROFIT_POST_INFIL] * ndi->htg[i].percent_heat_supplied) / ndi->htg[i].delivered_eff);
      }
      energy_post += (nir->heatload[PRE_RETROFIT_POST_INFIL] * ndi->htg[i].percent_heat_supplied) / ndi->htg[i].delivered_eff;
    }
  }
  nir->htengysav[nir->nms] = energy_pre - energy_post;
}

// #228 updated energy and economics for fuel switching
static void heating_system_dollar_savings(float load_fraction_met, float repeff, int rmc_index) {

  float dollars_pre = 0.0;
  float dollars_post = 0.0;

  nir->htengysav[nir->nms] *= nir->bladj[PRE_HEATING];
  nir->clengysav[nir->nms] *= nir->bladj[PRE_COOLING];
  nir->htlcs[nir->nms] = 0.0f;

  // old single expression of dollar savings, incorrect
  // nir->dslfsav[ecm_index] = 
  //   pw_fuel_cost(fuel[PRE_RETROFIT], life) * consumption[PRE_RETROFIT] - pw_fuel_cost(fuel[POST_RETROFIT], life) * consumption[POST_RETROFIT] +
  //   nir->clengysav[ecm_index] * pw_fuel_cost(ELECTRIC, life);

  float annual_fuel_use;
  // PRE
  for (int i = 0; i < ndi->num_htg; i++) {
    annual_fuel_use = ((nir->heatload[PRE_RETROFIT_POST_INFIL] * ndi->htg[i].percent_heat_supplied) / ndi->htg[i].delivered_eff) * nir->bladj[PRE_HEATING];
    if (cmds.debug_level & D_NEAT_HTG_DETAIL) {
      fprintf(stderr, "System Pre:%s FuelUse:%8.3f, PWDollars:%8.3f\n",
        ndi->htg[i].code,
        annual_fuel_use,
        pw_fuel_cost(ndi->htg[i].fuel_type, ndi->rmc[rmc_index].life) * annual_fuel_use);
    }
    dollars_pre += pw_fuel_cost(ndi->htg[i].fuel_type, ndi->rmc[rmc_index].life) * annual_fuel_use;
  }
  // POST
  for (int i = 0; i < ndi->num_htg; i++) {
    if (i == 0) {
      annual_fuel_use = ((nir->heatload[PRE_RETROFIT_POST_INFIL] * load_fraction_met) / repeff) * nir->bladj[PRE_HEATING];
      dollars_post += pw_fuel_cost(ndi->htg[i].retrofit_fuel_type, ndi->rmc[rmc_index].life) * annual_fuel_use;
    } else if (ndi->htg[i].replace_system != YES) {
      annual_fuel_use = ((nir->heatload[PRE_RETROFIT_POST_INFIL] * ndi->htg[i].percent_heat_supplied) / ndi->htg[i].delivered_eff) * nir->bladj[PRE_HEATING];
      dollars_post += pw_fuel_cost(ndi->htg[i].fuel_type, ndi->rmc[rmc_index].life) * annual_fuel_use;
    }
    if (cmds.debug_level & D_NEAT_HTG_DETAIL) {
      fprintf(stderr, "System Post:%s FuelUse:%8.3f, PWDollars:%8.3f\n",
        ndi->htg[i].code,
        annual_fuel_use,
        pw_fuel_cost(ndi->htg[i].fuel_type, ndi->rmc[rmc_index].life) * annual_fuel_use);
    }
  }

  nir->dslfsav[nir->nms] = dollars_pre - dollars_post;
  nir->ecm[nir->nms].sir = calculate_sir(nir->dslfsav[nir->nms], nir->ecm[nir->nms].cost);
  nir->ecm[nir->nms].lifetime = ndi->rmc[rmc_index].life;
  nir->npv[nir->nms] = nir->dslfsav[nir->nms] - nir->ecm[nir->nms].cost;
}

// Programmable Thermostat for Heating Night Setback

void smart_thermostat(int iperm) {
  int m;
  float x, tstar, tau, ta, tahns, dhld, dcld;
  int rmc_index;

  if (ndi->htg[PRIMARY].smart_thermostat == YES)    // already have one
    return;

  if (ndi->htg[PRIMARY].system_type != HE_GRAVITY_FURNACE && 
      ndi->htg[PRIMARY].system_type != HE_FORCED_AIR_FURNACE &&
      ndi->htg[PRIMARY].system_type != HE_STEAM_BOILER &&
      ndi->htg[PRIMARY].system_type != HE_HOT_WATER_BOILER) /* Must be furnace or boiler */
    return;

  if(ndi->htg[PRIMARY].fuel_type == WOOD || ndi->htg[PRIMARY].fuel_type == COAL)    // #247
    return;   // not applicable to these (batcfuel types

  tahns = ndi->key.nighttime_heating_setpoint;
  ndi->key.nighttime_heating_setpoint = ndi->key.nighttime_heating_setpoint - ndi->key.nighttime_heating_setback;
  tau = THERMAL_MASS * ndi->gnl.floor_area / nir->building_load_coeff[AVG];
  for (m = 1; m <= MONTHS; m++) {
    nir->dua[nir->nms][m] = 0.0f;
    nir->dfreht[nir->nms][m] = 0.;
    if (ndi->key.nighttime_heating_setpoint <= nir->teffn[m])
      continue;
    x = (ndi->key.daytime_heating_setpoint - ndi->key.nighttime_heating_setpoint) /
        (ndi->key.nighttime_heating_setpoint - nir->teffn[m]);
    tstar = tau * (float)log((double)(1.0f + x));
    if (tstar < 12.)
      ta = ndi->key.nighttime_heating_setpoint +
           (ndi->key.daytime_heating_setpoint - ndi->key.nighttime_heating_setpoint) * tau / 12.0f *
               (1.0f - (float)log((double)(1.0f + x)) / x);
    else
      ta = nir->teffn[m] + tau / 12.0f * (ndi->key.daytime_heating_setpoint - nir->teffn[m]) * (1.0f - (float)exp((double)(-12.0f / tau)));
    nir->night_setback_temperature[m] = ta;
    nir->night_setback_temperature[m] = ndi->key.daytime_heating_setpoint / 3.0f + 2.0f * nir->night_setback_temperature[m] / 3.0f;
  } /* Assume 8 hr setback, not 12 */
  annual_energy_load_change(nir->dua[nir->nms], nir->dfreht[nir->nms], &dhld, &dcld);
  ndi->key.nighttime_heating_setpoint = tahns;
  if (iperm == 1) {
    nir->heatload[CURRENT_MEASURE] = nir->heatload[LAST_GOOD_MEASURE] - dhld;
    nir->heatengy[CURRENT_MEASURE] = nir->heatload[CURRENT_MEASURE] / nir->sysht_seaseff;
    nir->coolload[CURRENT_MEASURE] = nir->coolload[LAST_GOOD_MEASURE];
    nir->coolengy[CURRENT_MEASURE] = nir->coolengy[LAST_GOOD_MEASURE];
    return;
  } else {
    for (m = 1; m <= MONTHS; m++){
      nir->night_setback_temperature[m] = ndi->key.nighttime_heating_setpoint;
    }

    add_component_code_once(ndi->htg[PRIMARY].code);

    nir->htengysav[nir->nms] = dhld / nir->sysht_seaseff;
    nir->clengysav[nir->nms] = 0.;
    nir->ecm[nir->nms].cms_measure_num = N_CMS_SMART_THERMOSTAT;
    nir->ecm[nir->nms].audit_section_id = N_HEATING;              // heating setback only savings as of 3/2020

    rmc_index = N_MAT_SMART_THERMOSTAT;
    nir->ecm[nir->nms].dwelling_component_index = PRIMARY;
    nir->ecm[nir->nms].cost = (ndi->rmc[rmc_index].matcost + ndi->rmc[rmc_index].labcost) + ndi->rmc[rmc_index].itemcost;
    nir->ecm[nir->nms].costum = ndi->rmc[rmc_index].matcost;
    nir->ecm[nir->nms].costul = ndi->rmc[rmc_index].labcost;
    nir->ecm[nir->nms].costi1 = ndi->rmc[rmc_index].itemcost;
    nir->ecm[nir->nms].qtym = 1;
    nir->ecm[nir->nms].qtyl = 1;
    nir->ecm[nir->nms].qtyi = 1;

    non_zero_measure_cost(nir->ecm[nir->nms].cost, nir->ecm[nir->nms].cms_measure_num);
    measure_economics_using_rmc(nir->nms, rmc_index);
    increment_measure_counter();
  }

  return;
}

// Air Conditioner Replacement

static void cooling_replacement(void) {
  int rmc_index, nc;
  float prop; // for interpolating costs

  if (ndi->htg[PRIMARY].retrofit_option == ES_HEATPUMP_REQUIRED)
    return;

  // Assume if installing a heat pump, existing AC units would not also be
  // replaced. MBG 2/10

  for (nc = 0; nc < ndi->num_clg; nc++) {
    if (ndi->clg[nc].system_type != CET_CENTRAL && ndi->clg[nc].system_type != CET_WINDOW)
      continue;
    if (ndi->clg[nc].replace == YES) { /* replacement required 9/09 */
      nir->measure_required[nir->nms] = YES;
      if (ndi->clg[nc].inc_sir == YES)
        nir->measure_priority[nir->nms] = MPS_EQUIP_REQUIRED;
      else
        nir->measure_priority[nir->nms] = MPS_EQUIP_REQUIRED_NO_SIR;
    } else if (ndi->clg[nc].seer >= 9.0)
      continue; /* don't replace efficient a/c */

    nir->ecm[nir->nms].dwelling_component_index = nc;
    //nir->ecm[nir->nms].intgr = ndi->clg[nc].system_type;

    /* Compute factor which when * by clload gives energy saved */
    float replacement_seer;
    switch (ndi->clg[nc].system_type) {
    case CET_CENTRAL:
      replacement_seer = ndi->key.new_central_ac_seer;
      break;
    case CET_WINDOW:
      replacement_seer = ndi->key.new_window_ac_seer;
      break;
    case CET_HEATPUMP:
      replacement_seer = ndi->key.new_heatpump_cooling_seer;
      break;
    default:
      ASSERT(FALSE, sprintf(msg, "Incorrect AC replacement type:%d", ndi->clg[nc].system_type));
    }
    nir->ecm[nir->nms].value = ndi->clg[nc].area_cooled_ratio * 3.413f * (1.0f / ndi->clg[nc].seer - 1.0f / replacement_seer) *
                     ndi->clgs.fraction_cooled;
    nir->clengysav[nir->nms] = nir->ecm[nir->nms].value * nir->coolload[PRE_RETROFIT_POST_INFIL];
    nir->htengysav[nir->nms] = 0.;
    nir->ecm[nir->nms].cms_measure_num = N_CMS_REPLACE_AC;
    nir->ecm[nir->nms].audit_section_id = N_COOLING;

    // remember only central and window systems, excludes heat pumps
    nir->ecm[nir->nms].cost = cooling_replacement_material_and_cost(ndi->clg[nc].size, ndi->clg[nc].system_type, &rmc_index);

    prop = nir->ecm[nir->nms].cost / (ndi->rmc[rmc_index].matcost + ndi->rmc[rmc_index].labcost);
    nir->ecm[nir->nms].costum = prop * ndi->rmc[rmc_index].matcost;
    nir->ecm[nir->nms].costul = prop * ndi->rmc[rmc_index].labcost;
    nir->ecm[nir->nms].costi1 = 0;
    nir->ecm[nir->nms].qtym = 1;
    nir->ecm[nir->nms].qtyl = 1;
    nir->ecm[nir->nms].qtyi = 1;

    add_material(ndi->clg[nc].code, 0, 0, rmc_index, ndi->rmc[rmc_index].material, ndi->rmc[rmc_index].retrofit_type, 
          MT_COOLING_EQUIPMENT, ndi->rmc[rmc_index].units, 1, nir->ecm[nir->nms].costum, "");
    add_material(ndi->clg[nc].code, 0, 0, rmc_index, ndi->rmc[rmc_index].material, ndi->rmc[rmc_index].retrofit_type, 
          MT_LABOR, ndi->rmc[rmc_index].units, 1, nir->ecm[nir->nms].costul, "");

    add_component_code(ndi->clg[nc].code);

    non_zero_measure_cost(nir->ecm[nir->nms].cost, nir->ecm[nir->nms].cms_measure_num);
    measure_economics_using_rmc(nir->nms, rmc_index);
    increment_measure_counter();
  }

  return;
}

// costs and material number for cooling system replacements of all kinds

static float cooling_replacement_material_and_cost(float kbtu, enum CLGEQUIPTYPE type, int *nmat) {
  float cost, costo, costcat[7];
  int rmc_index;

  if (type == CET_WINDOW) { /* Window units */

    if (kbtu <= 5) {
      rmc_index = N_MAT_REPLACE_AC_WIN_5K;
      cost = ndi->rmc[rmc_index].matcost + ndi->rmc[rmc_index].labcost + ndi->rmc[rmc_index].itemcost;
    } else if (kbtu >= 25) {
      rmc_index = N_MAT_REPLACE_AC_WIN_25K;
      cost = ndi->rmc[rmc_index].matcost + ndi->rmc[rmc_index].labcost + ndi->rmc[rmc_index].itemcost;
    } else if (kbtu <= 15) {
      rmc_index = N_MAT_REPLACE_AC_WIN_5K;
      costo = ndi->rmc[rmc_index].matcost + ndi->rmc[rmc_index].labcost + ndi->rmc[rmc_index].itemcost;
      cost = costo +
        (ndi->rmc[N_MAT_REPLACE_AC_WIN_15K].matcost + ndi->rmc[N_MAT_REPLACE_AC_WIN_15K].labcost + ndi->rmc[N_MAT_REPLACE_AC_WIN_15K].itemcost - costo) *
        (kbtu - 5.0f) / 10.0f;
    } else {
      rmc_index = N_MAT_REPLACE_AC_WIN_15K;
      costo = ndi->rmc[rmc_index].matcost + ndi->rmc[rmc_index].labcost + ndi->rmc[rmc_index].itemcost;
      cost = costo +
        (ndi->rmc[N_MAT_REPLACE_AC_WIN_25K].matcost + ndi->rmc[N_MAT_REPLACE_AC_WIN_25K].labcost + ndi->rmc[N_MAT_REPLACE_AC_WIN_25K].itemcost - costo) *
        (kbtu - 15.0f) / 10.0f;
    }

  } else {    /* Central units, heatpumps, none */

    // there are three sizes of cooling systems and we assign the cost based on one of
    // 7 cost categories basd on size interpoating and extrapolating the end points.

    switch (type){
    case CET_CENTRAL:
      costcat[1] = ndi->rmc[N_MAT_REPLACE_AC_CENTRAL_2T_24K].matcost + ndi->rmc[N_MAT_REPLACE_AC_CENTRAL_2T_24K].labcost + ndi->rmc[N_MAT_REPLACE_AC_CENTRAL_2T_24K].itemcost;
      costcat[3] = ndi->rmc[N_MAT_REPLACE_AC_CENTRAL_3T_36K].matcost + ndi->rmc[N_MAT_REPLACE_AC_CENTRAL_3T_36K].labcost + ndi->rmc[N_MAT_REPLACE_AC_CENTRAL_3T_36K].itemcost;
      costcat[5] = ndi->rmc[N_MAT_REPLACE_AC_CENTRAL_4T_48K].matcost + ndi->rmc[N_MAT_REPLACE_AC_CENTRAL_4T_48K].labcost + ndi->rmc[N_MAT_REPLACE_AC_CENTRAL_4T_48K].itemcost;
      break;
    case CET_HEATPUMP:
      costcat[1] = ndi->rmc[N_MAT_HEATPUMP_2T_24K].matcost + ndi->rmc[N_MAT_HEATPUMP_2T_24K].labcost + ndi->rmc[N_MAT_HEATPUMP_2T_24K].itemcost;
      costcat[3] = ndi->rmc[N_MAT_HEATPUMP_3T_36K].matcost + ndi->rmc[N_MAT_HEATPUMP_3T_36K].labcost + ndi->rmc[N_MAT_HEATPUMP_3T_36K].itemcost;
      costcat[5] = ndi->rmc[N_MAT_HEATPUMP_4T_48K].matcost + ndi->rmc[N_MAT_HEATPUMP_4T_48K].labcost + ndi->rmc[N_MAT_HEATPUMP_4T_48K].itemcost;
      break;
    default:
      ASSERT(FALSE, sprintf(msg, "Unknow cooling system type: %d", type));
    }

    // interpolate
    costcat[2] = (costcat[1] + costcat[3]) / 2.0f;
    costcat[4] = (costcat[3] + costcat[5]) / 2.0f;
    // and extrapolate
    costcat[0] = 2.0f * costcat[1] - costcat[2];
    costcat[6] = 2.0f * costcat[5] - costcat[4];

    // assign kbtu size into one of 7 size categories sz 0 to 6
    int size = (int)((kbtu + 3.0) / 6 - 3);   //truncates
    if (size < 0)
      size = 0;
    if (size > 6)
      size = 6;

    switch (size){
    case 0:
    case 1:
      rmc_index = (type == CET_CENTRAL) ? N_MAT_REPLACE_AC_CENTRAL_2T_24K : N_MAT_HEATPUMP_2T_24K;
      break;
    case 2:
    case 3:
    case 4:
      rmc_index = (type == CET_CENTRAL) ? N_MAT_REPLACE_AC_CENTRAL_3T_36K : N_MAT_HEATPUMP_3T_36K;
      break;
    case 5:
    case 6:
      rmc_index = (type == CET_CENTRAL) ? N_MAT_REPLACE_AC_CENTRAL_4T_48K : N_MAT_HEATPUMP_4T_48K;
      break;
    }
    cost = costcat[size];
  }
  *nmat = rmc_index;
  return (cost);
}

// Replace A/C's with Evaporative Cooler

static void cooling_replacement_evaporative(void) {
  int i, nc;
  int rmc_index;

  if (nir->consider_evaporative_cooler != 1)
    return;
  if (ndi->num_clg == 0)
    return;
  for (i = 0; i < ndi->num_clg; i++) {
    if (ndi->clg[i].system_type == CET_EVAPORATIVE)
      return;
  }
  /* Compute factor which when * by clload gives energy saved */
  nir->ecm[nir->nms].value =
      3.413f * (1.0f / ndi->clgs.avg_seer - 1.0f / nir->evaporative_cooler_eer) * ndi->clgs.fraction_cooled; // MBG 10/07
  nir->clengysav[nir->nms] = nir->ecm[nir->nms].value * nir->coolload[PRE_RETROFIT_POST_INFIL];
  nir->htengysav[nir->nms] = 0.;
  nir->ecm[nir->nms].cms_measure_num = N_CMS_EVAPORATIVE_COOLER;
  nir->ecm[nir->nms].audit_section_id = N_COOLING;

  rmc_index = N_MAT_EVAPORATIVE_COOLER;
  nir->ecm[nir->nms].cost = (ndi->rmc[rmc_index].matcost + ndi->rmc[rmc_index].labcost) + ndi->rmc[rmc_index].itemcost;

  nir->ecm[nir->nms].costum = ndi->rmc[rmc_index].matcost;
  nir->ecm[nir->nms].costul = ndi->rmc[rmc_index].labcost;
  nir->ecm[nir->nms].costi1 = ndi->rmc[rmc_index].itemcost;
  nir->ecm[nir->nms].qtym = 1;
  nir->ecm[nir->nms].qtyl = 1;
  nir->ecm[nir->nms].qtyi = 1;

  for (nc = 0; nc < ndi->num_clg; nc++) {
    add_component_code(ndi->clg[nc].code);
    nir->ecm[nir->nms].dwelling_component_index = nc;
  }

  non_zero_measure_cost(nir->ecm[nir->nms].cost, nir->ecm[nir->nms].cms_measure_num);
  measure_economics_using_rmc(nir->nms, rmc_index);
  increment_measure_counter();

  return;
}

// Install or Replace Heatpump

static void heatpump_replacement(void) {
  float htsys_repcstm; 
  float htsys_repcstl; 
  float load_fraction_met; 
  float inv_seaseff_post;
  float hpscopnw;

  float clgvaluepre, clgvaluepost;
  int rmc_index;

  if (ndi->htg[PRIMARY].fuel_type != ELECTRIC)
    return;

  if (ndi->htg[PRIMARY].retrofit_option != ES_EVALUATE_HEATPUMP && ndi->htg[PRIMARY].retrofit_option != ES_HEATPUMP_REQUIRED)
    return;

  // Currently restrict to electric systems for which user has specified
  // installation of a heatpump. However, program has been modified
  // to generate correct recommendations when other than electric existing
  // heating systems exist. MBG 9/07

  /* Add load fractions from replaced secondary systems and
     form component string. Note, they must be electric systems. */

  add_component_code(ndi->htg[PRIMARY].code);
  nir->ecm[nir->nms].dwelling_component_index = PRIMARY;

  load_fraction_met = ndi->htg[PRIMARY].percent_heat_supplied;
  for (int i = 1; i < ndi->num_htg; i++) {
    //if (ndi->htg[i].replace_system == YES && ndi->htg[i].fuel_type == ELECTRIC) {   // #228
    if (ndi->htg[i].replace_system == YES) {
      load_fraction_met += ndi->htg[i].percent_heat_supplied;
      add_component_code(ndi->htg[i].code);
    }
  }

  // hpscopnw = ndi->htg[PRIMARY].retrofit_afue_standard_eff;
  hpscopnw = ndi->htg[PRIMARY].retrofit_hspf;
  adjust_hspf(cwd->winter_hp_design_temp, &hpscopnw);
  hpscopnw /= 3.413f;

  // The above element of ndi->htg[PRIMARY] structure has dual units for
  //    hp and furnaces 10/07
  // Climate adjustment added 4/08 MBG

  // Handle the case where no existing cooling is present  ====NEW SYSTEM====

  if (ndi->num_clg == 0) {
    //  nir->ecm[nir->nms].value = -3.413f/seern[2];  Give free cooling if none existed
    //  nir->clengysav[nir->nms]=nir->ecm[nir->nms].value*nir->coolload[PRE_RETROFIT_POST_INFIL];
    nir->ecm[nir->nms].value = 0.0f;
    nir->clengysav[nir->nms] = 0.0f;

    //    nir->htengysav[nir->nms] = (1.0f-ndi->htg[PRIMARY].delivered_eff/hpscopnw)*
    //       nir->heatload[PRE_RETROFIT_POST_INFIL]/ndi->htg[PRIMARY].delivered_eff*ndi->htg[PRIMARY].percent_heat_supplied;

    inv_seaseff_post = load_fraction_met / hpscopnw / ndi->inf.post_duct_seal_efficiency;
    for (int i = 1; i < ndi->num_htg; i++) {
      if (ndi->htg[i].replace_system != YES || ndi->htg[i].fuel_type != ELECTRIC)
        // Eliminate removed systems - Corrected 8/16/11 MBG
        //    if(ndi->htg[i].replace_system != YES) Eliminate removed systems
        inv_seaseff_post += ndi->htg[i].percent_heat_supplied / ndi->htg[i].delivered_eff;
    }

    // #228 updated energy and economics for fuel switching
    heating_system_energy_savings(load_fraction_met, hpscopnw);

    //nir->htengysav[nir->nms] = nir->heatload[PRE_RETROFIT_POST_INFIL] * (1.0f / nir->sysht_seaseff - inv_seaseff_post);   // #228
    nir->ecm[nir->nms].value2 = inv_seaseff_post;

    //  Compute locally defined consumptions for call to fuelswitch
    //  consumption[0] = nir->heatload[PRE_RETROFIT_POST_INFIL] * ndi->htg[PRIMARY].percent_heat_supplied / ndi->htg[PRIMARY].delivered_eff;
    //  consumption[1] = nir->heatload[PRE_RETROFIT_POST_INFIL] * ndi->htg[PRIMARY].percent_heat_supplied / hpscopnw;
    //    Change over to replaceing multiple heating systems, 11/07

    // #228
    // consumption[0] = nir->heatload[PRE_RETROFIT_POST_INFIL] * (1.0f / nir->sysht_seaseff);
    // consumption[1] = nir->heatload[PRE_RETROFIT_POST_INFIL] * inv_seaseff_post;
    // fuel[0] = ndi->htg[PRIMARY].fuel_type;
    // fuel[1] = ELECTRIC;

    nir->ecm[nir->nms].cms_measure_num = N_CMS_INSTALL_OR_REPLACE_HEATPUMP;
    nir->ecm[nir->nms].audit_section_id = N_HEATING;      // no cooling system

    nir->ecm[nir->nms].cost = cooling_replacement_material_and_cost(36, CET_HEATPUMP, &rmc_index); // temporarily assume 3 ton/36kBtu
    nir->ecm[nir->nms].rmc_index = rmc_index;
    ndi->rmc[rmc_index].ecm_index = nir->nms;

    htsys_repcstl = ndi->htg[PRIMARY].labor_cost_standard_eff;
    htsys_repcstm = ndi->htg[PRIMARY].material_cost_standard_eff;

    nir->ecm[nir->nms].cost = htsys_repcstl + htsys_repcstm;
    nir->ecm[nir->nms].costum = htsys_repcstm;
    nir->ecm[nir->nms].costul = htsys_repcstl;
    nir->ecm[nir->nms].costi1 = 0;

    nir->ecm[nir->nms].costi1 = 0;
    nir->ecm[nir->nms].qtym = 1;
    nir->ecm[nir->nms].qtyl = 1;
    nir->ecm[nir->nms].qtyi = 1;

    add_material("", 0, 0, rmc_index, ndi->rmc[rmc_index].material, ndi->rmc[rmc_index].retrofit_type, 
            MT_HEATING_EQUIPMENT, ndi->rmc[rmc_index].units, 1, nir->ecm[nir->nms].costum, "");
    add_material("", 0, 0, rmc_index, ndi->rmc[rmc_index].material, ndi->rmc[rmc_index].retrofit_type, 
            MT_LABOR, ndi->rmc[rmc_index].units, 1, nir->ecm[nir->nms].costul, "");

    nir->mslife[nir->nms] = ndi->rmc[rmc_index].life;

    // # 228
    //measure_economics_with_fuel_switch(nir->nms, ndi->rmc[rmc_index].life, fuel, consumption);

    if (ndi->htg[PRIMARY].retrofit_option == ES_HEATPUMP_REQUIRED) {
      nir->measure_required[nir->nms] = YES;
      nir->measure_priority[nir->nms] = MPS_EQUIP_REQUIRED;
      if (ndi->htg[PRIMARY].inc_sir != YES)
        nir->measure_priority[nir->nms] = MPS_EQUIP_REQUIRED_NO_SIR;
    }

    non_zero_measure_cost(nir->ecm[nir->nms].cost, nir->ecm[nir->nms].cms_measure_num);
    // #228 with fuel switching
    heating_system_dollar_savings(load_fraction_met, hpscopnw, rmc_index);\
    increment_measure_counter();

    return;
  }

  // === REPLACE EXISTING COOLING SYSTEM(s) ===
  
  // iflag = 0;     // dead assign
  // ac_frephpsize = 0.0f;
  nir->ecm[nir->nms].value = 0.0f;
  clgvaluepre = 0.0f;
  clgvaluepost = 0.0f;

  for (int nc = 0; nc < ndi->num_clg; nc++) {
    //  if(ac_type[nc]==4) { iflag=0; break; } /* do not replace evaporative clr */
    //  if(ndi->clg[nc].seer>=9.0) continue;  /* don't replace efficient a/c */
    //  if(iflag==1) continue;         /* only first heatpump or central AC evaluated */
    nir->ecm[nir->nms].dwelling_component_index = nc;
    // iflag = 1; // dead assignment removal MJF 12/2018

    /* Compute factor which when * by clload gives energy saved */

    clgvaluepre += ndi->clg[nc].area_cooled_ratio * 3.413f * 1.0f / ndi->clg[nc].seer * ndi->clgs.fraction_cooled;
    if ((nc != 0 && ndi->clg[nc].eliminate_system != YES) || ndi->clg[nc].system_type == CET_EVAPORATIVE)
      clgvaluepost += ndi->clg[nc].area_cooled_ratio * 3.413f * 1.0f / ndi->clg[nc].seer * ndi->clgs.fraction_cooled;
    else {
      clgvaluepost +=
          ndi->clg[nc].area_cooled_ratio * 3.413f * 1.0f / ndi->key.new_heatpump_cooling_seer * ndi->clgs.fraction_cooled;
      add_component_code(ndi->clg[nc].code);
      ndi->clgs.size_replaced_by_heatpump += ndi->clg[nc].size;
    }

    nir->ecm[nir->nms].value += ndi->clg[nc].area_cooled_ratio * 3.413f * 1.0f / ndi->clg[nc].seer * ndi->clgs.fraction_cooled;
  }

  nir->ecm[nir->nms].value = clgvaluepre - clgvaluepost;
  nir->clengysav[nir->nms] = nir->ecm[nir->nms].value * nir->coolload[PRE_RETROFIT_POST_INFIL];

  //  nir->htengysav[nir->nms] = (1.0f-ndi->htg[PRIMARY].delivered_eff/hpscopnw)*
  //         nir->heatload[PRE_RETROFIT_POST_INFIL]/ndi->htg[PRIMARY].delivered_eff*ndi->htg[PRIMARY].percent_heat_supplied;

  inv_seaseff_post = load_fraction_met / hpscopnw / ndi->inf.post_duct_seal_efficiency;
  for (int i = 1; i < ndi->num_htg; i++) {
    if (ndi->htg[i].replace_system != YES || ndi->htg[i].fuel_type != ELECTRIC)
      // Eliminate removed systems - Corrected 8/16/11 MBG
      //    if(ndi->htg[i].replace_system != YES)   Eliminate removed systems
      inv_seaseff_post += ndi->htg[i].percent_heat_supplied / ndi->htg[i].delivered_eff;
  }

  // #228 updated energy and economics for fuel switching
  heating_system_energy_savings(load_fraction_met, hpscopnw);

  //nir->htengysav[nir->nms] = nir->heatload[PRE_RETROFIT_POST_INFIL] * (1.0f / nir->sysht_seaseff - inv_seaseff_post);
  nir->ecm[nir->nms].value2 = inv_seaseff_post;

  //  Compute locally defined consumptions for call to fuelswitch

  //  consumption[0] = nir->heatload[PRE_RETROFIT_POST_INFIL] * ndi->htg[PRIMARY].percent_heat_supplied / ndi->htg[PRIMARY].delivered_eff;
  //  consumption[1] = nir->heatload[PRE_RETROFIT_POST_INFIL] * ndi->htg[PRIMARY].percent_heat_supplied / hpscopnw;

  // #228
  //    Change over to replaceing multiple heating systems, 11/07
  // consumption[0] = nir->heatload[PRE_RETROFIT_POST_INFIL] * (1.0f / nir->sysht_seaseff);
  // consumption[1] = nir->heatload[PRE_RETROFIT_POST_INFIL] * inv_seaseff_post;
  // fuel[0] = ndi->htg[PRIMARY].fuel_type;
  // fuel[1] = ELECTRIC;

  nir->ecm[nir->nms].cms_measure_num = N_CMS_INSTALL_OR_REPLACE_HEATPUMP;

  // assign audit+section_id based on lions share of energy savings
  if (nir->htengysav[nir->nms] > nir->clengysav[nir->nms]) {
    nir->ecm[nir->nms].audit_section_id = N_HEATING;
  }
  else {
    nir->ecm[nir->nms].audit_section_id = N_COOLING;
  }

  // Bug #171
  //nir->ecm[nir->nms].cost = cooling_replacement_material_and_cost(ndi->clg[nc].size, CET_HEATPUMP, &n);
  nir->ecm[nir->nms].cost = cooling_replacement_material_and_cost(ndi->clgs.size_replaced_by_heatpump, CET_HEATPUMP, &rmc_index);
  nir->ecm[nir->nms].rmc_index = rmc_index;
  ndi->rmc[rmc_index].ecm_index = nir->nms;

  //  prop = nir->ecm[nir->nms].cost / (ndi->rmc[rmc_index].matcost+ndi->rmc[rmc_index].labcost);
  //  nir->ecm[nir->nms].costum = prop * ndi->rmc[rmc_index].matcost;
  //  nir->ecm[nir->nms].costul = prop * ndi->rmc[rmc_index].labcost;
  //  nir->ecm[nir->nms].costi1 = 0;

  htsys_repcstl = ndi->htg[PRIMARY].labor_cost_standard_eff;
  htsys_repcstm = ndi->htg[PRIMARY].material_cost_standard_eff;

  nir->ecm[nir->nms].cost = htsys_repcstl + htsys_repcstm;
  nir->ecm[nir->nms].costum = htsys_repcstm;
  nir->ecm[nir->nms].costul = htsys_repcstl;
  nir->ecm[nir->nms].costi1 = 0;

  nir->ecm[nir->nms].qtym = 1;
  nir->ecm[nir->nms].qtyl = 1;
  nir->ecm[nir->nms].qtyi = 1;

  add_material("", 0, 0, rmc_index, ndi->rmc[rmc_index].material, ndi->rmc[rmc_index].retrofit_type, 
        MT_HEATING_EQUIPMENT, ndi->rmc[rmc_index].units, 1, nir->ecm[nir->nms].costum, "");
  add_material("", 0, 0, rmc_index, ndi->rmc[rmc_index].material, ndi->rmc[rmc_index].retrofit_type, 
        MT_LABOR, ndi->rmc[rmc_index].units, 1, nir->ecm[nir->nms].costul, "");

  nir->mslife[nir->nms] = ndi->rmc[rmc_index].life;
  //measure_economics_with_fuel_switch(nir->nms, ndi->rmc[rmc_index].life, fuel, consumption);

  if (ndi->htg[PRIMARY].retrofit_option == ES_HEATPUMP_REQUIRED && ndi->clg[0].replace == YES) {
    nir->measure_required[nir->nms] = YES;
    nir->measure_priority[nir->nms] = MPS_EQUIP_REQUIRED;
    if (ndi->htg[PRIMARY].inc_sir != YES && ndi->clg[0].inc_sir != YES)
      nir->measure_priority[nir->nms] = MPS_EQUIP_REQUIRED_NO_SIR;
  }
  
  non_zero_measure_cost(nir->ecm[nir->nms].cost, nir->ecm[nir->nms].cms_measure_num);
  // #228 with fuel switching
  heating_system_dollar_savings(load_fraction_met, hpscopnw, rmc_index);
  increment_measure_counter();

  return;
}

// Infiltration Reduction (if data entered)

static void infiltration_reduction(void) {
  int rmc_index;

  if (nir->infiltration_treatment == INF_DEFAULT ||
      nir->infiltration_treatment == INF_INCIDENTAL_COST)   // no savingsj to compute
    return;

  /* Compute factor which when * by clload gives energy saved */
  nir->clengysav[nir->nms] = nir->coolengy[PRE_RETROFIT_PRE_INFIL] - nir->coolengy[PRE_RETROFIT_POST_INFIL];
  nir->htengysav[nir->nms] = nir->heatengy[PRE_RETROFIT_PRE_INFIL] - nir->heatengy[PRE_RETROFIT_POST_INFIL];
  nir->ecm[nir->nms].cms_measure_num = N_CMS_INFILTRATION_REDUCTION;
  nir->ecm[nir->nms].audit_section_id = N_DUCTS_AND_INFILTRATION;

  nir->measure_priority[nir->nms] = MPS_INFILTRATION_REDUCTION;
  rmc_index = N_MAT_INFILTRATION_REDUCTION;

  nir->ecm[nir->nms].dwelling_component_index = NOT_APPLICABLE;

  if (nir->infiltration_treatment == INF_FULL_MEASURE)
    nir->ecm[nir->nms].cost = ndi->inf.air_leak_red_cost;
  else    // NO_COST_COMPUTE_SAVINGS_ONLY, make the SIR artificially low with very high cost
    nir->ecm[nir->nms].cost = 10000.;

  nir->ecm[nir->nms].costum = 0;
  nir->ecm[nir->nms].costul = 0;
  nir->ecm[nir->nms].costi1 = 0;
  nir->ecm[nir->nms].qtym = 1;
  nir->ecm[nir->nms].qtyl = 1;
  nir->ecm[nir->nms].qtyi = 1;

  nir->ecm[nir->nms].costi2 = ndi->inf.air_leak_red_cost;
  //nir->ecm[nir->nms].typei2 = MT_INFILTRATION_REDUCTION;
  nir->ecm[nir->nms].typei2 = MT_MISCELLANEOUS_SUPPLIES;
  STRCPY(nir->ecm[nir->nms].desci2, "Infiltration Reduction");

  non_zero_measure_cost(nir->ecm[nir->nms].cost, nir->ecm[nir->nms].cms_measure_num);
  measure_economics_using_rmc(nir->nms, rmc_index);
  increment_measure_counter();

  return;
}

// Duct Insulation

void duct_insulation(void) {
  float dhld = 0., dcld = 0., ductdu, tempht = 0., tempcl = 0., supply_temp;
  int m, nc, ncl = NOT_APPLICABLE, nht = NOT_APPLICABLE;
  int rmc_index;

  if (nir->ductarea < 1.0f)
    return;
  if (ndi->htg[PRIMARY].system_type == HE_GRAVITY_FURNACE || ndi->htg[PRIMARY].system_type == HE_FORCED_AIR_FURNACE ||
      ndi->htg[PRIMARY].system_type == HE_HEAT_PUMP)
    nht = 1;
  /* must be forced air or gravity furnace or heatpump */
  for (nc = 0; nc < ndi->num_clg; nc++) {
    if (ndi->clg[nc].system_type == CET_CENTRAL || ndi->clg[nc].system_type == CET_HEATPUMP)
      ncl = nc;
  }
  /*  must be central A/C or heatpump for cooling savings */
  if (nht == NOT_APPLICABLE && ncl == NOT_APPLICABLE)
    return; /* No ducted equipment */

  /*  nir->ductarea = duct_perim/12.0f * duct_lngth;  */
  if (ndi->htg[PRIMARY].system_type == HE_FORCED_AIR_FURNACE)
    supply_temp = 120.; /* Forced air */
  else
    supply_temp = 95.; /* Heatpumps  */
  ductdu = 0.65f - 1.0f / (1.0f / 0.65f + ndi->key.new_duct_insulation_r_value);
  for (m = 1; m <= MONTHS; m++) {
    if (ndi->htg[PRIMARY].duct_location == SUBSPACE && nht != NOT_APPLICABLE) {
      tempht = ductdu * (supply_temp - nir->tucsbsp[m]) * nir->ductarea * nir->htgmengy[1][m] / ndi->htg[PRIMARY].output_capacity / 1000.0f;
      dhld += tempht;
    } else if (ndi->htg[PRIMARY].duct_location == ATTIC) {
      if (ncl > -1) {
        tempcl = ductdu * (nir->tattic[m] - 55.0f) * nir->ductarea * nir->clgmengy[1][m] / ndi->clg[ncl].size / 1000.0f;
        if (tempcl > 0)
          dcld += tempcl;
      }
      if (nht > -1) {
        tempht = ductdu * (supply_temp - nir->tattic[m]) * nir->ductarea * nir->htgmengy[1][m] / ndi->htg[PRIMARY].output_capacity / 1000.0f;
        if (tempht > 0)
          dhld += tempht;
      }
    }
  /*    fprintf(tempout,"%5d%7.2f%7.2f%7.1f%7.2f%7.3f%7.3f%7.1f%7.3f\n",
   *        m,tempht,dhld,nir->tucsbsp[m],nir->htgmengy[1][m],
   *        tempcl,dcld,nir->tattic[m],nir->clgmengy[1][m]); */  
  }

  nir->htengysav[nir->nms] = dhld / nir->sysht_seaseff;
  if (ncl > -1)
    nir->clengysav[nir->nms] = dcld / ndi->clg[ncl].seer * 3.413f;
  else
    nir->clengysav[nir->nms] = 0.0;

  nir->ecm[nir->nms].cms_measure_num = N_CMS_DUCT_INSULATION;
  nir->ecm[nir->nms].audit_section_id = N_DUCTS_AND_INFILTRATION;
  nir->ecm[nir->nms].dwelling_component_index = NOT_APPLICABLE;

  rmc_index = N_MAT_DUCT_INSULATION;

  nir->ecm[nir->nms].cost = (ndi->rmc[rmc_index].matcost + ndi->rmc[rmc_index].labcost) * nir->ductarea + ndi->rmc[rmc_index].itemcost;
  nir->ecm[nir->nms].costum = ndi->rmc[rmc_index].matcost;
  nir->ecm[nir->nms].costul = ndi->rmc[rmc_index].labcost;
  nir->ecm[nir->nms].costi1 = ndi->rmc[rmc_index].itemcost;
  nir->ecm[nir->nms].qtym = nir->ductarea;
  nir->ecm[nir->nms].qtyl = nir->ductarea;
  nir->ecm[nir->nms].qtyi = 1;

  non_zero_measure_cost(nir->ecm[nir->nms].cost, nir->ecm[nir->nms].cms_measure_num);
  measure_economics_hvac(nir->nms, rmc_index);
  increment_measure_counter();

  return;
}

// User Defined Measure/ Itemized Costs

static void itemized_cost_measures(void) {
  int nudm = 0, im, nc;
  int fuel_type;

  for (nc = 0; nc < ndi->num_itc; nc++) {
    ndi->itc[nc].ecm_index = NOT_APPLICABLE;
    if (ndi->itc[nc].savings > .0001) {

      im = nir->ecm[nir->nms].cms_measure_num = N_CMS_ITEMIZED_COST + nudm;
      nir->ecm[nir->nms].audit_section_id = N_ITEMIZED_COST; 
      STRCPY(nir->measure_name[im], ndi->itc[nc].measure);
      nir->ecm[nir->nms].component_id = ndi->itc[nc].component_id;
      nir->meas_type[im] = CMT_OTHER;

      // we may have a generic fuel type that needs
      // to be set to one of our standard fuel types

      fuel_type = ndi->itc[nc].fuel; // base ONE here
      if (fuel_type == PRIMARY_HEAT)
        fuel_type = ndi->htg[PRIMARY].fuel_type;
      if (fuel_type == WATER_HEAT)
        fuel_type = ndi->dwh.exist_fuel_type_id;
      if (fuel_type == 0)
        fuel_type = NATURAL_GAS; // just so fuel_type is never undefined

      nir->ecm[nir->nms].dwelling_component_index = nc;
      nir->ecm[nir->nms].rmc_index = NOT_APPLICABLE;

      nir->ecm[nir->nms].ensav = ndi->itc[nc].savings;
      nir->ecm[nir->nms].dlsav = ndi->itc[nc].savings * fuel_cost(fuel_type) / fuel_heat_content(fuel_type);
      nir->ecm[nir->nms].cost = ndi->itc[nc].cost;

      nir->ecm[nir->nms].costum = 0;
      nir->ecm[nir->nms].costul = 0;
      nir->ecm[nir->nms].costi1 = 0;
      nir->ecm[nir->nms].qtym = 0;
      nir->ecm[nir->nms].qtyl = 0;
      nir->ecm[nir->nms].qtyi = 0;
      //nir->ecm[nir->nms].material_id = N_MAT_FROM_AUDIT;                     // signal
      //nir->ecm[nir->nms].measure_id = ndi->itc[nc].measure_id; // database ID

      // We will always lump the cost into costi2  Issue #12 MJF 1/20
      //if (ndi->itc[nc].measure_id == 0) { // lumped cost only if no measure referenced
        nir->ecm[nir->nms].costi2 = ndi->itc[nc].cost;
        //nir->ecm[nir->nms].typei2 = MT_ITEMIZED_MEASURE_COST;
        nir->ecm[nir->nms].typei2 = MT_NONE;
        if (strlen(ndi->itc[nc].material)) // copy our material description
          STRCPY(nir->ecm[nir->nms].desci2, ndi->itc[nc].material);
        else
          STRCPY(nir->ecm[nir->nms].desci2, "Itemized Material");
      //}

      measure_economics_baseline(nir->nms, (int)(ndi->itc[nc].life + 0.5), fuel_type);
      if (nir->ecm[nir->nms].sir >= ndi->key.minimum_acceptable_sir){
        nir->pntitcretrofit_materials[nc] = 1;
        ndi->itc[nc].ecm_index = nir->nms;
      }
      /* Allows for printing materials only if btcr > ndi->key.minimum_acceptable_sir */
      increment_measure_counter();
      nudm++;
    } else
      nir->pntitcretrofit_materials[nc] = 1;
  }
  return;
}

// Lighting Retrofits

static void lighting_replacement(void) {

  for (int nc = 0; nc < ndi->num_ltg; nc++) {
    LTG ltg = ndi->ltg[nc];
    if (ltg.new_lamp_count <= 0)
      continue;

    int nm = nir->nms;  // the next [n]eat [m]easure number

    float exist_mmbtu = ltg.exist_lamp_watts * ltg.exist_hours_per_day * ltg.exist_lamp_count * 365.0f / KWH_PER_MMBTU / 1000.0f;
    float new_mmbtu = ltg.new_lamp_watts * ltg.new_hours_per_day * ltg.new_lamp_count * 365.0f / KWH_PER_MMBTU / 1000.0f;
    float annual_saved_mmbtu = exist_mmbtu - new_mmbtu;
    float cost = (ltg.install_cost_per_lamp + ltg.added_cost_per_lamp) * ltg.new_lamp_count + ltg.added_cost;
    float life = ltg.new_lifetime_hrs / ltg.new_hours_per_day / 365.0f;  // years

    if (life > MAX_LIGHT_LIFE) life = MAX_LIGHT_LIFE;

    float dollars_per_mmbtu;
    { // interpolate the PW LCC electricity cost base on the decimal life
      int l1 = (int)(life);
      int l2 = l1 + 1;
      float h1 = pw_fuel_cost(ELECTRIC, l1);
      float h2 = pw_fuel_cost(ELECTRIC, l2);
      dollars_per_mmbtu = h1 + (life - (float)(l1)) * (h2 - h1); // interpolate
    }
    float lifetime_dollars_saved = dollars_per_mmbtu * annual_saved_mmbtu;
    float sir = lifetime_dollars_saved / cost;

    if (sir >= ndi->key.minimum_acceptable_sir) {

      nir->ecm[nm].cms_measure_num = N_CMS_LIGHTING_RETROFITS;
      nir->ecm[nm].audit_section_id = N_LIGHTING;
      nir->ecm[nir->nms].dwelling_component_index = nc;

      int rmc_index = NMAT + nir->nother_materials;       // tacked onto end of rmc array
      nir->ecm[nm].rmc_index = rmc_index;
      ndi->rmc[rmc_index].ecm_index = nm;

      STRCPY(ndi->rmc[rmc_index].material, "Replacement Lighting");
      { char r_type[TYPE_LEN + 1];
      sprintf(r_type, "%s %0.1f watts", lighting_type_name(ltg.new_lamp_type), ltg.new_lamp_watts);
      STRCPY(ndi->rmc[rmc_index].retrofit_type, r_type);
      }
      STRCPY(ndi->rmc[rmc_index].units, "Each");
      STRCPY(nir->ecm[nm].components, ltg.code);

      ndi->rmc[rmc_index].quant = ltg.new_lamp_count;
      nir->ecm[nm].cost = cost;

      nir->ecm[nm].costum = ltg.install_cost_per_lamp;
      nir->ecm[nm].costul = ltg.added_cost_per_lamp;
      nir->ecm[nm].costi1 = ltg.added_cost;

      nir->ecm[nm].qtym += ltg.new_lamp_count;
      nir->ecm[nm].qtyl += ltg.new_lamp_count;
      nir->ecm[nm].qtyi += 1;

      nir->dslfsav[nm] = lifetime_dollars_saved;
      nir->blengysav[nm] = nir->ecm[nm].ensav = annual_saved_mmbtu;
      nir->ecm[nm].dlsav = nir->bldlsav[nm] = nir->ecm[nir->nms].ensav * fuel_cost(ELECTRIC) / fuel_heat_content(ELECTRIC);
      nir->ecm[nm].sir = sir;
      nir->ecm[nm].lifetime = life;                      // don't report an overall life, since each ltg record has its own lifetime and SIR
      nir->npv[nm] = lifetime_dollars_saved - cost;

      nir->nother_materials++;
      non_zero_measure_cost(nir->ecm[nm].cost, nir->ecm[nm].cms_measure_num);
      increment_measure_counter();
    }
  }   // next ltg, individual measures
}

// A/C Tunuep 

static void cooling_tuneup(void) {
  int nc;
  float seertuned, fract;
  int rmc_index;

  for (nc = 0; nc < ndi->num_clg; nc++) {
    if (ndi->clg[nc].system_type != CET_CENTRAL && ndi->clg[nc].system_type != CET_WINDOW)
      continue;                        /* must not be heatpump */
    if (ndi->clg[nc].tune_up == YES) { /* tuneup required 9/09 */
      nir->measure_required[nir->nms] = YES;
      if (ndi->clg[nc].inc_sir == YES)
        nir->measure_priority[nir->nms] = MPS_EQUIP_REQUIRED;
      else
        nir->measure_priority[nir->nms] = MPS_EQUIP_REQUIRED_NO_SIR;
    } else if (ndi->clg[nc].seer >= 10.0)
      continue; /* don't replace efficient a/c */

    // Compute factor of efficiency increase based on existing efficiency
    fract = 1.25;
    if (ndi->clg[nc].seer < 5.0)
      fract = 1.36f;
    else if (ndi->clg[nc].seer > 5.0 && ndi->clg[nc].seer < 6.5)
      fract = 1.25f + 0.11f * (6.5f - ndi->clg[nc].seer);
    else if (ndi->clg[nc].seer > 8.5)
      fract = 1.00f + 0.167f * (10.0f - ndi->clg[nc].seer);

    nir->ecm[nir->nms].dwelling_component_index = nc;
    /* Compute factor which when * by clload gives energy saved */
    seertuned = nir->ecm[nir->nms].value2 = fract * ndi->clg[nc].seer;
    /* temporarily increase seer by fract */
    nir->ecm[nir->nms].value = ndi->clg[nc].area_cooled_ratio * 3.413f * (1.0f / ndi->clg[nc].seer - 1.0f / seertuned);
    nir->clengysav[nir->nms] = nir->ecm[nir->nms].value * nir->coolload[PRE_RETROFIT_POST_INFIL];
    nir->htengysav[nir->nms] = 0.;
    nir->ecm[nir->nms].cms_measure_num = N_CMS_TUNE_UP_AC;
    nir->ecm[nir->nms].audit_section_id = N_COOLING;

    rmc_index = N_MAT_TUNE_UP_AC;
    nir->ecm[nir->nms].cost = (ndi->rmc[rmc_index].matcost + ndi->rmc[rmc_index].labcost) + ndi->rmc[rmc_index].itemcost;

    nir->ecm[nir->nms].costum = ndi->rmc[rmc_index].matcost;
    nir->ecm[nir->nms].costul = ndi->rmc[rmc_index].labcost;
    nir->ecm[nir->nms].costi1 = ndi->rmc[rmc_index].itemcost;
    nir->ecm[nir->nms].qtym = 1;
    nir->ecm[nir->nms].qtyl = 1;
    nir->ecm[nir->nms].qtyi = 1;

    add_component_code(ndi->clg[nc].code);
    non_zero_measure_cost(nir->ecm[nir->nms].cost, nir->ecm[nir->nms].cms_measure_num);
    measure_economics_using_rmc(nir->nms, rmc_index);
    increment_measure_counter();
  }

  return;
}

// Window Sealing 

static void window_sealing(void) {
  int m, nc;
  float leakage_savings_factor, dhld, dcld;
  float lata, /*  Latent load of affected windows after retrofit */
      latb;   /*  Latent load of affected windows before retrofit  */
  /*  value2      Correction for dcld = latent load after
                   installation of the measure   */
  int rmc_index;

  for (nc = 0; nc < ndi->num_win; nc++) {
    lata = latb = 0.f;
    if (ndi->win[nc].retrofit_option != WS_EVALUATE_ALL && ndi->win[nc].retrofit_option != WS_SEAL) // Sealing not requested
      continue;
    if (ndi->win[nc].retrofit_option == WS_EVALUATE_ALL && ndi->win[nc].leak_coef <= SEALED_WINDOW_LEAKAGE_COEF) // No Savings
      continue;
    if (ndi->win[nc].retrofit_option == WS_SEAL) {
      nir->measure_required[nir->nms] = YES;
      nir->measure_priority[nir->nms] = MPS_ENVELOPE_REQUIRED; // Measure considered required
      if (ndi->win[nc].inc_sir != YES)
        nir->measure_priority[nir->nms] = MPS_ENVELOPE_REQUIRED_NO_SIR;
    }
    nir->ecm[nir->nms].dwelling_component_index = nc;

    leakage_savings_factor = window_sealing_leak_coef(ndi->win[nc].leak_coef) / ndi->win[nc].leak_coef;    // #255

    for (m = 1; m <= MONTHS; m++) {
      nir->dfreht[nir->nms][m] = 0.0f;
      nir->dua[nir->nms][m] += 60.f * RHOCAIR * nir->wn_leak_cfm[nc][m] * (1.0f - leakage_savings_factor);
      if (leakage_savings_factor < 1.0f) {
        lata += get_latent_infil_load(cwd->avg_drybulb_temp[m], cwd->avg_wetbulb_temp[m], leakage_savings_factor * nir->wn_leak_cfm[nc][m]) * 24.0f * cwd->days_in_month[m];
        latb += get_latent_infil_load(cwd->avg_drybulb_temp[m], cwd->avg_wetbulb_temp[m],         nir->wn_leak_cfm[nc][m]) * 24.0f * cwd->days_in_month[m];
      }
    } 
    add_component_code(ndi->win[nc].code);

    rmc_index = N_MAT_WINDOW_SEALING;
    nir->ecm[nir->nms].cost +=
        (ndi->rmc[rmc_index].matcost + ndi->rmc[rmc_index].labcost + ndi->rmc[rmc_index].itemcost) * ndi->win[nc].number;

    nir->ecm[nir->nms].costum = ndi->rmc[rmc_index].matcost;
    nir->ecm[nir->nms].costul = ndi->rmc[rmc_index].labcost;
    nir->ecm[nir->nms].costi1 = ndi->rmc[rmc_index].itemcost;
    nir->ecm[nir->nms].qtym += ndi->win[nc].number;
    nir->ecm[nir->nms].qtyl += ndi->win[nc].number;
    nir->ecm[nir->nms].qtyi += ndi->win[nc].number; // units of itemized cost is each window

    // Add added cost from input

    nir->ecm[nir->nms].cost += ndi->win[nc].cost_seal * ndi->win[nc].number;
    additional_cost(ndi->win[nc].cost_seal * ndi->win[nc].number);

    annual_energy_load_change(nir->dua[nir->nms], nir->dfreht[nir->nms], &dhld, &dcld);
    nir->ecm[nir->nms].value2 = (float)((latb - lata) / 1.e6 * ndi->clgs.fraction_cooled);
    dcld += nir->ecm[nir->nms].value2;
    nir->htengysav[nir->nms] = dhld / nir->sysht_seaseff;
    nir->clengysav[nir->nms] = dcld / ndi->clgs.avg_seer * 3.413f * ndi->clgs.fraction_cooled; // MBG 10/07
    nir->ecm[nir->nms].cms_measure_num = N_CMS_WINDOW_SEALING;
    nir->ecm[nir->nms].audit_section_id = N_WINDOW;

    non_zero_measure_cost(nir->ecm[nir->nms].cost, nir->ecm[nir->nms].cms_measure_num);
    measure_economics_using_rmc(nir->nms, rmc_index);
    increment_measure_counter();
  }

  return;
}

// Window Replacement

static void window_replacement(void) {
  int m, nc;
  float unew, duaw, dsaw, leakage_savings_factor, dhld, dcld;
  float lata, /*  Latent load of affected windows after retrofit */
      latb;   /*  Latent load of affected windows before retrofit  */
  /*  value2      Correction for dcld = latent load after
                   installation of the measure   */
  int rmc_index;

  for (nc = 0; nc < ndi->num_win; nc++) {
    lata = latb = 0.f;
    if (!(ndi->win[nc].retrofit_option == WS_EVALUATE_ALL || ndi->win[nc].retrofit_option == WS_REPLACE))
      continue;

    if (ndi->win[nc].retrofit_option == WS_REPLACE) {
      nir->measure_required[nir->nms] = YES;
      nir->measure_priority[nir->nms] = MPS_ENVELOPE_REQUIRED; // Measure considered required
      if (ndi->win[nc].inc_sir != YES)
        nir->measure_priority[nir->nms] = MPS_ENVELOPE_REQUIRED_NO_SIR;
    }
    
    nir->ecm[nir->nms].dwelling_component_index = nc;
    //    unew = 1.0f/WDWREPR;
    unew = ndi->key.new_standard_window_u_value;
    duaw = (ndi->win[nc].u_value - unew) * ndi->win[nc].area_gross;
    //    dsaw = (ndi->win[nc].shgc_summer-WDREPSGF)*ndi->win[nc].area_glazing;
    // Convert to whole window SHGC and areas.  2/09
    //    dsaw = (ndi->win[nc].shgc_summer-WDREPSGF)*ndi->win[nc].area_gross;
    dsaw = (ndi->win[nc].shgc_summer - ndi->key.new_standard_window_shgc) * ndi->win[nc].area_gross;

    leakage_savings_factor = WINDOW_MEC_LEAKAGE_COEF / ndi->win[nc].leak_coef;
    for (m = 1; m <= MONTHS; m++) {
      if (m > 3 && m < 10)
        nir->dfreht[nir->nms][m] += dsaw * (cwd->solar_load[m][ndi->win[nc].solar_orient] * ndi->win[nc].shade_factor_summer +
                                  cwd->solar_load[m][SOLAR_DIFFUSE]);
      else
        nir->dfreht[nir->nms][m] += dsaw * (cwd->solar_load[m][ndi->win[nc].solar_orient] * ndi->win[nc].shade_factor_winter +
                                  cwd->solar_load[m][SOLAR_DIFFUSE]);
      // note that wn_leak_cfm[nc][m] is already reduced for MAX_WINDOW_DOOR_PERCENT
      nir->dua[nir->nms][m] += duaw + 60.f * RHOCAIR * nir->wn_leak_cfm[nc][m] * (1.0f - leakage_savings_factor);
      lata += get_latent_infil_load(cwd->avg_drybulb_temp[m], cwd->avg_wetbulb_temp[m], leakage_savings_factor * nir->wn_leak_cfm[nc][m]) * 24.0f * cwd->days_in_month[m];
      latb += get_latent_infil_load(cwd->avg_drybulb_temp[m], cwd->avg_wetbulb_temp[m], nir->wn_leak_cfm[nc][m]) * 24.0f * cwd->days_in_month[m];
    }
    add_component_code(ndi->win[nc].code);

    rmc_index = N_MAT_WINDOW_REPLACEMENT;
    nir->ecm[nir->nms].cost += (ndi->rmc[rmc_index].matcost + ndi->rmc[rmc_index].labcost) * ndi->win[nc].area_gross +
                     ndi->rmc[rmc_index].itemcost * ndi->win[nc].number;

    nir->ecm[nir->nms].costum = ndi->rmc[rmc_index].matcost;
    nir->ecm[nir->nms].costul = ndi->rmc[rmc_index].labcost;
    nir->ecm[nir->nms].costi1 = ndi->rmc[rmc_index].itemcost;
    nir->ecm[nir->nms].qtym += ndi->win[nc].area_gross;
    nir->ecm[nir->nms].qtyl += ndi->win[nc].area_gross;
    nir->ecm[nir->nms].qtyi += ndi->win[nc].number; // units is each window

    // Add added cost from input
    nir->ecm[nir->nms].cost += ndi->win[nc].cost_replace * ndi->win[nc].number;
    additional_cost(ndi->win[nc].cost_replace * ndi->win[nc].number);

    annual_energy_load_change(nir->dua[nir->nms], nir->dfreht[nir->nms], &dhld, &dcld);
    nir->ecm[nir->nms].value2 = (float)((latb - lata) / 1.e6 * ndi->clgs.fraction_cooled);
    dcld += nir->ecm[nir->nms].value2;
    nir->htengysav[nir->nms] = dhld / nir->sysht_seaseff;
    nir->clengysav[nir->nms] = dcld / ndi->clgs.avg_seer * 3.413f * ndi->clgs.fraction_cooled; // MBG 10/07
    nir->ecm[nir->nms].cms_measure_num = N_CMS_WINDOW_REPLACEMENT;
    nir->ecm[nir->nms].audit_section_id = N_WINDOW;

    non_zero_measure_cost(nir->ecm[nir->nms].cost, nir->ecm[nir->nms].cms_measure_num);
    measure_economics_using_rmc(nir->nms, rmc_index);
    increment_measure_counter();
  }

  return;
}

// Door Replacement

static void door_replacement(void) {
  //  int m,n,nc,it,temp;
  float unew, duad, dsad, leakage_savings_factor, dhld, dcld;
  int m, nc;
  float lata, /*  Latent load of affected doors after retrofit */
      latb;   /*  Latent load of affected doors before retrofit  */
  /*  value2      Correction for dcld = latent load after
                   installation of the measure   */
  int rmc_index;

  for (nc = 0; nc < ndi->num_dor; nc++) {
    if (ndi->dor[nc].door_type == SINGLE_SLIDING_GLASS || ndi->dor[nc].door_type == DOUBLE_SLIDING_GLASS)
      continue; // Not replacing sliding glass doors
    lata = latb = 0.f;
    if (ndi->dor[nc].replace == YES) {
      nir->measure_required[nir->nms] = YES;
      nir->measure_priority[nir->nms] = MPS_ENVELOPE_REQUIRED; // Measure considered required
      if (ndi->dor[nc].inc_sir != YES)
        nir->measure_priority[nir->nms] = MPS_ENVELOPE_REQUIRED_NO_SIR;
    }
    nir->ecm[nir->nms].dwelling_component_index = nc;
    unew = 0.4f;
    duad = (ndi->dor[nc].u_value - unew) * ndi->dor[nc].area;
    dsad = duad * WALL_ABSORPTIVITY / ndi->key.annual_outside_film_coeff;

    leakage_savings_factor = NEW_DOOR_LEAKAGE_COEF / ndi->dor[nc].leak_coef;
    for (m = 1; m <= MONTHS; m++) {
      nir->dfreht[nir->nms][m] += dsad * (cwd->solar_load[m][ndi->dor[nc].solar_orient] + cwd->solar_load[m][SOLAR_DIFFUSE]);
      nir->dua[nir->nms][m] += duad + 60.f * RHOCAIR * nir->dr_leak_cfm[nc][m] * (1.0f - leakage_savings_factor);
      //      nir->dua[nir->nms][m] += duad;
      lata += get_latent_infil_load(cwd->avg_drybulb_temp[m], cwd->avg_wetbulb_temp[m], leakage_savings_factor * nir->dr_leak_cfm[nc][m]) * 24.0f * cwd->days_in_month[m];
      latb += get_latent_infil_load(cwd->avg_drybulb_temp[m], cwd->avg_wetbulb_temp[m], nir->dr_leak_cfm[nc][m]) * 24.0f * cwd->days_in_month[m];
    }
    add_component_code(ndi->dor[nc].code);

    rmc_index = N_MAT_DOOR_REPLACEMENT;
    nir->ecm[nir->nms].cost +=
        (ndi->rmc[rmc_index].matcost + ndi->rmc[rmc_index].labcost + ndi->rmc[rmc_index].itemcost) * ndi->dor[nc].number;

    nir->ecm[nir->nms].costum = ndi->rmc[rmc_index].matcost;
    nir->ecm[nir->nms].costul = ndi->rmc[rmc_index].labcost;
    nir->ecm[nir->nms].costi1 = ndi->rmc[rmc_index].itemcost;
    nir->ecm[nir->nms].qtym += ndi->dor[nc].number;
    nir->ecm[nir->nms].qtyl += ndi->dor[nc].number;
    nir->ecm[nir->nms].qtyi += ndi->dor[nc].number; // units is each door

    // Add added cost from input
    nir->ecm[nir->nms].cost += ndi->dor[nc].cost * ndi->dor[nc].number;
    additional_cost(ndi->dor[nc].cost * ndi->dor[nc].number);

    annual_energy_load_change(nir->dua[nir->nms], nir->dfreht[nir->nms], &dhld, &dcld);

    nir->ecm[nir->nms].value2 = (float)((latb - lata) / 1.e6 * ndi->clgs.fraction_cooled);
    dcld += nir->ecm[nir->nms].value2;
    //fprintf(stderr, "\nDOOR REP latb:%f lata:%f  dcld:%f", latb, lata, dcld);

    nir->htengysav[nir->nms] = dhld / nir->sysht_seaseff;
    nir->clengysav[nir->nms] = dcld / ndi->clgs.avg_seer * 3.413f * ndi->clgs.fraction_cooled; // MBG 10/07
    //fprintf(stderr, "\nDOOR REP seer:%f fraction:%f", ndi->clgs.avg_seer, ndi->clgs.fraction_cooled);
    nir->ecm[nir->nms].cms_measure_num = N_CMS_DOOR_REPLACEMENT;
    nir->ecm[nir->nms].audit_section_id = N_DOOR;

    non_zero_measure_cost(nir->ecm[nir->nms].cost, nir->ecm[nir->nms].cms_measure_num);
    measure_economics_using_rmc(nir->nms, rmc_index);
    increment_measure_counter();
  }

  return;
}

// Water Heater Tank Insulation

static void water_heater_tank_insulation(void) {
  float dwh_area, dwh_gal, dwh_upre;
  // int m, n, nm, dwhFuel;
  int m;
  float dwh_ambtemp[MONTHS + 1], tia, dwh_eff;
  float dwhRpre;
  int rmc_index;

  if (ndi->dwh.exist_tank_wrap == YES) // already insulated
    return;
  if (ndi->dwh.replace == YES)
    return;     // Prevent tank wrap if replacement is required (1/11/10)
  if (ndi->dwh.exist_type == WH_TANKLESS)
    return;     // No tank to insulate #270
  if (ndi->dwh.exist_energy_factor == 0)
    return;   // #270

  // dwhFuel = ndi->dwh.exist_fuel_type_id; // make code easier to read

  if (ndi->dwh.exist_fuel_type_id == 0)
    return;

  // here is the new code that determines if the
  // rvalue of the insulation is derived from the type
  // and thickness, or directly from the rvalue on the
  // label,  MJF 5/01

  if (ndi->dwh.exist_rvalue > 0.1)
    dwhRpre = ndi->dwh.exist_rvalue;
  else
    dwhRpre = water_heater_insulation_rpi(ndi->dwh.exist_insul_type_id) * ndi->dwh.exist_insul_thick;

  // estimate the dwh surface area using the configuration (determined
  // by fuel type) and the capacity in gallons.

  if (ndi->dwh.exist_gal > 0)
    dwh_gal = ndi->dwh.exist_gal;
  else
    dwh_gal = 30.0f;

  if (ndi->dwh.exist_fuel_type_id == WH_ELECTRIC)
    dwh_area = 5.08f + 0.2975f * dwh_gal;
  else
    dwh_area = 3.92f + 0.2740f * dwh_gal;

  // here are some conditions used to decide if we are going
  // to do the tank wrap

  if (dwhRpre > 8.0f && ndi->dwh.exist_tank_location_id == HEATED)
    return; // Conditioned, R > 8
  if (dwhRpre > 9.9f && ndi->dwh.exist_fuel_type_id != WH_ELECTRIC)
    return; // Non Electric, R > 9.9
  if (dwhRpre > 12.4f)
    return; // R > 12.4

  tia = (ndi->key.daytime_cooling_setpoint + ndi->key.nighttime_cooling_setpoint + ndi->key.daytime_heating_setpoint +
         ndi->key.nighttime_heating_setpoint) /
        4.0f; // our average indoor temp
  dwh_upre = (float)(1.0f / (0.66 + dwhRpre));
  switch (ndi->dwh.exist_tank_location_id) { // set our monthly dwh ambient temp array
  case UNHEATED: {                           // Nonconditioned
    for (m = 1; m <= MONTHS; m++)
      dwh_ambtemp[m] = cwd->avg_daytime_temp[m];
    break;
  }
  case UNINTENTIONALLY_HEATED: { // Unintentionall Conditoned
    for (m = 1; m <= MONTHS; m++)
      dwh_ambtemp[m] = nir->tucsbsp[m];
    break;
  }
  default: { // Conditioned
    for (m = 1; m <= MONTHS; m++)
      dwh_ambtemp[m] = tia;
    break;
  }
  }

  nir->ecm[nir->nms].cms_measure_num = N_CMS_WATER_HEATER_TANK_INSULATION;
  nir->ecm[nir->nms].audit_section_id = N_WATER_HEATING;
  nir->ecm[nir->nms].dwelling_component_index = NOT_APPLICABLE;

  nir->ecm[nir->nms].ensav = 0.0f;
  for (m = 1; m <= MONTHS; m++) {
    nir->ecm[nir->nms].ensav += (float)(dwh_area * (dwh_upre - 1. / (1. / dwh_upre + ndi->key.new_dwh_blanket_r_value)) *
                              (DWH_TEMP - dwh_ambtemp[m]) * 24 * cwd->days_in_month[m]);
  }

  dwh_eff = ndi->dwh.exist_energy_factor;


  nir->ecm[nir->nms].ensav *= (float)(1.0e-6 / dwh_eff);
  nir->ecm[nir->nms].dlsav = nir->ecm[nir->nms].ensav * fuel_cost(ndi->dwh.exist_fuel_type_id) / fuel_heat_content(ndi->dwh.exist_fuel_type_id);

  // new assigment of savings to baseload array (MJF 1/01)
  nir->blengysav[nir->nms] = nir->ecm[nir->nms].ensav;
  nir->bldlsav[nir->nms] = nir->ecm[nir->nms].dlsav;

  rmc_index = N_MAT_WATER_HEATER_TANK_INSULATION;
  nir->ecm[nir->nms].rmc_index = rmc_index;
  ndi->rmc[rmc_index].ecm_index = nir->nms;

  nir->ecm[nir->nms].cost = (ndi->rmc[rmc_index].matcost + ndi->rmc[rmc_index].labcost) + ndi->rmc[rmc_index].itemcost;

  nir->ecm[nir->nms].costum = ndi->rmc[rmc_index].matcost;
  nir->ecm[nir->nms].costul = ndi->rmc[rmc_index].labcost;
  nir->ecm[nir->nms].costi1 = ndi->rmc[rmc_index].itemcost;
  nir->ecm[nir->nms].qtym = 1;
  nir->ecm[nir->nms].qtyl = 1;
  nir->ecm[nir->nms].qtyi = 1;
  nir->ecm[nir->nms].lifetime = ndi->rmc[rmc_index].life;

  non_zero_measure_cost(nir->ecm[nir->nms].cost, nir->ecm[nir->nms].cms_measure_num);
  measure_economics_baseline(nir->nms, ndi->rmc[rmc_index].life, ndi->dwh.exist_fuel_type_id);
  increment_measure_counter();

  return;
}

// Water Heater Pipe Insulation

static void water_heater_pipe_insulation(void) {
  // int m, n, nm, dwhFuel;
  int m;
  float tempval, dwh_ambtemp[MONTHS + 1], tia;
  // Make constant or have read in
  float dwh_pipe_length = 5.0f, dwh_pipe_diam_in = 0.84f;
  float kins = 0.0225f, ins_thickness_in = 0.5f, ho_ins = 1.35f;
  float ho_unins, tavg, tdelta, kair, dwh_eff;
  float titemp, tatemp;
  // float debug1;
  float qconv, qrad, qins, qsaved, r1, r2;
  int rmc_index;

  if (ndi->dwh.exist_pipe_insul == YES) // already insulated
    return;
  if (ndi->dwh.exist_energy_factor == 0)
    return;   // #270

  // dwhFuel = ndi->dwh.exist_fuel_type_id; // make code easier to read

  if (ndi->dwh.exist_fuel_type_id == 0)
    return;

  tia = (ndi->key.daytime_cooling_setpoint + ndi->key.nighttime_cooling_setpoint + ndi->key.daytime_heating_setpoint +
         ndi->key.nighttime_heating_setpoint) /
        4.0f;
  switch (ndi->dwh.exist_tank_location_id) {
  case UNHEATED: { // Nonconditioned
    for (m = 1; m <= MONTHS; m++)
      dwh_ambtemp[m] = cwd->avg_daytime_temp[m];
    break;
  }
  case UNINTENTIONALLY_HEATED: { // Unintentionall Conditoned
    for (m = 1; m <= MONTHS; m++)
      dwh_ambtemp[m] = nir->tucsbsp[m];
    break;
  }
  default: { // Conditioned
    for (m = 1; m <= MONTHS; m++)
      dwh_ambtemp[m] = tia;
    break;
  }
  }

  nir->ecm[nir->nms].cms_measure_num = N_CMS_WATER_HEATER_PIPE_INSULATION;
  nir->ecm[nir->nms].audit_section_id = N_WATER_HEATING;
  nir->ecm[nir->nms].dwelling_component_index = NOT_APPLICABLE;

  nir->ecm[nir->nms].ensav = 0.0f;
  r1 = dwh_pipe_diam_in / 24.0f;
  r2 = r1 + ins_thickness_in / 12.0f;
  tempval = PI * dwh_pipe_diam_in / 12.0f;
  titemp = (DWH_TEMP + 460.0f) / 100.0f;
  for (m = 1; m <= MONTHS; m++) {
    tavg = (dwh_ambtemp[m] + DWH_TEMP) / 2.0f;
    tdelta = DWH_TEMP - dwh_ambtemp[m];
    kair = (float)(2.059e-5 * tavg + 0.01334);
    // debug1 = (float)(tdelta / dwh_pipe_length * EXPC(-.008698 * tavg));
    ho_unins = (float)(23.1 * kair * POWC((tdelta / dwh_pipe_length * EXPC(-.008698 * tavg)), 0.25));
    qconv = tempval * ho_unins * tdelta;
    tatemp = (dwh_ambtemp[m] + 460.0f) / 100.0f;
    qrad = (float)(tempval * 0.1714 * 0.8f * (POWC(titemp, 4.0f) - POWC(tatemp, 4.0f)));
    qins = (float)(2.0f * PI * tdelta / (log((double)(r2 / r1)) / kins + 1.0f / (r2 * ho_ins)));
    qsaved = qconv + qrad - qins;
    nir->ecm[nir->nms].ensav += qsaved * 24 * cwd->days_in_month[m];
  }

  dwh_eff = ndi->dwh.exist_energy_factor;

  nir->ecm[nir->nms].ensav *= (float)(1.0e-6 / dwh_eff * dwh_pipe_length);
  nir->ecm[nir->nms].dlsav = nir->ecm[nir->nms].ensav * fuel_cost(ndi->dwh.exist_fuel_type_id) / fuel_heat_content(ndi->dwh.exist_fuel_type_id);

  // new assigment of savings to baseload array (MJF 1/01)
  nir->blengysav[nir->nms] = nir->ecm[nir->nms].ensav;
  nir->bldlsav[nir->nms] = nir->ecm[nir->nms].dlsav;

  rmc_index = N_MAT_WATER_HEATER_PIPE_INSULATION;
  nir->ecm[nir->nms].rmc_index = rmc_index;
  ndi->rmc[rmc_index].ecm_index = nir->nms;

  nir->ecm[nir->nms].cost = (ndi->rmc[rmc_index].matcost + ndi->rmc[rmc_index].labcost) + ndi->rmc[rmc_index].itemcost;
  nir->ecm[nir->nms].costum = ndi->rmc[rmc_index].matcost;
  nir->ecm[nir->nms].costul = ndi->rmc[rmc_index].labcost;
  nir->ecm[nir->nms].costi1 = ndi->rmc[rmc_index].itemcost;
  nir->ecm[nir->nms].qtym = 1;
  nir->ecm[nir->nms].qtyl = 1;
  nir->ecm[nir->nms].qtyi = 1;

  non_zero_measure_cost(nir->ecm[nir->nms].cost, nir->ecm[nir->nms].cms_measure_num);
  measure_economics_baseline(nir->nms, ndi->rmc[rmc_index].life, ndi->dwh.exist_fuel_type_id);
  if (nir->ecm[nir->nms].sir >= ndi->key.minimum_acceptable_sir)      // TODO shouldn't this be done in second pass?
    ndi->rmc[rmc_index].quant = 1;
  increment_measure_counter();

  return;
}

// Low-Flow Showerheads

static void water_heater_shower_heads(void) {
  float shwr_oldW, shwr_newW, shwr_delT_old = 50, shwr_delT_new = 54;
  float dwh_eff;
  int rmc_index;

  if (ndi->dwh.shower_usage_per_day == 0 || ndi->dwh.shower_heads == 0 || ndi->dwh.shower_gpm <= 0.1)
    return;
  if (ndi->dwh.exist_energy_factor == 0)
    return;   // #270

  dwh_eff = ndi->dwh.exist_energy_factor;

  shwr_oldW = 365.0f * ndi->dwh.shower_usage_per_day * ndi->dwh.shower_gpm / 1000.0f;

  shwr_newW = 365.0f * ndi->dwh.shower_usage_per_day * ndi->key.new_showerhead_gpm / 1000.0f;

  nir->ecm[nir->nms].ensav = nir->blengysav[nir->nms] = (float)((shwr_oldW * shwr_delT_old - shwr_newW * shwr_delT_new) * 8340.0f / dwh_eff * 1.0e-6);

  nir->ecm[nir->nms].dlsav = nir->ecm[nir->nms].ensav * fuel_cost(ndi->dwh.exist_fuel_type_id) / fuel_heat_content(ndi->dwh.exist_fuel_type_id);

  // new assigment of savings to baseload array (MJF 1/01)
  nir->blengysav[nir->nms] = nir->ecm[nir->nms].ensav;
  nir->bldlsav[nir->nms] = nir->ecm[nir->nms].dlsav;

  rmc_index = N_MAT_LOW_FLOW_SHOWERHEADS;
  nir->ecm[nir->nms].rmc_index = rmc_index;
  ndi->rmc[rmc_index].ecm_index = nir->nms;

  nir->ecm[nir->nms].cost =
      ((ndi->rmc[rmc_index].matcost + ndi->rmc[rmc_index].labcost) + ndi->rmc[rmc_index].itemcost) * ndi->dwh.shower_heads;
  nir->ecm[nir->nms].costum = ndi->rmc[rmc_index].matcost;
  nir->ecm[nir->nms].costul = ndi->rmc[rmc_index].labcost;
  nir->ecm[nir->nms].costi1 = ndi->rmc[rmc_index].itemcost;
  nir->ecm[nir->nms].qtym += ndi->dwh.shower_heads;
  nir->ecm[nir->nms].qtyl += ndi->dwh.shower_heads;
  nir->ecm[nir->nms].qtyi += ndi->dwh.shower_heads;

  nir->ecm[nir->nms].cms_measure_num = N_CMS_LOW_FLOW_SHOWERHEADS;
  nir->ecm[nir->nms].audit_section_id = N_WATER_HEATING;
  nir->ecm[nir->nms].dwelling_component_index = NOT_APPLICABLE;

  non_zero_measure_cost(nir->ecm[nir->nms].cost, nir->ecm[nir->nms].cms_measure_num);
  measure_economics_baseline(nir->nms, ndi->rmc[rmc_index].life, ndi->dwh.exist_fuel_type_id);
  if (nir->ecm[nir->nms].sir >= ndi->key.minimum_acceptable_sir)    // TODO should be done in second pass?
    ndi->rmc[rmc_index].quant = (float)ndi->dwh.shower_heads;
  increment_measure_counter();

  return;
}

// Water Heater Replacement (electric and gas)

static void water_heater_replacement(void) {
  int rmc_index;

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
  float dhld = 0.0f;              // delta heating load
  float dcld = 0.0f;              // resulting delta of cooling load
  double a1 = 0.00019738;         // MM:Coefficients for performance adjustment due to ambient temperature
  double a2 = 0.01842;
  double a3 = 1.3222625;

  // Here's our check to make sure that we have valid input
  // for the replacment

  if (water_heater_replace_data_check(ndi->dwh) == FALSE)
    return;

  if (ndi->dwh.replace == YES) { /* replacement required 9/09 */
    nir->measure_required[nir->nms] = YES;
    if (ndi->dwh.inc_sir == YES)
      nir->measure_priority[nir->nms] = MPS_EQUIP_REQUIRED;
    else
      nir->measure_priority[nir->nms] = MPS_EQUIP_REQUIRED_NO_SIR;
  }

  // Do our assignments from our set of input.  Note conditionals
  // guarantee certain inputs will always have a suitable default

  // dwh_fuel[PRE_RETROFIT] = ndi->dwh.exist_fuel_type_id;
  // dwh_fuel[POST_RETROFIT] = ndi->dwh.replace_fuel_type_id;

  dwh_type[PRE_RETROFIT] = ndi->dwh.exist_type;
  dwh_type[POST_RETROFIT] = ndi->dwh.replace_type;

  // #270 defaults for some of these are set upstream

  EF[PRE_RETROFIT]  = ndi->dwh.exist_energy_factor;
  EF[POST_RETROFIT] = ndi->dwh.replace_energy_factor;

  RE[PRE_RETROFIT]  = ndi->dwh.exist_recovery_efficiency;
  RE[POST_RETROFIT] = ndi->dwh.replace_recovery_efficiency;

  Pon[PRE_RETROFIT] = ndi->dwh.exist_input * 1000.0f;
  if (ndi->dwh.exist_input_units_id == WH_KW)
    Pon[PRE_RETROFIT] *= 3.415f;
  Pon[POST_RETROFIT] = ndi->dwh.replace_input * 1000.0f;
  if (ndi->dwh.replace_input_units_id == WH_KW)
    Pon[POST_RETROFIT] *= 3.415f;

  Tanksz[PRE_RETROFIT] = ndi->dwh.exist_gal;
  Tanksz[POST_RETROFIT] = ndi->dwh.replace_gal;


  // Set tank ambiant temperature

  tia = (ndi->key.daytime_cooling_setpoint + 
         ndi->key.nighttime_cooling_setpoint + 
         ndi->key.daytime_heating_setpoint +
         ndi->key.nighttime_heating_setpoint) /
        4.0f;
        
  switch (ndi->dwh.exist_tank_location_id) {
  case UNHEATED: { // Nonconditioned
    for (int m = 1; m <= MONTHS; m++)
      dwh_ambtemp[m] = cwd->avg_drybulb_temp[m];
    break;
  }
  case UNINTENTIONALLY_HEATED: { // Unintentionally Conditoned
    for (int m = 1; m <= MONTHS; m++)
      dwh_ambtemp[m] = nir->tucsbsp[m];
    break;
  }
  default: { // Conditioned
    for (int m = 1; m <= MONTHS; m++)
      dwh_ambtemp[m] = tia;
    break;
  }
  }

  if (ndi->gnl.avg_no_occupants < 4.0f)
    age3 = ndi->gnl.avg_no_occupants;
  else
    age3 = 3.0f;

  if (ndi->gnl.avg_no_occupants < 4.0f)
    age2 = 0.0f;
  else
    age2 = ndi->gnl.avg_no_occupants - 3.0f;

  if (ndi->gnl.avg_no_occupants < 3.0f)
    athome = 0.0f;
  else
    athome = 1.0f;

  // now do our before/after calculations

  for (int unit = PRE_RETROFIT; unit <= POST_RETROFIT; unit++) {

    vol = -1.78f + 0.9744f * ndi->gnl.avg_no_occupants + 10.5178f * age2 + 15.3052f * age3 - 0.1277f * DWH_TEMP +
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

    //if (EF[unit] <= RE[unit] || dwh_fuel[unit] != WH_ELECTRIC) {    // Standard water heater

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
        break;

      default:
        ASSERT(FALSE, sprintf(msg, "Unrecognized water heater type: %d",dwh_type[unit]));
        break;

    } // equipment type switch

    dwh_annual_cons[unit] /= 1.e6f;
  } // End loop through existing / retrofit units

  // Effects on Free Heat
  // Note there is no effect for standard water heaters or heaters in unconditioned space
  for (int m = 1; m <= MONTHS; m++) {
    if (ndi->dwh.exist_tank_location_id == HEATED) {
      nir->dfreht[nir->nms][m] += (float)(dwh_clg[m] / 24.0f);
    } else {
      nir->dfreht[nir->nms][m] = 0.0f;
    }
    nir->dua[nir->nms][m] = 0.0f;
  }

  nir->ecm[nir->nms].ensav = nir->blengysav[nir->nms] = (dwh_annual_cons[0] - dwh_annual_cons[1]); // MMBtu
  nir->ecm[nir->nms].dlsav = nir->bldlsav[nir->nms] =
      dwh_annual_cons[0] * fuel_cost(ndi->dwh.exist_fuel_type_id) / fuel_heat_content(ndi->dwh.exist_fuel_type_id) -
      dwh_annual_cons[1] * fuel_cost(ndi->dwh.replace_fuel_type_id) / fuel_heat_content(ndi->dwh.replace_fuel_type_id);
  nir->ecm[nir->nms].cms_measure_num = N_CMS_WATER_HEATER_REPLACEMENT;
  nir->ecm[nir->nms].audit_section_id = N_WATER_HEATING;
  nir->ecm[nir->nms].dwelling_component_index = NOT_APPLICABLE;

  annual_energy_load_change(nir->dua[nir->nms], nir->dfreht[nir->nms], &dhld, &dcld);
  nir->htengysav[nir->nms] = dhld / nir->sysht_seaseff;
  nir->clengysav[nir->nms] = dcld / ndi->clgs.avg_seer * 3.413f * ndi->clgs.fraction_cooled;
  nir->htdlsav[nir->nms] = nir->htengysav[nir->nms] * CompFuelCost();
  nir->cldlsav[nir->nms] = nir->clengysav[nir->nms] * fuel_cost(ELECTRIC) * 1000.0f / 3.413f;

  nir->ecm[nir->nms].ensav += nir->htengysav[nir->nms] + nir->clengysav[nir->nms];
  nir->ecm[nir->nms].dlsav += nir->htdlsav[nir->nms] + nir->cldlsav[nir->nms];

  // Material cost and life now comes directly from the input
  // and our material list for this and all other 'equipment library'
  // items goes into the other_materials array

  nir->ecm[nir->nms].cost = ndi->dwh.replace_install_cost + ndi->dwh.replace_added_cost;

  nir->ecm[nir->nms].costum = 0;
  nir->ecm[nir->nms].costul = 0;
  nir->ecm[nir->nms].costi1 = 0;
  nir->ecm[nir->nms].qtym = 0;
  nir->ecm[nir->nms].qtyl = 0;
  nir->ecm[nir->nms].qtyi = 0;
  //nir->ecm[nir->nms].material_id = N_MAT_FROM_AUDIT; // signal no material in mat library
  //nir->ecm[nir->nms].measure_id = -1;

  nir->ecm[nir->nms].costi2 = ndi->dwh.replace_install_cost;
  nir->ecm[nir->nms].typei2 = MT_HOT_WATER_EQUIPMENT;
  STRCPY(nir->ecm[nir->nms].desci2, ndi->dwh.replace_manufacturer);
  STRCAT(nir->ecm[nir->nms].desci2, " - ");
  STRCAT(nir->ecm[nir->nms].desci2, ndi->dwh.replace_model);

  nir->ecm[nir->nms].costi3 = ndi->dwh.replace_added_cost;
  nir->ecm[nir->nms].typei3 = MT_LABOR;
  STRCPY(nir->ecm[nir->nms].desci3, "Installation Labor");

  int life = ndi->dwh.replace_life;
  nir->dslfsav[nir->nms] = pw_fuel_cost(ndi->dwh.exist_fuel_type_id, life) * dwh_annual_cons[PRE_RETROFIT] -
                 pw_fuel_cost(ndi->dwh.replace_fuel_type_id, life) * dwh_annual_cons[POST_RETROFIT] +
                 nir->clengysav[nir->nms] * pw_fuel_cost(ELECTRIC, life) +
                 nir->htengysav[nir->nms] * DCompFuelCost(life);

  non_zero_measure_cost(nir->ecm[nir->nms].cost, nir->ecm[nir->nms].cms_measure_num);

  nir->ecm[nir->nms].sir = calculate_sir(nir->dslfsav[nir->nms], nir->ecm[nir->nms].cost);
  nir->ecm[nir->nms].lifetime = life;
  nir->npv[nir->nms] = nir->dslfsav[nir->nms] - nir->ecm[nir->nms].cost;

  // if (cmds.debug_level & D_NORMAL) {
  //   fprintf(stderr, "DWH, Energy Saved: %f\n", nir->ecm[nir->nms].ensav);
  //   fprintf(stderr, "DWH, Dollars Saved: %f\n", nir->ecm[nir->nms].dlsav);
  //   fprintf(stderr, "DWH, Cost: %f\n", nir->ecm[nir->nms].cost);
  //   fprintf(stderr, "DWH, BTCR: %f\n", nir->ecm[nir->nms].sir);
  // }

  if (nir->ecm[nir->nms].sir >= ndi->key.minimum_acceptable_sir || ndi->dwh.replace == YES) {
    //char change_description[STRING_LEN];

    rmc_index = NMAT + nir->nother_materials;       // tacked onto end of rmc array
    nir->ecm[nir->nms].rmc_index = rmc_index;
    ndi->rmc[rmc_index].ecm_index = nir->nms;

    ndi->rmc[rmc_index].quant = 1;
    STRCPY(ndi->rmc[rmc_index].material, "New Water Heater");
    STRNCPY(ndi->rmc[rmc_index].retrofit_type, ndi->dwh.replace_model, TYPE_LEN);
    // ndi->rmc[NMAT + other_materials].type[TYPE_LEN] = '\0';
    STRCPY(ndi->rmc[rmc_index].units, "Each");

    //sprintf(change_description, "Replace %s with %s", water_heater_type[PRE_RETROFIT], water_heater_type[POST_RETROFIT]);
    //add_component_code_once(change_description);

    nir->ecm[nir->nms].rmc_index = rmc_index;
    ndi->rmc[rmc_index].ecm_index = nir->nms;

    nir->nother_materials++;
  }

  increment_measure_counter();
  return;
}

// Refrigerator Replacement
// Added 1/30/01
//
// Special note.  This is the first of the ecm routines to use
// data directly from the ndi-> structure rather than a separate
// set of global variables assigned in ioconv.c.  See the special
// note in the ioconv.c module where the ndi->ref variables used to
// be accessed.

static void refrigerator_replacement(void) {

  // local variables used in analysis

  char source_exist;         // 'T' for AHAM table or label, 'M' for metering
  float Cn;                  // annual kWh consumption of new     , all corrections applied
  float Ce;                  // annual kWh consumption of existing, all corrections applied
  float space_temp;          // ambient temperature around refrigerator (tia or measured)
  float kwh_per_day;         // existing refrigerator kWh/day, unadjusted (except for open_factor and defrost for metered data)
  float age_factor;          // factor adjusting refrig consumption for age
  float defrost_factor;      // factor to adjust metered consumption for defrost cycles
  float min_defrost = 12.0f; // assumed minutes duration of defrost cycle
  float doorsealfactor;             // fraction degradation due to door seal condition (Blasnik)
  float occupancyfactor;            // occupancy factor (Blasnik)
  float totalfactor_exist = 0.0f;   // overall factor for existing unit
  float totalfactor_replace = 0.0f; // overall factor for replacement unit
  float Tia = 75.0f;                // AHAM standard ambient temperature for ratings.

  float annual_sav;          // annual kWh savings
  int rmc_index;

  // local variables used as placehold for constant
  // candidates for parameter set definitions table

  // float open_factor=0.882f;        // factor to adjust metered cons for door openings

  if ((ndi->ref.meter_energy_reading < 0.001f && ndi->ref.label_kwh_per_year < 0.001f) || ndi->ref.replace_kwh_per_year < 0.001f)
    return; // No input to compute measure

  // decide if we are given AHAM/label or metering information and
  // set the source character flag appropriately

  if (ndi->ref.label_kwh_per_year > 0.001f && ndi->ref.label_year_id != 0) {

    // Existing unit consumption from AHAM or label
    source_exist = 'T';
    space_temp = Tia; // AHAM base temperature
    kwh_per_day = ndi->ref.label_kwh_per_year / 365.0f;
  } else if (ndi->ref.meter_energy_reading > 0.001f && ndi->ref.meter_energy_interval > 0.001) {

    // Existing unit cons. from metered data
    source_exist = 'M';
    space_temp = ndi->ref.meter_temperature;
    if (ndi->ref.meter_includes_defrost == YES) {
      ndi->ref.meter_energy_interval -= min_defrost;
      ndi->ref.meter_energy_reading -= ndi->key.single_defrost_kwh;
    }
    kwh_per_day = ndi->ref.meter_energy_reading / ndi->ref.meter_energy_interval * 60.0f * 24.0f;
    //   kwh_per_day /= open_factor;  Deleted 5/05 in favor of occupancy dep factor
  } else // did not get enough data, so we are returning w/o analysis
    return;

  // make adjustments for unit being in unconditioned space *****/

  if (ndi->ref.location_id == HEATED || ndi->ref.location_id == 0) { // In conditioned space
    Cn = ndi->ref.replace_kwh_per_year;
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
      if (ndi->ref.location_id == UNHEATED) { // Determine consumptions in uncond. space
        monthsptemp = cwd->avg_drybulb_temp[m];
        lowlimit = 30.0f; // Space temp min of 35 F;
        upperlimit = Tia + 20.0f;
      }      // Space temp max of 20 + room temp
      else { // Determine consumptions in unintentionally cond. space
        monthsptemp = nir->tucsbsp[m];     // an adjusted sub-basement for unintentionally conditioned space
        lowlimit = 45.0f; // Space temp min of 45 F;
        upperlimit = Tia + 10.0f;
      } // Space temp max of 10 + room temp
      uncond_temp[m] = MAX(monthsptemp, lowlimit);
      uncond_temp[m] = MIN(uncond_temp[m], upperlimit);

      Cn += ndi->ref.replace_kwh_per_year / 365 * cwd->days_in_month[m] * (1.0f - 0.025f * (Tia - uncond_temp[m]));
      Ce += kwh_per_day * cwd->days_in_month[m] * (1.0f - 0.025f * (space_temp - uncond_temp[m]));
    }
  }

  // adjust existing unit consumption for door seal condition

  if (source_exist == 'T') {
    switch (ndi->ref.door_seal_condition_id) {
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

  switch ((int)ndi->gnl.avg_no_occupants) {
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
  if (ndi->gnl.avg_no_occupants > 5.0f)
    occupancyfactor = 0.16f;
  totalfactor_exist += occupancyfactor;
  totalfactor_replace += occupancyfactor;

  // adjust for age of unit if existing unit consumption is from label/AHAM

  if (source_exist == 'T') {
    switch (ndi->ref.label_year_id) {
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

  if (source_exist == 'T' || ndi->ref.meter_manual_defrost == YES)
    defrost_factor = 1.0f;
  else
    defrost_factor = 1.08f;
  Ce *= defrost_factor;

  // now we have a complete picture and can add to our
  // array of ecms

  annual_sav = Ce - Cn;

  nir->ecm[nir->nms].ensav = annual_sav * fuel_heat_content(ELECTRIC); // MMBtu
  nir->ecm[nir->nms].dlsav = nir->ecm[nir->nms].ensav * fuel_cost(ELECTRIC) / fuel_heat_content(ELECTRIC);

  // new assigment of savings to baseload array (MJF 1/01)
  nir->blengysav[nir->nms] = nir->ecm[nir->nms].ensav;
  nir->bldlsav[nir->nms] = nir->ecm[nir->nms].dlsav;

  // some differences here too, since we get the cost and life
  // of the new refrigerator NOT from the materials table, but rather
  // from the input fields for the new refrigerator (which comes from
  // the equipment library concept in the parameter set, new 1/01)

  nir->ecm[nir->nms].cms_measure_num = N_CMS_REFRIGERATOR_REPLACEMENT;
  nir->ecm[nir->nms].audit_section_id = N_REFRIGERATOR;
  nir->ecm[nir->nms].dwelling_component_index = NOT_APPLICABLE;

  nir->ecm[nir->nms].cost = ndi->ref.replace_install_cost + ndi->ref.replace_added_cost;
  nir->ecm[nir->nms].lifetime = ndi->ref.replace_life;

  nir->ecm[nir->nms].costum = 0;
  nir->ecm[nir->nms].costul = 0;
  nir->ecm[nir->nms].costi1 = 0;
  nir->ecm[nir->nms].qtym = 0;
  nir->ecm[nir->nms].qtyl = 0;
  nir->ecm[nir->nms].qtyi = 0;
  //nir->ecm[nir->nms].material_id = N_MAT_FROM_AUDIT; // signal no material in mat library
  //nir->ecm[nir->nms].measure_id = -1;

  nir->ecm[nir->nms].costi2 = ndi->ref.replace_install_cost;
  nir->ecm[nir->nms].typei2 = MT_REFRIGERATORS;
  STRCPY(nir->ecm[nir->nms].desci2, ndi->ref.replace_manufacturer);
  STRCAT(nir->ecm[nir->nms].desci2, " - ");
  STRCAT(nir->ecm[nir->nms].desci2, ndi->ref.replace_model);

  nir->ecm[nir->nms].costi3 = ndi->ref.replace_added_cost;
  nir->ecm[nir->nms].typei3 = MT_LABOR;
  STRCPY(nir->ecm[nir->nms].desci3, "Installation Labor");

  non_zero_measure_cost(nir->ecm[nir->nms].cost, nir->ecm[nir->nms].cms_measure_num);
  measure_economics_baseline(nir->nms, nir->ecm[nir->nms].lifetime, ELECTRIC);

  // here is some debug printout for diagnostics, can be commented out

  // if (cmds.debug_level & D_NORMAL) {
  //   fprintf(stderr, "Ref, nir->nms           %d\n", nir->nms);
  //   fprintf(stderr, "Ref, MMBtu savings %f\n", nir->ecm[nir->nms].ensav);
  //   fprintf(stderr, "Ref, Dollar saved  %f\n", nir->ecm[nir->nms].dlsav);
  //   fprintf(stderr, "Ref, Initial cost  %f\n", nir->ecm[nir->nms].cost);
  //   fprintf(stderr, "Ref, BTCR          %f\n", nir->ecm[nir->nms].sir);
  // }

  // here is the place to update the name of the refigerator
  // material replacement, we just put in as much of the new
  // refrigerator model number as will fit in TYPE_LEN characters

  if (nir->ecm[nir->nms].sir >= ndi->key.minimum_acceptable_sir) {

    rmc_index = NMAT + nir->nother_materials;       // tacked onto end of rmc array
    nir->ecm[nir->nms].rmc_index = rmc_index;
    ndi->rmc[rmc_index].ecm_index = nir->nms;

    ndi->rmc[rmc_index].quant = 1;
    STRCPY(ndi->rmc[rmc_index].material, "New Refrigerator");
    STRNCPY(ndi->rmc[rmc_index].retrofit_type, ndi->ref.replace_model, TYPE_LEN);
    // ndi->rmc[NMAT + other_materials].type[TYPE_LEN] = '\0';
    STRCPY(ndi->rmc[rmc_index].units, "Each");

    nir->nother_materials++;
  }

  increment_measure_counter();
  return;
}

// Duct Sealing

static void duct_sealing(void) {
  int rmc_index;

  //  fprintf(stderr, "ductseal routine entered\n");
  //  fprintf(stderr, "ndi->inf.post_duct_seal_efficiency = %8.2f\n", ndi->inf.post_duct_seal_efficiency);
  if (ndi->inf.evaluate_duct_sealing != YES)
    return;
  if (ndi->inf.post_duct_seal_efficiency == 0)
    return;
  //  fprintf(stderr, "nir->heatengy[PRE_RETROFIT_POST_DUCT_SEAL] & [1] = %8.2f%8.2f\n",nir->heatengy[PRE_RETROFIT_POST_DUCT_SEAL], nir->heatengy[PRE_RETROFIT_POST_INFIL]);
  nir->htengysav[nir->nms] = nir->heatengy[PRE_RETROFIT_POST_DUCT_SEAL] - nir->heatengy[PRE_RETROFIT_POST_INFIL];
  nir->clengysav[nir->nms] = nir->coolengy[PRE_RETROFIT_POST_DUCT_SEAL] - nir->coolengy[PRE_RETROFIT_POST_INFIL];

  nir->ecm[nir->nms].cms_measure_num = N_CMS_DUCT_SEALING;
  nir->ecm[nir->nms].audit_section_id = N_DUCTS_AND_INFILTRATION;
  nir->ecm[nir->nms].dwelling_component_index = NOT_APPLICABLE;

  if (ndi->inf.duct_seal_cost > 0.0f)
    nir->ecm[nir->nms].cost = ndi->inf.duct_seal_cost;
  else
    nir->ecm[nir->nms].cost = 1000.;

  rmc_index = N_MAT_DUCT_SEALING;
  
  nir->measure_priority[nir->nms] = MPS_DUCT_SEAL;  // evaluate ducts FIRST no matter what, regardless of SIR

  nir->ecm[nir->nms].costum = 0;
  nir->ecm[nir->nms].costul = 0;
  nir->ecm[nir->nms].costi1 = 0;
  nir->ecm[nir->nms].qtym = 0;
  nir->ecm[nir->nms].qtyl = 0;
  nir->ecm[nir->nms].qtyi = 0;
  //nir->ecm[nir->nms].material_id = N_MAT_FROM_AUDIT; // signal
  //nir->ecm[nir->nms].measure_id = -1;

  nir->ecm[nir->nms].costi2 = ndi->inf.duct_seal_cost;
  //nir->ecm[nir->nms].typei2 = MT_DUCT_SEALING;
  nir->ecm[nir->nms].typei2 = MT_MISCELLANEOUS_SUPPLIES;
  STRCPY(nir->ecm[nir->nms].desci2, "Duct Sealing");

  non_zero_measure_cost(nir->ecm[nir->nms].cost, nir->ecm[nir->nms].cms_measure_num);
  measure_economics_hvac(nir->nms, rmc_index);
  increment_measure_counter();
  return;
}

// These two functions provide a reverse lookup from the internal ID
// numbers to the external fixed ID numbers for the list of materials and the
// list of measures

// int material_id(int j) {
//   int i;
//   for (i = 0; i < NMAT; i++)
//     if (nir->material_translate[i] == j)
//       return (i);
//   return (500 + j); // signal that we did not find the material
// }

// Adds a record to the nir->ecmm[] measure-material array.  Added
// 7/07 to support additional cost detail records for measures
// with user defined types (ie insulation types).  This array
// is populated here and records for implemented measures are copied
// to the database results table in report.c  MJF 7/24/07

// studdepth applies only to walls, all other calls studdepth = 0

static void add_material(char *comp,      // what component does the material apply to
                 float studdepth, // stud depth in inches (walls only else = 0)
                 int wmeas,       // the wall measure number (walls only else = 0)
                 int rmc_index,   // index into the ndi->rmc[] material library array
                 char *desc,      // description of the material
                 char *type,      // string description of the material type
                 enum MATERIAL_TYPE mat_type,    // integer material type
                 char *units,     // description of the material units
                 float qty,       // the quantity in those units
                 float ucost,     // the per unit cost in dollars
                 char *comment)   // other comments
{
  int i;

  ASSERT(nir->nmsm < (4 * MAXECMS), sprintf(msg, "Maximum number of allowable materials exceeded"));

  if ((ucost == 0.0f) || (qty == 0.0f))
    return; // only make record that contributes to cost

  // possibly aggregate into an existing material detail record, in that case
  // aggregate the comp field as well.
  // only applies to different stud depth walls, skip for everything else

  if (studdepth != 0) {                           // only for non zero stud depth walls
    if (rmc_index >= 0 && nir->nmsm >= 1) {       // material from lib and there are existing records
      for (i = 0; i < nir->nmsm; i++) {
        if (nir->ecmm[i].rmc_index == rmc_index &&       // same ndi->rmc[] index
            nir->ecmm[i].material_type_id == mat_type && // same material type
            nir->ecmm[i].depth == studdepth &&           // same stud depth
            nir->ecmm[i].wmeas == wmeas) {               // in the same measure

          nir->ecmm[i].qty_est += qty; // aggregate
          STRCAT(nir->ecmm[i].components, ", ");
          STRCAT(nir->ecmm[i].components, comp); // accum comp list max 255 chars
          return;                     // thats all, no new ecmm record
        }
      }
    }
  }

  // otherwise we are going to create a new record

  nir->ecmm[nir->nmsm].ecm_index = nir->nms;        // associate our ecmm[] with ecm[] array
  nir->ecmm[nir->nmsm].rmc_index = rmc_index;       // >= valid id,  -1 = missing, not used
  nir->ecmm[nir->nmsm].material_type_id = mat_type; // passed arg
  nir->ecmm[nir->nmsm].depth = studdepth;           // to distinguish walls of different stud depth
  nir->ecmm[nir->nmsm].wmeas = wmeas;               // to distinguish wall measures
  nir->ecmm[nir->nmsm].qty_est = qty;
  nir->ecmm[nir->nmsm].unit_cost_est = ucost;

  if (strlen(comp) > 0) // copy non blank strings
    STRCPY(nir->ecmm[nir->nmsm].components, comp);
  if (strlen(desc) > 0)
    STRCPY(nir->ecmm[nir->nmsm].desc, desc);
  if (strlen(type) > 0)
    STRCAT(nir->ecmm[nir->nmsm].type, type);
  if (strlen(units) > 0)
    STRCPY(nir->ecmm[nir->nmsm].units, units);
  if (strlen(comment) > 0)
    STRCPY(nir->ecmm[nir->nmsm].comment, comment);

  nir->nmsm++; // on to the next record in ecmm array

  return;
}

/*****************************************************************************
*  Routine computes the ceiling joist U-value using the effective depth of the
*  joist if the total insulation depth is less than the joist depth or
*  the insulation value of insulation in path if the total depth of insulation
*  is greater than the joist depth.   MBG  6/3/08.
******************************************************************************/

static float get_ceiling_joist_u_value(int nc, float radded) {

  ASSERT(ndi->uas[nc].attic_type != UAS_KNEEWALL, sprintf(msg, "Ceiling joists NA for kneewalls"));

  float uclj;
  float rclj;
  float de = ndi->uas[nc].ins_depth;           // depth of existing insulation
  float da = radded / attic_ins_added_rpi(nc); // depth of added insulation
  float rpiA = attic_ins_added_rpi(nc);        // Rs/inch of added insulation
  float rpiJ = FRAMING_R_PER_IN;

  rclj = BASE_CEILING_R; // R-value of ceiling materials excluding joists and insulation

  if ((de + da) < 5.5f) {
    rclj += (de + da) * rpiJ;
  } else if (de > 5.5f) {
    rclj += 5.5f * rpiJ + (de - 5.5f) * attic_ins_exist_rpi(nc) + da * rpiA;
  } else {
    rclj += 5.5f * rpiJ + (de + da - 5.5f) * rpiA;
  }

  ASSERT(rclj, sprintf(msg, "Need non zero r value"));
  uclj = 1.0f / rclj;

  return (uclj);
}

static void increment_measure_counter(void) {
  ASSERT(nir->nms < MAXECMS, sprintf(msg, "Exceeded %d maximum energy conservation measures", MAXECMS));
  nir->ecm[nir->nms].index = nir->nms;            // tag the ecm[].index before incrementing
  nir->nms++;                                     // post measure implement increment makes this our measure counter
  return;
}

static int heating_repacement_material_standard_efficiency(void) {
  int fuel_type = ndi->htg[PRIMARY].fuel_type;

  switch(ndi->htg[PRIMARY].system_type){
  case HE_STEAM_BOILER:
  case HE_HOT_WATER_BOILER:
    return N_MAT_BOILER;

  case HE_GRAVITY_FURNACE:
  case HE_FORCED_AIR_FURNACE:
    ASSERT((fuel_type == NATURAL_GAS || fuel_type == OIL || fuel_type == ELECTRIC || fuel_type == PROPANE || fuel_type == KEROSENE), sprintf(msg, "Incorrect fuel for standard replacement:%d", fuel_type));
    return N_MAT_STANDARD_EFFICIENCY_FURNACE;

  case HE_UNVENTED_SPACE_HEATER:
  case HE_VENTED_SPACE_HEATER:
  case HE_FIXED_ELECTRIC_RESISTANCE:
    if (fuel_type == NATURAL_GAS) fuel_type = PROPANE;        // space heater replacement for natural gas primary system
    switch (fuel_type) {
    case OIL:
      return  N_MAT_SPACE_HEATER_OIL_40K;
    case PROPANE:
    case ELECTRIC:
      return N_MAT_SPACE_HEATER_GAS_8K;       // This rmc[] record is use for fixed electric replacement as well as gas MJF 4/2020
    case KEROSENE:
      return N_MAT_SPACE_HEATER_KER_10K;
    default:
      ASSERT(FALSE, sprintf(msg, "Incorrect space heater fuel: %d", fuel_type));
    }
  default:
    ASSERT(FALSE, sprintf(msg, "Incorrect heating system type: %d", ndi->htg[PRIMARY].system_type));
  }
}

static int heating_repacement_material_high_efficiency(void) {
  if (ndi->htg[PRIMARY].system_type == HE_STEAM_BOILER || ndi->htg[PRIMARY].system_type == HE_HOT_WATER_BOILER)
    return N_MAT_HIGH_EFFICIENCY_BOILER;
  else 
    return N_MAT_HIGH_EFFICIENCY_FURNACE;
}

static void add_component_code(char *code){
  STRCAT(nir->ecm[nir->nms].components, code);
  STRNCAT(nir->ecm[nir->nms].components, ",", 1);   // must be there for cumulative pass for component code searches, last comma removed later
}

static void add_component_code_once(char *code){
  STRCPY(nir->ecm[nir->nms].components, code);
  STRNCAT(nir->ecm[nir->nms].components, ", ", 1);   // must be there for cumulative pass for component code searches, last comma removed later
}

float get_air_space_r_value(float fAirSpaceDepth) {
  float fRAirSpace;

  // Here is an exponential function that constrains the returned
  // R-value to those found in the ASHRAE handbook of fundamentals
  // but is a monotonically decreasing function of gap width
  // fAirSpaceDepth is assumed to be in units of inches, MJF 2/2003

  if (fAirSpaceDepth <= 0.0f)
    fRAirSpace = 0.0f;
  else
    fRAirSpace = (float)(1.01f - exp(-4.434 * fAirSpaceDepth));
  return (fRAirSpace);

}

// Our delayed measure cost libraruy > 0 cost check #251
static void non_zero_measure_cost(float cost, int cms_measure_num){
  positive_initial_cost_required(cost, nir->measure_name[cms_measure_num]);
}

static void additional_cost(float cost) {
  nir->ecm[nir->nms].costi2 += cost;
  if (cost > 0.0f) {
    //nir->ecm[nir->nms].typei2 = MT_ADDED_COST_FROM_AUDIT_FORM;
    nir->ecm[nir->nms].typei2 = MT_MISCELLANEOUS_SUPPLIES;
    STRCPY(nir->ecm[nir->nms].desci2, "Additional Cost");
  }
}