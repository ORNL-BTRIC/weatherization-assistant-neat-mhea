/***************************************************************************
* MODULE:   intermediate.h            CREATED:    1/23/2020
*
* AUTHOR:   Mark Fishbaugher
*
* MDESC:    Structure to store all the MHEA intermediate results
****************************************************************************/
#ifndef _M_INTERMEDIATE_H
#define _M_INTERMEDIATE_H

// ****************************************************
// The Sizing struct contains system sizing results.
// NW, 3/37/97
//
// moved here from the old structs.h file MJF 5/97
// Units are in Btu/H for Winter design conditions
// ****************************************************
typedef struct {
  float fWal_loss;
  float fFlr_loss;
  float fRof_loss;
  float fWin_loss;
  float fDor_loss;
  float fInf_loss;
  float fDuc_loss;
  float fTot_loss;
} SIZING;

typedef struct {

  // measure description fields filled in by the
  // individual retrofit measures (others will be filled in
  // for baseload measures)

  char sName[MEASURENAME_LEN + 1];      // retrofit measure name
  char sComponents[STRING_LEN];         // list of component codes effected
  
  int measure_id;        // the fixed measure index number from measure_active_flags
  int audit_section_id;  // what section of the audit is this related to #164
  int material_id;       // the fixed material table index
  int component_id;      // field passed into the itemized cost, otherwise 0 #318
  int measure_index;     // FK into IMPLEMENTED mor->measures[]

  float fLife;           // lifetime in yrs (overrides rmc entry if != 0)
  float fQuant;          // the quantity of material in table units
  float fUnitMaterial;   // the cost in dollars per unit for material
  float fUnitLabor;      // the cost in dollars per unit for labor
  float fAddCost;        // additional costs ($) for this retrofit

  float costi1;    // another itemized cost
  float costi2;    // another cost adder
  char desci2[MEASURENAME_LEN + 1]; // description of qty=1 costi2 if <> 0
  int typei2;      // the material type/category if costi2<>0
  float costi3;    // another cost adder
  char desci3[MEASURENAME_LEN + 1]; // description of qty=1 costi3 if <> 0
  int typei3;      // the material type/category if costi2<>0

  enum MEASURE_PACKAGE_SORT_PRIORITY measure_priority;   // measure package ranking primary sort key before BTCR
  enum LOGICAL measure_required;                         // is the measure required

  // energy consumption fields

  float fEnerPreHtg; // annual pre retrofit heating energy Btu
  float fEnerPstHtg; // annual post retrofit heating energy Btu

  float fEnerPreClg; // annual pre retrofit cooling energy Btu
  float fEnerPstClg; // annual post retrofit cooling energy Btu

  float fEnerPreBas; // annual pre retrofit baseload energy Btu
  float fEnerPstBas; // annual post retrofit baseload energy Btu

  // fuel type fields

  enum FUEL PreHtgFuel; // pre retrofit heating fuel type
  enum FUEL PstHtgFuel; // post retrofit heating fuel type
  enum FUEL PreClgFuel; // pre retrofit cooling fuel type
  enum FUEL PstClgFuel; // post retrofit cooling fuel type
  enum FUEL PreBasFuel; // pre retrofit baseload fuel type
  enum FUEL PstBasFuel; // post retrofit baseload fuel type

  // computed fields

  char sMaterial[MEASURENAME_LEN + 1]; // material description (use this or material_id)
  char sUnits[20];    // the units for the material
  float fInitCost;    // initial cost (mat, labor, other)
  float fEnerSavHtg;  // annual heating savings in MMBtu
  float fCostSavHtg;  // annual heating dollar savings
  float fEnerSavClg;  // annual cooling savings in MMBtu
  float fCostSavClg;  // annual cooling dollar savings
  float fEnerSavBas;  // annual baseload savings in MMBtu
  float fCostSavBas;  // annual baseload dollar savings

  float fEnerSavTot;    // annual total savings in MMBtu
  float fCostAnnSavTot; // annual total dollar savings
  float fCostSavTot;    // present worth of energy savings in dollars

  float fBCR; // ratio of PW savings to fInitCost

  // fields just for reporting materials

  char sReportMaterial[MEASURENAME_LEN + 1]; // name of the material for report only
  char sReportUnits[20];    // description of the units in report only
  float fReportQuant;       // quantity for report purposes only

  // cummulative fields for reporting (filled in during cummulative pass)

  float fTotAnnualSav; // cummulative annual dollar savings
  float fTotInitCost;  // cummulative initial costs in dollars
  float fTotPWSav;     // cummulative present worth of energy savings
  float fTotSIR;       // cummulative savings to investment ratio

  // field added for post BestTest version to allow reducing measure savings
  //   by factor of 0.6

  int flgGetBCR;

} BCR_RES; // basic benefit to cost ratio results

typedef struct {

    int Rndx;    // base zero measure counter, incremented at the END of the measure calculation

    // the Results array does not have a one to one
    // correspondence with measures.  there can be more
    // than one measure with the same measure number and
    // different materials and quantites (ie lighting)

    BCR_RES Results[MAXECMS];  /* Bennefit to Cost Ratio Results data by measure */

    int iSeason[MONTHS];          // NOTE base zero
    float fMonthlyHtgUse[MONTHS];
    float fMonthlyClgUse[MONTHS];

    int flgRetrofits[MHEA_MAX_CMS]; /* For Energy Use Calc Loop */

    char projectNameMHEA[9]; /* the project name without extension or path */

    char ClientName[31];              // Client Name used in 2 forms
    char sFileName[M_FILENAME_LEN];   // global string for file names
    char sMsg[300];                   // global messages for output
    char sComponents[STRING_LEN];     // global for passing component strings

    int AutoOverwrite;       /* See setup screen */
    int flgChangeToData ; /* back in the runing MJF 5/97  */
    enum INFILTRATION_REDUCTION_TREATMENT infiltration_treatment;   // infiltration reduction treatment

    enum ENGINE_PASS flgWhichPass; // Different modes for the egine

    int flgNoCLG;            // =1 if no COL records are defined
    float fCfmUser;          /* Blower Door Test Value indicated by user */
    float fCfm;              /* Blower Door Test Value changed by retrofits */
    float fRofInsAirSpace;   /* SLF 4/13/93, added for use in retrofit.c & ua_calcs.c */
    float fARFInsAirSpace;   /* NLW 12/30/93, added for retrofit calcs */
    float fWalInsAirSpace;   /* NLW 12/30/93, added for retrofit calcs */
    float fAWLInsAirSpace;   /* NLW 12/30/93, added for retrofit calcs */
    float fNetWalArea;       /* NLW 12/30/93, added for retrofit calcs */
    float fNetAWLArea;       /* NLW 12/30/93, added for retrofit calcs */
    float fBellyAirSpace;    /* NLW 12/30/93, added for retrofit calcs */
    float fAddBellyAirSpace; /* MBG 08/13/08, added for retrofit calcs in addition */
    float fJoistAirSpace;    /* NLW 07/17/94 */
    float fWngAirSpace;      /* NLW 07/17/94 */

    // This next set of variables is ONLY used to control output messages in the event
    // that equipment capacity part load ratios are less than one #345
    int flgPreHighHTGLoad;                  // Flags indicating if we have unmet heating or cooling loads pre and post retrofit
    int flgPreHighCLGLoad;
    int flgPostHighHTGLoad;
    int flgPostHighCLGLoad;
    int iPreHighLoadMonths[MONTHS + 1];     // True false if month heating or cooling loads higher than system capacity, pre retrofit
    int iPostHighLoadMonths[MONTHS + 1];    // True false if month heating or cooling loads higher than system capacity, post retrofit

    int flgLimitBellyInsul;    /* MJF 11/02 */
    int flgLimitBellyInsulAdd; /* MJF 11/02 */

    int billing_flag; // command line flags

    // Added for WA version 7.2

    float fCfm_house[M_POST_RETROFIT + 1];     //  New pre-ret, post-duct, and post-ret BD Cfms
    float fPa_house[M_POST_RETROFIT + 1];      //  Corresponding BD house pressures

    float fPa_duct_op[POST_RETROFIT + 1][RETURN_DUCT + 1];  // Pre/post|supply/return duct operating pressures
    float fQduct50[RETURN_DUCT + 1];                        // Duct leakage at 50 Pa prior to dividing into supply / return components
    
    float fUWall;            // Wall U-Value used to account for uninsulatable space
    float fAreaCeilingTotal; // Area of ceiling (sqft) accounting for water closet and skylights
    float fThicknessCeilIns; // Total thickness of insulation in the ceiling (in)

    float fRinFGCompressed;          // R/inch for compressed (1.5 lb/cuft) fiberglass insulation
    float fWingInsDepth;             // Existing insulation depth in wings
    float fBellyInsDepth;            // Existing insulation depth in belly
    float fFractWallUnins;           // Total Fraction of wall area uninsulatable
    float fStndFractWallUnins;         // Standard (hardwired) fraction of wall area uninsulatable
    float fWallInsDepth;               // Existing insulation depth in walls

    float fDensExistCelInsul;  // Density of existing (uncompressed) cellulose insulation
    float fRinExistCelInsul;   // R's/inch of existing (uncompressed) cellulose insulation
    float fDensExistBatInsul;  // Density of existing (uncompressed) fiberglass batt insulation
    float fRinCompCelInsul;    // R's/inch of compressed cellulose insulation
    float fRinCompBatInsul;    // R's/inch of compressed fiberglass batt insulation
    float fRofThicknessBattIns;       // Thickness of existing batt/blanket insulation in roof
    float fRofThicknessLFGIns;        // Thickness of existing loose FG insulation in roof
    float fRinBellyLFGInsul;   // R's/inch of retrofitted loose FG insulation in belly
    float fRinBellyCelInsul;  // R's/inch of retrofitted cellulose insulation in belly
    float fDensBellyLFGInsul; // Density of retrofitted loose FG insulation in belly
    float fDensBellyCelInsul;  // Density of retrofitted cellulose insulation in belly

    float fAFloorInsDepth; // Existing insulation depth in addition floor
    float fARoofInsDepth;  // Existing insulation depth in addition roof
    float fAWallInsDepth;  // Existing insulation depth in addition walls

    // Added for WA Version 8.0

    float fNightSetpoint; // Monthly night setpoint (F) possibly restricted to 4F above night outdoor temp.

    //float wn_lat_load[MONTHS + 1];          // monthly latent loads from all windows
    //float wn_cfm[MHEA_MAX_WIN][MONTHS + 1]; // window monthly avg leakage (cfm)
    //float wn_cfm_tot[MONTHS + 1];           // total of all windows' leakage (cfm)
    float fInfMeasEnrgyHtg[MHEA_MAX_CMS]; // Infiltration energy saved by measures V 8.
    float fInfMeasEnrgyClg[MHEA_MAX_CMS]; // Infiltration energy saved by measures V 8.
    float fAdj_Htg, fAdj_Clg;          // Billing data adjustment factors

    // Try to standardize on a naming convention here for computed energy results

    float fPre_Heating;           // Current measure pre retrofit (Btu)
    float fPre_Cooling;

    float fHeating_Energy;        // Output of current measure mhea_energy_use() run (Btu)
    float fCooling_Energy;

    float fBasecase_Heating;      // Fixed basecase for dwelling (not per measure) (Btu  NOT MMBtu)
    float fBasecase_Cooling;
    float fBasecase_Baseload;

    float fFinal_Heating;         // Fixed Final cummulative retrofit effects (Btu  NOT MMBtu)
    float fFinal_Cooling;
    float fFinal_Baseload;

    // Added for WA Version 8.34

    float fThicknessCeilIns_Add;    // Total thickness of insulation in the ceiling (in) in addition
    float fRofThicknessBattIns_Add; // Thickness of existing batt/blanket insulation in roof in addition
    float fRofThicknessLFGIns_Add;  // Thickness of existing loose FG insulation in roof in addition

    SIZING Htg_Sizing[POST_RETROFIT + 1]; // Arrays of Htg sizing results

    // common between NEAT and MHEA
    float whole_house_cfm_50_pre;               // cfm pre retrofit, base 50 pascals
    float whole_house_cfm_50_post;              // cfm post retrofit, base 50 pascals
    float whole_house_cfm[POST_RETROFIT + 1][MONTHS + 1];   // monthly pre and post retrofit whole house CFM infiltration

    float window_cfm_adjustment;                    // average reduction of window cfm due to MAX_WINDOW_DOOR_PERCENT
    float wn_cfm_tot[MONTHS + 1];                   // total of all windows' leakage (cfm)
    //float wn_lat_load[MONTHS + 1];                  // monthly latent loads from all windows
    //float wn_sunscrn[COOLING + 1][MHEA_MAX_WIN];    // added to properly handle sun screens 3/11
    //float wn_leak_cfm[MHEA_MAX_WIN][MONTHS + 1];    // window leakage cfm by month and [AVG]

    float door_cfm_adjustment;                      // average reduction of door cfm due to MAX_WINDOW_DOOR_PERCENT
    float dr_cfm_tot[MONTHS + 1];                   // total of all doors' leakage (cfm)
    //float dr_lat_load[MONTHS + 1];                  // monthly latent loads from all doors
    //float dr_leak_cfm[MHEA_MAX_DOR][MONTHS + 1];    // door leakage cfm by month and [AVG]
    // end of common

} MIR;    // Mhea Intermediate Results (note static in size)

#endif