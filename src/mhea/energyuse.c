/*******************  FILE NAME:  ENRGYUSE.C  ****************************/
/**         DATE:  2/23/93                                                              **/
/**           BY:    SLF                                                                    **/
/**  DESCRIPTION:    Reads values from form data structures and contains    **/
/**                  equations for various calculations.                        **/
/**    REVISIONS:                                                                           **/
/**     12/28/94 SLF - Updated for Menuet 2.1a and MW4.4a.                  **/
/**     11/06/97 NLW - Modified getloads call to pass setback delta_T  **/
/**                                                                                         **/
/*************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wa_engine.h"

static int iMonthSeason[12];

static void get_distribution_losses(float *fDuctEffHtg, float *fDuctEffClg, float *fDistlossfactor_Htg, float *fDistlossfactor_Clg);

/***************************************************************************
** Function Name: get_base_load
**          Date: October 2, 2001
**     Author(s): Mark Fishbaugher
**
**  DESCRIPTION: Computes pre and post base load electric values using
**  results in the mor array.  mir->Rndx is assumed to point at the
**  'next' entry to the array on entry to this function
**************************************************************************/
void get_base_load(void) {

  mir->fBasecase_Baseload = 0.0f;
  mir->fFinal_Baseload  = 0.0F;

  for (int i = 0; i < mir->Rndx; i++) // all the defined entries
  {
    mir->fBasecase_Baseload  += mir->Results[i].fEnerPreBas; // only baseload measures will have non-zero here
    mir->fFinal_Baseload   += mir->Results[i].fEnerPstBas;
  }
  return;
}

/***************************************************************************
** Function Name: mhea_energy_use
**          Date:
**     Author(s): NREL
**
**  DESCRIPTION: Computes the heating and cooling energy use from
**               the building thermal loads and hvac system data
**               Does not compute the base loads (electric)
**************************************************************************/
void mhea_energy_use(void) {
  float fAirHeatCap; /* Vol. specific heat cap. of air */
  //float fTempOutDay, fTempOutNight, fTempWetbulb;
  float fDH_Day[9], /* Avg Degree-Hours per Day/night */
      fDH_Night[9];
  float fDistlossfactor_Htg; /* Default duct distribution loss factor for heating */
  float fDistlossfactor_Clg; /* Default duct distribution loss factor for cooling */

  //    int     iMonth, iHBin, i;
  
  //enum { COOLING = -1, HEATING = 1 } iSeason;
  int iSeason;
  enum { iFLOOR, iWALL, iWINDOW, iDOOR, iROOF } iComponent;

  float fUA_FLR_S = 0.0, /* UA value for the Floor */
      fUA_FLR_W = 0.0;   /*  in Summer and Winter  */

  float fUA_WAL_S = 0.0,   /* UA value for the Walls */
      fUA_WAL_W = 0.0;     /*  in Summer and Winter  */
  float fSA_WAL_S_N = 0.0, /* Solar Aperture value for the */
      fSA_WAL_S_S = 0.0,   /*  Walls in Summer and Winter  */
      fSA_WAL_S_E = 0.0,   /*  for each orientation        */
      fSA_WAL_S_W = 0.0, fSA_WAL_W_N = 0.0, fSA_WAL_W_S = 0.0, fSA_WAL_W_E = 0.0, fSA_WAL_W_W = 0.0, fSA_WAL_N = 0.0,
        fSA_WAL_S = 0.0, fSA_WAL_E = 0.0, fSA_WAL_W = 0.0;
  float fSG_WAL = 0.0; /* Solar Gain for the Walls */

  float fUA_WIN_S = 0.0,   /* UA value for the Windows */
      fUA_WIN_W = 0.0;     /*  in Summer and Winter   */
  float fSA_WIN_S_N = 0.0, /* Solar Aperture value for the  */
      fSA_WIN_S_S = 0.0,   /*  Windows in Summer and Winter */
      fSA_WIN_S_E = 0.0,   /*  for each orientation         */
      fSA_WIN_S_W = 0.0, fSA_WIN_W_N = 0.0, fSA_WIN_W_S = 0.0, fSA_WIN_W_E = 0.0, fSA_WIN_W_W = 0.0, fSA_WIN_N = 0.0,
        fSA_WIN_S = 0.0, fSA_WIN_E = 0.0, fSA_WIN_W = 0.0;
  float fSG_WIN = 0.0; /* Solar Gain for the Windows */

  float fUA_DOR_S = 0.0,   /* UA values for the Doors */
      fUA_DOR_W = 0.0;     /*  in Summer and Winter   */
  float fSA_DOR_S_N = 0.0, /* Solar Aperture value for the */
      fSA_DOR_S_S = 0.0,   /*  Doors in Summer and Winter  */
      fSA_DOR_S_E = 0.0,   /*  for each orientation          */
      fSA_DOR_S_W = 0.0, fSA_DOR_W_N = 0.0, fSA_DOR_W_S = 0.0, fSA_DOR_W_E = 0.0, fSA_DOR_W_W = 0.0, fSA_DOR_N = 0.0,
        fSA_DOR_S = 0.0, fSA_DOR_E = 0.0, fSA_DOR_W = 0.0;
  float fSG_DOR = 0.0; /* Solar Gain for the Doors */

  float fUA_ROF_S = 0.0, /* UA value for combined Roof & */
      fUA_ROF_W = 0.0,   /*  Ceiling in Summer & Winter  */
      fUA_ROF = 0.0;
  float fSA_ROF_S = 0.0, /* Solar Aperture value for Roof */
      fSA_ROF_W = 0.0,   /*  & Ceiling in Summer & Winter */
      fSA_ROF = 0.0;
  float fSG_ROF = 0.0; /* Solar Gain for the Roof */

  float fVolumeCathCeiling = 0.0, /* Volume of Cathedral Ceiling */
      fVolumeAddition = 0.0;      /* Volume of Home Addition */

  float fShadingRatioL = 0.0, /* Dimensional ratios for calc.      */
      fShadingRatioW2H = 0.0, /*  wall overhang modifier       */
      fShadingRatioAwn = 0.0, /* Dimensional ratios for calc.      */
      fShadingRatioPch = 0.0; /*  window overhang modifier         */

  float fUA_VertS_S = 0.0, /* UA total for vertical surfaces */
      fUA_VertS_W = 0.0,   /*  (walls, windows, doors) S & W */
      fUA_VertS = 0.0;

  float fConductionHeatLoss_S = 0.0, /* Heat Conduction in Summer&Winter */
      fConductionHeatLoss_W = 0.0,   /*  = sum of component UA values    */
      fConductionHeatLoss = 0.0;

  float fFreeHeatDay = 0.0, /* Free Heat (internal gains) */
      /*  during the Day (Btu/h)    */
      fFreeHeatNight = 0.0; /* Free Heat (internal gains) */
  /*  during the Night (Btu/h)  */

  float fValue1_W, fValue2_W, fValue3_W;
  float fSolarStorage = 0.0;   /* Solar Storage (fraction) */
  float fSolarGainTotal = 0.0; /* Solar Gain (Btu/h) */
  float fSkyRadLossDay = 0.0,  /* Sky Radiation Loss (Btu/h) */
      fSkyRadLossNight = 0.0;
  float fInfiltration = 0.0, fQILoss = 0.0, fInfMassFlow = 0.0;
  float fTIndoorAvg = 0.0, fTeff = 0.0, fTeffDay = 0.0, fTeffNight = 0.0;
  int iflgSetBack = FALSE;
  float fTSetDay = 0.0, fTSetNight = 0.0;

  // 8/27/97,NLW: Add variables to retain summer/winter setpoints

  //	float   fTSetDayHTG   = 0.0,
  //		fTSetNightHTG = 0.0;
  //	float   fTSetDayCLG   = 0.0,
  //		fTSetNightCLG = 0.0;

  float fTBalanceDay = 0.0, fTBalanceNight = 0.0;
  float fDegHourDay = 0.0, fDegHourNight = 0.0;
  float fHourLoadDay = 0.0, fHourLoadNight = 0.0;
  float fHtgEnerUseDay = 0.0, fHtgEnerUseNight = 0.0;
  float fClgEnerUseDay = 0.0, fClgEnerUseNight = 0.0;

  float fTotalLoadHtg = 0.0, // used in debug code only
      fTotalLoadClg = 0.0;

  float fDuctEffHtg[2], //   Htg/Clg Duct delivery efficiencies
      fDuctEffClg[2];

  float fDeltaT = 0.0;
  int iSet = 0;
  int iDuctStatus = 0;
  float fDuct_Loss_Factr = 0.0;

  mor->energy_calc_counter++;    // #94

  /****************************************************/
  /*  Initialize passed variables to zero in order to */
  /*  avoid cumulative affects in the retrofits code  */
  /****************************************************/

  *(fDuctEffHtg + 0) = *(fDuctEffHtg + 1) = 1.0f;
  *(fDuctEffClg + 0) = *(fDuctEffClg + 1) = 1.0f;

  // accumulators
  mir->fHeating_Energy = 0.0;
  mir->fCooling_Energy = 0.0;

  /***********************************************************/
  /*  Open file for intermediate calculation results output  */
  /***********************************************************/

  if (cmds.debug_level & D_MHEA_ENERGY_DETAIL)
    fprintf(stderr, "\n----ENERGY USE----\n");

  /*********************************************************/
  /* Calculation for Specific Heat Capacity of Air         */
  /* (See p. 30 for equation and p. 79 for conversion.)       */
  /* fAirHeatCap (Btu/ft^3-F) = 0.3312(W-h/m^3-C) *
  [ 0.0000186(Btu/ft^3-F)/(W-h/m^3-C) ] *
  e^[ -0.000121975 (m^-1) * elev(ft) * 0.305(m/ft) ] */
  /*********************************************************/

  //fAirHeatCap = (float)((0.3312 * 0.0000186) * exp(-0.0001219755 * fElevation * 0.305));
  fAirHeatCap = (float)((0.3312 * 0.0000186) * exp(-0.0001219755 * cwd->altitude * 0.305));

  /**********************************************/
  /*  Building Conduction Heat Loss  (Btu/F-h)  */
  /*   Solar Apertures  (1/ft^2)                */
  /**********************************************/

  ua_floor(&fUA_FLR_S, &fUA_FLR_W);

  ua_wall(&fVolumeAddition, &fUA_WAL_S, &fUA_WAL_W, &fSA_WAL_S_N, &fSA_WAL_S_S, &fSA_WAL_S_E, &fSA_WAL_S_W, &fSA_WAL_W_N,
          &fSA_WAL_W_S, &fSA_WAL_W_E, &fSA_WAL_W_W, &fShadingRatioL, &fShadingRatioW2H);

  ua_window(&fUA_WIN_S, &fUA_WIN_W, &fSA_WIN_S_N, &fSA_WIN_S_S, &fSA_WIN_S_E, &fSA_WIN_S_W, &fSA_WIN_W_N, &fSA_WIN_W_S,
            &fSA_WIN_W_E, &fSA_WIN_W_W, &fShadingRatioAwn, &fShadingRatioPch);

  ua_door(&fUA_DOR_S, &fUA_DOR_W, &fSA_DOR_S_N, &fSA_DOR_S_S, &fSA_DOR_S_E, &fSA_DOR_S_W, &fSA_DOR_W_N, &fSA_DOR_W_S,
          &fSA_DOR_W_E, &fSA_DOR_W_W);

  ua_roof(fAirHeatCap, &fVolumeCathCeiling, &fUA_ROF_S, &fUA_ROF_W, &fSA_ROF_S, &fSA_ROF_W);

  fUA_VertS_S = fUA_WAL_S + fUA_WIN_S + fUA_DOR_S;
  fUA_VertS_W = fUA_WAL_W + fUA_WIN_W + fUA_DOR_W;

  fConductionHeatLoss_S = fUA_FLR_S + fUA_VertS_S + fUA_ROF_S;
  fConductionHeatLoss_W = fUA_FLR_W + fUA_VertS_W + fUA_ROF_W;

  /*****************************************/
  /*  Sizing Calculations  (Btu/h)         */
  /*  NW, 3/26/97                          */
  /*****************************************/
  /** (Future implementation might move these out of the season loop so that
  the season determination calculations are truly only done once. 8/27/97,NLW **/

  if (mir->flgWhichPass == BASE_CASE) {
    iSet = PRE_RETROFIT;  // Pass back base case results set
  } else {
    iSet = POST_RETROFIT; // Pass back retrofit case results set
  }

  fDeltaT = (float)(70.0 - cwd->heating_design_temp);
  mir->Htg_Sizing[iSet].fWal_loss = fUA_WAL_W * fDeltaT;
  mir->Htg_Sizing[iSet].fFlr_loss = fUA_FLR_W * fDeltaT;
  mir->Htg_Sizing[iSet].fRof_loss = fUA_ROF_W * fDeltaT;
  mir->Htg_Sizing[iSet].fWin_loss = fUA_WIN_W * fDeltaT;
  mir->Htg_Sizing[iSet].fDor_loss = fUA_DOR_W * fDeltaT;
  mir->Htg_Sizing[iSet].fTot_loss = 
    mir->Htg_Sizing[iSet].fWal_loss + 
    mir->Htg_Sizing[iSet].fFlr_loss + 
    mir->Htg_Sizing[iSet].fRof_loss +
    mir->Htg_Sizing[iSet].fWin_loss + 
    mir->Htg_Sizing[iSet].fDor_loss;

  /* Infiltration and Duct Loss calculated below */

  if (cmds.debug_level & D_MHEA_ENERGY_DETAIL) {
    fprintf(stderr, "\nUA_WAL_W: %8.1f", fUA_WAL_W);
    fprintf(stderr, "\nUA_WIN_W: %8.1f", fUA_WIN_W);
    fprintf(stderr, "\nUA_DOR_W: %8.1f", fUA_DOR_W);

    fprintf(stderr, "\nUA_WAL_S: %8.1f", fUA_WAL_S);
    fprintf(stderr, "\nUA_WIN_S: %8.1f", fUA_WIN_S);
    fprintf(stderr, "\nUA_DOR_S: %8.1f", fUA_DOR_S);

    fprintf(stderr, "\nfUA_FLR_W: %8.1f", fUA_FLR_W);
    fprintf(stderr, "\nUA_VertS_W: %8.1f", fUA_VertS_W);
    fprintf(stderr, "\nUA_VertS_W: %8.1f", fUA_ROF_W);

    fprintf(stderr, "\nfUA_FLR_S: %8.1f", fUA_FLR_S);
    fprintf(stderr, "\nUA_VertS_S: %8.1f", fUA_VertS_S);
    fprintf(stderr, "\nUA_VertS_S: %8.1f", fUA_ROF_S);

    fprintf(stderr, "\nConduction Heat Loss = %8.1f", fConductionHeatLoss_W);
    fprintf(stderr, "\nWinter Design Temperature =  %8.1f\n", cwd->heating_design_temp);
  }

  /*****************************************/
  /*  Free Heat (Internal Gains)  (Btu/h)  */
  /*****************************************/

  fFreeHeatDay = mdi->key.free_heat_from_interior_sources_day;
  fFreeHeatNight = mdi->key.free_heat_from_interior_sources_night;

  /********************************************/
  /*  Determine if Heating or Cooling Season  */
  /********************************************/

  for (int zMonth = 0; zMonth <= 11; zMonth++) {        // BASE ZERO MONTH <<<<<<=====
    int iMonth = zMonth + 1;    // BASE ONE
    // /***************
    // Get average temperatures from NEAT temperature data.
    // ***************/
    // fTempOutDay =
    //     //(float)((cwd->fTempOutBins[iMonth][2] + cwd->fTempOutBins[iMonth][3] + cwd->fTempOutBins[iMonth][4]) / 3.0);
    //     cwd->avg_daytime_temp[iMonth];
    // fTempOutNight =
    //     //(float)((cwd->fTempOutBins[iMonth][0] + cwd->fTempOutBins[iMonth][1] + cwd->fTempOutBins[iMonth][5]) / 3.0);
    //     cwd->avg_nighttime_temp[iMonth];

    /********************************************************************/
    /*  Prevent night setpoint temp from being below outdoor temp to    */
    /*   prevent singularities. 12/03 MBG                               */
    /********************************************************************/

    if (mir->flgRetrofits[M_CMS_SETBACK_THERMOSTAT])
      mir->fNightSetpoint = MAX((mdi->key.heating_setpoint_night), (cwd->avg_nighttime_temp[iMonth] + 4.0f));
    else
      mir->fNightSetpoint = mdi->key.heating_setpoint_night;

    /***************
    Average UA and SA values.
    ***************/
    fSA_WAL_N = (float)((fSA_WAL_W_N + fSA_WAL_S_N) / 2.0);
    fSA_WAL_S = (float)((fSA_WAL_W_S + fSA_WAL_S_S) / 2.0);
    fSA_WAL_E = (float)((fSA_WAL_W_E + fSA_WAL_S_E) / 2.0);
    fSA_WAL_W = (float)((fSA_WAL_W_W + fSA_WAL_S_W) / 2.0);
    fSA_WIN_N = (float)((fSA_WIN_W_N + fSA_WIN_S_N) / 2.0);
    fSA_WIN_S = (float)((fSA_WIN_W_S + fSA_WIN_S_S) / 2.0);
    fSA_WIN_E = (float)((fSA_WIN_W_E + fSA_WIN_S_E) / 2.0);
    fSA_WIN_W = (float)((fSA_WIN_W_W + fSA_WIN_S_W) / 2.0);
    fSA_DOR_N = (float)((fSA_DOR_W_N + fSA_DOR_S_N) / 2.0);
    fSA_DOR_S = (float)((fSA_DOR_W_S + fSA_DOR_S_S) / 2.0);
    fSA_DOR_E = (float)((fSA_DOR_W_E + fSA_DOR_S_E) / 2.0);
    fSA_DOR_W = (float)((fSA_DOR_W_W + fSA_DOR_S_W) / 2.0);

    fSA_ROF = (float)((fSA_ROF_W + fSA_ROF_S) / 2.0);
    fUA_ROF = (float)((fUA_ROF_W + fUA_ROF_S) / 2.0);
    fUA_VertS = (float)((fUA_VertS_W + fUA_VertS_S) / 2.0);
    fConductionHeatLoss = (float)((fConductionHeatLoss_W + fConductionHeatLoss_S) / 2.0);

    /**************************/
    /*  Solar Gains  (Btu/h)  */
    /**************************/

    iComponent = iWALL;
    fSG_WAL = component_solar_gain(iComponent, iMonth, 0.0, fShadingRatioL, fShadingRatioW2H, cwd->avg_solar_horizontal[iMonth],
                        cwd->solar_load_ratio[iMonth], cwd->north_latitude, fSA_WAL_N, fSA_WAL_S, fSA_WAL_E, fSA_WAL_W, 0.0);

    iComponent = iWINDOW;
    fSG_WIN = component_solar_gain(iComponent, iMonth, mdi->key.ratio_of_awning_depth_to_window_height, fShadingRatioAwn,
                        fShadingRatioPch, cwd->avg_solar_horizontal[iMonth], cwd->solar_load_ratio[iMonth], cwd->north_latitude, fSA_WIN_N, fSA_WIN_S,
                        fSA_WIN_E, fSA_WIN_W, 0.0);

    iComponent = iDOOR;
    fSG_DOR = component_solar_gain(iComponent, iMonth, 0.0, fShadingRatioL, fShadingRatioW2H, cwd->avg_solar_horizontal[iMonth],
                        cwd->solar_load_ratio[iMonth], cwd->north_latitude, fSA_DOR_N, fSA_DOR_S, fSA_DOR_E, fSA_DOR_W, 0.0);

    iComponent = iROOF;
    fSG_ROF = component_solar_gain(iComponent, iMonth, 0.0, 0.0, 0.0, cwd->avg_solar_horizontal[iMonth], cwd->solar_load_ratio[iMonth], cwd->north_latitude, 0.0,
                        0.0, 0.0, 0.0, fSA_ROF);

    fSolarGainTotal = fSG_WAL + fSG_WIN + fSG_DOR + fSG_ROF;

    /**************************************/
    /*  Sky Radiation Heat Loss  (Btu/h)  */
    /**************************************/

    sky_radiation_losses(1, fUA_VertS, fUA_ROF, cwd->avg_daytime_temp[iMonth], cwd->avg_nighttime_temp[iMonth], &fSkyRadLossDay, &fSkyRadLossNight);

    fValue1_W = fSkyRadLossDay;
    fValue2_W = fSkyRadLossNight;

    sky_radiation_losses(-1, fUA_VertS, fUA_ROF, cwd->avg_daytime_temp[iMonth], cwd->avg_nighttime_temp[iMonth], &fSkyRadLossDay, &fSkyRadLossNight);

    fSkyRadLossDay = (float)((fSkyRadLossDay + fValue1_W) / 2.0);
    fSkyRadLossNight = (float)((fSkyRadLossNight + fValue2_W) / 2.0);

    /****** Sky radiation loss eliminated. Gives too large heat losses. MBG 1/13/03 ***/

    fSkyRadLossDay = fSkyRadLossNight = 0.0f;

    /************************************************/
    /*  Building Infiltration Heat Loss  (Btu/h-F)  */
    /*  Infiltration (ACH)                                  */
    /************************************************/

    mhea_infiltration_losses(HEATING, zMonth, cwd->altitude, cwd->fTempOutBins[iMonth], cwd->fWindSpdBins[iMonth], fVolumeCathCeiling,
                    fVolumeAddition, &fInfiltration, &fQILoss, &fInfMassFlow, &iDuctStatus);

    fValue1_W = fInfiltration;
    fValue2_W = fQILoss;
    fValue3_W = fInfMassFlow;

    // NW, 3/26/97: Estimate sizing infiltration and duct loss

    if (iMonth == JANUARY) {
      mir->Htg_Sizing[iSet].fInf_loss = fQILoss * fDeltaT;
      mir->Htg_Sizing[iSet].fTot_loss += mir->Htg_Sizing[iSet].fInf_loss;

      /* Implement abridged Manual J Table7A for duct loss */
      /* Assumes that ducts, if present, are in unheated space
      with supply temp. <= 120F and R4 insulation if insulated */

      if (cwd->heating_design_temp > 15.0) {
        switch (iDuctStatus) {
        case 0:
          fDuct_Loss_Factr = 0.0F; // No Ducts
          break;
        case 1:
          fDuct_Loss_Factr = 0.05F; // Insulated Ducts
          break;
        case 2:
          fDuct_Loss_Factr = 0.15F; // Uninsulated Ducts
          break;
        default:
          fDuct_Loss_Factr = 0.0F; // Default
          break;
        } // End Switch
      } else {
        switch (iDuctStatus) {
        case 0:
          fDuct_Loss_Factr = 0.0F; // No Ducts
          break;
        case 1:
          fDuct_Loss_Factr = 0.10F; // Insulated Ducts
          break;
        case 2:
          fDuct_Loss_Factr = 0.20F; // Uninsulated Ducts
          break;
        default:
          fDuct_Loss_Factr = 0.0F; // Default
          break;
        } // End Switch
      }

      mir->Htg_Sizing[iSet].fDuc_loss = mir->Htg_Sizing[iSet].fTot_loss * fDuct_Loss_Factr;
      mir->Htg_Sizing[iSet].fTot_loss += mir->Htg_Sizing[iSet].fDuc_loss;

      if (cmds.debug_level & D_MHEA_ENERGY_DETAIL) {
        sprintf(mir->sMsg, " Infil Heat Loss  =, %8.1f \n"
                           " Duct Heat Loss   =  %8.1f \n"
                           " Total Heat Loss  =  %8.1f \n",
                mir->Htg_Sizing[iSet].fInf_loss, mir->Htg_Sizing[iSet].fDuc_loss, mir->Htg_Sizing[iSet].fTot_loss);
        fprintf(stderr, "%s", mir->sMsg);
      }
    } // end of iMonth = 0

    mhea_infiltration_losses(COOLING, zMonth, cwd->altitude, cwd->fTempOutBins[iMonth], cwd->fWindSpdBins[iMonth], fVolumeCathCeiling,
                    fVolumeAddition, &fInfiltration, &fQILoss, &fInfMassFlow, &iDuctStatus);

    fInfiltration = (float)((fInfiltration + fValue1_W) / 2.0);
    fQILoss = (float)((fQILoss + fValue2_W) / 2.0);
    fInfMassFlow = (float)((fInfMassFlow + fValue3_W) / 2.0);

    /**************************************/
    /*  Solar Storage Factor  (unitless)  */
    /**************************************/

    mhea_solar_storage(cwd->avg_daytime_temp[iMonth], fSolarGainTotal, fFreeHeatDay, fSkyRadLossDay, fConductionHeatLoss, fQILoss, &fSolarStorage);

    /*************************************/
    /*  Effective Outside Temp  (deg F)  */
    /*************************************/

    get_balance_temp(1.0, cwd->avg_daytime_temp[iMonth], cwd->avg_nighttime_temp[iMonth], fSolarStorage, fSolarGainTotal, fFreeHeatDay, fFreeHeatNight, fSkyRadLossDay,
                   fSkyRadLossNight, fConductionHeatLoss, fQILoss, &fTeffDay, &fTeffNight);

    fTeff = (float)((fTeffDay + fTeffNight) / 2.0);

    /*******************
    SLF 07/20/94 - If there is no heating or no cooling equipment,
    the thermostat temperatures will be zero and skew the average
    indoor temperature.
    *******************/

    // moved setpoints into keyparameters for MHEA 4/3/02, so now
    // we can compute a simple average of setpoints as NEAT does

    fTIndoorAvg = (float)((mdi->key.heating_setpoint_day + mir->fNightSetpoint +
                           mdi->key.cooling_setpoint_day + mdi->key.cooling_setpoint_night) /
                          4.0);

    /***********************
    NLW, 8/26/97: Set season determination only with base run
    (Corrects previous scheme which incorrectly retained last
    value set whether heating or cooling)
    ***********************/
    if (mir->flgWhichPass == BASE_CASE) {
      if (fTeff < fTIndoorAvg) {
        iMonthSeason[zMonth] = HEATING;
        mir->iSeason[zMonth] = HEATING;
      } else {
        iMonthSeason[zMonth] = COOLING;
        mir->iSeason[zMonth] = COOLING;
      }
    } else {
      if (mir->iSeason[zMonth] == HEATING) {
        iMonthSeason[zMonth] = HEATING;
      } else {
        iMonthSeason[zMonth] = COOLING;
      }
    }

    if (cmds.debug_level & D_MHEA_ENERGY_DETAIL) {
      sprintf(mir->sMsg, " Season Determination(after): %d  \n   "
                       "fTeff      =, %4.1f,  "
                       "fTinAvg    =, %4.1f,  "
                       "fTsetday   =, %4.1f,  "
                       "fTsetNight =, %4.1f,  "
                       "iMthSeason =, %d \n",
              zMonth, fTeff, fTIndoorAvg, fTSetDay, fTSetNight, iMonthSeason[zMonth]);
      fprintf(stderr, "%s", mir->sMsg);
    }

  } // end of month loop

  /*********************
  Call duct_efficiency_MHEA() to get ASHRAE 152 P duct delivery efficiencies
  Lets only make the call if duct sealing evaluation is on AND we
  have either heating or cooling duct location specified, Gettings 11/2
  **********************/

  if (mdi->inf.evaluate_duct_sealing == YES)
    if (mdi->htg.duct_location != 0 || mdi->clg.duct_location != 0)
      if (mdi->htg.duct_location != DL_NONE || mdi->clg.duct_location == DL_NONE)
        //iretvalue = duct_efficMHEA(Weather, fElevation, fW_Dsgn_T, fS_Dsgn_T, mir->fQduct50, fDuctEffHtg, fDuctEffClg);
        //duct_efficiency_MHEA(Weather, cwd->altitude, cwd->heating_design_temp, fS_Dsgn_T, mir->fQduct50, fDuctEffHtg, fDuctEffClg);
        duct_efficiency_MHEA(cwd->altitude, cwd->heating_design_temp, cwd->cooling_design_temp, mir->fQduct50, fDuctEffHtg, fDuctEffClg);

  /*********************
  Call get_distribution_losses() to compute default duct distribution loss
  and determine which, default or ASHRAE 152P, should be used
  ********************/

  get_distribution_losses(fDuctEffHtg, fDuctEffClg, &fDistlossfactor_Htg, &fDistlossfactor_Clg);

  /* Since the cooling dist. loss factors have proven unreliable, use the heating
  * factor also for cooling.  fDuctEffClg is the input to get_cooling_consumption() */

  *(fDuctEffClg + 0) = *(fDuctEffHtg + 0), *(fDuctEffClg + 1) = *(fDuctEffHtg + 1);

  /**************************/
  /*  Monthly Calculations  */
  /**************************/

  for (int zMonth = 0; zMonth <= 11; zMonth++) {    // BASE ZERO MONTH <<<<=====
    int iMonth = zMonth + 1;    // BASE ONE

    // /***************
    // Get average temperatures from NEAT weather data.
    // ***************/
    // fTempOutDay =
    //     (float)((cwd->fTempOutBins[iMonth][BIN_08_12] + 
    //              cwd->fTempOutBins[iMonth][BIN_12_16] + 
    //              cwd->fTempOutBins[iMonth][BIN_16_20]) / 3.0);
    // fTempOutNight =
    //     (float)((cwd->fTempOutBins[iMonth][BIN_00_04] + 
    //              cwd->fTempOutBins[iMonth][BIN_04_08] + 
    //              cwd->fTempOutBins[iMonth][BIN_20_24]) / 3.0);

    // fTempWetbulb = 0.0;
    // for (int i = BIN_00_04; i <= BIN_20_24) fTempWetbulb += (float)((Weather[iMonth].fTempWetBins[i]);
    // fTempWetbulb /= 6.0f;

    if (cmds.debug_level & D_MHEA_ENERGY_DETAIL) {
      fprintf(stderr, "\niMonth:%d", iMonth);
      fprintf(stderr, "\ncwd->fTempOutBins[iMonth][BIN_00_04]:%8.3f", cwd->fTempOutBins[iMonth + 1][BIN_00_04]);
      fprintf(stderr, "\ncwd->fTempOutBins[iMonth][BIN_04_08]:%8.3f", cwd->fTempOutBins[iMonth][BIN_04_08]);
      fprintf(stderr, "\ncwd->fTempOutBins[iMonth][BIN_08_12]:%8.3f", cwd->fTempOutBins[iMonth][BIN_08_12]);
      fprintf(stderr, "\ncwd->fTempOutBins[iMonth][BIN_12_16]:%8.3f", cwd->fTempOutBins[iMonth][BIN_12_16]);
      fprintf(stderr, "\ncwd->fTempOutBins[iMonth][BIN_16_20]:%8.3f", cwd->fTempOutBins[iMonth][BIN_16_20]);
      fprintf(stderr, "\ncwd->fTempOutBins[iMonth][BIN_20_24]:%8.3f", cwd->fTempOutBins[iMonth][BIN_20_24]);
      fprintf(stderr, "\ncwd->avg_daytime_temp[iMonth]:%8.3f", cwd->avg_daytime_temp[iMonth]);
      fprintf(stderr, "\ncwd->avg_nighttime_temp[iMonth]:%8.3f", cwd->avg_nighttime_temp[iMonth]);
      fprintf(stderr, "\ncwd->avg_wet_temp[iMonth] :%8.3f", cwd->avg_wet_temp[iMonth] );

    }

    /********************************************************************/
    /*  Prevent night setpoint temp from being below outdoor temp to    */
    /*   prevent singularities. 12/03 MBG                               */
    /********************************************************************/

    if (mir->flgRetrofits[M_CMS_SETBACK_THERMOSTAT])
      mir->fNightSetpoint = MAX((mdi->key.heating_setpoint_night), (cwd->avg_nighttime_temp[iMonth] + 4.0f));
    else
      mir->fNightSetpoint = mdi->key.heating_setpoint_night;

    if (iMonthSeason[zMonth] == HEATING) /* Heating/Winter Month */
    {
      iSeason = HEATING;
      fTSetDay = mdi->key.heating_setpoint_day;
      fTSetNight = mir->fNightSetpoint;

      fSA_WAL_N = fSA_WAL_W_N;
      fSA_WAL_S = fSA_WAL_W_S;
      fSA_WAL_E = fSA_WAL_W_E;
      fSA_WAL_W = fSA_WAL_W_W;
      fSA_WIN_N = fSA_WIN_W_N;
      fSA_WIN_S = fSA_WIN_W_S;
      fSA_WIN_E = fSA_WIN_W_E;
      fSA_WIN_W = fSA_WIN_W_W;
      fSA_DOR_N = fSA_DOR_W_N;
      fSA_DOR_S = fSA_DOR_W_S;
      fSA_DOR_E = fSA_DOR_W_E;
      fSA_DOR_W = fSA_DOR_W_W;

      fSA_ROF = fSA_ROF_W;
      fUA_ROF = fUA_ROF_W;
      fUA_VertS = fUA_VertS_W;
      fConductionHeatLoss = fConductionHeatLoss_W;

      for (int iHBin = DH_BIN_40; iHBin <= DH_BIN_75; iHBin++) {
        // fDH_Day[iHBin] = Weather[iMonth].fHtgDayDH[iHBin];
        // fDH_Night[iHBin] = Weather[iMonth].fHtgNitDH[iHBin];
        fDH_Day[iHBin]   = cwd->degree_hours[DAY][HEATING][iHBin][iMonth];
        fDH_Night[iHBin] = cwd->degree_hours[NIGHT][HEATING][iHBin][iMonth];
      }

    } else /* iMonthSeason[iMonth] == COOLING */ /* Cooling/Summer Month */
    {
      iSeason = COOLING;
      fTSetDay = mdi->key.cooling_setpoint_day;
      fTSetNight = mdi->key.cooling_setpoint_night;

      fSA_WAL_N = fSA_WAL_S_N;
      fSA_WAL_S = fSA_WAL_S_S;
      fSA_WAL_E = fSA_WAL_S_E;
      fSA_WAL_W = fSA_WAL_S_W;
      fSA_WIN_N = fSA_WIN_S_N;
      fSA_WIN_S = fSA_WIN_S_S;
      fSA_WIN_E = fSA_WIN_S_E;
      fSA_WIN_W = fSA_WIN_S_W;
      fSA_DOR_N = fSA_DOR_S_N;
      fSA_DOR_S = fSA_DOR_S_S;
      fSA_DOR_E = fSA_DOR_S_E;
      fSA_DOR_W = fSA_DOR_S_W;

      fSA_ROF = fSA_ROF_S;
      fUA_ROF = fUA_ROF_S;
      fUA_VertS = fUA_VertS_S;
      fConductionHeatLoss = fConductionHeatLoss_S;

      for (int iHBin = DH_BIN_40; iHBin <= DH_BIN_75; iHBin++) {
        //fDH_Day[iHBin] = Weather[iMonth].fClgDayDH[iHBin];
        //fDH_Night[iHBin] = Weather[iMonth].fClgNitDH[iHBin];
        fDH_Day[iHBin]   = cwd->degree_hours[DAY][COOLING][iHBin][iMonth];
        fDH_Night[iHBin] = cwd->degree_hours[NIGHT][COOLING][iHBin][iMonth];
      }
    }

    if (cmds.debug_level & D_MHEA_ENERGY_DETAIL) {
      if (iSeason == HEATING) {
        sprintf(mir->sMsg, " Heating UA Values for Month:  %d  \n   "
                         "FLR_W =, %4.1f,  "
                         "WAL_W =, %4.1f,  "
                         "WIN_W =, %4.1f,  "
                         "DOR_W =, %4.1f,  "
                         "ROF_W =, %4.1f,  "
                         "Total =, %5.1f \n",
                iMonth, fUA_FLR_W, fUA_WAL_W, fUA_WIN_W, fUA_DOR_W, fUA_ROF, fConductionHeatLoss);
        fprintf(stderr, "%s", mir->sMsg);
      }
      if (iSeason == COOLING) {
        sprintf(mir->sMsg, " Cooling UA Values for Month:  %d  \n   "
                         "FLR_S =, %4.1f,  "
                         "WAL_S =, %4.1f,  "
                         "WIN_S =, %4.1f,  "
                         "DOR_S =, %4.1f,  "
                         "ROF_S =, %4.1f,  "
                         "Total =, %5.1f \n",
                iMonth, fUA_FLR_S, fUA_WAL_S, fUA_WIN_S, fUA_DOR_S, fUA_ROF, fConductionHeatLoss);
        fprintf(stderr, "%s", mir->sMsg);
      }

      sprintf(mir->sMsg, " Solar Aperture Values \n"
                       "WAL N, S, E, W =, %4.1f, %4.1f, %4.1f, %4.1f \n"
                       "WIN N, S, E, W =, %4.1f, %4.1f, %4.1f, %4.1f \n"
                       "DOR N, S, E, W =, %4.1f, %4.1f, %4.1f, %4.1f \n"
                       "ROF =, %4.1f \n",
              fSA_WAL_N, fSA_WAL_S, fSA_WAL_E, fSA_WAL_W, fSA_WIN_N, fSA_WIN_S, fSA_WIN_E, fSA_WIN_W, fSA_DOR_N, fSA_DOR_S,
              fSA_DOR_E, fSA_DOR_W, fSA_ROF);
      fprintf(stderr, "%s", mir->sMsg);
    }

    /**************************/
    /*  Solar Gains  (Btu/h)  */
    /**************************/

    iComponent = iWALL;
    fSG_WAL = component_solar_gain(iComponent, iMonth, 0.0, fShadingRatioL, fShadingRatioW2H, cwd->avg_solar_horizontal[iMonth],
                        cwd->solar_load_ratio[iMonth], cwd->north_latitude, fSA_WAL_N, fSA_WAL_S, fSA_WAL_E, fSA_WAL_W, 0.0);

    iComponent = iWINDOW;
    fSG_WIN = component_solar_gain(iComponent, iMonth, mdi->key.ratio_of_awning_depth_to_window_height, fShadingRatioAwn,
                        fShadingRatioPch, cwd->avg_solar_horizontal[iMonth], cwd->solar_load_ratio[iMonth], cwd->north_latitude, fSA_WIN_N, fSA_WIN_S,
                        fSA_WIN_E, fSA_WIN_W, 0.0);

    iComponent = iDOOR;
    fSG_DOR = component_solar_gain(iComponent, iMonth, 0.0, fShadingRatioL, fShadingRatioW2H, cwd->avg_solar_horizontal[iMonth],
                        cwd->solar_load_ratio[iMonth], cwd->north_latitude, fSA_DOR_N, fSA_DOR_S, fSA_DOR_E, fSA_DOR_W, 0.0);

    iComponent = iROOF;
    fSG_ROF = component_solar_gain(iComponent, iMonth, 0.0, 0.0, 0.0, cwd->avg_solar_horizontal[iMonth], cwd->solar_load_ratio[iMonth], cwd->north_latitude, 0.0,
                        0.0, 0.0, 0.0, fSA_ROF);

    fSolarGainTotal = fSG_WAL + fSG_WIN + fSG_DOR + fSG_ROF;

    if (cmds.debug_level & D_MHEA_ENERGY_DETAIL) {
      sprintf(mir->sMsg, " Solar Data for Month:  %d  \n"
                       "   fSolarFluxHoriz =, %6.1f,"
                       "   Solar Load Ratio =, %6.3f,\n",
              iMonth, cwd->avg_solar_horizontal[iMonth], cwd->solar_load_ratio[iMonth][SLR_NORTH]);
      fprintf(stderr, "%s", mir->sMsg);
      sprintf(mir->sMsg, " Solar Gain values for Month:  %d  \n   "
                       "WAL =, %4.1f,  "
                       "WIN =, %4.1f,  "
                       "DOR =, %4.1f,  "
                       "ROF =, %4.1f,  "
                       "Total =, %5.1f \n",
              iMonth, fSG_WAL, fSG_WIN, fSG_DOR, fSG_ROF, fSolarGainTotal);
      fprintf(stderr, "%s", mir->sMsg);
    }

    /**************************************/
    /*  Sky Radiation Heat Loss  (Btu/h)  */
    /**************************************/

    sky_radiation_losses(iSeason, fUA_VertS, fUA_ROF, cwd->avg_daytime_temp[iMonth], cwd->avg_nighttime_temp[iMonth], &fSkyRadLossDay, &fSkyRadLossNight);

    /****** Sky radiation loss eliminated. Gives too large heat losses. MBG 1/13/03 ***/

    fSkyRadLossDay = fSkyRadLossNight = 0.0f;

    /************************************************/
    /*  Building Infiltration Heat Loss  (Btu/h-F)  */
    /*  Infiltration (ACH)                                  */
    /************************************************/

    mhea_infiltration_losses(iSeason, zMonth, cwd->altitude, cwd->fTempOutBins[iMonth], cwd->fWindSpdBins[iMonth], fVolumeCathCeiling,
                    fVolumeAddition, &fInfiltration, &fQILoss, &fInfMassFlow, &iDuctStatus);

    /**************************************/
    /*  Solar Storage Factor  (unitless)  */
    /**************************************/

    mhea_solar_storage(cwd->avg_daytime_temp[iMonth], fSolarGainTotal, fFreeHeatDay, fSkyRadLossDay, fConductionHeatLoss, fQILoss, &fSolarStorage);

    if (cmds.debug_level & D_MHEA_ENERGY_DETAIL) {
      sprintf(mir->sMsg, "  Month:  %d  \n"
                       " Internal Gains for Day, Night =, %6.1f,  %6.1f \n"
                       " Sky Radiation Loss for Day, Night =, %6.1f,  %6.1f \n"
                       " Infiltration ACH  =,  %6.3f \n"
                       " Infiltration Loss  =,  %6.1f \n"
                       " Solar Storage Factor =,  %6.3f \n",
              iMonth, fFreeHeatDay, fFreeHeatNight, fSkyRadLossDay, fSkyRadLossNight, fInfiltration, fQILoss, fSolarStorage);
      fprintf(stderr, "%s", mir->sMsg);
    }

    /****************************************/
    /*  Balance Point Temperature  (deg F)  */
    /****************************************/

    get_balance_temp(-1.0, fTSetDay, fTSetNight, fSolarStorage, fSolarGainTotal, fFreeHeatDay, fFreeHeatNight, fSkyRadLossDay,
                   fSkyRadLossNight, fConductionHeatLoss, fQILoss, &fTBalanceDay, &fTBalanceNight);

    /**********************************/
    /*  Average degree hour  (deg F)  */
    /**********************************/

    get_degree_hours(cwd->days_in_month[iMonth], cwd->balance_point_temp, fDH_Day, fDH_Night, fTBalanceDay, fTBalanceNight, &fDegHourDay,
                &fDegHourNight);

    /**********************************/
    /*  Average hourly load  (Btu/h)  */
    /**********************************/

    get_hourly_loads(iSeason, mdi->key.length_of_night_thermostat_setback, fConductionHeatLoss, fQILoss, fDegHourDay,
             fDegHourNight, &iflgSetBack, mdi->key.thermostat_setback_amount, &fHourLoadDay, &fHourLoadNight);

    /*************************************************/
    /*  Energy Consumption for Heating  (Btu/month)  */
    /*  (Values are passed in English units          */
    /*    and calculations are in metric units)       */
    /*************************************************/

    get_heating_consumption(iSeason, iflgSetBack, zMonth, cwd->days_in_month[iMonth], cwd->altitude, cwd->avg_daytime_temp[iMonth], cwd->avg_nighttime_temp[iMonth], fHourLoadDay,
                   fHourLoadNight, fDistlossfactor_Htg, &fHtgEnerUseDay, &fHtgEnerUseNight);

    /*************************************************/
    /*  Energy Consumption for Cooling  (Btu/month)  */
    /*  (Values are passed in English units          */
    /*    and calculations are in metric units)       */
    /*************************************************/

    get_cooling_consumption(iSeason, iflgSetBack, zMonth, cwd->days_in_month[iMonth], cwd->altitude, fVolumeCathCeiling, fVolumeAddition, fInfMassFlow,
                   cwd->avg_wet_temp[iMonth], cwd->avg_daytime_temp[iMonth], cwd->avg_nighttime_temp[iMonth], fHourLoadDay, fHourLoadNight, fDuctEffClg, &fClgEnerUseDay,
                   &fClgEnerUseNight);

    /**********************
    These numbers are being fixed for intermediate reporting
    purposes only so that they are consistent with EEDO.
    **********************/
    if (fDegHourDay < 0.0)
      fDegHourDay = 0.0;
    if (fDegHourNight < 0.0)
      fDegHourNight = 0.0;

    if (fHourLoadDay < 0.0)
      fHourLoadDay = 0.0;
    if (fHourLoadNight < 0.0)
      fHourLoadNight = 0.0;

    sprintf(mir->sMsg, "   Month:  %d      Season:, %d  \n"
                     " Set Back Flag  =,  %d \n"
                     " Average Inside Temp Day, Night =,  %4.1f,  %4.1f \n"
                     " Balance Temperature for Day, Night =, %4.1f, %4.1f \n"
                     " Degree Hour for Day, Night =,  %8.1f,  %8.1f \n",
            iMonth, iSeason, iflgSetBack, fTSetDay, fTSetNight, fTBalanceDay, fTBalanceNight, fDegHourDay, fDegHourNight);

    if (cmds.debug_level & D_MHEA_ENERGY_DETAIL)
      fprintf(stderr, "%s", mir->sMsg);

    sprintf(mir->sMsg, "Hourly Load for Day, Night(kBtu) =,  %8.1f,  %8.1f \n"
                     " Htg Energy Use Day, Night(kBtu) =,  %8.1f,  %8.1f \n"
                     " Clg Energy Use Day, Night(kBtu) =,  %8.1f,  %8.1f \n\n",
            fHourLoadDay/1000, fHourLoadNight/1000, fHtgEnerUseDay/1000, fHtgEnerUseNight/1000, fClgEnerUseDay/1000, fClgEnerUseNight/1000);

    if (cmds.debug_level & D_MHEA_ENERGY_DETAIL)
      fprintf(stderr, "%s", mir->sMsg);

    if (iSeason == HEATING)
      fTotalLoadHtg += (float)((fHourLoadDay * 12.0 * cwd->days_in_month[iMonth]) + (fHourLoadNight * 12.0 * cwd->days_in_month[iMonth]));
    if (iSeason == COOLING)
      fTotalLoadClg += (float)((fHourLoadDay * 12.0 * cwd->days_in_month[iMonth]) + (fHourLoadNight * 12.0 * cwd->days_in_month[iMonth]));

    // increment
    mir->fHeating_Energy += fHtgEnerUseDay + fHtgEnerUseNight;
    mir->fCooling_Energy += fClgEnerUseDay + fClgEnerUseNight;

    // The monthly heating and cooling energy use values are
    // only used for billing comparison, so we only want to fill
    // these in for the basecase or first pass

    if (mir->flgWhichPass == BASE_CASE) {

      mir->fMonthlyHtgUse[zMonth] = fHtgEnerUseDay + fHtgEnerUseNight;
      mir->fMonthlyClgUse[zMonth] = fClgEnerUseDay + fClgEnerUseNight;

      if (cmds.debug_level & D_MHEA_ENERGY_DETAIL) {
        sprintf(mir->sMsg, " Monthly Heating Use[%d](kBtu) =,  %8.1f \n"
                         " Monthly Cooling Use[%d](kBtu) =,  %8.1f \n\n",
                zMonth, mir->fMonthlyHtgUse[zMonth]/1000, zMonth, mir->fMonthlyClgUse[zMonth]/1000);
        fprintf(stderr, "%s", mir->sMsg);
      }
    }

  } // end for "for( imonth" loop

  if (cmds.debug_level & D_MHEA_ENERGY_DETAIL) {
    sprintf(mir->sMsg, " Annual Heating Load(kBtu) =,  %8.1f \n"
                       " Annual Cooling Load(kBtu) =,  %8.1f \n"
                       " Annual _Htg_ Energy Use(kBtu) =,  %8.1f \n"
                       " Annual _Clg_ Energy Use(kBtu) =,  %8.1f \n\n",
            fTotalLoadHtg/1000, 
            fTotalLoadClg/1000, 
            mir->fHeating_Energy/1000, 
            mir->fCooling_Energy/1000);
    fprintf(stderr, "%s\n%s", "\nmhea_energy_use", mir->sMsg);
  }

  return;
}

static float fDistlossfactor_old; // Standard value of distribution loss factor for debugging

/***************************************************************************
** Function Name: get_distribution_losses
**          Date: 11/13/01
**     Author(s): NREL/MBG
**
**  DESCRIPTION: Computes the default heating duct distribution loss factor
**               as in the original NREL MHEA. Extracted from get_heating_consumption
**************************************************************************/

static void get_distribution_losses(float *fDuctEffHtg, float *fDuctEffClg, float *fDistlossfactor_Htg, float *fDistlossfactor_Clg) {
  float fHtgDuctLocation, /* HDL values (see p. 43.1), unitless */
      fHtgDuctInsulation, /* HDI values (see p. 43.1), percent */
      fDistlossfactor;    /* Distribution loss factor due to ducts */

  /******************
  Heating duct location loss factor
  ******************/
  if (mdi->htg.duct_location == DL_FLOOR)
    fHtgDuctLocation = 0.50;
  else if (mdi->htg.duct_location == DL_CEILING)
    fHtgDuctLocation = 0.75;
  else
    fHtgDuctLocation = 0.0;

  /******************
  Heating duct insulation loss factor
  ******************/
  fHtgDuctInsulation = 50.0;
  if (mdi->htg.duct_insl == DI_NONE) { //  No duct insulation
    fHtgDuctInsulation = 40.0;
    if (mdi->htg.duct_location == DL_FLOOR && //  Ducts in floor with poor belly wrap
        mdi->flr.belly_condition == BC_POOR)
      fHtgDuctInsulation = 50.0;
  }

  else if (mdi->htg.duct_insl == DI_BELOW) { //  Insulation below ducts
    if (mdi->htg.duct_location == DL_FLOOR)  //  Ducts in belly
      fHtgDuctInsulation = 30.0;
    else if (mdi->htg.duct_location == DL_CEILING) //  Ducts in ceilng
      fHtgDuctInsulation = 50.0;
  }

  else if (mdi->htg.duct_insl == DI_ABOVE) {  //  Insulation above ducts
    if (mdi->htg.duct_location == DL_FLOOR) { //  Ducts in floor
      fHtgDuctInsulation = 50.0;
      if (mdi->flr.belly_condition == BC_POOR) // Belly wrap is poor
        fHtgDuctInsulation = 60.0;
    } else if (mdi->htg.duct_location == DL_CEILING) //  Ducts in ceiling
      fHtgDuctInsulation = 30.0;
  }

  else // Insulation around ducts
  {
    if (mdi->htg.duct_location == DL_FLOOR) //  Ducts in floor
    {

      if ((mdi->flr.belly_mineral_insl >= 2.0) ||
          ((mdi->flr.belly_mineral_insl + mdi->flr.belly_loose_insl) >= mdi->flr.belly_depth))
        fHtgDuctInsulation = 10.0; // for 2" insulation or more
      else
        fHtgDuctInsulation = 20.0; // for 1" insulation
    }

    if (mdi->htg.duct_location == DL_CEILING) //  Ducts in ceiling
    {
      if ((mdi->rof.mineral_insl >= 2.0) || ((mdi->rof.mineral_insl + mdi->rof.loose_insl) >= 8.0))
        fHtgDuctInsulation = 10.0; // for 2" insulation or more */
      else
        fHtgDuctInsulation = 20.0; // for 1" insulation */
    }
  }

  /****************************
  Determine overall distribution loss factor
  ******************************/

  fDistlossfactor = (float)(fHtgDuctLocation * fHtgDuctInsulation / 100.0);

  // 5/8/95, NW - added an adjustment factor of 0.40 to reduce maximum distribution
  //          loss factor.  Per NREL TP-253-4490 (page 12) the worst-case effective
  //          furnace efficiency should be approx. 56%.

  // NW,8/31/95: Update adjustment factor to 0.55 after correction of billing
  //      adjustment problem which biased previous measure savings results

  fDistlossfactor *= 0.30F;

  // NW,9/5/95: Update adjustment factor to 0.20 after correction of billing
  //      adjustment problem which biased previous measure savings results
  //  Also move attenuation of combined location and insulation correction ahead
  //  of application of seal_duct adjustments
  // NW, 9/6/95: Returned to 30% factor above after correcting belly UA calcs.

  /* Temporary calculation of old duct distribution loss factor for comparison
  * with new from ASHRAE 152P */

  fDistlossfactor_old = fDistlossfactor; // Save standard for comparison
  if (mir->flgRetrofits[M_CMS_SEAL_DUCTS])
    fDistlossfactor_old *= (float)(1.0 - (mdi->key.duct_sealing_distribution_loss_reduction / 100.0));

  /* If user has requested ducts to be evaluated from his input data, replace
  *   constant duct loss factors with those computed from ASHREA 152P
  *    MBG 11/7/01  */

  if (mdi->inf.evaluate_duct_sealing == YES) { // Duct measurements available
    if (mir->flgRetrofits[M_CMS_SEAL_DUCTS])     // Duct sealing measure active
      fDistlossfactor = 1.0f - *(fDuctEffHtg + 1);
    else // Duct sealing measure not active
      fDistlossfactor = 1.0f - *(fDuctEffHtg + 0);
  }

  else { // Use standard duct adjustments
    if (mir->flgRetrofits[M_CMS_SEAL_DUCTS])
      fDistlossfactor *= (float)(1.0 - (mdi->key.duct_sealing_distribution_loss_reduction / 100.0));
  }
  // NW, 9/6/95: Disable duct loss credit due to overstated savings
  //      as currently formulated
  // MJF 9/01: we need to revisit these duct loss factor credits
  //      or replace them with new duct calculations.  Note that
  //      duct loss factors are computed and adjusted in several
  //      places in this module, so the corrections below may need
  //      to be copied where ever fDistlossfactor is computed

  // if( (mir->flgRetrofits[M_CMS_BELLY_CELLULOSE_LOOSE_INSL] ||
  //      mir->flgRetrofits[M_CMS_BELLY_FIBERGLASS_LOOSE_INSL]) &&
  //      mdi->htg.duct_location == DL_FLOOR )
  //     fDistlossfactor *= 1.0 -
  //     (mdi->key.duct_insulation_dist_loss_reduction / 100.0);

  // if( (mir->flgRetrofits[M_CMS_ROOF_CELLULOSE_LOOSE_INSL] ||
  //      mir->flgRetrofits[M_CMS_ROOF_FIBERGLASS_LOOSE_INSL]) &&
  //      mdi->htg.duct_location == DL_CEILING )
  //     fDistlossfactor *= 1.0 -
  //     (mdi->key.duct_insulation_dist_loss_reduction / 100.0);

  *fDistlossfactor_Htg = fDistlossfactor;
  *fDistlossfactor_Clg = 0.0f;
  //        printf("\nfDistlossfactor_old = %6.4f\nfDistlossfactor = %6.4f\n",
  //                  fDistlossfactor_old, fDistlossfactor);
}
