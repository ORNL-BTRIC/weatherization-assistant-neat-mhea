/******************  MODULE NAME:  LOADS.C  ******************************/
/**         DATE:  3/14/93                                              **/
/**           BY:    SLF                                                **/
/**  DESCRIPTION:    Computes (per month)                               **/
/**                  Variable-based Degree Hours                        **/
/**                  Loads (for Heating & Cooling)   (English Units)    **/
/**    REVISIONS:                                                       **/
/**         11/3/97,NLW - Revised test for setback                      **/
/**                                                                     **/
/*************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wa_engine.h"

/*******************  FUNCTION NAME:  get_degree_hours   **********************/
/**         DATE:  3/13/93                                              **/
/**           BY:    NW                                                 **/
/**  DESCRIPTION:    Computes Degree Hours (English units)              **/
/**                  See page 58 in Notes by Sheila Hayter.             **/
/**  This routine calculates the day and night degree hours.            **/
/**  REVISIONS:  03/24/94 SLF - Completely changed for NEAT weather.    **/
/**  Note:  There are two sets of DD emperical constants depending      **/
/**         whether it is the heating or cooling season.  It is assumed **/
/**         that the calling routine is passing the appropriate         **/
/**         season's coefficients extracted from the city weather file. **/
/*************************************************************************/

void get_degree_hours(float fNumDays, float fBaseTemps[], float fDH_Day[], float fDH_Night[], float fTBalanceDay, float fTBalanceNight,
                 float *fDegHourDay, float *fDegHourNight) {
  /***************************************************/
  /*  Variables passed in Degree Hour Calculation     */
  /***************************************************/
  /****************
  fBaseTemps[]        array of base temperatures
  fDH_Day[]           array of daytime degree hours
  fDH_Night[]         array of nighttime degree hours
  fTIndoorDay         computed avg. indoor day temp. (deg F)
  fTIndoorNight       computed avg. indoor night temp. (deg F)
  fTBalanceDay      balance temp (def F)
  fTBalanceNight    balance temp (def F)
  fDegHourDay         average degree-hours during daytime
  fDegHourNight       average degree-hours during nighttime
  *****************/

  float fDegHoursD, fDegHoursN;
  int top_bin = DH_BIN_75;

  if (fTBalanceDay <= fBaseTemps[DH_BIN_40])
    fDegHoursD = fDH_Day[DH_BIN_40];
  else if (fTBalanceDay >= fBaseTemps[top_bin])
    fDegHoursD = fDH_Day[top_bin];
  else {
    for (int bin = DH_BIN_40; bin < top_bin; bin++) {
      if (fTBalanceDay > fBaseTemps[bin] && fTBalanceDay <= fBaseTemps[bin + 1]) {
        //fDegHoursD =fDH_Day[bin] - ((fDH_Day[bin] - fDH_Day[bin + 1]) * ((fBaseTemps[bin] - fTBalanceDay) / (fBaseTemps[bin] - fBaseTemps[bin + 1])));
        fDegHoursD =fDH_Day[bin] + ((fDH_Day[bin + 1] - fDH_Day[bin]) * ((fTBalanceDay - fBaseTemps[bin]) / (fBaseTemps[bin + 1] - fBaseTemps[bin])));
        break;
      }
    }
  }

  if (fTBalanceNight <= fBaseTemps[DH_BIN_40])
    fDegHoursN = fDH_Night[DH_BIN_40];
  else if (fTBalanceNight >= fBaseTemps[top_bin])
    fDegHoursN = fDH_Night[top_bin];
  else {
    for (int bin = DH_BIN_40; bin < top_bin; bin++) {
      if (fTBalanceNight > fBaseTemps[bin] && fTBalanceNight <= fBaseTemps[bin + 1]) {
        //fDegHoursN = fDH_Night[bin] - ((fDH_Night[bin] - fDH_Night[bin + 1]) * ((fBaseTemps[bin] - fTBalanceNight) / (fBaseTemps[bin] - fBaseTemps[bin + 1])));
        fDegHoursN = fDH_Night[bin] + ((fDH_Night[bin + 1] - fDH_Night[bin]) * ((fTBalanceNight - fBaseTemps[bin]) / (fBaseTemps[bin + 1] - fBaseTemps[bin])));
        break;
      }
    }
  }

  ASSERT((fNumDays * 12.0) != 0, sprintf(msg, "Assertion Failure"));
  *fDegHourDay = (float)(fDegHoursD / (fNumDays * 12.0));
  *fDegHourNight = (float)(fDegHoursN / (fNumDays * 12.0));

  return;
}

/*******************  FUNCTION NAME:  get_hourly_loads   *************************/
/**         DATE:  3/13/93                                                              **/
/**           BY:    NW                                                                 **/
/**  DESCRIPTION:    See page 59 in Notes by Sheila Hayter.                 **/
/**                                                                                         **/
/**  This routine calculates hourly htg/clg loads (English Units).      **/
/*************************************************************************/

int get_hourly_loads(int iflgSeason, float fNightLength, float fCondLoss, float fInfilLoss, float fDegHourDay, float fDegHourNight,
             int *flgSetBack, float fTempDelta, float *fHourLoadDay, float *fHourLoadNight) {
  /***************************************************************/
  /*  Variables passed in Heating/Cooling Load Calculation       */
  /***************************************************************/
  /****************
  fCondLoss         Total building conduction loss (Btu/h-F)
  fInfilLoss        Total building infiltration loss (Btu/h-F)
  fDegHourDay         daytime degree-hours  (deg F)
  fDegHourNight     nighttime degree-hours (deg F)
  fHourLoadDay        computed daytime hourly load
  fHourLoadNight      computed nighttime hourly load
  ****************/

  float fTSetDay, // Heating or Cooling setpoints.
      fTSetNight; // (User input Average Indoor Temperatures (F))
  float fBLCtmp;  // scratch variable

  /************************
  Determine whether or not there is thermostat setback/up.
  *************************/
  if (iflgSeason == HEATING)
  {

    /****************
    If the heating equipment is none, coal, or wood,
    then there can be no thermostat control.
    *****************/
    if (mdi->htg.equip_type == ET_NONE || mdi->htg.fuel_type == COAL || mdi->htg.fuel_type == WOOD) {
      *flgSetBack = FALSE;
    } else {
      //fTSetDay = mdi->key.heating_setpoint_day;
      //fTSetNight = mir->fNightSetpoint;

      // 11/3/97, NLW:  Update setback test to require at least a five degree F difference.
      //          if( fTSetDay == fTSetNight )
      //          if( fabs(fTSetDay - fTSetNight) < fTempDelta )

      if (mir->flgRetrofits[M_CMS_SETBACK_THERMOSTAT])
        *flgSetBack = TRUE;
      else
        *flgSetBack = FALSE;
    }
  }

  else // It's the cooling season.
  {
    /********************
    Open the Primary Cooling data file and read in the structure.
    ********************/

    if (mir->flgNoCLG == FALSE) // there is cooling equipment
    {
      /****************
      If the cooling equipment is none or evaporative,
      then there can be no thermostat control.
      *****************/

      if (mdi->clg.equip_type == CE_NONE || mdi->clg.equip_type == CE_EVAPORATIVE) {
        *flgSetBack = FALSE;
      } else {
        fTSetDay = mdi->key.cooling_setpoint_day;
        fTSetNight = mdi->key.cooling_setpoint_night;

        // 11/3/97, NLW:  Update setback test to require at least a five degree F difference.
        //          if( fTSetDay == fTSetNight )
        //          if( (fTSetNight - fTSetDay) < 5.0 )

        if (fabs(fTSetDay - fTSetNight) < fTempDelta)
          *flgSetBack = FALSE;
        else
          *flgSetBack = TRUE;
      }
    } else {
      /****************
      If there is no CLG file, then there can be no thermostat control
      and no loads.
      *****************/
      *flgSetBack = FALSE;
      *fHourLoadDay = 0.0;
      *fHourLoadNight = 0.0;

      return (OK);
    }

  }

  /************************
  Calculate average hourly loads.
  *************************/

  fBLCtmp = fCondLoss + fInfilLoss;
  *fHourLoadDay = fBLCtmp * fDegHourDay;
  *fHourLoadNight = fBLCtmp * fDegHourNight;

  return (OK);
}
