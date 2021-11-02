/***************************************************************************
* MODULE:   fuels            CREATED:      4/18/2019
*
* AUTHOR:   Mark Fishbaugher (MJF)    Agile Mobile Soft
*
* MDESC:    Functions related to the present and future cost of fuels
****************************************************************************/
#include <string.h>

#include "wa_engine.h"

static FCS s_fcs;                                   // static copy of fuel cost struct
static int reference_count[FUEL_TYPES + 1];         // how many times fuel[i] is used in analysis
static float PW_cost[FUEL_TYPES + 1][MAXMLIFE + 1];     // UPW_factor times the fuels cost in $/mmbtu, used for economics and SIR
static float UPW_factor[FUEL_TYPES + 1][MAXMLIFE + 1];  // just thge UPW_factor for comparison to NIST tables

static float fuel_cost_value(int fuel, int check);
static float fuel_cost_per_mmbtu_value(int fuel, int check);

/*******************************************
 * Function name fuel_cost
 * Date:  3/16/2019
 * Author: MJF
 *
 * Description:  Refactor fuel costs to make order independent and remove magic numbers
 * ***************************************/
void initialize_fuel_cost_data(FCS fcs, FER *fer, float real_discount_rate) {
  s_fcs = fcs;
  compute_upw_factors(fer, real_discount_rate);
}

/*******************************************
 * Function name is_fuel_referenced
 * Date:  4/18/2019
 * Author: MJF
 *
 * Description:  Resets our fuel references
 * The zeroth array element should stay zero
 * since this is by fuel type enum which is base 1
 * ***************************************/
void reset_fuel_reference_counts(void) {
  for (int i = 0; i <= FUEL_TYPES; i++) {
    reference_count[i] = 0;
  }
}


/*******************************************
 * Function name fuel_cost
 * Date:  3/16/2019
 * Author: MJF
 *
 * Description:  Refactor fuel costs to make order independent and remove magic numbers
 * ***************************************/
float fuel_cost(int fuel) {
  return fuel_cost_value(fuel, TRUE);
}
float fuel_cost_no_check(int fuel) {
  return fuel_cost_value(fuel, FALSE);
}
static float fuel_cost_value(int fuel, int check) {
  float cost = 0;
  switch (fuel) {
  case NATURAL_GAS:
    cost = s_fcs.natural_gas;
    break;
  case OIL:
    cost = s_fcs.oil;
    break;
  case ELECTRIC:
    cost = s_fcs.electric;
    break;
  case PROPANE:
    cost = s_fcs.propane;
    break;
  case WOOD:
    cost = s_fcs.wood;
    break;
  case COAL:
    cost = s_fcs.coal;
    break;
  case KEROSENE:
    cost = s_fcs.kerosene;
    break;
  case OTHER:
    cost = s_fcs.other;
    break;
  default:
      ASSERT(FALSE, sprintf(msg, "No cost for fuel type: %d", fuel));
  }
  if (check) {
    reference_count[fuel]++;    //. only increment if we enforce a non-zero or actual fuel cost
    ASSERT(cost != 0, sprintf(msg, "No cost for fuel: %s", fuel_name(fuel)));
  }
  return cost;
}

/*******************************************
 * Function name fuel_heat_content
 * Date:  3/16/2019
 * Author: MJF
 *
 * Description:  Refactor fuel heat content to make order independent (units = MMBtu/fuel unit)
 * ***************************************/
float fuel_heat_content(int fuel) {
  float heat_content = 0;
  switch (fuel) {
  case NATURAL_GAS:
    heat_content = s_fcs.natural_gas_heat;
    break;
  case OIL:
    heat_content = s_fcs.oil_heat;
    break;
  case ELECTRIC:
    heat_content = s_fcs.electric_heat;
    break;
  case PROPANE:
    heat_content = s_fcs.propane_heat;
    break;
  case WOOD:
    heat_content = s_fcs.wood_heat;
    break;
  case COAL:
    heat_content = s_fcs.coal_heat;
    break;
  case KEROSENE:
    heat_content = s_fcs.kerosene_heat;
    break;
  case OTHER:
    heat_content = s_fcs.other_heat;
    break;
  default:
    ASSERT(FALSE, sprintf(msg, "No heat content for fuel type: %d", fuel));
  }
  ASSERT(heat_content != 0, sprintf(msg, "No heat content for fuel: %s", fuel_name(fuel)));
  return heat_content;
}

/*******************************************
 * Function name fuel_name
 * Date:  4/18/2019
 * Author: MJF
 *
 * Description:  The fuel name by fuel id enum
 * ***************************************/
char *fuel_name(int fuel) {
  switch (fuel) {
  case NATURAL_GAS:
    return "Natural Gas";
  case OIL:
    return "Oil";
  case ELECTRIC:
    return "Electricity";
  case PROPANE:
    return "Propane";
  case WOOD:
    return "Wood";
  case COAL:
    return "Coal";
  case KEROSENE:
    return "Kerosene";
  case OTHER:
    return "Other";
  default:
    ASSERT(FALSE, sprintf(msg, "No name for fuel type: %d", fuel));
  }
}

/*******************************************
 * Function name fuel_cost_units
 * Date:  4/18/2019
 * Author: MJF
 *
 * Description:  The name of the fuel cost units
 * ***************************************/
char *fuel_cost_units(int fuel) {
  switch (fuel) {
  case NATURAL_GAS:
    return "$/Mcf";
  case OIL:
    return "$/Gal";
  case ELECTRIC:
    return "$/kWh";
  case PROPANE:
    return "$/Gal";
  case WOOD:
    return "$/Chord";
  case COAL:
    return "$/Ton";
  case KEROSENE:
    return "$/Gal";
  case OTHER:
    return "$/MMBtu";
  default:
    ASSERT(FALSE, sprintf(msg, "No units for fuel type: %d", fuel));
  }
}

/*******************************************
 * Function name fuel_cost_per_mmbtu
 * Date:  4/16/2019
 * Author: MJF
 *
 * Description:  Our costs per million Btu
 * ***************************************/
float fuel_cost_per_mmbtu(int fuel) {
  return fuel_cost_per_mmbtu_value(fuel, TRUE);
}
float fuel_cost_per_mmbtu_no_check(int fuel) {
  return fuel_cost_per_mmbtu_value(fuel, FALSE);
}
static float fuel_cost_per_mmbtu_value(int fuel, int check) {
  if (fuel_cost_value(fuel, check) && fuel_heat_content(fuel)){
    return fuel_cost_value(fuel, check) / fuel_heat_content(fuel);
  } else {
    return 0;
  }
}

/*******************************************
 * Function name is_fuel_referenced
 * Date:  4/18/2019
 * Author: MJF
 *
 * Description:  Do the NIST UPW_factor calcs
 * storing results in static
 * ***************************************/
void compute_upw_factors(FER *fer, float real_discount_rate) {
  float fTemp[FUEL_TYPES + 1];    // Used in deriving PWF * fuel costs
  float fTempUPW[FUEL_TYPES + 1]; // Used in deriving the UPW_factor (as per the UI)
  int i, j;
  int fuel;
  int year;

  // NOTE all arrays EXCEPT the fer[x].rates[y] are base ONE fuel type enumeration
  // ALSO NOTE that the year = 0 for fer[fuel].rates[year] = 1 by definition, so the first non unity escalation rate is year = 1

  ASSERT(real_discount_rate, sprintf(msg, "Need non zero discount rate"));

  if (cmds.debug_level & D_FUEL_ESCALATION_RATES)
    fprintf(stderr, "\nreal_discount_rate: %.4g", real_discount_rate);

  // get started with the first two factors [0] and [1]
  for (i = 0; i < FUEL_TYPES; i++) {
    fuel = i + 1;
    year = 1;
    ASSERT(fer[i].rates[0], sprintf(msg, "Need non zero escalation factor for fuel: %d year: %d", i, 0));

    fTempUPW[fuel] = 1 / real_discount_rate;  // NOT times the fuel cost to match NIST document for cmds.debug_level == 4
    UPW_factor[fuel][0] = 1;                  // by definition for the zeoth/current year
    UPW_factor[fuel][year] = fTempUPW[fuel] * fer[i].rates[1];

    fTemp[fuel] = fuel_cost_per_mmbtu_no_check(fuel) / real_discount_rate; // times the fuel cost, even if zero  MJF #95
    PW_cost[fuel][0] = fTemp[fuel];
    PW_cost[fuel][year] = fTemp[fuel] * fer[i].rates[1];

    if (cmds.debug_level & D_FUEL_ESCALATION_RATES) {
      fprintf(stderr, "\nFuel: %s UPW_factor[%d][1]: %6.4g fer.rate: %6.4g",fuel_name(fuel), fuel, UPW_factor[fuel][1], fer[i].rates[year]);
    }
  }

  for (i = 0; i < FUEL_TYPES; i++) {
    for (year = 2; year <= MAXMLIFE; year++) {
      fuel = i + 1;
      j = year;
      if (year >= NUM_FUEL_RATES) {   // using year 30 for years 30 through MAXMLIFE
        j = NUM_FUEL_RATES - 1;
      }
      ASSERT(fer[i].rates[j], sprintf(msg, "Need non zero escalation factor for fuel: %d year: %d", i, j));

      fTempUPW[fuel] /= real_discount_rate;
      UPW_factor[fuel][year] = UPW_factor[fuel][year - 1] + (fTempUPW[fuel] * fer[i].rates[j]);

      fTemp[fuel] /= real_discount_rate;
      PW_cost[fuel][year] = PW_cost[fuel][year - 1] + (fTemp[fuel] * fer[i].rates[j]);

      if (cmds.debug_level & D_FUEL_ESCALATION_RATES) {
        fprintf(stderr, "\nFuel: %s UPW_factor[%d][%d]: %6.4g fer.rate: %6.4g",fuel_name(fuel), fuel, year, UPW_factor[fuel][year], fer[i].rates[j]);
      }
    }
  }
  reset_fuel_reference_counts();
}

/*******************************************
 * Function upw_fuel_factor
 * Date:  4/18/2019
 * Author: MJF
 *
 * Description: Returns the uniform present
 * worth (UPW) factor times the fuel cost for the fuel and year in units of $/mmbtu
 * Calls the routine to compute the factors
 * if called initially
 * ***************************************/
float pw_fuel_cost(int fuel, int year) {
  ASSERT(fuel >= 1 && fuel <= FUEL_TYPES, sprintf(msg, "Fuel type out of range 0-%d: %d", FUEL_TYPES, fuel ));
  ASSERT(year >= 0 && year <= MAXMLIFE, sprintf(msg, "Year out of range 0-%d: %d", MAXMLIFE, year));
  ASSERT(UPW_factor[fuel][year], sprintf(msg, "PW fuel cost not computed for fuel: %s year: %d", fuel_name(fuel), year)); 
  return PW_cost[fuel][year];
}

/*******************************************
 * Function upw_fuel_factor
 * Date:  4/18/2019
 * Author: MJF
 *
 * Description: Returns the uniform present
 * worth (UPW) factor for the fuel and year
 * Calls the routine to compute the factors
 * if called initially
 * ***************************************/
float upw_fuel_factor(int fuel, int year) {
  ASSERT(fuel >= 0 && fuel < FUEL_TYPES, sprintf(msg, "Fuel type out of range 0-%d: %d", FUEL_TYPES, fuel ));
  ASSERT(year >= 0 && year <= MAXMLIFE, sprintf(msg, "Year out of range 0-%d: %d", MAXMLIFE, year));
  ASSERT(UPW_factor[fuel][year], sprintf(msg, "UPW factors are not computed for fuel: %s year: %d", fuel_name(fuel), year)); 
  return UPW_factor[fuel][year];
}

/*******************************************
 * Function used_fuel_results
 * Date:  4/18/2019
 * Author: MJF
 *
 * Description:  Loop through our used fuels
 * and copy information to our global results (res) structure
 * ***************************************/
int used_fuel_results(USED_FUEL *used_fuel) {
  int num_used_fuel = 0;
  for (int i = 0; i <= FUEL_TYPES; i++) {
    if (reference_count[i] > 0) {
      STRCPY(used_fuel[num_used_fuel].fuel_name, fuel_name(i));
      used_fuel[num_used_fuel].fuel_cost = fuel_cost(i);
      STRCPY(used_fuel[num_used_fuel].fuel_cost_units, fuel_cost_units(i));
      used_fuel[num_used_fuel].fuel_cost_per_mmbtu = fuel_cost_per_mmbtu(i);
      num_used_fuel++;
    }
  }
  if (cmds.debug_level & D_NORMAL) {
    fprintf(stderr, "\n\n");
    for (int i = 0; i < num_used_fuel; i++) {
      fprintf(stderr, "\nFUEL NAME USED:%s", used_fuel[i].fuel_name);
    }
  }
  return num_used_fuel;
}
