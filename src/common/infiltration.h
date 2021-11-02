/***************************************************************************
 * MODULE:       infiltration.h            CREATED:    7/29/2020
 *
 * AUTHOR:       Mark Fishbaugher
 *
 * MDESC:        external function prototypes for this module
 ****************************************************************************/
#ifndef _INFILTRATION_H
#define _INFILTRATION_H

float blower_door_corrected_cfm(float pressure_ratio, float blower_door_cfm);
float window_leakage_coef(enum LEAKINESS leak);
float window_storm_leak_coef(float leak_coef);

float door_leakage_coef(enum DOOR_LEAKINESS leak);

float duct_blower_corrected_cfm(float pressure_ratio, float duct_blaster_cfm);
float get_specific_infiltration(float v, float dt);

void monthly_infiltration(
  float whole_house_cfm_50_pre,
  float whole_house_cfm_50_post,
  float daytime_heating_setpoint,
  float stories,
  float whole_house_cfm[POST_RETROFIT + 1][MONTHS + 1]);

float window_constrain_cfm_factor(float whole_house_cfm[POST_RETROFIT + 1][MONTHS + 1], 
  float wn_cfm_tot[MONTHS + 1], 
  float dr_cfm_tot[MONTHS + 1]);
void window_constrain_cfm(float whole_house_cfm[POST_RETROFIT + 1][MONTHS + 1], 
  float wn_leak_cfm[NEAT_MAX_WIN][MONTHS + 1], 
  float wn_cfm_tot[MONTHS + 1], 
  float dr_cfm_tot[MONTHS + 1], 
  int num_win,
  float floor_area);
float window_sealing_leak_coef(float leak_coef);

float door_constrain_cfm_factor(float whole_house_cfm[POST_RETROFIT + 1][MONTHS + 1], 
  float wn_cfm_tot[MONTHS + 1], 
  float dr_cfm_tot[MONTHS + 1]);
void door_constrain_cfm(float whole_house_cfm[POST_RETROFIT + 1][MONTHS + 1], 
  float dr_leak_cfm[NEAT_MAX_WIN][MONTHS + 1], 
  float wn_cfm_tot[MONTHS + 1], 
  float dr_cfm_tot[MONTHS + 1], 
  int num_dor,
  float floor_area);

#endif /* _INFILTRATION_H */
