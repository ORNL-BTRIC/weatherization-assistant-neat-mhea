/******************  MODULE NAME:  CONSMPTN.C  ***************************/
/**         DATE:  3/14/93                                              **/
/**           BY:    SLF                                                **/
/**  DESCRIPTION:    Computes (per month)                               **/
/**                         Energy Consumption for Heating              **/
/**                         Energy Consumption for Cooling              **/
/**                                                                     **/
/** NW,8/31/95  Adjusted Distr. Loss Factor                             **/
/** NW,9/6/95   Adjusted Distr. Loss Factor                             **/
/** NW,9/6/95   Disabled Floor adjustments to Dist. Loss                **/
/** NW,11/9/97  Adjust HTG and CLG setback calculations                 **/
/**                                                                     **/
/*************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wa_engine.h"

/*******************  FUNCTION NAME:  get_heating_consumption  **********************/
/**       DATE: 3/12/93                                                   **/
/**              BY:    JP/SLF/NW                                         **/
/** DESCRIPTION:    See pages 60 - 66 in Notes by Sheila Hayter.          **/
/** REVISIONS:  11/04/94 SLF - Added Kerosene heating                     **/
/**                 11/16/94 SLF - Reordered mir->flgRetrofits.             **/
/**     5/8/95, NW - modified get_heating_consumption for minimum effective        **/
/**                     heating efficiency of 56% per NREL/TP-253-4490,   **/
/**                     page 12.                                          **/
/**     11/03-1/04 MBG - Changes to allow setback duration to have effect **/
/**      4/08  MBG  -  modified to allow adjustment of heat pump hspf to  **/
/**                     climate differences using design temperature.     **/
/***************************************************************************/
void get_heating_consumption(int iSeason, int iflgSetBack, int zMonth, float fNumDays, float fElevation, float fTempOut_Day,
                    float fTempOut_Night, float fHourLoadDay, float fHourLoadNight, float fDistlossfactor, float *fHtgEnerUse_Day,
                    float *fHtgEnerUse_Night) {

  int iMonth = zMonth + 1;   // base 1 from base 0 month
  /*****************************************************************/
  /*  Variables used in Heating Equipment Energy Calculation       */
  /*****************************************************************/

  float fPercentPrimaryHeat; // Fraction of supplied by primary system 
  float fAltAdjust;          // Efficiency fraction derated for altitude 
  float fDayHours,           // Number of hours in "day"   
      fNightHours;           // Number of hours in "night" 
  float fTInsideDay,         // Inside daytime temperature (deg C) 
      fTInsideNight,         // Inside nighttime temperature (deg C) 
      fTOutsideDay,          // Average outdoor daytime temperature (C)  
      fTOutsideNight;        // Average outdoor nighttime temperature (C) 
  float fInCapacity,         // Input Capacity of heating equipment (W) 
      fOutCapacity;          // Ouput Capacity of heating equipment (W) 
                             //  float fAdjustEff1,          // Efficiency adjustment for retrofit 
                             //        fAdjustEff2,
  float fEfficiency;         // Rated or steady state efficiency (no units), or COP for Heat Pumps

  float fSensibleHtgLoad_Day, fSensibleHtgLoad_Night, fLoadSB;

  float fNightSetback;  // Temporary until Setup parameter is changed

  float fFanpower;      // Power of circulating fan (W)

  float fHeatpumpTFDay = 0;     // Emperical Function correlating Heatpump 
                                //  capacity w/ out & in Temps. for Days   
  float fHeatpumpTFNight = 0;   // Emperical Function correlating Heatpump 
                                //  capacity w/ out & in Temps for Nights  
  float fHeatpumpCapDay = 0;    // Heatpump output Capacity adjusted for   
                                // inside & outside temps. for Days (MMBtu) 
  float fHeatpumpCapNight = 0;  // Heatpump output Capacity adjusted for     
                                // inside & outside temps. for Nights (MMBtu) 

  float fPLRDay;    // Part load ratio for the day (no units) 
  float fPLRNight;  // Part load ratio for the night (no units) 
                    //  float fOHtgEffDay;          // Overall Heating Efficiency 
                    //                                      //  for Days (no units)       
                    //  float fOHtgEffNight;            // Overall Heating Efficiency 
                    //                                      //  for Nights (no units)      
  float fPhi2Day,   // Equipment specific functions describing 
      fPhi3Day;     //  the dependence on part load ratio for  
                    //  the day (no units)                     
  float fPhi2Night, // Equipment specific functions describing 
      fPhi3Night;   //  the dependence on part load ratio for  
                    //  the night (no units)                   
  float fPhi3SB;    // Interpolated value of Phi3 for night hours 
                    // w/o setback when SB is implemented 
  float fHours;     // scratch variable for setback calcs 

  /******************
  Calculating efficiency derating factor for altitude.
  ******************/
  //  if( fElevation > 2000.0 )
  //      fAltAdjust = ( (fElevation - 2000.0) / 1000.0 ) * 0.04;
  //  else
  fAltAdjust = 0.0;

  //        Adjustment eliminated - MBG 5/03.  Applies to capacity, not
  //        efficiency (see ASHREA Equipment Handbook, 1983 p. 27.13).
  //        Not applied to capacity since assume equipment has already
  //        been modified to operate in altitude in which they are used.

  /************************
  If it is the cooling season, set heating energy use to 0 and return.
  *************************/
  if (iSeason == COOLING) {
    *fHtgEnerUse_Day = 0.0;
    *fHtgEnerUse_Night = 0.0;
    return;
  }

  /*******************
  Determine whether or not there is setback.
  *******************/
  if (iflgSetBack == TRUE) // there is a setback.
  {
    // Changed Setup Parameter to Lenth of Thermostat Setback
    //   from Length of Day for Night Setback. 3/05.

    fNightSetback = mdi->key.length_of_night_thermostat_setback;
    fNightHours = fNightSetback;
    fNightHours = 5.0f / 7.0f * fNightHours; // Setback during weekdays only
    fDayHours = 24.0f - fNightHours;
  } else // No setback.
    fDayHours = fNightHours = 12.0;

  fHours = 12.0;

  /****************
  Convert outside temperatures to Metric units (deg F to deg C).
  ****************/
  fTOutsideDay = (float)((fTempOut_Day - 32.0) * 5.0 / 9.0);
  fTOutsideNight = (float)((fTempOut_Night - 32.0) * 5.0 / 9.0);

  /************************
  If no heating equipment, set heating energy use to 0 and return.
  *************************/
  if (mdi->htg.equip_type == ET_NONE) {
    *fHtgEnerUse_Day = 0.0;
    *fHtgEnerUse_Night = 0.0;

    return;
  }

  /****************
  Convert inside temperatures to Metric units (deg F to deg C).
  ****************/
  fTInsideDay = (float)((mdi->key.heating_setpoint_day - 32.0) * 5.0 / 9.0);
  fTInsideNight = (float)((mir->fNightSetpoint - 32.0) * 5.0 / 9.0);

  /******************
  Adjust Loads for multiple equipment and
  convert to metric units (Btu/h to W).
  ******************/
  fPercentPrimaryHeat = mdi->htg.percent_heated;
  if (fPercentPrimaryHeat < 100.0) {
    fSensibleHtgLoad_Day = (float)((fHourLoadDay * WH_PER_BTU) * fPercentPrimaryHeat / 100.0);
    fSensibleHtgLoad_Night = (float)((fHourLoadNight * WH_PER_BTU) * fPercentPrimaryHeat / 100.0);
  } else {
    fSensibleHtgLoad_Day = (float)(fHourLoadDay * WH_PER_BTU);
    fSensibleHtgLoad_Night = (float)(fHourLoadNight * WH_PER_BTU);
  }

  /******************
  Fan Power (W)
  ******************/
  if (mdi->htg.fuel_type == WOOD || mdi->htg.fuel_type == COAL)
    fFanpower = 0.0;
  else /* all other equipment types */
    fFanpower = 60.0;

  /***********************
  If the percent of heat supplied by the primary equipment is
  zero, set heating energyuse to 0 and go on to secondary calcs.
  ***********************/
  if (fPercentPrimaryHeat == 0.0) {
    *fHtgEnerUse_Day = 0.0;
    *fHtgEnerUse_Night = 0.0;
  }

  /**********************
  Do energy consumption calculations
  **********************/
  else { // Long else encompassing all primary system consumption calcs

    /***************
    Get the equipment input capacity and
    convert to Metric units (kBtu/h to W).
    Issue #156 avoid garbage value for fInCapacity
    ***************/

    if (mdi->htg.fuel_type != WOOD && mdi->htg.fuel_type != COAL) {
      fInCapacity = (float)(mdi->htg.capacity * W_PER_KBTUH);
    } else {
      fInCapacity = 0;
    }

    // Here is the heating equipment efficiency assignment, MJF 2/28/00
    // This also holds the Heat Pump COP, MJF 6/02
    // Add climate adjustment to HSPF.  MBG 4/08

    if (mdi->htg.eff_units == HE_COP) {
      fEfficiency = mdi->htg.efficiency_cop;
    } else if (mdi->htg.eff_units == HE_HSPF) {
      fEfficiency = mdi->htg.efficiency_hspf;
      //adjust_hspf(mir->fW_Dsgn_T_New, &fEfficiency);
      adjust_hspf(cwd->winter_hp_design_temp, &fEfficiency);
      fEfficiency /= 3.413F;
    } else {
      fEfficiency = (float)(mdi->htg.efficiency_percent / 100.0);
    }

    // Apply furnace tuneup measure if implemented.
    // Modified MBG 7/07 from flat percentage specified by user
    //  to method similar to NEAT's.
    // 2015 research seems to indicate lower potential for tuneup savings https://www.nrel.gov/docs/fy15osti/63702.pdf

    // #334 This needs to be in measure.c and modify the mdi
    // if (mir->flgRetrofits[M_CMS_TUNE_HEATING_SYSTEM]) {    // alter the steady state efficiency expressed as a factor not percent
    //   float deleff, e0, e1, e2, c1, s1, c2;

    //   deleff = 0.0f;
    //   e0 = .65f;
    //   e2 = .76f;

    //   ASSERT(mdi->htg.fuel_type == NATURAL_GAS || 
    //     mdi->htg.fuel_type == PROPANE || 
    //     mdi->htg.fuel_type == KEROSENE || 
    //     mdi->htg.fuel_type == OIL, sprintf(msg, "Incorrect fuel for tuneup"));

    //   if (mdi->htg.fuel_type == NATURAL_GAS || mdi->htg.fuel_type == PROPANE) {
    //     e1 = .70f, c1 = 0.67f, s1 = .04f;
    //   } // Gas or Propane
    //   else if (mdi->htg.fuel_type == KEROSENE || mdi->htg.fuel_type == OIL) {
    //     e1 = .69f, c1 = 1.0f, s1 = .07f;
    //   }

    //   c2 = 0.0f;
    //   if (fEfficiency <= e0) {
    //     c2 = 3.0f;
    //     deleff = s1;
    //   } else if (fEfficiency <= e1) {
    //     c2 = 2.0f;
    //     deleff = s1;
    //   } else if (fEfficiency <= e2) {
    //     c2 = 1.0f;
    //     deleff = c1 * (e2 - fEfficiency);
    //   }

    //   if (mdi->htg.equip_type != ET_FURNACE && mdi->htg.equip_type != ET_HEATPUMP)
    //     c2 = 0.0f;
    //   if (mdi->inf.evaluate_duct_sealing != YES)
    //     deleff += 0.02f * c2;

    //   fEfficiency += deleff;
    // }

    // Eliminated limiting system efficiency after tuneup to 100% since
    // use of new method would not allow this to occur. MBG 7/07

    // Eliminate efficiency adjustment (-=fAltAdjust) for NG, Propane, Oil,
    // and Kerosene fueled equipment. Was in error. See MBG notes, 4/28/03.
    // Indication of addition, 2/28/00, MJF. Eliminated 5/03, MBG

    /********************************************************
    If originally given an AFUE, eliminate conversion of intantaneous to
    seasonal value.  Use the seasonal AFUE given on input (MBG-7/22/03)
    ********************************************************/

    if (mdi->htg.eff_units == HE_AFUE || mdi->htg.eff_units == HE_HSPF) {

      *fHtgEnerUse_Day = fNumDays * fHours * fSensibleHtgLoad_Day / fEfficiency / (1.0f - fDistlossfactor);
      *fHtgEnerUse_Night = fNumDays * fHours * fSensibleHtgLoad_Night / fEfficiency / (1.0f - fDistlossfactor);

      /**********************
      If there is night setback, compute fLoadSB for either
         (1) hours during night-time outdoor conditions but without indoor
           air setback, if fDayHours > 12.
      or (2) hours during day-time  outdoor conditions but with indoor
           air setback, fDayHours < 12.
      Use temperature interpolation with fSensibleHtgLoad_Day/Night.MBG 11/03
      **********************/

      if (fDayHours > 12.0) { // Night setback less than 12 hours

        fLoadSB = fSensibleHtgLoad_Night * (mdi->key.heating_setpoint_day - fTempOut_Night) /
                  (mir->fNightSetpoint - fTempOut_Night);

        *fHtgEnerUse_Night = fNumDays * fNightHours * fSensibleHtgLoad_Night / fEfficiency / (1.0f - fDistlossfactor) +
                             fNumDays * (fDayHours - fHours) * fLoadSB / fEfficiency / (1.0f - fDistlossfactor);
      }

      else if (fDayHours < 12) { // Night setback greater than 12 hours

        fLoadSB = fSensibleHtgLoad_Day * (mir->fNightSetpoint - fTempOut_Day) /
                  (mdi->key.heating_setpoint_day - fTempOut_Day);

        *fHtgEnerUse_Day = fNumDays * fDayHours * fSensibleHtgLoad_Day / fEfficiency / (1.0f - fDistlossfactor) +
                           fNumDays * (fNightHours - fHours) * fLoadSB / fEfficiency / (1.0f - fDistlossfactor);
      }
    }

    else { // Long else for consmptn calcs using steady state efficienies

      /*******************
      Calculate Part Load Ratios
      *******************/
      if (mdi->htg.equip_type == ET_HEATPUMP) {
        /*******************
        fHeatpumpTF = Phi1 in SH notes
        *******************/
        fHeatpumpTFDay =
            (float)(((2.54 * pow(10.0, -4.0) * fTOutsideDay * fTOutsideDay) + (2.53 * pow(10.0, -2.0) * fTOutsideDay) + 0.7404) *
                    (1.0 + (0.0057 * (21.11 - fTInsideDay))));
        fHeatpumpTFNight =
            (float)(((2.54 * pow(10.0, -4.0) * pow(fTOutsideNight, 2.0)) + (2.53 * pow(10.0, -2.0) * fTOutsideNight) + 0.7404) *
                    (1.0 + (0.0057 * (21.11 - fTInsideNight))));

        ASSERT(fInCapacity, sprintf(msg, "Missing Input Capacity"));

        fHeatpumpCapDay = fInCapacity * fHeatpumpTFDay;
        fHeatpumpCapNight = fInCapacity * fHeatpumpTFNight;

        fPLRDay = (float)(fSensibleHtgLoad_Day / ((fHeatpumpCapDay + fFanpower) * (1.0 - fDistlossfactor)));
        if (fPLRDay > 1.0)
          fPLRDay = 1.0;

        fPLRNight = (float)(fSensibleHtgLoad_Night / ((fHeatpumpCapNight + fFanpower) * (1.0 - fDistlossfactor)));
        if (fPLRNight > 1.0)
          fPLRNight = 1.0;

        /***************
        SLF 07/25/94 - If the part load ratio (PLR) is clipped to 1.0,
        then the heating equipment does not meet the load.  Flags are
        set here to print this condition in the savings output file.
        ***************/
        if (mir->flgWhichPass == BASE_CASE) {
          if (fPLRDay >= 1.0 || fPLRNight >= 1.0) {
            mir->flgPreHighHTGLoad = TRUE;
            mir->iPreHighLoadMonths[zMonth] = TRUE;
          }
        } else if (mir->flgWhichPass == CUMULATIVE) {
          if (fPLRDay >= 1.0 || fPLRNight >= 1.0) {
            mir->flgPostHighHTGLoad = TRUE;
            mir->iPostHighLoadMonths[zMonth] = TRUE;
          }
          //              else
          //                  mir->iPostHighLoadMonths[zMonth] = FALSE;
        }

      } else if (mdi->htg.fuel_type == COAL || mdi->htg.fuel_type == WOOD) {

        /************************
        If coal or wood equipment, the output capacity is considered
        to be equivalent to the load.
        *************************/
        fPLRDay = 1.0;
        fPLRNight = 1.0;

      } else {          // all heating equipment types except heatpump, coal, or wood

        ASSERT(fInCapacity, sprintf(msg, "Missing Input Capacity"));

        fOutCapacity = fInCapacity * fEfficiency;   // this is a steady state efficiency

        fPLRDay = (float)(fSensibleHtgLoad_Day / ((fOutCapacity + fFanpower) * (1.0 - fDistlossfactor)));
        if (fPLRDay > 1.0)
          fPLRDay = 1.0;

        fPLRNight = (float)(fSensibleHtgLoad_Night / ((fOutCapacity + fFanpower) * (1.0 - fDistlossfactor)));
        if (fPLRNight > 1.0)
          fPLRNight = 1.0;

        /***************
        SLF 07/25/94 - If the part load ratio (PLR) is clipped to 1.0,
        then the heating equipment does not meet the load.  Flags are
        set here to print this condition in the savings output file.
        ***************/
        if (mir->flgWhichPass == BASE_CASE) {
          if (fPLRDay >= 1.0 || fPLRNight >= 1.0) {
            mir->flgPreHighHTGLoad = TRUE;
            mir->iPreHighLoadMonths[zMonth] = TRUE;
          }
        } else if (mir->flgWhichPass == CUMULATIVE) {
          if (fPLRDay >= 1.0 || fPLRNight >= 1.0) {
            mir->flgPostHighHTGLoad = TRUE;
            mir->iPostHighLoadMonths[zMonth] = TRUE;
          }
          //              else
          //                  mir->iPostHighLoadMonths[zMonth] = FALSE;
        }
      }

      /*******************
      Calculate Phi3 and Phi2
      *******************/
      if (mdi->htg.equip_type == ET_HEATPUMP) {
        fPhi2Day = (float)(((-1.79 * pow(10.0, -5.0) * pow(fTOutsideDay, 3.0)) +
                            (6.60 * pow(10.0, -4.0) * pow(fTOutsideDay, 2.0)) - (0.021 * fTOutsideDay) + 1.144) *
                           (1.0 - (0.0133 * (21.11 - fTInsideDay))));
        fPhi2Night = (float)(((-1.79 * pow(10.0, -5.0) * pow(fTOutsideNight, 3.0)) +
                              (6.60 * pow(10.0, -4.0) * pow(fTOutsideNight, 2.0)) - (0.021 * fTOutsideDay) + 1.144) *
                             (1.0 - (0.0133 * (21.11 - fTInsideNight))));
        fPhi3Day = (float)((0.3699 * pow(fPLRDay, 3.0)) - (0.8225 * pow(fPLRDay, 2.0)) + (1.4528 * pow(fPLRDay, 1.0)));
        fPhi3Night = (float)((0.3699 * pow(fPLRNight, 3.0)) - (0.8225 * pow(fPLRNight, 2.0)) + (1.4528 * pow(fPLRNight, 1.0)));
      } else /* all other heating equipment types */
      {
        fPhi2Day = 1.0;
        fPhi2Night = 1.0;

        if (mdi->htg.fuel_type == NATURAL_GAS) {
          fPhi3Day = (float)((-0.0418 * pow(fPLRDay, 2.0)) + (1.0397 * fPLRDay) + 0.0023);
          fPhi3Night = (float)((-0.0418 * pow(fPLRNight, 2.0)) + (1.0397 * fPLRNight) + 0.0023);
        } else if (mdi->htg.fuel_type == OIL || mdi->htg.fuel_type == KEROSENE) {
          fPhi3Day = (float)((0.025 * pow(fPLRDay, 2.0)) + (0.9495 * fPLRDay) + 0.0259);
          fPhi3Night = (float)((0.025 * pow(fPLRNight, 2.0)) + (0.9495 * fPLRNight) + 0.0259);
        } else /* all other htg equip. types */
        {
          fPhi3Day = fPLRDay;
          fPhi3Night = fPLRNight;
        }
      }

      /**********************
      If there is night setback, compute fPhi3SB for hours during night-time
      outdoor conditions but without indoor air setback or during day-time
      outdoor conditions with indoor air setback.  Use temperature
      interpolation with fPhi3Day/Night. Prior method was not correct.
      MBG 11/03
      **********************/

      if (fDayHours >= 12) { //  Setback less than 12 hours
        fPhi3SB =
            fPhi3Night * (mdi->key.heating_setpoint_day - fTempOut_Night) / (mir->fNightSetpoint - fTempOut_Night);
      } else { // Setback more than 12 hours
        fPhi3SB = fPhi3Day * (mir->fNightSetpoint - fTempOut_Day) / (mdi->key.heating_setpoint_day - fTempOut_Day);
      }

      /***********************
      Calculate overall heating efficiency
      ***********************/
      //      fOHtgEffDay   = fEfficiency * fPLRDay / ( fPhi2Day * fPhi3Day );
      //      fOHtgEffNight = fEfficiency * fPLRNight / ( fPhi2Night * fPhi3Night );

      /***********************
      Calculate Heating Energy Usage during the day for the month
      ***********************/
      // 11/3/03, MBG - modify load calcs for setback hours more or less
      //  than 12 hours, using fPhi3SB. Allows for daytime or nighttime
      //  indoor conditions (setpoints) with opposite outdoor conditions
      //  (average outdoor temperatures).

      if (fSensibleHtgLoad_Day > 0.0) {
        if (mdi->htg.equip_type == ET_HEATPUMP) {
          ASSERT(fEfficiency != 0, sprintf(msg, "Assertion Failure"));
          *fHtgEnerUse_Day = fNumDays * fHours * fPhi3Day * ((fInCapacity * fHeatpumpTFDay * fPhi2Day / fEfficiency) + fFanpower);
        } else if (mdi->htg.fuel_type == COAL || mdi->htg.fuel_type == WOOD) {
          ASSERT(fEfficiency != 0, sprintf(msg, "Assertion Failure"));
          *fHtgEnerUse_Day = fNumDays * fHours * fSensibleHtgLoad_Day / fEfficiency;
        } else /* gas, electric, propane, oil, or kerosene */
        {
          ASSERT(fInCapacity, sprintf(msg, "Missing Input Capacity"));
          if (fDayHours < 12) {
            *fHtgEnerUse_Day =
                fNumDays * (fInCapacity * fPhi2Day + fFanpower) * (fDayHours * fPhi3Day + (fNightHours - fHours) * fPhi3SB);
          } else {
            *fHtgEnerUse_Day = fNumDays * fHours * fPhi3Day * ((fInCapacity * fPhi2Day) + fFanpower);
          }
        }
      } else /* fSensibleHtgLoad_Day <= 0.0 */
        *fHtgEnerUse_Day = 0.0;

      /***********************
      Calculate Heating Energy Usage during the night for the month
      ***********************/
      if (fSensibleHtgLoad_Night > 0.0) {
        if (mdi->htg.equip_type == ET_HEATPUMP) {
          ASSERT(fInCapacity, sprintf(msg, "Missing Input Capacity"));
          ASSERT(fHeatpumpTFNight, sprintf(msg, "Missing fHeatpumpTFNight"));
          *fHtgEnerUse_Night =
              fNumDays * fNightHours * fPhi3Night * ((fInCapacity * fHeatpumpTFNight * fPhi2Night / fEfficiency) + fFanpower);
          if (fDayHours > 12.0) {
            *fHtgEnerUse_Day += fNumDays * (fDayHours - fHours) * fPhi3Night *
                                ((fInCapacity * fHeatpumpTFNight * fPhi2Night / fEfficiency) + fFanpower);
          }
        } else if (mdi->htg.fuel_type == COAL || mdi->htg.fuel_type == WOOD) {
          ASSERT(fEfficiency != 0, sprintf(msg, "Assertion Failure"));
          *fHtgEnerUse_Night = fNumDays * fNightHours * fSensibleHtgLoad_Night / fEfficiency;
          if (fDayHours > 12.0) {
            ASSERT(fEfficiency != 0, sprintf(msg, "Assertion Failure"));
            *fHtgEnerUse_Day += fNumDays * (fDayHours - fHours) * fSensibleHtgLoad_Night / fEfficiency;
          }
        } else /* gas, electric, propane, oil, or kerosene */
        {
          ASSERT(fInCapacity, sprintf(msg, "Missing Input Capacity"));
          *fHtgEnerUse_Night = fNumDays * fHours * fPhi3Night * ((fInCapacity * fPhi2Night) + fFanpower);
          if (fDayHours > 12.0) {
            *fHtgEnerUse_Night =
                fNumDays * (fInCapacity * fPhi2Night + fFanpower) * (fNightHours * fPhi3Night + (fDayHours - fHours) * fPhi3SB);
          }
        }
      } else /* fSensibleHtgLoad_Night <= 0.0 */
        *fHtgEnerUse_Night = 0.0;

      if (cmds.debug_level & D_MHEA_ENERGY_DETAIL && iMonth == JANUARY) {
        sprintf(mir->sMsg, " fPLRDay = %8.3f,  fPLRNight = %8.3f \n"
                         " fPhi2Day = %8.3f,  fPhi2Night = %8.3f \n"
                         " fPhi3Day = %8.3f,  fPhi3Night = %8.3f \n"
                         " fHtgEnerUse_Day(kBtu) = %8.3f,  fHtgEnerUse_Night(kBtu) = %8.3f \n\n",
                fPLRDay, fPLRNight, fPhi2Day, fPhi2Night, fPhi3Day, fPhi3Night, *fHtgEnerUse_Day/1000, *fHtgEnerUse_Night/1000);
        fprintf(stderr, "%s", mir->sMsg);
      }

    } //  End of else for use of steady state efficiencies

  }   // End of else for primary heating system usage

  /***********************************************************/
  /*  Open file for intermediate calculation results output  */
  /***********************************************************/
  if (cmds.debug_level & D_MHEA_ENERGY_DETAIL && iMonth == JANUARY) {
    sprintf(mir->sMsg, "get_heating_consumption iMonth = %d \n", iMonth);
    fprintf(stderr, "%s", mir->sMsg);

    sprintf(mir->sMsg, " fAltAdjust = %8.3f \n"
                     " fDayHours = %8.3f,  fNightHours = %8.3f \n"
                     " fTOutsideDay = %8.3f,  fTOutsideNight = %8.3f \n"
                     " fTInsideDay  = %8.3f,  fTInsideNight  = %8.3f \n"
                     " fSensibleHtgLoad_Day(kBtu) = %8.1f,  fSensibleHtgLoad_Night(kBtu) = %8.1f \n",
            fAltAdjust, fDayHours, fNightHours, fTOutsideDay, fTOutsideNight, fTInsideDay, fTInsideNight, 
            fSensibleHtgLoad_Day/1000,
            fSensibleHtgLoad_Night/1000);
    fprintf(stderr, "%s", mir->sMsg);

    sprintf(mir->sMsg, " fInCapacity = %8.3f \n"
                     " fEfficiency = %8.3f \n"
                     " fFanpower = %8.3f \n"
                     " fDistlossfactor = %8.3f \n",
            fInCapacity, fEfficiency, fFanpower, fDistlossfactor);
    fprintf(stderr, "%s", mir->sMsg);
  }

  /*******************************************
   Begin handling the secondary heating system
  ********************************************/

  if (fPercentPrimaryHeat < 100.0) {

    /**********************
    Do energy consumption calculations if there is heating equipment.
    **********************/
    if (mdi->ht2.equip_type != ET_NONE) {
      /******************
      Adjust Loads for multiple equipment and
      convert to metric units (Btu/h to W).
      ******************/
      fSensibleHtgLoad_Day = (float)((fHourLoadDay * WH_PER_BTU) * (1.0 - (fPercentPrimaryHeat / 100.0)));
      fSensibleHtgLoad_Night = (float)((fHourLoadNight * WH_PER_BTU) * (1.0 - (fPercentPrimaryHeat / 100.0)));

      /***************
      Get the equipment input capacity and
      convert to Metric units (kBtu/h to W).
      ***************/

      if (mdi->ht2.fuel_type != WOOD && mdi->ht2.fuel_type != COAL) {
        fInCapacity = (float)(mdi->ht2.capacity * W_PER_KBTUH);
      } else {
        fInCapacity = 0;
      }

      // Here is the heating equipment efficiency assignment, MJF 2/28/00
      // This also holds the Heat Pump COP, MJF 6/02

      if (mdi->ht2.eff_units == HE_COP) {
        fEfficiency = mdi->ht2.efficiency_cop;
      } else if (mdi->ht2.eff_units == HE_HSPF) {
        fEfficiency = (float)(mdi->ht2.efficiency_hspf / 3.413);
      } else {
        fEfficiency = (float)(mdi->ht2.efficiency_percent / 100.0);
      }

      // Apply furnace tuneup measure here if applicable... Mark T,
      // said to remove the connection between primary and secondary system
      // tunue-up.  MJF 2/19/2020

      // Eliminated limiting system efficiency after tuneup to 100% since
      // use of new method would not allow this to occur. MBG 7/07

      // Adjust the efficiency for the altitude for combustion
      // fuels.  MJF 2/28/00. Eliminated 5/03  MBG

      //          if( mdi->htg.fuel_type == NATURAL_GAS   ||
      //              mdi->htg.fuel_type == PROPANE       ||
      //              mdi->htg.fuel_type == OIL           ||
      //              mdi->htg.fuel_type == KEROSENE )
      //              fEfficiency -= fAltAdjust;

      /********************************************************
      If originally given an AFUE, eliminate conversion of intantaneous to
      seasonal value.  Use the seasonal AFUE given on input (MBG-7/24/03)
      ********************************************************/

      if (mdi->ht2.eff_units == HE_AFUE || mdi->ht2.eff_units == HE_HSPF) {

        /**********************
        If there is night setback, compute fLoadSB for either
           (1) hours during night-time outdoor conditions but without indoor
             air setback, if fDayHours > 12.
        or (2) hours during day-time  outdoor conditions but with indoor
             air setback, fDayHours < 12.
        Use temperature interpolation with fSensibleHtgLoad_Day/Night.MBG 11/03
        **********************/

        if (fDayHours > 12.0) { // Night setback less than 12 hours

          fLoadSB = fSensibleHtgLoad_Night * (mdi->key.heating_setpoint_day - fTempOut_Night) /
                    (mir->fNightSetpoint - fTempOut_Night);

          *fHtgEnerUse_Night += fNumDays / fEfficiency * (fNightHours * fSensibleHtgLoad_Night + (fDayHours - fHours) * fLoadSB);
          *fHtgEnerUse_Day += fNumDays * fHours * fSensibleHtgLoad_Day / fEfficiency;
        }

        else if (fDayHours < 12) { // Night setback greater than 12 hours

          fLoadSB = fSensibleHtgLoad_Day * (mir->fNightSetpoint - fTempOut_Day) /
                    (mdi->key.heating_setpoint_day - fTempOut_Day);

          *fHtgEnerUse_Day += fNumDays / fEfficiency * (fDayHours * fSensibleHtgLoad_Day + (fNightHours - fHours) * fLoadSB);
          *fHtgEnerUse_Night += fNumDays * fHours * fSensibleHtgLoad_Night / fEfficiency;
        }

        else { // No Setback or equal day and night periods

          *fHtgEnerUse_Day += fNumDays * fHours * fSensibleHtgLoad_Day / fEfficiency;
          *fHtgEnerUse_Night += fNumDays * fHours * fSensibleHtgLoad_Night / fEfficiency;
        }
      } else { // Long else for consmptn calcs using steady state efficienies

        /******************
        Fan Power (W)
        ******************/
        if (mdi->ht2.fuel_type == WOOD || mdi->ht2.fuel_type == COAL)
          fFanpower = 0.0;
        else /* all other equipment types */
          fFanpower = 60.0;

        /****************************
        Overall distribution loss factor is 0.0 for secondary system.
        Ducts assumed associated only with primary sytsem.
        ******************************/

        fDistlossfactor = 0.0f;

        /*******************
        Calculate Part Load Ratios
        *******************/
        if (mdi->ht2.equip_type == ET_HEATPUMP) {
          fHeatpumpTFDay = (float)(((2.54 * pow(10.0, -4.0) * fTOutsideDay * fTOutsideDay) +
                                    (2.53 * pow(10.0, -2.0) * fTOutsideDay) + 0.7404) *
                                   (1.0 + (0.0057 * (21.11 - fTInsideDay))));
          fHeatpumpTFNight =
              (float)(((2.54 * pow(10.0, -4.0) * pow(fTOutsideNight, 2.0)) + (2.53 * pow(10.0, -2.0) * fTOutsideNight) + 0.7404) *
                      (1.0 + (0.0057 * (21.11 - fTInsideNight))));

          ASSERT(fInCapacity, sprintf(msg, "Missing Input Capacity"));

          fHeatpumpCapDay = fInCapacity * fHeatpumpTFDay;
          fHeatpumpCapNight = fInCapacity * fHeatpumpTFNight;

          fPLRDay = (float)(fSensibleHtgLoad_Day / ((fHeatpumpCapDay + fFanpower) * (1.0 - fDistlossfactor)));
          if (fPLRDay > 1.0)
            fPLRDay = 1.0;

          fPLRNight = (float)(fSensibleHtgLoad_Night / ((fHeatpumpCapNight + fFanpower) * (1.0 - fDistlossfactor)));
          if (fPLRNight > 1.0)
            fPLRNight = 1.0;
        } else if (mdi->ht2.fuel_type == COAL || mdi->ht2.fuel_type == WOOD) {
          /************************
          If coal or wood equipment, the output capacity is considered
          to be equivalent to the load.
          *************************/
          fPLRDay = 1.0;
          fPLRNight = 1.0;
        } else /* all heating equipment types except heatpump */
        {
          ASSERT(fInCapacity, sprintf(msg, "Missing Input Capacity"));

          fOutCapacity = fInCapacity * fEfficiency;

          fPLRDay = (float)(fSensibleHtgLoad_Day / ((fOutCapacity + fFanpower) * (1.0 - fDistlossfactor)));
          if (fPLRDay > 1.0)
            fPLRDay = 1.0;

          fPLRNight = (float)(fSensibleHtgLoad_Night / ((fOutCapacity + fFanpower) * (1.0 - fDistlossfactor)));
          if (fPLRNight > 1.0)
            fPLRNight = 1.0;
        }

        /*******************
        Calculate Phi3 and Phi2
        *******************/
        if (mdi->ht2.equip_type == ET_HEATPUMP) {
          fPhi2Day = (float)(((-1.79 * pow(10.0, -5.0) * pow(fTOutsideDay, 3.0)) +
                              (6.60 * pow(10.0, -4.0) * pow(fTOutsideDay, 2.0)) - (0.021 * fTOutsideDay) + 1.144) *
                             (1.0 - (0.0133 * (21.11 - fTInsideDay))));
          fPhi2Night = (float)(((-1.79 * pow(10.0, -5.0) * pow(fTOutsideNight, 3.0)) +
                                (6.60 * pow(10.0, -4.0) * pow(fTOutsideNight, 2.0)) - (0.021 * fTOutsideDay) + 1.144) *
                               (1.0 - (0.0133 * (21.11 - fTInsideNight))));
          fPhi3Day = (float)((0.3699 * pow(fPLRDay, 3.0)) - (0.8225 * pow(fPLRDay, 2.0)) + (1.4528 * pow(fPLRDay, 1.0)));
          fPhi3Night = (float)((0.3699 * pow(fPLRNight, 3.0)) - (0.8225 * pow(fPLRNight, 2.0)) + (1.4528 * pow(fPLRNight, 1.0)));
        } else /* all other heating equipment types */
        {
          fPhi2Day = 1.0;
          fPhi2Night = 1.0;

          if (mdi->ht2.fuel_type == NATURAL_GAS) {
            fPhi3Day = (float)((-0.0418 * pow(fPLRDay, 2.0)) + (1.0397 * fPLRDay) + 0.0023);
            fPhi3Night = (float)((-0.0418 * pow(fPLRNight, 2.0)) + (1.0397 * fPLRNight) + 0.0023);
          } else if (mdi->ht2.fuel_type == OIL || mdi->ht2.fuel_type == KEROSENE) {
            fPhi3Day = (float)((0.025 * pow(fPLRDay, 2.0)) + (0.9495 * fPLRDay) + 0.0259);
            fPhi3Night = (float)((0.025 * pow(fPLRNight, 2.0)) + (0.9495 * fPLRNight) + 0.0259);
          } else /* all other htg equip. types */
          {
            fPhi3Day = fPLRDay;
            fPhi3Night = fPLRNight;
          }
        }

        /**********************
        If there is night setback, compute fPhi3SB for hours during night-time
        outdoor conditions but without indoor air setback or during day-time
        outdoor conditions with indoor air setback.  Use temperature
        interpolation with fPhi3Day/Night. Prior method was not correct.
        MBG 11/03
        **********************/

        if (fDayHours >= 12) { //  Setback less than 12 hours
          fPhi3SB =
              fPhi3Night * (mdi->key.heating_setpoint_day - fTempOut_Night) / (mir->fNightSetpoint - fTempOut_Night);
        } else  { // Setback more than 12 hours
          fPhi3SB = fPhi3Day * (mir->fNightSetpoint - fTempOut_Day) / (mdi->key.heating_setpoint_day - fTempOut_Day);
        }

        /***********************
        Calculate Heating Energy Usage during the day for the month
        ***********************/
        // 11/3/03, MBG - modify load calcs for setback hours more or less
        //  than 12 hours, using fPhi3SB. Allows for daytime or nighttime
        //  indoor conditions (setpoints) with opposite outdoor conditions
        //  (average outdoor temperatures).

        if (fSensibleHtgLoad_Day > 0.0) {
          if (mdi->ht2.equip_type == ET_HEATPUMP) {
            ASSERT(fInCapacity, sprintf(msg, "Missing Input Capacity"));
            ASSERT(fEfficiency != 0, sprintf(msg, "Assertion Failure"));
            *fHtgEnerUse_Day +=
                fNumDays * fHours * fPhi3Day * ((fInCapacity * fHeatpumpTFDay * fPhi2Day / fEfficiency) + fFanpower);
          } else if (mdi->ht2.fuel_type == COAL || mdi->ht2.fuel_type == WOOD) {
            ASSERT(fEfficiency != 0, sprintf(msg, "Assertion Failure"));
            *fHtgEnerUse_Day += fNumDays * fHours * fSensibleHtgLoad_Night / fEfficiency;
          } else /* gas, electric, propane, oil, or kerosene */
          {
            ASSERT(fInCapacity, sprintf(msg, "Missing Input Capacity"));
            if (fDayHours < 12) {
              *fHtgEnerUse_Day +=
                  fNumDays * (fInCapacity * fPhi2Day + fFanpower) * (fDayHours * fPhi3Day + (fNightHours - fHours) * fPhi3SB);
            } else {
              *fHtgEnerUse_Day += fNumDays * fHours * fPhi3Day * ((fInCapacity * fPhi2Day) + fFanpower);
            }
          }
        } else /* fSensibleHtgLoad_Day <= 0.0 */
          *fHtgEnerUse_Day += 0.0;

        /***********************
        Calculate Heating Energy Usage during the night for the month
        ***********************/
        if (fSensibleHtgLoad_Night > 0.0) {
          if (mdi->ht2.equip_type == ET_HEATPUMP) {
            ASSERT(fInCapacity, sprintf(msg, "Missing Input Capacity"));
            ASSERT(fHeatpumpTFNight, sprintf(msg, "Missing fHeatpumpTFNight"));
            *fHtgEnerUse_Night +=
                fNumDays * fNightHours * fPhi3Night * ((fInCapacity * fHeatpumpTFNight * fPhi2Night / fEfficiency) + fFanpower);
            if (fDayHours > 12.0) {
              *fHtgEnerUse_Day += fNumDays * (fDayHours - fHours) * fPhi3Night *
                                  ((fInCapacity * fHeatpumpTFNight * fPhi2Night / fEfficiency) + fFanpower);
            }
          } else if (mdi->ht2.fuel_type == COAL || mdi->ht2.fuel_type == WOOD) {
            ASSERT(fEfficiency != 0, sprintf(msg, "Assertion Failure"));
            *fHtgEnerUse_Night += fNumDays * fNightHours * fSensibleHtgLoad_Night / fEfficiency;
            if (fDayHours > 12.0) {
              ASSERT(fEfficiency != 0, sprintf(msg, "Assertion Failure"));
              *fHtgEnerUse_Day += fNumDays * (fDayHours - fHours) * fSensibleHtgLoad_Night / fEfficiency;
            }
          } else /* gas, electric, propane, oil, or kerosene */
          {
            ASSERT(fInCapacity, sprintf(msg, "Missing Input Capacity"));
            if (fDayHours > 12.0) {
              *fHtgEnerUse_Night +=
                  fNumDays * (fInCapacity * fPhi2Night + fFanpower) * (fNightHours * fPhi3Night + (fDayHours - fHours) * fPhi3SB);
            } else
              *fHtgEnerUse_Night += fNumDays * fHours * fPhi3Night * ((fInCapacity * fPhi2Night) + fFanpower);
          }
        } else /* fSensibleHtgLoad_Night <= 0.0 */
          *fHtgEnerUse_Night += 0.0;

        if (cmds.debug_level & D_MHEA_ENERGY_DETAIL && iMonth == JANUARY) {
          sprintf(mir->sMsg, " fPLRDay = %8.3f,  fPLRNight = %8.3f \n"
                           " fPhi2Day = %8.3f,  fPhi2Night = %8.3f \n"
                           " fPhi3Day = %8.3f,  fPhi3Night = %8.3f \n"
                           " fHtgEnerUse_Day(kBtu) = %8.3f,  fHtgEnerUse_Night(kBtu) = %8.3f \n\n",
                fPLRDay, fPLRNight, fPhi2Day, fPhi2Night, fPhi3Day, fPhi3Night, *fHtgEnerUse_Day/1000, *fHtgEnerUse_Night/1000);
          fprintf(stderr, "%s", mir->sMsg);
        }

      } // End long else for use of steady state efficiencies on secondary heating system

    }   // End long else for mdi->ht2.equip_type != ET_NONE

    if (cmds.debug_level & D_MHEA_ENERGY_DETAIL && iMonth == JANUARY) {
      sprintf(mir->sMsg, " fPercentPrimaryHeat = %8.3f \n"
                       " fDayHours = %8.3f,  fNightHours = %8.3f \n"
                       " fTOutsideDay = %8.3f,  fTOutsideNight = %8.3f \n"
                       " fTInsideDay  = %8.3f,  fTInsideNight  = %8.3f \n"
                       " fSensibleHtgLoad_Day(kBtu) = %8.1f,  fSensibleHtgLoad_Night(kBtu) = %8.1f \n",
              fPercentPrimaryHeat, fDayHours, fNightHours, fTOutsideDay, fTOutsideNight, fTInsideDay, fTInsideNight,
              fSensibleHtgLoad_Day/1000, 
              fSensibleHtgLoad_Night/1000);
      fprintf(stderr, "%s", mir->sMsg);

      sprintf(mir->sMsg, " fInCapacity = %8.3f \n"
                       " fEfficiency = %8.3f \n"
                       " fFanpower = %8.3f \n"
                       " fDistlossfactor = %8.3f \n",
              fInCapacity, fEfficiency, fFanpower, fDistlossfactor);
      fprintf(stderr, "%s", mir->sMsg);
    }
  } // End long else for fPercentPrimaryHeat < 100.0

  /***********************
  Convert back to english units (W-h -> Btu/month)
  ***********************/
  *fHtgEnerUse_Day *= 3.413F;
  *fHtgEnerUse_Night *= 3.413F;

  return;
}

/*******************  FUNCTION NAME:  get_cooling_consumption  ********************/
/**       DATE: 3/12/93     07/29/93                                                **/
/**          BY:    CH/JG - ERG International, Inc.                             **/
/** DESCRIPTION:    Cooling load algorithms for WAM software                    **/
/**                                                                                         **/
/** Passes back the total monthly energy use (Btu) for the              **/
/** day and night time periods.                                                 **/
/**                                                                                         **/
/** Cool_energy holds the average hourly cooling load in kWh/day        **/
/** and returns it to the calling program.                                      **/
/**                                                                                         **/
/** An averaged hourly cooling load will be passed in.                      **/
/**                                                                                         **/
/** SetEnergy - Function which calculates energy consumption            **/
/**                 for the Day & Night setpoints and returns daily         **/
/**                 cooling energy requirements.                                    **/
/**                                                                                         **/
/** REVISIONS:  NW 3/15/93: added season flag & test for negative clg   **/
/**                 Added code for multiple cooling equipment.              **/
/**                 Added code for calculating distribution loss.           **/
/**                 SLF 11/10/94 Updated Parameters data sturcture index.   **/
/**                 11/16/94 SLF - Reordered mir->flgRetrofits.                   **/
/**                                                                                         **/
/*************************************************************************/

// MANEFEST CONSTANTS
// Unit Conversions to consistent units of kJ, kW, m, s, deg C, kg, etc.
#define T_AIR 26.6 // ARI reference inside dry bulb
                   //  temperature in degrees C.
// #define  P_FAN       0.06            // Power of circulating fan in kW.
#define CP_AIR 1.0073  // Specific heat of moist air in kJ/kg-C.
#define VOL_AIR 1.1922 // Volumetric specific heat of air in
                       //  kJ/cubic m-degree C.
#define C_SR 0.70      // Multiplier to determine sensible cooling
                       //  capacity at ARI reference conditions.
#define CP_EVAP 2260.8 // Latent heat of evaporation for water in kJ/kg.
// #define  AIR_DENSITY 1.1837   // Density of air at STP in kg/cubic meter.
#define AIR_DENSITY 1.201385 // Density of air at STP in kg/cubic meter.
#define MOIST_GENER 0.0      // Internal moisture generation rate kg/sec.
#define CFMTOCMS 4.7195e-4   // CFM to meters^3/sec.

// WEATHER DATA PARAMETERS PASSED FROM WEATHER FILE OR INPUT.
// fElevation           Elevation in feet.
// fTwbOutside          Outside wet bulb temp in degrees F.
// fTdbOutsideDay       Outside daytime dry bulb temp in degrees F.
// fTdbOutsideNight  Outside nighttime dry bulb temp in degrees F.

// VARIABLES ASSUMED TO BE PREVIOUSLY CALCULATED (E.G., DURING LOADS)
// iSeason           HEATING or COOLING season
// flgSetBack           Set to 1 if there's setback, 0 if there's not.
// fNumDays             Number of Days per Month.
// fVolumCathCeiling    Volume of Catherdral Ceiling added to total home volume.
// fInfMassFlow     Infiltration mass flow rate in CFM.
// fClgHourLoadDay  Daytime cooling load in Btuh.
// fClgHourLoadNight    Nighttime cooling load in Btuh.

int get_cooling_consumption(int iSeason, int flgSetBack, int zMonth, float fNumDays, float fElevation, float fVolumeCathCeiling,
                   float fVolumeAddition, float fInfMassFlow, float fTwbOutside, float fTdbOutsideDay, float fTdbOutsideNight,
                   float fClgHourLoadDay, float fClgHourLoadNight, float *fDuctEffClg, float *fClgEnergyDay,
                   float *fClgEnergyNight) {

  float fHomeWidth = 0.0;
  int i = 0;
  int iMonth = zMonth + 1;        // base 1 month from zero based month

  float fClgLoadDay = 0.0;
  float fClgLoadNight = 0.0;
  float fMetLoadDay = 0.0; 
  float fMetLoadNight = 0.0; 
  float fCoolLoad = 0.0; 
  float fClgLoad = 0.0;           // Day or night clg load
  float fDayHours = 0.0;          // Day hrs. value dependent on flgSetBack
  float fNightHours = 0.0;        // Night hrs. value dependent on flgSetBack
  int flgDay = 0;                 // Flags daytime or nighttime (1 = day)
  float fRatedCoolCapacity = 0.0; // Rated total clg capacity (Btu/h)
  float fRatedCOP = 0.0;          // Rated COP of clg equipment
  float fDayCoolSet = 0.0;        // Day & night time dry-bulb clg setpoints
  float fNightCoolSet = 0.0;
  float fDistlossfactor = 0.0;    // Loss factor due to ducts & insulation
  float fClgDuctLocation = 0.0;   // Factor based on duct location
  float fClgDuctInsulation = 0.0; // Factor based on duct insulation
  float fSetpoint = 0.0;          // Clg day/night setpoint
  float fTdbOutside = 0.0;        // Outdoor dry bulb air temp for day/night

  // VARIABLES USED IN ENERGY CONSUMPTION CALCULATIONS

  float fP_Fan = 0.0;           // Fan Power in kW
  float fPhi1 = 0.0;
  float fPhi2 = 0.0;
  float fPhi3 = 0.0;
  float fTwi_star = 0.0;        // Apparatus wet bulb temp of circulating room air above evap saturation
  float fTwi = 0.0;             // Inside wet bulb temperature in deg C
  float fT_dc = 0.0;            // Mixed air temp entering the coil in deg C
  float fAirRate = 0.0;         // Air flow rate in CMS (m^3/s)
  float fAirFlow = 0.0;         // Air flow rate through the evaporator in kg per sec
  float fNewInfMassFlow = 0.0;  // Infiltration mass flow rate multiplied by conversion factors
  float fBy_pass = 0.40;        // By-pass factor at rated conditions
  float fAlpha = 0.0;           // Intermediate calculation for Twi_star
  float fBeta = 0.0;            // Intermediate calculation for Twi_star
  float fGamma = 0.0;           // Intermediate calculation for Twi_star
  float fTotal_cap = 0.0;       // Total adjusted cooling capacity in kW
  float fSens_cap = 0.0;        // Adjusted sensible cooling capacity in kW
  float fW_in = 0.0;            // Inside wet bulb temperature in deg C
  float fPlr = 0.0;             // Part-load ratio
  float fW_out = 0.0;           // Outside humidity ratio
  float fP_ws = 0.0;            // Pressure of saturated pure water in psia
  float fPressure = 0.0;        // Total pressure of moist air in psia
  float fStep_one = 0.0;        // Intermediate calculation for fP_ws
  float fStep_two = 0.0;        // Intermediate calculation for fP_ws
  float fHours = 0.0;           // scratch variable for setback adjustment
  float fTwbOutside_C = 0.0;    // Outside wetbulb temperature in deg C.

  // Compute outside wetbulb temperature in deg C. Corrected MBG 8/03.

  fTwbOutside_C = (float)((fTwbOutside - 32.0) * (5.0 / 9.0));

  /*******************
  Determine whether or not there is setback.
  *******************/
  if (flgSetBack == TRUE) // there is a setback.
  {
    fNightHours = mdi->key.length_of_night_thermostat_setback;
    fDayHours = 24.0f - fNightHours;
  } else // No setback.
    fDayHours = fNightHours = 12.0;

  /** 11/10/97, NLW - adjustment of setback calculations **/

  fHours = 12.0;

  fHomeWidth = mdi->gnl.width;

  /******************
  Get fan power (convert W to kW).
  ******************/
  fP_Fan = (float)(mdi->key.cooling_system_fan_power / 1000.0);

  /*************************
  Get the Primary Cooling Equipment data.
  If the CLG file doesn't exist, set
  cooling energy use to 0 and return.
  *************************/
  if (mir->flgNoCLG == TRUE) {
    *fClgEnergyDay = 0.0;
    *fClgEnergyNight = 0.0;
    return (0);
  }

  /************************
  If no cooling equipment or heating season,
  set cooling energy use to 0 and return.
  *************************/
  if ((mdi->clg.equip_type == CE_NONE) || iSeason == HEATING) {
    *fClgEnergyDay = 0.0;
    *fClgEnergyNight = 0.0;
    return (0);
  }

  /**************************
  Get day and night cooling setpoints and convert to degrees Celcius.
  **************************/
  fDayCoolSet = (float)((mdi->key.cooling_setpoint_day - 32.0) * (5.0 / 9.0));
  fNightCoolSet = (float)((mdi->key.cooling_setpoint_night - 32.0) * (5.0 / 9.0));

  /*********************
  Reduce cooling load to the percent of the home cooled by system and convert
  to metric (kWh) units and give new variable name.
  *********************/
  fClgLoadDay = (float)((fClgHourLoadDay / (float)BTU_PER_KWH) * (mdi->clg.percent_area_room_ac / 100.0));
  fClgLoadNight = (float)((fClgHourLoadNight / (float)BTU_PER_KWH) * (mdi->clg.percent_area_room_ac / 100.0));

  /******************
  Compute the air flow rate (meters^3/sec.) and hours of operation for room
  air-conditioners. Remember that capacities are in kBTU/h
  *******************/
  if (mdi->clg.equip_type == CE_ROOMAC) {
    fAirRate = 0; // to avoid a compiler warning about uninitialized variables
    if (mdi->clg.capacity <= 16.0)
      fAirRate = (float)(400.0 * CFMTOCMS);
    else if ((mdi->clg.capacity > 16.0) && (mdi->clg.capacity <= 26.0))
      fAirRate = (float)(700.0 * CFMTOCMS);
    else if (mdi->clg.capacity > 26.0)
      fAirRate = (float)(1000.0 * CFMTOCMS);

    /** Per conversation w/ Sheila Hayter, NREL, 8/28/97, disable this redefinition
        of operation hours for room AC units.

            switch( zMonth )  //per tech notes page 72.1 - distr. of 750 op hours
                              //  based on national avg.
                {
                case  0: fDayHours =   0.0;          fNightHours = 0.0; break;
                case  1: fDayHours =   0.0;          fNightHours = 0.0; break;
                case  2: fDayHours =   0.0;          fNightHours = 0.0; break;
                case  3: fDayHours =  75.0/fNumDays; fNightHours = 0.0; break;
                case  4: fDayHours =  75.0/fNumDays; fNightHours = 0.0; break;
                case  5: fDayHours = 100.0/fNumDays; fNightHours = 0.0; break;
                case  6: fDayHours = 200.0/fNumDays; fNightHours = 0.0; break;
                case  7: fDayHours = 200.0/fNumDays; fNightHours = 0.0; break;
                case  8: fDayHours = 100.0/fNumDays; fNightHours = 0.0; break;
                case  9: fDayHours =   0.0;          fNightHours = 0.0; break;
                case 10: fDayHours =   0.0;          fNightHours = 0.0; break;
                case 11: fDayHours =   0.0;          fNightHours = 0.0; break;
                }
    **/
  }

  // 8/8/97,NLW Modify air flow rates and fan power for evap cooling equip.
  //   Based on units listed in 1997 RS MEANS Mech Cost Data (p271)
  //   two unit sizes will be used:
  //         1/3HP, 115V, 1785CFM and
  //         1/2HP, 115V, 3235CFM

  else if (mdi->clg.equip_type == CE_EVAPORATIVE)
    //      /* Air Flow Rate in m^3/s (volume in ft^3 * conversion factor). */
    //      fAirRate = fVolume * 0.2943;
    if (fHomeWidth < 20.0) {
      fAirRate = (float)(1785.0 * CFMTOCMS);
      fP_Fan = 0.250;
    } else {
      fP_Fan = 0.375;
      fAirRate = (float)(3235.0 * CFMTOCMS);
    }
  else /* all other cooling equipment types */
  {
    if (fHomeWidth < 20.0)
      fAirRate = (float)(2500.0 * CFMTOCMS);
    else
      fAirRate = (float)(5000.0 * CFMTOCMS);
  }

  /***********************
  Set fAirFlow (kg/s) = fAirRate (m^3/s) * adjusted air density.
  ***********************/
  fAirFlow = (float)(fAirRate * AIR_DENSITY * exp(-0.0001219755 * fElevation * 0.3048));

  fNewInfMassFlow = (float)(fInfMassFlow * CFMTOCMS * AIR_DENSITY * exp(-0.0001219755 * fElevation * 0.3048));

  /**********************
  Convert Capacity from Btuh to kW to keep in consistent units.
  Accommodate efficiencies entered as COP, EER, or SEER (MBG 1/30/03)
  Add climate adjustment to SEER.  MBG 4/08
  **********************/
  fRatedCoolCapacity = mdi->clg.capacity * 1000.0f / (float)BTU_PER_KWH;
  fRatedCOP = get_cooling_cop(mdi->clg.eff_units, mdi->clg.efficiency_cop, mdi->clg.efficiency_seer, mdi->clg.efficiency_eer);

  // changed to account for specific COP coming from the
  // clr replacement strucure 7/09 for 8.6

  // #334 set in measure.c now
  //if (mir->flgRetrofits[M_CMS_TUNE_COOLING_SYSTEM])
  //  fRatedCOP = 3.0;

  /****************************
  Determine overall distribution loss factor
  ******************************/
  if (mdi->clg.equip_type == CE_EVAPORATIVE || mdi->clg.equip_type == CE_ROOMAC)
    fDistlossfactor = 0.0;
  else /* centralAC or heatPump */
  {
    if (mdi->clg.duct_location == DL_FLOOR)
      fClgDuctLocation = 0.50;
    if (mdi->clg.duct_location == DL_CEILING)
      fClgDuctLocation = 0.75;
    if (mdi->clg.duct_location == DL_NONE)
      fClgDuctLocation = 0.0;

    if (mdi->clg.duct_insl == DI_ABOVE || mdi->clg.duct_insl == DI_BELOW || mdi->clg.duct_insl == DI_NONE)
      fClgDuctInsulation = 50.0;
    else /* mdi->clg.duct_insl == DI_AROUND */
    {
      if (mdi->clg.duct_location == DL_FLOOR) {
        if ((mdi->flr.belly_mineral_insl >= 2.0) ||
            ((mdi->flr.belly_mineral_insl + mdi->flr.belly_loose_insl) >= mdi->flr.belly_depth))
          fClgDuctInsulation = 8.75; /* for 2" insulation or more */
        else
          fClgDuctInsulation = 25.0; /* for 1" insulation */
      }
      if (mdi->clg.duct_location == DL_CEILING) {
        if ((mdi->rof.mineral_insl >= 2.0) || ((mdi->rof.mineral_insl + mdi->rof.loose_insl) >= 8.0))
          fClgDuctInsulation = 8.75; /* for 2" insulation or more */
        else
          fClgDuctInsulation = 25.0; /* for 1" insulation */
      }
    }

    fDistlossfactor = (float)(fClgDuctLocation * fClgDuctInsulation / 100.0);

    // 5/8/95, NW - added an adjustment factor of 0.40 to reduce maximum distribution
    //          loss factor.  Per NREL TP-253-4490 (page 12) the worst-case effective
    //          furnace efficiency should be approx. 56%.

    // NW,8/31/95: Update adjustment factor to 0.55 after correction of billing
    //      adjustment problem which biased previous measure savings results

    fDistlossfactor *= 0.30F;
    /* If user has requested ducts to be evaluated from his input data, replace
     *   constant duct loss factors with those computed from ASHREA 152P
     *    MBG 11/7/01 - applied to cooling 12/18/02  */

    if (mdi->inf.evaluate_duct_sealing == YES) { // Duct measurements available
      if (mir->flgRetrofits[M_CMS_SEAL_DUCTS])     // Duct sealing measure active
        fDistlossfactor = 1.0f - *(fDuctEffClg + 1);
      else // Duct sealing measure not active
        fDistlossfactor = 1.0f - *(fDuctEffClg + 0);
    } else { // Use standard duct loss adjustments
      if (mir->flgRetrofits[M_CMS_SEAL_DUCTS])
        fDistlossfactor *= (float)(1.0 - (mdi->key.duct_sealing_distribution_loss_reduction / 100.0));
    }
  }

  /********************************************************
   If originally given an SEER, eliminate conversion of intantaneous to
   seasonal value.  Use the seasonal SEER given on input (MBG-1/30/03)
  ********************************************************/

  if (cmds.debug_level & D_MHEA_ENERGY_DETAIL && iMonth == JULY) { // July  
    fprintf(stderr, "----COOLING ENERGY CONSUMPTION PRIMARY----\n");
  }

  // If given SEER, skip seasonal conversion

  if (mdi->clg.eff_units == CE_SEER && mdi->clg.equip_type != CE_EVAPORATIVE) {

    ASSERT(fRatedCOP != 0, sprintf(msg, "Assertion Failure"));
    ASSERT((1.0f - fDistlossfactor) != 0, sprintf(msg, "Assertion Failure"));
    *fClgEnergyDay = fClgLoadDay * fHours * fNumDays / fRatedCOP / (1.0f - fDistlossfactor) * (float)BTU_PER_KWH;
    *fClgEnergyNight = fClgLoadNight * fHours * fNumDays / fRatedCOP / (1.0f - fDistlossfactor) * (float)BTU_PER_KWH;

    if (cmds.debug_level & D_MHEA_ENERGY_DETAIL && iMonth == JULY) { // July
      fprintf(stderr, " SEER fClgEnergyDay(kBtu) = %8.1f\n", *fClgEnergyDay/1000);
      fprintf(stderr, " SEER fClgEnergyNight(kBtu) = %8.1f\n", *fClgEnergyNight/1000);
    }

  } else { // Long else for efficiencis in COP or EER - line ~ 1425

    /*************************
    Loop through calculations twice, once for the day cooling energy
    calculations and once for the night cooling energy calculations.
    **************************/
    flgDay = 1;

    for (i = 0; i < 2; i++) {   // day, then night
      /********************
      Set variables for day or night period calculation and
      convert the hourly sensible cooling load to metric.
      ********************/
      if (flgDay) // It's the daytime.
      {
        fClgLoad = fClgLoadDay;       // in kW
        fSetpoint = fDayCoolSet;      // in deg C
        fTdbOutside = fTdbOutsideDay; //  in deg F
      } else                          // It's the nighttime.
      {
        fClgLoad = fClgLoadNight;       // in kW
        fSetpoint = fNightCoolSet;      // in deg C
        fTdbOutside = fTdbOutsideNight; // in deg F
      }

      /*************************
      Test for negative cooling load and cooling season
      (checked once each pass for day or night).
      **************************/
      if ((iSeason == HEATING) || (fClgLoad < 0.0))
        fClgLoad = 0.0;

      /*********************
      Calculate the outdoor humidity ratio (fW_out).
      (fTwbOutside is still in degrees F for the fW_out calcs.)
      **********************/
      fStep_one = (float)((0.1289706 / 10000.0) * pow((459.67 + fTwbOutside), 2));
      fStep_two = (float)((0.2478068 / 100000000.0) * pow((459.67 + fTwbOutside), 3));

      fP_ws = (float)(exp((-10440.4 / (float)(459.67 + fTwbOutside)) - 11.2946669 - (0.02700133 * (459.67 + fTwbOutside)) +
                          fStep_one - fStep_two + (6.5459673 * log(459.67 + fTwbOutside))));

      fPressure = (float)(((-0.000938 * fElevation) + 29.737) / 2.036);

      ASSERT((fPressure - fP_ws) != 0, sprintf(msg, "Assertion Failure"));
      fW_out = (float)((((1093 - 0.556 * fTwbOutside) * (0.62198 * fP_ws / (float)(fPressure - fP_ws))) -
                        (0.24 * (fTdbOutside - fTwbOutside))) /
                       (float)(1093.0 + 0.444 * fTdbOutside - fTwbOutside));

      /************************
      Convert the outdoor temps (wet and dry) to Metric units.
      **************************/
      fTdbOutside = (float)((fTdbOutside - 32.0) * (5.0 / 9.0));
      //      fTwbOutside = (fTwbOutside - 32.0) * (5.0 / 9.0);  MBG 8/03.

      /***********************
      Procedure for mechanical compression cycle cooling equipment.
      ***********************/
      if (mdi->clg.equip_type == CE_CENTRALAC || mdi->clg.equip_type == CE_ROOMAC || mdi->clg.equip_type == CE_HEATPUMP) {
        /**********************
        Note that fT_dc definition indicates a recirculating system
        with no mixture of outside air.
        ************************/
        ASSERT((CP_AIR * fAirFlow) != 0, sprintf(msg, "Assertion Failure"));
        fT_dc = fSetpoint + (fP_Fan / (float)(CP_AIR * fAirFlow));

        /********************
        Calculate fTwi_star, the apparatus wet bulb temp:
        the wet bulb temp of circulating rooom air above
        which the evaporator coil would become wet.
        ********************/
        fAlpha = (float)(CP_AIR * fAirFlow * (1.0 - fBy_pass) * (fT_dc - T_AIR));
        fBeta = (float)(fRatedCoolCapacity * (((0.00927 * fTdbOutside) - 0.8593) - (((0.00399 * fTdbOutside) - 2.7351) * C_SR)));
        fGamma = (float)(fRatedCoolCapacity * (0.0239 + (0.08204 * C_SR)));
        ASSERT(fGamma != 0, sprintf(msg, "Assertion Failure"));
        fTwi_star = (fAlpha + fBeta) / fGamma;

        ASSERT(fNewInfMassFlow != 0, sprintf(msg, "Assertion Failure"));
        fW_in = (float)(fW_out + (MOIST_GENER / fNewInfMassFlow));
        fTwi = (float)(((1000000.0 * fW_in) +
                        ((((0.0015 * fElevation * 0.3048) + 413.6) * fSetpoint) + ((0.38 * fElevation * 0.3048) + 2124.0))) /
                       (float)(1255.1 + 0.1295 * fElevation * 0.3048));

        if (fTwi > fTwi_star) /* operation with wet coil */
        {
          fTotal_cap = (float)(fRatedCoolCapacity * (0.8593 + 0.02394 * fTwi - 0.00927 * fTdbOutside) - fP_Fan);

          fSens_cap = (float)((CP_AIR * fAirFlow) * (1.0 - fBy_pass) * (fT_dc - T_AIR));

          // 8/13/97, NLW: Determined that this test is not required as this
          //               term adjusts operational capacity from rated conditions.
          //               (similar changed made below for secondary cooling.)
          //              if( fSens_cap <= 0.0 )
          //                  fSens_cap = 0.0;

          fSens_cap += (float)((C_SR * fRatedCoolCapacity) * (2.7351 - (0.08204 * fTwi) - (0.00399 * fTdbOutside)) - fP_Fan);
        } else /* operation with dry coil */
        {
          fSens_cap = (float)(CP_AIR * fAirFlow * (1.0 - fBy_pass) * (fT_dc - T_AIR));

          //              if( fSens_cap <= 0.0 )
          //                  fSens_cap = 0.0;

          fSens_cap += (float)((C_SR * fRatedCoolCapacity) * (2.7351 - (0.08204 * fTwi_star) - (0.00399 * fTdbOutside)) - fP_Fan);

          fTotal_cap = fSens_cap;
        } // End if(Twi > Twi_star)

        /******************************
        Part Load ratio should = 1 if load is > derated capacity
        *******************************/
        if (fClgLoad > (fSens_cap * (1.0 - fDistlossfactor)))
          fPlr = 1.0;
        else {
          ASSERT((fSens_cap * (1.0 - fDistlossfactor)) != 0, sprintf(msg, "Assertion Failure"));
          fPlr = (float)(fClgLoad / (fSens_cap * (1.0 - fDistlossfactor)));
        }

        /***********************
        Hourly cooling load energy consumption
        ***********************/
        if (fPlr > 0.0) {
          fPhi1 = (float)(0.8593 + (0.02394 * fTwi) - (0.00927 * fTdbOutside));
          fPhi2 = (float)(0.5967 - (0.01378 * fTwi) + (0.01926 * fTdbOutside));
          fPhi3 = (float)((0.2952 * pow(fPlr, 3)) - (0.6591 * pow(fPlr, 2)) + (1.3639 * fPlr));
          //              fCOP = (fRatedCOP * fPlr) / (float)(fPhi2 * fPhi3);

          ASSERT(fRatedCOP != 0, sprintf(msg, "Assertion Failure"));
          fCoolLoad = fPhi3 * ((((fTotal_cap + fP_Fan) / fRatedCOP) * fPhi1 * fPhi2) + fP_Fan);
        } else /* fPlr < 0.0 */
          fCoolLoad = 0.0;

      } else {   // CE_EVAPORATIVE

        fSens_cap = (float)(((CP_AIR * fAirFlow * (mdi->clg.saturating_eff_for_evaporative / 100.0)) *
                             (fTdbOutside - fTwbOutside_C)) -
                            fP_Fan);

        ASSERT(fSens_cap != 0, sprintf(msg, "Assertion Failure"));
        fPlr = fClgLoad / fSens_cap;
        if (fPlr > 1.0)
          fPlr = 1.0;

        fCoolLoad = fP_Fan;
      }
      /***************
      NLW 08/25/97 - If the part load ratio (PLR) is clipped to 1.0,
      then the cooling equipment does not meet the load.  Flags are
      set here to print this condition in the savings output file.
      ***************/
      if (mir->flgWhichPass == BASE_CASE) {
        if (fPlr >= 1.0) {
          mir->flgPreHighCLGLoad = TRUE;
          mir->iPreHighLoadMonths[zMonth] = TRUE;
        }
        //              else
        //                  mir->iPreHighLoadMonths[zMonth] = FALSE;
      } else if (mir->flgWhichPass == CUMULATIVE) {
        if (fPlr >= 1.0) {
          mir->flgPostHighCLGLoad = TRUE;
          mir->iPostHighLoadMonths[zMonth] = TRUE;
        }
        //              else
        //                  mir->iPostHighLoadMonths[zMonth] = FALSE;
      }

      /***********************
      Total cooling day/night energy use for the month and
      convert back to English units (kWh -> Btu/month).
      ***********************/
      if (flgDay) // Daytime
      {
        if (fPlr < 1.0)
          fMetLoadDay = fClgLoad;
        else
          fMetLoadDay = (float)(fSens_cap * (1.0 - fDistlossfactor));

        // 8/28/97, NLW: Amazingly, a factor of 3143 was used here to convert
        // kWh to Btu - corrected to use BTU_PER_KWH!  Same correction in secondary calcs.

        *fClgEnergyDay = fCoolLoad * fHours * fNumDays * (float)BTU_PER_KWH;
      }

      else // Nighttime
      {
        if (fPlr < 1.0)
          fMetLoadNight = fClgLoad;
        else
          fMetLoadNight = (float)(fSens_cap * (1.0 - fDistlossfactor));

        *fClgEnergyNight = fCoolLoad * fNightHours * fNumDays * (float)BTU_PER_KWH;

        // let's do the setback calculations with the same shifting
        // day hours onto night time loads as is done in heating, MJF 8/02

        if (fDayHours > 12.0)
          *fClgEnergyDay += fCoolLoad * (fDayHours - fHours) * fNumDays * (float)BTU_PER_KWH;
      }

      flgDay = 0;

    } // End for( i = 0; i <= 2; i++ )

    if (cmds.debug_level & D_MHEA_ENERGY_DETAIL && iMonth == JULY) {   // July
      fprintf(stderr, "get_cooling_consumption1 Month = %d \n", iMonth);
      fprintf(stderr, " fAirRate = %8.3f\n", fAirRate);
      fprintf(stderr, " fAirFlow = %8.3f\n", fAirFlow);
      fprintf(stderr, " fRatedCoolCapacity = %8.3f\n",fRatedCoolCapacity);
      fprintf(stderr, " fRatedCOP = %8.3f\n", fRatedCOP);
      fprintf(stderr, " fClgDuctLocation = %8.3f\n", fClgDuctLocation);
      fprintf(stderr, " fClgDuctInsulation = %8.3f\n", fClgDuctInsulation);
      fprintf(stderr, " fDistlossfactor = %8.3f\n",fDistlossfactor);
      fprintf(stderr, " fClgLoadDay = %8.3f\n", fClgLoadDay);
      fprintf(stderr, " fClgLoadNight = %8.3f\n", fClgLoadNight);
      fprintf(stderr, " fDayCoolSet = %8.3f\n", fDayCoolSet);
      fprintf(stderr, " fNightCoolSet = %8.3f\n", fNightCoolSet);
      fprintf(stderr, " fTdbOutsideDay = %8.3f\n", fTdbOutsideDay);
      fprintf(stderr, " fTdbOutsideNight = %8.3f\n", fTdbOutsideNight);
      fprintf(stderr, " fDayHours = %8.3f\n", fDayHours);  
      fprintf(stderr, " fNightHours = %8.3f\n", fNightHours);
      fprintf(stderr, " fTdbOutside = %8.3f\n",  fTdbOutside);
      fprintf(stderr, " fTwbOutside = %8.3f\n", fTwbOutside);
      fprintf(stderr, " fT_dc = %8.3f\n", fT_dc);
      fprintf(stderr, " fTwi_star = %8.3f\n", fTwi_star);
      fprintf(stderr, " fTwi = %8.3f\n", fTwi);
      fprintf(stderr, " fSens_cap = %8.3f\n", fSens_cap);
      fprintf(stderr, " fTotal_cap = %8.3f\n", fTotal_cap);
      fprintf(stderr, " fPlr = %8.3f\n", fPlr);
      fprintf(stderr, " fPhi1 = %8.3f\n", fPhi1);
      fprintf(stderr, " fPhi2 = %8.3f\n", fPhi2);
      fprintf(stderr, " fPhi3 = %8.3f \n", fPhi3);
      fprintf(stderr, " fCoolLoad = %8.3f\n", fCoolLoad);
      fprintf(stderr, " fMetLoadDay = %8.3f\n", fMetLoadDay);
      fprintf(stderr, " fMetLoadNight = %8.3f\n", fMetLoadNight);
      fprintf(stderr, " fClgEnergyDay(kBtu) = %8.1f\n", *fClgEnergyDay/1000);
      fprintf(stderr, " fClgEnergyNight(kBtu) = %8.1f \n\n", *fClgEnergyNight/1000);
    }

  }   // End long else for primary equipment other than central A/C with SEER

  /*****************
  Consider the secondary cooling system if one exists. Eliminated the
  conditional restricting computation only if the Primary Equipment (fCoolLoad)
  cannot meet the hourly load (fClgLoad). Use % area cooled entries instead.
  MBG 11/8/02
  *****************/

  /*********************
  Reduce cooling load to the percent of the home cooled by system and convert
  to metric (kWh) units and give new variable name.
  *********************/

  fClgLoadDay = (float)((fClgHourLoadDay / (float)BTU_PER_KWH) * (mdi->cl2.percent_area_room_ac / 100.0));
  fClgLoadNight = (float)((fClgHourLoadNight / (float)BTU_PER_KWH) * (mdi->cl2.percent_area_room_ac / 100.0));

  /*************************
  Get the Secondary Cooling Equipment data.
  *************************/

  if (mdi->cl2.equip_type == 0 || mdi->cl2.equip_type == CE_NONE) {

    /************************
    If no cooling equipment, so no effect on cooling consumption
    No need to increment by zero...
    *************************/\
    //*fClgEnergyDay += 0.0;
    //*fClgEnergyNight += 0.0;

  } else { // start of a very long else clause IF there IS secondary cooling equip

    /*********************
    If secondary equipment type is Room AC, then compute the air flow rate
    (meters^3/sec.) and the hours of operation.
    *********************/
    if (mdi->cl2.equip_type == CE_ROOMAC) {
      if (mdi->cl2.capacity <= 16.0)
        fAirRate = (float)(400.0 * CFMTOCMS);
      else if ((mdi->cl2.capacity > 16.0) && (mdi->cl2.capacity <= 26.0))
        fAirRate = (float)(700.0 * CFMTOCMS);
      else if (mdi->cl2.capacity > 26.0)
        fAirRate = (float)(1000.0 * CFMTOCMS);

      /** remove reduced operation hours for room AC - see note is primary cooling section
      // 8/28/    97, NLW

      switch( zMonth )
          {
          case  0: fDayHours =   0.0;          fNightHours = 0.0; break;
          case  1: fDayHours =   0.0;          fNightHours = 0.0; break;
          case  2: fDayHours =   0.0;          fNightHours = 0.0; break;
          case  3: fDayHours =  75.0/fNumDays; fNightHours = 0.0; break;
          case  4: fDayHours =  75.0/fNumDays; fNightHours = 0.0; break;
          case  5: fDayHours = 100.0/fNumDays; fNightHours = 0.0; break;
          case  6: fDayHours = 200.0/fNumDays; fNightHours = 0.0; break;
          case  7: fDayHours = 200.0/fNumDays; fNightHours = 0.0; break;
          case  8: fDayHours = 100.0/fNumDays; fNightHours = 0.0; break;
          case  9: fDayHours =   0.0;          fNightHours = 0.0; break;
          case 10: fDayHours =   0.0;          fNightHours = 0.0; break;
          case 11: fDayHours =   0.0;          fNightHours = 0.0; break;
          }
                  **/

    } else if (mdi->cl2.equip_type == CE_EVAPORATIVE) {

      /* Air Flow Rate in m^3/s (volume in ft^3 * conversion factor). */
      // fAirRate = fVolume * 0.2943;
      // 8/8/97,NLW Modify air flow rates and fan power for evap cooling equip.
      //   Based on units listed in 1997 RS MEANS Mech Cost Data (p271)
      //   two unit sizes will be used:
      //         1/3HP, 115V, 1785CFM and
      //         1/2HP, 115V, 3235CFM

      if (fHomeWidth < 20.0) {
        fAirRate = (float)(1785.0 * CFMTOCMS);
        fP_Fan = 0.250;
      } else {
        fP_Fan = 0.375;
        fAirRate = (float)(3235.0 * CFMTOCMS);
      }
    }

    else /* all other cooling equipment types */
    {
      if (fHomeWidth < 20.0)
        fAirRate = (float)(2500.0 * CFMTOCMS);
      else
        fAirRate = (float)(5000.0 * CFMTOCMS);
    }

    /***********************
    Set fAirFlow = fAirRate (m^/s) * adjusted air density (kg/m^3).
    ***********************/
    fAirFlow = (float)(fAirRate * AIR_DENSITY * exp(-0.0001219755 * fElevation * 0.3048));

    fNewInfMassFlow = (float)(fInfMassFlow * CFMTOCMS * AIR_DENSITY * exp(-0.0001219755 * fElevation * 0.3048));

    /**********************
    Convert Capacity from Btuh to kW to keep in consistent units.
    Accommodate efficiencies entered as COP, EER, or SEER (MBG 1/30/03)
    **********************/
    fRatedCoolCapacity = mdi->cl2.capacity * 1000.0f / (float)BTU_PER_KWH;
    // if (mdi->cl2.eff_units == CE_COP)
    //   fRatedCOP = mdi->cl2.eff;
    // else
    //   fRatedCOP = (float)(mdi->cl2.eff / 3.413);
    fRatedCOP = get_cooling_cop(mdi->cl2.eff_units, mdi->cl2.efficiency_cop, mdi->cl2.efficiency_seer, mdi->cl2.efficiency_eer);

    // changed to account for specific COP coming from the
    // clr replacement strucure 7/09 for 8.6
    //
    // if( mir->flgRetrofits[M_CMS_TUNE_COOLING_SYSTEM] ||
    //    mir->flgRetrofits[M_CMS_REPLACE_DX_COOLING_EQUIP])
    //    fRatedCOP = 3.0;

    // #334 The cooling system tune up now effects the mdi in measure.c
    // if (mir->flgRetrofits[M_CMS_TUNE_COOLING_SYSTEM])
    //   fRatedCOP = 3.0;

    /****************************
    Determine overall distribution loss factor
    ******************************/
    if (mdi->cl2.equip_type == CE_EVAPORATIVE || mdi->cl2.equip_type == CE_ROOMAC)
      fDistlossfactor = 0.0;
    else /* centralAC or heatPump */
    {

      fDistlossfactor = 0.0;      // MJF #70

      // Retained for possible secondary system duct loss consideration, but as of 2019, ignore
      // duct lost factor for ducted secondary cooling systems (should be pretty rare for manufactured house
      // with the exception of addtions)

      // if (mdi->cl2.clg_duct_location == DL_FLOOR)
      //   fClgDuctLocation = 0.50;
      // if (mdi->cl2.clg_duct_location == DL_CEILING)
      //   fClgDuctLocation = 0.75;
      // if (mdi->cl2.clg_duct_location == DL_NONE || mdi->cl2.clg_duct_location == 0)
      //   fClgDuctLocation = 0.0;

      // if (mdi->cl2.clg_duct_insl == DI_ABOVE || mdi->cl2.clg_duct_insl == DI_BELOW || mdi->cl2.clg_duct_insl == DI_NONE ||
      //     mdi->cl2.clg_duct_insl == 0)
      //   fClgDuctInsulation = 50.0;
      // else /* mdi->cl2.clg_duct_insl == DI_AROUND */
      // {
      //   if (mdi->cl2.clg_duct_location == DL_FLOOR) {
      //     if ((mdi->flr.belly_mineral_insl >= 2.0) ||
      //         ((mdi->flr.belly_mineral_insl + mdi->flr.belly_loose_insl) >= mdi->flr.belly_depth))
      //       fClgDuctInsulation = 8.75; /* for 2" insulation or more */
      //     else
      //       fClgDuctInsulation = 25.0; /* for 1" insulation */
      //   }
      //   if (mdi->cl2.clg_duct_location == DL_CEILING) {
      //     if ((mdi->rof.mineral_insl >= 2.0) || ((mdi->rof.mineral_insl + mdi->rof.loose_insl) >= 8.0))
      //       fClgDuctInsulation = 8.75; /* for 2" insulation or more */
      //     else
      //       fClgDuctInsulation = 25.0; /* for 1" insulation */
      //   }
      // }

      // fDistlossfactor = (float)(fClgDuctLocation * fClgDuctInsulation / 100.0);

      // // 5/8/95, NW - added an adjustment factor of 0.40 to reduce maximum distribution
      // //         loss factor.  Per NREL TP-253-4490 (page 12) the worst-case effective
      // //         furnace efficiency should be approx. 56%.

      // // NW,8/31/95: Update adjustment factor to 0.55 after correction of billing
      // //         adjustment problem which biased previous measure savings results

      // fDistlossfactor *= 0.30F;

      // if (mir->flgRetrofits[M_CMS_SEAL_DUCTS])
      //   fDistlossfactor *= (float)(1.0 - (mdi->key.duct_sealing_distribution_loss_reduction / 100.0));

    }

    /********************************************************
     If originally given an SEER, eliminate conversion of intantaneous to
     seasonal value.  Use the seasonal COP or SEER given on input (MBG-1/30/03)
    ********************************************************/

    if (cmds.debug_level & D_MHEA_ENERGY_DETAIL && iMonth == JULY) {  // July  
      fprintf(stderr, "----COOLING ENERGY CONSUMPTION SECONDARY----\n");
    }

    // If given SEER, skip seasonal conversion

    if (mdi->cl2.eff_units == CE_SEER && mdi->cl2.equip_type != CE_EVAPORATIVE) {
      ASSERT(fRatedCOP != 0, sprintf(msg, "Assertion Failure"));
      ASSERT((1.0f - fDistlossfactor) != 0, sprintf(msg, "Assertion Failure"));
      *fClgEnergyDay += fClgLoadDay * fHours * fNumDays / fRatedCOP / (1.0f - fDistlossfactor) * (float)BTU_PER_KWH;
      *fClgEnergyNight += fClgLoadNight * fHours * fNumDays / fRatedCOP / (1.0f - fDistlossfactor) * (float)BTU_PER_KWH;

      if (cmds.debug_level & D_MHEA_ENERGY_DETAIL && iMonth == JULY) { // July
        fprintf(stderr, " SEER fClgEnergyDay(kBtu) = %8.1f\n", *fClgEnergyDay/1000);
        fprintf(stderr, " SEER fClgEnergyNight(kBtu) = %8.1f\n", *fClgEnergyNight/1000);
      }

    } else { 

      /*************************
      Loop through calculations twice, once for the day cooling energy
      calculations and once for the night cooling energy calculations.
      **************************/
      flgDay = 1;

      for (i = 0; i < 2; i++) {
        /********************
        Set variables for day or night period calculation and
        convert the hourly sensible cooling load to metric.
        ********************/
        if (flgDay) // It's the daytime.
        {
          fClgLoad = fClgLoadDay;       // in kW
          fSetpoint = fDayCoolSet;      // in deg C
          fTdbOutside = fTdbOutsideDay; //  in deg F
        } else                          // It's the nighttime.
        {
          fClgLoad = fClgLoadNight;       // in kW
          fSetpoint = fNightCoolSet;      // in deg C
          fTdbOutside = fTdbOutsideNight; // in deg F
        }

        /*************************
        Test for negative cooling load and cooling season
        (checked once each pass for day or night).
        **************************/
        if ((iSeason == HEATING) || (fClgLoad < 0.0))
          fClgLoad = 0.0;

        /*********************
        Calculate the outdoor humidity ratio (fW_out).
        (fTwbOutside is still in degrees F for the fW_out calcs.)
        **********************/
        fStep_one = (float)((0.1289706 / 10000.0) * pow((459.67 + fTwbOutside), 2));
        fStep_two = (float)((0.2478068 / 100000000.0) * pow((459.67 + fTwbOutside), 3));

        fP_ws = (float)(exp((-10440.4 / (float)(459.67 + fTwbOutside)) - 11.2946669 - (0.02700133 * (459.67 + fTwbOutside)) +
                            fStep_one - fStep_two + (6.5459673 * log(459.67 + fTwbOutside))));

        fPressure = (float)(((-0.000938 * fElevation) + 29.737) / 2.036);

        ASSERT((fPressure - fP_ws) != 0, sprintf(msg, "Assertion Failure"));
        fW_out = (float)((((1093 - 0.556 * fTwbOutside) * (0.62198 * fP_ws / (float)(fPressure - fP_ws))) -
                          (0.24 * (fTdbOutside - fTwbOutside))) /
                         (float)(1093.0 + 0.444 * fTdbOutside - fTwbOutside));

        /************************
        Convert the outdoor temps (wet and dry) to Metric units.
        **************************/
        fTdbOutside = (float)((fTdbOutside - 32.0) * (5.0 / 9.0));
        //              fTwbOutside = (fTwbOutside - 32.0) * (5.0 / 9.0);  MBG 8/03

        /***********************
        Procedure for mechanical compression cycle cooling equipment.
        ***********************/

        // remember we are inside an if clause confirming that we
        // DO have some kind of secondary cooling equipment

        if (mdi->cl2.equip_type == CE_CENTRALAC || mdi->cl2.equip_type == CE_ROOMAC || mdi->cl2.equip_type == CE_HEATPUMP) {
          /**********************
          Note that fT_dc definition indicates a recirculating system
          with no mixture of outside air.
          ************************/
          ASSERT((CP_AIR * fAirFlow) != 0, sprintf(msg, "Assertion Failure"));
          fT_dc = fSetpoint + ((float)fP_Fan / (float)(CP_AIR * fAirFlow));

          /********************
          Calculate fTwi_star, the apparatus wet bulb temp:
          the wet bulb temp of circulating rooom air above
          which the evaporator coil would become wet.
          ********************/
          fAlpha = (float)(CP_AIR * fAirFlow * (1.0 - fBy_pass) * (fT_dc - T_AIR));
          fBeta =
              (float)(fRatedCoolCapacity * (((0.00927 * fTdbOutside) - 0.8593) - (((0.00399 * fTdbOutside) - 2.7351) * C_SR)));
          fGamma = (float)(fRatedCoolCapacity * (0.0239 + (0.08204 * C_SR)));
          ASSERT(fGamma != 0, sprintf(msg, "Assertion Failure"));
          fTwi_star = (fAlpha + fBeta) / fGamma;

          ASSERT(fNewInfMassFlow != 0, sprintf(msg, "Assertion Failure"));
          fW_in = (float)(fW_out + (MOIST_GENER / fNewInfMassFlow));
          fTwi = (float)(((1000000.0 * fW_in) +
                          ((((0.0015 * fElevation * 0.3048) + 413.6) * fSetpoint) + ((0.38 * fElevation * 0.3048) + 2124.0))) /
                         (float)(1255.1 + 0.1295 * fElevation * 0.3048));

          if (fTwi > fTwi_star) /* operation with wet coil */
          {
            fTotal_cap = (float)(fRatedCoolCapacity * (0.8593 + 0.02394 * fTwi - 0.00927 * fTdbOutside) - fP_Fan);

            fSens_cap = (float)((CP_AIR * fAirFlow) * (1.0 - fBy_pass) * (fT_dc - T_AIR));

            // if( fSens_cap <= 0.0 )
            //    fSens_cap = 0.0;

            fSens_cap += (float)((C_SR * fRatedCoolCapacity) * (2.7351 - (0.08204 * fTwi) - (0.00399 * fTdbOutside)) - fP_Fan);
          } else /* operation with dry coil */
          {
            fSens_cap = (float)(CP_AIR * fAirFlow * (1.0 - fBy_pass) * (fT_dc - T_AIR));

            // if( fSens_cap <= 0.0 )
            //    fSens_cap = 0.0;

            fSens_cap +=
                (float)((C_SR * fRatedCoolCapacity) * (2.7351 - (0.08204 * fTwi_star) - (0.00399 * fTdbOutside)) - fP_Fan);

            fTotal_cap = fSens_cap;
          } // End if(Twi > Twi_star)

          /******************************
          Part Load ratio should = 1 if load is > derated capacity
          *******************************/
          if ((fClgLoad > (fSens_cap * (1.0 - fDistlossfactor))) == TRUE)
            fPlr = 1.0;
          else {
            ASSERT((fSens_cap * (1.0 - fDistlossfactor)) != 0, sprintf(msg, "Assertion Failure"));
            fPlr = (float)(fClgLoad / (fSens_cap * (1.0 - fDistlossfactor)));
          }

          /***********************
          Hourly cooling load energy consumption
          ***********************/
          if (fPlr > 0.0) {
            fPhi1 = (float)(0.8593 + (0.02394 * fTwi) - (0.00927 * fTdbOutside));
            fPhi2 = (float)(0.5967 - (0.01378 * fTwi) + (0.01926 * fTdbOutside));
            fPhi3 = (float)((0.2952 * pow(fPlr, 3)) - (0.6591 * pow(fPlr, 2)) + (1.3639 * fPlr));
            // ASSERT((fPhi2 * fPhi3) != 0, sprintf(msg, "Assertion Failure"));
            // fCOP = (fRatedCOP * fPlr) / (float)(fPhi2 * fPhi3);

            ASSERT(fRatedCOP != 0, sprintf(msg, "Assertion Failure"));
            fCoolLoad = fPhi3 * ((((fTotal_cap + fP_Fan) / fRatedCOP) * fPhi1 * fPhi2) + fP_Fan);
          } else /* fPlr < 0.0 */
            fCoolLoad = 0.0;

        } else {   // CE_EVAPORATIVE

          fSens_cap = (float)(((CP_AIR * fAirFlow * (mdi->cl2.saturating_eff_for_evaporative / 100.0)) *
                       (fTdbOutside - fTwbOutside_C)) -
                      fP_Fan);

          ASSERT(fSens_cap != 0, sprintf(msg, "Assertion Failure"));
          if ((fPlr = (fClgLoad / fSens_cap)) > 1.0)
            fPlr = 1.0;

          fCoolLoad = fP_Fan;
        }

        /***********************
        Total cooling day/night energy use for the month and
        convert back to English units (kWh -> Btu/month).
        ***********************/
        // Same correction as in primary cooling section for Btu conversion factor
        if (flgDay) // Daytime
        {

          *fClgEnergyDay += fCoolLoad * fHours * fNumDays * (float)BTU_PER_KWH;
        } else // Nighttime
        {

          *fClgEnergyNight += fCoolLoad * fNightHours * fNumDays * (float)BTU_PER_KWH;

          if (fDayHours > 12.0)
            *fClgEnergyDay += fCoolLoad * (fDayHours - fHours) * fNumDays * (float)BTU_PER_KWH;
        }

        flgDay = 0;

      } // End for( i = 0; i <= 2; i++ )
    }   // end of Long if clause if cooling eff is COP or EER

    if (cmds.debug_level & D_MHEA_ENERGY_DETAIL && iMonth == JULY) {   // July
      fprintf(stderr, "get_cooling_consumption2 Month = %d \n", iMonth);
      fprintf(stderr, " fAirRate = %8.3f\n", fAirRate);
      fprintf(stderr, " fAirFlow = %8.3f\n", fAirFlow);
      fprintf(stderr, " fClgDuctLocation = %8.3f\n", fClgDuctLocation);
      fprintf(stderr, " fClgDuctInsulation = %8.3f\n", fClgDuctInsulation);
      fprintf(stderr, " fDistlossfactor = %8.3f\n",fDistlossfactor);
      fprintf(stderr, " fClgLoadDay = %8.3f\n", fClgLoadDay);
      fprintf(stderr, " fClgLoadNight = %8.3f\n", fClgLoadNight);
      fprintf(stderr, " fDayCoolSet = %8.3f\n", fDayCoolSet);
      fprintf(stderr, " fNightCoolSet = %8.3f\n", fNightCoolSet);
      fprintf(stderr, " fTdbOutsideDay = %8.3f\n", fTdbOutsideDay);
      fprintf(stderr, " fTdbOutsideNight = %8.3f\n", fTdbOutsideNight);
      fprintf(stderr, " fDayHours = %8.3f\n", fDayHours);  
      fprintf(stderr, " fNightHours = %8.3f\n", fNightHours);
      fprintf(stderr, " fTdbOutside = %8.3f\n",  fTdbOutside);
      fprintf(stderr, " fTwbOutside = %8.3f\n", fTwbOutside);
      fprintf(stderr, " fTwi_star = %8.3f\n", fTwi_star);
      fprintf(stderr, " fTwi = %8.3f\n", fTwi);
      fprintf(stderr, " fSens_cap = %8.3f\n", fSens_cap);
      fprintf(stderr, " fTotal_cap = %8.3f\n", fTotal_cap);
      fprintf(stderr, " fPlr = %8.3f\n", fPlr);
      fprintf(stderr, " fCoolLoad = %8.3f\n", fCoolLoad);
      fprintf(stderr, " fClgEnergyDay(kBtu) = %8.1f\n", *fClgEnergyDay/1000);
      fprintf(stderr, " fClgEnergyNight(kBtu) = %8.1f \n\n", *fClgEnergyNight/1000);
    }

  }     // end of very LONG if clause for secondary cooling system

  return (0);

} // End void get_cooling_consumption( ... float *fClgEnergyDay ... ) user function.

float get_cooling_cop(int units, float efficiency_cop, float efficiency_seer, float efficiency_eer) {
  float fRatedCOP;
  switch (units) {
  case CE_COP:
    fRatedCOP = efficiency_cop;
    break;
  case CE_SEER:
    fRatedCOP = efficiency_seer;
    //adjust_seer(mir->fS_Dsgn_T_New, &fRatedCOP);
    adjust_seer(cwd->summer_hp_design_temp, &fRatedCOP);
    fRatedCOP /= 3.413F;
    break;
  case CE_EER:
    fRatedCOP = (float)(efficiency_eer / 3.413);
    break;
  default:
    return 0;
  }
  return fRatedCOP;
}