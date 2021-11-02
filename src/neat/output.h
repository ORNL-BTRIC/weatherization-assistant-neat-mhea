/***************************************************************************
 * MODULE:	output.h
 *
 * AUTHOR:	Mark Fishbaugher
 *
 * MDESC:	Analysis result data structures to be passed back to
 *           program calling the analysis engine run_neat()
 ****************************************************************************/
#ifndef _N_OUTPUT_H
#define _N_OUTPUT_H

// the first structure here is a combined measure structure that includes
// description, energy savings, economic savings, and material info.

typedef struct {
  int index;            // PK called measure_index below
  int measure_id;       // fixed measure number  (CMS)
  int audit_section_id; // what section of the audit is this related to
  int component_id;     // the field passed into the itemized cost, otherwise 0 #318
  char measure[MEASURENAME_LEN +1];       // name of measure
  char comp_group[LRG_STR + 1];           // name of component grouping based on measure_number from user
  char components[MAX_COMPONENTS_LEN + 1];  // list of component codes effected
  int required;         // is the measure required
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
  char desci2[SHORT_NAME_LEN + 1];      // description of qty=1 costi2 if <> 0
  int typei2;           // the material type/category if costi2<>0
  float costi3;         // another cost adder
  char desci3[SHORT_NAME_LEN + 1];      // description of qty=1 costi3 if <> 0
  int typei3;           // the material type/category if costi2<>0
} NEAT_MEASURE;

// added 8.3.2 MJD 7/7 to support more user defined insulation types see tblNResultMeasureMaterial
typedef struct {
  int index;              // number of the measure for this run
  int measure_index;      // FK into measures
  int material_id;        // the fixed material reference number
  enum MATERIAL_TYPE material_type_id;   // fixed cost detail record type
  char comp_group[LRG_STR + 1];          // name of component grouping based on measure_number from user
  char components[MAX_COMPONENTS_LEN + 1];  // list of components for this material detail record or blank = all for measure
  char description[STRING_LEN + 1];         // description of the material
  char units[RETRO_UNIT_LEN + 1];           // description of units
  float qty_est;         // quantity of the material itself, estimated
  float unit_cost_est;   // unit cost, estimated
  char comment[SHORT_NAME_LEN + 1];      // additional words of wisdom
} NEAT_MEASURE_MATERIAL;

typedef struct {
  int index;              // number to order the list
  int measure_index;      // FK into measures
  char measure[MEASURENAME_LEN + 1];    // name of measure
  char comp_group[LRG_STR + 1];         // name of component grouping based on measure_number from user
  char components[MAX_COMPONENTS_LEN + 1]; // list of component codes effected
  int required;         // is the measure required
  float heating_mmbtu;   // annual MMBtu heating savings
  float heating_sav;    // annual dollar savings for heating
  float cooling_kwh;    // annual kwh cooling savings
  float cooling_sav;    // annual dollar savings for cooling
  float baseload_kwh;   // annual kwh baseload savings
  float baseload_sav;   // annual dollar savings for baseloads
  float total_mmbtu;     // total annual heating and cooling MMBtu
} NEAT_ANNUAL_SAVE;

typedef struct {
  int index;              // keeps the order straight
  int measure_index;      // FK into measures
  enum MEASURE_PACKAGE_GROUPS group;     // measure grouping enum
  char measure[MEASURENAME_LEN + 1];     // name of measure
  char comp_group[LRG_STR + 1];          // name of component grouping based on measure_number from user
  char components[MAX_COMPONENTS_LEN + 1]; // list of component codes effected
  int required;         // is the measure required
  float savings;        // total annual heating and cooling dollar savings
  float cost;           // total initial costs
  float sir;            // computed savings to investment ratio
  float ccost;          // cummulative cost
  float csir;           // cummulative sir
} NEAT_ECONOMICS;

typedef struct {
  int index;         // to keep order of appearance
  int measure_index;    // FK into measures
  int material_id;      // the fixed material reference number
  char material[MATERIAL_LEN + 1]; // name of material
  char type[MATERIAL_LEN + 1];     // type of material
  float quantity;    // how much of it
  char units[RETRO_UNIT_LEN + 1];    // description of units
  float qtymat;      // XXX The quantity of the material itself
  float qtyhrs;      // XXX The number of labor hours associated
  float qtyeach;     // XXX Quantity of any associated Item Cost
} NEAT_MATERIALS;

typedef struct {
  int index;       // order of appearance
  char name[SHORT_NAME_LEN + 1];   // name of the load and energy units
  float pre_heat;  // pre retrofit heating
  float pre_cool;  // pre retrofit cooling
  float post_heat; // post retrofit heating
  float post_cool; // post retorfit cooling
} NEAT_LOADS;      // annual loads

// typedef struct {
//   int index;     // order of appearance
//   char msg[COMM_LEN + 1]; // Signal string for message to display
// } NEAT_MESSAGES;

// typedef struct {
//   int index;               // order of appearance
//   char code[CODE_LEN + 1]; // code of the component with a comment
//   char type[25];           // the component type
//   char msg[COMM_LEN + 1];  // user comment string
// } NEAT_COMMENT;

typedef struct {
  int index;               // order of appearance
  char heatcool[CODE_LEN + 1];        // is it 'heat' or 'cool' sizing results
  char type[TYPE_LEN + 1]; // component type (wall, door etc..) and units
  char name[CODE_LEN + 1]; // name of building component
  float area_vol;          // surface area or volume of component
  float pre_load;          // pre-retrofit peak load (BTU/hr)
  float post_load;         // post_retrofit peak load (BTU/hr)
} NEAT_MANJ;

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
} NEAT_COMPARE;     // energy use/billing and degree day comparison

typedef struct {

  int energy_calc_counter;  // how many times did the bin method energy calculation/simulation get called
  int energy_delta_counter; // how many times did the delta ua calculation get called

  int num_measure;               // number of overall result measures (all reported measures)
  NEAT_MEASURE measure[MAXECMS]; // array of results values

  int num_measure_material;                     // number of overall result measure materials (all reported measures)
  NEAT_MEASURE_MATERIAL mmaterial[4 * MAXECMS]; // array of measure materials, figure 4*MAXMEAS is very conservative

  int num_an_sav;                   // number of recommended measures in annual savings table
  NEAT_ANNUAL_SAVE an_sav[MAXMEAS]; // array of savings values

  int num_an_asav;                   // number of adjusted recommended measures in annual savings table
  NEAT_ANNUAL_SAVE an_asav[MAXMEAS]; // array of adjusted savings values

  int num_sir;                 // number of recommended measures in economics table
  NEAT_ECONOMICS sir[MAXMEAS]; // unadjusted economics for recommendations

  int num_asir;                 // number of recommended measures in adjusted economics table
  NEAT_ECONOMICS asir[MAXMEAS]; // adjusted economics for recommendations

  int num_material;                 // number of entries in materials table
  NEAT_MATERIALS material[MAXMEAS]; // out list of materials

  int num_amaterial;                 // number of entries in adjusted materials table
  NEAT_MATERIALS amaterial[MAXMEAS]; // out list of adjusted materials

  NEAT_LOADS an_load[DESIGN_DAY_OUTPUT_REQUIRED_BTUH + 1]; // annual and design (manual J) day loads

  int num_message;                        // number of extra message strings
  char message[MAXMESSAGE][MESSAGE_LEN];  // special notices and message strings from the analysis. All static strings so just store pointers

  // int num_comment;          // number of component comments
  // NEAT_COMMENT comment[75]; // 75 is just a guess at a size

  int num_manj;        // number of manual J compoent entries
  NEAT_MANJ manj[MAXMANUALJ]; // manual J output (changed dimension from 40 to 80 MJF 4/4/03, 80 to 100 when cooling added MJF 3/05)

  // heating and cooling utility bill comparison (.dat) output

  char heat_comp_units[RETRO_UNIT_LEN + 1];           // units for energy comparison
  int heat_dd_base;                   // base temperature for degree days
  int num_heat_comp;                  // number of heating bills in heat_comp[]
  NEAT_COMPARE heat_comp[MONTHS + 1]; // heating bill comparision

  char cool_comp_units[RETRO_UNIT_LEN + 1];           // units for energy comparison
  int cool_dd_base;                   // base temperature for degree days
  int num_cool_comp;                  // number of cooling bills in cool_comp[]
  NEAT_COMPARE cool_comp[MONTHS + 1]; // cooling bill comparision

  int num_used_fuel;                  // how many fuel types were used
  USED_FUEL used_fuel[FUEL_TYPES];    // list of used fuel types and pricing used

} NOR;    // Neat Output Results

#endif /* _RESULTN_H */
