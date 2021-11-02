/*
 * File:   subs.h
 * Author: g9a
 *
 * Created on September 11, 2009, 3:03 PM
 */
/***************************************************************************
 * MODULE:       subs.h            CREATED:      April 23, 1999
 *
 * AUTHOR:       Mark Fishbaugher
 *
 * MDESC:        external functions of this module
 ****************************************************************************/
#ifndef _SUBS_H
#define _SUBS_H

char *comp_group_name(enum MEASURE_COMPONENT_GROUP_TYPE type);
char *comp_group_name_short(enum MEASURE_COMPONENT_GROUP_TYPE type);

enum SOLAR_ORIENTATION solar_orientation(enum ORIENTATION orient_enum);

float wall_stud_depth(int i);
char *wall_stud_size_name(enum STUD_SIZE stud_size);

int wall_add_insulation_material(enum WALLADDINSTYPE  ins_type);

int attic_add_insulation_material(enum ATTICADDINSTYPE ins_type, int cms_index);
int attic_kneewall_add_insulation_material(enum KNEEWALLADDINSTYPE  ins_type);

int floor_add_insulation_material(enum FLOORADDINSTYPE ins_type, int cms_index);
int foundation_wall_add_insulation_material(enum FOUNDATIONADDINSTYPE  ins_type);
int foundation_sill_add_insulation_material(enum SILLADDINSTYPE ins_type);
float foundation_sill_ua_value(int nc);

float window_u_value(enum FRAME_TYPE frame, enum GLAZINGTYPE glazing, enum LOAD_CALCULATION_TYPE calc);
float window_shgc(enum FRAME_TYPE frame, enum GLAZINGTYPE glazing);
float window_treatment_shgc(int treatment);
int window_not_suitable_for_shading_retrofit(int window_i);

float door_u_value(enum DOOR_TYPE door, enum STORM_DOOR_INFO storm, enum LOAD_CALCULATION_TYPE calc);

float attic_ins_exist_rpi(int nc);
float attic_ins_added_rpi(int nc);

char *strupr(char *s);
int getf_line(char line[], int max, FILE *fln);

int mutually_exclusive_measures(int i, int j);
int rankms(float elc[], int npt, int kwhc[]);
int solinc(float alat);
float specific_infiltration(float v, float dt);
int skipl(FILE *file, int lines);
int skipc(FILE *file, int nc);
int savebase(void);
int restorebase(void);

float CompFuelCost(void);
float DCompFuelCost(int life);

#endif /* _SUBS_H */
