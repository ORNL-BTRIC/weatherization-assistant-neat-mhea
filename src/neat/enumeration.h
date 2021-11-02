/***************************************************************************
* MODULE:   enumeration.h            CREATED:    April 28, 1997, Modified 4/98
*
* AUTHORS:  Mark Fishbaugher, Mike Gettings, Gina Accawi
*
* MDESC:    defines all of the enumerated types used in NEAT dwelling data files
****************************************************************************/
#ifndef _N_ENUMERATION_H
#define _N_ENUMERATION_H

#ifdef __cplusplus
extern "C" {
#endif

// note that all enumerated types are base one following the convention
// established by the parse.c code to allow for blank type entries (ie.
// an enumeration with value=0 is 'unentered' or blank data)

// note also that I have used the convention of NN_ prefix to those
// enumerated types that are too simple -- and may thus be repeated somewhere
// the 'NN' is taken from the enumation name
// ANSI C requires that all enumerations have unique names

enum CONSERVATION_MEASURE_TYPE  // how we categorize the conservation measures
{ CMT_HEATING_ENVELOPE = 0,
  CMT_COOLING_ENVELOPE = 2,
  CMT_HEATING_SYSTEM_UPDATE = 3,
  CMT_SMART_THERMOSTAT = 4,
  CMT_COOLING_SYSTEM = 5,
  CMT_INFILTRATION_REDUCTION = 6,
  CMT_DUCT = 7,
  CMT_HEAT_PUMP_REPLACE = 8,
  CMT_BASELOAD = 9,
  CMT_WATER_HEATER = 10,
  CMT_HEATING_SYSTEM_REPLACE = 11,
  CMT_WINDOWS_DOORS = 12,
  CMT_OTHER = 13
};

enum MEASURE_COMPONENT_GROUP_TYPE // The categories of component grouping into measures
{ MG_NONE = 0,
  MG_WALL,
  
  MG_FOUNDATION_FLOOR,
  MG_FOUNDATION_SILL,
  MG_FOUNDATION_WALL,

  MG_ATTIC_RESTRICTED,            // IMPORTANT that all MG_ATTIC_* values are contiguous since they are used in a for loop
  MG_ATTIC_UNRESTRICTED,
  MG_ATTIC_SPECIFIED_REQUIRED,

  MG_ATTIC_KNEEWALL,

  MG_ATTIC_COLLAR_BEAM_RESTRICTED,
  MG_ATTIC_COLLAR_BEAM_UNRESTRICTED,
  MG_ATTIC_COLLAR_BEAM_SPECIFIED_REQUIRED,

  MG_ATTIC_ROOF_RAFTER_RESTRICTED,
  MG_ATTIC_ROOF_RAFTER_UNRESTRICTED,
  MG_ATTIC_ROOF_RAFTER_SPECIFIED_REQUIRED,

  MG_ATTIC_OUTER_CEILING_JOIST_RESTRICTED,
  MG_ATTIC_OUTER_CEILING_JOIST_UNRESTRICTED,
  MG_ATTIC_OUTER_CEILING_JOIST_SPECIFIED_REQUIRED };

enum STUD_SIZE // WALL STUD SIZE CHECK BOX FIELDS
{ SS_TWOXTWO = 1,
  SS_TWOXTHREE,
  SS_TWOXFOUR,
  SS_TWOXSIX,
  SS_TWOXEIGHT };

// #314 
// enum WALLEXPOSURE // CONDITIONS SEEN BY EXTERIOR SURFACE OF WALL
// { EX_EXPOSED = 1,
//   EX_BUFFERED,
//   EX_ATTIC };

enum WALLEXPOSURE // CONDITIONS SEEN BY EXTERIOR SURFACE OF WALL
{ EX_EXPOSED = 1,
  EX_BUFFERED };

enum WALLEXTRTYPE // EXTERIOR WALL SURFACE MATERIAL
{ EX_WOOD = 1,
  EX_METAL,
  EX_STUCCO,
  EX_BRICK,
  EX_NONE,
  EX_OTHER };

enum WALLLOADTYPE // TYPE OF LOAD BEARING WALL
{ LO_BALLOON = 1,
  LO_PLATFORM,
  LO_MASONRY,
  LO_CONCRETE,
  LO_ADOBE,
  LO_OTHER };

enum WALLEXTINSTYPE // EXISTING WALL INSULATION TYPE
{ EW_NONE = 1,
  EW_CELLULOSE_BLOWN,
  EW_FIBERGLASS_BLOWN,
  EW_ROCKWOOL,
  EW_FIBERGLASS_BATT,
  EW_POLYSTYRENE_BOARD,
  EW_OTHER };

enum WINDOW_TYPE // WINDOW TYPE CHECK BOX FIELDS
{ JALOUSIE = 1,
  AWNING,
  SLIDER,
  FIXEDWIN,
  DOORWIN,
  GLASSDOOR,
  SKYLIGHT };

enum FRAME_TYPE // MATERIAL OF WINDOW FRAMES
{ FT_WOOD_VINYL = 1,
  FT_METAL,
  FT_IMPROVED_METAL };

enum GLAZINGTYPE // PANES OF WINDOW
{ SINGLE = 1,
  SINGLE_W_WOOD_STORM,
  SINGLE_W_METAL_STORM,
  DOUBLE_GLAZED,
  SINGLE_W_BAD_STORM,
  DOUBLE_GLAZED_LOWE,
  DOUBLE_GLAZED_W_STORM };  // unused ( 7 not web_ui interface)

enum INTERIOR_SHADING // INTERIOR SHADING CHECK BOX FIELDS
{ IS_DRAPES = 1,
  IS_BLINDS,
  IS_DRAPESSHADES,
  IS_NONE,
};

enum EXTERIOR_SHADING // WINDOW EXTERIOR SHADING CHECK BOX FIELDS (not used 8.4 but available with same choices at MHEA)
{ WES_LOWEFILM = 1,
  WES_SUNSCREEN,
  WES_AWNING,
  WES_CARPORT,
  WES_NONE };

enum WINSTATUS // RETROFIT STATUS OF A WINDOW
{ WS_EVALUATE_ALL = 1,
  WS_SEAL,
  WS_REPLACE,
  WS_ADD_STORM,
  WS_EVALUATE_NONE,
  WS_REPLACE_LOWE };

enum DOOR_TYPE // TYPE OF DOOR
{ WOOD_HOLLOW_CORE = 1,
  WOOD_SOLID_CORE,
  STEEL_INSULATED,
  SINGLE_SLIDING_GLASS,
  DOUBLE_SLIDING_GLASS };

enum STORM_DOOR_INFO // DATA ON EXISTENCE/CONDITION OF STORM DOOR
{ DR_ADEQUATE = 1,
  DR_DETERIORATED,
  DR_NONE };

enum ATTICEXTINSTYPE // EXISTING ATTIC INSULATION TYPE
{ AE_NONE = 1,
  AE_CELLULOSE_BLOWN,
  AE_FIBERGLASS_BLOWN,
  AE_ROCKWOOL_BLOWN,
  AE_FIBERGLASS_BATT,
  AE_OTHER };

enum UNFINISHED_ATTIC_TYPE // TYPE OF UNFINISHED ATTIC FLOOR
{ UA_UNFLOORED = 1,
  UA_FLOORED,
  UA_CATHEDRAL };

enum FINISHED_ATTIC_TYPE // TYPE OF FINISHED ATTIC COMPONENT
{ FA_OUTER_CEILING_JOIST = 1,
  FA_COLLAR_BEAM,
  FA_KNEEWALL,
  FA_ROOF_RAFTER };

enum FINISHED_ATTIC_FLOOR_TYPE // TYPE OF FINISHED ATTIC FLOOR
{ FAF_UNFLOORED = 1,
  FAF_FLOORED };

enum UAS_ATTIC_TYPE // TYPE OF AGGREGATED ATTIC SPACES (ndi->uas[].attic_type)
{ UAS_UNFLOORED = 1,
  UAS_FLOORED,
  UAS_CATHEDRAL,
  UAS_KNEEWALL,
  UAS_COLLAR_BEAM,
  UAS_ROOF_RAFTER,
  UAS_CEILING_JOIST };

enum FOUNDATIONTYPE // TYPE OF FOUNDATION FOR HOUSE
{ CONDITIONED = 1,
  NON_CONDITIONED,
  VENTED_NON_CONTITIONED,
  UNINTENTIONALLY_CONDITIONED,
  UNINSULATED_SLAB,
  INSULATED_SLAB,
  EXPOSED_FLOOR_UNCLOSED,
  EXPOSED_FLOOR_CLOSED };

enum CLGEQUIPTYPE // COOLING EQUIPMENT TYPE
{ CET_CENTRAL = 1,
  CET_WINDOW,
  CET_HEATPUMP,
  CET_EVAPORATIVE,
  CET_NONE };

enum HTGEQUIPTYPE // HEATING EQUIPMENT TYPE
{ HE_OTHER = 1,
  HE_GRAVITY_FURNACE,
  HE_FORCED_AIR_FURNACE,
  HE_STEAM_BOILER,
  HE_HOT_WATER_BOILER,
  HE_FIXED_ELECTRIC_RESISTANCE,
  HE_PORT_ELECTRIC_RESISTANCE,
  HE_HEAT_PUMP,
  HE_UNVENTED_SPACE_HEATER,
  HE_VENTED_SPACE_HEATER };

enum HTGUNITS // UNITS FOR HEATING INPUT RATING
{ NO_INPUT = 1,
  KBTU_PER_HR,
  GAL_PER_HR,
  LB_PER_HR,
  CCM,
  HEATING_KW };

enum EQUIPCONDITION // HEATING SYSTEM CONDITIONS
{ POOR = 1,
  FAIR,
  GOOD };

// Temporary addition of old 89 enumeration (below) until new
//   new interace is employed

enum EQUIPRETSTATUS // HEATING SYSTEM RETROFIT STATUS
{ ES_EVAL_ALL = 1,
  ES_TUNEUP_PERFORMED_EVAL_REP,
  ES_TUNEUP_REQUIRED,
  ES_STDEFF_REP_REQUIRED,
  ES_HIEFF_REP_REQUIRED,
  ES_NO_REP_EVAL_TUNEUP,
  ES_EVALUATE_HEATPUMP,
  ES_HEATPUMP_REQUIRED,
  ES_ELECRES_REP_REQUIRED,
  ES_EVALUATE_NONE };

// all six of our NEAT insulation type enumerations moved here  MJF 10/06

enum ATTICADDINSTYPE // ADDED ATTIC INSULATION TYPE
{ AA_NONE = 1,
  AA_CELLULOSE_BLOWN,
  AA_FIBERGLASS_BLOWN,
  AA_TYPE3,
  AA_TYPE4,
  AA_TYPE5,
  AA_TYPE6 };

enum KNEEWALLADDINSTYPE // ADDED KNEEWALL INSULATION TYPE
{ AK_NONE = 1,
  AK_FIBERGLASS_BATT,
  AK_TYPE2,
  AK_TYPE3,
  AK_TYPE4,
  AK_TYPE5,
  AK_TYPE6 };

enum WALLADDINSTYPE // ADDED WALL INSULATION TYPE
{ AW_NONE = 1,
  AW_CELLULOSE_BLOWN,
  AW_TYPE2,
  AW_TYPE3,
  AW_TYPE4,
  AW_TYPE5,
  AW_TYPE6 };

enum FLOORADDINSTYPE // ADDED FLOOR INSULATION TYPE
{ AL_NONE = 1,
  AL_FIBERGLASS_BATT,
  AL_CELLULOSE_BLOWN,
  AL_FIBERGLASS_BLOWN,
  AL_TYPE4,
  AL_TYPE5,
  AL_TYPE6 };

enum SILLADDINSTYPE // ADDED SILL INSULATION TYPE
{ AS_NONE = 1,
  AS_FIBERGLASS_BATT,
  AS_TYPE2,
  AS_TYPE3,
  AS_TYPE4,
  AS_TYPE5,
  AS_TYPE6 };

enum FOUNDATIONADDINSTYPE // ADDED FOUNDATION WALL INSULATION TYPE
{ AF_NONE = 1,
  AF_RIGID_BOARD,
  AF_TYPE2,
  AF_TYPE3,
  AF_TYPE4,
  AF_TYPE5,
  AF_TYPE6 };

enum CLGEFFUNIT { COP = 1, EER, SEER };

enum HTGEFFUNIT { SSTATEMEAS = 1, SSTATERATED, AFUE, HSPF };

enum VENTCONFIGURATION { ATMOSPHERIC = 1, INDUCED_DRAFT, SEALED_COMBUSTION };

enum LOAD_CALCULATION_TYPE { ASHRAE = 1, MANUAL_J_HEAT, MANUAL_J_COOL };

#ifdef __cplusplus
}
#endif

#endif /* _ENUMN_H */