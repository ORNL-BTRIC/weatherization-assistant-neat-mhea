/***************************************************************************
 * MODULE:       infiltration.h            CREATED:    March 5, 2019
 *
 * AUTHOR:       Mark Fishbaugher
 *
 * MDESC:        external function prototypes for this module
 ****************************************************************************/
#ifndef _N_INFILTRATION_H
#define _N_INFILTRATION_H

void duct_leakage_neat(void);
void duct_efficiency_NEAT(float Q, float supply_pa, float return_pa, float *effht, float *effcl);

#endif /* _N_INFILTRATION_H */
