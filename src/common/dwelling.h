/***************************************************************************
 * MODULE:  struct.h            CREATED:    November 5, 1998
 *
 * AUTHOR:  Mark Fishbaugher
 *
 * MDESC:   common definitions for both NEAT and MHEA structures.  mostly
 *           common string lengths, counts and the like
 ****************************************************************************/
#ifndef _STRUCT_H
#define _STRUCT_H

// ***************************************************
// Weather station selection for dwelling
// MJF 8/1/00
// ***************************************************
typedef struct {
  char state[STATE_LEN + 1];     // State (abbreviation) field
  char city[CITY_LEN + 1];       // City field
  char file[SHORT_NAME_LEN + 1]; // actual name of the file
} WTH;




// ***************************************************
// GitLab Issue #303
// Multiple HVAC data structure.  Same record structure
// for both heating and cooling or combined (heat pump)
// ***************************************************

typedef struct {

  // FIELDS WHICH CHARACTERIZE THE EXISTING SYSTEM

  char code[CODE_LEN + 1];                        // System code for the HVAC system, every system gets a short name, EXCLUDE COMMA
  int id;                                         //PK

  enum HVAC_TYPE system_type;                       // what is the top level system type
  enum FUEL fuel;                                   // what is the type of fuel used
  enum EQUIP_MAINTENANCE equip_maintenance;         // always ask (not used by EPvalidator but is used for equip degredation)
  int year_installed;                               // always ask installed yyyy range check >= year_manufactured used in equip degradation calculation
  enum HVAC_EQUIP_LOCATION location;                // Location of Heating Equipment not asked for Cooling only system (not used in EPvalidator but is used in measure evaluation)
  enum HEAT_PUMP_BACKUP_FUEL heat_pump_backup_fuel; // what is the type of fuel used for HEAT_PUMP_CENTRAL backup(Electric, Natural Gas, Propane, Oil)

  // if combustion type fuel
  // NATURAL_GAS, OIL, PROPANE, KEROSENE, (COAL and WOOD are special cases or default = MANUAL ignition source)

    //enum IGNITION_SOURCE ignition_source;           // if combustion type fuel
    enum LOGICAL pilot_light;                       // is there a pilot light
    enum LOGICAL iid;                               // is there an intermittent ignition device

    enum LOGICAL atmospheric_combustion;            // only if heat_efficiency_units = STEADY_STATE_COMBUSTION
    enum LOGICAL pilot_light_summer;                // if ignition_source == PILOT_LIGHT, Is light on during the summer months 
    enum LOGICAL vent_damper;                       // is a vent damper present (if combustion fuel and not sealed/condensing)
  // endif

    enum EFFICIENCY_METHOD efficiency_method;       // how is the equipment efficiency to be input (options depend on system_type)
    int year_manufactured;                          // always will want to know this (potential cross check with efficiency) check <= year_installed

  // heating load is an attribute of the envelope and weather and 100% of that load is assumed]
  // to be met by some system (otherwise the interior temperature would not be maintained).  A possible
  // senario is partitioning the floor area and blocking supply registers during the heating season as
  // a cost saving measure. Not sure how we deal with partial loads met senarios.

  // if system_type =
  // GRAVITY_FURNACE
  // FORCED_AIR_FURNACE
  // BOILER_STEAM
  // BOILER_HOT_WATER
  // SPACE_HEATER
  // HEAT_PUMP_CENTRAL
  // HEAT_PUMP_ROOM
  // HEAT_PUMP_MINI_SPLIT
    //float percent_heat_load_served;                     // if system_type provides heating (logic for comparison of sum to 100%  ??)
    //enum LOGICAL primary_heating;                     // is this the primary heating system (OR computed based on highest percent)?
    enum HEAT_EFFICIENCY_UNITS heat_efficiency_units;   // if percent_heat_supplied, default HSPF if HeatPump (other defaults by system_type)
    float heat_efficiency;                              // if percent_heat_supplied
    enum HEAT_CAPACITY_UNITS heat_output_capacity_units;       // if percent_heat_supplied
    float heat_output_capacity;                         // if percent_heat_supplied, required
  // endif

  // cooling load is an attribute of the envelope and weather and 100% of that load is assumed]
  // to be met by some system (otherwise the interior temperature would not be maintained, but in the case
  // of cooling, we may NOT meet all the load like in the case of a broken or missing central system and poorly
  // maintained window units.  In that case the interior temperature does exceed the design IAT for the warmest
  // periods and people just live with it.

  // if system_type =
  // HEAT_PUMP_CENTRAL
  // HEAT_PUMP_ROOM
  // HEAT_PUMP_MINI_SPLIT
  // AC_CENTRAL
  // AC_ROOM
  // AC_MINI_SPLIT
  // EVAPORATIVE_COOLER
    //float percent_cool_load_served;                 // if system_type provides cooling (logic for comparison of sum to 100% ??)
    //enum LOGICAL primary_cooling;                   // is this the primary cooling system (OR computed based on highest percent)?
    enum COOL_EFFICIENCY_UNITS cool_efficiency_units;   // if percent_cool_supplied and not EVAPORATIVE, default SEER if HeatPump
    float cool_efficiency;                              // if percent_cool_supplied and not EVAPORATIVE
    enum COOL_CAPACITY_UNITS cool_output_capacity_units;       // if percent_cool_supplied
    float cool_output_capacity;                         // if percent_cool_supplied, required
  // endif

  enum LOGICAL heat_setback_used;                     // if applicable system_type and percent_heat_supplied (use default setback assumption for iat, hard coded)
  //enum LOGICAL cool_setup_used;                       // if applicable system_type percent_cool_supplied (ditto)

  // FIELDS WHICH DEFINE ECMs TO BE EVALUATED

  // =======SMART THERMOSTAT=======
  // Only applies to system_type =  
  // GRAVITY_FURNACE
  // FORCED_AIR_FURNACE
  // BOILER_STEAM         (a specialty item due to long response times, maybe eliminate)
  // BOILER_HOT_WATER     (zone control valve with programmable features, not sure if current calculation applies because of different thermal zones)
  // SPACE_HEATER         (for electric baseboard "line-voltage programmable thermostats" requried, specialty item)
  // HEAT_PUMP_CENTRAL    (specialty item, see https://www.energy.gov/energysaver/thermostats)
  // AC_CENTRAL           (if we decide to support calculation of cooling setup)
  // For other systems, skip all questions about smart thermostat
  // There appears to be no take-back effect in current calculations and no take-back factor estimates that MJF could find 11/2020

  // not asked for cooling only systems
  enum LOGICAL smart_thermostat_evaluate;             // if heat_setback_used == FALSE and/or cool_setup == FALSE and applicable system_type
  //  if smart_thermostat_evaluate == TRUE
      enum LOGICAL smart_thermostat_required;
      enum LOGICAL smart_thermostat_inc_sir;              // if smart_thermostat_required 

      float thermostat_heating_nightime_setpoint;                   // if percent_heat_supplied and smart_thermostat_evaluate default 65
      float thermostat_heating_setback_hours_per_day;               // if percent_heat_supplied and smart_thermostat_evaluate default 8

      float thermostat_additional_cost;                                // in addition to library costs
  // endif

  // ========TUNE UP========
  enum LOGICAL tuneup_evaluate;                       // if system_type/fuel_type == tunable consider a tuneup
  //  if tuneup_evaluate == TRUE
      enum LOGICAL tuneup_required;
      enum LOGICAL tuneup_inc_sir;                        // if tuneup_required

      float tuneup_heating_percent_improvement;           // if percent_heat_supplied (tight range or manipulated by code or dynamic default)
      float tuneup_cooling_percent_improvement;           // if  percent_cool_supplied (tight range or manipulated by code or dynamic default)

      float tuneup_additional_cost;                       // in addition to library costs
  // endif

  //. =======REPLACEMENT=======
  enum LOGICAL replace_evaluate;                          // Consider system replacement
  //  if replace_evaluate == TRUE
      enum LOGICAL replace_required;
      enum LOGICAL replace_inc_sir;                           // if replace_required == TRUE

      enum HVAC_TYPE replacement_system_type;                 // what is the top level system type
      enum FUEL replacment_fuel;                              // what is the type of fuel used

      char replace_manufacturer[LRG_STR + 1];           // name of manufacturer of new heating system (optional) ??
      char replace_model[LRG_STR + 1];                  // model of new heating system (optional) ??

      // The following can be defaulted to the sum of percent_heat_load_served for this and replaced system
      //float replacement_percent_heat_supplied;
      // The following can be defaulted to the sum of percent_cool_load_served for this and replaced system
      //float replacement_percent_cool_supplied;                    // if system_type provides cooling (OR computed from existing and replaced_ codes)

      // All nameplate efficiency values for replacement equipment (heating or cooling, or both for Heat Pumps only)

      enum HEAT_EFFICIENCY_UNITS replacement_heat_efficiency_units;   // if percent_heat_supplied
      float replacement_heat_efficiency;                              // if percent_heat_supplied

      enum COOL_EFFICIENCY_UNITS replacement_cool_efficiency_units;   // if percent_cool_supplied and not EVAPORATIVE
      float replacement_cool_efficiency;                              // if percent_cool_supplied and not EVAPORATIVE

      enum HEAT_CAPACITY_UNITS replacement_heat_capacity_units;       // if percent_heat_supplied (maybe E+ for now)
      float replacement_heat_output_capacity;                         // if percent_heat_supplied

      enum COOL_CAPACITY_UNITS replacement_cool_capacity_units;       // if percent_cool_supplied (mabe only E+m for now)
      float replacement_cool_output_capacity;                         // if percent_cool_supplied

      enum HEAT_PUMP_BACKUP_FUEL replacement_heat_pump_backup_fuel; // what is the type of fuel used for HEAT_PUMP_CENTRAL backup(Electric, Natural Gas, Propane, Oil)

      // Any systems appearing in the following codes lists should be checked that
      // smart_thermostat_evaluate = tuneup_evaluate = replace_evaluate = FALSE or better yet, limit the list of
      // systems that can be replace to ONLY those with all three action flags turned off

      // what list of ids is shown to user is a UI issue and logic may apply like
      // only showing heating/cooling ids
      char also_replaces_hvac_ids[MAX_HVAC];                     // if replacement_heat_efficiency, comma separated list of ids of heating system that this system will be replace

      float replacement_material_cost;
      float replacement_labor_cost;
      float replacement_other_cost;
  // endif

} HVAC;

// Only applies if one or more HVAC records have distribution type   SUPPLY_AND_RETURN_AIR or  SUPPLY_AIR
// otherwise there should be zero records for DUCT.

typedef struct {

  char code[CODE_LEN + 1];                      // The short code name of this duct
  int id;                                       // PK for this DUCT info

  enum DUCT_TYPE duct_type;                     // supply or return EPvalidator has separate [HVACDuct] for supply and return
  //enum DUCT_MATERIAL_TYPE duct_material_type; // what material is the duct constructed from (not in EPvalidator, only 152-2014), assume sheet metal

  int hvac_ids_served[MAX_HVAC];                // The hvac.ids of the systems served

  enum HVAC_DUCT_LOCATION duct_location;        // or None
  enum LOGICAL fill_in_defaults;                // use default thermal efficiency based on location and other assumptions see #306
  int measure_number;                           // Duct Measure number for combining multiple record

  // default surface area from ASHRAE 152-214
  // for supply   duct_surface_area = 0.27 * floor_area
  // for return   duct_surface_area = 0.05 * num_return_registers * floor_area
  // default duct_r_value = 0 for duct_location = CONDITIONED_SPACE  R4 ! CONDITIONED_SPACE (current standard is R6 and R8)

  float duct_r_value;                           // if !use_defaults, existing duct R-value or 0 if uninsulated or derated insulation
  int number_of_registers;                      // if !use_defaults AND duct_type = return, in EPvalidator but not in NEAT or MHEA
  float duct_surface_area;                      // if !use_defaults, surface area of the duct (UI may provide area calculator or popup help with common circular ducts)



  // Insulation measure handled here while duct sealing is handled 

  //enum LOGICAL insulate_duct_evaluate;         // consider adding to duct insulation
  //enum LOGICAL insulate_required;              // if insulate_duct_evaluate, require regardless of SIR
  //enum LOGICAL insulate_inc_sir;               // if insulate_required include in measure SIR or health and safety if FALSE

  // Duct Insulation measure details

  float add_duct_r_value;                      // if insulate_duct_evaluate, R value of added insulation
  float insulation_add_cost;                   // if insulate_duct_evaluate, Use cost per sqft from library PLUS this cost adder (e.g. setup costs)

} DUCT;     // Physical Air Duct Work, Duct air leakage handled in LEAK struct






// ***************************************************
// Infiltration and duct sealing info.  New structure
// MJF 8/1/00
// ***************************************************

typedef struct {
  enum LOGICAL evaluate_duct_sealing; // do you want to enter data for duct sealing
  enum DUCTSEAL duct_seal_method;     // what method to use to evaluate duct sealing

  // Duct Seal Method 
  // DUCT_SEAL_NONE   = 0 
  // PRE_POST_WHOLE   = 1
  // BLOWER_SUBTRACT  = 2
  // DUCT_BLOWER      = 3
  // PRESSURE_PAN     = 4 (MHEA Only)

  // Duct Seal Method                     0 | 1 | 2 | 3 | 4 |   Description
  float pre_inf_cfm;  //                  x   x   x   x   x     Air Leakage Rate (cfm) Before Weatherization (Existing)
  float pre_inf_pa;   //                  x   x   x   x   x     at House Pressure Difference (Pa) Before Weatherization (Existing)
  float post_duct_seal_cfm; //                x   x             Air Leakage Rate (cfm) After Duct Sealing and Before Other Weatherization (Target or Actual)
  float post_duct_seal_pa;  //                x   x             at House Pressure Difference (Pa) After Duct Sealing and Before Other Weatherization (Target or Actual)
  float post_inf_cfm; //                  x   x   x   x   x     Air Leakage Rate (cfm) After Weatherization (Target or Actual)
  float post_inf_pa;  //                  x   x   x   x   x     at House Pressure Difference (Pa) After Weatherization (Target or Actual)
  float pre_duct_seal_supply_pa;  //          x   x   x   x     Supply (Pa) Before Duct Sealing
  float post_duct_seal_supply_pa; //          x   x   x   x     Supply (Pa) After Duct Sealing
  float pre_duct_seal_return_pa;  //          x   x   x         Return (Pa) Before Duct Sealing (NEAT only)
  float post_duct_seal_return_pa; //          x   x   x         Return (Pa) After Duct Sealing (NEAT only)
  float pre_duct_seal_close_cfm;  //              x             Air Leakage Rate (cfm) With Registers/Grills Sealed Before Weatherization (Existing)
  float pre_duct_seal_close_pa;   //              x             at House Pressure Difference (Pa) With Registers/Grills Sealed Before Weatherization (Existing)
  float pre_duct_seal_close_diff_pa; //           x             Duct to House Pressure Difference (Pa) With Registers/Grills Sealed Before Weatherization (Existing)
  float post_duct_seal_close_cfm;    //           x             Air Leakage Rate (cfm) With Registers/Grills Sealed After Duct Sealing and Before Other Weatherization (Target or Actual)
  float post_duct_seal_close_pa;     //           x             at House Pressure Difference (Pa) With Registers/Grills Sealed After Duct Sealing and Before Other Weatherization (Target or Actual)
  float post_duct_seal_close_diff_pa; //          x             Duct to House Pressure Difference (Pa) With Registers/Grills Sealed After Duct Sealing and Before Other Weatherization (Target or Actual)
//float pre_duct_seal_close_pa;                       x         (reused) House Pressure WRT Outside (Pa) / Before Duct Sealing (Existing) / Outside *
//float post_duct_seal_close_pa;                      x         (reused) House Pressure WRT Outside (Pa) / After Duct Sealing (Target or Actual) / Outside *
  float pre_duct_seal_tot_cfm;      //                x         Fan Flow (cfm) Before Duct Sealing (Existing) Total
  float pre_duct_seal_tot_duct_pa;  //                x         at Duct Pressure (Pa) Before Duct Sealing (Existing) Total
  float pre_duct_seal_out_cfm;      //                x         Fan Flow (cfm) Before Duct Sealing (Existing) Outside *
  float pre_duct_seal_out_duct_pa;  //                x   x     at Duct Pressure (Pa) Before Duct Sealing (Existing) Outside * / (MHEA) Sum of Pressure Pan Measurements (Pa)
  float post_duct_seal_tot_cfm;     //                x         Fan Flow (cfm) After Duct Sealing (Target or Actual) Total
  float post_duct_seal_tot_duct_pa; //                x         at Duct Pressure (Pa) After Duct Sealing (Target or Actual) Total
  float post_duct_seal_out_cfm;     //                x         Fan Flow (cfm) After Duct Sealing (Target or Actual) Outside * 
  float post_duct_seal_out_duct_pa; //                x   x     at Duct Pressure (Pa) After Duct Sealing (Target or Actual) Outside * / (MHEA) Sum of Pressure Pan Measurements (Pa)
  float air_leak_red_cost; //             x   x   x   x   x     Infiltration Reduction ($)
  float duct_seal_cost;    //                 x   x   x   x     Duct Sealing ($)

  // computed fields MJF 5/2019
  float pre_duct_50_cfm;
  float post_duct_50_cfm;
  float pre_duct_seal_efficiency;
  float post_duct_seal_efficiency;
} INF;

// ***************************************************
// Base load measure information, added
// MJF 8/1/00
// ***************************************************

typedef struct {
  enum WH_EQUIP_TYPE exist_type;                    // existing equipment type
  enum WH_FUEL_TYPE exist_fuel_type_id;             // water heating fields, fuel type
  enum EQUIPLOCATION exist_tank_location_id;        // location of the water heater
  float exist_gal;                                  // capacity of existing tank in gallons

  float exist_uniform_energy_factor;                // uniform energy factor of existing tank #270 (UEF)
  float exist_energy_factor;                        // energy factor of existing tank (EF)
  float exist_recovery_efficiency;                  // recovery efficiency of existing tank (RE)

  enum WH_INPUT_RATING_TYPE exist_input_units_id;   // either kW or kBTU/hr
  float exist_input;                                // power input rating

  enum LOGICAL exist_tank_wrap;                     // is there a tank wrap on the existing water heater
  float exist_rvalue;                               // direct entry of rvalue from label (alternate to previous two fields)
  float exist_insul_thick;                          // thickness of existing insulation in inches (exist+any wrap)
  enum WH_INSULATION_TYPE exist_insul_type_id;      // type of insulation in the water heater
  enum LOGICAL exist_pipe_insul;                    // is the first 5' of hot water supply pipe insulated?

  int shower_heads;                                 // the number of shower heads in the home
  float shower_usage_per_day;                       // minutes of shower usage per day (all heads)
  float shower_gpm;                                 // average gpm of all existing shower heads

  enum LOGICAL replace;                             // Water heater required replacment (8.6)
  enum LOGICAL inc_sir;                             // Include replacement in Cumulative SIR ? (added 8.6)

  enum WH_EQUIP_TYPE replace_type;                  // replacemenbt equipment type
  char replace_manufacturer[LRG_STR + 1];           // name of manufacturer of new water heater
  char replace_model[LRG_STR + 1];                  // model of new water heater
  enum WH_FUEL_TYPE replace_fuel_type_id;           // water heating fields, fuel type
  float replace_gal;                                // capacity of replacement tank in gallons

  float replace_uniform_energy_factor;              // uniform energy factor of replacement tank (UEF)
  float replace_energy_factor;                      // energy factor of replacement tank (EF) (computed only)
  float replace_recovery_efficiency;                // recovery efficiency of replacement tank (RE)

  enum WH_INPUT_RATING_TYPE replace_input_units_id; // either kW or kBTU/hr
  float replace_input;                              // power input rating
  int replace_life;                                 // lifetime of the replacement tank in years
  float replace_install_cost;                       // material cost of the new tank
  float replace_added_cost;                         // labor+recycle+other costs
} DWH;

// ******************************************
// The REFRIGERATOR struct
// NOTE: this is the first structure where the
// variable names match the associated field
// names in the database. Note that mixed case
// names are used in the database as a convention
// and lowercase with undersores are used
// in the analysis engine, thus the field
// LabelkWhPerYear in the database becomes
// label_kwh_per_year in this structure, MJF 1/01
// NOTE: not all refirgerator fields are passed
// to the engine because not all fields are
// required for analysis.
// ******************************************

typedef struct {
  enum EQUIPLOCATION location_id;            // location of the refrigerator
  float label_kwh_per_year;                  // kWh/yr of existing refrigerator, AHAM table or label
  enum EQUIPAGE label_year_id;               // 1=<5, 2=>=5<10, 3=>=10<15, 4=>15yrs
  enum SEALCONDITION door_seal_condition_id; // 1=good,2=some deterioration,3=gaps visible
  float meter_energy_reading;                // kWh of refrigerator consumption during metering interval
  float meter_energy_interval;               // refrigerator metering interval in minutes
  float meter_temperature;                   // the temperature(F) of refrigerator in unconditioned space during metering
  enum LOGICAL meter_manual_defrost;         // is the refrigerator a manual defrost type
  enum LOGICAL meter_includes_defrost;       // did metering inteval include a defrost cycle
  char replace_manufacturer[LRG_STR + 1];    // name of manufacturer of new refigerator
  char replace_model[LRG_STR + 1];           // model of new refrigerator
  float replace_kwh_per_year;                // new refrigerator annual energy consumption in kWh
  int replace_life;                          // lifetime of the replacement refrigerator in years
  float replace_install_cost;                       // material cost of the new refrigerator
  float replace_added_cost;                       // labor+recycle+other costs
} REF;

// *****************************************************
// The LIGHTING struct is a representation of the info
// collected by the Lighting input form.
// *****************************************************

typedef struct {
  char code[CODE_LEN + 1]; // Lighting Code

  enum LIGHTING_LAMP_TYPE exist_lamp_type;
  int exist_lamp_count;
  float exist_lamp_watts;
  float exist_hours_per_day;

  enum LIGHTING_LAMP_TYPE new_lamp_type;
  int new_lamp_count;
  float new_lamp_watts;
  float new_hours_per_day;
  float new_lifetime_hrs;

  float install_cost_per_lamp;
  float added_cost_per_lamp;
  float added_cost;
} LTG;

// ****************************************************
// The USER-DEFINED ITEMS struct is a representation of the info collected
//  by the Itemized Cost and User-Defined Measure input form
// ****************************************************

typedef struct {
  char measure[MEASURENAME_LEN + 1];      // Description of User Defined Item/Measure
  int component_id;                       // optional field passed from db into the itemized cost or 0 #318
  float cost;                   // Cost of User Defined Item
  enum LOGICAL inc_sir;         // Include item in Cumulative SIR ?
  char material[UDMAT_LEN + 1]; // Material for User Defined Item
  enum ENERGYUNITS units;       // Units of energy saved (added MJF 1/6/4)
  float savings;                // Savings for User Defined Item (converted to MMBtu on input)
  enum FUEL fuel;               // Fuel saved by User Defined Item
  float life;                   // Life of measure

  int measure_index;            // index PK in the output report 
  int ecm_index;                // if the itemized cost record is associated with an ecm[] then store the index otherwise NOT_APPLICABLE
} ITC;

// ****************************************************
// Utility bill information, both heating ubh and cooling ubc
// ****************************************************

typedef struct {
  enum BILLING_UNITS usage_units; // what units are the data in
  int period_days;                // how many days in the first billing period
  float base_temp;                // degree day base temp
  float base_load;                // base load
} UBI;

// ****************************************************
// The BILLING record struct is use for ALL billing
// information for all fuels both heating and cooling
// ****************************************************

typedef struct {
  int year;          // year for bill (yyyy)
  int month;         // month of bill (1-12)
  int day;           // day of the month for bill (1-31)
  float usage;       // consumption is specified units
  float degree_days; // heating or cooling degree days
} UBR;

// ****************************************************
// The FUEL PRICES struct
// ****************************************************

typedef struct {
  float natural_gas; // Natural Gas (1000 cf)
  float oil;         // #2 Fuel Oil (gallons)
  float electric;    // Electricity (kWh)
  float propane;     // Propane (gallons)
  float wood;        // Wood (cord)
  float coal;        // Coal (ton)
  float kerosene;    // Kerosene - #1 Fuel Oil (gallons)
  float other;       // Other (MMBtu of something else)

  float natural_gas_heat; // Heat content values in MMBtu/unit for the same set of fuels
  float oil_heat;         //
  float electric_heat;    //
  float propane_heat;     //
  float wood_heat;        //
  float coal_heat;        //
  float kerosene_heat;    //
  float other_heat;       //
} FCS;

// ****************************************************
// The FUEL ESCALATION RATES struct (reference EAI year and region)
// ****************************************************

typedef struct {
  int year;              // NIST/EIA publication year (base for ndi->fer.rages[0])
  char state[STATE_LEN]; // need either the state or region
  int region;            // census region (1-4) or 5 for US average table, looked up from state if not given directly
} RER;

// ****************************************************
// The FUEL ESCALATION RATES struct (explicit listed)
// ****************************************************

typedef struct {
  char fuelname[FUEL_LEN + 1]; // Current Fuel Type name, changed from confusing name fuel_type to fuel_name MJF 10/18
  float rates[NUM_FUEL_RATES];  // array of escalation factors years 0-30  (MJF 1/2019)
} FER;

// ****************************************************
// The CANDIDATE MEASURES struct is a representation of the info
// collected by the Setup - Candidate Measures input form.
// ****************************************************

// typedef struct {
//   enum LOGICAL measures[NEAT_MAX_CMS]; // Retrofit Measure (group 2)
// } N_CMS;

typedef struct {
  int id;                                 // measure identifier, MJF 7/2019
  char measure_name[MEASURENAME_LEN + 1];  // name of the retrofit measure, MJF 7/2019
  enum LOGICAL active;                    // is measure active in analysis
} CMS;

#endif // _STRUCT_H
