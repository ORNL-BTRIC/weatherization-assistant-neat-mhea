 /***************************************************************************
* MODULE:   neat_prep.c            CREATED:    February 24, 1999 (migrate Nov 5, 2018)
*
* AUTHOR:   Mike Gettings       Oak Ridge National Laboratory
*           Mark Fishbaugher    mark@fishbaugher.com
*
* MDESC:    This is used to compute a number of intermediate analysis variables
            from the globals constructed from BLD inputs. Originally called data22.c
****************************************************************************/
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wa_engine.h"\

static void weather_initialize(void);
static void window_initialize(void);
static void door_initialize(void);
static void foundation_initialize(void);
static void attic_initialize(void);
static void wall_initialize(void);

//static int compare_floats(const void *va, const void *vb);

/***************************************************************************
 ** Function Name: translate_parms
 **          Date: January 20, 1999
 **     Author(s): Mark Fishbaugher
 **
 **  DESCRIPTION: Some direct data translations
 **************************************************************************/
void translate_parms(void) {
 
  ndi->key.real_discount_rate /= 100; // as factor from percent
  ndi->key.real_discount_rate += 1;

  // See the engineering manual description of window film measures
  if (ndi->key.window_film_emittance > 0.0f && ndi->key.window_film_emittance < 0.84f) {
    ndi->key.window_film_delta_r = 0.04f + 0.9f / (ndi->key.window_film_emittance + 0.492f) - 0.716f;
  } else {
    ndi->key.window_film_delta_r = 0.0f;
  }

  return;
}

/***************************************************************************
 ** Function Name: translate_ndi
 **          Date: February 16, 1999
 **     Author(s): Mark Fishbaugher
 **
 **  DESCRIPTION: uses the ndi structure to fill in all of the global
 **  variables associated with the NDI files.  The neat_preparation() function in
 **  neat_prep.c is used to compute a number of intermediate analysis variables
 **  from these inputs.  See neat.h for declarations of all global variables
 **  that come from the NDI file
 **************************************************************************/
void translate_ndi(void) {

  ndi->gnl.impute_cooling = NO; // never impute cooling, but retain the code 2/2019 MJF

  duct_leakage_neat();

  // Convert itemized cost savings to units of MMBtu
  for (int nc = 0; nc < ndi->num_itc; nc++) {
    switch (ndi->itc[nc].units) {
    case EN_THERMS:
      ndi->itc[nc].savings /= 10.0;
      break;
    case EN_MMBTU:
      // nothing, already in MMBtu
      break;
    case EN_KWH:
      ndi->itc[nc].savings /= KWH_PER_MMBTU;
      break;
    }
  }

  for (int nc = 0; nc < ndi->num_htg; nc++) {
    ndi->htg[nc].percent_heat_supplied /= 100.f; // convert percentage to factor
  }

  if (ndi->htg[PRIMARY].duct_location == 0)
    ndi->htg[PRIMARY].duct_location = NO_DUCTS;

  if (ndi->htg[PRIMARY].fuel_type == ELECTRIC) {
    if (ndi->htg[PRIMARY].input_units == HEATING_KW)
      ndi->htg[PRIMARY].output_capacity *= 3.413f;
  }

  return;
}

/***************************************************************************
 ** Function Name: initialize
 **          Date: April 19, 1999
 **     Author(s): Mark Fishbaugher
 **
 **  DESCRIPTION: Here are some initialization statements to populate
 **  our measure desciption and type arrays plus allocate memory from the
 **  heap for our larger global variables.
 **************************************************************************/
void initialize_neat_measure_exclusion(void) {
  int n;

  // measure inter dependence/exclusion (cant do both to the same dwelling)

  n = N_CMS_ATTIC_INSULATION_R11;
  //STRCPY(nir->measure_name[n], "Attic Insulation R-11");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_HEATING_ENVELOPE; 
  nir->measure_exclusion_count[n] = 6;
  nir->measure_exclusion[n][0] = n;
  nir->measure_exclusion[n][1] = N_CMS_ATTIC_INSULATION_R19;
  nir->measure_exclusion[n][2] = N_CMS_ATTIC_INSULATION_R30;
  nir->measure_exclusion[n][3] = N_CMS_ATTIC_INSULATION_R38;
  nir->measure_exclusion[n][4] = N_CMS_FILL_CEILING_CAVITY;
  nir->measure_exclusion[n][5] = N_CMS_ATTIC_INSULATION_R49;

  n = N_CMS_ATTIC_INSULATION_R19;
  //STRCPY(nir->measure_name[n], "Attic Insulation R-19");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_HEATING_ENVELOPE;
  nir->measure_exclusion_count[n] = 6;
  nir->measure_exclusion[n][0] = n;
  nir->measure_exclusion[n][1] = n;
  nir->measure_exclusion[n][2] = N_CMS_ATTIC_INSULATION_R30;
  nir->measure_exclusion[n][3] = N_CMS_ATTIC_INSULATION_R38;
  nir->measure_exclusion[n][4] = N_CMS_FILL_CEILING_CAVITY;
  nir->measure_exclusion[n][5] = N_CMS_ATTIC_INSULATION_R49;

  n = N_CMS_ATTIC_INSULATION_R30;
  //STRCPY(nir->measure_name[n], "Attic Insulation R-30");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_HEATING_ENVELOPE;
  nir->measure_exclusion_count[n] = 6;
  nir->measure_exclusion[n][0] = N_CMS_ATTIC_INSULATION_R11;
  nir->measure_exclusion[n][1] = N_CMS_ATTIC_INSULATION_R19;
  nir->measure_exclusion[n][2] = n;
  nir->measure_exclusion[n][3] = N_CMS_ATTIC_INSULATION_R38;
  nir->measure_exclusion[n][4] = N_CMS_FILL_CEILING_CAVITY;
  nir->measure_exclusion[n][5] = N_CMS_ATTIC_INSULATION_R49;

  n = N_CMS_ATTIC_INSULATION_R38;
  //STRCPY(nir->measure_name[n], "Attic Insulation R-38");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_HEATING_ENVELOPE;
  nir->measure_exclusion_count[n] = 6;
  nir->measure_exclusion[n][0] = N_CMS_ATTIC_INSULATION_R11;
  nir->measure_exclusion[n][1] = N_CMS_ATTIC_INSULATION_R19;
  nir->measure_exclusion[n][2] = N_CMS_ATTIC_INSULATION_R30;
  nir->measure_exclusion[n][3] = n;
  nir->measure_exclusion[n][4] = N_CMS_FILL_CEILING_CAVITY;
  nir->measure_exclusion[n][5] = N_CMS_ATTIC_INSULATION_R49;

  n = N_CMS_WALL_INSULATION;
  //STRCPY(nir->measure_name[N_CMS_WALL_INSULATION], "Wall Insulation");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[N_CMS_WALL_INSULATION] = CMT_HEATING_ENVELOPE;
  nir->measure_exclusion_count[N_CMS_WALL_INSULATION] = 1;
  nir->measure_exclusion[N_CMS_WALL_INSULATION][0] = N_CMS_WALL_INSULATION;

  n = N_CMS_SILLBOX_INSULATION;
  //STRCPY(nir->measure_name[n], "Sillbox Insulation");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_HEATING_ENVELOPE;
  nir->measure_exclusion_count[n] = 5;
  nir->measure_exclusion[n][0] = n;
  nir->measure_exclusion[n][1] = N_CMS_FLOOR_INSULATION_R11;
  nir->measure_exclusion[n][2] = N_CMS_FLOOR_INSULATION_R19;
  nir->measure_exclusion[n][3] = N_CMS_FLOOR_INSULATION_R30;
  nir->measure_exclusion[n][4] = N_CMS_FLOOR_INSULATION_R38;

  n = N_CMS_FOUNDATION_WALL_INSULATION;
  //STRCPY(nir->measure_name[n], "Foundation Wall Insulation");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_HEATING_ENVELOPE;
  nir->measure_exclusion_count[n] = 5;
  nir->measure_exclusion[n][0] = n;
  nir->measure_exclusion[n][1] = N_CMS_FLOOR_INSULATION_R11;
  nir->measure_exclusion[n][2] = N_CMS_FLOOR_INSULATION_R19;
  nir->measure_exclusion[n][3] = N_CMS_FLOOR_INSULATION_R30;
  nir->measure_exclusion[n][4] = N_CMS_FLOOR_INSULATION_R38;

  n = N_CMS_FLOOR_INSULATION_R11;
  //STRCPY(nir->measure_name[n], "Floor Insulation R-11");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_HEATING_ENVELOPE;
  nir->measure_exclusion_count[n] = 7;
  nir->measure_exclusion[n][0] = N_CMS_SILLBOX_INSULATION;
  nir->measure_exclusion[n][1] = N_CMS_FOUNDATION_WALL_INSULATION;
  nir->measure_exclusion[n][2] = n;
  nir->measure_exclusion[n][3] = N_CMS_FLOOR_INSULATION_R19;
  nir->measure_exclusion[n][4] = N_CMS_FLOOR_INSULATION_R30;
  nir->measure_exclusion[n][5] = N_CMS_FLOOR_INSULATION_R38;
  nir->measure_exclusion[n][6] = N_CMS_FILL_FLOOR_CAVITY;

  n = N_CMS_FLOOR_INSULATION_R19;
  //STRCPY(nir->measure_name[n], "Floor Insulation R-19");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_HEATING_ENVELOPE;
  nir->measure_exclusion_count[n] = 7;
  nir->measure_exclusion[n][0] = N_CMS_SILLBOX_INSULATION;
  nir->measure_exclusion[n][1] = N_CMS_FOUNDATION_WALL_INSULATION;
  nir->measure_exclusion[n][2] = N_CMS_FLOOR_INSULATION_R11;
  nir->measure_exclusion[n][3] = n;
  nir->measure_exclusion[n][4] = N_CMS_FLOOR_INSULATION_R30;
  nir->measure_exclusion[n][5] = N_CMS_FLOOR_INSULATION_R38;
  nir->measure_exclusion[n][6] = N_CMS_FILL_FLOOR_CAVITY;

  n = N_CMS_FLOOR_INSULATION_R30;
  //STRCPY(nir->measure_name[n], "Floor Insulation R-30");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_HEATING_ENVELOPE;
  nir->measure_exclusion_count[n] = 7;
  nir->measure_exclusion[n][0] = N_CMS_SILLBOX_INSULATION;
  nir->measure_exclusion[n][1] = N_CMS_FOUNDATION_WALL_INSULATION;
  nir->measure_exclusion[n][2] = N_CMS_FLOOR_INSULATION_R11;
  nir->measure_exclusion[n][3] = N_CMS_FLOOR_INSULATION_R19;
  nir->measure_exclusion[n][4] = n;
  nir->measure_exclusion[n][5] = N_CMS_FLOOR_INSULATION_R38;
  nir->measure_exclusion[n][6] = N_CMS_FILL_FLOOR_CAVITY;

  n = N_CMS_FLOOR_INSULATION_R38;
  //STRCPY(nir->measure_name[n], "Floor Insulation R-38");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_HEATING_ENVELOPE;
  nir->measure_exclusion_count[n] = 7;
  nir->measure_exclusion[n][0] = N_CMS_SILLBOX_INSULATION;
  nir->measure_exclusion[n][1] = N_CMS_FOUNDATION_WALL_INSULATION;
  nir->measure_exclusion[n][2] = N_CMS_FLOOR_INSULATION_R11;
  nir->measure_exclusion[n][3] = N_CMS_FLOOR_INSULATION_R19;
  nir->measure_exclusion[n][4] = N_CMS_FLOOR_INSULATION_R30;
  nir->measure_exclusion[n][5] = n;
  nir->measure_exclusion[n][6] = N_CMS_FILL_FLOOR_CAVITY;

  n = N_CMS_STORM_WINDOWS;
  //STRCPY(nir->measure_name[n], "Storm Windows");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_WINDOWS_DOORS;
  nir->measure_exclusion_count[n] = 8;
  nir->measure_exclusion[n][0] = n;
  nir->measure_exclusion[n][1] = N_CMS_WINDOW_SHADING_AWNING;
  nir->measure_exclusion[n][2] = N_CMS_SUN_SCREEN_FABRIC;
  nir->measure_exclusion[n][3] = N_CMS_SUN_SCREEN_LOUVERED;
  nir->measure_exclusion[n][4] = N_CMS_WINDOW_FILM;
  nir->measure_exclusion[n][5] = N_CMS_LOW_E_WINDOWS;
  nir->measure_exclusion[n][6] = N_CMS_WINDOW_SEALING;
  nir->measure_exclusion[n][7] = N_CMS_WINDOW_REPLACEMENT;

  n = N_CMS_KNEEWALL_INSULATION;
  //STRCPY(nir->measure_name[n], "Kneewall Insulation");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_HEATING_ENVELOPE;
  nir->measure_exclusion_count[n] = 1;
  nir->measure_exclusion[n][0] = n;

  n = N_CMS_WINDOW_SHADING_AWNING;
  //STRCPY(nir->measure_name[n], "Window Shading");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_COOLING_ENVELOPE;
  nir->measure_exclusion_count[n] = 6;
  nir->measure_exclusion[n][0] = N_CMS_STORM_WINDOWS;
  nir->measure_exclusion[n][1] = n;
  nir->measure_exclusion[n][2] = N_CMS_SUN_SCREEN_FABRIC;
  nir->measure_exclusion[n][3] = N_CMS_SUN_SCREEN_LOUVERED;
  nir->measure_exclusion[n][4] = N_CMS_WINDOW_FILM;
  nir->measure_exclusion[n][5] = N_CMS_LOW_E_WINDOWS;

  n = N_CMS_SUN_SCREEN_FABRIC;
  //STRCPY(nir->measure_name[n], "Sun Screen, Fabric");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_COOLING_ENVELOPE;
  nir->measure_exclusion_count[n] = 6;
  nir->measure_exclusion[n][0] = N_CMS_STORM_WINDOWS;
  nir->measure_exclusion[n][1] = N_CMS_WINDOW_SHADING_AWNING;
  nir->measure_exclusion[n][2] = n;
  nir->measure_exclusion[n][3] = N_CMS_SUN_SCREEN_LOUVERED;
  nir->measure_exclusion[n][4] = N_CMS_WINDOW_FILM;
  nir->measure_exclusion[n][5] = N_CMS_LOW_E_WINDOWS;

  n = N_CMS_SUN_SCREEN_LOUVERED;
  //STRCPY(nir->measure_name[n], "Sun Screen, Louvered");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_COOLING_ENVELOPE;
  nir->measure_exclusion_count[n] = 6;
  nir->measure_exclusion[n][0] = N_CMS_STORM_WINDOWS;
  nir->measure_exclusion[n][1] = N_CMS_WINDOW_SHADING_AWNING;
  nir->measure_exclusion[n][2] = N_CMS_SUN_SCREEN_FABRIC;
  nir->measure_exclusion[n][3] = n;
  nir->measure_exclusion[n][4] = N_CMS_WINDOW_FILM;
  nir->measure_exclusion[n][5] = N_CMS_LOW_E_WINDOWS;

  n = N_CMS_WINDOW_FILM;
  //STRCPY(nir->measure_name[n], "Window Films");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_COOLING_ENVELOPE;
  nir->measure_exclusion_count[n] = 6;
  nir->measure_exclusion[n][0] = N_CMS_STORM_WINDOWS;
  nir->measure_exclusion[n][1] = N_CMS_WINDOW_SHADING_AWNING;
  nir->measure_exclusion[n][2] = N_CMS_SUN_SCREEN_FABRIC;
  nir->measure_exclusion[n][3] = N_CMS_SUN_SCREEN_LOUVERED;
  nir->measure_exclusion[n][4] = n;
  nir->measure_exclusion[n][5] = N_CMS_LOW_E_WINDOWS;

  n = N_CMS_LOW_E_WINDOWS;
  //STRCPY(nir->measure_name[n], "Low-E Windows");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_WINDOWS_DOORS;
  nir->measure_exclusion_count[n] = 8;
  nir->measure_exclusion[n][0] = N_CMS_STORM_WINDOWS;
  nir->measure_exclusion[n][1] = N_CMS_WINDOW_SHADING_AWNING;
  nir->measure_exclusion[n][2] = N_CMS_SUN_SCREEN_FABRIC;
  nir->measure_exclusion[n][3] = N_CMS_SUN_SCREEN_LOUVERED;
  nir->measure_exclusion[n][4] = N_CMS_WINDOW_FILM;
  nir->measure_exclusion[n][5] = n;
  nir->measure_exclusion[n][6] = N_CMS_WINDOW_SEALING;
  nir->measure_exclusion[n][7] = N_CMS_WINDOW_REPLACEMENT;

  n = N_CMS_THERMAL_VENT_DAMPER;
  //STRCPY(nir->measure_name[n], "Thermal Vent Damper");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_HEATING_SYSTEM_UPDATE;
  nir->measure_exclusion_count[n] = 7;
  nir->measure_exclusion[n][0] = n;
  nir->measure_exclusion[n][1] = N_CMS_ELECTRIC_VENT_DAMPER;
  nir->measure_exclusion[n][2] = N_CMS_ELECTRIC_VENT_DAMPER_AND_IID;
  nir->measure_exclusion[n][3] = N_CMS_FLAME_RETENTION_BURNER;
  nir->measure_exclusion[n][4] = N_CMS_REPLACE_HEATING_SYSTEM;
  nir->measure_exclusion[n][5] = N_CMS_HIGH_EFFICIENCY_FURNACE;
  nir->measure_exclusion[n][6] = N_CMS_HIGH_EFFICIENCY_BOILER;

  n = N_CMS_ELECTRIC_VENT_DAMPER;
  //STRCPY(nir->measure_name[n], "Electric Vent Damper");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_HEATING_SYSTEM_UPDATE;
  nir->measure_exclusion_count[n] = 7;
  nir->measure_exclusion[n][0] = N_CMS_THERMAL_VENT_DAMPER;
  nir->measure_exclusion[n][1] = n;
  nir->measure_exclusion[n][2] = N_CMS_ELECTRIC_VENT_DAMPER_AND_IID;
  nir->measure_exclusion[n][3] = N_CMS_FLAME_RETENTION_BURNER;
  nir->measure_exclusion[n][4] = N_CMS_REPLACE_HEATING_SYSTEM;
  nir->measure_exclusion[n][5] = N_CMS_HIGH_EFFICIENCY_FURNACE;
  nir->measure_exclusion[n][6] = N_CMS_HIGH_EFFICIENCY_BOILER;

  n = N_CMS_IID;
  //STRCPY(nir->measure_name[n], "Intermittent Ignition Device");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_HEATING_SYSTEM_UPDATE;
  nir->measure_exclusion_count[n] = 5;
  nir->measure_exclusion[n][0] = n;
  nir->measure_exclusion[n][1] = N_CMS_ELECTRIC_VENT_DAMPER_AND_IID;
  nir->measure_exclusion[n][2] = N_CMS_REPLACE_HEATING_SYSTEM;
  nir->measure_exclusion[n][3] = N_CMS_HIGH_EFFICIENCY_FURNACE;
  nir->measure_exclusion[n][4] = N_CMS_HIGH_EFFICIENCY_BOILER;

  n = N_CMS_ELECTRIC_VENT_DAMPER_AND_IID;
  //STRCPY(nir->measure_name[n], "Electric Vent Damper/IID");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_HEATING_SYSTEM_UPDATE;
  nir->measure_exclusion_count[n] = 8;
  nir->measure_exclusion[n][0] = N_CMS_THERMAL_VENT_DAMPER;
  nir->measure_exclusion[n][1] = N_CMS_ELECTRIC_VENT_DAMPER;
  nir->measure_exclusion[n][2] = N_CMS_IID;
  nir->measure_exclusion[n][3] = n;
  nir->measure_exclusion[n][4] = N_CMS_FLAME_RETENTION_BURNER;
  nir->measure_exclusion[n][5] = N_CMS_REPLACE_HEATING_SYSTEM;
  nir->measure_exclusion[n][6] = N_CMS_HIGH_EFFICIENCY_FURNACE;
  nir->measure_exclusion[n][7] = N_CMS_HIGH_EFFICIENCY_BOILER;

  n = N_CMS_FLAME_RETENTION_BURNER;
  //STRCPY(nir->measure_name[n], "Flame Retention Burners");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_HEATING_SYSTEM_UPDATE;
  nir->measure_exclusion_count[n] = 7;
  nir->measure_exclusion[n][0] = N_CMS_THERMAL_VENT_DAMPER;
  nir->measure_exclusion[n][1] = N_CMS_ELECTRIC_VENT_DAMPER;
  nir->measure_exclusion[n][2] = N_CMS_ELECTRIC_VENT_DAMPER_AND_IID;
  nir->measure_exclusion[n][3] = n;
  nir->measure_exclusion[n][4] = N_CMS_REPLACE_HEATING_SYSTEM;
  nir->measure_exclusion[n][5] = N_CMS_HIGH_EFFICIENCY_FURNACE;
  nir->measure_exclusion[n][6] = N_CMS_HIGH_EFFICIENCY_BOILER;

  n = N_CMS_FURNACE_TUNE_UP;
  //STRCPY(nir->measure_name[n], "Furnace Tuneup");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_HEATING_SYSTEM_UPDATE;
  nir->measure_exclusion_count[n] = 4;
  nir->measure_exclusion[n][0] = n;
  nir->measure_exclusion[n][1] = N_CMS_REPLACE_HEATING_SYSTEM;
  nir->measure_exclusion[n][2] = N_CMS_HIGH_EFFICIENCY_FURNACE;
  nir->measure_exclusion[n][3] = N_CMS_HIGH_EFFICIENCY_BOILER;

  n = N_CMS_REPLACE_HEATING_SYSTEM;
  //STRCPY(nir->measure_name[n], "Replace Heating System");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_HEATING_SYSTEM_REPLACE;
  nir->measure_exclusion_count[n] = 9;
  nir->measure_exclusion[n][0] = N_CMS_THERMAL_VENT_DAMPER;
  nir->measure_exclusion[n][1] = N_CMS_ELECTRIC_VENT_DAMPER;
  nir->measure_exclusion[n][2] = N_CMS_IID;
  nir->measure_exclusion[n][3] = N_CMS_ELECTRIC_VENT_DAMPER_AND_IID;
  nir->measure_exclusion[n][4] = N_CMS_FLAME_RETENTION_BURNER;
  nir->measure_exclusion[n][5] = N_CMS_FURNACE_TUNE_UP;
  nir->measure_exclusion[n][6] = n;
  nir->measure_exclusion[n][7] = N_CMS_HIGH_EFFICIENCY_FURNACE;
  nir->measure_exclusion[n][8] = N_CMS_HIGH_EFFICIENCY_BOILER;

  n = N_CMS_HIGH_EFFICIENCY_FURNACE;
  //STRCPY(nir->measure_name[n], "High Efficiency Furnace");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_HEATING_SYSTEM_REPLACE;
  nir->measure_exclusion_count[n] = 8;
  nir->measure_exclusion[n][0] = N_CMS_THERMAL_VENT_DAMPER;
  nir->measure_exclusion[n][1] = N_CMS_ELECTRIC_VENT_DAMPER;
  nir->measure_exclusion[n][2] = N_CMS_IID;
  nir->measure_exclusion[n][3] = N_CMS_ELECTRIC_VENT_DAMPER_AND_IID;
  nir->measure_exclusion[n][4] = N_CMS_FLAME_RETENTION_BURNER;
  nir->measure_exclusion[n][5] = N_CMS_FURNACE_TUNE_UP;
  nir->measure_exclusion[n][6] = N_CMS_REPLACE_HEATING_SYSTEM;
  nir->measure_exclusion[n][7] = n;

  n = N_CMS_SMART_THERMOSTAT;
  //STRCPY(nir->measure_name[n], "Smart Thermostat");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_SMART_THERMOSTAT;
  nir->measure_exclusion_count[n] = 1;
  nir->measure_exclusion[n][0] = n;

  n = N_CMS_REPLACE_AC;
  //STRCPY(nir->measure_name[n], "Replace Air Conditioner");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_COOLING_SYSTEM;
  nir->measure_exclusion_count[n] = 4;
  nir->measure_exclusion[n][0] = n;
  nir->measure_exclusion[n][1] = N_CMS_EVAPORATIVE_COOLER;
  nir->measure_exclusion[n][2] = N_CMS_INSTALL_OR_REPLACE_HEATPUMP;
  nir->measure_exclusion[n][3] = N_CMS_TUNE_UP_AC;

  n = N_CMS_EVAPORATIVE_COOLER;
  //STRCPY(nir->measure_name[n], "Evaporative Cooler");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_COOLING_SYSTEM;
  nir->measure_exclusion_count[n] = 4;
  nir->measure_exclusion[n][0] = N_CMS_REPLACE_AC;
  nir->measure_exclusion[n][1] = n;
  nir->measure_exclusion[n][2] = N_CMS_INSTALL_OR_REPLACE_HEATPUMP;
  nir->measure_exclusion[n][3] = N_CMS_TUNE_UP_AC;

  n = N_CMS_INSTALL_OR_REPLACE_HEATPUMP;
  //STRCPY(nir->measure_name[n], "Install/Replace Heatpump");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_HEAT_PUMP_REPLACE;
  nir->measure_exclusion_count[n] = 4;
  nir->measure_exclusion[n][0] = N_CMS_REPLACE_AC;
  nir->measure_exclusion[n][1] = N_CMS_EVAPORATIVE_COOLER;
  nir->measure_exclusion[n][2] = n;
  nir->measure_exclusion[n][3] = N_CMS_TUNE_UP_AC;

  n = N_CMS_INFILTRATION_REDUCTION;
  STRCPY(nir->measure_name[n], "Infiltration Reduction");     // not in cms array
  nir->meas_type[n] = CMT_INFILTRATION_REDUCTION;
  nir->measure_exclusion_count[n] = 1;
  nir->measure_exclusion[n][0] = n;

  n = N_CMS_DUCT_INSULATION;
  //STRCPY(nir->measure_name[n], "Duct Insulation");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_DUCT;
  nir->measure_exclusion_count[n] = 1;
  nir->measure_exclusion[n][0] = n;

  n = N_CMS_FILL_CEILING_CAVITY;
  //STRCPY(nir->measure_name[n], "Fill Ceiling Cavity");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_HEATING_ENVELOPE;
  nir->measure_exclusion_count[n] = 6;
  nir->measure_exclusion[n][0] = N_CMS_ATTIC_INSULATION_R11;
  nir->measure_exclusion[n][1] = N_CMS_ATTIC_INSULATION_R19;
  nir->measure_exclusion[n][2] = N_CMS_ATTIC_INSULATION_R30;
  nir->measure_exclusion[n][3] = N_CMS_ATTIC_INSULATION_R38;
  nir->measure_exclusion[n][4] = n;
  nir->measure_exclusion[n][5] = N_CMS_ATTIC_INSULATION_R49;

  n = N_CMS_LIGHTING_RETROFITS;
  //STRCPY(nir->measure_name[n], "Lighting Retrofits");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_BASELOAD;
  nir->measure_exclusion_count[n] = 1;
  nir->measure_exclusion[n][0] = n;

  n = N_CMS_TUNE_UP_AC;
  //STRCPY(nir->measure_name[n], "Air Conditioner Tuneup");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_COOLING_SYSTEM;
  nir->measure_exclusion_count[n] = 4;
  nir->measure_exclusion[n][0] = N_CMS_REPLACE_AC;
  nir->measure_exclusion[n][1] = N_CMS_EVAPORATIVE_COOLER;
  nir->measure_exclusion[n][2] = N_CMS_INSTALL_OR_REPLACE_HEATPUMP;
  nir->measure_exclusion[n][3] = n;

  n = N_CMS_WINDOW_SEALING;
  //STRCPY(nir->measure_name[n], "Window Sealing");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_WINDOWS_DOORS;
  nir->measure_exclusion_count[n] = 4;
  nir->measure_exclusion[n][0] = N_CMS_STORM_WINDOWS;
  nir->measure_exclusion[n][1] = N_CMS_LOW_E_WINDOWS;
  nir->measure_exclusion[n][2] = n;
  nir->measure_exclusion[n][3] = N_CMS_WINDOW_REPLACEMENT;

  n = N_CMS_WINDOW_REPLACEMENT;
  //STRCPY(nir->measure_name[n], "Window Replacement");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_WINDOWS_DOORS;
  nir->measure_exclusion_count[n] = 4;
  nir->measure_exclusion[n][0] = N_CMS_STORM_WINDOWS;
  nir->measure_exclusion[n][1] = N_CMS_LOW_E_WINDOWS;
  nir->measure_exclusion[n][2] = N_CMS_WINDOW_SEALING;
  nir->measure_exclusion[n][3] = n;

  n = N_CMS_WATER_HEATER_TANK_INSULATION;
  //STRCPY(nir->measure_name[n], "Water Heater Tank Insulation");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_WATER_HEATER;
  nir->measure_exclusion_count[n] = 2;
  nir->measure_exclusion[n][0] = n;
  nir->measure_exclusion[n][1] = N_CMS_WATER_HEATER_REPLACEMENT;

  n = N_CMS_WATER_HEATER_PIPE_INSULATION;
  //STRCPY(nir->measure_name[n], "Water Heater Pipe Insulation");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_BASELOAD;
  nir->measure_exclusion_count[n] = 1;
  nir->measure_exclusion[n][0] = n;

  n = N_CMS_LOW_FLOW_SHOWERHEADS;
  //STRCPY(nir->measure_name[n], "Low Flow Showerheads");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_BASELOAD;
  nir->measure_exclusion_count[n] = 1;
  nir->measure_exclusion[n][0] = n;

  n = N_CMS_REFRIGERATOR_REPLACEMENT;
  //STRCPY(nir->measure_name[n], "Refrigerator Replacement");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_BASELOAD;
  nir->measure_exclusion_count[n] = 1;
  nir->measure_exclusion[n][0] = n;

  n = N_CMS_DUCT_SEALING;
  STRCPY(nir->measure_name[n], "Seal Ducts");           // not in cms array
  nir->meas_type[n] = CMT_INFILTRATION_REDUCTION;
  nir->measure_exclusion_count[n] = 1;
  nir->measure_exclusion[n][0] = n;

  n = N_CMS_WATER_HEATER_REPLACEMENT;
  //STRCPY(nir->measure_name[n], "Water Heater Replacement");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_WATER_HEATER;
  nir->measure_exclusion_count[n] = 2;
  nir->measure_exclusion[n][0] = N_CMS_WATER_HEATER_TANK_INSULATION;
  nir->measure_exclusion[n][1] = n;

  n = N_CMS_HIGH_EFFICIENCY_BOILER;
  //STRCPY(nir->measure_name[n], "High Efficiency Boiler");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_HEATING_SYSTEM_REPLACE;
  nir->measure_exclusion_count[n] = 8;
  nir->measure_exclusion[n][0] = N_CMS_THERMAL_VENT_DAMPER;
  nir->measure_exclusion[n][1] = N_CMS_ELECTRIC_VENT_DAMPER;
  nir->measure_exclusion[n][2] = N_CMS_IID;
  nir->measure_exclusion[n][3] = N_CMS_ELECTRIC_VENT_DAMPER_AND_IID;
  nir->measure_exclusion[n][4] = N_CMS_FLAME_RETENTION_BURNER;
  nir->measure_exclusion[n][5] = N_CMS_FURNACE_TUNE_UP;
  nir->measure_exclusion[n][6] = N_CMS_REPLACE_HEATING_SYSTEM;
  nir->measure_exclusion[n][7] = n;

  n = N_CMS_ATTIC_INSULATION_R49;
  //STRCPY(nir->measure_name[n], "Attic Insulation R-49");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_HEATING_ENVELOPE;
  nir->measure_exclusion_count[n] = 6;
  nir->measure_exclusion[n][0] = N_CMS_ATTIC_INSULATION_R11;
  nir->measure_exclusion[n][1] = N_CMS_ATTIC_INSULATION_R19;
  nir->measure_exclusion[n][2] = N_CMS_ATTIC_INSULATION_R30;
  nir->measure_exclusion[n][3] = N_CMS_ATTIC_INSULATION_R38;
  nir->measure_exclusion[n][4] = N_CMS_FILL_CEILING_CAVITY;
  nir->measure_exclusion[n][5] = n;

  n = N_CMS_DOOR_REPLACEMENT;
  //STRCPY(nir->measure_name[n], "Door Replacement");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_WINDOWS_DOORS;
  nir->measure_exclusion_count[n] = 1;
  nir->measure_exclusion[n][0] = n;

  n = N_CMS_WHITE_ROOF_COATING;
  //STRCPY(nir->measure_name[n], "White Roof Coating");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_COOLING_ENVELOPE;
  nir->measure_exclusion_count[n] = 1;
  nir->measure_exclusion[n][0] = n;

  n = N_CMS_FILL_FLOOR_CAVITY;
  //STRCPY(nir->measure_name[n], "Fill Closed Floor Cavity");
  STRCPY(nir->measure_name[n], ndi->cms[n].measure_name);
  nir->meas_type[n] = CMT_HEATING_ENVELOPE;
  nir->measure_exclusion_count[n] = 7;
  nir->measure_exclusion[n][0] = N_CMS_SILLBOX_INSULATION;
  nir->measure_exclusion[n][1] = N_CMS_FOUNDATION_WALL_INSULATION;
  nir->measure_exclusion[n][2] = N_CMS_FLOOR_INSULATION_R11;
  nir->measure_exclusion[n][3] = N_CMS_FLOOR_INSULATION_R19;
  nir->measure_exclusion[n][4] = N_CMS_FLOOR_INSULATION_R30;
  nir->measure_exclusion[n][5] = N_CMS_FLOOR_INSULATION_R38;
  nir->measure_exclusion[n][6] = n;

  // optionally show our measure interactions
  if (cmds.debug_level & D_MEASURE_EXCLUSION) {
    fprintf(stderr, "%s", "\n");
    fprintf(stderr, "%34s", " ");
    for (int m = 0; m < MAXMEAS; m++)
      fprintf(stderr, "%3d", m);
    fprintf(stderr, "\n");
    for (int cms_measure_num = 0; cms_measure_num < MAXMEAS; cms_measure_num++) {
      fprintf(stderr, "%-30s%5d", nir->measure_name[cms_measure_num], cms_measure_num);
      for (int m = 0; m < MAXMEAS; m++) {
        if (!mutually_exclusive_measures(cms_measure_num, m))
          fprintf(stderr, "   ");
        else
          fprintf(stderr, " X ");
      }
      fprintf(stderr, "\n");
    }
  }

  return;
}

void initialize_billing(void) {
  // logic needs to see -1 if not read from file
  nir->bill_base_load_consumption[PRE_HEATING]  = -1.0;
  nir->bill_base_load_consumption[PRE_COOLING]  = -1.0;
  nir->bill_base_load_consumption[POST_HEATING] = -1.0;   // unused
  nir->bill_base_load_consumption[POST_COOLING] = -1.0;   // unused

  // input units flag -1 missing, 0=therms, 1=kWh
  nir->bill_consumption_units[PRE_HEATING] = BILL_UNITS_MISSING;
  nir->bill_consumption_units[PRE_COOLING] = BILL_UNITS_MISSING;
  nir->bill_consumption_units[POST_HEATING] = BILL_UNITS_MISSING;   // unused
  nir->bill_consumption_units[POST_COOLING] = BILL_UNITS_MISSING;   // unused

  // utility billing adjustment factors
  nir->bladj[PRE_HEATING]  = 1.0;      //[0] = pre retorfit heating
  nir->bladj[PRE_COOLING]  = 1.0;      //[1] = pre retrofit cooling
  nir->bladj[POST_HEATING] = 0.0;      //[2] = post retrofit heating (unused)
  nir->bladj[POST_COOLING] = 0.0;      //[3] = post retrofit cooling (unused)

  // default lifetimes for these measures here rather than from RMC input
  // since these two measures are always ON
  ndi->rmc[N_MAT_INFILTRATION_REDUCTION].life = 10;
  ndi->rmc[N_MAT_DUCT_SEALING].life = 10;

  return;
}

void neat_preparation(void) {

  water_heater_factors(&ndi->dwh);

  weather_initialize();

  window_initialize();

  door_initialize();

  foundation_initialize();

  attic_initialize();;

  wall_initialize();

  //  Compute the un-insulated supply duct area from input parameters. MBG 10/08

  nir->ductarea = 0.0f;

  if (ndi->htg[PRIMARY].duct_type1 == DS_ROUND)
    nir->ductarea += PI * ndi->htg[PRIMARY].duct_diameter1 / 12.0f * ndi->htg[PRIMARY].duct_length1;
  else if (ndi->htg[PRIMARY].duct_type1 == DS_RECTANGULAR)
    nir->ductarea += 2.0F * (ndi->htg[PRIMARY].duct_width1 + ndi->htg[PRIMARY].duct_height1) / 12.0f * ndi->htg[PRIMARY].duct_length1;

  if (ndi->htg[PRIMARY].duct_type2 == DS_ROUND)
    nir->ductarea += PI * ndi->htg[PRIMARY].duct_diameter2 / 12.0f * ndi->htg[PRIMARY].duct_length2;
  else if (ndi->htg[PRIMARY].duct_type2 == DS_RECTANGULAR)
    nir->ductarea += 2.0F * (ndi->htg[PRIMARY].duct_width2 + ndi->htg[PRIMARY].duct_height2) / 12.0f * ndi->htg[PRIMARY].duct_length2;

  if (ndi->htg[PRIMARY].duct_type3 == DS_ROUND)
    nir->ductarea += PI * ndi->htg[PRIMARY].duct_diameter3 / 12.0f * ndi->htg[PRIMARY].duct_length3;
  else if (ndi->htg[PRIMARY].duct_type3 == DS_RECTANGULAR)
    nir->ductarea += 2.0F * (ndi->htg[PRIMARY].duct_width3 + ndi->htg[PRIMARY].duct_height3) / 12.0f * ndi->htg[PRIMARY].duct_length3;

  return;
}

static void weather_initialize(void) {
  int ndbt = 0, nrh = 0;
  float dbtmax = 0.0;
  float wbtmax = 0.0;
  float ecsat;

  ndi->ubc.usage_units= KWH;    // #263

  for (int i = 0; i < MAXECMS; i++)
    nir->measure_priority[i] = MPS_SIR; // initialization

  // Compute values of monthly specific infiltration

  for (int m = 1; m <= MONTHS; m++) {

    // see monthly_infiltration() #242  MJF
    // for (int i = 0; i <= BIN_20_24; i++) {
    //   nir->specific_infiltration[m] += specific_infiltration(cwd->fWindSpdBins[m][i], cwd->fTempOutBins[m][i] - ndi->key.daytime_heating_setpoint);
    // }
    // nir->specific_infiltration[m] /= (BIN_20_24 + 1.0f);

    // Determine max DBT and WBT and number of cooling bins Evap Clrs apply

    for (int i = 0; i <= BIN_20_24; i++) {
      if (cwd->fTempOutBins[m][i] > dbtmax) {
        dbtmax = cwd->fTempOutBins[m][i];
        wbtmax = cwd->fTempWetBins[m][i];
      }
      if (cwd->fTempOutBins[m][i] > 78.0f) {
        ndbt++;
        if (relative_humidity(cwd->fTempOutBins[m][i], cwd->fTempWetBins[m][i], cwd->altitude) <= .5)
          nrh++;
      }
    }
  }

  nir->consider_evaporative_cooler = FALSE;
  if (ndbt > 0) {
    if ((float)nrh / (float)ndbt > 0.9)
      nir->consider_evaporative_cooler = TRUE;
  }

  // Determine the evaporative cooler effective EER from the maximum DBT and WBT

  ecsat = dbtmax - 0.75f * (dbtmax - wbtmax);
  if (ecsat < 70.0f)
    nir->evaporative_cooler_eer = 3.1f * (78.0f - ecsat) / 0.75f;
  else
    nir->evaporative_cooler_eer = 33.1f;

  // assign our translated orientation to a value used to index various orientation dependent arrays
  // the solar_orient of windows and doors is assigned from the wall solar_orient a little later
  for (int nc = 0; nc < ndi->num_wal; nc++) {
    ndi->wal[nc].solar_orient = solar_orientation(ndi->wal[nc].orient);
  }

  return;
}

static void window_initialize(void) {

    for (int nc = 0; nc < ndi->num_win; nc++) {
    ndi->win[nc].u_value = window_u_value(ndi->win[nc].frame_type, ndi->win[nc].glazing_type, ASHRAE);
    ndi->win[nc].area_glazing =
        (ndi->win[nc].height - WINDOW_FRAME_HEIGHT) * (ndi->win[nc].width - WINDOW_FRAME_WIDTH) * ndi->win[nc].number / 144.0f;
    ndi->win[nc].area_gross = ndi->win[nc].height * ndi->win[nc].width * ndi->win[nc].number / 144.0f;

    ndi->win[nc].shade_factor_winter = ndi->win[nc].shade_factor_summer = 1.0f - ndi->win[nc].shade / 100.0f;
    nir->wn_sunscrn[HEATING][nc] = nir->wn_sunscrn[COOLING][nc] = 1.0f; // 3/28/11 for proper modeling of sunscreens

    ndi->win[nc].shgc_winter = window_shgc(ndi->win[nc].frame_type, ndi->win[nc].glazing_type);
    ndi->win[nc].shgc_summer = ndi->win[nc].shgc_winter;

    // Determine the window's cfm leakage
    ndi->win[nc].avg_leak_cfm[AVG] = 0.0f;
    ndi->win[nc].leak_coef = window_leakage_coef(ndi->win[nc].leak);
    for (int m = 1; m <= MONTHS; m++) {
      nir->wn_leak_cfm[nc][m] = 0.1f * ndi->win[nc].leak_coef * (float)POWC(cwd->avg_wind_mph[m], 1.6);
      nir->wn_leak_cfm[nc][m] *=
          2.0f * (ndi->win[nc].height + ndi->win[nc].width) / 12.0f * ndi->win[nc].number; // Perimeter (ft)
      ndi->win[nc].avg_leak_cfm[AVG] += nir->wn_leak_cfm[nc][m];
      nir->wn_cfm_tot[m] += nir->wn_leak_cfm[nc][m];
    }
    ndi->win[nc].avg_leak_cfm[AVG] /= 12.0f;

    // Subtract window area from net wall area
    for (int n = 0; n < ndi->num_wal; n++) {
      if ((strcmp(ndi->win[nc].wall, ndi->wal[n].code)) == 0) {
        ndi->win[nc].solar_orient = ndi->wal[n].solar_orient;
        ndi->wal[n].area -= ndi->win[nc].area_gross;
        //fprintf(stderr, "\nnc:%d wall_area:%.4g", nc, ndi->wal[n].area);
        break;
      }
    }
  }

  nir->wn_cfm_tot[AVG] = 0.0f; // Used only for analysis purposes. MBG 1/19/00
  for (int m = 1; m <= MONTHS; m++)
    nir->wn_cfm_tot[AVG] += nir->wn_cfm_tot[m];
  nir->wn_cfm_tot[AVG] /= 12.0f;

  return;
}

static void door_initialize(void) {

  for (int nc = 0; nc < ndi->num_dor; nc++) {
    float doorperim;

    ndi->dor[nc].u_value = door_u_value(ndi->dor[nc].door_type, ndi->dor[nc].condition, ASHRAE);

    //  Estimate door perimeter (ft)
    if (ndi->dor[nc].area < 30.0f) // Single entry door - aspect ratio 2:1
      doorperim = 6.0f * (float)(sqrt(ndi->dor[nc].area / 2.0f)) * ndi->dor[nc].number;
    else // Square double entry door
      doorperim = 4.0f * (float)(sqrt(ndi->dor[nc].area)) * ndi->dor[nc].number;

    // Modify door area to be total area for all doors in description
    // Subtract door area from net wall area

    ndi->dor[nc].area *= ndi->dor[nc].number;
    for (int n = 0; n < ndi->num_wal; n++) {
      if ((strcmp(ndi->dor[nc].wall, ndi->wal[n].code)) == 0) {
        ndi->dor[nc].solar_orient = ndi->wal[n].solar_orient;
        ndi->wal[n].area -= ndi->dor[nc].area;
        //fprintf(stderr, "\nnc:%d wall_area:%.4g", nc, ndi->wal[n].area);
        break;
      }
    }

    // Determine the door's cfm leakage

    ndi->dor[nc].avg_leak_cfm[AVG] = 0.0f;
    ndi->dor[nc].leak_coef = door_leakage_coef(ndi->dor[nc].leakiness);

    for (int m = 1; m <= MONTHS; m++) {
      nir->dr_leak_cfm[nc][m] = 0.1f * ndi->dor[nc].leak_coef * (float)POWC(cwd->avg_wind_mph[m], 1.6);
      nir->dr_leak_cfm[nc][m] *= doorperim;
      ndi->dor[nc].avg_leak_cfm[AVG] += nir->dr_leak_cfm[nc][m];
      nir->dr_cfm_tot[m] += nir->dr_leak_cfm[nc][m];
    }
    ndi->dor[nc].avg_leak_cfm[AVG] /= 12.0f;
  }   // next door

  nir->dr_cfm_tot[AVG] = 0.0f;
  for (int m = 1; m <= MONTHS; m++)
    nir->dr_cfm_tot[AVG] += nir->dr_cfm_tot[m];
  nir->dr_cfm_tot[AVG] /= 12.0f;

  return;
}

static void foundation_initialize(void) {
  float max_area = 0;
  float total_area = 0;

  // Determine which subspace, if any, will have ducts assumed present
  // Use the largest unintentionally heated subspace, else assume ambient
  ndi->fnds.subspace_with_ductwork = NOT_APPLICABLE;
  for (int nc = 0; nc < ndi->num_fnd; nc++) {
    total_area += ndi->fnd[nc].area;
    if (ndi->fnd[nc].space_type == UNINTENTIONALLY_CONDITIONED) {
      if (ndi->fnd[nc].area > max_area) {
        ndi->fnds.subspace_with_ductwork = nc;
        max_area = ndi->fnd[nc].area;
      }
    }
  }
  ASSERT(total_area < (ndi->gnl.floor_area * FLOOR_AREA_COMPARE_FACTOR), sprintf(msg, "The sum of the foundation spaces: %7.3f is more than %d percent of conditioned floor area: %7.3f", total_area, (int) (FLOOR_AREA_COMPARE_FACTOR * 100), ndi->gnl.floor_area));

  //Determine the subspace/foundation floor_u_value from user inputs and space type
  for (int nc = 0; nc < ndi->num_fnd; nc++) {
    float added_r = 1.72f;
    float effective_framing_r = (ndi->fnd[nc].flr_ins_r / EXIST_BATT_INS_R_PER_INCH) * FRAMING_R_PER_IN;
    ndi->fnd[nc].ua_value_basement_sill = foundation_sill_ua_value(nc);   // #232
    switch (ndi->fnd[nc].space_type) {
    case CONDITIONED:
      added_r = 3.17f;    // assume furing and wallboard
    case NON_CONDITIONED:
    case VENTED_NON_CONTITIONED:
    case UNINTENTIONALLY_CONDITIONED:
      ndi->fnd[nc].floor_cavity_r = BASE_FLOOR_R + ndi->fnd[nc].flr_ins_r;
      ndi->fnd[nc].floor_framing_r = BASE_FLOOR_R + effective_framing_r;
      ndi->fnd[nc].floor_u_value = FLOOR_FRAMING_RATIO / ndi->fnd[nc].floor_framing_r + (1.0f - FLOOR_FRAMING_RATIO) / ndi->fnd[nc].floor_cavity_r;

      ASSERT((added_r + ndi->fnd[nc].wall_ins_r), sprintf(msg, "Need non zero foundation wall r value"));
      ndi->fnd[nc].above_grade_wall_u_value = ndi->fnd[nc].below_grade_wall_u_value = 1.0f / (added_r + ndi->fnd[nc].wall_ins_r);
      ndi->fnd[nc].wall_area = ndi->fnd[nc].perim_length * ndi->fnd[nc].wall_height;
      ndi->fnd[nc].below_grade_wall_height = ndi->fnd[nc].wall_height * (1.0f - ndi->fnd[nc].wall_exp / 100.0f);
    break;
    case UNINSULATED_SLAB:
    case INSULATED_SLAB:
      ndi->fnd[nc].floor_u_value = 0.442f;
    break;
    case EXPOSED_FLOOR_CLOSED:
      ndi->fnd[nc].floor_cavity_r = BASE_EXPOSED_FLOOR_CLOSED_R + ndi->fnd[nc].flr_ins_r;
      ndi->fnd[nc].floor_framing_r = BASE_EXPOSED_FLOOR_CLOSED_R + effective_framing_r;
      ndi->fnd[nc].floor_u_value = FLOOR_FRAMING_RATIO / ndi->fnd[nc].floor_framing_r + (1.0f - FLOOR_FRAMING_RATIO) / ndi->fnd[nc].floor_cavity_r;
    break;
    case EXPOSED_FLOOR_UNCLOSED:
      ndi->fnd[nc].floor_cavity_r = BASE_EXPOSED_FLOOR_UNCLOSED_R + ndi->fnd[nc].flr_ins_r;
      ndi->fnd[nc].floor_framing_r = BASE_EXPOSED_FLOOR_UNCLOSED_R + effective_framing_r;
      ndi->fnd[nc].floor_u_value = FLOOR_FRAMING_RATIO / ndi->fnd[nc].floor_framing_r + (1.0f - FLOOR_FRAMING_RATIO) / ndi->fnd[nc].floor_cavity_r;
    break;
    default:
    ASSERT(FALSE, sprintf(msg, "Unknown foundation type: %d",ndi->fnd[nc].space_type));
    }
    ndi->fnd[nc].comp_group_num = ndi->fnd[nc].measure_number;
  }

  // Foundation segments that have the same comp_group_num MUST have the same added insulation types

  for (int nc = 0; nc < ndi->num_fnd; nc++) {
    int comp_group_num = ndi->fnd[nc].comp_group_num;
    int added_floor = ndi->fnd[nc].added_floor;
    int added_sill = ndi->fnd[nc].added_sill;
    int added_found = ndi->fnd[nc].added_found;
    for (int nc2 = nc + 1; nc2 < ndi->num_fnd; nc2++) {
      if (comp_group_num && comp_group_num == ndi->fnd[nc2].comp_group_num){
        // Only compare non zero and not NONE insulation types
        if (added_floor && added_floor != AL_NONE && ndi->fnd[nc2].added_floor && ndi->fnd[nc2].added_floor != AL_NONE) {
          ASSERT(added_floor == ndi->fnd[nc2].added_floor, sprintf(msg, "Floor (%s) and (%s) must have the same added insulation type to be in the same measure group", ndi->fnd[nc].code, ndi->fnd[nc2].code));
        }
        if (added_sill && added_sill != AS_NONE && ndi->fnd[nc2].added_sill && ndi->fnd[nc2].added_sill != AS_NONE) {
          ASSERT(added_sill == ndi->fnd[nc2].added_sill, sprintf(msg, "Sill (%s) and (%s) must have the same added insulation type to be in the same measure group", ndi->fnd[nc].code, ndi->fnd[nc2].code));
        }
        if (added_found && added_found != AF_NONE && ndi->fnd[nc2].added_found && ndi->fnd[nc2].added_found != AF_NONE) {
          ASSERT(added_found == ndi->fnd[nc2].added_found, sprintf(msg, "Foundation Wall (%s) and (%s) must have the same added insulation type to be in the same measure group", ndi->fnd[nc].code, ndi->fnd[nc2].code));
        }
      }
    }
  }

  return;
}

// changed this declaration to  follow the syntax
// for the qsort comparison function to resolve compiler warnings
// MJF 1/31/00

// static int compare_floats(const void *va, const void *vb) {
//   float *a, *b;

//   a = (float *)va;
//   b = (float *)vb;

//   return (*a < *b) ? -1 : (*a > *b);
// }

static void attic_initialize(void) {

  // First unfinished attic spaces accumulated to ndi->uas

  for (int nc = 0; nc < ndi->num_atc; nc++, ndi->num_uas++) {   // NOTE double end statement to increment num_uas
    int next = ndi->num_uas;
    STRCPY(ndi->uas[next].code, ndi->atc[nc].code);
    //ndi->uas[next].attic_type = (enum ATTICSPACETYPE)ndi->atc[nc].attic_type;
    ndi->uas[next].audit_section_id = N_UNFINISHED_ATTIC;
    ndi->uas[next].roof_color = ndi->atc[nc].roof_color;
    ndi->uas[next].joist_sp = ndi->atc[nc].joist_sp;
    ndi->uas[next].area = ndi->atc[nc].area;
    ndi->uas[next].exist_insulation = ndi->atc[nc].exist_insulation;
    ndi->uas[next].ins_depth = ndi->atc[nc].ins_depth;
    ndi->uas[next].added_insulation = ndi->atc[nc].added_insulation;
    ndi->uas[next].added_kneewall = AK_NONE;
    ndi->uas[next].max_depth = ndi->atc[nc].max_depth;
    ndi->uas[next].added_R = ndi->atc[nc].added_R;
    ndi->uas[next].cost = ndi->atc[nc].cost;
    
    ndi->uas[next].measure_number = ndi->atc[nc].measure_number;

    switch (ndi->atc[nc].attic_type) {
    case UA_UNFLOORED:
      ndi->uas[next].attic_type = UAS_UNFLOORED;
      break;
    case UA_FLOORED:
      ndi->uas[next].attic_type = UAS_FLOORED;
      break;
    case UA_CATHEDRAL:
      ndi->uas[next].attic_type = UAS_CATHEDRAL;
      //ASSERT(ndi->uas[next].max_depth > 0.01f, sprintf(msg, "Cathedral ceiling with no depth specified: %s", ndi->atc[nc].code));
      break;
    default:
      ASSERT(FALSE, sprintf(msg, "Unrecognized unfinished attic floor type: %d", ndi->atc[nc].attic_type));
    }

    if (ndi->uas[next].added_R > 0.01f)
      ndi->uas[next].comp_group_type = MG_ATTIC_SPECIFIED_REQUIRED;
    else if (ndi->uas[next].max_depth > 0.01f)
      ndi->uas[next].comp_group_type = MG_ATTIC_RESTRICTED;
    else
      ndi->uas[next].comp_group_type = MG_ATTIC_UNRESTRICTED;
  }

  // Next we aggregate the finished attics to our ndi->uas arrauy
  for (int nc = 0; nc < ndi->num_fat; nc++, ndi->num_uas++) { // NOTE double end statement to increment num_uas
    int next = ndi->num_uas;
    STRCPY(ndi->uas[next].code, ndi->fat[nc].code);

    ndi->uas[next].audit_section_id = N_FINISHED_ATTIC;
    ndi->uas[next].joist_sp = 16.0f;                                 // inches
    ndi->uas[next].area = ndi->fat[nc].area;                         // sq ft
    ndi->uas[next].exist_insulation = ndi->fat[nc].exist_insulation; // the same enumeration

    if (ndi->fat[nc].acode == FA_KNEEWALL) {
      ndi->uas[next].added_kneewall = (int)ndi->fat[nc].added_kneewall;
      ndi->uas[next].added_insulation = AA_NONE;
    } else {
      ndi->uas[next].added_kneewall = AK_NONE;
      ndi->uas[next].added_insulation = (int)ndi->fat[nc].added_insulation;
    }
    
    ndi->uas[next].max_depth = ndi->fat[nc].max_depth;    // Maximum Insulation Depth (in)
    ndi->uas[next].added_R = ndi->fat[nc].added_R;        // User specified added insulation R-Value
    ndi->uas[next].ins_depth = ndi->fat[nc].ins_depth;    // Existing Insulation Depth (in)
    ndi->uas[next].cost = ndi->fat[nc].cost;              // Added cost for insulating ($)
    ndi->uas[next].roof_color = ndi->fat[nc].roof_color;

    ndi->uas[next].measure_number = ndi->fat[nc].measure_number;

    switch (ndi->fat[nc].acode) {
    case FA_OUTER_CEILING_JOIST:
      ndi->uas[next].attic_type = UAS_CEILING_JOIST;
      if (ndi->uas[next].added_R > 0.01f)
        ndi->uas[next].comp_group_type = MG_ATTIC_OUTER_CEILING_JOIST_SPECIFIED_REQUIRED;
      else if (ndi->uas[next].max_depth > 0.01f)
        ndi->uas[next].comp_group_type = MG_ATTIC_OUTER_CEILING_JOIST_RESTRICTED;
      else
        ndi->uas[next].comp_group_type = MG_ATTIC_OUTER_CEILING_JOIST_UNRESTRICTED;
      break;
    case FA_COLLAR_BEAM:
      ndi->uas[next].attic_type = UAS_COLLAR_BEAM;
      if (ndi->uas[next].added_R > 0.01f)
        ndi->uas[next].comp_group_type = MG_ATTIC_COLLAR_BEAM_SPECIFIED_REQUIRED;
      else if (ndi->uas[next].max_depth > 0.01f)
        ndi->uas[next].comp_group_type = MG_ATTIC_COLLAR_BEAM_RESTRICTED;
      else
        ndi->uas[next].comp_group_type = MG_ATTIC_COLLAR_BEAM_UNRESTRICTED;
      break;
    case FA_KNEEWALL:
      ndi->uas[next].attic_type = UAS_KNEEWALL;
      ndi->uas[next].comp_group_type = MG_ATTIC_KNEEWALL;
      break;
    case FA_ROOF_RAFTER:
      ndi->uas[next].attic_type = UAS_ROOF_RAFTER;
      if (ndi->uas[next].added_R > 0.01f)
        ndi->uas[next].comp_group_type = MG_ATTIC_ROOF_RAFTER_SPECIFIED_REQUIRED;
      else if (ndi->uas[next].max_depth > 0.01f)
        ndi->uas[next].comp_group_type = MG_ATTIC_ROOF_RAFTER_RESTRICTED;
      else
        ndi->uas[next].comp_group_type = MG_ATTIC_ROOF_RAFTER_UNRESTRICTED;
      break;
    }
  }

  // Based on group_type and user entered measure_number, assign the 
  // component measure grouping number used in the attic insulation analysis later, MJF #258
  // #349 expand the list of component groupings 

  int last_group_num[MG_ATTIC_OUTER_CEILING_JOIST_SPECIFIED_REQUIRED + 1];
  for (int i = 0; i <= MG_ATTIC_OUTER_CEILING_JOIST_SPECIFIED_REQUIRED; i++)
    last_group_num[i] = 1;

  for (int nc = 0; nc < ndi->num_uas; nc++) {
    // only operate on records that have not alreaduy be assigned
    if (ndi->uas[nc].comp_group_num == 0) {
      ndi->uas[nc].comp_group_num = last_group_num[ndi->uas[nc].comp_group_type];
      last_group_num[ndi->uas[nc].comp_group_type]++;
      // look through all attic components finding ones that match on type and number, then assign to group_num
      for (int nc2 = nc; nc2 < ndi->num_uas; nc2++) {
        if (ndi->uas[nc2].comp_group_type == ndi->uas[nc].comp_group_type && 
          ndi->uas[nc2].measure_number == ndi->uas[nc].measure_number) {
          ndi->uas[nc2].comp_group_num = ndi->uas[nc].comp_group_num;
        }
      }
    }
  }

 // Determine the roof/attic parameters for all our ndi->uas array elements

  for (int nc = 0; nc < ndi->num_uas; nc++) {
    float added_r = BASE_CEILING_R;
    // if (ndi->uas[nc].attic_type == UAS_KNEEWALL)
    if (ndi->uas[nc].comp_group_type == MG_ATTIC_KNEEWALL)
      added_r = 1.81f;
    if (ndi->uas[nc].exist_insulation == AE_NONE)
      ndi->uas[nc].ins_depth = 0.0f;

    if (ndi->uas[nc].roof_color == RC_LIGHT)
      ndi->uas[nc].roof_absorptance = WHITE_ROOF_ABSORPTIVITY;
    else
      ndi->uas[nc].roof_absorptance = ROOF_ABSORPTIVITY; // Added 1/11 MBG

    ASSERT((attic_ins_exist_rpi(nc) * ndi->uas[nc].ins_depth + added_r), sprintf(msg, "Need non zero r per inch"));

    ndi->uas[nc].u_value = 1.0f / (attic_ins_exist_rpi(nc) * ndi->uas[nc].ins_depth + added_r);

    ndi->uas[nc].solar_orient = SOLAR_HORIZONTAL_TOTAL;
    ndi->uas[nc].frame_u_value = 0.443f;

    ndi->uas[nc].r_value = attic_ins_exist_rpi(nc) * ndi->uas[nc].ins_depth;
    //  Above added 5/08 for sizing.  MBG
    if (ndi->uas[nc].max_depth < 1.e-6)
      ndi->uas[nc].max_depth = 200.;
    else
      ndi->uas[nc].max_depth -= ndi->uas[nc].ins_depth;
  }

  //  Eliminate attic segments that are not to receive additional insulation

  float total_area = 0;
  for (int nc = 0; nc < ndi->num_uas; nc++) {
    if (ndi->uas[nc].attic_type != UAS_KNEEWALL) 
      total_area += ndi->uas[nc].area;
    if (ndi->uas[nc].attic_type == UAS_KNEEWALL && ndi->uas[nc].added_kneewall == AK_NONE)
      ndi->uas[nc].comp_group_num = 0;

    if (ndi->uas[nc].attic_type != UAS_KNEEWALL && ndi->uas[nc].added_insulation == AA_NONE)
      ndi->uas[nc].comp_group_num = 0;
  }
  ASSERT(total_area < (ndi->gnl.floor_area * FLOOR_AREA_COMPARE_FACTOR), sprintf(msg, "The sum of the attic spaces: %7.3f is more than %d percent of conditioned floor area: %7.3f", total_area, (int) (FLOOR_AREA_COMPARE_FACTOR * 100), ndi->gnl.floor_area));

  // Attic segments that have the same comp_group_num MUST have the same added insulation

  for (int nc = 0; nc < ndi->num_uas; nc++) {
    if (ndi->uas[nc].attic_type == UAS_KNEEWALL) {
      int comp_group_num = ndi->uas[nc].comp_group_num;
      int added = ndi->uas[nc].added_kneewall;
      for (int nc2 = nc + 1; nc2 < ndi->num_uas; nc2++) {
        if (ndi->uas[nc2].attic_type == UAS_KNEEWALL && 
          added && added != AK_NONE && 
          ndi->uas[nc2].added_kneewall && ndi->uas[nc2].added_kneewall != AK_NONE &&
          comp_group_num && comp_group_num == ndi->uas[nc2].comp_group_num)
          ASSERT(added == ndi->uas[nc2].added_kneewall, sprintf(msg, "Kneewalls (%s) and (%s) must have the same added insulation type to be in the same measure group", ndi->uas[nc].code, ndi->uas[nc2].code));
      }
    } else {
      int comp_group_num = ndi->uas[nc].comp_group_num;
      int added = ndi->uas[nc].added_insulation;
      int group_type = ndi->uas[nc].comp_group_type;
      for (int nc2 = nc + 1; nc2 < ndi->num_uas; nc2++) {
        if (ndi->uas[nc2].attic_type != UAS_KNEEWALL && 
          group_type == ndi->uas[nc2].comp_group_type &&
          added && added != AA_NONE && 
          ndi->uas[nc2].added_insulation && ndi->uas[nc2].added_insulation != AA_NONE &&
          comp_group_num && 
          comp_group_num == ndi->uas[nc2].comp_group_num)
          ASSERT(added == ndi->uas[nc2].added_insulation, sprintf(msg, "Attics (%s) and (%s) must have the same added insulation type to be in the same measure group", ndi->uas[nc].code, ndi->uas[nc2].code));
      }
    }
  }

  return;
}

static void wall_initialize(void) {

  // after subtracting door and window area at this point

  for (int nc = 0; nc < ndi->num_wal; nc++) {
    ASSERT(ndi->wal[nc].area > 1.0,
           sprintf(msg, "Net area:%.4g (after subtracting doors and windows) for wall:%d must be bigger than 1 sqft",
                   ndi->wal[nc].area, nc));
  }

  // exclude walls that will not receive any treatment for various reasons.
  // mark them as NA with a zero measure_number

  for (int nc = 0; nc < ndi->num_wal; nc++) {
    int add_blown_insulation;

    if (strcmp(ndi->ins_wall[ndi->wal[nc].added_insulation].units, "R/in") == 0)
      add_blown_insulation = TRUE;
    else
      add_blown_insulation = FALSE;

    // Wall declared uninsulatable
    if (ndi->wal[nc].added_insulation == AW_NONE) {
      ndi->wal[nc].measure_number = 0;
      continue;
    }

    // Wall already has blown insulation, impractical to add more
    if ((ndi->wal[nc].exist_insulation == EW_CELLULOSE_BLOWN || ndi->wal[nc].exist_insulation == EW_FIBERGLASS_BLOWN) && add_blown_insulation) {
      ndi->wal[nc].measure_number = 0;
      continue;
    }

    // Can not blow insulation into these wall types
    if ((ndi->wal[nc].wall_type == LO_MASONRY || ndi->wal[nc].wall_type == LO_CONCRETE ||
        ndi->wal[nc].wall_type == LO_ADOBE) &&  add_blown_insulation) {
      ndi->wal[nc].measure_number = 0;
      continue;
    }

    //  loose fill requested but air space in cavity less than 1.5 inches
    if (add_blown_insulation) {
      float fDepthExistIns = ndi->wal[nc].exist_r / EXIST_BATT_INS_R_PER_INCH;
      if (wall_stud_depth(nc) - fDepthExistIns < 1.5f && ndi->wal[nc].exist_insulation != EW_POLYSTYRENE_BOARD) {
        ndi->wal[nc].measure_number = 0;
        continue;
      }
    }
  }

  // Determine wall paramters needed in neat_energy_use() from user inputs.
  // The following adds 0.68 R for interior films, but does not include an
  // exterior film. Such will be added in the loads module.

  for (int nc = 0; nc < ndi->num_wal; nc++) {
    float rwlcav;
    float rwlfrm;

    if (ndi->wal[nc].wall_type == LO_BALLOON || ndi->wal[nc].wall_type == LO_PLATFORM) { /* frame type wall */
      if (ndi->wal[nc].exist_insulation == EW_NONE || ndi->wal[nc].exist_insulation == EW_FIBERGLASS_BATT ||
          ndi->wal[nc].exist_insulation == EW_ROCKWOOL) { // No or Batt insulation with possible air space
        float fDepthExistIns;
        fDepthExistIns = ndi->wal[nc].exist_r / EXIST_BATT_INS_R_PER_INCH;
        ndi->wal[nc].wall_air_space_thickness = wall_stud_depth(nc) - fDepthExistIns;
        if (ndi->wal[nc].wall_air_space_thickness < 0.0f)
          ndi->wal[nc].wall_air_space_thickness = 0.0f;
      } else {
        ndi->wal[nc].wall_air_space_thickness = 0.0f;
      }
      rwlcav = 2.45f;
      if (ndi->wal[nc].wall_air_space_thickness > 0.0f)
        rwlcav += get_air_space_r_value(ndi->wal[nc].wall_air_space_thickness);
      rwlfrm = 2.45f + FRAMING_R_PER_IN * wall_stud_depth(nc);

    } else if (ndi->wal[nc].wall_type == LO_MASONRY) // 6/6/08
    {
      rwlcav = 4.74f;
      rwlfrm = 3.67f;
    } else if (ndi->wal[nc].wall_type == LO_CONCRETE) {
      rwlcav = 4.77f;
      rwlfrm = 4.07f;
    } else if (ndi->wal[nc].wall_type == LO_ADOBE) {
      rwlcav = 4.98f;
      rwlfrm = 4.98f;
    } else /* other */
    {
      rwlcav = rwlfrm = ndi->key.r_value_uninsulated_other_wall;
    }

    if (ndi->wal[nc].wall_type != LO_MASONRY && ndi->wal[nc].wall_type != LO_ADOBE) {
      if (ndi->wal[nc].ext_type == EX_WOOD || ndi->wal[nc].ext_type == EX_BRICK) {
        rwlcav += 0.8f;
        rwlfrm += 0.8f;
      }

      if (ndi->wal[nc].ext_type == EX_STUCCO) {
        rwlcav += 0.2f;
        rwlfrm += 0.2f;
      }

      if (ndi->wal[nc].ext_type == EX_METAL || ndi->wal[nc].ext_type == EX_OTHER) {
        rwlcav += ndi->key.r_value_exterior_siding_other;
        rwlfrm += ndi->key.r_value_exterior_siding_other;
      }
    }
    rwlcav += ndi->wal[nc].exist_r;

    if (ndi->wal[nc].exist_insulation == EW_OTHER) {
      rwlfrm += ndi->wal[nc].exist_r;
    }

    ASSERT(rwlfrm != 0, sprintf(msg, "Need non zero rwlfrm r value"));
    ASSERT(rwlcav != 0, sprintf(msg, "Need non zero rwlcav r value"));

    ndi->wal[nc].u_frame = 1.0f / (rwlfrm + WALL_ADDED_R);

    ndi->wal[nc].u_cavity = 1.0f / (rwlcav + WALL_ADDED_R);

    ndi->wal[nc].comp_group_num = ndi->wal[nc].measure_number;
  }

  // Wall segments that have the same comp_group_num MUST have the same added_insulation

  for (int nc = 0; nc < ndi->num_wal; nc++) {
    int comp_group_num = ndi->wal[nc].comp_group_num;
    int added_insulation = ndi->wal[nc].added_insulation;
    for (int nc2 = nc + 1; nc2 < ndi->num_wal; nc2++) {
      if (added_insulation && added_insulation != AW_NONE && 
        ndi->wal[nc2].added_insulation && ndi->wal[nc2].added_insulation != AW_NONE &&
        comp_group_num && comp_group_num == ndi->wal[nc2].comp_group_num)
        ASSERT(added_insulation == ndi->wal[nc2].added_insulation, sprintf(msg, "Walls (%s) and (%s) must have the same added insulation type to be in the same measure group", ndi->wal[nc].code, ndi->wal[nc2].code));
    }
  }

  return;
}
