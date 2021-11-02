/***************************************************************************
 * MODULE:       hvac_2.h            CREATED:    1/22/2020
 *
 * AUTHOR:       Mark Fishbaugher
 *
 * MDESC:        external function prototypes for this module
 ****************************************************************************/
#ifndef _HVAC_2_H
#define _HVAC_2_H

float standard_central_ac_seer(int year);
float standard_room_ac_seer(int year);
float standard_central_heatpump_hspf(int year);
float standard_central_heatpump_seer(int year);
float standard_room_heat_pump_hspf(int year);
float standard_room_heat_pump_seer(int year);

#endif /* _HVAC_2_H */
