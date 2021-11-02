/***************************************************************************
 * MODULE:       subs.h            CREATED:      April 23, 1999
 *
 * AUTHOR:       Mark Fishbaugher
 *
 * MDESC:        external functions of this module
 ****************************************************************************/
#ifndef _FUELS_H
#define _FUELS_H

void initialize_fuel_cost_data(FCS fcs, FER *fer, float real_discount_rate);
void reset_fuel_reference_counts(void);

float fuel_cost(int fuel);
float fuel_cost_no_check(int fuel);

float fuel_heat_content(int fuel);
char *fuel_name(int fuel);
char *fuel_cost_units(int fuel);

float fuel_cost_per_mmbtu(int fuel);
float fuel_cost_per_mmbtu_no_check(int fuel);

enum LOGICAL is_fuel_referenced(int fuel);

void compute_upw_factors(FER *fer, float real_discount_rate);

float pw_fuel_cost(int fuel, int year);
float upw_fuel_factor(int fuel, int year);

int used_fuel_results(USED_FUEL *used_fuel);

#endif /* _FUELS_H */
