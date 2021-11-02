/***************************************************************************
* MODULE:   hvac_2.c            CREATED:    12/22/2020
*
* AUTHOR:   Mark Fishbaugher
*
* MDESC:    Common HVAC calculations
****************************************************************************/

#include "wa_engine.h"

typedef struct {
  int year;
  float central_ac_seer;
  float room_ac_eer;
  float central_heatpump_hspf;
  float central_heatpump_seer;
  float room_heat_pump_hspf;
  float room_heat_pump_eer;
  // combustion heatings systems excluded
} HVAC_SYSTEM_EFFICIENCY_STANDARD;

// Source of data is work by Mini Malholtra, ORNL from multiple
// sources.  This data also used by MulTEA.
static HVAC_SYSTEM_EFFICIENCY_STANDARD hvac_system_efficiency_standard[] = {
  {1970,  6.50,   5.80,   6.21,  5.50,   6.21,  5.80 },
  {1971,  6.58,   5.89,   6.21,  5.86,   6.21,  5.89 },
  {1972,  6.66,   5.98,   6.21,  6.21,   6.21,  5.98 },
  {1973,  6.75,   6.00,   6.21,  6.21,   6.21,  6.00 },
  {1974,  6.85,   6.10,   6.21,  6.21,   6.21,  6.10 },
  {1975,  6.97,   6.20,   6.21,  6.21,   6.21,  6.20 },
  {1976,  7.03,   6.40,   6.21,  6.87,   6.21,  6.40 },
  {1977,  7.13,   6.55,   6.21,  6.89,   6.21,  6.55 },
  {1978,  7.34,   6.72,   6.21,  7.24,   6.21,  6.72 },
  {1979,  7.47,   6.87,   6.21,  7.34,   6.21,  6.87 },
  {1980,  7.55,   7.02,   6.21,  7.51,   6.21,  7.02 },
  {1981,  7.78,   7.06,   6.21,  7.70,   6.21,  7.06 },
  {1982,  8.31,   7.14,   6.21,  7.79,   6.21,  7.14 },
  {1983,  8.43,   7.29,   6.20,  8.23,   6.20,  7.29 },
  {1984,  8.66,   7.48,   6.36,  8.45,   6.36,  7.48 },
  {1985,  8.82,   7.70,   6.39,  8.56,   6.39,  7.70 },
  {1986,  8.87,   7.80,   6.55,  8.70,   6.55,  7.80 },
  {1987,  8.97,   8.06,   6.71,  8.93,   6.71,  8.06 },
  {1988,  9.11,   8.23,   6.88,  9.13,   6.88,  8.23 },
  {1989,  9.25,   8.48,   6.92,  9.26,   6.92,  8.48 },
  {1990,  9.31,   8.73,   7.03,  9.46,   7.03,  8.73 },
  {1991,  9.49,   8.80,   7.06,  9.77,   7.06,  8.80 },
  {1992,  10.46,  8.88,   7.10,  10.60,  7.10,  8.88 },
  {1993,  10.56,  9.05,   7.10,  10.86,  7.10,  9.05 },
  {1994,  10.61,  8.97,   7.10,  10.94,  7.10,  8.97 },
  {1995,  10.68,  9.03,   7.10,  10.97,  7.10,  9.03 },
  {1996,  10.68,  9.08,   7.40,  11.00,  7.40,  9.08 },
  {1997,  10.66,  9.09,   7.10,  10.97,  7.10,  9.09 },
  {1998,  10.92,  9.08,   7.40,  11.29,  7.40,  9.08 },
  {1999,  10.96,  9.07,   7.40,  11.29,  7.40,  9.07 },
  {2000,  10.95,  9.30,   7.40,  11.21,  7.40,  9.30 },
  {2001,  11.07,  9.63,   7.40,  11.30,  7.40,  9.63 },
  {2002,  11.07,  9.75,   7.40,  11.31,  7.40,  9.75 },
  {2003,  11.19,  9.75,   7.40,  11.46,  7.40,  9.75 },
  {2004,  11.29,  9.71,   7.40,  11.56,  7.40,  9.71 },
  {2005,  11.32,  9.95,   7.40,  11.60,  7.40,  9.95 },
  {2006,  13.17,  10.02,  7.90,  13.17,  7.90,  10.02},
  {2007,  13.66,  9.81,   7.90,  13.66,  7.90,  9.81 },
  {2008,  13.76,  9.93,   7.90,  13.76,  7.90,  9.93 },
  {2009,  13.91,  10.08,  8.06,  14.02,  8.06,  10.08},
  {2010,  14.03,  10.50,  8.15,  14.18,  8.15,  10.50},
  {2011,  14.05,  10.83,  8.14,  14.12,  8.14,  10.83},
  {2012,  14.07,  10.89,  8.18,  14.17,  8.18,  10.89},
  {2013,  14.09,  11.06,  8.23,  14.27,  8.23,  11.06},
  {2014,  14.17,  11.25,  8.26,  14.29,  8.26,  11.25},
  {2015,  14.24,  11.59,  8.32,  14.39,  8.32,  11.59},
  {2016,  14.25,  11.42,  8.32,  14.41,  8.32,  11.42},
  {2017,  14.28,  11.37,  8.32,  14.45,  8.32,  11.37},
  {2018,  14.37,  11.46,  8.33,  14.49,  8.33,  11.46},
  {2019,  14.41,  11.55,  8.38,  14.65,  8.38,  11.55},
  {2020,  14.41,  11.55,  8.38,  14.65,  8.38,  11.55},
  {2021,  14.41,  11.55,  8.38,  14.65,  8.38,  11.55},
  {2022,  14.41,  11.55,  8.38,  14.65,  8.38,  11.55},
  {2023,  14.41,  11.55,  8.38,  14.65,  8.38,  11.55}
};

// from https://en.wikipedia.org/wiki/Seasonal_energy_efficiency_ratio 
// SEER = EER / 0.875
static float seer_from_eer(float eer) {
  return (eer / 0.875);
}

static int std_year_count(){
  return (sizeof(hvac_system_efficiency_standard) / sizeof(hvac_system_efficiency_standard[0]));
}

float standard_central_ac_seer(int year) {
  int count = std_year_count();
  float seer = hvac_system_efficiency_standard[0].central_ac_seer;
  for (int i = 1; i < (count-1); i++) 
    if (year == hvac_system_efficiency_standard[i].year) seer = hvac_system_efficiency_standard[i].central_ac_seer;
  if (year >= hvac_system_efficiency_standard[count-1].year) seer = hvac_system_efficiency_standard[count-1].central_ac_seer;
  return seer;
}

float standard_room_ac_seer(int year) {
  int count = std_year_count();
  float seer = seer_from_eer(hvac_system_efficiency_standard[0].room_ac_eer);
  for (int i = 1; i < (count-1); i++)
    if (year == hvac_system_efficiency_standard[i].year) seer = seer_from_eer(hvac_system_efficiency_standard[i].room_ac_eer);
  if (year >= hvac_system_efficiency_standard[count-1].year) seer = seer_from_eer(hvac_system_efficiency_standard[count-1].room_ac_eer);
  return seer;
}

float standard_central_heatpump_hspf(int year) {
  int count = std_year_count();
  float seer = hvac_system_efficiency_standard[0].central_heatpump_hspf;
  for (int i = 1; i < (count-1); i++) 
    if (year == hvac_system_efficiency_standard[i].year) seer = hvac_system_efficiency_standard[i].central_heatpump_hspf;
  if (year >= hvac_system_efficiency_standard[count-1].year) seer = hvac_system_efficiency_standard[count-1].central_heatpump_hspf;
  return seer;
}

float standard_central_heatpump_seer(int year) {
  int count = std_year_count();
  float seer = hvac_system_efficiency_standard[0].central_heatpump_seer;
  for (int i = 1; i < (count-1); i++) 
    if (year == hvac_system_efficiency_standard[i].year) seer = hvac_system_efficiency_standard[i].central_heatpump_seer;
  if (year >= hvac_system_efficiency_standard[count-1].year) seer = hvac_system_efficiency_standard[count-1].central_heatpump_seer;
  return seer;
}

float standard_room_heat_pump_hspf(int year) {
  int count = std_year_count();
  float seer = hvac_system_efficiency_standard[0].room_heat_pump_hspf;
  for (int i = 1; i < (count-1); i++) 
    if (year == hvac_system_efficiency_standard[i].year) seer = hvac_system_efficiency_standard[i].room_heat_pump_hspf;
  if (year >= hvac_system_efficiency_standard[count-1].year) seer = hvac_system_efficiency_standard[count-1].room_heat_pump_hspf;
  return seer;
}

float standard_room_heat_pump_seer(int year) {
  int count = std_year_count();
  float seer = seer_from_eer(hvac_system_efficiency_standard[0].room_heat_pump_eer);
  for (int i = 1; i < (count-1); i++)
    if (year == hvac_system_efficiency_standard[i].year) seer = seer_from_eer(hvac_system_efficiency_standard[i].room_heat_pump_eer);
  if (year >= hvac_system_efficiency_standard[count-1].year) seer = seer_from_eer(hvac_system_efficiency_standard[count-1].room_heat_pump_eer);
  return seer;
}

