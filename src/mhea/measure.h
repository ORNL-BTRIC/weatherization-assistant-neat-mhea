/***************************************************************************
* MODULE:   measure.h            CREATED:  9/15/01
*
* AUTHOR:   MJF (www.fishbaugher.com)
*
* MDESC:    prototype retrofit measure functions plus declaration of our
*           array of measure_function() function pointers
****************************************************************************/
#ifndef _MEASURE_H
#define _MEASURE_H

void copy_mdi(MDI **dest, MDI *src);

void retro_replace_heating(void);
void retro_seal_ducts(void);
void retro_air_seal(void);
void retro_insulate_wall_batt(void);
void retro_insulate_wall_batt_add(void);
void retro_insulate_wall_cellulose(void);
void retro_insulate_wall_cellulose_add(void);
void retro_insulate_wall_fiberglass(void);
void retro_insulate_wall_fiberglass_add(void);
void retro_insulate_belly_cellulose(void);
void retro_insulate_belly_cellulose_add(void);
void retro_insulate_belly_fiberglass(void);
void retro_insulate_belly_fiberglass_add(void);
void retro_insulate_roof_cellulose(void);
void retro_insulate_roof_cellulose_add(void);
void retro_insulate_roof_fiberglass(void);
void retro_insulate_roof_fiberglass_add(void);
void retro_skirt(void);
void retro_skirt_add(void);
void retro_white_coat(void);
void retro_white_coat_add(void);
//void retro_door_required(void);  #245
void retro_door(void);
void retro_door_add(void);
void retro_storm_door(void);
void retro_storm_door_add(void);
void retro_window(void);
void retro_window_add(void);
void retro_plastic_storm(void);
void retro_plastic_storm_add(void);
void retro_glass_storm(void);
void retro_glass_storm_add(void);
void retro_awning(void);
void retro_awning_add(void);
void retro_shade(void);
void retro_shade_add(void);
void retro_smart_thermostat(void);
void retro_tune_heating(void);
void retro_evaporative_cooling(void);
void retro_tune_cooling(void);
void retro_replace_cooling(void);
void retro_replace_lighting(void);
void retro_refrigerator(void);
void retro_water_heater_insulation(void);
void retro_water_heater_pipe_insulation(void);
void retro_show_heads(void);
void retro_water_heater(void);
void retro_window_sealing(void);
void retro_window_sealing_add(void);

void retro_itemized(void); // Purposely not in measure function array

// now here is our array of function pointers.  there should be
// MHEA_MAX_CMS entries in this array.  see the def.h file
// for the ordered list of measures -- this should be the same ordering

typedef void (*measure_function_pointer)(void);

measure_function_pointer Measure_Function[MHEA_MAX_CMS];

#else

extern measure_function_pointer *Measure_Function;

#endif

