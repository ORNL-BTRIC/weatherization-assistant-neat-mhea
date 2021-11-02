/***************************************************************************
* MODULE:   infiltration.c            CREATED:    7/29/2020
*
* AUTHOR:   Mark Fishbaugher
*
* MDESC:    Calculations for ducts and infiltration
****************************************************************************/

#include "wa_engine.h"

static float corrected_cfm(float pressure_ratio, float cfm, float exponent);

/***************************************************************************
 ** Function Name: corrected_cfm
 **          Date: March 3, 2019
 **     Author(s): Mark Fishbaugher
 **
 **  DESCRIPTION: We make a correction if the pressure factor (ratio with 50)
 **  is greater that 4% absolute (positive or negative correction).  The
 **  previous version compared to 0.2f pa difference with 50pa base, or 0.4 %
 **************************************************************************/

static float corrected_cfm(float pressure_ratio, float cfm, float exponent){
  //return (fabs(1 - pressure_ratio) > 0.004f) ? (float)(cfm * POWC(pressure_ratio, exponent)) : (float)cfm;
  return (float)(cfm * POWC(pressure_ratio, exponent));
}

float blower_door_corrected_cfm(float pressure_ratio, float blower_door_cfm) {
  return corrected_cfm(pressure_ratio, blower_door_cfm, 0.65f);
}

float duct_blower_corrected_cfm(float pressure_ratio, float duct_blaster_cfm) {
  return corrected_cfm(pressure_ratio, duct_blaster_cfm, 0.6f);
}

/*******************************************
 * Function name window_leakage_coef
 * Date:  3/17/2019
 * Author: MJF
 *
 * Description:  This was an index static array
 * but this removes the magic numbers and clearly
 * associates the coefficients with the leakiness enumeration
 * ***************************************/
float window_leakage_coef(enum LEAKINESS leak) {
  switch (leak) {
  case VERY_TIGHT:
    return 0.0035f;
  case TIGHT:
    return 0.0120f;
  case MEDIUM:
    return 0.0460f;
  case LOOSE:
    return 0.1125f;
  case VERY_LOOSE:
    return 0.2400f;
  case MEC_REPLACEMENT:
    return WINDOW_MEC_LEAKAGE_COEF;
  }
  ASSERT(FALSE, sprintf(msg, "Unrecognized window leakiness: %d", leak));
}

// set our new sealing leakage coefficient based on the existing leakage coefficient as upward limit
float window_sealing_leak_coef(float leak_coef) {
  float save_coef, new_coef;
  save_coef = leak_coef;
  new_coef = leak_coef - SEALING_FRACTION * (leak_coef - SEALED_WINDOW_LEAKAGE_COEF);
  if (new_coef > save_coef)
    new_coef = save_coef;
  return new_coef;
}

// the effect on window infiltration from adding a storm window
float window_storm_leak_coef(float leak_coef) {
  return (float)((POWC((1 / (POWC(1 / leak_coef, 1.25) + 80.09)), 0.8)));
}

/*******************************************
 * Function name door_leakage_coef
 * Date:  5/10/2019
 * Author: MJF
 *
 * Description:  This was an index static array
 * but this removes the magic numbers and clearly
 * associates the coefficients with the leakiness enumeration
 * ***************************************/
float door_leakage_coef(enum DOOR_LEAKINESS leak) {
  switch (leak) {
  case LEAKY_NONE:
    return 0.0460f; // a default value if none is selected = medium
  case LEAK_TIGHT:
    return 0.0120f;
  case LEAK_MEDIUM:
    return 0.0460f;
  case LEAK_LOOSE:
    return 0.2400f;
  }
  ASSERT(FALSE, sprintf(msg, "Unrecognized door leakiness: %d", leak));
}

/****************
 ****************
 * Routine for computing the specific infiltration (See Sherman, 1987)   *
 * given an average wind speed and indoor-outdoor temperature difference *
 * v is in mi/h converted to m/s, dt in F converted to K                 */

float get_specific_infiltration(float v, float dt) {
  dt *= 5.0f / 9.0f; /* Convert deg F to deg K */
  v *= 0.447f;       /* Convert mi/h to m/s    */
  return ((float)sqrt(.0169f * (double)(v * v) + .0144f * fabs((double)dt)) * 196.9f);
  /* Convert to ft/min */
}

// Fills in the whole_house_cfm (monthly and average array with weather factor adjustments

void monthly_infiltration(
  float whole_house_cfm_50_pre,
  float whole_house_cfm_50_post,
  float daytime_heating_setpoint,
  float stories,
  float whole_house_cfm[POST_RETROFIT + 1][MONTHS + 1]) {

  double fAltFactor = -3.72e-5;
  float density_adj = (float)exp(fAltFactor * (double)cwd->altitude * 0.5f);  // Factor adjusting cfm for density of air
    // see issue #183 and curve fit to the 1987 Sherman paper at
  // https://www.researchgate.net/publication/245196537_Estimation_of_infiltration_from_leakage_and_climate_indicators
  float cf1 = 1.225373f - (0.2549041f * stories) + 0.0249467f * pow(stories, 2);
  float cf2 = 1.2f;
  float cf3 = 1.0f;

  float cfinf = cf1 * cf2 * cf3;
  ASSERT(cfinf, sprintf(msg, "Need non zero infiltration flow rate"));

  whole_house_cfm[PRE_RETROFIT][AVG] = 0.0f;
  whole_house_cfm[POST_RETROFIT][AVG] = 0.0f;
  for (int m = 1; m <= MONTHS; m++) {
    float specific_infiltration = 0.0;
    for (int i = 0; i <= BIN_20_24; i++) {
      specific_infiltration += get_specific_infiltration(cwd->fWindSpdBins[m][i], cwd->fTempOutBins[m][i] - daytime_heating_setpoint);
    }
    specific_infiltration /= (BIN_20_24 + 1.0f);
    whole_house_cfm[POST_RETROFIT][m]    = (whole_house_cfm_50_post / 2756.0f / cfinf) * specific_infiltration * 0.5f * (1.0f + BUFFERED_TO_EXPOSED_WALL_DT_RATIO) * density_adj * INFILTRATION_ADJ_FACTOR;
    whole_house_cfm[POST_RETROFIT][AVG] += whole_house_cfm[POST_RETROFIT][m];
    whole_house_cfm[PRE_RETROFIT][m] = (whole_house_cfm_50_pre / 2756.0f / cfinf) * specific_infiltration * 0.5f * (1.0f + BUFFERED_TO_EXPOSED_WALL_DT_RATIO) * density_adj * INFILTRATION_ADJ_FACTOR;
    whole_house_cfm[PRE_RETROFIT][AVG] += whole_house_cfm[PRE_RETROFIT][m];
  }
  whole_house_cfm[PRE_RETROFIT][AVG] /= 12.0;
  whole_house_cfm[POST_RETROFIT][AVG] /= 12.0;
}

// Given the montly total window and door cfm, compute a CFM window adjustment factor
float window_constrain_cfm_factor(float whole_house_cfm[POST_RETROFIT + 1][MONTHS + 1], 
  float wn_cfm_tot[MONTHS + 1], 
  float dr_cfm_tot[MONTHS + 1]) {

  float factor = 1.0f;

  if (wn_cfm_tot[AVG] > 0.0f) {
    float old = wn_cfm_tot[AVG];
    for (int m = 1; m <= MONTHS; m++) {
      float base_cfm = (wn_cfm_tot[m] + dr_cfm_tot[m]);
      float adj = (MAX_WINDOW_DOOR_PERCENT / 100.0f) * whole_house_cfm[PRE_RETROFIT][m] / base_cfm;
      wn_cfm_tot[m] *= adj < 1.0f ? adj : 1.0;
    }
    wn_cfm_tot[AVG] = 0.0f;
    for (int m = 1; m <= MONTHS; m++)
      wn_cfm_tot[AVG] += wn_cfm_tot[m];
    wn_cfm_tot[AVG] /= 12.0f;
    factor = (old - wn_cfm_tot[AVG]) / old;
  }
  return factor < 1.0 ? factor : 1.0;
}

// Given the montly total window and door cfm, apply adjustment to individual window cfm
void window_constrain_cfm(float whole_house_cfm[POST_RETROFIT + 1][MONTHS + 1], 
  float wn_leak_cfm[NEAT_MAX_WIN][MONTHS + 1], 
  float wn_cfm_tot[MONTHS + 1], 
  float dr_cfm_tot[MONTHS + 1], 
  int num_win,
  float floor_area) {

  if (wn_cfm_tot[AVG] > 0.0f) {
    if (cmds.debug_level & D_INFILTRATION_DETAIL) {
      fprintf(stderr, "\n   Average initial monthly average WINDOW cfm = %7.2f", wn_cfm_tot[AVG]);
      fprintf(stderr, "\n    Raw WINDOW Leakage rates (cfm under natural conditions)\n m  ");
      for (int nc = 0; nc < num_win; nc++) {
        fprintf(stderr, "wn(%2d) ", nc);
      }
      fprintf(stderr, "Window House Percent\n");
      for (int m = 1; m <= MONTHS; m++) {
        fprintf(stderr, "%2d", m);
        for (int nc = 0; nc < num_win; nc++)
          fprintf(stderr, "%7.2f", wn_leak_cfm[nc][m]);
        fprintf(stderr, "%7.2f%7.2f%6.1f\n", 
          wn_cfm_tot[m], 
          whole_house_cfm[PRE_RETROFIT][m], 
          wn_cfm_tot[m] / whole_house_cfm[PRE_RETROFIT][m] * 100.);
      }
      fprintf(stderr, "\n m  whole_house_cfm[m]     ac    maxper  wnadj\n");
    }
    for (int m = 1; m <= MONTHS; m++) {
      float base_cfm = (wn_cfm_tot[m] + dr_cfm_tot[m]);
      float adj = (MAX_WINDOW_DOOR_PERCENT / 100.0f) * whole_house_cfm[PRE_RETROFIT][m] / base_cfm;
      if (cmds.debug_level & D_INFILTRATION_DETAIL) {
        float ac = (float)(whole_house_cfm[PRE_RETROFIT][m] * 60. / 8. / floor_area);
        fprintf(stderr, "%2d %7.2f %7.2f %7.1f%7.3f \n", m, whole_house_cfm[PRE_RETROFIT][m], ac, MAX_WINDOW_DOOR_PERCENT, adj);
      }
      if (adj < 1.0f) {
        for (int nc = 0; nc < num_win; nc++) {
          wn_leak_cfm[nc][m] *= adj;
        }
      }
    }
    if (cmds.debug_level & D_INFILTRATION_DETAIL) {
      fprintf(stderr, "\n    Adjusted WINDOW Leakage rates (cfm under natural conditions)\n mnth ");
      for (int nc = 0; nc < num_win; nc++) {
        fprintf(stderr, "wn(%2d) ", nc);
      }
      fprintf(stderr, "Total\n");
      for (int m = 1; m <= MONTHS; m++) {
        fprintf(stderr, " %2d ", m);
        for (int nc = 0; nc < num_win; nc++)
          fprintf(stderr, "%7.2f", wn_leak_cfm[nc][m]);
        fprintf(stderr, "%7.2f\n", wn_cfm_tot[m]);
      }
    }
  }
  return;
}

// Given the montly total window and door cfm, compute a CFM door adjustment factor
float door_constrain_cfm_factor(float whole_house_cfm[POST_RETROFIT + 1][MONTHS + 1], 
  float wn_cfm_tot[MONTHS + 1], 
  float dr_cfm_tot[MONTHS + 1]) {

  float factor = 1.0f;

  if (dr_cfm_tot[AVG] > 0.0f) {
    float old = dr_cfm_tot[AVG];
    for (int m = 1; m <= MONTHS; m++) {
      float base_cfm = (wn_cfm_tot[m] + dr_cfm_tot[m]);
      float adj = (MAX_WINDOW_DOOR_PERCENT / 100.0f) * whole_house_cfm[PRE_RETROFIT][m] / base_cfm;
      dr_cfm_tot[m] *= adj < 1.0f ? adj : 1.0;
    }
    dr_cfm_tot[AVG] = 0.0f;
    for (int m = 1; m <= MONTHS; m++)
      dr_cfm_tot[AVG] += dr_cfm_tot[m];
    dr_cfm_tot[AVG] /= 12.0f;
    factor = (old - dr_cfm_tot[AVG]) / old;
  }
  return factor < 1.0f ? factor : 1.0;
}

// Given the montly total window and door cfm, apply adjustment to individual door cfm
void door_constrain_cfm(float whole_house_cfm[POST_RETROFIT + 1][MONTHS + 1], 
  float dr_leak_cfm[NEAT_MAX_WIN][MONTHS + 1], 
  float wn_cfm_tot[MONTHS + 1], 
  float dr_cfm_tot[MONTHS + 1], 
  int num_dor,
  float floor_area) {


  if (dr_cfm_tot[AVG] > 0.0f) {
    if (cmds.debug_level & D_INFILTRATION_DETAIL) {
      fprintf(stderr, "\n   Average initial monthly average DOOR cfm = %7.2f", dr_cfm_tot[AVG]);
      fprintf(stderr, "\n    Raw DOOR Leakage rates (cfm under natural conditions)\n m  ");
      for (int nc = 0; nc < num_dor; nc++) {
        fprintf(stderr, "wn(%2d) ", nc);
      }
      fprintf(stderr, "Door House Percent\n");
      for (int m = 1; m <= MONTHS; m++) {
        fprintf(stderr, "%2d", m);
        for (int nc = 0; nc < num_dor; nc++)
          fprintf(stderr, "%7.2f", dr_leak_cfm[nc][m]);
        fprintf(stderr, "%7.2f%7.2f%6.1f\n", 
          dr_cfm_tot[m], 
          whole_house_cfm[PRE_RETROFIT][m], 
          dr_cfm_tot[m] / whole_house_cfm[PRE_RETROFIT][m] * 100.);
      }
      fprintf(stderr, "\n m  whole_house_cfm[m]     ac    maxper  wnadj\n");
    }
    for (int m = 1; m <= MONTHS; m++) {
      float base_cfm = (wn_cfm_tot[m] + dr_cfm_tot[m]);
      float adj = (MAX_WINDOW_DOOR_PERCENT / 100.0f) * whole_house_cfm[PRE_RETROFIT][m] / base_cfm;
      if (cmds.debug_level & D_INFILTRATION_DETAIL) {
        float ac = (float)(whole_house_cfm[PRE_RETROFIT][m] * 60. / 8. / floor_area);
        fprintf(stderr, "%2d %7.2f %7.2f %7.1f%7.3f \n", m, whole_house_cfm[PRE_RETROFIT][m], ac, MAX_WINDOW_DOOR_PERCENT, adj);
      }
      if (adj < 1.0f) {
        for (int nc = 0; nc < num_dor; nc++) {
          dr_leak_cfm[nc][m] *= adj;
        }
      }
    }
    if (cmds.debug_level & D_INFILTRATION_DETAIL) {
      fprintf(stderr, "\n    Adjusted DOOR Leakage rates (cfm under natural conditions)\n mnth ");
      for (int nc = 0; nc < num_dor; nc++) {
        fprintf(stderr, "dr(%2d) ", nc);
      }
      fprintf(stderr, "Total\n");
      for (int m = 1; m <= MONTHS; m++) {
        fprintf(stderr, " %2d ", m);
        for (int nc = 0; nc < num_dor; nc++)
          fprintf(stderr, "%7.2f", dr_leak_cfm[nc][m]);
        fprintf(stderr, "%7.2f\n", dr_cfm_tot[m]);
      }
    }
  }
  return;
}
