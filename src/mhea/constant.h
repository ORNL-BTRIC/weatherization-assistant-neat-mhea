/***************************************************************************
* MODULE:       const.h            CREATED:     May 4, 2020
*
* AUTHOR:       Mark Fishbaugher
*
* MDESC:        defined constants for various constants used in MHEA
****************************************************************************/
#ifndef _M_CONST_H
#define _M_CONST_H

#define MHEASAVINGSADJ 0.6f     // Savings multiplier per MHEA Field Test Steering Committee recommendations.  MBG 4/08, 7/08.

#define DENSITY_EXIST_FG_INSUL 0.55f        // Density of uncompressed existing fiberglass insulation
#define R_PER_INCH_FG_COMPRESSED 4.2f       // R/inch for compressed (1.5 lb/cuft) fiberglass insulation
#define FRACTION_UNINSULATABLE_WALL 0.15f   // Standard fraction of wall area that can not be insulated

#define QSDUCT50 100.0f // Default Post-sealed duct leakage at 50 PA (CFM)

#endif // _M_CONST_H