/******************  MODULE NAME:  BALANCE.C  ****************************/
/**       DATE: 3/14/93                                                             **/
/**          BY:    SLF                                                                 **/
/** DESCRIPTION:    Functions for computing -                                       **/
/**                         Degree Days (for htg & clg, first estimate)     **/
/**                         Solar Gain                                                  **/
/**                         Free Heat (Internal Gains)  (future)                **/
/**                         Sky Radiation                                               **/
/**                         Infiltration Heat Loss                                  **/
/**                         Solar Storage Factor                                        **/
/**                             Balance Point Temperature                               **/
/** REVISIONS:  10/07/93 SLF - Updated comments and calculations        **/
/**     03/24/94 SLF - Changed for NEAT weather data.                       **/
/**     10/18/94 SLF - Allowed for no CLG file to exist.                    **/
/**     11/16/94 SLF - Reordered mir->flgRetrofits.                               **/
/**      6/15/95 NLW - Correcting awning problem in SolarGain               **/
/**      7/12/95 NLW - Adjusted "DuctFraction" impact                       **/
/**      7/12/95 NLW - Adjusted leakiness parameter "fL"                    **/
/**      7/12/95 NLW - Corrected parameter index problem                    **/
/**                                     for home leakiness                          **/
/**      8/8/95 NLW  - Updated Transmismod expression for missing terms**/
/**      8/11/95 NLW - Corrected sign error in solar storage    factor  **/
/*************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wa_engine.h"

/*******************  FUNCTION NAME:  SolarGain  *************************/
/**         DATE:  3/9/93                                                               **/
/**           BY:    SLF                                                                    **/
/**  DESCRIPTION:    See pages 47 - 47.2 in Notes by Sheila Hayter.         **/
/**    REVISIONS:    03/24/94 SLF - Changed for NEAT weather data.        **/
/**    REVISIONS:    08/08/95 NLW - Updated Transmission Modifier.        **/
/*************************************************************************/
float component_solar_gain(int iComponent, int iMonth, float fShadingRatioAwn, float fShadingRatioL, float fShadingRatioW2H,
                float fSolarFluxHoriz, float fSHGFs[], float fLatitude, float fSA_N, float fSA_S, float fSA_E, float fSA_W,
                float fSA_H) {
  /**********************************/
  /*  Data file handling variables  */
  /**********************************/
  int i, iOrien; /* Counting variables */

  float fB[5][7];            /* Overhang Modifier Coefficients */
  float fOverhangMod;        /* Overhang Modifier (for South */
                             /*  Facing Surfaces) (unitless) */
  float fOverhangMod0 = 0.0; /* Overhang Modifier for window */
                             /*   surfaces completely shaded  */
  float fExposure;           /* Factor from GNL input screen */
  float fSA;                 /* Solar Apeture (Btus...) */

  float fSHGF; /* Solar Heat Gain Factor */
  float fSolarFluxTotal, fSolarFluxDiffuse, fSolarFluxDirect;
  float fSG = 0.0;          /* Solar Gain (Btu/h) */
  float fTransmisMod = 0.0; /* Transmissivity Modifier (frac.) */
  float fDeclination;       /* Solar Declination (degrees) */
  float fSolarAngle;        /* Solar Noon Zenith Angle (degrees */
  float fN[MONTHS + 1] =            /* Day of Year - for Solar Decl. */
      {0, 15.0, 45.0, 75.0, 106.0, 136.0, 167.0, 197.0, 228.0, 259.0, 289.0, 320.0, 350.0};

  /**NW, 8/8/95:  The seasonal modifier for glazing transmission has been
  estimated as follows - Single and Double glazed coefficients were averaged for
  latitude=35, Kt=0.5 based on corelations in ASHRAE Passive Solar Design Manual
  (Refer to project worknotes and spreadsheet TRN_INC.XLS dated 8/9/95. The seasonal
  modifier has been normalized since glass transmissivity has already be included
  in solar aperture calculations (UA_WIN.C).  EAST/WEST seasonal corrections are weaker
  based on Balcomb's corelation and have therefore been set to a constant 1.0. **/

  float fNrmlTmod[MONTHS + 1] = /* Normalized transmission modifier data */
      {0, 1.00F, 0.98F, 0.94F, 0.88F, 0.83F, 0.82F, 0.83F, 0.82F, 0.84F, 0.90F, 0.96F, 0.99F};
  // NW, 8/10/95, test ramp for higher summer solar to match observed trend
  //     from DOE2
  //  float fNrmlTmod[12] =                               /* Normalized transmission modifier data */
  //          {  1.00, 1.10, 1.20, 1.30, 1.40, 1.50,
  //             1.50, 1.40, 1.30, 1.20, 1.10, 1.00 };

  /*****************************/
  /*  Solar Exposure Modifier  */
  /*****************************/

  // NW, 9/12/95: This exposure modifier factor is deactivated.
  //  As defined it overshadows infiltration effects of the
  //  windshielding modifier and may overstate the reduction of
  //  solar gains for typical mobile home siting.

  switch(mdi->gnl.wind_shielding) {
    case WS_WELL:
      fExposure = 0.50;
      break;
    case WS_NORMAL:
      fExposure = 0.75;
      break;
    default:
      fExposure = 1.0;
      break;
  }

  // if (mdi->gnl.wind_shielding == WS_WELL)
  //   fExposure = 0.50;
  // if (mdi->gnl.wind_shielding == WS_NORMAL)
  //   fExposure = 0.75;
  // if (mdi->gnl.wind_shielding == WS_EXPOSED)
  //   fExposure = 1.0;

  /***************************************************/
  /*  This array of coefficients is for calculating  */
  /*   the Overhang Modifier (see p. 47.2)                */
  /***************************************************/

  /* For fRatioW2H == 1/8 */
  fB[0][0] = 1.113f;
  fB[0][1] = 5.1346f;
  fB[0][2] = 34.787f;
  fB[0][3] = 110.97f;
  fB[0][4] = 188.87f;
  fB[0][5] = 164.35f;
  fB[0][6] = 57.283f;
  /* For fRatioW2H == 1/4 */
  fB[1][0] = 1.389f;
  fB[1][1] = 10.235f;
  fB[1][2] = 57.238f;
  fB[1][3] = 153.67f;
  fB[1][4] = 223.11f;
  fB[1][5] = 167.96f;
  fB[1][6] = 51.280f;
  /* For fRatioW2H == 3/8 */
  fB[2][0] = 1.349f;
  fB[2][1] = 7.8189f;
  fB[2][2] = 28.720f;
  fB[2][3] = 42.75f;
  fB[2][4] = 27.37f;
  fB[2][5] = 5.710f;
  fB[2][6] = 0.0f;
  /* For fRatioW2H == 1/2 */
  fB[3][0] = 1.325f;
  fB[3][1] = 6.7539f;
  fB[3][2] = 19.550f;
  fB[3][3] = 20.28f;
  fB[3][4] = 6.9f;
  fB[3][5] = 0.0f;
  fB[3][6] = 0.0f;
  /* For fRatioW2H > 1 */
  fB[4][0] = 0.0f;
  fB[4][1] = 0.0f;
  fB[4][2] = 0.0f;
  fB[4][3] = 0.0f;
  fB[4][4] = 0.0f;
  fB[4][5] = 0.0f;
  fB[4][6] = 0.0f;

  /*****************************/
  /*  Solar Noon Zenith Angle  */
  /*****************************/
  fDeclination = 23.45f * (float)sin(2.0f * PI * (284.0f + fN[iMonth]) / 365.0f);
  fSolarAngle = (fLatitude - fDeclination) / 100.0f;

  /*************************************/
  /*  Orientation specific components  */
  /*************************************/

  if (iComponent != 4) /* Not iROOF */
  {
    float fShadingRatioLtmp = fShadingRatioL; /* Save ratio for loop on orientation */

    for (iOrien = 1; iOrien <= 4; iOrien++) {
      float fShadingRatio0 = 0.0;         /* Shading Ratio for length of    */
                                          /*   window surface complete shaded */
      fShadingRatioL = fShadingRatioLtmp; /* Restore value after each pass thru loop */

      switch (iOrien) {
      case 1: /* North */
        fSA = fSA_N;
        fSHGF = (float)fSHGFs[SLR_NORTH];
        fShadingRatioL = 0.0;
        fOverhangMod = 1.0; /* No shading effect */
        fTransmisMod = 1.0; /* No transmissivity modifier effect */
        break;

      case 2: /* East */
        fSA = fSA_E;
        fSHGF = (float)fSHGFs[SLR_EAST_WEST];
        fShadingRatioL = 0.0;
        fOverhangMod = 1.0; /* No shading effect */

        if (iComponent == 2) /* iWINDOW */
                             /** NW, 8/8/95: Corelation missing Kt portion - this will be replaced by
                             a simple constant for now.
                                                     fTransmisMod = ( ( 0.6431 * pow(fSolarAngle, 0.0)) +
                                                                           ( 0.0115 * pow(fSolarAngle, 1.0)) +
                                                                           (-0.0392 * pow(fSolarAngle, 2.0)) +
                                                                           ( 0.0    * pow(fSolarAngle, 3.0)) +
                                                                           ( 0.0    * pow(fSolarAngle, 4.0)) +
                                                                           ( 0.0    * pow(fSolarAngle, 5.0)) );  **/
          fTransmisMod = 1.00;
        else                  /* Walls & Doors */
          fTransmisMod = 1.0; /* No transmissivity modifier effect */

        break;

      case 3: /* South */
        fSA = fSA_S;
        fSHGF = (float)fSHGFs[SLR_SOUTH];

        if (iComponent == 1) /* iWALL */
        {
          fTransmisMod = 1.0; /* No transmissivity modifier effect */

          if ((fShadingRatioW2H == 0.0) || (fShadingRatioL == 0.0))
            fOverhangMod = 1.0;
          else {
            if (fShadingRatioW2H <= (3.0 / 16.0))
              i = 0; /* Use values for 1/8 */
            else if (fShadingRatioW2H <= (5.0 / 16.0))
              i = 1; /* Use values for 1/4 */
            else if (fShadingRatioW2H <= (7.0 / 16.0))
              i = 2; /* Use values for 3/8 */
            else if (fShadingRatioW2H < 1.0)
              i = 3; /* Use values for 1/2 */
            else     /* fShadingRatioW2H >= 1.0  */
              i = 4; /* Use values for 1 */
            fOverhangMod = (fB[i][0] * (float)pow(fSolarAngle, 0)) - (fB[i][1] * (float)pow(fSolarAngle, 1)) +
                           (fB[i][2] * (float)pow(fSolarAngle, 2)) - (fB[i][3] * (float)pow(fSolarAngle, 3)) +
                           (fB[i][4] * (float)pow(fSolarAngle, 4)) - (fB[i][5] * (float)pow(fSolarAngle, 5)) +
                           (fB[i][6] * (float)pow(fSolarAngle, 6));
          }
        }
        if (iComponent == 2) /* iWINDOW */
        {
          /** NW, 8/8/95: Corelation missing Kt portion - this will be replaced by
          a simple constant for now.
                                  fTransmisMod = ( ( 0.6465 * (float)pow(fSolarAngle, 0.0)) +
                                                   ( 0.7766 * (float)pow(fSolarAngle, 1.0)) +
                                                   (-7.1570 * (float)pow(fSolarAngle, 2.0)) +
                                                   (22.7710 * (float)pow(fSolarAngle, 3.0)) +
                                                   (-29.310 * (float)pow(fSolarAngle, 4.0)) +
                                                   (13.2940 * (float)pow(fSolarAngle, 5.0)) );  **/
          fTransmisMod = fNrmlTmod[iMonth];
          if (fShadingRatioL > 0.0)
          /*******************
          Some or all of south windows are shaded by awnings.
          *******************/
          {
            if (fShadingRatioAwn <= (3.0 / 16.0))
              i = 0; /* Use values for 1/8 */
            else if (fShadingRatioAwn <= (5.0 / 16.0))
              i = 1; /* Use values for 1/4 */
            else if (fShadingRatioAwn <= (7.0 / 16.0))
              i = 2; /* Use values for 3/8 */
            else if (fShadingRatioAwn < 1.0)
              i = 3; /* Use values for 1/2 */
            else     /* fShadingRatioAwn >= 1.0  */
              i = 4; /* Use values for 1 */
            fOverhangMod = (fB[i][0] * (float)pow(fSolarAngle, 0)) - (fB[i][1] * (float)pow(fSolarAngle, 1)) +
                           (fB[i][2] * (float)pow(fSolarAngle, 2)) - (fB[i][3] * (float)pow(fSolarAngle, 3)) +
                           (fB[i][4] * (float)pow(fSolarAngle, 4)) - (fB[i][5] * (float)pow(fSolarAngle, 5)) +
                           (fB[i][6] * (float)pow(fSolarAngle, 6));
          }
          if (fShadingRatioW2H > 0.0)
          /*****************
          Some or all of the south windows are shaded by
          a carport or porch roof.  Assume that they are
          completely shaded so that fOverhangMod is zero.
          *****************/
          {
            fOverhangMod0 = 0.0;
            fShadingRatio0 = fShadingRatioW2H;
          }
        }
        if (iComponent == 3) /* iDOOR */
        {
          if (fShadingRatioL > 0.0)
            /*****************
            Some or all of the south doors are shaded by a carport
            or porch roof.  Assume that the fShadingRatioL
            fraction (from the wall calc.) is completely shaded.
            *****************/
            fOverhangMod = 0.0;
          else
            fOverhangMod = 1.0;
        }
        break;

      case 4: /* West */
        fSA = fSA_W;
        fSHGF = (float)fSHGFs[SLR_EAST_WEST];
        fShadingRatioL = 0.0;
        fOverhangMod = 1.0; /* No shading effect */

        if (iComponent == 2) /* iWINDOW */
                             /** NW, 8/8/95: Corelation missing Kt portion - this will be replaced by
                             a simple constant for now.
                                                     fTransmisMod = ( ( 0.6431 * pow(fSolarAngle, 0.0)) +
                                                                           ( 0.0115 * pow(fSolarAngle, 1.0)) +
                                                                           (-0.0392 * pow(fSolarAngle, 2.0)) +
                                                                           ( 0.0    * pow(fSolarAngle, 3.0)) +
                                                                           ( 0.0    * pow(fSolarAngle, 4.0)) +
                                                                           ( 0.0    * pow(fSolarAngle, 5.0)) );  **/
          fTransmisMod = 1.00;

        else                  /* Walls & Doors */
          fTransmisMod = 1.0; /* No transmissivity modifier effect */

        break;

      } /* End of switch */

      /*******************/
      /* Solar Flux Data */
      /*******************/

      fSolarFluxTotal = fSHGF * fSolarFluxHoriz / 24.0f;
      fSolarFluxDiffuse = (float)fSHGFs[SLR_NORTH] * fSolarFluxHoriz / 24.0f;
      fSolarFluxDirect = fSolarFluxTotal - fSolarFluxDiffuse;

      /********************
      The number of hours of sunshine per day for all months is
      currently hard wired to 12.0.  This approx. may need to change.
      fSG updated 03/24/94 to NREL/NEAT specifications
                      03/17/95
      ********************/
      fSG += (float)((fSA * fTransmisMod * fExposure * fSolarFluxDirect *
                      ((1.0 - fShadingRatioL - fShadingRatio0) + (fOverhangMod * fShadingRatioL) +
                       (fOverhangMod0 * fShadingRatio0))) +
                     (fSA * fSolarFluxDiffuse));

    } /* End of for() */

  } /* End of if() */

  /* iComponent == iROOF */
  else {
    fSA = fSA_H;
    fSHGF = 1.0;
    fOverhangMod = 1.0;
    fTransmisMod = 1.0; /* No transmissivity modifier effect */

    fSolarFluxTotal = (float)(fSHGF * fSolarFluxHoriz / 24.0);
    fSolarFluxDiffuse = (float)(fSHGFs[SLR_NORTH] * fSolarFluxHoriz / 24.0);
    fSolarFluxDirect = fSolarFluxTotal - fSolarFluxDiffuse;

    fSG = (fSA * fTransmisMod * fExposure * fOverhangMod * fSolarFluxDirect) + (fSA * fSolarFluxDiffuse);
  }

  return (fSG);

} /* End of function component_solar_gain() */

/*******************  FUNCTION NAME:  sky_radiation_losses  *******************/
/**         DATE:  March 1993                                                           **/
/**           BY:    JG                                                                 **/
/**  DESCRIPTION:    Calculates the daily daytime and nighttime sky         **/
/**  radiation losses.  All values area calculated in English units         **/
/*************************************************************************/

#define STEPHBOLTZ 0.1714e-8 // Units: Btu/h-ft^2-R^4
#define ROOFEMMISIVITY 0.91
#define WALLEMMISIVITY 0.80
#define SKYEMMISIVDAY 0.5921
#define SKYEMMISIVNIGHT 0.6081
#define FDEGREESTORANKIN 460.0

void sky_radiation_losses(int iSeason, float fUAWallTotal, float fUARoof, float fTOutsideAirDay, float fTOutsideAirNight,
                     float *fSkyRadLossDay, float *fSkyRadLossNight) {
  float fhConvective; // Radiative-conductive film coefficient.

  if (iSeason == HEATING)   // It's the heating season.
    fhConvective = 6.0; // In English units.
  else                  // It's the cooling season.
    fhConvective = 4.0; // In English units.

  /*********************
  Convert air temperatures from degrees F to Rankin.
  *********************/
  fTOutsideAirDay += FDEGREESTORANKIN;
  fTOutsideAirNight += FDEGREESTORANKIN;

  //   Units are Btu/h.
  *fSkyRadLossDay = (float)((4.0 * STEPHBOLTZ * pow(fTOutsideAirDay, 4) * (1.0 - pow((SKYEMMISIVDAY / ROOFEMMISIVITY), 0.25)) *
                             ((fUARoof * ROOFEMMISIVITY) + ((WALLEMMISIVITY / 3.0) * fUAWallTotal))) /
                            fhConvective);

  *fSkyRadLossNight =
      (float)((4.0 * STEPHBOLTZ * pow(fTOutsideAirNight, 4) * (1.0 - pow((SKYEMMISIVNIGHT / ROOFEMMISIVITY), 0.25)) *
               ((fUARoof * ROOFEMMISIVITY) + ((WALLEMMISIVITY / 3.0) * fUAWallTotal))) /
              fhConvective);


} // End of sky_radiation_lossesLoss() user function.

/*******************  FUNCTION NAME:  mhea_infiltration_losses  *******************/
/**       DATE: 3/13/93     07/29/93                                                **/
/**          BY:    NREL/ERG        SLF                                                 **/
/** DESCRIPTION:    Routine to compute total infiltration loss              **/
/**                 coefficient.                                                        **/
/**                                                                                         **/
/**  REVISION:  Updated to account for cooling equipment ducts.         **/
/**                 SLF 11/10/94 Updated Parameters data sturcture index.   **/
/**                                                                                         **/
/**  English Units Version                                                              **/
/*************************************************************************/

#define A -0.00003718 //"A" factor per feet

void mhea_infiltration_losses(int iSeason, int iMonth, float fElevation, float fTempOut[], float fWindSpeed[], float fVolumeCathCeiling,
                     float fVolumeAddition, float *fInfiltration, float *fQILoss, float *fInfMassFlow, int *iDuctStatus) {
  int i, j;                       // Counter variables
  float fVolume,                  // Building Volume
      fTempIn,                    // Inside setpoint Temperature
      fTempInDay = 0.0,           // Inside Daytime setpoint Temperature
      fTempInNight = 0.0;         // Inside Nighttime setpoint Temperature
  float fH = 1.0,                 // Height correction factor.
      fL,                         // Leak correction factor from building database.
      fS;                         // Wind Shielding factor from building database.
  float fCF,                      // Correction Factor, product of fH, fS, fL.
      fELA,                       // Effective Leakage Area (sf)
      fSpecInfl = 0.0,            // Specific Infiltration (ft/min)
      fCFM,                       // Adjusted infiltration CFM
      fACH;                       // Air Changes per Hour calculated in this function.
  float fDuctFraction = 1.0;      // Outside duct fraction.
  float fwindsp;                  // Monthly average wind speed (mhp)

  float fwn_cfm_tot;              // Total window plus windows in addition leakage rate under natural conditions
  static float STfwnCfmTot[MONTHS + 1];  // Prior pass' total window inf. If current change subtract difference from whole house loss

  float fdr_cfm_tot;              // Total door plus doors in addition leakage rate under natural conditions
  static float STfdrCfmTot[MONTHS + 1];  // Prior pass' total door inf. If current change subtract difference from whole house loss

  if (cmds.debug_level & D_MHEA_ENERGY_DETAIL)
    fprintf(stderr, "\n----INFILTRATION DETAILS----");

  /*********************
  Get the wind shielding factor from general data file.
  *********************/

  switch(mdi->gnl.wind_shielding) {
    case WS_WELL:
      fS = 1.2F;
      break;
    case WS_NORMAL:
      fS = 1.0F;
      break;
    default:
      fS = 0.9F;
      break;
  }

  /***************
  Compute monthly average wind speed.  Translate window leakage coefficients
  into cfm leakage rates through windows and door (MJF #242)
  ***************/

  fwindsp = 0.0f;
  for (i = BIN_00_04; i <= BIN_20_24; i++) // time of day bins
    fwindsp += fWindSpeed[i];
  fwindsp /= 6.0f;

  // All windows infiltration CFM
  fwn_cfm_tot = 0.0f;
  for (j = 0; j < mdi->num_win; j++) {
    fwn_cfm_tot += 0.1f * mdi->win[j].leak_coef * (float)POWC(fwindsp, 1.6) * mir->window_cfm_adjustment * window_perimeter(&mdi->win[j]) / fS;
  }
  for (j = 0; j < mdi->num_awn; j++) {
    fwn_cfm_tot += 0.1f * mdi->awn[j].leak_coef * (float)POWC(fwindsp, 1.6) * mir->window_cfm_adjustment * window_perimeter(&mdi->awn[j]) / fS;
  }

  // All doors infiltration CFM
  fdr_cfm_tot = 0.0f;
  for (j = 0; j < mdi->num_dor; j++) {
    fdr_cfm_tot += 0.1f * mdi->dor[j].leak_coef * (float)POWC(fwindsp, 1.6) * mir->window_cfm_adjustment * door_perimeter(&mdi->dor[j]) / fS;
  }
  for (j = 0; j < mdi->num_adr; j++) {
    fdr_cfm_tot += 0.1f * mdi->adr[j].leak_coef * (float)POWC(fwindsp, 1.6) * mir->window_cfm_adjustment * door_perimeter(&mdi->adr[j]) / fS;
  }

  /*********************
  Get the blower door test value (in units of cfm) from general data file.
  *********************/
  if (mir->flgWhichPass == BASE_CASE) /* Base Case pass through energy use calcs. */
  {
    // Base cfm is either from blower door or leakiness descriptor (also in units of CFM)
    if (mir->fCfm_house[M_PRE_RETROFIT] <= 0.0) {
      if (mdi->gnl.leakiness == LEAK_TIGHT)
        mir->fCfmUser = mdi->key.home_leakiness_tight;
      else if (mdi->gnl.leakiness == LEAK_LOOSE)
        mir->fCfmUser = mdi->key.home_leakiness_loose;
      else /* mdi->gnl.leakiness == LEAK_MEDIUM */
        mir->fCfmUser = mdi->key.home_leakiness_medium;
    } else {  /* mdi->gnl.pre_blowerdoor_test > 0.0 */
      ASSERT(mir->fPa_house[M_PRE_RETROFIT] != 0, sprintf(msg, "Assertion Failure"));
      // This looks to be a bug.  The blower door pressure correction exponent is 0.65, not the 0.50 used here, MJF 8/2020 #242
      //mir->fCfmUser = mir->fCfm_house[M_PRE_RETROFIT] * (float)sqrt(BASE_PRESSURE / mir->fPa_house[M_PRE_RETROFIT]);
      mir->fCfmUser = blower_door_corrected_cfm((BASE_PRESSURE / mir->fPa_house[M_PRE_RETROFIT]), mir->fCfm_house[M_PRE_RETROFIT]);
    }
    mir->fCfm = mir->fCfmUser;
  }

  // 8/28/97,NLW: Cleanup logic here for blowerdoor data

  // old stuff-caused replace htg to see different load between
  // first pass and cumulative pass and flip/flop on passing SIR test.
  // This wasn't anticipated in downstream savings logic.

  // #334 huge change here.  Previously, the duct seal measure is applied
  // in EACH engine run regardless if the measure flag is set or not.  It was
  // only based on the presence of the M_POST_DUCT_SEAL data in the input.
  // The correct code ONLY assigned the updated cfm value IF the seal duct
  // measure is implemented

  if (mir->flgWhichPass != BASE_CASE && // Not base case
      mir->fCfm_house[M_POST_DUCT_SEAL] > 0.0 &&
      mir->flgRetrofits[M_CMS_SEAL_DUCTS]) {
    ASSERT(mir->fPa_house[M_POST_DUCT_SEAL] != 0, sprintf(msg, "Assertion Failure"));
    mir->fCfm = mir->fCfm_house[M_POST_DUCT_SEAL] * (float)sqrt(BASE_PRESSURE / mir->fPa_house[M_POST_DUCT_SEAL]);

    // Lets make sure that the post retrofit CFM does not exceed the
    // pre-retrofit case...  This is what happens when the user does not
    // enter pre-retrofit CFM values and the program automatically fills
    // in the post-retrofit CFM values.  The result is that the mir->fCfmUser
    // gets default values from the HOME_LEAKINESS and that gets compared
    // to the explicit values filled in for post-retrofit.  So, here we just
    // need to make sure that the post-retrofit CFMs do not exceed the
    // pre-retrofit (MJF 4/29/02)

    if (mir->fCfm > mir->fCfmUser)
      mir->fCfm = mir->fCfmUser;

    //      mdi->gnl.post_ductseal_test > 0.0 )
    //      mir->fCfm = mdi->gnl.post_ductseal_test;
  } else
    mir->fCfm = mir->fCfmUser;

  // If we're considering duct sealing adjust cfm use blowerdoor
  // results (set above) or adjust by loss reduction factor
  // Note: If duct sealing passes it will persist for ranking
  // subsequent measures in cumulative pass - see logic in retrofit.c

  if (mir->flgWhichPass != BASE_CASE &&              // First Pass and Cumulative Pass
      mir->flgRetrofits[M_CMS_GENERAL_AIR_SEALING])  // General Air Sealing Retrofit
  {
    ASSERT(mir->fPa_house[M_POST_RETROFIT] != 0, sprintf(msg, "Assertion Failure"));
    mir->fCfm = mir->fCfm_house[M_POST_RETROFIT] * (float)sqrt(BASE_PRESSURE / mir->fPa_house[M_POST_RETROFIT]);

    // limit the post CFM to the pre-retrofit value just in case the
    // user has entered post retrofit CFM values w/o pre-retrofit data

    if (mir->fCfm > mir->fCfmUser)
      mir->fCfm = mir->fCfmUser;
  }

  /********************
  Get the building volume from general data file.
  ********************/
  fVolume = (mdi->gnl.length * mdi->gnl.width * mdi->gnl.height) + fVolumeCathCeiling + fVolumeAddition;

  /*****************
  Determine fDuctFraction re: P.53 of SH Notes.
  Assume only one set of heating equipment ducts, but could
  be on either primary or secondary equipment input screen.

  Eliminate fDuctFraction. Overestimates effect of infiltration. Duct
  leakage also now measured. MBG 12/20/02
  *****************/

  switch (iSeason){

  case HEATING:

    fTempInDay = mdi->key.heating_setpoint_day;
    fTempInNight = mir->fNightSetpoint;

    /* Set Duct Status for Manual J Heating Calculations */
    /*  NW, 3/28/97;  Modified 2/19/03 reflecting elimination of variable
        fDuctFraction parameter MBG */

    *iDuctStatus = 2; // Uninsulated ducts

    if (mdi->htg.duct_location == DL_NONE)
      *iDuctStatus = 0; // No Ducts

    else if (mdi->htg.duct_insl == DI_AROUND)
      *iDuctStatus = 1;

    else if (mdi->htg.duct_location == DL_CEILING && mdi->htg.duct_insl == DI_ABOVE)
      *iDuctStatus = 1;

    else if (mdi->htg.duct_location == DL_FLOOR) {

      if (mdi->htg.duct_insl == DI_BELOW)
        *iDuctStatus = 1;

      else if (mdi->flr.belly_condition == BC_GOOD && ((mdi->flr.belly_mineral_insl + mdi->flr.belly_loose_insl) <= 0.0f))
        *iDuctStatus = 1;
    }

    break;

  case COOLING:

    if (mir->flgNoCLG == FALSE) {  // there is at least one
      fTempInDay = mdi->key.cooling_setpoint_day;
      fTempInNight = mdi->key.cooling_setpoint_night;
    }

    break;

  default:
    ASSERT(FALSE, sprintf(msg, "Must be either heating or cooling"));
  }

  // if (iSeason == HEATING)  {
  //   fTempInDay = mdi->key.heating_setpoint_day;
  //   fTempInNight = mir->fNightSetpoint;

  //   /* Set Duct Status for Manual J Heating Calculations */
  //   /*  NW, 3/28/97;  Modified 2/19/03 reflecting elimination of variable
  //       fDuctFraction parameter MBG */

  //   *iDuctStatus = 2; // Uninsulated ducts

  //   if (mdi->htg.duct_location == DL_NONE)
  //     *iDuctStatus = 0; // No Ducts

  //   else if (mdi->htg.duct_insl == DI_AROUND)
  //     *iDuctStatus = 1;

  //   else if (mdi->htg.duct_location == DL_CEILING && mdi->htg.duct_insl == DI_ABOVE)
  //     *iDuctStatus = 1;

  //   else if (mdi->htg.duct_location == DL_FLOOR) {

  //     if (mdi->htg.duct_insl == DI_BELOW)
  //       *iDuctStatus = 1;

  //     else if (mdi->flr.belly_condition == BC_GOOD && ((mdi->flr.belly_mineral_insl + mdi->flr.belly_loose_insl) <= 0.0f))
  //       *iDuctStatus = 1;
  //   }
  //   /* Belly in good condition and no insulation between
  //      ducts and living space. Heat is likely to re-enter
  //      living space  */
  // } else if (iSeason == COOLING)  {
  //   if (mir->flgNoCLG == FALSE) // there is at least one
  //   {
  //     fTempInDay = mdi->key.cooling_setpoint_day;
  //     fTempInNight = mdi->key.cooling_setpoint_night;
  //   }
  // } else {
  //   ASS
  // }

  /*******************
  Get the leakiness factor based on the current Cfm value.
  *******************/

  /*******************
  Per discussion with Ron Judkoff, NREL-7/12/95, home leakiness adjustments
  will be attenuated to calibrate better with documented mobile home utility
  bills.  See note above for "DuctFraction"
  *******************/

  // Issue #156 remove dead assignments
  // if (mir->fCfm <= 1200.0)
  //   fL = 1.5; // NW, 7/12/95
  // //      fL = 1.4;
  // else if ((mir->fCfm > 1200.0) && (mir->fCfm <= 2200.0))
  //   fL = 1.25; // NW, 7/12/95
  // //      fL = 1.0;
  // else if (mir->fCfm > 2200.0)
  //   fL = 1.0; // NW, 7/12/95
  // //      fL = 0.7;

  /***********************************************************************
  The above discontinuous values of fL caused wild fluctuations in the
  savings for the air sealing measure.  Use a continuous nearly equivalent
  function instead.  MBG 5/23/03
  ************************************************************************/

  fL = (float)(1.0f + exp(-mir->fCfm / 1000.0f));

  /******************
  Calculate an adjusted infiltration cfm and then
  convert to units of air changes per hour (ACH).
  *******************/
  /* fH = height correction, fs = shielding correction, fL = leakiness correction */

  ASSERT(fH != 0, sprintf(msg, "Assertion Failure"));
  ASSERT(fS != 0, sprintf(msg, "Assertion Failure"));
  ASSERT(fL != 0, sprintf(msg, "Assertion Failure"));

  fCF = fH * fS * fL;

  fELA = (float)(mir->fCfm / (2756.0 * fCF));

  for (i = BIN_00_04; i <= BIN_20_24; i++) {
    if (i == BIN_00_04 || i == BIN_04_08 || i == BIN_20_24)
      fTempIn = fTempInNight;
    else
      fTempIn = fTempInDay;

    fSpecInfl += (float)(196.0 * sqrt((0.0169 * pow((0.447 * fWindSpeed[i]), 2.0)) +
                                      (0.0144 * (5.0 / 9.0) * fabs(fTempOut[i] - fTempIn))));
  }

  fCFM = (float)(fELA * (fSpecInfl / 6.0));
  ASSERT(fVolume != 0, sprintf(msg, "Assertion Failure"));
  fACH = (float)(fCFM * 60.0 / fVolume); // (ft^3/min) * (min/hour) * (vols/ft^3)

  /******************
  Calculate the infiltration, including a correction factor.
  *******************/
  *fInfiltration = (float)(fACH * exp(A * fElevation) * 0.75);

  /********************
  Calculate the mass flow of infiltration in cfm.
  (bldg_vols / hour) * (hour / 60min) * (ft^3 / bldg_vols)
  ********************/
  *fInfMassFlow = (float)(*fInfiltration * fVolume / 60.0);

  /******************
  Set base case monthly window  and door leakage. If leakage in successive
  passes changes, leakage has been reduced and we subtract the difference
  from the whole house leakage to account for the savings. 3/04  (updated 8/20 MJF #242)
  *******************/

  if (mir->flgWhichPass == BASE_CASE) {
    STfwnCfmTot[iMonth] = fwn_cfm_tot;
    STfdrCfmTot[iMonth] = fdr_cfm_tot;
  }

  if (fwn_cfm_tot != STfwnCfmTot[iMonth]) {
    *fInfMassFlow -= (float)((STfwnCfmTot[iMonth] - fwn_cfm_tot) * exp(A * fElevation));
    //ASSERT(*fInfMassFlow > 0.0, sprintf(msg, "Infiltration mass flow can not be negative.  Too much window leakage relative to blower door reading."));
  }
  if (fdr_cfm_tot != STfdrCfmTot[iMonth]) {
    *fInfMassFlow -= (float)((STfdrCfmTot[iMonth] - fdr_cfm_tot) * exp(A * fElevation));
    //ASSERT(*fInfMassFlow > 0.0, sprintf(msg, "Infiltration mass flow can not be negative.  Too much door leakage relative to blower door reading."));
  }

  // Do not reduce below zero #354
  if (*fInfMassFlow < 0.0) {
    static int added_message = FALSE;
    if (added_message == FALSE) {
      add_mhea_message("Infiltration mass flow (other than windows and doors) is ZERO.  Perhaps you have too much window and door leakiness relative to your blower door values.");
      added_message = TRUE;
    }
  *fInfMassFlow = 10.0;    // don't want to trip over later assert failure so this is a minimum airflow #369
  }

  /******************
  Calculate fQILoss in English units.
  (Btu/h-F) = (Btu/cf-F) * (ft^3) * (ACH) * ().
  *****************/
  //fDuctFraction = 1.0;  // Affect eliminated 12/20/02 MBG.
  fDuctFraction = 0.8f; // Use instead to reduce infiltration affects
  // which have always been too high using Sherman Grimsrud method.

/*******************
Replace expression for *fQILoss with equivalent form but in terms of
*InfMassFlow to allow affect of window infiltration reduction. 3/04.
*******************/

/*******************
fQILoss in metric units
(1 / 3.413) * (9 / 5) = 0.527175 (Btuh / W) * (F to C).
*******************/
  *fQILoss = (float)(1.098 * *fInfMassFlow * fDuctFraction);

  return;

} // End of the mhea_infiltration_losses user function.

/*******************  FUNCTION NAME:  getSolarStorage  *******************/
/**         DATE:  November 1993                                                        **/
/**           BY:    SLF                                                                    **/
/**  DESCRIPTION:    Calculates the fraction of the solar gain received **/
/**                  over 24 hours that is released during the night        **/
/**                  (Beta) (see p. 49 of Sheila Hayter's notes).           **/
/**         Revisions: 8/11/95, NW: transmismod update and solar            **/
/**             storage factor correction                                           **/
/*************************************************************************/
void mhea_solar_storage(float fTempOutDay, float fSolarGainTotal, float fFreeHeatDay, float fSkyRadLossDay,
                     float fConductionHeatLoss, float fQILoss, float *fSolarStorage) {
  float fTempAdjE;              /* Adjusted temp. (Tm), English units */
  float fTempAdjM;              /* Adjusted temp. (Tm), Metric units */
                                //  float fSolarStorageM = 0.22;    /* 1st approx. of solar storage factor */
                                // NW, 8/11/95: lower storage factor for mobile homes
  float fSolarStorageM = 0.10F; /* 1st approx. of solar storage factor */

  fTempAdjE = fTempOutDay + ((fSolarGainTotal + fFreeHeatDay - fSkyRadLossDay) / (fConductionHeatLoss + fQILoss));

  fTempAdjM = (float)((fTempAdjE - 32.0) * 5.0 / 9.0);

  ASSERT(fSolarStorageM != 0, sprintf(msg, "Assertion Failure"));
  if (fTempAdjM > 21.0)
    *fSolarStorage = (float)(fSolarStorageM * pow((1.64 - (fTempAdjM / 38.7)), (0.44 / fSolarStorageM)));
  else /* fTempAdjM <= 21.0 */
       // NW, 8/11/95: Correct sign error exponentiation function
       //      *fSolarStorage  = fSolarStorageM * pow( (0.56 - (fTempAdjM / 38.7)),
       //                        (0.44 / fSolarStorageM) );
    *fSolarStorage = (float)(fSolarStorageM * pow((0.56 + (fTempAdjM / 38.7)), (0.44 / fSolarStorageM)));

} // End mhea_solar_storage() function

/*******************  FUNCTION NAME:  get_balance_temp   *******************/
/**         DATE:  3/13/93                                                              **/
/**           BY:    NW                                                                 **/
/**  DESCRIPTION:    See page 52 in Notes by Sheila Hayter.                 **/
/**                                                                                         **/
/**  This routine calculates the day and night outdoor balance point        **/
/**  temperature for the home.  (English Units)                                 **/
/*************************************************************************/
void get_balance_temp(float fFactor, float fTempDay, float fTempNight, float fBeta, float fSolarGain, float fIntGainDay,
                    float fIntGainNight, float fSkyRadDay, float fSkyRadNight, float fCondLoss, float fInfilLoss,
                    float *fTBalanceDay, float *fTBalanceNight) {
  /*****************************************************************/
  /*  No extra variables used in calculating Balance Point Temp.   */
  /*****************************************************************/

  *fTBalanceDay =
      (float)(fTempDay +
              (fFactor * (((2.0 * (1.0 - fBeta) * fSolarGain) + fIntGainDay - fSkyRadDay) / (fCondLoss + fInfilLoss))));
  *fTBalanceNight =
      (float)(fTempNight + (fFactor * (((2.0 * fBeta * fSolarGain) + fIntGainNight - fSkyRadNight) / (fCondLoss + fInfilLoss))));
  return;
}
