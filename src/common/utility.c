/***************************************************************************
* MODULE:       utility.c            CREATED:     4/2020 MJF
*
* AUTHOR:       Mark Fishbaugher (mark@fishbaugher.com)
*
* MDESC:        Common NEAT and MHEA routines
****************************************************************************/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "wa_engine.h"

static float water_heater_energy_factor(enum WH_EQUIP_TYPE type, enum WH_FUEL_TYPE fuel_type, float energy_factor, float uniform_energy_factor);
static float water_heater_recovery_efficiency(enum WH_EQUIP_TYPE type, enum WH_FUEL_TYPE fuel_type, float energy_factor, float recovery_efficiency);

// assign the existing and replacement water heater energy_factor and recovery_efficiency
// based on passed inputs  See web_ui#496 August 7 entry for source of equations and wa_engine#270

void water_heater_factors(DWH *dwh) {
  dwh->exist_energy_factor = water_heater_energy_factor(dwh->exist_type, dwh->exist_fuel_type_id, dwh->exist_energy_factor, dwh->exist_uniform_energy_factor);
  dwh->exist_recovery_efficiency = water_heater_recovery_efficiency(dwh->exist_type, dwh->exist_fuel_type_id, dwh->exist_energy_factor, dwh->exist_recovery_efficiency);

  dwh->replace_energy_factor = water_heater_energy_factor(dwh->replace_type, dwh->replace_fuel_type_id, dwh->replace_energy_factor, dwh->replace_uniform_energy_factor);
  dwh->replace_recovery_efficiency = water_heater_recovery_efficiency(dwh->replace_type, dwh->replace_fuel_type_id, dwh->replace_energy_factor, dwh->replace_recovery_efficiency);
}

int water_heater_replace_data_check(DWH dwh) {
  // Assumes that water_heater_factors() has been called first before this data check
  if (dwh.exist_type == WH_EQUIP_NONE ||
    dwh.exist_fuel_type_id == WH_FUEL_NONE ||
    dwh.exist_input == 0 ||
    dwh.exist_energy_factor == 0 ||
    dwh.exist_recovery_efficiency == 0 ||
    dwh.replace_type == WH_EQUIP_NONE ||
    dwh.replace_fuel_type_id == WH_FUEL_NONE ||
    dwh.replace_input == 0 || 
    dwh.replace_energy_factor == 0 ||
    dwh.replace_recovery_efficiency == 0 ||
    dwh.replace_install_cost == 0)
    return FALSE;
  else
    return TRUE;
}

static float water_heater_energy_factor(enum WH_EQUIP_TYPE type, enum WH_FUEL_TYPE fuel_type, float energy_factor, float uniform_energy_factor) {
  float computed_energy_factor = energy_factor;
  if (energy_factor == 0) {                 // we do not have an entered EF, so assign a default or compute one
    if (type != WH_EQUIP_NONE) {            // and an equipment type specified
      if (uniform_energy_factor == 0) {     // use default based on 2014 Build America Protocols NREL/TP-5500-60988 March 2014
        switch(type) {
          case WH_STORAGE:
            switch(fuel_type){
              case WH_NATURAL_GAS:
              case WH_PROPANE:
                computed_energy_factor = 0.54; // Gas water heater, 40-gal tank, pilot light, natural draft combustion, 1-in. insulation, no heat traps, standard flue baffling
                break;
              case WH_ELECTRIC:
                computed_energy_factor = 0.87;  // Electric water heater, 50-gal tank, 1.5-in. insulation, heat traps
                break;
              default:
                ASSERT(FALSE, sprintf(msg, "Unrecognized water heater fuel type: %d", fuel_type));
            }
            break;
          case WH_HEAT_PUMP:
            computed_energy_factor = 3.0;  // https://neea.org/img/documents/qualified-products-list.pdf mid-range value
            break;
          case WH_TANKLESS:
            switch(fuel_type){
              case WH_NATURAL_GAS:
              case WH_PROPANE:
                computed_energy_factor = 0.76; // Gas tankless water heater
                break;
              case WH_ELECTRIC:
                computed_energy_factor = 0.91;  // Electric tankless water heater
                break;
              default:
                ASSERT(FALSE, sprintf(msg, "Unrecognized water heater fuel type: %d", fuel_type));
            }
            break;
          default:
            ASSERT(FALSE, sprintf(msg, "Unrecognized water heater type: %d", type));
        }
      } else {    // we have an entered UEF, so compute the EF from that
        switch(type) {
          case WH_STORAGE:
            switch(fuel_type){
              case WH_NATURAL_GAS:
              case WH_PROPANE:
                computed_energy_factor = 0.9066f * uniform_energy_factor + 0.0711f;
                break;
              case WH_ELECTRIC:
                computed_energy_factor = MIN(0.96f, 2.4029f * uniform_energy_factor - 1.2844f);
                break;
              default:
                ASSERT(FALSE, sprintf(msg, "Unrecognized water heater fuel type: %d", fuel_type));
            }
            break;
          case WH_HEAT_PUMP:
            computed_energy_factor = 1.2101f * uniform_energy_factor - 0.6052f;
            break;
          case WH_TANKLESS:
            computed_energy_factor = uniform_energy_factor;
            break;
          default:
            ASSERT(FALSE, sprintf(msg, "Unrecognized water heater type: %d", type));
        }
      }
    }
  }
  return computed_energy_factor;
}

static float water_heater_recovery_efficiency(enum WH_EQUIP_TYPE type, enum WH_FUEL_TYPE fuel_type, float energy_factor, float recovery_efficiency) {
  float computed_recovery_efficiency = recovery_efficiency;
  if (type != WH_EQUIP_NONE && recovery_efficiency == 0 && energy_factor != 0) { 
    switch(type) {
      case WH_STORAGE:
        switch(fuel_type){
          case WH_NATURAL_GAS:
          case WH_PROPANE:
            if (energy_factor >= 0.75)
              computed_recovery_efficiency = 0.778114f * energy_factor + 0.276679f;
            else
              computed_recovery_efficiency = 0.252117f * energy_factor + 0.607997f;
            break;
          case WH_ELECTRIC:
            computed_recovery_efficiency = 0.98f;
            break;
          default:
            ASSERT(FALSE, sprintf(msg, "Unrecognized water heater fuel type: %d", fuel_type));
        }
        break;
      case WH_HEAT_PUMP:
        computed_recovery_efficiency = 1.1619f * (energy_factor + 0.6052f) / 1.2101f;
        break;
      case WH_TANKLESS:
        computed_recovery_efficiency = energy_factor;
        break;
      default:
        ASSERT(FALSE, sprintf(msg, "Unrecognized water heater type: %d", type));
    }
  }
  return computed_recovery_efficiency;
}

int include_measure_in_package(int flag, int cms_measure_num, enum MEASURE_PACKAGE_SORT_PRIORITY priority, int required, float sir) {

  if ((priority == MPS_SIR && sir >= ndi->key.minimum_acceptable_sir) ||
      priority == MPS_DUCT_SEAL ||
      priority == MPS_INFILTRATION_REDUCTION ||
      priority == MPS_EQUIP_REQUIRED ||
      priority == MPS_ENVELOPE_REQUIRED ||
      (priority >= MPS_NPV && required == TRUE))
    return TRUE;

  switch (flag) {
  case NEAT_FLG:
    if (cms_measure_num == N_CMS_DUCT_SEALING || cms_measure_num == N_CMS_INFILTRATION_REDUCTION)
      return TRUE;
    break;
  case MHEA_FLG:
    if (cms_measure_num == M_CMS_SEAL_DUCTS || cms_measure_num == M_CMS_GENERAL_AIR_SEALING)
      return TRUE;
    break;
  default:
    return FALSE;
  }
  return FALSE;
}

// #251 all first pass measures for both NEAT and MHEA need to have a positive initial cost
// for the SIR calculations to make any sense.  Fail if that is not the case
void positive_initial_cost_required(float cost, char *measure_name){
  ASSERT(cost > 0.0f, sprintf(msg, "The cost for %s should be positive but it is: %f", measure_name, cost));
}

// Protected Savings to Investment Ratio (SIR) calculation
// 1) If the cost is negative or zero then SIR = 999.9 as a signal something has come off the rails
// 2) If the savings is negative but cost > 0 then the SIR is negative float
// 3) If both savings and cost are > 0, then normal positive SIR
float calculate_sir(float lcc_save, float initial_cost) {
  if (initial_cost <= 0.0) 
    return (999.9f);
  else
    return (lcc_save / initial_cost);
}

// How will infiltration reduction be treated

enum INFILTRATION_REDUCTION_TREATMENT infiltration_reduction_treatment(INF inf) {
  enum INFILTRATION_REDUCTION_TREATMENT treatment = INF_DEFAULT;
  if (inf.air_leak_red_cost > 0.01f)
    treatment = INF_INCIDENTAL_COST;          // some non zero infil reduction cost
  if (inf.pre_inf_cfm > .01) {                // non blank pre retrofit blower door cfm
    if (treatment == INF_DEFAULT)
      treatment = INF_NO_COST_COMPUTE_SAVINGS_ONLY;
    else
      treatment = INF_FULL_MEASURE;
  }
  return treatment;
}

enum MEASURE_PACKAGE_GROUPS measure_package_group(enum MEASURE_PACKAGE_SORT_PRIORITY priority) {

  switch (priority) {

    case MPS_ITEMIZED_NO_SAVE_SIR:
    case MPS_ITEMIZED_W_SAVE_SIR:

      return GROUP_INCIDENTAL_REPAIRS;
      break;
      
    case MPS_DUCT_SEAL:
    case MPS_INFILTRATION_REDUCTION:
    case MPS_EQUIP_REQUIRED:
    case MPS_ENVELOPE_REQUIRED:
    case MPS_SIR:
    case MPS_NPV:

      return GROUP_ENERGY_SAVINGS_MEASURES;
      break;

    case MPS_ENVELOPE_REQUIRED_NO_SIR:
    case MPS_EQUIP_REQUIRED_NO_SIR:
    case MPS_ITEMIZED_NO_SIR:

      return GROUP_HEALTH_AND_SAFETY_MEASURES;
      break;

    default:
      ASSERT(FALSE, sprintf(msg,"Unrecognized measure package priority"));
  }

  // #252
  // if (priority > MPS_SIR)
  //   return GROUP_INCIDENTAL_REPAIRS;
  // else if (priority < MPS_NPV)
  //   return GROUP_HEALTH_AND_SAFETY_MEASURES;
  // else if (priority == MPS_SIR || priority == MPS_NPV)
  //   return GROUP_ENERGY_SAVINGS_MEASURES;
  // else
  //   ASSERT(FALSE, sprintf(msg,"Unrecognized measure package priority"));

}

char *lighting_type_name(enum LIGHTING_LAMP_TYPE lamp_type) {
  switch (lamp_type) {
  case LTG_INCANDESCENT:
    return "Incandescent";
  case LTG_HALOGEN:
    return "Halogen Lamp";
  case LTG_FLUORESCENT:
    return "Fluorescent Lamp";
  case LTG_CFL:
    return "CFL Lamp";
  case LTG_LED:
    return "LED Lamp";
  case LTG_OTHER:
    return "Other Lamp";
  }
  ASSERT(FALSE, sprintf(msg, "Unrecognized lamp type: %d", lamp_type));
}

/*******************  FUNCTION NAME:  adjust_hspf  ****************************/
/**        DATE:    4/08                                                  **/
/**          BY:    MBG                                                   **/
/** DESCRIPTION:    Modifies the label HSPF of a heat pump to account for **/
/**                   climate differences                                 **/
/**      SOURCE:    Philip Fairy, et.al. Climate Impacts on Heating       **/
/**                   Seasonal Performance Factor ...                     **/
/**                   ASHRAE Transactions, June 2004                      **/
/***************************************************************************/
void adjust_hspf(float temp, float *fhspf) {
  double ffractchange;
  double coef[2][4] = {{0.1392, -0.008460, -0.0001074, 0.02280}, {0.1041, -0.008862, -0.0001153, 0.02817}};

  if (*fhspf < 8.5f)
    ffractchange = coef[0][0] + coef[0][1] * temp + coef[0][2] * temp * temp + coef[0][3] * (*fhspf);
  else
    ffractchange = coef[1][0] + coef[1][1] * temp + coef[1][2] * temp * temp + coef[1][3] * (*fhspf);

  *fhspf = (float)(1.0f - ffractchange) * (*fhspf);
}

/*******************  FUNCTION NAME:  adjust_seer  ****************************/
/**        DATE:    4/08                                                  **/
/**          BY:    MBG                                                   **/
/** DESCRIPTION:    Modifies the label SEER of a heat pump or AC to       **/
/**                 account for climate differences.                      **/
/**      SOURCE:    Philip Fairy, et.al. Climate Impacts on Heating       **/
/**                   Seasonal Performance Factor and Seasonal Energy     **/
/**                   Efficiency Ratio ...                                **/
/**                   ASHRAE Transactions, June 2004                      **/
/***************************************************************************/
void adjust_seer(float temp, float *fseer) {
  double ffractchange;
  double coef[2][3] = {{-0.5655, 0.005414, 0.01039}, {-0.5864, 0.005668, 0.01029}};

  if (*fseer < 13.5f)
    ffractchange = coef[0][0] + coef[0][1] * temp + coef[0][2] * (*fseer);
  else
    ffractchange = coef[1][0] + coef[1][1] * temp + coef[1][2] * (*fseer);

  *fseer = (float)(1.0f - ffractchange) * (*fseer);
}

// R-value per inch for the defined water heater tank insulation types
float water_heater_insulation_rpi(enum WH_INSULATION_TYPE insul_type) {  
  switch (insul_type) {
  case WH_NONE:
    return 0.0f;
  case WH_FIBERGLASS:
    return 3.33f;
  case WH_POLYURETHANE:
    return 6.25f;
  }
  ASSERT(FALSE, sprintf(msg, "Unrecognized water heater insulation type: %d", insul_type));
}

// Comma delimited string search. 
// NOTE: assumes there are no commas embedded in the haystack or needle on entry
char *comma_delimited_strstr(char *haystack, char *needle) {
  char comma_haystack[MAX_COMPONENTS_LEN + 1];
  char comma_needle[MAX_COMPONENTS_LEN + 1];
  comma_haystack[0] = NULL_CHAR;
  comma_needle[0] = NULL_CHAR;
  sprintf(comma_haystack, ",%s,",haystack);
  sprintf(comma_needle, ",%s,",needle);
  return(strstr(comma_haystack, comma_needle));
}


// Returns 1 if the strings are the same, or if one of the comma delimited
// sub strings in needle_list is found in the comma delimited haystack_list
int components_in_common(char *haystack_list, char *needle_list) {
  char haystack_list_copy[MAX_COMPONENTS_LEN + 1];
  char needle_list_copy[MAX_COMPONENTS_LEN + 1];
  char *token;

  if (strcmp(haystack_list, needle_list) == 0)    // they are the same list or both zero length strings
    return (1);

  STRCPY(haystack_list_copy, haystack_list);
  STRCPY(needle_list_copy, needle_list);
  token = strtok(needle_list_copy, COMMA);   // first token
  // walk through tokens
  while( token != NULL ) {
    if (comma_delimited_strstr(haystack_list_copy, token))
      return (1);
    token = strtok(NULL, COMMA);  // next token
  }
  return (0);
}