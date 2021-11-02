/***************************************************************************
* MODULE:       definition.h            CREATED:       9/7/01
*
* AUTHOR:       Mark Fishbaugher  2001-2020
*
****************************************************************************/
#ifndef _N_DEFINITION_H
#define _N_DEFINITION_H

#define NEAT_INPUT_JSON_SCHEMA_FILE SCHEMA_DIR "neat_input_schema.json"
#define NEAT_OUTPUT_JSON_SCHEMA_FILE SCHEMA_DIR "neat_output_schema.json"

// Neat audit_section_ids SYNC WITH WEB_UI e_audit_panel TABLE
// this is a subset of that table for ids that may be associated with 
// a measure.  Order is roughly the order of appearance in audit dock of web_ui

#define N_AUDIT 64
#define N_WALL 1
#define N_WINDOW 60
#define N_DOOR 61
#define N_UNFINISHED_ATTIC 3
#define N_FINISHED_ATTIC 4
#define N_FOUNDATION 6
#define N_HEATING 8
#define N_COOLING 22
#define N_DUCTS_AND_INFILTRATION 24
#define N_WATER_HEATING 28
#define N_REFRIGERATOR 30
#define N_LIGHTING 32
#define N_HEALTH_AND_SAFETY 34
#define N_ITEMIZED_COST 36

// A minimum value > 0 indicates that a record must exist.  A zero value allows missing
// records of that type

// IMPORTANT NOTE:  Make sure these min/maximums match the schema.json

#define NEAT_MAX_WAL 18 // maximum number of allowable walls
#define NEAT_MIN_WAL 1

#define NEAT_MAX_WIN 18 // maximum number of windows
#define NEAT_MIN_WIN 0

#define NEAT_MAX_DOR 10 // doors
#define NEAT_MIN_DOR 0

#define NEAT_MAX_ATC 7 // unfinished attics
#define NEAT_MIN_ATC 0

#define NEAT_MAX_FAT 16 // finished attics reasonalble limit is 4 spaces x 4 space types
#define NEAT_MIN_FAT 0

#define NEAT_MAX_FND 5 // sub basements or foundations
#define NEAT_MIN_FND 0

#define NEAT_MAX_CLG 5 // air conditioners (cooling)
#define NEAT_MIN_CLG 0

#define NEAT_MAX_HTG 5 // heating systems
#define NEAT_MIN_HTG 1

#define NEAT_MAX_INF 1 // duct/infiltration records (1to1)
#define NEAT_MIN_INF 1

#define NEAT_MAX_DWH 1 // base load/ water heater records (1to1)
#define NEAT_MIN_DWH 0

#define NEAT_MAX_REF 1 // base load/ refrigerator records (1to1)
#define NEAT_MIN_REF 0

#define NEAT_MAX_INS 36 // number of user defined insulation types (must have all)s 6 areas times 6 types
#define NEAT_MIN_INS NEAT_MAX_INS

#define NEAT_MAX_TOT_INS 8 // Iinsulation types for(wall, sill etc), index 0 unused, index 1 = None, 2 = first defined INS type

#define NEAT_MAX_UAS NEAT_MAX_ATC + NEAT_MAX_FAT + 7 // max number of attic areas, unfinished, finished, walls with attic buffer

#define NEAT_MAX_MEASURE_NUMBER 10

// CMS array based defines. Order of the CMS input array.

#define N_CMS_NONE                         -1
#define N_CMS_ATTIC_INSULATION_R11          0
#define N_CMS_ATTIC_INSULATION_R19          1
#define N_CMS_ATTIC_INSULATION_R30          2
#define N_CMS_ATTIC_INSULATION_R38          3
#define N_CMS_FILL_CEILING_CAVITY           4
#define N_CMS_SILLBOX_INSULATION            5
#define N_CMS_FOUNDATION_WALL_INSULATION    6
#define N_CMS_FLOOR_INSULATION_R11          7
#define N_CMS_FLOOR_INSULATION_R19          8
#define N_CMS_FLOOR_INSULATION_R30          9
#define N_CMS_WALL_INSULATION               10
#define N_CMS_KNEEWALL_INSULATION           11
#define N_CMS_DUCT_INSULATION               12
#define N_CMS_WINDOW_SEALING                13
#define N_CMS_STORM_WINDOWS                 14
#define N_CMS_WINDOW_REPLACEMENT            15
#define N_CMS_LOW_E_WINDOWS                 16
#define N_CMS_WINDOW_SHADING_AWNING         17
#define N_CMS_SUN_SCREEN_FABRIC             18
#define N_CMS_SUN_SCREEN_LOUVERED           19
#define N_CMS_WINDOW_FILM                   20
#define N_CMS_THERMAL_VENT_DAMPER           21
#define N_CMS_ELECTRIC_VENT_DAMPER          22
#define N_CMS_IID                           23
#define N_CMS_ELECTRIC_VENT_DAMPER_AND_IID  24
#define N_CMS_FLAME_RETENTION_BURNER        25
#define N_CMS_FURNACE_TUNE_UP               26
#define N_CMS_REPLACE_HEATING_SYSTEM        27
#define N_CMS_HIGH_EFFICIENCY_FURNACE       28
#define N_CMS_HIGH_EFFICIENCY_BOILER        29
#define N_CMS_SMART_THERMOSTAT              30
#define N_CMS_TUNE_UP_AC                    31
#define N_CMS_REPLACE_AC                    32
#define N_CMS_EVAPORATIVE_COOLER            33
#define N_CMS_INSTALL_OR_REPLACE_HEATPUMP   34
#define N_CMS_LIGHTING_RETROFITS            35
#define N_CMS_REFRIGERATOR_REPLACEMENT      36
#define N_CMS_WATER_HEATER_TANK_INSULATION 	37
#define N_CMS_WATER_HEATER_PIPE_INSULATION  38
#define N_CMS_LOW_FLOW_SHOWERHEADS          39
#define N_CMS_WATER_HEATER_REPLACEMENT      40
#define N_CMS_ATTIC_INSULATION_R49          41
#define N_CMS_FLOOR_INSULATION_R38          42
#define N_CMS_DOOR_REPLACEMENT              43
#define N_CMS_WHITE_ROOF_COATING            44
#define N_CMS_FILL_FLOOR_CAVITY             45

#define NEAT_MAX_CMS N_CMS_FILL_FLOOR_CAVITY + 1        // number of conservation measures flags from JSON user input
#define NEAT_MIN_CMS NEAT_MAX_CMS                       // must have them all to do a run

#define N_CMS_INFILTRATION_REDUCTION NEAT_MAX_CMS       // Extra two measures (always on) tacked onto the end of the list, always considered
#define N_CMS_DUCT_SEALING NEAT_MAX_CMS + 1             // Always considered, always LAST measure
#define MAXMEAS NEAT_MAX_CMS + 2                        // all our cms measure plus the two that are always on, does not include ITC

#define N_CMS_ITEMIZED_COST NEAT_MAX_CMS + 3            // Index of first itemized cost measure which are N_CMS_ITEMIZED_COST + i

// Material IDs (index into rmc)

#define N_MAT_NONE                             -1
#define N_MAT_ATTIC_INSULATION_R11_CELL         0
#define N_MAT_ATTIC_INSULATION_R19_CELL         1
#define N_MAT_ATTIC_INSULATION_R30_CELL         2
#define N_MAT_ATTIC_INSULATION_R38_CELL         3
#define N_MAT_ATTIC_INSULATION_R11_FIBER        4
#define N_MAT_ATTIC_INSULATION_R19_FIBER        5
#define N_MAT_ATTIC_INSULATION_R30_FIBER        6
#define N_MAT_ATTIC_INSULATION_R38_FIBER        7
#define N_MAT_ATTIC_INSULATION_R11_UT3          8
#define N_MAT_ATTIC_INSULATION_R19_UT3          9
#define N_MAT_ATTIC_INSULATION_R30_UT3          10
#define N_MAT_ATTIC_INSULATION_R38_UT3          11
#define N_MAT_ATTIC_INSULATION_R11_UT4          12
#define N_MAT_ATTIC_INSULATION_R19_UT4          13
#define N_MAT_ATTIC_INSULATION_R30_UT4          14
#define N_MAT_ATTIC_INSULATION_R38_UT4          15
#define N_MAT_WALL_INSULATION_CELL              16
#define N_MAT_WALL_INSULATION_UT2               17
#define N_MAT_WALL_INSULATION_UT3               18
#define N_MAT_KNEEWALL_INSULATION               19
#define N_MAT_SILLBOX_INSULATION                20
#define N_MAT_FLOOR_INSULATION_R11              21
#define N_MAT_FLOOR_INSULATION_R19              22
#define N_MAT_FLOOR_INSULATION_R30              23
#define N_MAT_FOUNDATION_WALL_INSULATION        24
#define N_MAT_DUCT_INSULATION                   25
#define N_MAT_THERMAL_VENT_DAMPER               26
#define N_MAT_ELECTRIC_VENT_DAMPER              27
#define N_MAT_IID                               28
#define N_MAT_ELECTRIC_VENT_DAMPER_AND_IID      29
#define N_MAT_FLAME_RETENTION_BURNER            30
#define N_MAT_FURNACE_TUNE_UP                   31
#define N_MAT_STANDARD_EFFICIENCY_FURNACE       32
#define N_MAT_HIGH_EFFICIENCY_FURNACE           33
#define N_MAT_BOILER                            34
#define N_MAT_SPACE_HEATER_GAS_8K               35
#define N_MAT_SPACE_HEATER_GAS_55K              36
#define N_MAT_SPACE_HEATER_OIL_40K              37
#define N_MAT_SPACE_HEATER_OIL_75K              38
#define N_MAT_SPACE_HEATER_KER_10K              39
#define N_MAT_SPACE_HEATER_KER_40K              40
#define N_MAT_SMART_THERMOSTAT                  41
#define N_MAT_REPLACE_AC_WIN_5K                 42
#define N_MAT_REPLACE_AC_WIN_15K                43
#define N_MAT_REPLACE_AC_WIN_25K                44
#define N_MAT_REPLACE_AC_CENTRAL_2T_24K         45
#define N_MAT_REPLACE_AC_CENTRAL_3T_36K         46
#define N_MAT_REPLACE_AC_CENTRAL_4T_48K         47
#define N_MAT_TUNE_UP_AC                        48
#define N_MAT_EVAPORATIVE_COOLER                49
#define N_MAT_HEATPUMP_2T_24K                   50
#define N_MAT_HEATPUMP_3T_36K                   51
#define N_MAT_HEATPUMP_4T_48K                   52
#define N_MAT_WINDOW_SHADING_AWNING             53
#define N_MAT_SUN_SCREEN_FABRIC                 54
#define N_MAT_SUN_SCREEN_LOUVERED               55
#define N_MAT_WINDOW_FILM                       56
#define N_MAT_WINDOW_SEALING                    57
#define N_MAT_STORM_WINDOWS                     58
#define N_MAT_WINDOW_REPLACEMENT                59
#define N_MAT_LOW_E_WINDOWS                     60
// #define N_MAT_LIGHTING_RETROFITS_5W             61
// #define N_MAT_LIGHTING_RETROFITS_7W             62
// #define N_MAT_LIGHTING_RETROFITS_9W             63
// #define N_MAT_LIGHTING_RETROFITS_13W            64
// #define N_MAT_LIGHTING_RETROFITS_18W            65
// #define N_MAT_LIGHTING_RETROFITS_25W            66
// #define N_MAT_LIGHTING_RETROFITS_26W            67
// #define N_MAT_LIGHTING_RETROFITS_38W            68
// #define N_MAT_LIGHTING_RETROFITS_11FW           69
// #define N_MAT_LIGHTING_RETROFITS_15FW           70
// #define N_MAT_LIGHTING_RETROFITS_18FW           71
#define N_MAT_WATER_HEATER_TANK_INSULATION      72
#define N_MAT_WATER_HEATER_PIPE_INSULATION      73
#define N_MAT_LOW_FLOW_SHOWERHEADS              74
#define N_MAT_HIGH_EFFICIENCY_BOILER            75
#define N_MAT_ATTIC_INSULATION_R49_CELL         76
#define N_MAT_ATTIC_INSULATION_R49_FIBER        77
#define N_MAT_ATTIC_INSULATION_R49_UT3          78
#define N_MAT_ATTIC_INSULATION_R49_UT4          79
#define N_MAT_ATTIC_INSULATION_R11_UT5          80
#define N_MAT_ATTIC_INSULATION_R19_UT5          81
#define N_MAT_ATTIC_INSULATION_R30_UT5          82
#define N_MAT_ATTIC_INSULATION_R38_UT5          83
#define N_MAT_ATTIC_INSULATION_R49_UT5          84
#define N_MAT_ATTIC_INSULATION_R11_UT6          85
#define N_MAT_ATTIC_INSULATION_R19_UT6          86
#define N_MAT_ATTIC_INSULATION_R30_UT6          87
#define N_MAT_ATTIC_INSULATION_R38_UT6          88
#define N_MAT_ATTIC_INSULATION_R49_UT6          89
#define N_MAT_WALL_INSULATION_UT4               90
#define N_MAT_WALL_INSULATION_UT5               91
#define N_MAT_WALL_INSULATION_UT6               92
#define N_MAT_KNEEWALL_INSULATION_UT2           93
#define N_MAT_KNEEWALL_INSULATION_UT3           94
#define N_MAT_KNEEWALL_INSULATION_UT4           95
#define N_MAT_KNEEWALL_INSULATION_UT5           96
#define N_MAT_KNEEWALL_INSULATION_UT6           97
#define N_MAT_SILLBOX_INSULATION_UT2            98
#define N_MAT_SILLBOX_INSULATION_UT3            99
#define N_MAT_SILLBOX_INSULATION_UT4            100
#define N_MAT_SILLBOX_INSULATION_UT5            101
#define N_MAT_SILLBOX_INSULATION_UT6            102
#define N_MAT_FLOOR_INSULATION_R38              103
// These are replaced with two new recoreds for blowen cell and fiberglass for fill floor cavity #232
// #define N_MAT_FLOOR_INSULATION_R11_UT2          104
// #define N_MAT_FLOOR_INSULATION_R19_UT2          105
// #define N_MAT_FLOOR_INSULATION_R30_UT2          106
// #define N_MAT_FLOOR_INSULATION_R38_UT2          107
// #define N_MAT_FLOOR_INSULATION_R11_UT3          108
// #define N_MAT_FLOOR_INSULATION_R19_UT3          109
// #define N_MAT_FLOOR_INSULATION_R30_UT3          110
// #define N_MAT_FLOOR_INSULATION_R38_UT3          111
#define N_MAT_FLOOR_INSULATION_R11_UT4          112
#define N_MAT_FLOOR_INSULATION_R19_UT4          113
#define N_MAT_FLOOR_INSULATION_R30_UT4          114
#define N_MAT_FLOOR_INSULATION_R38_UT4          115
#define N_MAT_FLOOR_INSULATION_R11_UT5          116
#define N_MAT_FLOOR_INSULATION_R19_UT5          117
#define N_MAT_FLOOR_INSULATION_R30_UT5          118
#define N_MAT_FLOOR_INSULATION_R38_UT5          119
#define N_MAT_FLOOR_INSULATION_R11_UT6          120
#define N_MAT_FLOOR_INSULATION_R19_UT6          121
#define N_MAT_FLOOR_INSULATION_R30_UT6          122
#define N_MAT_FLOOR_INSULATION_R38_UT6          123
#define N_MAT_FOUNDATION_WALL_INSULATION_UT2    124
#define N_MAT_FOUNDATION_WALL_INSULATION_UT3    125
#define N_MAT_FOUNDATION_WALL_INSULATION_UT4    126
#define N_MAT_FOUNDATION_WALL_INSULATION_UT5    127
#define N_MAT_FOUNDATION_WALL_INSULATION_UT6    128
#define N_MAT_DOOR_REPLACEMENT                  129
#define N_MAT_WHITE_ROOF_COATING                130
// Issue #232
#define N_MAT_FLOOR_FILL_CELLULOSE              131
#define N_MAT_FLOOR_FILL_FIBERGLASS             132
#define N_MAT_FLOOR_FILL_UT4                    133
#define N_MAT_FLOOR_FILL_UT5                    134
#define N_MAT_FLOOR_FILL_UT6                    135

#define NEAT_MAX_RMC N_MAT_FLOOR_FILL_UT6 + 1  // number of retrofit material costs from JSON input(must have them all)
#define NEAT_MIN_RMC NEAT_MAX_RMC

#define N_MAT_FROM_AUDIT 500

// #243 energy tracking array indexes
#define PRE_RETROFIT_POST_INFIL 0
#define LAST_GOOD_MEASURE 1
#define CURRENT_MEASURE 2
#define PRE_RETROFIT_PRE_INFIL 3
#define PRE_RETROFIT_POST_DUCT_SEAL 4

// #243 an_load array index definitions
#define ANNUAL_THERMAL_LOAD_MMBTU 0
#define ANNUAL_SYSTEM_ENERGY_MMBTU 1
#define DESIGN_DAY_HEAT_LOSS_GAIN_PEAK_BTUH 2
#define DESIGN_DAY_OUTPUT_REQUIRED_BTUH 3

#define PRIMARY 0       // heating system

#define NEAT_MAX_ANLOAD 4 // Number of NEAT_LOADS items in result

#define OK 0            // generic OK and NOT_OK return values
#define NOT_OK -1
#define NOT_APPLICABLE -1

#define N_MAT_INFILTRATION_REDUCTION N_MAT_FLOOR_FILL_FIBERGLASS + 6
#define N_MAT_DUCT_SEALING N_MAT_INFILTRATION_REDUCTION + 1

#define NMAT N_MAT_DUCT_SEALING + 1         // maximum number of materials both measure_cost inputs and a few others
#define NMAT_OTHER MAX_LTG + 10             // extra 'other' materials at the end of the rmc[] array
#define MAX_INERACTIONS_PER_MEASURE 11      // Max number of interactions for any single measure

#define N_MAX_RMC NMAT + NMAT_OTHER     // how big to dimension array

#endif
