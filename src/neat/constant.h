/***************************************************************************
* MODULE:       const.h            CREATED:    May 4 2020
*
* AUTHOR:       Mark Fishbaugher
*
* MDESC:        defined constants for various constants used in NEAT
****************************************************************************/
#ifndef _N_CONST_H
#define _N_CONST_H

#define EXIST_BATT_INS_R_PER_INCH 3.5f      // R/in of existing batt insulation 5/08  Replaces old local fBattInsRpin parameter

#define MAX_CEILING_R_VALUE 60.0f           // Maximum R value limit for mandatory measure

#define WINDOW_FRAME_WIDTH 4.0f             // How many inches to reduce gross window width to get glazing area
#define WINDOW_FRAME_HEIGHT 7.0f            // Ditto for height

#define EXTERIOR_FILM_RESISTANCE_R 0.25f
#define INTERIOR_FILM_RESISTANCE_R 0.76f
#define PLYWOOD_R 0.93f
#define SHEATHING_R 0.70f
#define CAVITY_AIR_R 1.0f
#define FLOOR_COVERING_R 1.50f
#define BASE_FLOOR_R INTERIOR_FILM_RESISTANCE_R + PLYWOOD_R + FLOOR_COVERING_R + INTERIOR_FILM_RESISTANCE_R
#define BASE_EXPOSED_FLOOR_CLOSED_R EXTERIOR_FILM_RESISTANCE_R + SHEATHING_R + PLYWOOD_R + FLOOR_COVERING_R + INTERIOR_FILM_RESISTANCE_R
#define BASE_EXPOSED_FLOOR_UNCLOSED_R PLYWOOD_R + FLOOR_COVERING_R + INTERIOR_FILM_RESISTANCE_R

#define FLOOR_AREA_COMPARE_FACTOR 2.0f      // If attic or foundation total area exceeds the conditioned floor area by this factor, then fail

#define FLOOR_FRAMING_RATIO 0.10f           // ratio of floor framing area to overall floor area
#define FLOOR_CFM_PER_SQFT 0.1f             // ventilated foundation space venilation. rate - cfm/sqft floor area #230

#define FRAMING_R_PER_IN 1.25f              // R/in of framing, i.e. joists, studs, ect.
#define WALL_ADDED_R 0.0f                   // Due to multiple wall types, used only added R
#define BASE_CEILING_R 1.97f

#define DEFAULT_CFM_POST_SEALED_DUCT_LEAKAGE_50 100.0f      // Default Post-sealed duct leakage at 50 PA in (CFM)
#define THERMAL_MASS 3.8f                                   // CIRA specific thermal mass 1.9 - Light  3.8 - medium   5.7 Heavy  (Btu/sqft)
#define BUFFERED_TO_EXPOSED_WALL_DT_RATIO 0.66f             // Ratio of Delta T across buffered wall vs exposed wall (unitless)
#define ATTIC_CFM_PER_SQFT 0.3f                             // attic venilation. rate - cfm/sqft floor area
#define KNEEWALL_JOIST_U_VALUE 0.085f                       // Knee wall joist u-value
#define SILL_U_VALUE 0.2941f                                // uninsulated sill u-value= 1/(1.87(wood)+1.32(shthng)+.21(film)

#define WALL_ABSORPTIVITY 0.8f                              // wall absorptivity (unitless)
#define ROOF_ABSORPTIVITY 0.7f                              // roof absorptivity (unitless)
#define WHITE_ROOF_ABSORPTIVITY 0.3f                        // white roof coating absorptivity (unitless)

#define FRAMING_FACTOR_WALL 0.20f                           // ratio of overall wall to framing area (unitless)
#define FRAMING_FACTOR_CEILING 0.15f                        // framing factor for ceiling joists (unitless)

#define WINDOW_MEC_LEAKAGE_COEF 0.011f                      // New window leakage coefficient (MEC)
#define NEW_DOOR_LEAKAGE_COEF 0.012f                        // New door leakage coefficient

#define SEASONAL_TO_SS_RATIO 0.95f                          // ratio of seasonal to steady state furnace efficiency
#define DEFAULT_STANDARD_FURNACE_SS_EFFICIENCY 77.0f        // default heating steady state efficiency
#define DEFAULT_COOLING_SEER 8.5f                           // default cooling seasonal energy efficiency

#define HTG_DUCT_ADJ 1.0f                                   // Adjustment factor for heating duct effic and general overprediction

#define ATTIC_ASPECT_RATIO 2.0f                                             // Ratio of length to width of attic to compute area of gable ends
#define GABLE_U_VALUE 0.308f                                                // U-Value for attic gable ends.  5/28/08 MBG
#define INFILTRATION_ADJ_FACTOR 0.75f                                       // Overall infiltration adjustment factor 7/15/08

#endif // _N_CONST_H
