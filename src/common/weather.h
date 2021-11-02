/***************************************************************************
 * MODULE:       weather.h            REWRITE:    May 9, 2020
 *
 * AUTHOR:       Mark Fishbaugher  (mark@fishbaugher.com)
 *
 * MDESC:        Our common weather data pre-processor
 ****************************************************************************/
#ifndef _WEATHER_H
#define _WEATHER_H

// our six time bins in the weather files
#define BIN_00_04 0
#define BIN_04_08 1
#define BIN_08_12 2
#define BIN_12_16 3
#define BIN_16_20 4
#define BIN_20_24 5

// Solar Load Ratio data array
#define SLR_NORTH 0
#define SLR_EAST_WEST 1
#define SLR_SOUTH 2
#define SLR_DIFFUSE 3

// Our 8 degree hour bins in the weather files
#define DH_BIN_40 0
#define DH_BIN_45 1
#define DH_BIN_50 2
#define DH_BIN_55 3
#define DH_BIN_57 4
#define DH_BIN_60 5
#define DH_BIN_65 6
#define DH_BIN_70 7
#define DH_BIN_75 8

#define NIGHT 0
#define DAY 1

#define HEATING 0
#define COOLING 1

#define MONTHS 12

#define JANUARY 1
#define FEBRUARY 2
#define MARCH 3
#define APRIL 4
#define MAY 5
#define JUNE 6
#define JULY 7
#define AUGUST 8
#define SEPTEMBER 9
#define OCTOBER 10
#define NOVEMBER 11
#define DECEMBER 12

enum SOLAR_ORIENTATION // NEAT uses NESW ordering in arrays whereas input is NSEW ordering
{ SOLAR_NORTH = 0,
  SOLAR_EAST,
  SOLAR_SOUTH,
  SOLAR_WEST,
  SOLAR_HORIZONTAL_TOTAL,
  SOLAR_DIFFUSE };

// ****************************************************
// The WEATHER struct is a representation of the info
// collected by the Setup - Weather Data Site input form.
// ****************************************************

typedef struct {

  // Solar Load Ratios (old slr.inp data)
  float slr_24[MONTHS + 1][SLR_DIFFUSE + 1];    // latitude 24 solar load ratio data
  float slr_32[MONTHS + 1][SLR_DIFFUSE + 1];    // latitude 32 solar load ratio data
  float slr_40[MONTHS + 1][SLR_DIFFUSE + 1];    // latitude 40 solar load ratio data
  float slr_48[MONTHS + 1][SLR_DIFFUSE + 1];    // latitude 48 solar load ratio data

  // weather file .WX data
  char city_name[CITYNAME_LEN];               // city name from the WX file
  float north_latitude;                       // (Degrees) = Degrees north latitude
  float altitude;                             // (feet) = Altitude of weather city
  float heating_design_temp;                  // heating design temperature F
  float cooling_design_temp;                  // cooling design temperature F
  float cooling_design_wetbulb_temp;          // coincident cooling design wetbulb temp F
  float winter_hp_design_temp;                // New (ASHRAE 1997 HOF) winter heat pump design adjust temperature F
  float summer_hp_design_temp;                // New (ASHRAE 1997 HOF) summer heat pump design adjust temperature F
  float avg_solar_horizontal[MONTHS + 1];     // average solar horizontal insolation (Btu/sqft/day)
  float avg_drybulb_temp[MONTHS + 1];         // average dry bulb temperature F
  float avg_wetbulb_temp[MONTHS + 1];         // average wet bulb temperature F
  float avg_wind_mph[MONTHS + 1];             // average wind speed (mph)

  float fTempOutBins[MONTHS + 1][BIN_20_24 + 1];   // outdoor drybulb temp for bin F
  float fTempWetBins[MONTHS + 1][BIN_20_24 + 1];   // outdoor wetbuld temp for bin F
  float fWindSpdBins[MONTHS + 1][BIN_20_24 + 1];   // windspeed MPH for bin

  float balance_point_temp[DH_BIN_75 + 1];      // balance point temperatures F
  float degree_hours[DAY + 1][COOLING + 1][DH_BIN_75 + 1][MONTHS + 1]; // degree F hours assoc with each balance_point_temp[]

  // computed or derived data from the above two sources of raw data
  float solar_load[MONTHS + 1][SOLAR_DIFFUSE + 1];           // interpolated solar load for local latitude by orientation in Btu/hr/sqft
  float solar_load_ratio[MONTHS + 1][SLR_DIFFUSE + 1];  // interpolated solar load ratios for local latitude by compass orientation

  float avg_daytime_temp[MONTHS + 1];         // temperature F outside avg daytime 12hr avg
  float avg_nighttime_temp[MONTHS + 1];       // temperature F outside avg nighttime 12hr avg
  float avg_temp[MONTHS + 1];                 // temperature F outside daily 24hr avg dry bulb F
  float avg_wet_temp[MONTHS + 1];             // temperature F outside daily 24hr avg wet bulb F

  float avg_drybulb_temp_55;                  // avg dry bulb temp F for all months where avg_drybulb_temp[i] < 55F

  float hdd65;          // ( Deg F - Days) = HDD base 65 F
  float cdd75;          // ( Deg F - Days) = CDD base 75 F
  float cdd74;          // ( Deg F - Days) = CDD base 74 F
  float cdd65;          // ( Deg F - Days) = CDD base 65 F
  float cdd70;          // ( Deg F - Days) = CDD base 70 F

  int days_in_month[MONTHS + 1];
  int days_in_year_for_month[MONTHS + 2];
  
} CWD;    // Common Weather Data

void read_weather_file(WTH *w);
float relative_humidity(float drybt, float wetbt, float alt);
float enthalpy(float dbt, float RH, float tambR, float *wout);
void adjusted_monthly_degree_hours(float tbalt[][COOLING + 1][MONTHS + 1], float adht[][MONTHS + 1]);
float interpolate_degree_hours(float balance_temp, int heat_cool, int day_night, int month);
int doy(int month, int day);

#endif // _WEATHER_H
