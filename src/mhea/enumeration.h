/***************************************************************************
* MODULE:   enummhea.h            CREATED:    April 28, 1997
*
* AUTHOR:   Mark Fishbaugher
*
* MDESC:    defines all of the enumerated types used in the application.
*           this is a shared file showing the enumerated types for all file
*           types used.  note, this header is shared amoung all of the other
*           headers that have typedefs involving enumerated types.  because
*           it is shared, we also include the defines for various string
*           lengths that are also to be shared amoung header files
****************************************************************************/
#ifndef _ENUMM_H
#define _ENUMM_H

// note that all enumerated types are base one following the convention
// established by the parse.c code to allow for blank type entries

// note also that I have used the convention of NN_ prefix to those
// enumerated types that are too simple -- and may thus be repeated somewhere
// the 'NN' is taken from the enumation name
// ANSI C required that all enumerations have unique names

enum ENGINE_PASS // Which mode is the engine currently running
{ BASE_CASE = 1,
  FIRST_PASS,
  CUMULATIVE,
  NOT_BASE_CASE };

enum WSHIELD // SHIELDING FROM THE WIND
{ WS_WELL = 1,
  WS_NORMAL,
  WS_EXPOSED };

enum JOIST_SIZE // FLOOR JOIST SIZES
{ JS_TWOXFOUR = 1,
  JS_TWOXSIX,
  JS_TWOXEIGHT };

enum INSUL_LOC // INSULATION LOCATIONs (wing, belly, addition)
{ ATTACHEDFLOOR = 1,
  BETWEENJOISTS,
  ATTACHEDUNDER,
  DRAPEDBELOW,
  LOC_NONE };

enum BELLY_COND // CONDITION OF BELLY CHECK BOX FIELDS
{ BC_GOOD = 1,
  BC_AVERAGE,
  BC_POOR };

enum BELLY_CAV // BELLY CAVITY CONFIGURATION
{ BC_SQUARE = 1,
  BC_ROUNDED,
  BC_FLAT };

enum ADD_FLOOR_TYPE // ADDITION FLOOR TYPE
{ CRAWL = 1,
  SLABONGRADE,
  EXPOSEDFLOOR };

enum STUD_SIZEMHEA // WALL STUD SIZE CHECK BOX FIELDS
{ SS_TWOXTWOMHEA = 1,
  SS_TWOXTHREEMHEA,
  SS_TWOXFOURMHEA,
  SS_TWOXSIXMHEA };

enum WALL_VENT // WALL INTENTIONALLY VENTILATED
{ WV_VENT = 1,
  WV_NOTVENT };

enum WINDOW_TYPEMHEA // WINDOW TYPE CHECK BOX FIELDS
{ JALOUSIEMHEA = 1,
  AWNINGMHEA,
  SLIDERMHEA,
  FIXEDWINMHEA,
  DOORWINMHEA,
  GLASSDOORMHEA,
  SKYLIGHTMHEA };

enum FRAMETYPEMHEA // MATERIAL OF WINDOW FRAMES
{ FT_WOOD_VINYLMHEA = 1,
  FT_METALMHEA,
  FT_IMPROVED_METALMHEA };

enum GLAZING_TYPEMHEA // GLAZING TYPE CHECK BOX FIELDS
{ GT_SINGLE = 1,
  GT_DBL,
  GT_SINGLEGLASS,
  GT_DOUBLEGLASS,
  GT_SINGLEPLASTIC,
  GT_DOUBLEPLASTIC };

enum INTERIOR_SHADINGMHEA // INTERIOR SHADING CHECK BOX FIELDS
{ IS_DRAPESMHEA = 1,
  IS_BLINDSMHEA,
  IS_DRAPESSHADESMHEA,
  IS_NONEMHEA,
};

enum EXTERIOR_SHADINGMHEA // EXTERIOR SHADING CHECK BOX FIELDS
{ ES_LOWEFILM = 1,
  ES_SUNSCREEN,
  ES_AWNINGMHEA,
  ES_CARPORT,
  ES_NONEMHEA };

enum MWINSTATUS // RETROFIT STATUS OF A WINDOW for MHEA
{ MWS_EVALUATE_ALL = 1,
  MWS_SEAL,
  MWS_REPLACE,
  MWS_ADD_GLASS_STORM,
  MWS_ADD_PLASTIC_STORM,
  MWS_EVALUATE_NONE };

enum M_DOOR_TYPE // DOOR TYPE CHECK BOX FIELDS
{ SOLIDWOOD = 1,
  HOLLOWWOOD,
  METAL,
  INSUL_STEEL,
  REPLACEMENT };

enum ROOF_TYPE // ROOF TYPE CHECK BOX FIELDS
{ RT_FLAT = 1,
  RT_BOWSTRING,
  RT_PITCHED };

enum EQUIP_TYPE // HEATING EQUIPMENT TYPE, new MJF 2/23/00
{               // previous enumeration mixed type and fuel
  ET_FURNACE = 1,
  ET_HEATPUMP,
  ET_SPACEHEAT,
  ET_NONE
};

enum DUCT_LOCATION // HEATING or COOLING DUCT LOCATION
{ DL_FLOOR = 1,
  DL_CEILING,
  DL_NONE };

enum DUCT_INSULATION // HEATING or COOLING DUCT INSULATION LOCATION
{ DI_ABOVE = 1,
  DI_BELOW,
  DI_AROUND,
  DI_NONE };

enum CLG_EQUIP_TYPE // COOLING EQUIPMENT TYPE
{ CE_EVAPORATIVE = 1,
  CE_CENTRALAC,
  CE_ROOMAC,
  CE_HEATPUMP,
  CE_NONE };

enum CLG_EFF_UNITS // COOLING EQUIPMENT EFFICIENCY UNITS, new 1/30/03
{ CE_COP = 1,
  CE_EER,
  CE_SEER };

enum HTG_EFF_UNITS // HEATING EQUIPMENT EFFICIENCY UNITS, new 7/23/03
{ HE_SSTATE = 1,
  HE_AFUE,
  HE_COP,
  HE_HSPF };

enum WALL_CONFIG // WALL CONFIGURATION CHECK BOX FIELDS
{ WC_INTERIOR = 1,
  WC_CENTER,
  WC_FLAT };

#endif
