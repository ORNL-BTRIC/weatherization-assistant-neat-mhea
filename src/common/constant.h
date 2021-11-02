/***************************************************************************
* MODULE:       const.h            CREATED:     March 19, 1999
*
* AUTHOR:       Mark Fishbaugher
*
* MDESC:        defined constants for various constants used in analysis code
****************************************************************************/
#ifndef _CONST_H
#define _CONST_H

#define BTU_PER_KWH 3412.14f                  // Btu per kWh
#define MMBTU_PER_KWH 0.00341214              // MMBtu per kWh
#define KWH_PER_MMBTU  293.07107              // kWH per MMBtu
#define W_PER_KBTUH  293.07107                // Watts per kBtu/h
#define WH_PER_BTU  0.29307107                // WH per Btu
#define THERM_PER_MMBTU 0.10f                 // Therms per MMBtu
#define BTU_PER_THERM 100000.0f               // Btu per Therm
#define MMBTU_PER_THERM 0.10f                 // MMBtu per Therm
#define BTUH_PER_TON 12000.0f                 // Btu/h per ton of cooling

#define PI 3.141592f                          // limit of 32bit float precision
#define BASE_PRESSURE 50.0f                   // blower door base pressure in Pa

#define RHO 0.075f                            // density of dry air, (lb/cuft)
#define CAIR 0.24f                            // specific heat of dry air, (Btu/lb-F) */
#define RHOCAIR (RHO * CAIR)                  // rho*cp for air

#define MAX_WINDOW_DOOR_PERCENT 50.0f         // Maximum whole house cfm percent for constraining window and door leakage cfm
#define SEALED_WINDOW_LEAKAGE_COEF 0.015f     // Leakage coefficient for a sealed window
#define SEALING_FRACTION 0.7f                 // Fraction of optimal window sealing achieved

#define DWH_TEMP 130                           // Hot water set point temperature

#define MAX_LIGHT_LIFE 20.0                    // #200 maximum reasonable bulb lifetime in years regardless of life hours (Mark T.)

#define STICK_BUILT_ROOF_PITCH 4.0f/12.0f      // Default pitch for attics (stick built)
#define STICK_BUILT_ROOF_TO_ATTIC_AREA_RATIO (float)sqrt(1.0f + STICK_BUILT_ROOF_PITCH * STICK_BUILT_ROOF_PITCH) 

#define MOBILE_ROOF_PITCH 3.0f/12.0f           // Default pitch for attics (manufactured housing)
#define MOBILE_ROOF_TO_ATTIC_AREA_RATIO (float)sqrt(1.0f + MOBILE_ROOF_PITCH * MOBILE_ROOF_PITCH)  

#endif // _CONST_H
