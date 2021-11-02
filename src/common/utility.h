/***************************************************************************
 * MODULE:       utility.h            CREATED:      4/2020
 *
 * AUTHOR:       MJF
 *
 * MDESC:        Common utility function prototypes
 ****************************************************************************/
#ifndef _C_UTILITY_H
#define _C_UTILITY_H

void water_heater_factors(DWH *dwh);

int include_measure_in_package(int flag, int cms_measure_num, enum MEASURE_PACKAGE_SORT_PRIORITY priority, int required, float sir);
void positive_initial_cost_required(float cost, char *measure_name);
float calculate_sir(float lcc_save, float initial_cost);
enum INFILTRATION_REDUCTION_TREATMENT infiltration_reduction_treatment(INF inf);
enum MEASURE_PACKAGE_GROUPS measure_package_group(enum MEASURE_PACKAGE_SORT_PRIORITY priority);

char *lighting_type_name(enum LIGHTING_LAMP_TYPE lamp_type);

void adjust_hspf(float temp, float *fhspf);
void adjust_seer(float temp, float *fseer);

float water_heater_insulation_rpi(enum WH_INSULATION_TYPE insul_type);
int water_heater_replace_data_check(DWH dwh);

char *comma_delimited_strstr(char *haystack, char *needle);
int components_in_common(char *haystack_list, char *needle_list);

#endif /* _C_UTILITY_H */