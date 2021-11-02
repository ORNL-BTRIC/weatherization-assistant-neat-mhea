/*******************  MODULE NAME:  PRECALCS.C  **************************/
/**                     DATE:  May 1994 **/
/**                       BY:    SLF **/
/**  DESCRIPTION:        Contains functions called in the Run function before **/
/**     energyuse and retrofit calculations are called including:                       **/
/**                     initmor **/
/**                     GetWeatherData **/
/**      REVISIONS: **/
/**                                     12/23/94 SLF - Updated for Menuet 2.1a and MW4.4a.              **/
/**                                     03/02/95 SLF - Fixed problem reading slr.inp file!              **/
/**                                                                     **/
/**  MW3 revisions by MJF 4/97                                          **/
/**  7/18/97,NLW: - Amend DMAP for additional parameter page                            **/
/**  8/12/97,NLW: - Corrected RUN_MISSING_AWL reference (~line 354)     **/
/**  11/5/97,NLW: - Corrected run time door window test logic           **/
/**  10/18/01,MBG: - Added duct_leakage_MHEA() routine for new duct algorithms **/
/*************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <math.h>

#include "wa_engine.h"

static float rhum(float drybt, float wetbt, double alt);

// This routine computes the relative humidity given the wet and dry-bulb temperatures.

#define HYWEX(t) (c1 / t + c2 + c3 * t + c4 * t * t + c5 * t * t * t + c6 * log(t))

/***************************************************************************
 ** Function Name: AnalysisInit
 **          Date: December 18, 1998
 **     Author(s): Mark Fishbaugher
 **
 **  DESCRIPTION: initializes arrays and some structures used in analysis
 **************************************************************************/
void analysis_initialize(void) {

  water_heater_factors(&mdi->dwh);

  //Required vs optional door replacements now handled by retro_door and retro_door_add #245
  //mdi->cms[M_CMS_REPLACE_DOORS_REQUIRED].active = NO;     // #271

  mdi->ubc.usage_units= KWH;    // #263

  // it is possible that we may have some blank structures, so just
  // make the NONE choice explicit at this point.  MJF 11/2

  if (mdi->htg.equip_type == 0)    // same for heating equip
    mdi->htg.equip_type = ET_NONE; // make it an explicit NONE in the absence of data
  if (mdi->ht2.equip_type == 0)    // same for heating equip
    mdi->ht2.equip_type = ET_NONE; // make it an explicit NONE in the absence of data
  if (mdi->htr.equip_type == 0)    // same for heating equip
    mdi->htr.equip_type = ET_NONE; // make it an explicit NONE in the absence of data

  if (mdi->clg.equip_type == 0) // same for cooling, just in case
    mdi->clg.equip_type = CE_NONE;
  if (mdi->cl2.equip_type == 0)
    mdi->cl2.equip_type = CE_NONE;
  if (mdi->clr.equip_type == 0)
    mdi->clr.equip_type = CE_NONE;

  // set our global flag to indicate if we have a cooling system or not

  if (mdi->clg.equip_type == CE_NONE) // no equipment type selected
    mir->flgNoCLG = TRUE;                // not an error, just set GLB flag
  else
    mir->flgNoCLG = FALSE;

  mir->flgPreHighHTGLoad = FALSE;
  mir->flgPreHighCLGLoad = FALSE;
  mir->flgPostHighHTGLoad = FALSE;
  mir->flgPostHighCLGLoad = FALSE;

  // Set mdi->win|awn[].leak_coef from input structure values.
  // also sum the monthly total infiltration for windows plus windows in the addition if anyt

  for (int nc = 0; nc < mdi->num_win; nc++) {
    mdi->win[nc].leak_coef = window_leakage_coef(mdi->win[nc].leak);
    for (int m = 1; m <= MONTHS; m++) {
      mir->wn_cfm_tot[m] += 0.1f * mdi->win[nc].leak_coef * (float)POWC(cwd->avg_wind_mph[m], 1.6) *
          (2.0f * (mdi->win[nc].height + mdi->win[nc].width) / 12.0f * 
          (mdi->win[nc].num_n + mdi->win[nc].num_s + mdi->win[nc].num_e + mdi->win[nc].num_w));
    }
  }

  for (int nc = 0; nc < mdi->num_awn; nc++) {
    mdi->awn[nc].leak_coef = window_leakage_coef(mdi->awn[nc].leak);
    for (int m = 1; m <= MONTHS; m++) {
      mir->wn_cfm_tot[m] += 0.1f * mdi->awn[nc].leak_coef * (float)POWC(cwd->avg_wind_mph[m], 1.6) *
          (2.0f * (mdi->awn[nc].height + mdi->awn[nc].width) / 12.0f * 
          (mdi->awn[nc].num_n + mdi->awn[nc].num_s + mdi->awn[nc].num_e + mdi->awn[nc].num_w));
    }
  }

  // Ditto for doors plus doors in the addition

  for (int nc = 0; nc < mdi->num_dor; nc++) {
    mdi->dor[nc].leak_coef = door_leakage_coef(mdi->dor[nc].leakiness);
    for (int m = 1; m <= MONTHS; m++) {
      mir->dr_cfm_tot[m] += 0.1f * mdi->dor[nc].leak_coef * (float)POWC(cwd->avg_wind_mph[m], 1.6) *
          (2.0f * (mdi->dor[nc].height + mdi->dor[nc].width) / 12.0f * 
          (mdi->dor[nc].num_n + mdi->dor[nc].num_s + mdi->dor[nc].num_e + mdi->dor[nc].num_w));
    }
  }

  for (int nc = 0; nc < mdi->num_adr; nc++) {
    mdi->adr[nc].leak_coef = door_leakage_coef(mdi->adr[nc].leakiness);
    for (int m = 1; m <= MONTHS; m++) {
      mir->dr_cfm_tot[m] += 0.1f * mdi->adr[nc].leak_coef * (float)POWC(cwd->avg_wind_mph[m], 1.6) *
          (2.0f * (mdi->adr[nc].height + mdi->adr[nc].width) / 12.0f * 
          (mdi->adr[nc].num_n + mdi->adr[nc].num_s + mdi->adr[nc].num_e + mdi->adr[nc].num_w));
    }
  }

  if (mdi->clg.equip_type == CE_EVAPORATIVE) {
    mdi->clg.saturating_eff_for_evaporative = mdi->key.evaporative_cooler_actual_saturating_eff;
    if (mdi->cl2.equip_type == CE_EVAPORATIVE)
      mdi->clg.percent_area_room_ac = 50;
    else
      mdi->clg.percent_area_room_ac = 100;
  }

  if (mdi->cl2.equip_type == CE_EVAPORATIVE) {
    mdi->cl2.saturating_eff_for_evaporative = mdi->key.evaporative_cooler_actual_saturating_eff;
    mdi->cl2.percent_area_room_ac = 50;
  }

}

// all runtime error and completness checks are now done in the
// database front end that calls this engine, MJF 4/25/02

static float rhum(float drybt, float wetbt, double alt) {
  double c1 = -1.044039708e4, c2 = -1.12946496e1, c3 = -2.7022355e-2, c4 = 1.289036e-5, c5 = -2.478068e-9, c6 = 6.5459673;
  double conv = 459.67;
  double pwsw, pwsd, patm, wsw, wsd, wnum, wden, w, mu, rh;

  drybt += (float)conv;
  wetbt += (float)conv;
  ASSERT(wetbt != 0, sprintf(msg, "Assertion Failure"));
  pwsw = HYWEX((double)wetbt);
  ASSERT(drybt != 0, sprintf(msg, "Assertion Failure"));
  pwsd = HYWEX((double)drybt);
  pwsw = exp(pwsw);
  pwsd = exp(pwsd);
  patm = 14.696 * exp(-0.0000368 * alt);
  ASSERT((patm - pwsw) != 0, sprintf(msg, "Assertion Failure"));
  ASSERT((patm - pwsd) != 0, sprintf(msg, "Assertion Failure"));
  wsw = 0.62198 * pwsw / (patm - pwsw);
  wsd = 0.62198 * pwsd / (patm - pwsd);
  wnum = (1093. - 0.556 * wetbt) * wsw - 0.24 * (drybt - wetbt);
  wden = 1093. + 0.444 * drybt - wetbt;
  ASSERT(wden != 0, sprintf(msg, "Assertion Failure"));
  w = wnum / wden;
  ASSERT(wsd != 0, sprintf(msg, "Assertion Failure"));
  mu = w / wsd;
  ASSERT(patm != 0, sprintf(msg, "Assertion Failure"));
  ASSERT((1. - (1. - mu) * pwsd / patm) != 0, sprintf(msg, "Assertion Failure"));
  rh = mu / (1. - (1. - mu) * pwsd / patm);
  return ((float)rh);
}

/***************************************************************************
 ** Function Name: duct_leakage_MHEA
 **          Date: July 27, 2000
 **     Author(s): Mike Gettings
 **
 **  DESCRIPTION: assignments from mdi structure for duct leakage
 **  and whole house infiltration, also computes estimate of
 **  pre/post duct leakage
 **************************************************************************/
void duct_leakage_MHEA(void) {
  float QH[2], Qopen, Qtaped;
  //double arg;
  double apn, bpn, cpn;
  int tt;

  // Duct Leakage input parameters declared locally since only need to
  // compute the duct leakage values

  int duct_method;          // Method used to measure duct leakage
  float cfm_house_bds[2];   // Pre/post whole house cfm - registers taped
  float pa_house_out[2];    // Pre/post whole house pa wrt outside under special
                            //   conditions for subtraction and duct-blower methods
  float pa_duct_bds[2];     // Pre/post duct-house pressure for subtration method
  float cfm_fan_dbpt[2][2]; // Pre/post|tot/out fan flow cfm for duct-blower test
  float pa_duct_dbpt[2][2]; // Pre/post|tot/out duct pressure wrt outside for duct blower test

  // Assignments to analysis global variables from the bld structure

  duct_method = mdi->inf.duct_seal_method;

  // first for whole house blower door numbers

  mir->fCfm_house[M_PRE_RETROFIT] = mdi->inf.pre_inf_cfm; // Pre-ret Blower Door Test (CFM)
  mir->fPa_house[M_PRE_RETROFIT] = mdi->inf.pre_inf_pa;   // Pre-ret Blower Door Test Pressure (PA)

  if (mir->fPa_house[M_PRE_RETROFIT] == 0.0f && mir->fCfm_house[M_PRE_RETROFIT] > 0.01f) // handle case where cfm is filled in but not the pa value (MJF 7/05)
    mir->fPa_house[M_PRE_RETROFIT] = BASE_PRESSURE;

  if (mdi->inf.evaluate_duct_sealing == YES && (duct_method == PRE_POST_WHOLE || duct_method == BLOWER_SUBTRACT)) {
    mir->fCfm_house[M_POST_DUCT_SEAL] = mdi->inf.post_duct_seal_cfm; // post duct sealing blower door cfm
    mir->fPa_house[M_POST_DUCT_SEAL] = mdi->inf.post_duct_seal_pa;
  } // post duct sealing blower door pa
  else {
    mir->fCfm_house[M_POST_DUCT_SEAL] = mir->fCfm_house[M_PRE_RETROFIT];
    mir->fPa_house[M_POST_DUCT_SEAL] = mir->fPa_house[M_PRE_RETROFIT];
  }

  mir->fCfm_house[M_POST_RETROFIT] = mdi->inf.post_inf_cfm; // Post-ret Blower Door Test (CFM)
  mir->fPa_house[M_POST_RETROFIT] = mdi->inf.post_inf_pa;   // Post-ret Blower Door Test Pressure (PA)

  // now for duct sealing variables

  mir->fPa_duct_op[PRE_RETROFIT][SUPPLY_DUCT] = mdi->inf.pre_duct_seal_supply_pa;  // pre duct seal supply operating pressure Pa
  //mir->fPa_duct_op[0][1] = mdi->inf.pre_duct_seal_return_pa;  // pre duct seal return operating pressure Pa
  mir->fPa_duct_op[POST_RETROFIT][SUPPLY_DUCT] = mdi->inf.post_duct_seal_supply_pa; // post duct seal supply operating pressure Pa
  //mir->fPa_duct_op[1][1] = mdi->inf.post_duct_seal_return_pa; // pre duct seal return operating pressure Pa

  cfm_house_bds[PRE_RETROFIT] = mdi->inf.pre_duct_seal_close_cfm;  // pre duct sealing blower door cfm with grills closed
  cfm_house_bds[POST_RETROFIT] = mdi->inf.post_duct_seal_close_cfm; // post duct sealing blower door cfm with grills closed

  pa_house_out[PRE_RETROFIT] = mdi->inf.pre_duct_seal_close_pa;  // pre duct sealing blower door pa with grills closed
  pa_house_out[POST_RETROFIT] = mdi->inf.post_duct_seal_close_pa; // post duct sealing blower door pa with grills closed

  pa_duct_bds[PRE_RETROFIT] = mdi->inf.pre_duct_seal_close_diff_pa;  // duct/house differential pressure pa grills closed
  pa_duct_bds[POST_RETROFIT] = mdi->inf.post_duct_seal_close_diff_pa; // duct/house differential pressure pa grills closed

  cfm_fan_dbpt[PRE_RETROFIT][0] = mdi->inf.pre_duct_seal_tot_cfm;  // duct blower flow total cfm
  cfm_fan_dbpt[PRE_RETROFIT][1] = mdi->inf.pre_duct_seal_out_cfm;  // duct blower flow to outside cfm
  cfm_fan_dbpt[POST_RETROFIT][0] = mdi->inf.post_duct_seal_tot_cfm; // duct blower flow total cfm
  cfm_fan_dbpt[POST_RETROFIT][1] = mdi->inf.post_duct_seal_out_cfm; // duct blower flow to outside cfm

  pa_duct_dbpt[PRE_RETROFIT][0] = mdi->inf.pre_duct_seal_tot_duct_pa;  // duct blower pressure wrt outside
  pa_duct_dbpt[PRE_RETROFIT][1] = mdi->inf.pre_duct_seal_out_duct_pa;  // duct blower pressure wrt to outside
  pa_duct_dbpt[POST_RETROFIT][0] = mdi->inf.post_duct_seal_tot_duct_pa; // duct blower pressure wrt outside
  pa_duct_dbpt[POST_RETROFIT][1] = mdi->inf.post_duct_seal_out_duct_pa; // duct blower pressure wrt to outside

  switch (duct_method) {

    case PRE_POST_WHOLE: { // Pre/Post Whole House Blower-Door Measurements
      // Must assume a sealed duct leakage = QSDUCT50
      for (tt = 0; tt < 2; tt++) {
        // Adjust leakages to 50 Pa
        if (fabs(mir->fPa_house[tt] - BASE_PRESSURE) > .2) {
          ASSERT(mir->fPa_house[tt] != 0, sprintf(msg, "Assertion Failure"));
          //QH[tt] = (float)(mir->fCfm_house[tt] * POWC((BASE_PRESSURE / mir->fPa_house[tt]), 0.6f));
          QH[tt] = blower_door_corrected_cfm((BASE_PRESSURE / mir->fPa_house[tt]), mir->fCfm_house[tt]);
        } else
          QH[tt] = mir->fCfm_house[tt];
      }
      // Determine duct leakage reduction as difference in house readings
      // Form pre/post duct leakages based on assumed post sealed leakage
      mir->fQduct50[1] = (float)QSDUCT50;
      mir->fQduct50[0] = (float)(QSDUCT50 + QH[0] - QH[1]);
      for (tt = 0; tt < 2; tt++) {
        if (mir->fQduct50[tt] < 0.0f)
          mir->fQduct50[tt] = 0.0f;
      }
      break;
    }

    case BLOWER_SUBTRACT: { // Blower-Door Subtraction Method
      for (tt = 0; tt < 2; tt++) {
        // Adjust leakages to 50 Pa
        if (fabs(mir->fPa_house[tt] - BASE_PRESSURE) > .2) {
          ASSERT(mir->fPa_house[tt] != 0, sprintf(msg, "Assertion Failure"));
          //Qopen = (float)(mir->fCfm_house[tt] * POWC((BASE_PRESSURE / mir->fPa_house[tt]), 0.6f));
          Qopen = blower_door_corrected_cfm((BASE_PRESSURE / mir->fPa_house[tt]), mir->fCfm_house[tt]);
        } else
          Qopen = mir->fCfm_house[tt];
        if (fabs(pa_house_out[tt] - BASE_PRESSURE) > .2) {
          ASSERT(pa_house_out[tt] != 0, sprintf(msg, "Assertion Failure"));
          //Qtaped = (float)(cfm_house_bds[tt] * POWC((pa_house_out[tt] / BASE_PRESSURE), 0.6f));
          Qtaped = blower_door_corrected_cfm((BASE_PRESSURE / pa_house_out[tt]), cfm_house_bds[tt]);
        } else
          Qtaped = cfm_house_bds[tt];
        // Subtract leakages during tests with registers open and taped
        // Adjust for duct leakage to inside
        mir->fQduct50[tt] = (float)((Qopen - Qtaped) * EXPC(0.041317 * (BASE_PRESSURE - pa_duct_bds[tt])));
        if (mir->fQduct50[tt] < 0.0f)
          mir->fQduct50[tt] = 0.0f;
      }
      break;
    }

    case DUCT_BLOWER: { // Duct-Blower Pressure Tests
      for (tt = 0; tt < 2; tt++) {
        ASSERT(pa_duct_dbpt[tt][1] != 0, sprintf(msg, "Assertion Failure"));
        //arg = (double)(BASE_PRESSURE / pa_duct_dbpt[tt][1]);
        //mir->fQduct50[tt] = (float)(cfm_fan_dbpt[tt][1] * POWC(arg, 0.6f));
        mir->fQduct50[tt] = duct_blower_corrected_cfm((BASE_PRESSURE / pa_duct_dbpt[tt][1]), cfm_fan_dbpt[tt][1]);
      }
      break;
    }

    case PRESSURE_PAN: { // Pressure Pan Tests  ONLY supported for MHEA not NEAT
      apn = 150.0;
      bpn = 0.4;
      cpn = 1.206834; // Logarithmic fit
      for (tt = 0; tt < 2; tt++) {
        mir->fQduct50[tt] = (float)(apn * log(bpn * (double)pa_duct_dbpt[tt][1] + cpn));
      }
      break;
    }

    default: {
      break;    // nothing
    }

  }
}

// This routine computes duct distribution efficiencies per ASHRAE 152P given
// the leakage of the ducts measured from three established techniques.

int duct_efficiency_MHEA(float fElevation, float fW_Dsgn_T, float fS_Dsgn_T, float *Q50, float *effht, float *effcl) {

  float enthalpyMHEA(float dbt, float RH, float tambR, float *wout);

  int m, nhtm, nclm;
  int iret;

  float arg, asBs, arBr, Qinf15, Qimb15, wout, wdum, win;
  float tom, tdb_max_cl, tdb_min_ht, rhatmax;
  float enthalpyMHEA(float dbt, float RH, float tambR, float *w);
  int compute_ducts = TRUE;

  //   Duct perimeters based on x-sectional area of 0.89 & 1.78 sqft for
  //   single and double wide homes assuming duct aspect ratio of 2:1

  float fDuctPerimSingle = 4.00f;
  float fDuctPerimDouble = 5.66f;

  float tin_ht,        // Indoor air temperature, heating
      tin_cl,          // Indoor air temperature, cooling
      tdb_ht,          // Outdoor dry bulb temperature, heating
      tdb_cl,          // Outdoor dry bulb temperature, cooling
      RH,              // Seasonal (summer) relative humidity
                       //    fW_Dsgn_T,         // Winter design temperature
                       //    fS_Dsgn_T,         // Summer design temperature
      tground,         // Ground temperature (year-round)
      tsp,             // Supply pleum dry-bulb temperature, cooling
      fAreaHome,       // Total living space floor area of home and addition
      Fout_S,          // Fraction of supply duct outside conditioned space
      Fout_R,          // Fraction of return duct outside conditioned space
      Aout_S,          // Area of supply duct outside conditioned space
      Aout_R,          // Area of return duct outside conditioned space
      Atot_S,          // Total area of supply duct
      //Atot_R,          // Total area of return duct
      tamb_S_ht,       // Ambient temperature at supply duct location, heating
      tamb_R_ht,       // Ambient temperature at return duct location, heating
      tamb_S_cl,       // Ambient temperature at supply duct location, cooling
      tamb_R_cl,       // Ambient temperature at return duct location, cooling
      hin,             // Enthalpy of inside conditioned air
      hout,            // Outdoor seasonal enthalpyMHEA
      hamb_R,          // Enthalpy of ambient air for return
      ductR_S,         // Supply duct R-value  (h-sqft-F/Btu)
      ductR_R,         // Return duct R-value  (h-sqft-F/Btu)
      dtemax_ht,       // Maximum temperture change across heat exchanger
      Ecap_ht,         // Heating capacity, Btu/hr
      Ecap_cl,         // Cooling capacity, Btu/hr
      Qe_ht,           // Heating fan flow rate (CFM)
      Qe_htmin,        // Heating fan flow rate minimum based on floor area (CFM)
      Qe_htmax,        // Heating fan flow rate maximum based on floor area (CFM)
      Qe_cl,           // Cooling fan flow rate (CFM)
      Qs_ht,           // Supply duct leakage rate (CFM), heating (measured)
      Qs_cl,           // Supply duct leakage rate (CFM), cooling (measured)
      Qr_ht,           // Return duct leakage rate (CFM), heating (measured)
      Qr_cl,           // Return duct leakage rate (CFM), cooling (measured)
      Qnet_ht,         // Net inf., combined natural and imbalance, CFM, heating
      Qnet_cl,         // Net inf., combined natural and imbalance, CFM, cooling
      Qinf,            // Natural infiltrtion rate (CFM)
      Qimb,            // Supply/Return flow imbalance, heating or cooling
      Fcyc,            // Cyclic loss factor
      F_cycloss,       // Duct thermal mass factor
      DTe_ht,          // Temperature change across equipment, heating
      DTe_cl,          // Temperature change across equipment, cooling
      DTr_ht,          // Building-return duct ambient temperature difference, heating
      DTr_cl,          // Building-return duct ambient temperature difference, cooling
      DTs_ht,          // Building-supply duct ambient temperature difference
      Bs_ht,           // Conduction fraction, supply, heating
      Bs_cl,           // Conduction fraction, supply, cooling
      Br_ht,           // Conduction fraction, return, heating
      Br_cl,           // Conduction fraction, return, cooling
      as_ht,           // Duct leakage factor, supply, heating
      as_cl,           // Duct leakage factor, supply, cooling
      ar_ht,           // Duct leakage factor, return, heating
      ar_cl,           // Duct leakage factor, return, cooling
      F_regain_S,      // Thermal regain factor, supply
      F_regain_R,      // Thermal regain factor, return
      DE_seas_ht,      // Delivery effectiveness, heating
      DE_seas_cl,      // Delivery effectiveness, cooling
      F_load_ht,       // Air infiltration load, heating
      F_load_cl,       // Air infiltration load, cooling
      F_equip_ht,      // Equipment efficiency factor, heating
      F_equip_cl,      // Equipment efficiency factor, cooling
      DE_corr_ht,      // Delivery effectiveness, corrected, heating
      DE_corr_cl,      // Delivery effectiveness, corrected, cooling
      N_dist_effic_ht, // Seasonal distribution system efficiency, heating
      N_dist_effic_cl; // Seasonal distribution system efficiency, cooling

  // Set fixed values

  tin_ht = 68.0f; // Prescribed pg  9 152P Standard, May 99
  tin_cl = 78.0f; // Prescribed pg  9 152P Standard, May 99
  tsp = 55.0f;    // Prescribed pg 27 152P Standard, May 99
  dtemax_ht = 50.0f;
  //Fcyc = 0.02f; // non-metalic ducts (someday)
  Fcyc = 0.05f; // sheet metal ducts
  ductR_R = ductR_S = 1.54f;

  /**** Determine supply duct surface area outside condtnd space ***/

  if (mdi->gnl.width <= 20.0)
    Atot_S = fDuctPerimSingle * mdi->gnl.length;
  else
    Atot_S = fDuctPerimDouble * mdi->gnl.length;

  /**********
     Repeat calculation for pre- and post-duct sealing parameters
  ***********/
  for (iret = PRE_RETROFIT; iret <= POST_RETROFIT; iret++) {

    if (mdi->htg.duct_location == DL_FLOOR) { // Ducts in belly
      if (mdi->htg.duct_insl == DI_ABOVE) {
        if (mdi->flr.belly_condition == BC_GOOD && ((mdi->flr.belly_mineral_insl + mdi->flr.belly_loose_insl) > 0.0))
          Fout_S = 0.75f;
        else
          Fout_S = 1.0f;
      } else if (mdi->htg.duct_insl == DI_BELOW) {
        if (mdi->flr.belly_condition == BC_GOOD && ((mdi->flr.belly_mineral_insl + mdi->flr.belly_loose_insl) > 0.0))
          Fout_S = 0.25f;
        else
          Fout_S = 0.5f;
      } else if (mdi->htg.duct_insl == DI_AROUND)
        Fout_S = 0.250f;
      else
        Fout_S = 1.0f;
    }

    else if (mdi->htg.duct_location == DL_CEILING) { // Ducts in ceiling
      if (mdi->htg.duct_insl == DI_ABOVE)
        Fout_S = 0.25f;
      else if (mdi->htg.duct_insl == DI_BELOW)
        Fout_S = 1.0f;
      else if (mdi->htg.duct_insl == DI_AROUND)
        Fout_S = 0.25f;
      else
        Fout_S = 0.25f;
    }

    Aout_S = Atot_S * Fout_S;

    //Atot_R = 0.0f; // Assume no ducted return
    Aout_R = 0.0f;
    Fout_R = 0.0f;

    fAreaHome = (mdi->gnl.length * mdi->gnl.width);
    if (mdi->gnl.water_heater_closet == YES)
      fAreaHome -= 6.25;                           // WHC area sqft
    fAreaHome += mdi->afl.length * mdi->afl.width; // Add area of addtion

    // Divide leakage into supply and return components based on area fractions

    if ((Aout_S + Aout_R) <= 0.0) {
      effht[iret] = 1.0f;
      effcl[iret] = 1.0f;
      return (0);
    }
    Qs_ht = Q50[iret] * Aout_S / (Aout_S + Aout_R);
    Qr_ht = Q50[iret] * Aout_R / (Aout_S + Aout_R);

    // Adjust leakages to operating pressures in supply and return plenums
    // small change in code here to move arg assignment inside the if statement
    // because we have an undefined return duct op pressure from the input
    // MJF 4/02

    if (Qs_ht > 0.0f) {
      //arg = mir->fPa_duct_op[iret][SUPPLY_DUCT] / BASE_PRESSURE;
      //Qs_ht = (float)(Qs_ht * POWC(arg, 0.6f));
      Qs_ht = blower_door_corrected_cfm((mir->fPa_duct_op[iret][SUPPLY_DUCT] / BASE_PRESSURE), Qs_ht);
    } else
      Qs_ht = 0.0f;
    Qs_cl = Qs_ht;
    if (Qr_ht > 0.0f) {
      // arg = (float)(mir->fPa_duct_op[iret][RETURN_DUCT] / BASE_PRESSURE);
      // Qr_ht = (float)(Qr_ht * POWC(arg, 0.6f));
      // Qs_ht = blower_door_corrected_cfm((mir->fPa_duct_op[iret][RETURN_DUCT] / BASE_PRESSURE), Qr_ht);
      ASSERT(FALSE, sprintf(msg, "MHEA should not use return duct pressures or flow readings"));
    } else
      Qr_ht = 0.0f;
    Qr_cl = Qr_ht;

    // Read in values used for parametric study

    //  inputf = fopen("ductef.inp","r");

    //  outputf = fopen("duct.out","a");

    //  fprintf(outputf,"Total area  area outside  Fan flow      Sup leaks       Ret leaks     Dist Eff\n");
    //  fprintf(outputf," Sup   Ret   Sup   Ret    Htg  Clg     Htg     Clg     Htg     Clg    Htg  Clg\n");

    //  fprintf(outputf2,"    dry bulb             equip size  duct    leakage     dist effic\n");
    //  fprintf(outputf2,"    ht    cl   RH   area  ht    cl    R      ht    cl     ht      cl\n\n");

    //  for(;(nfread=fscanf(inputf,"%f %f %f %f %f %f %f %f %f %f %f",
    //    &tdb_ht, &tdb_cl, &fW_Dsgn_T, &fS_Dsgn_T, &RH, &fAreaHome, &Ecap_ht, &Ecap_cl,
    //    &ductR_S, &Qs_ht, &Qs_cl))==11;) [if used would have squiggly bracket here]

    /*  Find the average heating and cooling outdoor ambient temperatures and RH.
     *  Cooling is average of monthly temperautures above 65 F, heating those below.
     *  If no monthly temperatures are above or below 65 to form seasonal average,
     *  use the maximum or minumum monthly temperatures as the average. */

    tdb_ht = tdb_cl = 0.0f;
    nhtm = nclm = 0;
    RH = 0.0f;
    tdb_min_ht = 100.0f;
    tdb_max_cl = -100.0f;
    for (m = 1; m <= MONTHS + 1 - 1; m++) {
      //tom = mir->ftoa[m]; // changed to global 1/02
      tom = cwd->avg_temp[m];
      if (tom < 65.0f) {
        if (m == DECEMBER || m < APRIL) {
          tdb_ht += tom;
          nhtm++;
        }
      }
      if (tom < tdb_min_ht)
        tdb_min_ht = tom;
      if (tom > 65.0f) {
        tdb_cl += tom;
        RH += rhum(tom, cwd->avg_wet_temp[m], fElevation);
        nclm++;
      }
      if (tom > tdb_max_cl) {
        tdb_max_cl = tom;
        rhatmax = rhum(tdb_max_cl, cwd->avg_wet_temp[m], fElevation);
      }
    }

    if (nhtm > 0)
      tdb_ht /= (float)nhtm;
    else
      tdb_ht = tdb_min_ht;
    if (nclm > 0) {
      tdb_cl /= (float)nclm;
      RH /= (float)nclm;
    } else {
      tdb_cl = tdb_max_cl;
      RH = rhatmax;
    }

    // Derived values

    tground = (fW_Dsgn_T + fS_Dsgn_T) / 2.0f;
    Qinf = 8.0f * 0.35f * fAreaHome / 60;     // Based on 0.35 ACH infiltration
    if (mdi->htg.duct_location == DL_FLOOR) { // Ducts in uninsulated basement
      tamb_R_ht = tamb_S_ht = (5.0f * tground + 2.0f * tdb_ht + 3.0f * tin_ht) / 10.0f;
      tamb_R_cl = tamb_S_cl = (5.0f * tground + 2.0f * tdb_cl + 3.0f * tin_cl) / 10.0f;
    } else { // Ducts in vented attic (duct_loc = A)
      tamb_R_ht = tamb_S_ht = tdb_ht + 2.0f;
      tamb_R_cl = tamb_S_cl = tdb_cl + 9.0f;
    }
    if (Fout_R == 0.0) {
      tamb_R_cl = tin_cl;
      tamb_R_ht = tin_ht;
    }

    if (tamb_R_cl < tin_cl)
      tamb_R_cl = (tdb_cl + tamb_R_cl) / 2.0f;
    if (tamb_R_ht > tin_ht)
      tamb_R_ht = (tdb_ht + tamb_R_ht) / 2.0f;

    F_equip_ht = 1.0f; // All single capacity furnaces - Table 6.7 pg.28
    F_equip_cl = 1.0f; // Thermostatic control A/C - Table 6.7 pg.28
    F_cycloss = Fcyc * (Fout_S + Fout_R) / 2.0f;

    if (mdi->htg.duct_location == DL_FLOOR) { // Ducts in uninsulated basement
      F_regain_S = F_regain_R = 0.5f;
    } // Uninsulated basement
    else {
      F_regain_S = F_regain_R = 0.1f;
    } // Vented attic

    /*******************************/
    /***  Heating Computations  ****/
    /*******************************/

    if ((mdi->htg.equip_type != ET_FURNACE && mdi->htg.equip_type != ET_HEATPUMP) || compute_ducts == FALSE) {
      // No forced air system or no ducts in unconditioned spaces
      N_dist_effic_ht = 1.0f;
      //Ecap_ht = 0.0f;
      //Qe_ht = 0.0f;
    }

    else {

      if (mdi->htg.capacity  == 0) {
        mdi->htg.capacity  = 60;            // MJF Issue #80, assign a ballpark capacity if missing this value
      }

      Ecap_ht = mdi->htg.capacity * 1000.0f;
      Qe_ht = 0.85f * Ecap_ht / (60.0f * dtemax_ht * RHOCAIR); // (H-2, pg.86 for furnaces)

      // Compute limits to fan flow rate and restrain rate used to these limits (MBG 4/06)

      Qe_htmin = 0.5f * (float)fAreaHome;
      Qe_htmax = 1.0f * (float)fAreaHome;
      if (Qe_ht < Qe_htmin)
        Qe_ht = Qe_htmin;
      if (Qe_ht > Qe_htmax)
        Qe_ht = Qe_htmax;

      // Qs_ht = 0.17f*Fout_S*Qe_ht;  // Fout's must be less than or equal to 0.5
      // Qr_ht = 0.17f*Fout_R*Qe_ht;

      // Prevent Qs_ht and Qr_ht from becoming greater than 0.95 of Qe_ht

      if (Qs_ht >= 0.95f * Qe_ht)
        Qs_ht = 0.95f * Qe_ht; // Leakage greater than supply or
      if (Qr_ht >= 0.95f * Qe_ht)
        Qr_ht = 0.95f * Qe_ht; //  return flow rates

      ASSERT((60.0f * Qe_ht * RHOCAIR * ductR_S) != 0, sprintf(msg, "Assertion Failure"));
      ASSERT((60.0f * Qe_ht * RHOCAIR * ductR_R) != 0, sprintf(msg, "Assertion Failure"));

      arg = -Aout_S / (60.0f * Qe_ht * RHOCAIR * ductR_S);
      Bs_ht = (float)(exp((float)(arg)));
      arg = -Aout_R / (60.0f * Qe_ht * RHOCAIR * ductR_R);
      Br_ht = (float)(exp((float)(arg)));

      ASSERT(Qe_ht != 0, sprintf(msg, "Assertion Failure"));
      as_ht = (Qe_ht - Qs_ht) / Qe_ht;
      ar_ht = (Qe_ht - Qr_ht) / Qe_ht;

      DTe_ht = Ecap_ht / (60.0f * Qe_ht * RHOCAIR);
      DTs_ht = tin_ht - tamb_S_ht;
      DTr_ht = tin_ht - tamb_R_ht;

      /***** Compute the imbalance flow, Qnet ******/

      Qimb = (float)(fabs((float)(Qs_ht - Qr_ht)));
      Qinf15 = (float)(POWC(Qinf, 1.5f));
      if (Qimb < 1.0e-3)
        Qnet_ht = (float)(POWC(Qinf15, 0.67f));
      else {
        Qimb15 = (float)(POWC(Qimb, 1.5f));
        if (Qs_ht > Qr_ht)
          Qnet_ht = (float)(POWC((Qinf15 + Qimb15), 0.67f));
        else {
          if (Qimb >= Qinf)
            Qnet_ht = 0.0f;
          else
            Qnet_ht = (float)(POWC((Qinf15 - Qimb15), 0.67f));
        }
      }

      /***** Compute delivery effectiveness and air infiltration load, F_load *****/

      asBs = as_ht * Bs_ht;
      arBr = ar_ht * Br_ht;

      ASSERT(DTe_ht != 0, sprintf(msg, "Assertion Failure"));
      ASSERT(Ecap_ht != 0, sprintf(msg, "Assertion Failure"));

      DE_seas_ht = asBs - asBs * (1.0f - arBr) * DTr_ht / DTe_ht - as_ht * (1.0f - Bs_ht) * DTs_ht / DTe_ht;
      ASSERT(DE_seas_ht != 0, sprintf(msg, "Assertion Failure"));
      F_load_ht = 1.0f - (60.0f * RHOCAIR * (tin_ht - tdb_ht) * (Qnet_ht - Qinf)) / Ecap_ht / DE_seas_ht; // (6-40) p.30

      /***** Compute the corrected delivery effectiveness and distribution efficiency ******/

      DE_corr_ht = DE_seas_ht + F_regain_S * (1.0f - DE_seas_ht) -
                   (F_regain_S - F_regain_R - Br_ht * (ar_ht * F_regain_S - F_regain_R)) * DTr_ht / DTe_ht; // (6-34) p.30

      N_dist_effic_ht = DE_corr_ht * F_equip_ht * F_load_ht * (1.0f - F_cycloss); // (6-44) p.31
    }

    /*******************************/
    /***  Cooling Computations  ****/
    /*******************************/

    /**** Determine supply duct surface area outside condtnd space for cooling ***/

    if (mir->flgNoCLG == TRUE || compute_ducts == FALSE || (mdi->clg.equip_type != CE_CENTRALAC && mdi->clg.equip_type != CE_HEATPUMP)) {
      N_dist_effic_cl = 1.0f; // no duct losses computed
      //Qe_cl = 0.0f;
    } else {                                    // otherwise compute duct losses for primary system
      if (mdi->clg.duct_location == DL_FLOOR) { // Ducts in belly
        if (mdi->clg.duct_insl == DI_ABOVE) {
          if (mdi->flr.belly_condition == BC_GOOD && ((mdi->flr.belly_mineral_insl + mdi->flr.belly_loose_insl) > 0.0))
            Fout_S = 0.75f;
          else
            Fout_S = 1.0f;
        } else if (mdi->clg.duct_insl == DI_BELOW) {
          if (mdi->flr.belly_condition == BC_GOOD && ((mdi->flr.belly_mineral_insl + mdi->flr.belly_loose_insl) > 0.0))
            Fout_S = 0.0f;
          else
            Fout_S = 0.5f;
        } else if (mdi->clg.duct_insl == DI_AROUND)
          Fout_S = 0.0f;
        else
          Fout_S = 1.0f;
      }

      else if (mdi->clg.duct_location == DL_CEILING) { // Ducts in ceiling
        if (mdi->clg.duct_insl == DI_ABOVE)
          Fout_S = 0.0f;
        else if (mdi->clg.duct_insl == DI_BELOW)
          Fout_S = 1.0f;
        else if (mdi->clg.duct_insl == DI_AROUND)
          Fout_S = 0.25f;
        else
          Fout_S = 0.0f;
      }

      Aout_S = Atot_S * Fout_S;
      Ecap_cl = mdi->clg.capacity * 1000.0f;

      if (Ecap_cl < 0.001f || compute_ducts == FALSE) {
        // No central A/C or no ducts in unconditioned spaces
        N_dist_effic_cl = 1.0f;
        // Qe_cl = 0.0f;    // Issue #156 remove dead assignment
      } else {

        Qe_cl = 340.0f * Ecap_cl / 12000.0f; // Default value
        if (Qs_cl >= Qe_cl)
          Qs_cl = 0.95f * Qe_cl; // Leakage greater than supply or
        if (Qr_cl >= Qe_cl)
          Qr_cl = 0.95f * Qe_cl; //  return flow rates

        ASSERT((60.0f * Qe_cl * RHOCAIR * ductR_S) != 0, sprintf(msg, "Assertion Failure"));
        ASSERT((60.0f * Qe_cl * RHOCAIR * ductR_R) != 0, sprintf(msg, "Assertion Failure"));

        arg = -Aout_S / (60.0f * Qe_cl * RHOCAIR * ductR_S);
        Bs_cl = (float)(exp((float)(arg)));
        arg = -Aout_R / (60.0f * Qe_cl * RHOCAIR * ductR_R);
        Br_cl = (float)(exp((float)(arg)));

        ASSERT(Qe_cl != 0, sprintf(msg, "Assertion Failure"));

        as_cl = (Qe_cl - Qs_cl) / Qe_cl;
        ar_cl = (Qe_cl - Qr_cl) / Qe_cl;

        DTe_cl = -Ecap_cl / (60.0f * Qe_cl * RHOCAIR);
        DTr_cl = tin_cl - tamb_R_cl;

        /****** Compute the cooling season return enthalpyMHEA from psychometrics *****/

        hout = enthalpyMHEA(tdb_cl, RH, tdb_cl, &wout);
        win = 0.004f + 0.4f * wout; // Per e-mail of 5/22/00 from Iain Walker
        hin = 0.240f * tin_cl + win * (1061.0f + 0.444f * tin_cl);
        if (mdi->clg.duct_location == DL_FLOOR || Fout_R == 0.0)
          hamb_R = hin;
        else {
          hamb_R = enthalpyMHEA(tdb_cl, RH, tamb_R_cl, &wdum);
          if (hamb_R < hin)
            hamb_R = (hamb_R + hout) / 2.0f;
        }

        /***** Compute the imbalance flow, Qnet ******/

        Qimb = (float)(fabs((float)(Qs_cl - Qr_cl)));
        if (Qimb < 1.0e-3)
          Qnet_cl = (float)(POWC(Qinf15, 0.67f));
        else {
          Qimb15 = (float)(POWC(Qimb, 1.5f));
          if (Qs_ht > Qr_ht)
            Qnet_cl = (float)(POWC((Qinf15 + Qimb15), 0.67f));
          else {
            if (Qimb >= Qinf)
              Qnet_cl = 0.0f;
            else
              Qnet_cl = (float)(POWC((Qinf15 - Qimb15), 0.67f));
          }
        }

        /***** Compute delivery effectiveness and air infiltration load, F_load *****/

        ASSERT(Ecap_cl != 0, sprintf(msg, "Assertion Failure"));

        DE_seas_cl =
            as_cl -
            60.0f * as_cl * Qe_cl * RHO / Ecap_cl * ((1.0f - ar_cl) * (hamb_R - hin) + ar_cl * CAIR * (Br_cl - 1.0f) * DTr_cl +
                                                     CAIR * (Bs_cl - 1.0f) * (tsp - tamb_S_cl));
        ASSERT(DE_seas_cl != 0, sprintf(msg, "Assertion Failure"));
        F_load_cl = 1.0f + (60.0f * RHO * (hin - hout) * (Qnet_cl - Qinf)) / Ecap_cl / DE_seas_cl; // (6-40) p.30

        /***** Compute the corrected delivery effectiveness and distribution efficiency ******/

        ASSERT(DTe_cl != 0, sprintf(msg, "Assertion Failure"));

        DE_corr_cl = DE_seas_cl + F_regain_S * (1.0f - DE_seas_cl) -
                     (F_regain_S - F_regain_R - Br_cl * (ar_cl * F_regain_S - F_regain_R)) * DTr_cl / DTe_cl; // (6-34) p.30

        N_dist_effic_cl = DE_corr_cl * F_equip_cl * F_load_cl * (1.0f - F_cycloss); // (6-44) p.31
      }
    }

    /**************************************************/
    /******  Print interim and final results  *********/
    /**************************************************/
    //  fprintf(outputf,"\n%7.0f%7.0f%7.2f%7.2f%6.0f%6.0f%6.0f%6.0f%7.0f  %c%7.3f%7.3f",
    //  Atot_S,Atot_R,Fout_S,Fout_R,Qs_ht,Qr_ht,Qe_ht,Qe_cl,Qinf,duct_loc,
    //  N_dist_effic_ht,N_dist_effic_cl);
    //
    //  fprintf(outputf2,"\n%6.0f%6.0f%6.2f%6.0f%6.0f%6.0f%5.1f%6.0f%6.0f%8.3f%8.3f",
    //  tdb_ht,tdb_cl,RH,fAreaHome,Ecap_ht,Ecap_cl,ductR_S,Qs_ht,Qs_cl,N_dist_effic_ht,N_dist_effic_cl);
    /*
     *  printf("\n%8.3f  Total supply duct area (Sqft)   ",Atot_S);
     *  printf("\n%8.3f  Total return duct area (Sqft)   ",Atot_R);
     *  printf("\n%8.3f  Supply duct area outside conditioned space (Sqft)",Aout_S);
     *  printf("\n%8.3f  Return duct area outside conditioned space (Sqft)",Aout_R);
     *  printf("\n%8.3f  Heating fan flow rate (CFM)     ",Qe_ht);
     *  printf("\n%8.3f  Cooling fan flow rate (CFM)     ",Qe_cl);
     *  printf("\n%8.3f  Supply duct leakage rate (CFM)(heating)    ",Qs_ht);
     *  printf("\n%8.3f  Supply duct leakage rate (CFM)(cooling)    ",Qs_cl);
     *  printf("\n%8.3f  Return duct leakage rate (CFM)(heating)    ",Qr_ht);
     *  printf("\n%8.3f  Return duct leakage rate (CFM)(cooling)    ",Qr_cl);
     *  printf("\n%8.3f  Seasonal distribution efficiency (heating) ",N_dist_effic_ht);
     *  printf("\n%8.3f  Seasonal distribution efficiency (cooling) ",N_dist_effic_cl);
    */
    //  fclose(inputf);
    //  fclose(outputf);
    //  fclose(outputf2);

    if (N_dist_effic_ht > 1.0)
      N_dist_effic_ht = 1.0f;
    if (N_dist_effic_cl > 1.0)
      N_dist_effic_cl = 1.0f;
    if (N_dist_effic_ht < 0.0)
      N_dist_effic_ht = 0.0f;
    if (N_dist_effic_cl < 0.0)
      N_dist_effic_cl = 0.0f;
    effht[iret] = 0.5f * (N_dist_effic_ht + 1.0f); //  Map efficiencies into range
    effcl[iret] = 0.5f * (N_dist_effic_cl + 1.0f); //    of 0.5 to 1.0

  } // End pre- and post-duct sealing loop

  return (0);
}

// Compute the enthalpyMHEA of air in space where return duct runs, given outdoor
//m dry bulb temperature and relative humidity and approximate temperature of the space.

float enthalpyMHEA(float dbt, float RH, float tambR, float *wout) {
  double c1 = -1.044039708e4, c2 = -1.12946496e1, c3 = -2.7022355e-2, c4 = 1.289036e-5, c5 = -2.478068e-9, c6 = 6.5459673;
  double conv = 459.67;
  float pws, pw, w, patm = 14.696f, h, dbtabs;

  // printf("\n dbt = %5.1f,  RH = %5.2f,  tambR = %5.2f",dbt,RH,tambR);

  dbtabs = dbt + (float)conv;
  pws = (float)HYWEX((double)dbtabs);
  pws = (float)exp(pws);
  pw = RH * pws;
  w = 0.62198f * pw / (patm - pw);
  *wout = w;
  h = 0.240f * tambR + w * (1061.0f + 0.444f * tambR);
  // printf("\n pws = %8.3f,  h = %8.3f", pws,h);
  return (h);
}
