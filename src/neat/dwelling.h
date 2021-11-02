/***************************************************************************
* MODULE:       structs.h            CREATED:   April 29, 1997
*               struct_n.h           MODIFIED:   June  30, 1998 for NEAT
*                                   MODIFIED:   Nov 6, 19988  MJF
*
* AUTHOR:       SLF, Mark Fishbaugher, MB Gettings
*
* MDESC:        Re-write by MBG for use with NEAT
*
*               Contains the structure definitions for all file IO in NEAT
****************************************************************************/
#ifndef _N_DWELLING_H
#define _N_DWELLING_H

// ***************************************************
// The GENERAL struct is a representation of the info
// collected by the General House Data input form.
// ***************************************************

typedef struct {
  char audit_type[5];             // should say "NEAT"
  long audit_id;                  // The primary integer numeric identifier for the audit, pass through only
  long audit_number;              // A secondary integer numeric identifier, also just pass through
  enum LOGICAL do_billing_adjust; // Runs should include billing adjustments
  enum LOGICAL impute_cooling;    // cooling savings should be imputed
  float no_cond_stories;          // Number of Conditioned stories
  float floor_area;               // Living Space Floor Area (sqft)
  float avg_no_occupants;         // Average Number of Occupants
} N_GNL;

// ******************************************
// The WALL struct is a representation of the
// info collected by the Wall input form.
// ******************************************

typedef struct {
  char code[CODE_LEN + 1];              // Wall Code
  enum WALLLOADTYPE wall_type;          // Type of Load-bearing Wall
  enum STUD_SIZE stud_size;             // Wall Stud Size check box fields
  enum WALLEXTRTYPE ext_type;           // Wall Exterior Surface Type
  enum WALLEXPOSURE exposure;           // Conditions outside wall exposed to
  enum ORIENTATION orient;              // Orientation of Wall
  float area;                           // Wall Area (Sqft)
  enum WALLEXTINSTYPE exist_insulation; // Existing Insulation Type for Wall
  float exist_r;                        // Existing Insulation R-Value
  enum WALLADDINSTYPE added_insulation; // Added Insulation Type for Wall
  float add_cost;                       // Added Insulation Cost
  int measure_number;                   // Wall Measure number

  // computed fields
  enum MEASURE_COMPONENT_GROUP_TYPE comp_group_type;    // component grouping type/category
  int comp_group_num;                                   // The measure group number for this component uas based on measure_number
  float u_frame;                       // u value of the wall frame   (same as u_cavity for non-framed walls)
  float u_cavity;                      // u value of the wall cavity  (same as u_frame for non-framed walls)
  float ua_value;                      // aggregate ua value for the wall frame and cavity
  enum SOLAR_ORIENTATION solar_orient; // compass orientation used to index various arrays
  float wall_air_space_thickness;      // (inches) Added 5/08 to give more accurate cavity R-value
} N_WAL;

// *********************************************
// The WINDOW struct is a representation of
// the info collected by the Window input form.
// I re-arranged the order of fields in this structure to
// more closely match the arrangement of the fields on the WA input form (MJF 9/08)
// *********************************************

typedef struct {
  // primary inputs from API JSON
  char code[CODE_LEN + 1];           // Window Code
  enum WINDOW_TYPE window_type;      // Window Type check box fields
  enum FRAME_TYPE frame_type;        // Frame Type check box field
  enum GLAZINGTYPE glazing_type;     // Glazing Type scroll box fields
  enum INTERIOR_SHADING int_shading; // Interior Shading check box fields
  enum EXTERIOR_SHADING ext_shading; // Exterior Shading check box fields (future reference to match MHEA at 8.4)
  float shade;                       // Time/Area Avg Percent Window is Shaded
  enum LEAKINESS leak;               // How leaky is the window
  float width;                       // Average Window Width in inches
  float height;                      // Average Window Height in inches
  char wall[CODE_LEN + 1];           // Code of Wall Window lies on
  float number;                      // Number of Windows

  enum WINSTATUS retrofit_option;    // What do do with the window
  enum LOGICAL inc_sir;              // Force include retrofit in Cumulative SIR
  float cost_seal;                   // Added per window cost for sealing retrofit
  float cost_replace;                // Added per window cost for replacement
  float cost_low_e;                  // Added per window cost for replacement with lowE window
  float cost_add_storm;              // Added per window cost for adding storms retrofit
  // char comm[COMM_LEN + 1];           // Free Form Comment

  // computed fields MJF 3/2019
  float leak_coef;                     // leakage coefficient
  float u_value;                       // u value the window
  float area_gross;                    // sq ft of gross opening
  float area_glazing;                  // sq ft of glazing only (minus frame allowance)
  float shgc_winter;                   // solar heat gain coefficients Winter
  float shade_factor_winter;           // really a not-shaded transmisson fraction
  float shgc_summer;                   // Summer
  float shade_factor_summer;           // really a not-shaded transmisson fraction
  enum SOLAR_ORIENTATION solar_orient; // compass orientation used to index various arrays
  float avg_leak_cfm[MONTHS + 1];      // window monthly avg leakage in cfm
} N_WIN;

// *******************************************
// The DOOR struct is a representation of
// the info collected by the Door input form.
// *******************************************

typedef struct {
  char code[CODE_LEN + 1];        // Door Code
  enum DOOR_TYPE door_type;       // Door Type check box field

  float area;                     // Door Area
  enum STORM_DOOR_INFO condition; // Storm Door condition information
  enum DOOR_LEAKINESS leakiness;           // Door leakiness           (new in 8.8)

  // float width;                     // Door Width  removed Issue #35
  // float height;                    // Door Height
  char wall[CODE_LEN + 1];            // Code of Wall Door lies on
  float number;                       // Number of Doors
  enum LOGICAL replace;               // Mandatory replacement    (new in 8.8)
  enum LOGICAL inc_sir;               // Include in SIR           (new in 8.8)
  float cost;                         // Added Insulation Cost for replacement door (new in 8.9 4/2011)

  // computed fields MJF 3/2019
  float u_value;                       // u value the door
  enum SOLAR_ORIENTATION solar_orient; // compass orientation used to index various arrays
  float leak_coef;                     // leakage coefficient for door
  float avg_leak_cfm[MONTHS + 1];      // door monthly avg leakage in cfm
} N_DOR;

// ***********************************************
// The UNFINISHED ATTIC struct is a representation of the
// info collected by the Unfinished Attic input forms.
// ***********************************************

typedef struct {
  char code[CODE_LEN + 1];               // Attic Code
  enum UNFINISHED_ATTIC_TYPE attic_type;           // Attic Type check box
  float joist_sp;                        // Attic Floor Joist Spacing
  float area;                            // Attic Floor or Ceiling Area
  enum ROOF_COLOR roof_color;            // Roof color GKA 05/2010
  enum ATTICEXTINSTYPE exist_insulation; // Existing Insulation Type for Attic
  float ins_depth;                       // Existing Insulation Depth (in)
  enum ATTICADDINSTYPE added_insulation; // Added Insulation Type for Attic
  float max_depth;                       // Maximum Depth of Attic Insulation
  float added_R;                         // User specified added R-value
  float cost;                            // Added Insulation Cost for Attic
  int measure_number;                    // Attic Measure number
} N_ATC;

// ***********************************************
// The FINISHED ATTIC struct is a representation of the
// info collected by the Finished Attic input forms.
// ***********************************************

typedef struct {
  char code[CODE_LEN + 1];                // Attic Code (multiple finished attic mod MJF 7/04)
  enum FINISHED_ATTIC_TYPE acode;         // Attic Type check box
  enum FINISHED_ATTIC_FLOOR_TYPE floor_type;           // Attic Floor Type check box
  float area;                             // Attic Floor or Ceiling Area
  enum ROOF_COLOR roof_color;             // Roof color GKA 05/2010
  enum ATTICEXTINSTYPE exist_insulation;  // Existing Insulation Type for Attic
  float ins_depth;                        // Existing Insulation Depth (in)
  enum ATTICADDINSTYPE added_insulation;  // Added Insulation Type for Attic (other than kneewall)
  enum KNEEWALLADDINSTYPE added_kneewall; // Added Insulation to the kneewall, 0 for non-kneewall, MJF 10/06
  float max_depth;                        // Maximum Depth of Attic Insulation
  float added_R;                          // User specified added R-value
  float cost;                             // Added Insulation Cost for Attic
  int measure_number;                     // Finished Attic Measure number
} N_FAT;

// ***********************************************
// Unfinished attic spaces combined/computed from
// N_ATC N_FAT N_WAL
// ***********************************************

typedef struct {                         // combined unfinished attic spaces (both N_ATC and N_WAL with attic exposure plus N_FAT)
  char code[CODE_LEN + 1];               // Attic Code
  int audit_section_id;                  // what section of audit this this section come from
  enum UAS_ATTIC_TYPE attic_type;        // Attic Type of combined spaces
  float joist_sp;                        // Attic Area Joist Spacing
  enum ROOF_COLOR roof_color;            // Roof color
  float area;                            // Area in sq ft
  enum ATTICEXTINSTYPE exist_insulation; // Existing Insulation Type for Attic
  float ins_depth;                       // Existing Insulation Depth (in)
  enum ATTICADDINSTYPE added_insulation; // Added Insulation Type for Attic if attic_type != UAS_KNEEWALL, otherwise AA_NONE
  enum KNEEWALLADDINSTYPE added_kneewall;// Added Insulation to the kneewall if attic_type = UAS_KNEEWALL, otherwise AS_NONE
  float max_depth;                       // Maximum Depth of Attic Insulation or
  float added_R;                         // User specified added R-value
  float cost;                            // Added Insulation Cost for Attic
  int measure_number;                    // Attic Measure number entered by user and unchanged by code

  // computed fields
  enum MEASURE_COMPONENT_GROUP_TYPE comp_group_type;    // component grouping type/category
  int comp_group_num;                                   // The measure group number for this component uas based on measure_number
  float u_value;
  float r_value;
  float frame_u_value;
  float ventilation_cuft_per_hr;
  float ua_value;
  float joist_u_value;
  int various_r_values_in_measure;     // set to TRUE if various added_R values are mixed in the same measure_number #349
  // double pitch;
  enum SOLAR_ORIENTATION solar_orient; // index into various solar based arrays
  float roof_absorptance;
} N_UAS;

typedef struct {
  int last_measure_number; // last measure number for unfinished attics
} N_UASS;

// *******************************************
// The FOUNDATION struct is a representation of
// the info colleted by the Foundation input form.
// *******************************************

typedef struct {
  char code[CODE_LEN + 1];               // Foundation Space Code
  enum FOUNDATIONTYPE space_type;        // Foundation Space Type
  int measure_number;                    // Foundation Space Measure number

  float area;                            // Foundation Area (Sqft)
  enum FLOORADDINSTYPE added_floor;      // Type of insulation added to floor
  float flr_ins_r;                       // Foundation Ceiling (living area floor) R-Value
  float floor_cost;                      // Added Insulation Cost for floor

  float joist_height;                    // Height of the floor joists in inches added 8.3
  enum SILLADDINSTYPE added_sill;        // Type of insulation added to Sill
  //float sill_perim;                    // Length of sill which can be insulated (ft) added 8.3
  float sill_perimeter;                  // Total length of the foundation sill (not just uninsulated perimeter) #232
  float sill_r;                          // Sill existing R-Value
  float sill_cost;                       // Added Insulation Cost for addint sill insulation

  float wall_height;                     // Foundation Space Wall Height (ft)
  enum FOUNDATIONADDINSTYPE added_found; // Type of insulation added to foundation walls
  float perim_length;                    // Perimeter of Foundation Space (full)
  float wall_exp;                        // Percent of Foundation Wall Exposed
  float wall_ins_r;                      // Foundation Wall R-Value
  float wall_cost;                       // Added Insulation Cost for Foundation wall

  // computed fields
  enum MEASURE_COMPONENT_GROUP_TYPE comp_group_type;    // component grouping type/category
  int comp_group_num;                                   // The measure group number for this component uas based on measure_number
  float floor_u_value;
  float above_grade_wall_u_value;
  float below_grade_wall_u_value;
  float wall_area;
  float below_grade_wall_height;
  float equipment_waste_heat; // Btu/h-F
  float ua_value_basement;
  float ua_value_basement_sill;
  float ua_value_basement_effective;
  float floor_cavity_r;
  float floor_framing_r;
  float u_value_basement_wall_total;
} N_FND;

typedef struct {
  int subspace_with_ductwork; // index of the largest unitentionally heated sub space
} N_FNDS;

// ****************************************************
// The HEATING struct is a representation of the info
// collected by the Heating System Descriptoin input form.
// ****************************************************

typedef struct {
  char code[CODE_LEN + 1];       // Heating Equipment Code
  enum HTGEQUIPTYPE system_type; // Heating Equipment Type
  enum FUEL fuel_type;           // Fuel used by Heating Equipment
  enum EQUIPLOCATION location;   // Location of Heating Equipment
  float percent_heat_supplied;   // Percent of Total Heat Supplied % not fraction
  enum LOGICAL primary;          // true if this record is the primary system
  enum LOGICAL replace_system;   // true if system is to be replaced with new primary system

  enum DUCTLOCATION duct_location;         // Uninsulated Heating Supply Duct Location
  enum DUCTSHAPE duct_type1;               // Uninsulated duct cross section type
  float duct_length1;                      // Length of uninsulated duct (ft)
  float duct_width1;                       // Width of uninsulated duct (in)
  float duct_height1;                      // Height of uninsulated duct (in)
  float duct_diameter1;                    // Diameter of round uninsulated duct (in)
  enum DUCTSHAPE duct_type2;               // Uninsulated duct cross section type
  float duct_length2;                      // Length of uninsulated duct (ft)
  float duct_width2;                       // Width of uninsulated duct (in)
  float duct_height2;                      // Height of uninsulated duct (in)
  float duct_diameter2;                    // Diameter of round uninsulated duct (in)
  enum DUCTSHAPE duct_type3;               // Uninsulated duct cross section type
  float duct_length3;                      // Length of uninsulated duct (ft)
  float duct_width3;                       // Width of uninsulated duct (in)
  float duct_height3;                      // Height of uninsulated duct (in)
  float duct_diameter3;                    // Diameter of round uninsulated duct (in)

  float hspf;                              // HSPF of Heatpump
  float year_manufactured;                 // Approx. Year System was manufactured

  enum HTGUNITS input_units;               // Units for Htg Input/Output Rating
  float output_capacity;                   // Rated Output Capacity kBtu/hr
  float steady_state_eff;                  // Htg System SS Efficiency removed in favor of efficiency and efficiency_units v9

  enum EQUIPCONDITION condition;           // Htg System Condition
  enum LOGICAL smart_thermostat;           // Set Back Thermostat Exists ?
  enum LOGICAL vent_damper_present;        // Automatic Vent Damper Exists ?
  enum LOGICAL vent_damper_recommended;    // Recommend Automatic Vent Damper ?
  enum LOGICAL pilot_light_present;        // Pilot Light Exists ?
  enum LOGICAL pilot_light_on_summer;      // Pilot Light on in Summer?
  enum LOGICAL intermittent_ignition;      // IID Exists ?
  enum LOGICAL retention_head_burner;      // Retention Head Burner Exists ?
  enum LOGICAL retention_head_recommended; // Retention Head Recommended ?
  enum LOGICAL power_burner;               // Power Burner Exists ?

  enum EQUIPRETSTATUS retrofit_option;     // System Replacement/Tuneup Status
  enum LOGICAL inc_sir;                    // Include retrofit in Cumulative SIR ? (added 8.4)
  float retrofit_hspf;                     // HSPF of Replacment Heat Pump Equipment (added MJF 7/2019 for first normal form)
  float retrofit_afue_standard_eff;        // AFUE of Standard Eff Replacment Equipment
  float retrofit_afue_high_eff;            // AFUE of High Eff Replacement Equipment
  enum FUEL retrofit_fuel_type;            // Fuel used by replacement heating system
  float labor_cost_standard_eff;           // Labor Cost to Rep with Standard Eff Equipment
  float material_cost_standard_eff;        // Material Cost to Rep with Standared Eff Equipment
  float labor_cost_high_eff;               // Labor Cost to Rep with High Eff Equipment
  float material_cost_high_eff;            // Material Cost to Rep with High Eff Equipment


  // char comm[COMM_LEN + 1];                 // Free Form Comment
  // New v9 inputs
  // float               efficiency;
  // enum HTGEFFUNIT     efficiency_units;
  // float               other_cost_standard_eff;
  // float               labor_cost_tuneup;
  // float               material_cost_tuneup;
  // float               other_cost_tuneup;
  // float                       ss_to_seas_factor;
  // enum VENTCONFIGURATION     vent_configuration;
  // end of new v9 inputs

  // computed fields MJF 7/2019
  float delivered_eff;                  // Htg System delivered Efficiency
} N_HTG;

// ****************************************************
// The COOLING struct is a representation of the info
// collected by the Primary Cooling Equipment input form.
// ****************************************************

typedef struct {
  char code[CODE_LEN + 1];       // Cooling Equipment Code
  enum CLGEQUIPTYPE system_type; // Cooling Equipment Type
  float size;                    // Rated Total Capacity (kbtu/hr)
  float area_cooled;             // Floor Area Cooled by System
  float seer;                    // Rated SEER
  float year_manufactured;       // Approx. Year System was manufactured

  enum LOGICAL replace; // Cooling system required replacment (8.6)
  enum LOGICAL tune_up; // Cooling system required tuneup - mutually exclusive with replacment (8.6)
  enum LOGICAL inc_sir; // Include replacement or tune up cost in Cumulative SIR ? (added 8.6)

  // version 9 additions relative to 8.5
  // float efficiency; // units of COP
  // enum CLGEFFUNIT efficiency_units;
  // enum LOGICAL primary;
  // float replacement_seer;
  // float tuneup_seer;
  // float replacement_labor_cost;
  // float tuneup_labor_cost;
  // float replacement_material_cost;
  // float tuneup_material_cost;
  // float replacement_other_cost;
  // float tuneup_other_cost;
  // enum EQUIPRETSTATUS replacement_equipment_option;
  // end of version 9 additions

  // char comm[COMM_LEN + 1]; // Free Form Comment

  // computed fields MJF 7/2019
  enum LOGICAL eliminate_system; // is this system to be eliminated by retrofit
  float area_cooled_ratio;       // ratio of floor area cooled by system to total cooled area
} N_CLG;

typedef struct {
  float size_replaced_by_heatpump; // Sum of ac unit sizes being replaced by heatpump
  float avg_seer;                  // ( Btu/Wt-hr ) = Cooling seasonal energy eff. ratio, area weighted avg of systems
  float fraction_cooled;           // fraction of the house floor area that is cooled
} N_CLGS;

// ****************************************************
// The Retrofit Measure Costs struct is a representation of the info
// collected by the Setup - Retrofit Measure Costs input form.
// ****************************************************

typedef struct {
  int material_id;                  // material number 0 - MAX_RMC - 1
  // enum LOGICAL idefined;            // true if this record is for a defined insulation type (8.3)
  // int itype;                        // the insulation type number 1-13 for defined insulation types, otherwise 0 (8.3)
  // int inum;                         // the insulation number 1-6 for a defined insulation types, otherwise 0 (8.3)
  char material[MATERIAL_LEN + 1];  // name of the retrofit measure
  char retrofit_type[TYPE_LEN + 1]; // type name of the retrofit measure
  float life;                       // its lifetime in years
  char units[RETRO_UNIT_LEN + 1];   // material costs units
  float matcost;                    // material cost, dollars
  float labcost;                    // labor cost, dollars
  float itemcost;                   // other cost, dollars

  // computed fields MJF 4/2020 #164 remove retrofit_materials magic
  float quant;
  int ecm_index;                    // index into the nir->ecm[] array where this material is used
  int measure_index;                // the base 1 index where this material is used in output JSON measures[] arrray
} N_RMC;

// ****************************************************
// The Defaults struct is a representation of the info
// collected by the Setup - Defaults - Parameters input form.
// ****************************************************

// typedef struct {
//   char param_name[N_CMS_NAME_LEN + 1]; // key parameter name
//   char units[N_CMS_UNITS_LEN + 1];     // the units for the parameter
//   float value;                       // the value of the parameter changed from char* MJF 11/18
// } N_DEF;

// ****************************************************
// Standard and user defined insulation types
// ****************************************************

typedef struct {
  char insul_name[31];  // Name of the insulation type
  char units[5];        // is the value in R or R/in units
  float value;          // Either the R or R/inch value
} N_INS;

// ****************************************************
// Key parameters, previousl the N_DEF array
// ****************************************************

typedef struct {
  float real_discount_rate;
  float minimum_acceptable_sir;
  float daytime_heating_setpoint;
  float nighttime_heating_setpoint;
  float daytime_cooling_setpoint;
  float nighttime_cooling_setpoint;
  float nighttime_heating_setback;
  float annual_outside_film_coeff;
  float base_free_heat_from_internals;
  float r_value_uninsulated_other_wall;
  float r_value_exterior_siding_other;
  float new_window_ac_seer;
  float new_central_ac_seer;
  float new_heatpump_cooling_seer;
  //float impute_cooling_seer;
  float new_showerhead_gpm;
  float new_dwh_blanket_r_value;
  float single_defrost_kwh;
  float new_duct_insulation_r_value;
  float new_standard_window_u_value;
  float new_standard_window_shgc;
  float new_lowe_window_u_value;
  float new_lowe_window_shgc;
  float storm_window_inside_emittance;
  float storm_window_shgc;
  float window_film_shgc;
  float window_film_emittance;

  // computed fields MJF 7/2019
  float window_film_delta_r;
} N_KEY;

// ---------------drum roll please for the whole enchalada -------------------------
// note that the size is static... and should stay that way (MJF 5/2019)

typedef struct {

  // FIRST items associated with the Audit and assoicated computed or values derived from the audit
  // typedefs that begin with N_ are NEAT specific, while other typdegs are common with MHEA

  N_GNL gnl;

  WTH wth;   // weather file selection

  int num_wal;
  N_WAL wal[NEAT_MAX_WAL];

  int num_win;
  N_WIN win[NEAT_MAX_WIN];

  int num_dor;
  N_DOR dor[NEAT_MAX_DOR];

  int num_atc;
  N_ATC atc[NEAT_MAX_ATC];

  int num_fat;
  N_FAT fat[NEAT_MAX_FAT];

  int num_fnd;
  N_FND fnd[NEAT_MAX_FND];
  N_FNDS fnds;

  int num_htg;
  N_HTG htg[NEAT_MAX_HTG];

  int num_clg;
  N_CLG clg[NEAT_MAX_CLG];
  N_CLGS clgs;

  INF inf;

  // Start of baseload portion of Audit

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

  // SECOND are things that are associated with the Setup Library

  FCS fcs; // Start of Setup Library portion

  RER rer; // optional referenced rates, added MJF 1/2019
  int num_fer;
  FER fer[FUEL_TYPES];

  int num_cms;
  CMS cms[MAXMEAS];

  int num_rmc;
  N_RMC rmc[N_MAX_RMC];     // room for measure costs from library PLUS materials we tack onto the array

  int num_ins;
  N_INS ins_attic[NEAT_MAX_TOT_INS];
  N_INS ins_kneewall[NEAT_MAX_TOT_INS];
  N_INS ins_wall[NEAT_MAX_TOT_INS];
  N_INS ins_floor[NEAT_MAX_TOT_INS];
  N_INS ins_sill[NEAT_MAX_TOT_INS];
  N_INS ins_foundation[NEAT_MAX_TOT_INS];

  N_KEY key; // a more direct/clear definition of the key parameters by name rather than the prior N_DEF ordered array

  // LAST are computed/derived inputs attached to the main
  // bld structure and combined with the structure for memory allocation and free
  // NOTE: each sub structure may have computed/derived variables also placed within
  // the bld structure for convenience and clear assoication to the various input components

  // the whole uas array is a computed array containing N_ATC, N_FAT, and N_WAL elements
  // each having in common some unfinished attic element
  int num_uas;
  N_UAS uas[NEAT_MAX_UAS];
  N_UASS uass;

} NDI;    // Neat Dwelling Input

#endif
