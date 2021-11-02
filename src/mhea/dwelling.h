/***************************************************************************
* MODULE:	dwelling.h            CREATED:	April 29, 1997
*                               MODIFIED:   Nov 5, 1998
*
* AUTHOR:	Mark Fishbaugher
*
* MDESC:	MHEA MDI file defines and data structures
*           Contains the structure definitions for all file IO in MHEA
*
*           see enum_m.h for lists of used enumerations
*
*           see enummhea.h and structmhea.h for common structures and enumerations
*           between NEAT and MHEA
****************************************************************************/
#ifndef _M_DWELLING_H
#define _M_DWELLING_H

// ***************************************************
// The GENERAL struct is a representation of the info
// collected by the General Information input form.
// ***************************************************

// Special Note:  All typedefs specific to MHEA have the M_ prefix.
// the overall MDI structure also contains 'common' structures
// with NEAT whose typedefs do NOT has such prefixes.  Those
// 'common' typedefs can be found in common/structs.h

typedef struct {
  char audit_type[5];               // should say "MHEA"
  long audit_id;                    // The primary integer numeric identifier for the audit, pass through only
  long audit_number;                // A secondary integer numeric identifier, also just pass through
  float avg_no_occupants;           // Average Number of Occupants
  float length;                     // Dimension - Home Length
  float width;                      // Dimension - Home Width
  float height;                     // Dimension - Home Height
  enum LOGICAL do_billing_adjust;   // runs should include billing adjustments
  enum LOGICAL water_heater_closet; // Water Heater Closet check box fields
  enum WSHIELD wind_shielding;      // Wind Shielding check box fields
  enum DOOR_LEAKINESS leakiness;             // Home Leakiness check box fields
} M_GNL;

// ******************************************
// The WALL struct is a representation of the
// info collected by the Wall input form.
// ******************************************

typedef struct {
  enum STUD_SIZEMHEA stud_size;       // Wall Stud Size check box fields
  enum ORIENTATION home_orientation;  // Orientation of One Long Wall of Home
  enum WALL_VENT wall_vent;           // Wall Intentionally Ventilated
  float batt_insl;                    // Batt/Blanket Insulation Thickness
  float loose_insl;                   // Loose Fill Insulation Thickness
  float foam_insl;                    // Foam Core Insulation Thickness
  float uninsulatable_area;           // area in sqft that cannot be insulated
  float porch_length;                 // Carport/Porch Roof Length
  float porch_width;                  // Carport/Porch Roof Width
  enum ORIENTATION porch_orientation; // Carport/Porch Roof Orientation
  float add_cost;                     // Added Insulation Cost
} M_WAL;

// ******************************************
// The ADDITIONS WALL struct is a representation of the
// info collected by the Additions Wall input form.
// ******************************************

typedef struct {
  enum STUD_SIZEMHEA stud_size; // Wall Stud Size check box fields
  enum ORIENTATION orientation; // Addition Orientation
  enum WALL_VENT wall_vent;     // Wall Intentionally Ventilated
  enum WALL_CONFIG wall_config; // Wall Configuration
  float height_max;             // Interior Wall Height - Maximum
  float height_min;             // Interior Wall Height - Minimum
  float batt_insl;              // Batt/Blanket Insulation Thickness
  float loose_insl;             // Loose Fill Insulation Thickness
  float foam_insl;              // Foam Core Insulation Thickness
  float add_cost;               // Added Insulation Cost
} M_AWL;

// *********************************************
// The WINDOW struct is a representation of
// the info collected by the Window input form.
// *********************************************

typedef struct {
  char code[CODE_LEN + 1];               // Window Code
  enum WINDOW_TYPEMHEA window_type;      // Window Type check box fields
  enum FRAMETYPEMHEA frame_type;         // Frame Type check box field
  enum GLAZING_TYPEMHEA glazing_type;    // Glazing Type check box fields
  enum INTERIOR_SHADINGMHEA int_shading; // Interior Shading check box fields
  enum EXTERIOR_SHADINGMHEA ext_shading; // Exterior Shading check box fields
  enum LEAKINESS leak;                   // How leaky is the window
  float width;                           // Average Window Width
  float height;                          // Average Window Height
  float num_n;                           // Number of Windows facing North
  float num_s;                           // Number of Windows facing South
  float num_e;                           // Number of Windows facing East
  float num_w;                           // Number of Windows facing West
  enum MWINSTATUS retrofit_option;       // What do do with the window (new 8.3.2)
  enum LOGICAL inc_sir;                  // Include retrofit in Cumulative SIR ?
  float cost_seal;                       // Added per window cost for sealing retrofit
  float cost_replace;                    // Added per window cost for replacement
  float cost_add_glass_storm;            // Added per window cost for adding storms retrofit
  float cost_add_plastic_storm;          // Added per window cost for adding storms retrofit

  // Computed fields
  float leak_coef;   // Leakiness coefficient of window
  int imeas_applied; // Number of Window measure applied to window
} M_WIN;

// *******************************************
// The DOOR struct is a representation of
// the info collected by the Door input form.
// *******************************************

typedef struct {
  char code[CODE_LEN + 1];    // Door Code
  enum M_DOOR_TYPE door_type; // Door Type check box fields
  enum LOGICAL storm;         // Storm Door check box fields
  enum DOOR_LEAKINESS leakiness;   // Door leakiness  #242
  float width;                // Average Door Width
  float height;               // Average Door Height
  float num_n;                // Number of Doors facing North
  float num_s;                // Number of Doors facing South
  float num_e;                // Number of Doors facing East
  float num_w;                // Number of Doors facing West
  enum LOGICAL replace;       // Replacement required
  enum LOGICAL inc_sir;       // Include retrofit in Cumulative SIR ?
  float cost_replace;         // Added per door cost for replacement

  // computed fields MJF 3/2019
  float leak_coef;                     // leakage coefficient for door
} M_DOR;

// ***********************************************
// The ROOF struct is a representation of the
// info collected by the Roof/Ceiling input form.
// ***********************************************

typedef struct {
  enum ROOF_TYPE roof_type;   // Roof Type check box fields
  enum ROOF_COLOR roof_color; // Roof Color check box fields
  enum JOIST_SIZE joist_size; // Roof/Ceiling Joist Size for flat roots (MJF 6/06)
  float ceiling_height;       // height of peak bowstring roofs (in)
  float pitch_add_insl;       // Insulation to be added to pitched roofs (in)
  float mineral_insl;         // Mineral Fiber Batt/Blanket Insulation Thickness
  float loose_insl;           // Loose Fill Insulation Thicknedd
  float rigid_insl;           // Rigid Insulation Thickness
  float cathedral_ceiling;    // Percent Cathedral Ceiling
  enum ORIENTATION step_wall; // Orientation of Cathedral Ceiling  Step Wall
  float add_cost;             // Added Insulation Cost
} M_ROF;

// ***********************************************
// The ADDITIONS ROOF struct is a representation of the
// info collected by the Additions Roof/Ceiling input form.
// ***********************************************

typedef struct {
  enum ROOF_COLOR roof_color; // Roof Color check box fields
  enum JOIST_SIZE joist_size; // Roof/Ceiling Joist Size
  float mineral_insl;         // Mineral Fiber Batt/Blanket Insulation Thickness
  float loose_insl;           // Loose Fill Insulation Thicknedd
  float rigid_insl;           // Rigid Insulation Thickness
  float add_cost;             // Added Insulation Cost
} M_ARF;

// *******************************************
// The FLOOR struct is a representation of
// the info colleted by the Floor input form.
// *******************************************

typedef struct {
  enum LOGICAL skirt;                 // Skirt check box fields
  enum JOIST_SIZE wing_joist_size;    // FLOOR WING Joist Size
  float wing_loose_insl;              // Loose Fill Insulation Thickness
  enum INSUL_LOC wing_insl_location;  // FLOOR WING Insulation Location
  float wing_mineral_insl;            // Mineral Fiber Batt/Blanket
  enum JOIST_SIZE belly_joist_size;   // FLOOR BELLY Joist Size
  enum BELLY_CAV belly_cavity;        // Belly Cavity Configuration
  enum BELLY_COND belly_condition;    // Condition of Belly check box fields
  float belly_depth;                  // Depth of Belly Cavity
  float belly_loose_insl;             // Loose Fill Insulation Thickness
  enum INSUL_LOC belly_insl_location; // FLOOR BELLY Insulation Location
  float belly_mineral_insl;           // Mineral Fiber Batt/Blanket Insulation Thickness
  float add_cost;                     // Added Insulation Cost
} M_FLR;

// *******************************************
// The ADDITIONS FLOOR struct is a representation of the
// info colleted by the Additions Floor input form.
// *******************************************

typedef struct {
  enum ADD_FLOOR_TYPE add_floor_type; // Addition floor type
  enum LOGICAL skirt;                 // Skirt check box fields
  enum JOIST_SIZE joist_size;         // FLOOR Joist Size check box fields
  enum INSUL_LOC insl_location;       // FLOOR Insulation Location
  float length;                       // Addition Dimension - Length
  float width;                        // Addition Dimension - Width
  float mineral_insl;                 // Mineral Fiber Batt/Blanket Insulation Thickness
  float loose_insl;                   // Loose Fill Insulation Thickness
  float avail_insl;                   // Depth of Available Insulation
} M_AFL;

// ****************************************************
// The HEATING struct is a representation of the info
// collected by the Primary Heating Equipment input form.
// ****************************************************

typedef struct {
  enum EQUIP_TYPE equip_type;   // Heating Equipment Type
  enum FUEL fuel_type;          // Fuel used by this system
  float capacity;               // Rated Capacity
  enum HTG_EFF_UNITS eff_units; // Heating Efficiency rating units
  float efficiency_percent;     // Efficiency of the system in percent
  float efficiency_hspf;        // Efficiency of the system in hspf
  float efficiency_cop;         // Efficiency of the system in cop
  // float day;                        // Thermostat Setpoint during the Day, found unused (MJF 5/19)
  // float night;                      // Thermostat Setpoint at Night, found unused (MJF 5/19)
  enum DUCT_LOCATION duct_location; // Heating Duct Location
  enum DUCT_INSULATION duct_insl;   // Heating Duct Insulation Location
  float percent_heated;             // Percent of Total Heat Supplied
  enum LOGICAL smart_thermostat;    // Set Back Thermostat Exists
  enum LOGICAL tuneup;              // required primary heating system tune up
  enum LOGICAL inc_sir;             // Include tuneup in Cumulative SIR
  // char comm[COMM_LEN + 1];          // Free Form Comment
} M_HTG;

// ****************************************************
// The HEATING-2 struct is a representation of the info
// collected by the Secondary Heating Equipment input form.
// ****************************************************

typedef struct {
  enum EQUIP_TYPE equip_type;       // Heating Equipment Type
  enum FUEL fuel_type;              // Fuel used by this system
  float capacity;                   // Rated Capacity
  enum HTG_EFF_UNITS eff_units;     // Heating Efficiency rating units
  float efficiency_percent;         // Efficiency of the system in percent
  float efficiency_hspf;            // Efficiency of the system in hspf
  float efficiency_cop;             // Efficiency of the system in cop
  //enum DUCT_LOCATION duct_location; // Heating Duct Location
  //enum DUCT_INSULATION duct_insl;   // Heating Duct Insulation Location
  // char comm[COMM_LEN + 1];          // Free Form Comment
} M_HT2;

// ****************************************************
// The HEATING-R struct is a representation of the info
// collected by the Replacement Heating Equipment input form.
// ****************************************************

typedef struct {
  enum EQUIP_TYPE equip_type;       // Heating Equipment Type
  enum FUEL fuel_type;              // Fuel used by this system
  float capacity;                   // Rated Capacity
  enum HTG_EFF_UNITS eff_units;     // Heating Efficiency rating units
  float efficiency_percent;         // Efficiency of the system in percent
  float efficiency_hspf;            // Efficiency of the system in hspf
  float efficiency_cop;             // Efficiency of the system in cop
  enum DUCT_LOCATION duct_location; // Heating Duct Location
  enum DUCT_INSULATION duct_insl;   // Heating Duct Insulation Location
  enum LOGICAL replacement;         // Replacement Heating Required
  enum LOGICAL incl_costs;          // Replacement Costs Included
  float labor_cost;                 // Labor cost of replacement heating system (MJF 9/07)
  float material_cost;              // Material cost of replacment heating system (MJF 9/07)
  // char comm[COMM_LEN + 1];          // Free Form Comment
} M_HTR;

// ***************************************************
// The COOLING struct is a representation of the info
// collected by the Primary Cooling Equipment input form.
// new tuneup required and include in sir flags for v8.6 7/09
// ****************************************************

typedef struct {
  // char description[LRG_STR + 1];        // Description of equipment, not used MJF 6/2019
  enum CLG_EQUIP_TYPE equip_type; // Cooling Equipment Type
  float capacity;                 // Rated Total Capacity
  enum CLG_EFF_UNITS eff_units;   // Cooling Efficiency rating units
  float efficiency_cop;           // Rated efficiency in COP
  float efficiency_seer;          // Rated efficiency in SEER
  float efficiency_eer;           // Rated efficiency in EER
  // float day;                          // Thermostat Setpoint during the Day, not used MJF 6/2019
  // float night;                        // Thermostat Setpoint at Night, not used MJF 6/2019
  enum DUCT_LOCATION duct_location; // Cooling Duct Location
  enum DUCT_INSULATION duct_insl;   // Cooling Duct Insulation Location
  float percent_area_room_ac;       // Approx. Home Area Cooled by Room AC
  enum LOGICAL tuneup;              // required primary cooling system tune up
  enum LOGICAL inc_sir;             // Include tune up in Cumulative SIR
  // char comm[COMM_LEN + 1];              // Free Form Comment

  // computed fields MJF #334
  float saturating_eff_for_evaporative;   // from the mdi-key
} M_CLG;

// ***************************************************
// The COOLING-2 struct is a representation of the info
// collected by the Secondaryy Cooling Equipment input form.
// ****************************************************

typedef struct {
  //char description[LRG_STR + 1];        // Description of equipment
  enum CLG_EQUIP_TYPE equip_type;       // Cooling Equipment Type
  float capacity;                       // Rated Total Capacity
  enum CLG_EFF_UNITS eff_units;         // Cooling Efficiency rating units
  float efficiency_cop;                 // Rated efficiency in COP
  float efficiency_seer;                // Rated efficiency in SEER
  float efficiency_eer;                 // Rated efficiency in EER
  //enum DUCT_LOCATION clg_duct_location; // Cooling Duct Location                  MJF #70
  //enum DUCT_INSULATION clg_duct_insl;   // Cooling Duct Insulation Location       MJF #70
  float percent_area_room_ac;           // Approx. Home Area Cooled by Room AC
  // char comm[COMM_LEN + 1];              // Free Form Comment
  
  // computed fields MJF #334
  float saturating_eff_for_evaporative;   // from the mdi-key
} M_CL2;

// ****************************************************
// The COOLING-R struct is a representation of the info
// collected by the Replacement Cooling Equipment input form.
// new for v8.6 7/09
// ****************************************************

typedef struct {
  //char description[LRG_STR + 1];        // Description of equipment
  enum CLG_EQUIP_TYPE equip_type;       // Cooling Equipment Type
  float capacity;                       // Rated Total Capacity
  enum CLG_EFF_UNITS eff_units;         // Cooling Efficiency rating units
  float efficiency_cop;                 // Rated efficiency in COP
  float efficiency_seer;                // Rated efficiency in SEER
  float efficiency_eer;                 // Rated efficiency in EER
  enum DUCT_LOCATION clg_duct_location; // Cooling Duct Location
  enum DUCT_INSULATION clg_duct_insl;   // Cooling Duct Insulation Location
  enum LOGICAL replacement;             // Replacement Cooling Required
  enum LOGICAL incl_costs;              // Replacement Costs Included
  float labor_cost;                     // Labor cost of replacement cooling system (MJF 9/07)
  float material_cost;                  // Material cost of replacment cooling system (MJF 9/07)
  // char comm[COMM_LEN + 1];              // Free Form Comment
} M_CLR;

// ****************************************************
// The Retrofit Measure Costs struct is a representation of the info
// collected by the Setup - Retrofit Measure Costs input form.
// ****************************************************

typedef struct {
  //int num;                               // just an index into the table
  long measure_id;                       // database long identifier
  char retro_name[M_RETRO_NAME_LEN + 1]; // name of the retrofit measure
  float life;                            // its lifetime in years
  char units[M_RETRO_UNITS_LEN + 1];     // material costs units
  float material;                        // material cost, dollars
  float labor;                           // labor cost, dollars
  float extra;                           // other cost, dollars
} M_RMC;

// ****************************************************
// The Key struct is a representation of the info
// collected by the Setup - Defaults - Parameters input form.
// ****************************************************

typedef struct {
  float real_discount_rate;                           
  float minimum_acceptable_sir;                       
  float spending_limit;                               
  float heating_setpoint_day;                         
  float heating_setpoint_night;                       
  float cooling_setpoint_day;                         
  float cooling_setpoint_night;                       
  float thermostat_setback_amount;                    
  float length_of_night_thermostat_setback;           

  float bag_size_for_loose_fiberglass_insulation;     
  float density_of_loose_fiberglass_insulation;       
  float bag_size_for_loose_cellulose_insulation;      
  float density_of_loose_cellulose_insulation;        
  float interior_wall_r_value_winter;                 
  float interior_wall_r_value_summer;                 
  float interior_ceiling_r_value_winter;              
  float interior_ceiling_r_value_summer;              
  float interior_floor_r_value_winter;                
  float interior_floor_r_value_summer;                
  float outside_wall_r_value_winter;                  
  float outside_wall_r_value_summer;                  
  float loose_insulation_r_value_per_inch;            
  float batt_blanket_insulation_r_value_per_inch;     
  float rigid_insulation_r_value_per_inch;            
  float foamcore_insulation_r_value_per_inch;         

  float home_leakiness_loose;                         
  float home_leakiness_medium;                        
  float home_leakiness_tight;                         
  float free_heat_from_interior_sources_day;          
  float free_heat_from_interior_sources_night;        
  float duct_sealing_distribution_loss_reduction;     
  float duct_insulation_dist_loss_reduction;          
  float evaporative_cooler_actual_saturating_eff;     
  float saturating_eff_for_evaporative_tune_up;       
  float saturating_eff_for_evaporative_rplcmnt;       
  float cooling_system_fan_power;                     

  float door_u_value_wood_with_hollow_core;           
  float door_u_value_wood_with_solid_core;            
  float door_u_value_standard_mfg_home_door;          
  float u_value_of_replacement_door;                  

  float window_u_value_1_glazing_winter;              
  float window_u_value_1_glazing_summer;              
  float window_u_value_2_glazing_winter;              
  float window_u_value_2_glazing_summer;              
  float window_u_value_1_glass_storm_winter;          
  float window_u_value_1_glass_storm_summer;          
  float window_u_value_2_glass_storm_winter;          
  float window_u_value_2_glass_storm_summer;          
  float window_u_value_1_plastic_storm_winter;        
  float window_u_value_1_plastic_storm_summer;        
  float window_u_value_2_plastic_storm_winter;        
  float window_u_value_2_plastic_storm_summer;        
  float skylight_u_value_1_glazing_winter;            
  float skylight_u_value_1_glazing_summer;            
  float skylight_u_value_2_glazing_winter;            
  float skylight_u_value_2_glazing_summer;            
  float skylight_u_value_1_glass_storm_winter;        
  float skylight_u_value_1_glass_storm_summer;        
  float skylight_u_value_2_glass_storm_winter;        
  float skylight_u_value_2_glass_storm_summer;        
  float skylight_u_value_1_plstc_storm_winter;        
  float skylight_u_value_1_plstc_storm_summer;        
  float skylight_u_value_2_plstc_storm_winter;        
  float skylight_u_value_2_plstc_storm_summer;        
  float window_shading_r_value_drapes;                
  float window_shading_r_value_blinds_shades;         
  float window_shading_r_value_drapes_shades;         
  float sun_screen_solar_trans_reduction_summer;      
  float sun_screen_solar_trans_reduction_winter;      
  float ratio_of_awning_depth_to_window_height;       
  
  float low_flow_shower_head_flow_rate;               
  float water_heater_wrap_added_r_value;              
  float refrigerator_defrost_cycle_energy;            
} M_KEY;


// ---------------drum roll please-------------------------

typedef struct {
  M_GNL gnl; // Start of mobile home desc structs

  WTH wth;   // weather file selection

  M_WAL wal; // wall in main home
  M_AWL awl; // wall in addition

  int num_win; // windows in the main home
  M_WIN win[MHEA_MAX_WIN];
  int num_awn; // same for addition, same structure typdef
  M_WIN awn[MHEA_MAX_WIN];

  int num_dor; // doors in main home
  M_DOR dor[MHEA_MAX_DOR];
  int num_adr; // in addition
  M_DOR adr[MHEA_MAX_DOR];

  M_ROF rof; // Roof/Ceiling main home
  M_ARF arf; // in addition

  M_FLR flr; // Floors in main home
  M_AFL afl; // in addition

  M_HTG htg; // primary heating system info
  M_HT2 ht2; // secondary heating system
  M_HTR htr; // heating system replacement

  M_CLG clg; // primary cooling system info
  M_CL2 cl2; // secondary cooling system info
  M_CLR clr; // cooling system replacement

  INF inf; // stuctures common with NEAT
  DWH dwh;
  REF ref;

  int num_ltg;
  LTG ltg[MAX_LTG];
  int num_itc;
  ITC itc[MAX_ITC];

  // int num_ubi;   #58  5/2019
  // UBI *ubi;
  // int num_ubr;
  // UBR *ubr;

  UBI ubh; // Utility Bill info for Heating
  int num_urh;
  UBR urh[MONTHS + 1]; // Utility Records for Heating

  UBI ubc; // Utility Bill info for Cooling
  int num_urc;
  UBR urc[MONTHS + 1]; // Utility Records for Cooling

  FCS fcs;

  RER rer; // optional referenced rates, added MJF 1/2019
  int num_fer;
  FER fer[FUEL_TYPES];

  int num_cms;
  CMS cms[MHEA_MAX_CMS];

  int num_rmc;
  M_RMC rmc[MHEA_MAX_RMC];

  M_KEY key;

} MDI; // Mhea Dwelling Input

#endif
