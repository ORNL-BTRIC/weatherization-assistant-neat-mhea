/*
 * File:   ecms.h
 * Author: g9a
 *
 * Created on September 11, 2009, 3:04 PM
 */
/***************************************************************************
 * MODULE:       ecms.h            CREATED:      April 7, 1999
 *
 * AUTHOR:       Mark Fishbaugher  Fishbaugher and Associates  1999
 *
 * MDESC:        prototype declarations for the external functions in ecms.c
 ****************************************************************************/
#ifndef _ECMS_H
#define _ECMS_H

void first_pass_measures(void);
void second_pass_measure_interaction(int il);
int annual_energy_load_change(float duam[], float dfheat[], float *dhtld, float *dclld);
int measure_id(int j);  // XXX new functions
float CompFuelCost(void);
float DCompFuelCost(int life);

void smart_thermostat(int iperm);
void duct_insulation(void);
void white_roof_coating(int icall, int nmsi); // 11/10/10 - needs to be called from neat.c

float get_air_space_r_value(float fAirSpaceDepth);

float attic_rcgable(int nc);

#endif /* _ECMS_H */
