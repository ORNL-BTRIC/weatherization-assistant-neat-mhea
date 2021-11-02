/***************************************************************************
 * MODULE:  enum.h            CREATED:  November 5, 1998
 *
 * AUTHOR:  Mark Fishbaugher
 *
 * MDESC:   enumerated types common to both NEAT and MHEA
 ****************************************************************************/
#ifndef _ENUM_H
#define _ENUM_H

// align with cJSON Boolean with the addition of the NA (cJSON = null) state.

enum LOGICAL { NO = 0, YES, NA };

// The overall top level sorting used in the cumulative measure ranking
// The normal ranking by SIR is all done withing MPS_SIR
enum MEASURE_PACKAGE_SORT_PRIORITY
{ MPS_ITEMIZED_NO_SAVE_SIR = 7,
  MPS_ITEMIZED_W_SAVE_SIR = 6,
  
  MPS_DUCT_SEAL = 5,
  MPS_INFILTRATION_REDUCTION = 4,

  MPS_EQUIP_REQUIRED = 3,
  MPS_ENVELOPE_REQUIRED = 2,

  MPS_SIR = 1,
  MPS_NPV = 0,

  MPS_ENVELOPE_REQUIRED_NO_SIR = -2,
  MPS_EQUIP_REQUIRED_NO_SIR = -3,

  MPS_ITEMIZED_NO_SIR = -5,
  MPS_BOTTOM_MARK = -999 };

enum INFILTRATION_REDUCTION_TREATMENT
{ INF_DEFAULT = 0,                          // only the post blower door number entered
  INF_INCIDENTAL_COST = 1,                  // reduction cost>0 but no pre-ret blower-door readings - treat as cost
  INF_NO_COST_COMPUTE_SAVINGS_ONLY = 2,     // pre/post bd readings available but no cost - compute energy savings only
  INF_FULL_MEASURE = 3 };                   // pre/post bd readings and cost available - treat as full measure

// Grouping of reported measure economics and savings #82 MJF 8/2019
enum MEASURE_PACKAGE_GROUPS
{ GROUP_UNDEFINED = 0,                         // initial state
  GROUP_INCIDENTAL_REPAIRS = 1,                // MEASURE_PACKAGE_SORT_PRIORITY > 1
  GROUP_ENERGY_SAVINGS_MEASURES = 2,           // MEASURE_PACKAGE_SORT_PRIORITY = 1 or 0
  GROUP_HEALTH_AND_SAFETY_MEASURES = 3 };      // MEASURE_PACKAGE_SORT_PRIORITY < 0

enum MATERIAL_TYPE  // Full set of material types although we may not use all of them currently (source e_material_type )
{ MT_NONE = 0,
  MT_INSULATION = 1,
  MT_MISCELLANEOUS_SUPPLIES =2,
  MT_WINDOWS = 3,
  MT_HEATING_EQUIPMENT = 4,
  MT_COOLING_EQUIPMENT = 5,
  MT_REFRIGERATORS = 6,
  MT_HOT_WATER_EQUIPMENT = 7,
  MT_LIGHTING = 8,
  MT_LABOR = 10,
  MT_CONSTRUCTION_MATERIALS_OR_HARDWARE = 11,
  MT_DOORS = 12,
  MT_HEALTH_AND_SAFETY_ITEMS = 13,
  // MT_ADDED_COST_FROM_AUDIT_FORM = 14,    Some day  #315
  // MT_INFILTRATION_REDUCTION = 15,
  // MT_DUCT_SEALING = 16,
  // MT_ITEMIZED_MEASURE_COST = 17,
  MT_OTHER = 100,
  MT_UNSPECIFIED = 101};

enum ORIENTATION // ORIENTATION of perpendicular line to walls windows
{ NORTH = 1,
  SOUTH,
  EAST,
  WEST,
  NONE };

enum FUEL // HEATING EQUIPMENT FUEL
{ FUEL_NONE = 0,
  NATURAL_GAS,
  OIL,
  ELECTRIC,
  PROPANE,
  WOOD,
  COAL,
  KEROSENE,
  OTHER,
  PRIMARY_HEAT,
  WATER_HEAT,
  ELECTRIC_HEATPUMP,
  ELECTRIC_RESISTANCE };

enum WH_EQUIP_TYPE // TYPES OF WATER HEATERS
{ WH_EQUIP_NONE = 0,
  WH_STORAGE,
  WH_HEAT_PUMP,
  WH_TANKLESS };

enum WH_FUEL_TYPE // TYPES OF FUELS FOR WATER HEATERS
{ WH_FUEL_NONE = 0,
  WH_NATURAL_GAS,
  WH_BLANK, // NOT USED, PLACE HOLDER ONLY to match order of enum FUEL
  WH_ELECTRIC,
  WH_PROPANE };

enum WH_INSULATION_TYPE // TYPES OF WATER HEATER INSULATION
{ WH_NONE = 0,
  WH_FIBERGLASS,
  WH_POLYURETHANE };

enum WH_INPUT_RATING_TYPE // TYPES OF INPUT POWER RATINGS FOR WATER HEATERS
{ WH_KBTU = 1,
  WH_KW };

enum BILLING_UNITS // THE UNITS FOR THE BILLING DATA
{ THERMS = 1,
  KWH };

// enum LTGLOCATION // LOCATION OF LIGHT IN ROOM
// { LT_CEILING = 1,
//   LT_FLOOR,
//   LT_TABLE,
//   LT_WALL };

// enum LTGTYPE // TYPE OF LAMP, STANDARD/FLOOD, Deprecated
// { STANDRD = 1,
//   FLOOD };

enum LIGHTING_LAMP_TYPE // Lamp technology type
{ LTG_INCANDESCENT = 1,
  LTG_HALOGEN = 2,
  LTG_FLUORESCENT = 3,
  LTG_CFL = 4,
  LTG_LED = 5,
  LTG_OTHER = 100
};

enum EQUIPLOCATION // EQUIPMENT LOCATION
{ HEATED = 1,
  UNHEATED,
  UNINTENTIONALLY_HEATED };

enum EQUIPAGE // EQUIPMENT AGE ENUMERATION
{ AGE_NEW = 1,
  AGE_5_10,
  AGE_10_15,
  AGE_15 };

enum SEALCONDITION // CONDITION OF REFRIGERATOR DOOR SEAL
{ SEAL_GOOD = 1,
  SEAL_SOME_DETERIORATION,
  SEAL_GAPS_VISIBLE };

enum ENERGYUNITS // UNITS FOR ENERGY IN USER DEFINED MEASURES
{ EN_THERMS = 1,
  EN_MMBTU,
  EN_KWH };

enum LEAKINESS // A WINDOW'S LEAKINESS
{ VERY_TIGHT = 1,
  TIGHT,
  MEDIUM,
  LOOSE,
  VERY_LOOSE,
  MEC_REPLACEMENT };

//  Moved the DOOR_LEAKINESS enum to the common, to be used in neat for 
//  the new door replacement measure. Previously only in mhea enumeration 
//  GKA:  May 2010 
enum DOOR_LEAKINESS // A door's leakiness
{ LEAKY_NONE = 0,
  LEAK_TIGHT,
  LEAK_MEDIUM,
  LEAK_LOOSE };

//  Moved the "ROOF_COLOR" enum to the commonlib, to be used in
//  neat for the white roof coating measure. Previously only in mhea enumeration
//  GKA:  May 2010
enum ROOF_COLOR // ROOF COLOR CHECK BOX FIELDS
{ ROOF_COLOR_NONE = 0,
  RC_LIGHT,
  RC_DARK };

//  The first three duct seal methods are supported by both NEAT
//  and MHEA, while only MHEA supports the Pressure Pan method
enum DUCTSEAL         // METHODS FOR EVALUATING DUCT SEALING
{ DUCT_SEAL_NONE = 0,
  PRE_POST_WHOLE,     // PRE/POST WHOLE BUILDING BLOWER DOOR
  BLOWER_SUBTRACT,    // BLOWER DOOR SUBTRACTION
  DUCT_BLOWER,        // DUCT BLOWER TESTS
  PRESSURE_PAN,       // PRESSURE PAN READINGS, only supported in MHEA, NOT NEAT
  NO_DUCT_SEAL        // Duplicate duct seal = none selection
};




enum HVAC_TYPE                  // HT_
{ HT_FORCED_AIR_FURNACE = 1,       // Forced Air Furnace (regular velocity)
  HT_GRAVITY_FURNACE,              // Gravity Furnace (natural convection)
  HT_BOILER_HOT_WATER,             // Hot Water Boiler (Hydronic heating sytstem (water) (distinguish from just boiler just for HPXMP))
  HT_BOILER_STEAM,                 // Steam Boiler (Hydronic heating sytstem (steam) (distinguish from just boiler just for HPXMP))
  HT_SPACE_HEATER,                 // Individual room heating system, fixed baseboard, and portable
  HT_HEAT_PUMP_CENTRAL,            // Central heat pump with or without resistance heating (in HSPF)
  HT_HEAT_PUMP_ROOM,               // Heat pump that conditions one space(e.g. PTAC)
  HT_HEAT_PUMP_MINI_SPLIT,         // Mini split (inverter compressor variable speed)
  HT_AC_CENTRAL,                   // Central air conditioner
  HT_AC_ROOM,                      // Window or portable air conditioner or PTAC cooling only
  HT_AC_MINI_SPLIT,                // Cooling only mini split(separate indoor and outdoor components)
  HT_EVAPORATIVE_COOLER };         // Evaporative cooler

enum HVAC_EQUIP_LOCATION        // HEL_
{ HEL_CONDITIONED_LIVING_SPACE = 1,
  HEL_ATTIC,
  HEL_GARAGE,
  HEL_BASEMENT,
  HEL_CRAWLSPACE_BELLY,
  HEL_AMBIENT
};

enum EQUIP_MAINTENANCE    // EM_
{ EM_ANNUAL_WELL_MAINTAINED = 1,
  EM_SELDOM_OR_NEVER,
  EM_NOT_WORKING};

// EPvalidator [HeatingSystemFuel]
// 'electricity' or       ELECTRIC
// 'natural gas' or       NATURAL_GAS
// 'fuel oil' or          OIL
// 'fuel oil 1' or        N/A
// 'fuel oil 2' or        N/A
// 'fuel oil 4' or        N/A
// 'fuel oil 5/6' or      N/A
// 'diesel' or            N/A
// 'propane' or           PROPANE
// 'kerosene' or          KEROSENE
// 'coal' or              COAL
// 'coke' or              N/A
// 'bituminous coal' or   N/A
// 'wood' or              WOOD
// 'wood pellets'         N/A

enum HEAT_PUMP_BACKUP_FUEL      // HPBF_  intentional overlap with FUEL enumeration, just a restricted subset
{ HPBF_FUEL_NONE = 0,
  HPBF_NATURAL_GAS,
  HPBF_OIL,
  HPBF_ELECTRIC,
  HPBF_PROPANE };

#if 0
// https://hpxml.nrel.gov/datadictionary/2.3.0/Building/BuildingDetails/Systems/CombustionVentilation/CombustionVentilationSystem/VentSystemType
// sealed combustion
// direct vented (outside combustion air)
// power vented (at exterior)
// power vented (at unit)
// induced draft
// atmospheric
enum COMBUSTION_TYPE            // For sytems using a combustion fuel (!ELECTRIC and !OTHER)
{ NO_COMBUSTION = 0,            // Does not have a combustion fuel (default)
  UNVENTED,                     // No venting of combustion
  ATMOSPHERIC,                  // Vented at atmospheric pressure (used in afue vs steady state)
  INDUCED_DRAFT,                // Fan assisted
  SEALED_COMBUSTION  };         // Sealed and fan assisted

// https://hpxml.nrel.gov/datadictionary/2.3.0/Building/BuildingDetails/Systems/HVAC/HVACDistribution/DistributionSystemType/AirDistribution
// AirDistribution
//    gravity
//    high velocity
//    regular velocity
// https://hpxml.nrel.gov/datadictionary/2.3.0/Building/BuildingDetails/Systems/HVAC/HVACDistribution/DistributionSystemType/HydronicDistribution
// HydronicDistribution
//    other
//    radiant ceiling
//    radiant floor
//    baseboard
//    radiator
enum HVAC_DISTRIBUTION          // This is HVAC distribution including terminal unit type
{ DIST_NONE = 0,                // No distribution
  AIR_DUCTS,                    // Forced air duct work
  BASEBOARD,                    // Baseboards (both hydronic and electric)
  RADIANT_NATURAL_CONVECTION,   // Radiant and natural convection (hydronic)
  FAN_IN_UNIT,                  // Fan and source in unit
  RADIANT_FLOOR};               // Floor radiator
#endif

// Not in HPXML, used for evaluation of 2 measures plus the calculation of SS to AFUE conversion for
// ATMOSPHERIC combustion type.
// enum IGNITION_SOURCE            // Applicable if NOT NO_COMBUSTION
// { PILOT_LIGHT,                  // standing pilot light
//   IID,                          // intermittent ignition device (spark, hotplate, etc)
//   MANUAL};                      // typically fireplace, wood stove etc)

enum EFFICIENCY_METHOD          // EM_
{ EM_ESTIMATED_FROM_YEAR = 1,   // The efficiency is either printed on the nameplate or computed from nameplate output/input capacity
  EM_NAME_PLATE,                // Only apply to existing rather than replacement systems, (only furnace or boiler and fuel = NG, Oil, Propane)
  EM_MEASURED_AT_SITE};         // Use estimated efficiency by the year manufactured (not year_installed) (only applies to ac and heat pump, otherwise NOT shown)

// https://hpxml.nrel.gov/datadictionary/2.3.0/Building/BuildingDetails/Systems/HVAC/HVACPlant/HeatingSystem/AnnualHeatingEfficiency/Units
// Percent
// AFUE
// COP
// HSPF
enum HEAT_EFFICIENCY_UNITS          // HEU_
{ HEU_AFUE = 1,                     // Percent Combustion efficiency (only choice if MEASURED_AT_SITE) (Steady state for MEASURED_AT_SITE and Thermal Efficiency for NAME_PLATE)
  HEU_HEAT_COP,                     // Coefficient of Performance. Ratio of useful heating provided to work required, steady state
  HEU_PERCENT,                      // Annual Fuel Utilization Efficiency (target for converting other units)
  HEU_HSPF};                        // Heating Seasonal Performance Factor (only option for Heat Pump)

// https://hpxml.nrel.gov/datadictionary/2.3.0/Building/BuildingDetails/Systems/HVAC/HVACPlant/CoolingSystem/AnnualCoolingEfficiency/Units
// kW/ton
// COP
// EER
// SEER
enum COOL_EFFICIENCY_UNITS      // CEU_
{ CEU_EER = 1,                      // Energy Efficiency Ratio. The EER is the ratio of the cooling capacity (in Btu/h) to the power input (in watts)
  CEU_CEER,                         // Combined Energy Efficiency Ratio is the new standard as of June 2014. It takes in account the energy used while the air conditioner is running, as well as the standby power used when the unit is not running but is powered on
  CEU_COOL_COP,                     // Coefficient of Performance. Ratio of useful cooling provided to work required, steady state
  CEU_SEER};                        // Seasonal Energy Efficiency Ratio is most commonly used to measure the efficiency of a central air conditioner (target for converting other units, only option for HeatPumps)

enum HEAT_CAPACITY_UNITS        // HCU_
{ HCU_KBTU_PER_HOUR = 1,
  HCU_BTU_PER_HOUR,
  HCU_KW,
  HCU_GALLONS_PER_HOUR,
  HCU_TONS};

enum COOL_CAPACITY_UNITS        // CCU_
{ CCU_KBTU_PER_HOUR = 1,
  CCU_TON,
  CCU_BTU_PER_HOUR};

enum DUCT_TYPE                  // DT_
{ DT_SUPPLY_DUCT = 1,
  DT_RETURN_DUCT };

// Not in EPvalidator, must be metalic ducts assumed.  Not sure if EPlus does duct simulation or an estimation tool like ASHRAE 152
// NO Scott H. says duct material is not part of the EPlus modelling
enum DUCT_MATERIAL_TYPE         // DMT_
{ DMT_METALLIC = 1,
  DMT_NON_METALLIC,
  DMT_BUILDING_CAVITY};     // not suppored by HPXML -- ask Scott if it should be

//EPvalidator
// 'living space' or                     CONDITIONED_SPACE
// 'basement - conditioned' or           N/A
// 'basement - unconditioned' or         N/A
// 'crawlspace - vented' or              UNCONDITIONED_SUBSPACE
// 'crawlspace - unvented' or            UNINTENTIONALLY_CONDITIONED_SPACE
// 'attic - vented' or                   UNCONDITIONED_ATTIC
// 'attic - unvented' or                 N/A
// 'garage' or                           N/A
// 'exterior wall' or                    N/A
// 'under slab' or                       N/A
// 'roof deck' or                        N/A
// 'outside' or                          N/A
// 'other housing unit' or               N/A
// 'other heated space' or               N/A
// 'other multifamily buffer space' or   N/A
// 'other non-freezing space'            N/A
enum HVAC_DUCT_LOCATION   // HDL_
{ HDL_DUCT_NONE = 0,
  HDL_CONDITIONED_SPACE,
  HDL_UNINTENTIONALLY_CONDITIONED_SPACE,
  HDL_UNCONDITIONED_ATTIC,
  HDL_UNCONDITIONED_SUBSPACE};


enum DUCTLOCATION // HEATING SUPPLY UNINSULATED DUCT LOCATION, IGNORE DUCTS IN CONDITIONED SPACE
{ ATTIC = 1,
  SUBSPACE,
  NO_DUCTS };

enum DUCTSHAPE // HEATING SUPPLY CROSS SECTION
{ DS_ROUND = 1,
  DS_RECTANGULAR,
  DS_NONE };


#endif // _ENUM_H
