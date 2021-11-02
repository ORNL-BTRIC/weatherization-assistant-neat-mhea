/***************************************************************************
* MODULE:   intermediate.h            CREATED:    1/23/2020
*
* AUTHOR:   Mark Fishbaugher
*
* MDESC:    Structure to store all the MHEA intermediate results
****************************************************************************/
#ifndef _N_INTERMEDIATE_H
#define _N_INTERMEDIATE_H

struct measure {
  int index;             // base 0 counter from first pass
  int measure_index;     // base 1 primary key of output nor->measure[] array IF this measure is implemented, otherwise = 0
  int rmc_index;         // the base 0 index of the measure_cost material (RMC) used by this energy conservation  measure
  int component_id;      // the optional field passed into the itemized cost, otherwise 0 #318

  int cms_measure_num;    // Issue #84, need to attach the original measure number from the cms structure (active flags)
  int audit_section_id;   // what audit section is this measure realted to #164

  enum MEASURE_COMPONENT_GROUP_TYPE comp_group_type;    // component grouping type/category
  int  comp_group_num;                                  // number within that type (based on .measure_number input from user)
  char components[MAX_COMPONENTS_LEN + 1];              // list of component code strings effected by this measure
  
  int dwelling_component_index;             // if measure associated with SINGLE specific dwelling component in audit_section_id, then which index, base 0
  
  int lifetime;           // need to carry this forward for each measure (MJF 2/2019 Issue 25) lifetime in years
  float ensav; 
  float cost;
  float dlsav; 
  float sir;              // savings to investment ratio unitless
  float value;
  float value2;
  float qtym, qtyl, qtyi; // qty for material, labor, itemized added 7/04
  float costum;           // material unit cost
  float costul;           // labor unit cost
  float costi1;           // other itemized cost
  float costi2;           // another cost adder
  char desci2[80];        // description of the other cost adder
  int typei2;             // the material type or category for costi2
  float costi3;           // another cost adder
  char desci3[80];        // description of the other cost adder
  int typei3;             // the material type or category for costi3
}; // index of the assoicated measure implemented in the results output to db

struct measure_material {      // added 8.3.2 MJF 7/7 to support more user defined insulation types see tblNResultMeasureMaterial
  int ecm_index;               // base zero first pass ecm[] array index for matching to associated ecm[] entry
  int measure_index;           // primary key for the associated implemented nor->measure[] array
  int rmc_index;               // fixed material from RMC
  char components[MAX_COMPONENTS_LEN + 1];  // list of audit component CODES effected changed comp from 80 to 255 characters to match db
  //int matn;                    // optional material number or index into the retrofit_materials[] array (for following desc and type fields)
  char desc[MATERIAL_LEN + 2]; // description of the material, lots of string space
  char type[TYPE_LEN + 1];     // option material type modifier
  enum MATERIAL_TYPE material_type_id;   // fixed material reference number
  float depth;                 // set to studdepth for walls only, otherwise = 0
  int wmeas;                   // the wall measure number, = 0 otherwise
  char units[20];              // description of units
  float qty_est;               // quantity of the material itself, estimated
  float unit_cost_est;         // unit cost, estimated
  char comment[80];
}; 

// Note that the intermediate structure has no pointers, so static in size and entirely zeroed on CALLOC
typedef struct {

    enum MEASURE_PACKAGE_SORT_PRIORITY measure_priority[MAXECMS];   // measure package ranking primary sort key before BTCR
    enum LOGICAL measure_required[MAXECMS];                        // Is the measure required

    struct measure ecm[MAXECMS];        // structure array to store our ecm results
    struct measure_material ecmm[4 * MAXECMS];  // structure array to store our ecm measure material detail results

    int nms;       // index of last mesasure ecm[] analyzed
    int nmsm;      // index of last measure material ecmm[] analyzed

    float measure_sir[MAXECMS];
    int associated_winner_ecm_index[MAXECMS];
    int sorted_measure_index[MAXECMS];

    float ua_total;    // UA total for building
    float ua_walls;    // UA of walls
    float ua_windows;  // UA of windows
    float ua_doors;    // UA of doors
    float ua_attics;   // UA of all attic areas
    float ua_foundations;    // UA of sub spaces
    float ua_infiltration;   // UA for infiltration

    float solar_aperture_direct[COOLING + 1][SOLAR_DIFFUSE + 1];
    float solar_aperture_diffuse[COOLING + 1][SOLAR_DIFFUSE + 1];

    float internal_gain_day[MONTHS + 1];        // internal+solar heat gain by month daytime Btu
    float internal_gain_night[MONTHS + 1];      // internal+solar heat gain by month nightime Btu

    float specific_infiltration[MONTHS + 1];    // monthly specific infiltration
    int consider_evaporative_cooler;            // =1 if > 90% of months with toa > 78 have rhum < .5 (evap cooler flag)
    float evaporative_cooler_eer;               // the EER of evaporative coolers for this weather location

    float heat_dumped_to_unint_cond_space;
    float total_annual_free_heat;                          // annual total sum of freeheat
    float degree_hours[COOLING + 1][MONTHS + 1];    // actual monthly degree hours based on our balance temp Heating and cooling
    float primary_heating_system_seasonal_efficiency_pre_ductseal;         // pre-duct sealing primary system effic
    float heating_system_seasonal_efficiency_pre_ductseal;                 // pre-duct sealing aggregate heating efficiency
    float cooling_seer_pre_ductseal;                                       // pre-duct sealing cooling efficiency
    float save_win_cfm_tot[MONTHS + 1]; // a copy of window cfm totals by month for comparison
    float save_dor_cfm_tot[MONTHS + 1]; // a copy of door cfm totals by month for comparison

    char measure_name[N_CMS_ITEMIZED_COST + MAX_ITC][MEASURENAME_LEN + 1];          // list of measure names
    enum CONSERVATION_MEASURE_TYPE meas_type[N_CMS_ITEMIZED_COST + MAX_ITC];        // measure type codes, see initialize_neat_measure_exclusion() in neat.c (MJF 4/99 DEBUG - was only dimensioned to MAXMEAS)

    int measure_exclusion[MAXMEAS][MAX_INERACTIONS_PER_MEASURE];                    // list of mutually exclusive measures
    int measure_exclusion_count[MAXMEAS];                       // how many mutually exclusive measures for each measure

    int billing_record_count[POST_COOLING + 1];                 // number of billing records in group
    int bill_year[POST_COOLING + 1][MONTHS + 1];                // year of each bill (yyyy)
    int bill_month[POST_COOLING + 1][MONTHS + 1];               // month number of each bill (1-12)
    int bill_day[POST_COOLING + 1][MONTHS + 1];                 // day of the month (1-31)
    int bill_first_period_days[POST_COOLING + 1];               // number of days in first billing period
    int bill_consumption[POST_COOLING + 1][MONTHS + 1];         // consumption of each bill
    int bill_degree_days[POST_COOLING + 1][MONTHS + 1];         // degree days relative to base F (optional) for each bill
    float bill_degree_day_base[POST_COOLING + 1];               // heating or cooling degree day base temperature, deg F
    float bill_base_load_consumption[POST_COOLING + 1];         // base load in consumption units
    int bill_consumption_units[POST_COOLING + 1];               // input units flag -1 missing, 0=therms, 1=kWh

    float night_setback_temperature[MONTHS + 1];                // monthly nighttime thermostat setback temperatures

    float balance_point_temp[DAY + 1][COOLING + 1][MONTHS + 1];   // balance point temps in (F) [][][AVG] = monthly average
    float building_load_coeff[MONTHS + 1];                        // monthly building load coefficients [AVG] = monthly average
    
    float heatload[PRE_RETROFIT_POST_DUCT_SEAL + 1];              // heating load, see index description below
    float coolload[PRE_RETROFIT_POST_DUCT_SEAL + 1];              // cooling load   same index meaning as heatload
    float heatengy[PRE_RETROFIT_POST_DUCT_SEAL + 1];              // heating energy same index meaning as heatload
    float coolengy[PRE_RETROFIT_POST_DUCT_SEAL + 1];              // cooling energy same index meaning as heatload

    float latentload[POST_RETROFIT + 1][MONTHS + 1]; // monthly latent loads [0]=pre retrofit [1]=post retrofit

    // float wn_cfm[NEAT_MAX_WIN][MONTHS + 1]; // window monthly avg leakage (cfm)
    // float wn_leak_coef[NEAT_MAX_WIN];            // window leakage coefficient
    float teff[MONTHS + 1];                                       // monthly average effective outdoor temperatures
    float teffn[MONTHS + 1];                                      // monthly average effective outdoor temperatures (night)
    float sysht_seaseff;               // composite heating system efficiency
    float htgmengy[POST_RETROFIT + 1][MONTHS + 1];     // monthly heating energy [0]=pre [1]=post retrofit
    float clgmengy[POST_RETROFIT + 1][MONTHS + 1];     // monthly cooling energy [0]=pre [1]=post retrofit
    
    float bladj[4];                     // utility billing adjustment factors

    // float uabsmt [NEAT_MAX_FND];        // UA of each basement record
    // float uasill[NEAT_MAX_FND];        // UA of each sill in each basement record
    // float uaeff[NEAT_MAX_FND];         // UA effective of each basement record
    float htengysav[MAXECMS]; // heating energy savings for each ecm
    float clengysav[MAXECMS]; // cooling energy savings for each ecm
    float blengysav[MAXECMS]; // base load energy savings in kWh for each ecm (MJF 1/01)
    float htdlsav[MAXECMS];   // heating energy dollar savings by each ecm
    float cldlsav[MAXECMS];   // ditto for cooling
    float bldlsav[MAXECMS];   // ditto for base loads (MJF 1/01)
    float htlcs[MAXECMS];     // lumped composite heating fuel cost
    float cllcs[MAXECMS];     // ditto for cooling
    float bllcs[MAXECMS];     // ditto for baseloads (MJF 1/01)
    float dslfsav[MAXECMS];   // total life cycle dollar savings by measure (numerator in benefit to cost ratio)
    int mslife[MAXECMS];      // measure life time in years (from materials list)
    int index_by_sir[MAXECMS];       // ordered index into the ecm[] array of IMPLEMENTED measures
    float npv[MAXECMS];                  // Net present value for measure
    float dfreht[MAXECMS][MONTHS + 1]; // Change in monthly freeheat for measure
    float dua[MAXECMS][MONTHS + 1];    // Change in monthly UA for measure
    enum INFILTRATION_REDUCTION_TREATMENT infiltration_treatment;   // infiltration reduction treatment

    int nother_materials; // number of last other_materials entry (MJF 3/01)
    enum LOGICAL screens_removed_for_winter;      // Oct through March (defaults to NO)

    float dblcs;           // Difference in winter and summer blc
    float rdbrdldh;        // Change in htg load from radiant barrier once implemented
    float rdbrdldc;        // Change in clg load from radiant barrier once implemented

    int adjflg;        // adjflg - 0/1 - run without/with billing adjustments
    int blapflg[PRE_COOLING + 1];          // apply billing adjustment flag, only for pre retrofit heating and cooling bills
    float tucsbsp[MONTHS + 1];             // monthly sub-basement (crawl space) average temperature F
    float tattic[MONTHS + 1];              // monthly attic temperature F
    float htmt[POST_RETROFIT + 1];         // total envelope heat loss (manual J) [0]=pre [1]=post retrofit
    float htduct[POST_RETROFIT + 1];       // duct heating losses (manual J) same indices as htmt[]

    int pntitcretrofit_materials[MAX_ITC]; // 1/0 print/don't print user defined measure materials (array of flags)
    float clmt[POST_RETROFIT + 1];         // total envelope cooling gain (manual J) [0]=pre [1]=post retrofit

    float totsol[MONTHS + 1]; // Total solar load on house.  6/08
    float ductarea;           // Area of exposed supply duct

    // common between NEAT and MHEA
    float whole_house_cfm_50_pre;               // cfm pre retrofit, base 50 pascals
    float whole_house_cfm_50_post;              // cfm post retrofit, base 50 pascals
    float whole_house_cfm[POST_RETROFIT + 1][MONTHS + 1];   // monthly pre and post retrofit whole house CFM infiltration

    float window_cfm_adjustment;                    // average reduction of window cfm due to MAX_WINDOW_DOOR_PERCENT
    float wn_cfm_tot[MONTHS + 1];                   // total of all windows' leakage (cfm)
    float wn_lat_load[MONTHS + 1];                  // monthly latent loads from all windows
    float wn_sunscrn[COOLING + 1][NEAT_MAX_WIN];    // added to properly handle sun screens 3/11
    float wn_leak_cfm[NEAT_MAX_WIN][MONTHS + 1];    // window leakage cfm by month and [AVG]

    float door_cfm_adjustment;                      // average reduction of door cfm due to MAX_WINDOW_DOOR_PERCENT
    float dr_cfm_tot[MONTHS + 1];                   // total of all doors' leakage (cfm)
    float dr_lat_load[MONTHS + 1];                  // monthly latent loads from all doors
    float dr_leak_cfm[NEAT_MAX_WIN][MONTHS + 1];    // door leakage cfm by month and [AVG]
    // end of common
    
} NIR;    // Neat Intermediate Results

#endif