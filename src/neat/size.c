/***************************************************************************
* MODULE:       size.c            CREATED:      February 24, 1999
*
* AUTHOR:       Mike Gettings       Oak Ridge National Laboratory
*               Mark Fishbaugher    mark@fishbaugher.com
*
* MDESC:        equipment sizing routines
****************************************************************************/
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "wa_engine.h"

static int infiltration_house_size(float area) {

  if (area < 900)
    return 0;
  else if (area < 1500)
    return 1;
  else if (area < 2100)
    return 2;
  else if (area >= 2100)
    return 3;

  return 0;
}

// Single or single with bad storm = 0
// Double or single with storm = 1 default
// Double with storm or Low-E = 2

/*******************************************
 * Function name direct_solar_gain
 * Date:  3/27/2019
 * Author: MJF
 *
 * Description:  Solar gain in Btuh/sq.ft from
 * ACCA Manual J 1986 Table 3
 * MJF rewrite of Gettings original indexed array
 * Includes both direct solar and total horizontal and
 * diffuse gains depending on solar orientation parameter
 * ***************************************/
static float solar_gain(enum SOLAR_ORIENTATION orient, enum GLAZINGTYPE glazing) {

  switch (orient) {
  case SOLAR_NORTH:
    switch (glazing) {
    case SINGLE:
    case SINGLE_W_BAD_STORM:
    case SINGLE_W_WOOD_STORM:
    case SINGLE_W_METAL_STORM:
    case DOUBLE_GLAZED:
    case DOUBLE_GLAZED_LOWE:
    case DOUBLE_GLAZED_W_STORM:
      return 0.0f;
    }
  case SOLAR_EAST:
    switch (glazing) {
    case SINGLE:
    case SINGLE_W_BAD_STORM:
      return 58.0f;
    case SINGLE_W_WOOD_STORM:
    case SINGLE_W_METAL_STORM:
    case DOUBLE_GLAZED:
      return 49.0f;
    case DOUBLE_GLAZED_LOWE:
    case DOUBLE_GLAZED_W_STORM:
      return 45.0f;
    }
  case SOLAR_SOUTH:
    switch (glazing) {
    case SINGLE:
    case SINGLE_W_BAD_STORM:
      return 17.0f;
    case SINGLE_W_WOOD_STORM:
    case SINGLE_W_METAL_STORM:
    case DOUBLE_GLAZED:
      return 15.0f;
    case DOUBLE_GLAZED_LOWE:
    case DOUBLE_GLAZED_W_STORM:
      return 13.0f;
    }
  case SOLAR_WEST:
    switch (glazing) {
    case SINGLE:
    case SINGLE_W_BAD_STORM:
      return 58.0f;
    case SINGLE_W_WOOD_STORM:
    case SINGLE_W_METAL_STORM:
    case DOUBLE_GLAZED:
      return 49.0f;
    case DOUBLE_GLAZED_LOWE:
    case DOUBLE_GLAZED_W_STORM:
      return 45.0f;
    }
  case SOLAR_HORIZONTAL_TOTAL:
    switch (glazing) {
    case SINGLE:
    case SINGLE_W_BAD_STORM:
      return 137.0f;
    case SINGLE_W_WOOD_STORM:
    case SINGLE_W_METAL_STORM:
    case DOUBLE_GLAZED:
      return 120.0f;
    case DOUBLE_GLAZED_LOWE:
    case DOUBLE_GLAZED_W_STORM:
      return 110.0f;
    }
  case SOLAR_DIFFUSE:
    switch (glazing) {
    case SINGLE:
    case SINGLE_W_BAD_STORM:
    case SINGLE_W_WOOD_STORM:
    case SINGLE_W_METAL_STORM:
    case DOUBLE_GLAZED:
    case DOUBLE_GLAZED_LOWE:
    case DOUBLE_GLAZED_W_STORM:
      return 15.0f;
    }
  }
  ASSERT(FALSE, sprintf(msg, "No direct solar gain for orient: %d and glazing: %d", orient, glazing));
}

// static float wnmju[5][3] =     // manual J window U values for 5
//     {{0.990f, 1.155f, 1.045f}, // glazing types and 3 frame types
//      {0.475f, 0.650f, 0.525f},
//      {0.551f, 0.725f, 0.609f},
//      {0.341f, 0.490f, 0.385f},  // special case of double pane with storm NOT used in NEAT but IS used in Manual J
//      {0.361f, 0.475f, 0.399f}}; // Low-E values corrected for Version 8.9

// static float drmju[2][5] =                      // manual j door U values
//     {{0.345f, 0.305f, 0.342f, 0.525f, 0.385f},  // for 2 door conditions
//      {0.560f, 0.460f, 0.530f, 1.045f, 0.609f}}; //  5 door types (2 for SGDs)

static float mjach[4][4] =     // manual j infiltration rates, winter
    {{0.4f, 0.4f, 0.3f, 0.3f}, // only mjach[1][n] is used
     {1.2f, 1.0f, 0.8f, 0.7f},
     {1.2f, 1.0f, 0.8f, 0.7f},
     {2.2f, 1.6f, 1.2f, 1.0f}};

static float mjachs[4][4] =    // manual j infiltration rates, summer
    {{0.2f, 0.2f, 0.2f, 0.2f}, // only mjach[1][n] is used
     {0.5f, 0.5f, 0.4f, 0.4f},
     {0.5f, 0.5f, 0.4f, 0.4f},
     {0.8f, 0.7f, 0.6f, 0.5f}};

static float finfactors[3][4] = // manual j wind exposure and # stories factor,
    {{0.4f, 0.5f, 0.6f, 0.6f},  //    summer.  [exposure][#stories].
     {0.6f, 0.7f, 0.8f, 0.8f},  //    only infactors[1][n] is used (average)
     {0.8f, 0.9f, 1.0f, 1.0f}}; //   Table A5-1

/*static float finfactorw[3][4] = // manual j wind exposure and # stories factor,
   {{1.3f,1.7f,2.0f,2.0f},      //    winter.  [exposure][#stories].
    {1.6f,2.0f,2.3f,2.3f},      //    only infactorw[1][n] is used (average)
    {1.9f,2.3f,2.6f,2.6f}};     //   Table A5-1
*/
static float htmwl[POST_RETROFIT +1][NEAT_MAX_WAL]; // wall peak heat load
static float htmwn[POST_RETROFIT +1][NEAT_MAX_WIN]; // window heat loads
static float htmdr[POST_RETROFIT +1][NEAT_MAX_DOR]; // ditto for doors
static float htmua[POST_RETROFIT +1][NEAT_MAX_UAS]; // yea you guessed it -- attic areas
static float htmsb[POST_RETROFIT +1][NEAT_MAX_FND]; // sub basements
static float htminf[POST_RETROFIT +1];              // infiltration

static float clmwl[POST_RETROFIT +1][NEAT_MAX_WAL];
static float clmwn[POST_RETROFIT +1][NEAT_MAX_WIN];
static float clmdr[POST_RETROFIT +1][NEAT_MAX_DOR];
static float clmua[POST_RETROFIT +1][NEAT_MAX_UAS];
static float clmsb[POST_RETROFIT +1][NEAT_MAX_FND];
static float clminf[POST_RETROFIT +1];
static float clduct[POST_RETROFIT +1];

static float dtd;            // delta T indoor - heating design temperature
static float volume;         // interior volume of the house in ft^3
static float frductlos_cl;   // fractional duct loss value for cooling
static float fpeople;        // Gain from people according to NEAT's prescription
static float fappliances;    // Gain from appliances using Manual J's value
static float flatent_occ;    // Latent loads from occupants
static float flatent[POST_RETROFIT +1];     // Pre and Post latent load from infiltration
static float flatent_tot[POST_RETROFIT +1]; // Pre and Post total latent load

void sizing_heating(int rt) {
  float wlr, temp, areaag, areabg, fndu;
  // int fr, gt, dt, dc, nc;
  // int dt, dc, nc;
  int nc;
  // char ch;
  float frductlos = 0.0f; // fractional duct loss value, heating
  // int gtt[] = {0, 1, 1, 2, 0, 4, 3}; // Added 10/11, Version 89

  nir->htmt[rt] = 0.0;
  dtd = 70.0f - cwd->heating_design_temp;
  if (dtd < 0.0f)
    dtd = 0.0f;
  if (ndi->htg[PRIMARY].system_type == HE_FIXED_ELECTRIC_RESISTANCE || ndi->htg[PRIMARY].system_type == HE_PORT_ELECTRIC_RESISTANCE ||
      ndi->htg[PRIMARY].system_type == HE_UNVENTED_SPACE_HEATER || ndi->htg[PRIMARY].system_type == HE_VENTED_SPACE_HEATER)
    frductlos = 0.;
  else if (ndi->htg[PRIMARY].system_type == HE_HOT_WATER_BOILER)
    frductlos = 0.01f; /* DOE-2 value */
  else if (ndi->htg[PRIMARY].system_type == HE_STEAM_BOILER)
    frductlos = 0.02f; /* Estimated from above & dT */
                       //  else if(rt==0 && duct_lngth>10.) frductlos=0.2f;
  else if (rt == PRE_RETROFIT && nir->ductarea > 40.)
    frductlos = 0.2f;
  else
    frductlos = 0.15f;

  /*  Note final value of htmt must be multiplied by the design temp. diff., dtd */
  /*  Walls */
  for (nc = 0; nc < ndi->num_wal; nc++) {
    // #314
    // if (ndi->wal[nc].exposure == EX_ATTIC) { //  Kneewalls handled by attics
    //   htmwl[rt][nc] = 0.0f;
    //   continue;
    // }
    wlr = ndi->wal[nc].exist_r;
    if (ndi->wal[nc].wall_type == LO_BALLOON || ndi->wal[nc].wall_type == LO_PLATFORM ||
        ndi->wal[nc].wall_type == LO_OTHER) { // frame wall Table 2-12
      if (wlr <= 2.6)
        htmwl[rt][nc] = 0.2714f * (float)exp(-0.2789 * (wlr)) * ndi->wal[nc].area;
      else
        htmwl[rt][nc] = 0.1505f * (float)exp(-0.04908 * (wlr)) * ndi->wal[nc].area;
    } else { /* Masonry wall, Table 2-14.  Altered 10/04 */
      if (ndi->wal[nc].ext_type != EX_BRICK) {
        htmwl[rt][nc] = 1.0f / (0.994092f * wlr + 1.983091f) * ndi->wal[nc].area;
      } else {
        htmwl[rt][nc] = 1.0f / (0.988394f * wlr + 2.553782f) * ndi->wal[nc].area;
      }
    }
    nir->htmt[rt] += htmwl[rt][nc];
  }

  /*  Windows, Tables 2-2, 2-3 and 2-4 */

  for (nc = 0; nc < ndi->num_win; nc++) {
    htmwn[rt][nc] = window_u_value(ndi->win[nc].frame_type, ndi->win[nc].glazing_type, MANUAL_J_HEAT) * ndi->win[nc].area_gross;
    nir->htmt[rt] += htmwn[rt][nc];
  }

  /*  Doors, Tables 2-10 and 2-11 */

  for (nc = 0; nc < ndi->num_dor; nc++) {
    // dt = dr_type[nc] - 1;
    // dc = dr_cond[nc] - 1;
    // if (dc > 0)
    //   dc = 1;       // inadequate storm door or missing
    // htmdr[rt][nc] = drmju[dc][dt] * dr[nc].area;
    htmdr[rt][nc] = door_u_value(ndi->dor[nc].door_type, ndi->dor[nc].condition, MANUAL_J_HEAT) * ndi->dor[nc].area;
    nir->htmt[rt] += htmdr[rt][nc];
  }

  /*  Roof/Ceilings */

  for (nc = 0; nc < ndi->num_uas; nc++) {
    temp = ndi->uas[nc].r_value;
    if (ndi->uas[nc].attic_type != UAS_CATHEDRAL) {
      if (temp >= 23)
        temp = 0.996f * temp + 0.555f; /* roof/attic, Tab 2-16 */
      else {
        ASSERT((temp + 1.683028), sprintf(msg, "Need non zero r value"));
        temp = 1.0f / (1.0f / 206.9193f + 1.0f / (temp + 1.683028f));
      }
    } else {
      if (temp >= 15)
        temp = 0.79365f * temp + 4.62963f; /* cthdrl ceiling, Tab 2-18 */
      else {
        ASSERT((temp + 3.266993), sprintf(msg, "Need non zero r value"));
        temp = 1.0f / (1.0f / 524.068831f + 1.0f / (temp + 2.266993f));
      }
    }

    ASSERT(temp, sprintf(msg, "Need non zero r value"));
    htmua[rt][nc] = 1.0f / temp * ndi->uas[nc].area;
    nir->htmt[rt] += htmua[rt][nc];
  }

  /*  Foundations */

  for (nc = 0; nc < ndi->num_fnd; nc++) {
    switch (ndi->fnd[nc].space_type) {
      case CONDITIONED:
        wlr = ndi->fnd[nc].wall_ins_r;

        ASSERT(ndi->fnd[nc].wall_height, sprintf(msg, "Need non zero wall height"));
        areabg = ndi->fnd[nc].wall_area * ndi->fnd[nc].below_grade_wall_height / ndi->fnd[nc].wall_height; // Tab 2-15
        areaag = ndi->fnd[nc].wall_area - areabg;
        if (ndi->fnd[nc].below_grade_wall_height > 5.) {  // Wall extends 5 or more feet below grade
          htmsb[rt][nc] = 1.0f / (1.09f * wlr + 11.45f) * areabg;
        } else {    // Wall extends <5 feet below grade 
          htmsb[rt][nc] = 1.0f / (1.12f * wlr + 7.83f) * areabg;
        }
        if (wlr <= 5.0) // Above grade portion Tab 2-14
          htmsb[rt][nc] += (0.00564f * wlr * wlr - 0.1014f * wlr + 0.51f) * areaag;
        else
          htmsb[rt][nc] += (0.00054f * wlr * wlr - 0.01979f * wlr + 0.22946f) * areaag;
        htmsb[rt][nc] += (0.024f * ndi->fnd[nc].area) + ndi->fnd[nc].ua_value_basement_sill;
        break;

      case NON_CONDITIONED:
        fndu = 0.801f * ndi->fnd[nc].flr_ins_r + 4.081f; // Tab 2-20
        fndu = 1.0f / fndu;
        fndu /= 2.0; // Tab 2-19
        htmsb[rt][nc] = fndu * ndi->fnd[nc].area;
        break;

      case VENTED_NON_CONTITIONED:
        fndu = 0.801f * ndi->fnd[nc].flr_ins_r + 4.081f; // Tab 2-20
        fndu = 1.0f / fndu;
        htmsb[rt][nc] = fndu * ndi->fnd[nc].area;
        break;

      case UNINTENTIONALLY_CONDITIONED:
        fndu = 0.801f * ndi->fnd[nc].flr_ins_r + 4.081f; // Tab 2-20
        fndu = 1.0f / fndu;
        fndu /= 2.0; // Tab 2-19
        htmsb[rt][nc] = (fndu * ndi->fnd[nc].area) + ndi->fnd[nc].ua_value_basement_sill;
        break;

      case UNINSULATED_SLAB:
        htmsb[rt][nc] = 0.810f * ndi->fnd[nc].perim_length; // Tab 2-22
      break;

      case INSULATED_SLAB:
        htmsb[rt][nc] = 0.410f * ndi->fnd[nc].perim_length; // Tab 2-22
        break;

      case EXPOSED_FLOOR_UNCLOSED:
        fndu = 0.801f * ndi->fnd[nc].flr_ins_r + 4.081f; // Tab 2-20
        fndu = 1.0f / fndu;
        htmsb[rt][nc] = fndu * ndi->fnd[nc].area;
        break;

      case EXPOSED_FLOOR_CLOSED:
        fndu = 0.801f * ndi->fnd[nc].flr_ins_r + 4.081f;    // Tab 2-20
        fndu += EXTERIOR_FILM_RESISTANCE_R + SHEATHING_R;   // # 232
        fndu = 1.0f / fndu;
        htmsb[rt][nc] = fndu * ndi->fnd[nc].area;
        break;

    }
    nir->htmt[rt] += htmsb[rt][nc];
  }

  /* Infiltration */

  if (rt == POST_RETROFIT) {
    htminf[rt] = nir->whole_house_cfm[POST_RETROFIT][JANUARY] * 1.1f; /* Use January's cfm for sizing */

    ASSERT(dtd, sprintf(msg, "Need non zero cfm"));
    if ((nir->infiltration_treatment == INF_DEFAULT || 
         nir->infiltration_treatment == INF_INCIDENTAL_COST) && 
      htminf[PRE_RETROFIT] / dtd < htminf[POST_RETROFIT]) {
      /* Set pre = post value if none specified & manj gives lower for pre.*/
      nir->htmt[PRE_RETROFIT] -= htminf[PRE_RETROFIT];
      htminf[PRE_RETROFIT] = htminf[POST_RETROFIT] * dtd;
      nir->htmt[PRE_RETROFIT] += htminf[PRE_RETROFIT];
    }
  }  else { /* Pre-Retrofit */
    if (nir->infiltration_treatment ==  INF_NO_COST_COMPUTE_SAVINGS_ONLY ||
        nir->infiltration_treatment ==  INF_FULL_MEASURE)
      htminf[rt] = nir->whole_house_cfm[PRE_RETROFIT][JANUARY] * 1.1f;
    else
      htminf[rt] = 8.8f * ndi->gnl.floor_area * mjach[1][infiltration_house_size(ndi->gnl.floor_area)] / 60.0f;
  }
  /* Manual J average infiltration rate */

  nir->htmt[rt] += htminf[rt];

  /* Print results of Manual J computation by component */

  /*  manjout = fopen ("manj.out","w"); */

  /*  fprintf(manjout,"\nComponent    Area/Volume    Btu/hr\n\n"); */

  for (nc = 0; nc < ndi->num_wal; nc++) {
    htmwl[rt][nc] *= dtd;
  }
  for (nc = 0; nc < ndi->num_win; nc++) {
    htmwn[rt][nc] *= dtd;
  }
  for (nc = 0; nc < ndi->num_dor; nc++) {
    htmdr[rt][nc] *= dtd;
  }
  for (nc = 0; nc < ndi->num_uas; nc++) {
    htmua[rt][nc] *= dtd;
  }
  for (nc = 0; nc < ndi->num_fnd; nc++) {
    htmsb[rt][nc] *= dtd;
  }
  volume = ndi->gnl.floor_area * 8.0f;
  htminf[rt] *= dtd;
  nir->htmt[rt] *= dtd; // UA delta T
  nir->htduct[rt] = frductlos * nir->htmt[rt];
  nir->htmt[rt] += nir->htduct[rt];
  /* fprintf(manjout,"     Inf%10.0f%12.0f\n",volume,htminf[rt]);
   * fprintf(manjout,"\n   Total%22.0f\n",nir->htmt[rt]); */

  /*  fclose(manjout); */
}

/***********************************************************
  Cooling sizing routine. MBG - 10/04
***********************************************************/

void sizing_cooling(int rt) {
  // int nc, dt, dc;
  int nc;
  // char ch;
  // int wngt; // Windows
  // int dtsa;
  float wnsd, wn_U_value;
  // float wn_diffuse = 15.0f;   // solar gain (Btuh/sqft) for diffuse radiation
  float temp, wlr, fndu, areabg, areaag, etd;
  float color = 24.0f; // Dark roof; 16 for light; 20 for average
  float cfmmj, L2;     // Temporary constants for infiltration calculations

  fpeople = 0.0f;
  fappliances = 0.0f;
  flatent_occ = 0.0f;

  dtd = cwd->cooling_design_temp - 78.0f;
  if (dtd < 0.0f)
    dtd = 0.0f;
  nir->clmt[rt] = 0.0f;

  frductlos_cl = 0.0f;
  for (nc = 0; nc < ndi->num_clg; nc++) {
    if (ndi->clg[nc].system_type == CET_CENTRAL || ndi->clg[nc].system_type == CET_HEATPUMP)
      frductlos_cl = 0.1f;
  }

  /*  Walls Table 4 */

  for (nc = 0; nc < ndi->num_wal; nc++) {
    // #314
    // if (ndi->wal[nc].exposure == EX_ATTIC) { //  Kneewalls handled by attics
    //   clmwl[rt][nc] = 0.0f;
    //   continue;
    // }

    wlr = ndi->wal[nc].exist_r;

    /* frame wall, Table 4-12,13 */

    if (ndi->wal[nc].wall_type == LO_BALLOON || ndi->wal[nc].wall_type == LO_PLATFORM || ndi->wal[nc].wall_type == LO_OTHER) {
      etd = dtd + 3.6f;                           // Medium weight house
      if (ndi->wal[nc].exposure == EX_BUFFERED) { //  Buffered wall
        etd = dtd - 5.0f;
        if (etd < 0.0f)
          etd = 0.0f;
      }

      if (wlr <= 2.6)
        clmwl[rt][nc] = etd * 0.2714f * (float)exp(-0.2789 * (wlr)) * ndi->wal[nc].area;

      else
        clmwl[rt][nc] = etd * 0.1505f * (float)exp(-0.04908 * (wlr)) * ndi->wal[nc].area;
    }

    else { /* Masonry wall, Table 4-14 */

      etd = dtd - 3.7f;                           // Medium weight house
      if (ndi->wal[nc].exposure == EX_BUFFERED) { //  Buffered wall
        etd = dtd - 11.5f;
        if (etd < 0.0f)
          etd = 0.0f;
      }

      if (ndi->wal[nc].ext_type != EX_BRICK) {
        clmwl[rt][nc] = etd / (0.994092f * wlr + 1.983091f) * ndi->wal[nc].area;
      }

      else { // Brick of Stone faced
        clmwl[rt][nc] = etd / (0.988394f * wlr + 2.553782f) * ndi->wal[nc].area;
      }
    }

    nir->clmt[rt] += clmwl[rt][nc];
  }

  /*  Windows, Table 4-3  */

  for (nc = 0; nc < ndi->num_win; nc++) {

    wnsd = 1.0f - ndi->win[nc].shade_factor_summer * nir->wn_sunscrn[COOLING][nc];

    clmwn[rt][nc] = (window_u_value(ndi->win[nc].frame_type, ndi->win[nc].glazing_type, MANUAL_J_COOL) * dtd +
                     solar_gain(SOLAR_DIFFUSE, ndi->win[nc].glazing_type) +
                     solar_gain(ndi->win[nc].solar_orient, ndi->win[nc].glazing_type) * (1.0f - wnsd)) *
                    ndi->win[nc].area_gross;

    nir->clmt[rt] += clmwn[rt][nc];
  }

  /*  Doors, Items, Tab 4-10 and 4-11  */

  for (nc = 0; nc < ndi->num_dor; nc++) {
    switch (ndi->dor[nc].door_type) {
    case SINGLE_SLIDING_GLASS:
      wn_U_value = 0.8f;
      clmdr[rt][nc] =
          (wn_U_value * dtd + solar_gain(SOLAR_DIFFUSE, SINGLE) + solar_gain(ndi->dor[nc].solar_orient, SINGLE) * 0.7f) *
          ndi->dor[nc].area; // Assume 30% shade
      break;
    case DOUBLE_SLIDING_GLASS:
      wn_U_value = 0.4f;
      clmdr[rt][nc] = (wn_U_value * dtd + solar_gain(SOLAR_DIFFUSE, DOUBLE_GLAZED) +
                       solar_gain(ndi->dor[nc].solar_orient, DOUBLE_GLAZED) * 0.7f) *
                      ndi->dor[nc].area; // Assume 30% shade
      break;
    default:            // opaque doors
      etd = dtd + 3.6f; // Medium weight house
      clmdr[rt][nc] = etd * door_u_value(ndi->dor[nc].door_type, ndi->dor[nc].condition, MANUAL_J_COOL) * ndi->dor[nc].area;
      break;
    }
    nir->clmt[rt] += clmdr[rt][nc];
  }

  /*  Roofs / Ceilings  */

  for (nc = 0; nc < ndi->num_uas; nc++) {
    etd = dtd + color;
    temp = ndi->uas[nc].r_value;
    if (ndi->uas[nc].attic_type != UAS_CATHEDRAL) {
      if (temp >= 23.0f)
        temp = 1.0216f * temp - 0.400373f; /* roof/attic, Tab 4-16 */
      else {
        ASSERT((temp + 2.312784f), sprintf(msg, "Need non zero r value"));
        temp = 1.0f / (1.0f / 216.4161f + 1.0f / (temp + 2.312784f));
      }
    } else { /* cthdrl ceiling, */
      if (temp >= 21.0f)
        temp = 0.79365f * temp + 4.62963f;
      else {
        ASSERT((temp + 3.522904f), sprintf(msg, "Need non zero r value"));
        temp = 1.0f / (1.0f / 318.1424f + 1.0f / (temp + 3.522904f));
      }
    }

    ASSERT(temp, sprintf(msg, "Need non zero r value"));
    clmua[rt][nc] = etd / temp * ndi->uas[nc].area;
    nir->clmt[rt] += clmua[rt][nc];
  }

 /*  Foundations */

  for (nc = 0; nc < ndi->num_fnd; nc++) {
    switch (ndi->fnd[nc].space_type) {
      case CONDITIONED:
        etd = dtd - 3.7f;
        wlr = ndi->fnd[nc].wall_ins_r;

        ASSERT(ndi->fnd[nc].wall_height, sprintf(msg, "Need non zero foundation wall height"));
        areabg = ndi->fnd[nc].wall_area * ndi->fnd[nc].below_grade_wall_height / ndi->fnd[nc].wall_height; // Tab 4-14
        areaag = ndi->fnd[nc].wall_area - areabg;
        clmsb[rt][nc] = etd * ((1 / (0.994092f * wlr + 1.983091f) * areaag) + ndi->fnd[nc].ua_value_basement_sill);
        break;

      case NON_CONDITIONED:
      case UNINTENTIONALLY_CONDITIONED:
      case UNINSULATED_SLAB:
      case INSULATED_SLAB:
        clmsb[rt][nc] = 0.0f;
        break;

      case VENTED_NON_CONTITIONED:
      case EXPOSED_FLOOR_UNCLOSED:
      case EXPOSED_FLOOR_CLOSED:
        fndu = 0.822736f * ndi->fnd[nc].flr_ins_r + 3.309243f; /* Tab 4-20 */
        ASSERT(fndu, sprintf(msg, "Need non zero foundation uvalue"));
        fndu = 1.0f / fndu;
        etd = dtd - 5.0f;
        clmsb[rt][nc] = etd * fndu * ndi->fnd[nc].area;
        break;
    }
    nir->clmt[rt] += clmsb[rt][nc];
  }

  /* Infiltration */

  if (rt == PRE_RETROFIT) {

    if (nir->infiltration_treatment ==  INF_NO_COST_COMPUTE_SAVINGS_ONLY ||
        nir->infiltration_treatment ==  INF_FULL_MEASURE) {
      L2 = 0.6999f * ndi->inf.post_duct_seal_cfm * (float)POWC(ndi->inf.post_duct_seal_pa, -0.65f);
      cfmmj = 0.01333f * ndi->gnl.floor_area + L2 * finfactors[1][(int)(ndi->gnl.no_cond_stories - 0.6f)];
      clminf[rt] = cfmmj * 1.1f;
    }

    else {
      clminf[rt] = 8.8f * ndi->gnl.floor_area * mjachs[1][infiltration_house_size(ndi->gnl.floor_area)] / 60.0f;
      /* 8.8 = 1.1 (what manj uses for 1.08) * 8 ft ceiling height */
      cfmmj = clminf[rt] / 1.1f;
    }
  }

  /* Post-Retrofit */
  else {
    L2 = 0.6999f * ndi->inf.post_inf_cfm * (float)POWC(ndi->inf.post_inf_pa, -0.65f);
    cfmmj = 0.01333f * ndi->gnl.floor_area + L2 * finfactors[1][(int)(ndi->gnl.no_cond_stories - 0.6f)];
    clminf[rt] = cfmmj * 1.1f;
  }

  clminf[rt] *= dtd;
  nir->clmt[rt] += clminf[rt];

  /* Latent Loads  */

  flatent[rt] = nir->latentload[rt][JULY] / 24.0f / 31.0f;
  flatent[rt] = get_latent_infil_load(cwd->cooling_design_temp, cwd->cooling_design_wetbulb_temp, cfmmj);
  if (flatent[rt] < 0.0f)
    flatent[rt] = 0.0f;

  flatent_occ = ndi->gnl.avg_no_occupants * 230.0f;

  flatent_tot[rt] = flatent[rt] + flatent_occ;

  /* If pre rates are not specified, prevent post inf loads from being greater
   * than pre inf loads, i.e. set pre = post then re-adjust totals  */

  if (rt == POST_RETROFIT) {
    if ((nir->infiltration_treatment == INF_DEFAULT || 
         nir->infiltration_treatment == INF_INCIDENTAL_COST) && 
         clminf[PRE_RETROFIT] / dtd < clminf[POST_RETROFIT]) {
      nir->clmt[PRE_RETROFIT] -= clminf[PRE_RETROFIT];
      clminf[PRE_RETROFIT] = clminf[POST_RETROFIT];
      nir->clmt[PRE_RETROFIT] += clminf[PRE_RETROFIT];
      flatent_tot[PRE_RETROFIT] -= flatent[PRE_RETROFIT];
      flatent[PRE_RETROFIT] = flatent[POST_RETROFIT];
      flatent_tot[PRE_RETROFIT] += flatent[PRE_RETROFIT];
    }
  }

  /* Internals - People and Appliances  */

  if (ndi->gnl.avg_no_occupants < 1.5f)
    fpeople = 552.0f / 2.0f;
  else if (ndi->gnl.avg_no_occupants < 2.5f)
    fpeople = 552.0f;
  else
    fpeople = 552.0f + 224.0f * (ndi->gnl.avg_no_occupants - 2.0f);

  fappliances = 1200.0f;

  nir->clmt[rt] += fpeople + fappliances;
}

/*******************
 *******************
 This routine prints the results of the manualj calculation */

// removed references to mode and eliminated the mode parameter MJF 2/99

void report_manual_j(FILE *fp) {
  int nc, cnt;

  ASSERT(nor, sprintf(msg, "The NEAT Output Results are not found"));

  if (fp)
    fprintf(fp, "\n  APPROXIMATE MANUAL J COMPONENT CONTRIBUTIONS TO PEAK HEATING LOADS\n");

  if (fp)
    fprintf(fp, "\n Component    Area or    Pre-Retrofit   Post-Retrofit");
  if (fp)
    fprintf(fp, "\n             Volume(Inf)     Btu/h          Btu/h\n\n");

  for (nc = 0; nc < ndi->num_wal; nc++) {
    // #314
    // if (ndi->wal[nc].exposure == EX_ATTIC)  //  Kneewalls handled by attics
    //   continue;
    if (fp)
      fprintf(fp, "  %6s%10.0f%14.0f%17.0f\n", ndi->wal[nc].code, ndi->wal[nc].area, htmwl[PRE_RETROFIT][nc], htmwl[POST_RETROFIT][nc]);
    cnt = nor->num_manj;
    nor->manj[cnt].index = cnt;
    STRCPY(nor->manj[cnt].heatcool, "heat");
    STRCPY(nor->manj[cnt].type, "Wall (Btu/h)");
    STRCPY(nor->manj[cnt].name, ndi->wal[nc].code);
    nor->manj[cnt].area_vol = ndi->wal[nc].area;
    nor->manj[cnt].pre_load = htmwl[PRE_RETROFIT][nc];
    nor->manj[cnt].post_load = htmwl[POST_RETROFIT][nc];
    nor->num_manj++;
  }

  for (nc = 0; nc < ndi->num_win; nc++) {
    if (fp)
      fprintf(fp, "  %6s%10.0f%14.0f%17.0f\n", ndi->win[nc].code, ndi->win[nc].area_gross, htmwn[PRE_RETROFIT][nc], htmwn[POST_RETROFIT][nc]);
    cnt = nor->num_manj;
    nor->manj[cnt].index = cnt;
    STRCPY(nor->manj[cnt].heatcool, "heat");
    STRCPY(nor->manj[cnt].type, "Window (Btu/h)");
    STRCPY(nor->manj[cnt].name, ndi->win[nc].code);
    nor->manj[cnt].area_vol = ndi->win[nc].area_gross;
    nor->manj[cnt].pre_load = htmwn[PRE_RETROFIT][nc];
    nor->manj[cnt].post_load = htmwn[POST_RETROFIT][nc];
    nor->num_manj++;
  }

  for (nc = 0; nc < ndi->num_dor; nc++) {
    if (fp)
      fprintf(fp, "  %6s%10.0f%14.0f%17.0f\n", ndi->dor[nc].code, ndi->dor[nc].area, htmdr[PRE_RETROFIT][nc], htmdr[POST_RETROFIT][nc]);
    cnt = nor->num_manj;
    nor->manj[cnt].index = cnt;
    STRCPY(nor->manj[cnt].heatcool, "heat");
    STRCPY(nor->manj[cnt].type, "Door (Btu/h)");
    STRCPY(nor->manj[cnt].name, ndi->dor[nc].code);
    nor->manj[cnt].area_vol = ndi->dor[nc].area;
    nor->manj[cnt].pre_load = htmdr[PRE_RETROFIT][nc];
    nor->manj[cnt].post_load = htmdr[POST_RETROFIT][nc];
    nor->num_manj++;
  }

  for (nc = 0; nc < ndi->num_uas; nc++) {
    if (fp)
      fprintf(fp, "  %6s%10.0f%14.0f%17.0f\n", ndi->uas[nc].code, ndi->uas[nc].area, htmua[PRE_RETROFIT][nc], htmua[POST_RETROFIT][nc]);
    cnt = nor->num_manj;
    nor->manj[cnt].index = cnt;
    STRCPY(nor->manj[cnt].heatcool, "heat");
    STRCPY(nor->manj[cnt].type, "Attic (Btu/h)");
    STRCPY(nor->manj[cnt].name, ndi->uas[nc].code);
    nor->manj[cnt].area_vol = ndi->uas[nc].area;
    nor->manj[cnt].pre_load = htmua[PRE_RETROFIT][nc];
    nor->manj[cnt].post_load = htmua[POST_RETROFIT][nc];
    nor->num_manj++;
  }

  for (nc = 0; nc < ndi->num_fnd; nc++) {
    if (fp)
      fprintf(fp, "  %6s%10.0f%14.0f%17.0f\n", ndi->fnd[nc].code, ndi->fnd[nc].area, htmsb[PRE_RETROFIT][nc], htmsb[POST_RETROFIT][nc]);
    int cnt = nor->num_manj;
    nor->manj[cnt].index = cnt;
    STRCPY(nor->manj[cnt].heatcool, "heat");
    STRCPY(nor->manj[cnt].type, "Foundation (Btu/h)");
    STRCPY(nor->manj[cnt].name, ndi->fnd[nc].code);
    nor->manj[cnt].area_vol = ndi->fnd[nc].area;
    nor->manj[cnt].pre_load = htmsb[PRE_RETROFIT][nc];
    nor->manj[cnt].post_load = htmsb[POST_RETROFIT][nc];
    nor->num_manj++;
  }

  volume = ndi->gnl.floor_area * 8.0f;
  if (fp)
    fprintf(fp, "     Inf%10.0f%14.0f%17.0f\n", volume, htminf[PRE_RETROFIT], htminf[POST_RETROFIT]);
  cnt = nor->num_manj;
  nor->manj[cnt].index = cnt;
  STRCPY(nor->manj[cnt].heatcool, "heat");
  STRCPY(nor->manj[cnt].type, "Infiltration (Btu/h)");
  STRCPY(nor->manj[cnt].name, "Inf");
  nor->manj[cnt].area_vol = volume;
  nor->manj[cnt].pre_load = htminf[PRE_RETROFIT];
  nor->manj[cnt].post_load = htminf[POST_RETROFIT];
  nor->num_manj++;

  if (fp)
    fprintf(fp, "\n Total heat loss%16.0f%17.0f\n", nir->htmt[PRE_RETROFIT] - nir->htduct[PRE_RETROFIT], nir->htmt[POST_RETROFIT] - nir->htduct[POST_RETROFIT]);
  cnt = nor->num_manj;
  nor->manj[cnt].index = cnt;
  STRCPY(nor->manj[cnt].heatcool, "heat");
  STRCPY(nor->manj[cnt].type, "Total heat loss (Btu/h)");
  STRCPY(nor->manj[cnt].name, "Tot");
  nor->manj[cnt].area_vol = 0;
  nor->manj[cnt].pre_load = nir->htmt[PRE_RETROFIT] - nir->htduct[PRE_RETROFIT];
  nor->manj[cnt].post_load = nir->htmt[POST_RETROFIT] - nir->htduct[POST_RETROFIT];
  nor->num_manj++;

  if (fp)
    fprintf(fp, "  Duct loss%21.0f%17.0f\n", nir->htduct[PRE_RETROFIT], nir->htduct[POST_RETROFIT]);
  cnt = nor->num_manj;
  nor->manj[cnt].index = cnt;
  STRCPY(nor->manj[cnt].heatcool, "heat");
  STRCPY(nor->manj[cnt].type, "Duct loss (Btu/h)");
  STRCPY(nor->manj[cnt].name, "Duct");
  nor->manj[cnt].area_vol = nir->ductarea;
  nor->manj[cnt].pre_load = nir->htduct[PRE_RETROFIT];
  nor->manj[cnt].post_load = nir->htduct[POST_RETROFIT];
  nor->num_manj++;

  if (fp)
    fprintf(fp, "\n Output required%16.0f%17.0f\n", nir->htmt[PRE_RETROFIT], nir->htmt[POST_RETROFIT]);
  cnt = nor->num_manj;
  nor->manj[cnt].index = cnt;
  STRCPY(nor->manj[cnt].heatcool, "heat");
  STRCPY(nor->manj[cnt].type, "Output required (Btu/h)");
  STRCPY(nor->manj[cnt].name, "Output");
  nor->manj[cnt].area_vol = 0;
  nor->manj[cnt].pre_load = nir->htmt[PRE_RETROFIT];
  nor->manj[cnt].post_load = nir->htmt[POST_RETROFIT];
  nor->num_manj++;

  // that marks the end of the HEATING MANUALJ report, now
  // for the COOLING secgion

  if (fp)
    fprintf(fp, "\n\n");
  if (fp)
    fprintf(fp, "\n  APPROXIMATE MANUAL J COMPONENT CONTRIBUTIONS TO PEAK COOLING LOADS\n");

  if (fp)
    fprintf(fp, "\n Component    Area or    Pre-Retrofit   Post-Retrofit");
  if (fp)
    fprintf(fp, "\n             Volume(Inf)     Btu/h          Btu/h\n\n");

  for (nc = 0; nc < ndi->num_wal; nc++) {
    // #314
    // if (ndi->wal[nc].exposure == EX_ATTIC) //  Kneewalls handled by attics
    //   continue;
    if (fp)
      fprintf(fp, "  %6s%10.0f%14.0f%17.0f\n", ndi->wal[nc].code, ndi->wal[nc].area, clmwl[PRE_RETROFIT][nc], clmwl[POST_RETROFIT][nc]);
    cnt = nor->num_manj;
    nor->manj[cnt].index = cnt;
    STRCPY(nor->manj[cnt].heatcool, "cool");
    STRCPY(nor->manj[cnt].type, "Wall (Btu/h)");
    STRCPY(nor->manj[cnt].name, ndi->wal[nc].code);
    nor->manj[cnt].area_vol = ndi->wal[nc].area;
    nor->manj[cnt].pre_load = clmwl[PRE_RETROFIT][nc];
    nor->manj[cnt].post_load = clmwl[POST_RETROFIT][nc];
    nor->num_manj++;
  }

  for (nc = 0; nc < ndi->num_win; nc++) {
    if (fp)
      fprintf(fp, "  %6s%10.0f%14.0f%17.0f\n", ndi->win[nc].code, ndi->win[nc].area_gross, clmwn[PRE_RETROFIT][nc], clmwn[POST_RETROFIT][nc]);
    cnt = nor->num_manj;
    nor->manj[cnt].index = cnt;
    STRCPY(nor->manj[cnt].heatcool, "cool");
    STRCPY(nor->manj[cnt].type, "Window (Btu/h)");
    STRCPY(nor->manj[cnt].name, ndi->win[nc].code);
    nor->manj[cnt].area_vol = ndi->win[nc].area_gross;
    nor->manj[cnt].pre_load = clmwn[PRE_RETROFIT][nc];
    nor->manj[cnt].post_load = clmwn[POST_RETROFIT][nc];
    nor->num_manj++;
  }

  for (nc = 0; nc < ndi->num_dor; nc++) {
    if (fp)
      fprintf(fp, "  %6s%10.0f%14.0f%17.0f\n", ndi->dor[nc].code, ndi->dor[nc].area, clmdr[PRE_RETROFIT][nc], clmdr[POST_RETROFIT][nc]);
    cnt = nor->num_manj;
    nor->manj[cnt].index = cnt;
    STRCPY(nor->manj[cnt].heatcool, "cool");
    STRCPY(nor->manj[cnt].type, "Door (Btu/h)");
    STRCPY(nor->manj[cnt].name, ndi->dor[nc].code);
    nor->manj[cnt].area_vol = ndi->dor[nc].area;
    nor->manj[cnt].pre_load = clmdr[PRE_RETROFIT][nc];
    nor->manj[cnt].post_load = clmdr[POST_RETROFIT][nc];
    nor->num_manj++;
  }

  for (nc = 0; nc < ndi->num_uas; nc++) {
    if (fp)
      fprintf(fp, "  %6s%10.0f%14.0f%17.0f\n", ndi->uas[nc].code, ndi->uas[nc].area, clmua[PRE_RETROFIT][nc], clmua[POST_RETROFIT][nc]);
    cnt = nor->num_manj;
    nor->manj[cnt].index = cnt;
    STRCPY(nor->manj[cnt].heatcool, "cool");
    STRCPY(nor->manj[cnt].type, "Attic (Btu/h)");
    STRCPY(nor->manj[cnt].name, ndi->uas[nc].code);
    nor->manj[cnt].area_vol = ndi->uas[nc].area;
    nor->manj[cnt].pre_load = clmua[PRE_RETROFIT][nc];
    nor->manj[cnt].post_load = clmua[POST_RETROFIT][nc];
    nor->num_manj++;
  }

  for (nc = 0; nc < ndi->num_fnd; nc++) {
    if (fp)
      fprintf(fp, "  %6s%10.0f%14.0f%17.0f\n", ndi->fnd[nc].code, ndi->fnd[nc].area, clmsb[PRE_RETROFIT][nc], clmsb[POST_RETROFIT][nc]);
    cnt = nor->num_manj;
    nor->manj[cnt].index = cnt;
    STRCPY(nor->manj[cnt].heatcool, "cool");
    STRCPY(nor->manj[cnt].type, "Foundation (Btu/h)");
    STRCPY(nor->manj[cnt].name, ndi->fnd[nc].code);
    nor->manj[cnt].area_vol = ndi->fnd[nc].area;
    nor->manj[cnt].pre_load = clmsb[PRE_RETROFIT][nc];
    nor->manj[cnt].post_load = clmsb[POST_RETROFIT][nc];
    nor->num_manj++;
  }

  volume = ndi->gnl.floor_area * 8.0f;
  if (fp)
    fprintf(fp, "     Inf%10.0f%14.0f%17.0f\n", volume, clminf[PRE_RETROFIT], clminf[POST_RETROFIT]);
  cnt = nor->num_manj;
  nor->manj[cnt].index = cnt;
  STRCPY(nor->manj[cnt].heatcool, "cool");
  STRCPY(nor->manj[cnt].type, "Infiltration (Btu/h)");
  STRCPY(nor->manj[cnt].name, "Inf");
  nor->manj[cnt].area_vol = volume;
  nor->manj[cnt].pre_load = clminf[PRE_RETROFIT];
  nor->manj[cnt].post_load = clminf[POST_RETROFIT];
  nor->num_manj++;

  if (fp)
    fprintf(fp, "  People%10.0f%14.0f%17.0f\n", ndi->gnl.avg_no_occupants, fpeople, fpeople);
  cnt = nor->num_manj;
  nor->manj[cnt].index = cnt;
  STRCPY(nor->manj[cnt].heatcool, "cool");
  STRCPY(nor->manj[cnt].type, "People (Btu/h)");
  STRCPY(nor->manj[cnt].name, "People");
  nor->manj[cnt].area_vol = ndi->gnl.avg_no_occupants;
  nor->manj[cnt].pre_load = fpeople;
  nor->manj[cnt].post_load = fpeople;
  nor->num_manj++;

  if (fp)
    fprintf(fp, "    Appl%24.0f%17.0f\n", fappliances, fappliances);
  cnt = nor->num_manj;
  nor->manj[cnt].index = cnt;
  STRCPY(nor->manj[cnt].heatcool, "cool");
  STRCPY(nor->manj[cnt].type, "Appliances (Btu/h)");
  STRCPY(nor->manj[cnt].name, "Appl");
  nor->manj[cnt].area_vol = 1;
  nor->manj[cnt].pre_load = fappliances;
  nor->manj[cnt].post_load = fappliances;
  nor->num_manj++;

  if (fp)
    fprintf(fp, "\nTotal (Sensible)%16.0f%17.0f\n", nir->clmt[PRE_RETROFIT], nir->clmt[POST_RETROFIT]);
  cnt = nor->num_manj;
  nor->manj[cnt].index = cnt;
  STRCPY(nor->manj[cnt].heatcool, "cool");
  STRCPY(nor->manj[cnt].type, "Total Sensible (Btu/h)");
  STRCPY(nor->manj[cnt].name, "TotS");
  nor->manj[cnt].area_vol = 0;
  nor->manj[cnt].pre_load = nir->clmt[PRE_RETROFIT];
  nor->manj[cnt].post_load = nir->clmt[POST_RETROFIT];
  nor->num_manj++;

  clduct[PRE_RETROFIT] = frductlos_cl * nir->clmt[PRE_RETROFIT];
  nir->clmt[PRE_RETROFIT] += clduct[PRE_RETROFIT];
  clduct[POST_RETROFIT] = frductlos_cl * nir->clmt[POST_RETROFIT];
  nir->clmt[POST_RETROFIT] += clduct[POST_RETROFIT];

  if (fp)
    fprintf(fp, "Ducts   %10.0f%14.0f%17.0f\n", nir->ductarea, clduct[PRE_RETROFIT], clduct[POST_RETROFIT]);
  cnt = nor->num_manj;
  nor->manj[cnt].index = cnt;
  STRCPY(nor->manj[cnt].heatcool, "cool");
  STRCPY(nor->manj[cnt].type, "Ducts (Btu/h)");
  STRCPY(nor->manj[cnt].name, "Ducts");
  nor->manj[cnt].area_vol = 0;
  nor->manj[cnt].pre_load = clduct[PRE_RETROFIT];
  nor->manj[cnt].post_load = clduct[POST_RETROFIT];
  nor->num_manj++;

  if (fp)
    fprintf(fp, " Total (with ducts)%13.0f%17.0f\n", nir->clmt[PRE_RETROFIT], nir->clmt[POST_RETROFIT]);
  cnt = nor->num_manj;
  nor->manj[cnt].index = cnt;
  STRCPY(nor->manj[cnt].heatcool, "cool");
  STRCPY(nor->manj[cnt].type, "Total (with ducts) (Btu/h)");
  STRCPY(nor->manj[cnt].name, "TotW");
  nor->manj[cnt].area_vol = 0;
  nor->manj[cnt].pre_load = nir->clmt[PRE_RETROFIT];
  nor->manj[cnt].post_load = nir->clmt[POST_RETROFIT];
  nor->num_manj++;

  if (fp)
    fprintf(fp, " Size (tons)%19.1f%17.1f\n", nir->clmt[PRE_RETROFIT] / BTUH_PER_TON, nir->clmt[POST_RETROFIT] / BTUH_PER_TON);
  cnt = nor->num_manj;
  nor->manj[cnt].index = cnt;
  STRCPY(nor->manj[cnt].heatcool, "cool");
  STRCPY(nor->manj[cnt].type, "Size (tons)");
  STRCPY(nor->manj[cnt].name, "Size");
  nor->manj[cnt].area_vol = 0;
  nor->manj[cnt].pre_load = nir->clmt[PRE_RETROFIT] / BTUH_PER_TON;
  nor->manj[cnt].post_load = nir->clmt[POST_RETROFIT] / BTUH_PER_TON;
  nor->num_manj++;

  if (fp)
    fprintf(fp, "\nLatent Load (inf)%14.0f%17.0f\n", flatent[PRE_RETROFIT], flatent[POST_RETROFIT]);
  cnt = nor->num_manj;
  nor->manj[cnt].index = cnt;
  STRCPY(nor->manj[cnt].heatcool, "cool");
  STRCPY(nor->manj[cnt].type, "Latent Load (inf) (Btu/h)");
  STRCPY(nor->manj[cnt].name, "LatentI");
  nor->manj[cnt].area_vol = 0;
  nor->manj[cnt].pre_load = flatent[PRE_RETROFIT];
  nor->manj[cnt].post_load = flatent[POST_RETROFIT];
  nor->num_manj++;

  if (fp)
    fprintf(fp, "Latent Load (occ)%14.0f%17.0f\n", flatent_occ, flatent_occ);
  cnt = nor->num_manj;
  nor->manj[cnt].index = cnt;
  STRCPY(nor->manj[cnt].heatcool, "cool");
  STRCPY(nor->manj[cnt].type, "Latent Load (occ) (Btu/h)");
  STRCPY(nor->manj[cnt].name, "LatentO");
  nor->manj[cnt].area_vol = 0;
  nor->manj[cnt].pre_load = flatent_occ;
  nor->manj[cnt].post_load = flatent_occ;
  nor->num_manj++;

  if (fp)
    fprintf(fp, "Latent Load (tot)%14.0f%17.0f\n", flatent_tot[PRE_RETROFIT], flatent_tot[POST_RETROFIT]);
  cnt = nor->num_manj;
  nor->manj[cnt].index = cnt;
  STRCPY(nor->manj[cnt].heatcool, "cool");
  STRCPY(nor->manj[cnt].type, "Latent Load (tot) (Btu/h)");
  STRCPY(nor->manj[cnt].name, "LatentT");
  nor->manj[cnt].area_vol = 0;
  nor->manj[cnt].pre_load = flatent_tot[PRE_RETROFIT];
  nor->manj[cnt].post_load = flatent_tot[POST_RETROFIT];
  nor->num_manj++;

  nir->clmt[PRE_RETROFIT] += flatent_tot[PRE_RETROFIT];
  nir->clmt[POST_RETROFIT] += flatent_tot[POST_RETROFIT];

  if (fp)
    fprintf(fp, "\n Total Load%21.0f%17.0f\n", nir->clmt[PRE_RETROFIT], nir->clmt[POST_RETROFIT]);
  cnt = nor->num_manj;
  nor->manj[cnt].index = cnt;
  STRCPY(nor->manj[cnt].heatcool, "cool");
  STRCPY(nor->manj[cnt].type, "Total Load (Btu/h)");
  STRCPY(nor->manj[cnt].name, "Total");
  nor->manj[cnt].area_vol = 0;
  nor->manj[cnt].pre_load = nir->clmt[PRE_RETROFIT];
  nor->manj[cnt].post_load = nir->clmt[POST_RETROFIT];
  nor->num_manj++;

  // update our summary total output figures, now that we
  // have added latent loads to the sensible

  nor->an_load[3].pre_cool = nir->clmt[PRE_RETROFIT] / BTUH_PER_TON;
  nor->an_load[3].post_cool = nir->clmt[POST_RETROFIT] / BTUH_PER_TON;

  if (fp)
    fprintf(fp, " Size (tons)%20.1f%17.1f\n", nir->clmt[PRE_RETROFIT] / BTUH_PER_TON, nir->clmt[POST_RETROFIT] / BTUH_PER_TON);
  cnt = nor->num_manj;
  nor->manj[cnt].index = cnt;
  STRCPY(nor->manj[cnt].heatcool, "cool");
  STRCPY(nor->manj[cnt].type, "Size (tons)");
  STRCPY(nor->manj[cnt].name, "Size");
  nor->manj[cnt].area_vol = 0;
  nor->manj[cnt].pre_load = nir->clmt[PRE_RETROFIT] / BTUH_PER_TON;
  nor->manj[cnt].post_load = nir->clmt[POST_RETROFIT] / BTUH_PER_TON;
  nor->num_manj++;

}
