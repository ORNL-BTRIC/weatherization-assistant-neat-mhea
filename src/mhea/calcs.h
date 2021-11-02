/* --------------------------------------------------------*/
/* Prototypes for data structures refered to in enrgyuse.c */
/* --------------------------------------------------------*/
#ifndef _CALCS_H
#define _CALCS_H

/* --------------------------------------------------*/
/* Prototypes for functions refered to in enrgyuse.c */
/* --------------------------------------------------*/

float window_perimeter(M_WIN *win);
float door_perimeter(M_DOR *dor);

void ua_floor(float *, float *);
void ua_wall(float *, float *, float *, float *, float *, float *, float *, float *, float *, float *, float *, float *, float *);
void ua_window(float *, float *, float *, float *, float *, float *, float *, float *, float *, float *, float *, float *);
void ua_door(float *, float *, float *, float *, float *, float *, float *, float *, float *, float *);
void ua_roof(float, float *, float *, float *, float *, float *);

float component_solar_gain(int, int, float, float, float, float, float *, float, float, float, float, float, float);
/* void getFreeHeat( void );    */
void sky_radiation_losses(int, float, float, float, float, float *, float *);
void mhea_infiltration_losses(int, int, float, float *, float *, float, float, float *, float *, float *, int *);
void mhea_solar_storage(float, float, float, float, float, float, float *);
void get_balance_temp(float, float, float, float, float, float, float, float, float, float, float, float *, float *);

void get_degree_hours(float fNumDays, float fBaseTemps[], float fDH_Day[], float fDH_Night[], float fTBalanceDay, float fTBalanceNight,
                 float *fDegHourDay, float *fDegHourNight);

int get_hourly_loads(int iflgSeason, float fDayLength, float fCondLoss, float fInfilLoss, float fDegHourDay, float fDegHourNight,
             int *flgSetBack, float fTempDelta, float *fHourLoadDay, float *fHourLoadNight);

void get_heating_consumption(int, int, int, float, float, float, float, float, float, float, float *, float *);
int get_cooling_consumption(int, int, int, float, float, float, float, float, float, float, float, float, float, float *, float *,
                   float *);

float get_cooling_cop(int units, float efficiency_cop, float efficiency_seer, float efficiency_eer);

#endif // _CALCS_H
