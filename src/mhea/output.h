/***************************************************************************
* MODULE:	result_m.h            CREATED:	1/26/00
*                               MODIFIED:
*
* AUTHOR:	Mark Fishbaugher
*
* MDESC:	Analysis result data structures to be passed back to
*           program calling the analysis engine run_mhea()
****************************************************************************/
#ifndef _RESULTM_H
#define _RESULTM_H

// the first structure here is a combined measure structure that includes
// description, energy savings, economic savings, and material info.

typedef struct {
  int index;            // primary key of measure[] output
  int measure_id;       // input JSON measure_flag[] array index 
  int audit_section_id; // what section of the audit is this related to
  int component_id;     // field passed into the itemized cost, otherwise 0 #318
  char measure[MEASURENAME_LEN + 1];      // name of measure
  char components[STRING_LEN]; // list of component codes effected
  float heating_mmbtu;   // annual MMBtu heating savings
  float heating_sav;    // annual dollar savings for heating
  float cooling_kwh;    // annual kwh cooling savings
  float cooling_sav;    // annual dollar savings for cooling
  float baseload_kwh;   // annual kwh baseload savings
  float baseload_sav;   // annual dollar savings for baseloads
  float total_mmbtu;     // total annual heating and cooling MMBtu
  float savings;        // total annual heating and cooling dollar savings
  float cost;           // total initial costs
  float sir;            // computed savings to investment ratio
  int lifetime;         // lifetime (years) of the SIR calculation for the measure (MJF 2/2019 Issue 25)
  float qtym;           // The quantity of the material itself in units (new 7/04)
  float qtyl;           // The quantity for labor cost calculation (new 7/04)
  float qtyi;           // Quantity of any associated Item Cost (new 7/04)
  float costum;         // material unit cost
  float costul;         // labor unit cost
  float costi1;         // other itemized cost
  float costi2;         // another cost adder
  char desci2[MEASURENAME_LEN + 1];      // description of qty=1 costi2 if <> 0
  int typei2;           // the material type/category if costi2<>0
  float costi3;         // another cost adder
  char desci3[MEASURENAME_LEN + 1];      // description of qty=1 costi3 if <> 0
  int typei3;           // the material type/category if costi2<>0
} MHEA_MEASURE;

// these structures are mainly copied from the same results header
// for NEAT, since most of the outputs we need to feed back to the calling
// database are the same

typedef struct {
  int index;            // number to order the list
  int measure_index;    // FK into measures[]
  char measure[MEASURENAME_LEN + 1];     // name of measure
  char components[STRING_LEN]; // list of component codes effected
  float heating_mmbtu;   // annual MMBtu heating savings
  float heating_sav;    // annual dollar savings for heating
  float cooling_kwh;    // annual kwh cooling savings
  float cooling_sav;    // annual dollar savings for cooling
  float baseload_kwh;   // annual kwh baseload savings
  float baseload_sav;   // annual dollar savings for baseloads
  float total_mmbtu;     // total annual heating and cooling MMBtu
} MHEA_ANNUAL_SAVE;

typedef struct {
  int index;            // keeps the order straight
  int measure_index;    // FK into measures[]
  enum MEASURE_PACKAGE_GROUPS group;     // measure grouping enum
  char measure[MEASURENAME_LEN + 1];     // name of measure
  char components[STRING_LEN]; // list of component codes effected
  float savings;        // total annual heating and cooling dollar savings
  float cost;           // total initial costs
  float sir;            // computed savings to investment ratio
  float csav;           // cummulative annual $ savings
  float ccost;          // cummulative cost
  float csir;           // cummulative sir
} MHEA_ECONOMICS;

typedef struct {
  int index;            // to keep order of appearance
  int measure_index;    // FK into measures[]
  int material_id;      // the fixed id from the measure_cost JSON input
  char material[MEASURENAME_LEN + 1]; // name of material
  char type[MEASURENAME_LEN + 1];     // type of material
  float quantity;    // how much of it
  char units[20];    // description of units
} MHEA_MATERIALS;

// typedef struct {
//   int index;            // order of appearance
//   char msg[STRING_LEN]; // Signal string for message to display
// } MHEA_MESSAGES;

// might be useful if we ever add comments to components of
// a MHEA audit.  Added to structs 8/02 MJF

// typedef struct {
//   int index;               // order of appearance
//   char code[CODE_LEN + 1]; // code of the component with a comment
//   char type[25];           // the component type
//   char msg[COMM_LEN + 1];  // user comment string
// } MHEA_COMMENT;

typedef struct {
  int index;        // order of appearance
  int year;         // reading year (yyyy)
  int month;        // reading month
  int day;          // reading day
  int period_days;  // number of days in period
  int consump_act;  // actual consumption
  int consump_pred; // predicted consumption
  int dd_act;       // actual degree days
  int dd_pred;      // predicted degree days
} MHEA_COMPARE;     // energy use/billing and degree day comparison

typedef struct {
  int index;                          // order of appearance
  char heatcool[CODE_LEN + 1];        // is it 'heat' or 'cool' sizing results
  char type[TYPE_LEN + 1];            // component type (wall, door etc..) and units
  char name[CODE_LEN + 1];            // name of building component, not used in MHEA #342
  float area_vol;                     // surface area or volume of component, not used in MHEA #342
  float pre_load;                     // pre-retrofit peak load (BTU/hr)
  float post_load;                    // post_retrofit peak load (BTU/hr)
} MHEA_MANJ;               // manual J output

typedef struct {

  int energy_calc_counter;  // how many times did the bin method energy calculation/simulation get called

  int num_measure;               // number of overall result measures (all reported measures)
  MHEA_MEASURE measure[MAXECMS]; // array of results values

  int num_an_sav;                             // number of recommended measures in annual savings table
  MHEA_ANNUAL_SAVE an_sav[MAXECMS]; // array of savings values

  int num_an_asav;                             // number of adjusted recommended measures in annual savings table
  MHEA_ANNUAL_SAVE an_asav[MAXECMS]; // array of adjusted savings values

  int num_sir;                           // number of recommended measures in economics table
  MHEA_ECONOMICS sir[MAXECMS]; // unadjusted economics for recommendations

  int num_asir;                           // number of recommended measures in adjusted economics table
  MHEA_ECONOMICS asir[MAXECMS]; // adjusted economics for recommendations

  int num_material;                           // number of entries in materials table
  MHEA_MATERIALS material[MAXECMS]; // out list of materials

  int num_amaterial;                           // number of entries in adjusted materials table
  MHEA_MATERIALS amaterial[MAXECMS]; // out list of adjusted materials

  int num_message;                        // number of extra message strings
  char message[MAXMESSAGE][MESSAGE_LEN];  // special notices and message strings from the analysis. All static strings so just store pointers

  //int num_message;           // number of extra message strings
  //MHEA_MESSAGES message[30]; // special notices
  // int num_comment;                                  // number of component comments
  // MHEA_COMMENT comment[75];                         // 75 is just a guess at a size

  // annual loads section all in MMBtu

  float pre_heat;  // pre retrofit heating
  float pre_cool;  // pre retrofit cooling
  float pre_base;  // pre retrofit base loads
  float post_heat; // post retrofit heating
  float post_cool; // post retorfit cooling
  float post_base; // post retrofit base loads

  // heating and cooling utility bill comparison (.dat) output

  char heat_comp_units[20];           // units for energy comparison
  int heat_dd_base;                   // base temperature for degree days
  int num_heat_comp;                  // number of heating bills in heat_comp[]
  MHEA_COMPARE heat_comp[MONTHS + 1]; // heating bill comparision
  char cool_comp_units[20];           // units for energy comparison
  int cool_dd_base;                   // base temperature for degree days
  int num_cool_comp;                  // number of cooling bills in cool_comp[]
  MHEA_COMPARE cool_comp[13];         // cooling bill comparision

  // manual J peak load calcs

  int num_manj;       // number of manual J compoent entries
  MHEA_MANJ manj[10]; // manual J output

  int num_used_fuel;                  // how many fuel types were used
  USED_FUEL used_fuel[FUEL_TYPES];    // list of used fuel types and pricing used

} MOR;    // Mhea Output Results

#endif
