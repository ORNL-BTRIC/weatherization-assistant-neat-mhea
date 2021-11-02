/***************************************************************************
* MODULE:       weather.c            CREATED:     Nov 5, 2018 (migration MJF)
*
* AUTHOR:       Mike Gettings    Oak Ridge National Laboratory
*               Mark Fishbaugher (mark@fishbaugher.com)
*
* MDESC:        Processes the ndi->weather structure into globals for use by engine
*               other weather related roiutines
****************************************************************************/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "wa_engine.h"

static void solar_load_ratio_data(void);
static void solar_load_ratio_calculate();
static void do_interpolate(float fract, float slr1[MONTHS + 1][SLR_DIFFUSE + 1], float slr2[MONTHS + 1][SLR_DIFFUSE + 1]);

// All weather and solar data read here
// Solar data base on north_latitude contained in weather WX file

void read_weather_file(WTH *w) {
  FILE *wxfile;
  char line[80];
  int nc, nmths = 0;
  char filepath[PATH_LEN];

  ASSERT(cwd, sprintf(msg, "You must have Common Weather Data structure to run engine"));

  // weather_name(filepath); // gets path name of the weather data file
  STRCPY(filepath, WEATHER_DIR);
  STRCAT(filepath, w->file);

  wxfile = fopen(filepath, "r");
  ASSERT(wxfile, sprintf(msg, "Failed to open the input weather file: %s code:%d:%s", filepath, errno, strerror(errno)));

  fgets(line, 80, wxfile);

  // the longest cityname is Truth or Consequences, NM which is 25
  // char long excluding the training newline character.  the cityname
  // variable should be dimensioned to at least 30 characters to hold
  // the longest name plus the newline character plus the null MJF 11/13/97

  line[29] = '\0';                  // truncate possible longest line
  STRCPY(cwd->city_name, line); // replaces previous sscanf function

  for (int m = 1; m <= MONTHS; m++) {

    /* Read in average and time-of-day values */

    // Avg Horiz Solar, Avg DB T, Avg WB T, Avg Wind Speed
    // Latitude, W Dsgn T, *S Dsgn T, *Mean WB T, Elevation (month 1 only)
    // clang-format off

    nc = getf_line(line, 80, wxfile);
    ASSERT(nc > 0, sprintf(msg, "Error in first line of wx file: %s", filepath));
    fgets(line, 80, wxfile);
    if (m == JANUARY) {

      sscanf(line, "%f%f%f%f%f%f%f%f%f%f%f\n", 
        &cwd->avg_solar_horizontal[m], 
        &cwd->avg_drybulb_temp[m],
        &cwd->avg_wetbulb_temp[m], 
        &cwd->avg_wind_mph[m],
        &cwd->north_latitude, 
        &cwd->heating_design_temp,    // fW_Dsgn_T
        &cwd->cooling_design_temp,    // fS_Dsgn_T
        &cwd->cooling_design_wetbulb_temp, 
        &cwd->altitude,               // fElevation
        &cwd->winter_hp_design_temp,  // fW_Dsgn_T_New
        &cwd->summer_hp_design_temp); // fS_Dsgn_T_New
    } else {
      sscanf(line, "%f%f%f%f\n", 
        &cwd->avg_solar_horizontal[m], 
        &cwd->avg_drybulb_temp[m], 
        &cwd->avg_wetbulb_temp[m],
        &cwd->avg_wind_mph[m]);
    }
    // clang-format on

    ASSERT(cwd->north_latitude > 20.0 && cwd->north_latitude < 52.0,
           sprintf(msg, "Latitude outside acceptable range: %f", cwd->north_latitude));

    // read dry bulb, wet bulb, and wind speed bin data

    for (int iHBin = 0; iHBin <= BIN_20_24; iHBin++)
      fscanf(wxfile, "%f", &cwd->fTempOutBins[m][iHBin]);
    for (int iHBin = 0; iHBin <= BIN_20_24; iHBin++)
      fscanf(wxfile, "%f", &cwd->fTempWetBins[m][iHBin]);
    for (int iHBin = 0; iHBin <= BIN_20_24; iHBin++)
      fscanf(wxfile, "%f", &cwd->fWindSpdBins[m][iHBin]);

    cwd->avg_nighttime_temp[m] = (cwd->fTempOutBins[m][BIN_00_04] + cwd->fTempOutBins[m][BIN_04_08] + cwd->fTempOutBins[m][BIN_20_24]) / 3.0f;
    
    cwd->avg_daytime_temp[m] = (cwd->fTempOutBins[m][BIN_08_12] + cwd->fTempOutBins[m][BIN_12_16] + cwd->fTempOutBins[m][BIN_16_20]) / 3.0f;
    
    cwd->avg_temp[m] = (cwd->fTempOutBins[m][BIN_00_04] + cwd->fTempOutBins[m][BIN_04_08] + cwd->fTempOutBins[m][BIN_20_24] +
                        cwd->fTempOutBins[m][BIN_08_12] + cwd->fTempOutBins[m][BIN_12_16] + cwd->fTempOutBins[m][BIN_16_20]) / 6.0f;
    
    cwd->avg_wet_temp[m] = (cwd->fTempWetBins[m][BIN_00_04] + cwd->fTempWetBins[m][BIN_04_08] + cwd->fTempWetBins[m][BIN_20_24] +
                           cwd->fTempWetBins[m][BIN_08_12] + cwd->fTempWetBins[m][BIN_12_16] + cwd->fTempWetBins[m][BIN_16_20]) / 6.0f;

    if ((65.0f - cwd->avg_drybulb_temp[m]) >= 10.0f) {
      cwd->avg_drybulb_temp_55 += (65.0f - cwd->avg_drybulb_temp[m]);
      nmths++;
    }

    // clang-format off
    // Read in degree hours at specified balance points */

    for (int bin = DH_BIN_75; bin >= DH_BIN_40; bin--) {
      fscanf(wxfile, "%4f%8f%8f%8f%8f\n", 
        &cwd->balance_point_temp[bin], 
        &cwd->degree_hours[DAY]  [HEATING][bin][m],
        &cwd->degree_hours[NIGHT][HEATING][bin][m], 
        &cwd->degree_hours[DAY]  [COOLING][bin][m],
        &cwd->degree_hours[NIGHT][COOLING][bin][m]);
    }
    // clang-format on
  }
  
  fclose(wxfile);

  if (nmths > 0)
    cwd->avg_drybulb_temp_55 /= nmths;
  else
    cwd->avg_drybulb_temp_55 = 0.0f;

  solar_load_ratio_data();
  solar_load_ratio_calculate();

  for (int j = 1; j <= MONTHS; j++) {
    for (int bin = DH_BIN_40; bin <= DH_BIN_75; bin++) {
      cwd->degree_hours[DAY][HEATING][bin][TOTAL] += cwd->degree_hours[DAY][HEATING][bin][j];
      cwd->degree_hours[NIGHT][HEATING][bin][TOTAL] += cwd->degree_hours[NIGHT][HEATING][bin][j];
      cwd->degree_hours[DAY][COOLING][bin][TOTAL] += cwd->degree_hours[DAY][COOLING][bin][j];
      cwd->degree_hours[NIGHT][COOLING][bin][TOTAL] += cwd->degree_hours[NIGHT][COOLING][bin][j];
    }
  }
  cwd->hdd65 = (cwd->degree_hours[DAY][HEATING][DH_BIN_65][TOTAL] + cwd->degree_hours[NIGHT][HEATING][DH_BIN_65][TOTAL]) / 24.0f;
  cwd->cdd65 = (cwd->degree_hours[DAY][COOLING][DH_BIN_65][TOTAL] + cwd->degree_hours[NIGHT][COOLING][DH_BIN_65][TOTAL]) / 24.0f;
  cwd->cdd70 = (cwd->degree_hours[DAY][COOLING][DH_BIN_70][TOTAL] + cwd->degree_hours[NIGHT][COOLING][DH_BIN_70][TOTAL]) / 24.0f;
  cwd->cdd75 = (cwd->degree_hours[DAY][COOLING][DH_BIN_75][TOTAL] + cwd->degree_hours[NIGHT][COOLING][DH_BIN_75][TOTAL]) / 24.0f;
  
  cwd->cdd74 = cwd->cdd75 + (cwd->cdd70 - cwd->cdd75) / 5.0f;

  cwd->days_in_month[JANUARY]     = 31;
  cwd->days_in_month[FEBRUARY]    = 28;
  cwd->days_in_month[MARCH]       = 31;
  cwd->days_in_month[APRIL]       = 30;
  cwd->days_in_month[MAY]         = 31;
  cwd->days_in_month[JUNE]        = 30;
  cwd->days_in_month[JULY]        = 31;
  cwd->days_in_month[AUGUST]      = 31;
  cwd->days_in_month[SEPTEMBER]   = 30;
  cwd->days_in_month[OCTOBER]     = 31;
  cwd->days_in_month[NOVEMBER]    = 30;
  cwd->days_in_month[DECEMBER]    = 31;

  cwd->days_in_year_for_month[0] = 0;
  cwd->days_in_year_for_month[JANUARY] = 1;
  for (int m = 2; m <= MONTHS + 1; m++) {   // yes there are 14 entries in the array
    cwd->days_in_year_for_month[m] = cwd->days_in_year_for_month[m - 1] + cwd->days_in_month[m - 1];
  }

  return;
}

// This assigns solar load ratio values from what what previously stored in a separate slr.inp file
// These should not change  #126 MJF 5/2020

static void solar_load_ratio_data(void) {
  ASSERT(cwd, sprintf(msg, "You must have CWD allocated"));

  // clang-format off
  // 24 latitude
  cwd->slr_24[JANUARY][SLR_NORTH]   = 0.143; cwd->slr_24[JANUARY][SLR_EAST_WEST]   = 0.578; cwd->slr_24[JANUARY][SLR_SOUTH]   = 1.265;
  cwd->slr_24[FEBRUARY][SLR_NORTH]  = 0.138; cwd->slr_24[FEBRUARY][SLR_EAST_WEST]  = 0.578; cwd->slr_24[FEBRUARY][SLR_SOUTH]  = 0.852;
  cwd->slr_24[MARCH][SLR_NORTH]     = 0.138; cwd->slr_24[MARCH][SLR_EAST_WEST]     = 0.540; cwd->slr_24[MARCH][SLR_SOUTH]     = 0.476;
  cwd->slr_24[APRIL][SLR_NORTH]     = 0.160; cwd->slr_24[APRIL][SLR_EAST_WEST]     = 0.522; cwd->slr_24[APRIL][SLR_SOUTH]     = 0.233;
  cwd->slr_24[MAY][SLR_NORTH]       = 0.219; cwd->slr_24[MAY][SLR_EAST_WEST]       = 0.512; cwd->slr_24[MAY][SLR_SOUTH]       = 0.172;
  cwd->slr_24[JUNE][SLR_NORTH]      = 0.259; cwd->slr_24[JUNE][SLR_EAST_WEST]      = 0.507; cwd->slr_24[JUNE][SLR_SOUTH]      = 0.171;
  cwd->slr_24[JULY][SLR_NORTH]      = 0.230; cwd->slr_24[JULY][SLR_EAST_WEST]      = 0.512; cwd->slr_24[JULY][SLR_SOUTH]      = 0.178;
  cwd->slr_24[AUGUST][SLR_NORTH]    = 0.174; cwd->slr_24[AUGUST][SLR_EAST_WEST]    = 0.522; cwd->slr_24[AUGUST][SLR_SOUTH]    = 0.236;
  cwd->slr_24[SEPTEMBER][SLR_NORTH] = 0.149; cwd->slr_24[SEPTEMBER][SLR_EAST_WEST] = 0.539; cwd->slr_24[SEPTEMBER][SLR_SOUTH] = 0.485;
  cwd->slr_24[OCTOBER][SLR_NORTH]   = 0.144; cwd->slr_24[OCTOBER][SLR_EAST_WEST]   = 0.557; cwd->slr_24[OCTOBER][SLR_SOUTH]   = 0.840;
  cwd->slr_24[NOVEMBER][SLR_NORTH]  = 0.147; cwd->slr_24[NOVEMBER][SLR_EAST_WEST]  = 0.576; cwd->slr_24[NOVEMBER][SLR_SOUTH]  = 1.252;
  cwd->slr_24[DECEMBER][SLR_NORTH]  = 0.147; cwd->slr_24[DECEMBER][SLR_EAST_WEST]  = 0.585; cwd->slr_24[DECEMBER][SLR_SOUTH]  = 1.452;
  for (int m = 1; m <= MONTHS; m++) {
    cwd->slr_24[m][SLR_DIFFUSE] = MIN(cwd->slr_24[m][SLR_NORTH], cwd->slr_24[m][SLR_SOUTH]);
  }

  // 32 latitude
  cwd->slr_32[JANUARY][SLR_NORTH]   = 0.154; cwd->slr_32[JANUARY][SLR_EAST_WEST]   = 0.635; cwd->slr_32[JANUARY][SLR_SOUTH]   = 1.666;
  cwd->slr_32[FEBRUARY][SLR_NORTH]  = 0.145; cwd->slr_32[FEBRUARY][SLR_EAST_WEST]  = 0.604; cwd->slr_32[FEBRUARY][SLR_SOUTH]  = 1.129;
  cwd->slr_32[MARCH][SLR_NORTH]     = 0.142; cwd->slr_32[MARCH][SLR_EAST_WEST]     = 0.572; cwd->slr_32[MARCH][SLR_SOUTH]     = 0.674;
  cwd->slr_32[APRIL][SLR_NORTH]     = 0.159; cwd->slr_32[APRIL][SLR_EAST_WEST]     = 0.544; cwd->slr_32[APRIL][SLR_SOUTH]     = 0.355;
  cwd->slr_32[MAY][SLR_NORTH]       = 0.202; cwd->slr_32[MAY][SLR_EAST_WEST]       = 0.528; cwd->slr_32[MAY][SLR_SOUTH]       = 0.228;
  cwd->slr_32[JUNE][SLR_NORTH]      = 0.233; cwd->slr_32[JUNE][SLR_EAST_WEST]      = 0.521; cwd->slr_32[JUNE][SLR_SOUTH]      = 0.201;
  cwd->slr_32[JULY][SLR_NORTH]      = 0.212; cwd->slr_32[JULY][SLR_EAST_WEST]      = 0.527; cwd->slr_32[JULY][SLR_SOUTH]      = 0.228;
  cwd->slr_32[AUGUST][SLR_NORTH]    = 0.171; cwd->slr_32[AUGUST][SLR_EAST_WEST]    = 0.543; cwd->slr_32[AUGUST][SLR_SOUTH]    = 0.352;
  cwd->slr_32[SEPTEMBER][SLR_NORTH] = 0.154; cwd->slr_32[SEPTEMBER][SLR_EAST_WEST] = 0.569; cwd->slr_32[SEPTEMBER][SLR_SOUTH] = 0.679;
  cwd->slr_32[OCTOBER][SLR_NORTH]   = 0.152; cwd->slr_32[OCTOBER][SLR_EAST_WEST]   = 0.598; cwd->slr_32[OCTOBER][SLR_SOUTH]   = 1.109;
  cwd->slr_32[NOVEMBER][SLR_NORTH]  = 0.157; cwd->slr_32[NOVEMBER][SLR_EAST_WEST]  = 0.631; cwd->slr_32[NOVEMBER][SLR_SOUTH]  = 1.646;
  cwd->slr_32[DECEMBER][SLR_NORTH]  = 0.161; cwd->slr_32[DECEMBER][SLR_EAST_WEST]  = 0.649; cwd->slr_32[DECEMBER][SLR_SOUTH]  = 1.936;
  for (int m = 1; m <= MONTHS; m++) {
    cwd->slr_32[m][SLR_DIFFUSE] = MIN(cwd->slr_32[m][SLR_NORTH], cwd->slr_32[m][SLR_SOUTH]);
  }

  // 40 latitude
  cwd->slr_40[JANUARY][SLR_NORTH]   = 0.177; cwd->slr_40[JANUARY][SLR_EAST_WEST]   = 0.745; cwd->slr_40[JANUARY][SLR_SOUTH]   = 2.357;
  cwd->slr_40[FEBRUARY][SLR_NORTH]  = 0.153; cwd->slr_40[FEBRUARY][SLR_EAST_WEST]  = 0.669; cwd->slr_40[FEBRUARY][SLR_SOUTH]  = 1.498;
  cwd->slr_40[MARCH][SLR_NORTH]     = 0.149; cwd->slr_40[MARCH][SLR_EAST_WEST]     = 0.619; cwd->slr_40[MARCH][SLR_SOUTH]     = 0.908;
  cwd->slr_40[APRIL][SLR_NORTH]     = 0.161; cwd->slr_40[APRIL][SLR_EAST_WEST]     = 0.577; cwd->slr_40[APRIL][SLR_SOUTH]     = 0.510;
  cwd->slr_40[MAY][SLR_NORTH]       = 0.199; cwd->slr_40[MAY][SLR_EAST_WEST]       = 0.554; cwd->slr_40[MAY][SLR_SOUTH]       = 0.331;
  cwd->slr_40[JUNE][SLR_NORTH]      = 0.225; cwd->slr_40[JUNE][SLR_EAST_WEST]      = 0.544; cwd->slr_40[JUNE][SLR_SOUTH]      = 0.280;
  cwd->slr_40[JULY][SLR_NORTH]      = 0.207; cwd->slr_40[JULY][SLR_EAST_WEST]      = 0.553; cwd->slr_40[JULY][SLR_SOUTH]      = 0.327;
  cwd->slr_40[AUGUST][SLR_NORTH]    = 0.173; cwd->slr_40[AUGUST][SLR_EAST_WEST]    = 0.573; cwd->slr_40[AUGUST][SLR_SOUTH]    = 0.501;
  cwd->slr_40[SEPTEMBER][SLR_NORTH] = 0.161; cwd->slr_40[SEPTEMBER][SLR_EAST_WEST] = 0.614; cwd->slr_40[SEPTEMBER][SLR_SOUTH] = 0.911;
  cwd->slr_40[OCTOBER][SLR_NORTH]   = 0.163; cwd->slr_40[OCTOBER][SLR_EAST_WEST]   = 0.659; cwd->slr_40[OCTOBER][SLR_SOUTH]   = 1.465;
  cwd->slr_40[NOVEMBER][SLR_NORTH]  = 0.178; cwd->slr_40[NOVEMBER][SLR_EAST_WEST]  = 0.718; cwd->slr_40[NOVEMBER][SLR_SOUTH]  = 2.254;
  cwd->slr_40[DECEMBER][SLR_NORTH]  = 0.184; cwd->slr_40[DECEMBER][SLR_EAST_WEST]  = 0.757; cwd->slr_40[DECEMBER][SLR_SOUTH]  = 2.748;
  for (int m = 1; m <= MONTHS; m++) {
    cwd->slr_40[m][SLR_DIFFUSE] = MIN(cwd->slr_40[m][SLR_NORTH], cwd->slr_40[m][SLR_SOUTH]);
  }

  // 48 latitude
  cwd->slr_48[JANUARY][SLR_NORTH]   = 0.212; cwd->slr_48[JANUARY][SLR_EAST_WEST]   = 0.884; cwd->slr_48[JANUARY][SLR_SOUTH]   = 3.453;
  cwd->slr_48[FEBRUARY][SLR_NORTH]  = 0.172; cwd->slr_48[FEBRUARY][SLR_EAST_WEST]  = 0.771; cwd->slr_48[FEBRUARY][SLR_SOUTH]  = 2.058;
  cwd->slr_48[MARCH][SLR_NORTH]     = 0.157; cwd->slr_48[MARCH][SLR_EAST_WEST]     = 0.689; cwd->slr_48[MARCH][SLR_SOUTH]     = 1.206;
  cwd->slr_48[APRIL][SLR_NORTH]     = 0.168; cwd->slr_48[APRIL][SLR_EAST_WEST]     = 0.627; cwd->slr_48[APRIL][SLR_SOUTH]     = 0.691;
  cwd->slr_48[MAY][SLR_NORTH]       = 0.206; cwd->slr_48[MAY][SLR_EAST_WEST]       = 0.593; cwd->slr_48[MAY][SLR_SOUTH]       = 0.462;
  cwd->slr_48[JUNE][SLR_NORTH]      = 0.232; cwd->slr_48[JUNE][SLR_EAST_WEST]      = 0.579; cwd->slr_48[JUNE][SLR_SOUTH]      = 0.394;
  cwd->slr_48[JULY][SLR_NORTH]      = 0.214; cwd->slr_48[JULY][SLR_EAST_WEST]      = 0.591; cwd->slr_48[JULY][SLR_SOUTH]      = 0.455;
  cwd->slr_48[AUGUST][SLR_NORTH]    = 0.181; cwd->slr_48[AUGUST][SLR_EAST_WEST]    = 0.621; cwd->slr_48[AUGUST][SLR_SOUTH]    = 0.675;
  cwd->slr_48[SEPTEMBER][SLR_NORTH] = 0.171; cwd->slr_48[SEPTEMBER][SLR_EAST_WEST] = 0.680; cwd->slr_48[SEPTEMBER][SLR_SOUTH] = 1.200;
  cwd->slr_48[OCTOBER][SLR_NORTH]   = 0.182; cwd->slr_48[OCTOBER][SLR_EAST_WEST]   = 0.756; cwd->slr_48[OCTOBER][SLR_SOUTH]   = 1.992;
  cwd->slr_48[NOVEMBER][SLR_NORTH]  = 0.216; cwd->slr_48[NOVEMBER][SLR_EAST_WEST]  = 0.868; cwd->slr_48[NOVEMBER][SLR_SOUTH]  = 3.363;
  cwd->slr_48[DECEMBER][SLR_NORTH]  = 0.234; cwd->slr_48[DECEMBER][SLR_EAST_WEST]  = 0.947; cwd->slr_48[DECEMBER][SLR_SOUTH]  = 4.369;
    for (int m = 1; m <= MONTHS; m++) {
    cwd->slr_48[m][SLR_DIFFUSE] = MIN(cwd->slr_48[m][SLR_NORTH], cwd->slr_48[m][SLR_SOUTH]);
  }
  // clang-format on

}

static void solar_load_ratio_calculate() {

  ASSERT(cwd, sprintf(msg, "You must have CWD allocated"));
  ASSERT(cwd->north_latitude > 22 && cwd->north_latitude < 50, sprintf(msg, "Outside allowable latitude for solar load ratios"));
  ASSERT(cwd->slr_24[1][1], sprintf(msg, "Must have allocated sloar load ratio data"));
  ASSERT(cwd->avg_solar_horizontal[1], sprintf(msg, "Weather file data must be populated"));

  if (cwd->north_latitude > 40.0) {
    do_interpolate((cwd->north_latitude - 40.0) / 8.0f, cwd->slr_40, cwd->slr_48);
  } else if (cwd->north_latitude > 32.0) {
    do_interpolate((cwd->north_latitude - 32.0) / 8.0f, cwd->slr_32, cwd->slr_40);
  } else {
    do_interpolate((cwd->north_latitude - 24.0) / 8.0f, cwd->slr_24, cwd->slr_32);
  }

}


static void do_interpolate(float fract, float slr1[MONTHS + 1][SLR_DIFFUSE + 1], float slr2[MONTHS + 1][SLR_DIFFUSE + 1]) {

  for (int m = 1; m <= MONTHS; m++) {

    // unitless solar load ratio (the ratio between horizontal plane and a plane with given orientationh)
    cwd->solar_load_ratio[m][SLR_NORTH] = slr1[m][SLR_NORTH] + (slr2[m][SLR_NORTH] - slr1[m][SLR_NORTH]) * fract;
    cwd->solar_load_ratio[m][SLR_EAST_WEST] = slr1[m][SLR_EAST_WEST] + (slr2[m][SLR_EAST_WEST] - slr1[m][SLR_EAST_WEST]) * fract;
    cwd->solar_load_ratio[m][SLR_SOUTH] = slr1[m][SLR_SOUTH] + (slr2[m][SLR_SOUTH] - slr1[m][SLR_SOUTH]) * fract;
    cwd->solar_load_ratio[m][SLR_DIFFUSE] = slr1[m][SLR_DIFFUSE] + (slr2[m][SLR_DIFFUSE] - slr1[m][SLR_DIFFUSE]) * fract;

    // solar loads in Btu/hr/sqft for different orientations
    cwd->solar_load[m][SOLAR_DIFFUSE] = (slr1[m][SLR_DIFFUSE] + fract * (slr2[m][SLR_DIFFUSE] - slr1[m][SLR_DIFFUSE])) *
                                       (cwd->avg_solar_horizontal[m] / 24.0f);

    cwd->solar_load[m][SOLAR_NORTH] =
        (slr1[m][SLR_NORTH] + fract * (slr2[m][SLR_NORTH] - slr1[m][SLR_NORTH])) * (cwd->avg_solar_horizontal[m] / 24.0f) -
        cwd->solar_load[m][SOLAR_DIFFUSE];

    cwd->solar_load[m][SOLAR_EAST] = (slr1[m][SLR_EAST_WEST] + fract * (slr2[m][SLR_EAST_WEST] - slr1[m][SLR_EAST_WEST])) *
                                        (cwd->avg_solar_horizontal[m] / 24.0f) -
                                    cwd->solar_load[m][SOLAR_DIFFUSE];

    cwd->solar_load[m][SOLAR_SOUTH] =
        (slr1[m][SLR_SOUTH] + fract * (slr2[m][SLR_SOUTH] - slr1[m][SLR_SOUTH])) * (cwd->avg_solar_horizontal[m] / 24.0f) -
        cwd->solar_load[m][SOLAR_DIFFUSE];

    cwd->solar_load[m][SOLAR_WEST] = cwd->solar_load[m][SOLAR_EAST];

    cwd->solar_load[m][SOLAR_HORIZONTAL_TOTAL] = (cwd->avg_solar_horizontal[m] / 24.0f) - cwd->solar_load[m][SOLAR_DIFFUSE];
  }

  // sum them up and store in the 0th element still in units of Btu/hr/sqft for the whole year

  cwd->solar_load[TOTAL][SOLAR_NORTH] = 0.0f;
  cwd->solar_load[TOTAL][SOLAR_EAST] = 0.0f;
  cwd->solar_load[TOTAL][SOLAR_SOUTH] = 0.0f;
  cwd->solar_load[TOTAL][SOLAR_WEST] = 0.0f;

  for (int m = 1; m <= MONTHS; m++) {
    cwd->solar_load[TOTAL][SOLAR_NORTH] += cwd->solar_load[m][SOLAR_NORTH];
    cwd->solar_load[TOTAL][SOLAR_EAST] += cwd->solar_load[m][SOLAR_EAST];
    cwd->solar_load[TOTAL][SOLAR_SOUTH] += cwd->solar_load[m][SOLAR_SOUTH];
    cwd->solar_load[TOTAL][SOLAR_WEST] += cwd->solar_load[m][SOLAR_WEST];
  }

}

// This routine computes the relative humidity given the wet and dry-bulb temperatures.

#define HYWEX(t) (c1 / t + c2 + c3 * t + c4 * t * t + c5 * t * t * t + c6 * log(t))

float relative_humidity(float drybt, float wetbt, float alt) {
  double c1 = -1.044039708e4, c2 = -1.12946496e1, c3 = -2.7022355e-2, c4 = 1.289036e-5, c5 = -2.478068e-9, c6 = 6.5459673;
  double conv = 459.67;
  double pwsw, pwsd, patm, wsw, wsd, wnum, wden, w, mu, rh;

  drybt += (float)conv;
  wetbt += (float)conv;
  ASSERT(wetbt, sprintf(msg, "Need non zero temperature"));
  pwsw = HYWEX((double)wetbt);
  ASSERT(drybt, sprintf(msg, "Need non zero temperature"));
  pwsd = HYWEX((double)drybt);
  pwsw = exp(pwsw);
  pwsd = exp(pwsd);
  patm = 14.696 * exp(-0.0000368 * (double)alt);
  ASSERT((patm - pwsw), sprintf(msg, "Need non zero pressure"));
  ASSERT((patm - pwsd), sprintf(msg, "Need non zero pressure"));
  wsw = 0.62198 * pwsw / (patm - pwsw);
  wsd = 0.62198 * pwsd / (patm - pwsd);
  wnum = (1093. - 0.556 * wetbt) * wsw - 0.24 * (drybt - wetbt);
  wden = 1093. + 0.444 * drybt - wetbt;
  ASSERT(wden, sprintf(msg, "Need non zero temperature"));
  w = wnum / wden;
  ASSERT(wsd, sprintf(msg, "Need non zero pressure"));
  mu = w / wsd;
  ASSERT(patm, sprintf(msg, "Need non zero pressure"));
  ASSERT((1. - (1. - mu) * pwsd / patm), sprintf(msg, "Need non zero pressure"));
  rh = mu / (1. - (1. - mu) * pwsd / patm);
  return ((float)rh);
}

// Compute the enthalpy of air in space where return duct runs, given outdoor dry bulb temperature and 
// relative humidity and approximate temperature of the space.

float enthalpy(float dbt, float RH, float tambR, float *wout) {
  double c1 = -1.044039708e4, c2 = -1.12946496e1, c3 = -2.7022355e-2, c4 = 1.289036e-5, c5 = -2.478068e-9, c6 = 6.5459673;
  double conv = 459.67;
  float pws, pw, w, patm = 14.696f, h, dbtabs;

  // fprintf(stderr, "\n dbt = %5.1f,  RH = %5.2f,  tambR = %5.2f",dbt,RH,tambR);

  dbtabs = dbt + (float)conv;
  pws = (float)HYWEX((double)dbtabs);
  pws = (float)exp(pws);
  pw = RH * pws;
  w = 0.62198f * pw / (patm - pw);
  *wout = w;
  h = 0.240f * tambR + w * (1061.0f + 0.444f * tambR);
  // fprintf(stderr, "\n pws = %8.3f,  h = %8.3f", pws,h);
  return (h);
}

 // Routine to determine degree hours from balance point using linear interpolation between tabulated values from WX file

void adjusted_monthly_degree_hours(float tbalt[][COOLING + 1][MONTHS + 1], float adht[][MONTHS + 1]) {
  int is;       // heating or cooling
  int id;       // night and day
  int m;        // month base 1
  //int bin;      // temperature bin base 0
  //float fract, dtb;
  float adhs[DAY + 1][COOLING + 1][MONTHS + 1];

  for (is = HEATING; is <= COOLING; is++) {
    for (m = 1; m <= MONTHS; m++) {
      for (id = NIGHT; id <= DAY; id++) {
        adhs[id][is][m] = interpolate_degree_hours(tbalt[id][is][m], is, id, m);
      }
      adht[is][m] = adhs[NIGHT][is][m] + adhs[DAY][is][m];
    }
  }
  return;
}


float interpolate_degree_hours(float balance_temp, int heat_cool, int day_night, int month) {
  int bin;
  float fract = 1.0;
  float adjusted_degree_days = 0.0;

  for (bin = DH_BIN_45; bin <= DH_BIN_75; bin++) {
    if (balance_temp < cwd->balance_point_temp[bin]) {
      float dtb = cwd->balance_point_temp[bin] - cwd->balance_point_temp[bin - 1];
      ASSERT(dtb, sprintf(msg, "Need non zero temperature"));
      fract = (balance_temp - cwd->balance_point_temp[bin - 1]) / dtb;
      break;
    }
  }

  if (balance_temp <= cwd->balance_point_temp[DH_BIN_40])
    adjusted_degree_days = cwd->degree_hours[day_night][heat_cool][DH_BIN_40][month];
  else if (balance_temp >= cwd->balance_point_temp[DH_BIN_75])
    adjusted_degree_days = cwd->degree_hours[day_night][heat_cool][DH_BIN_75][month];
  else
    adjusted_degree_days = cwd->degree_hours[day_night][heat_cool][bin - 1][month] +
                      fract * (cwd->degree_hours[day_night][heat_cool][bin][month] - cwd->degree_hours[day_night][heat_cool][bin - 1][month]);

  return adjusted_degree_days;
}

/**********************
 **********************
 This function computes the day of year from month and day of month */

int doy(int month, int day) {
  int ndays = 0;

  for (int m = 1; m < month; m++)
    ndays += cwd->days_in_month[m];
  ndays += day;
  return (ndays);
}
