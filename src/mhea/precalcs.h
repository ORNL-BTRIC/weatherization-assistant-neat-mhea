/***************************************************************************
* MODULE:       precalcs.h            CREATED:  December 19, 1998
*
* AUTHOR:       Mark Fishbaugher, Fishbaugher and Associates  1998
*
* MDESC:        prototypes for the associated .c module
****************************************************************************/
#ifndef _M_PRE_CALCS_H
#define _M_PRE_CALCS_H

void analysis_initialize(void);
void duct_leakage_MHEA(void);
int duct_efficiency_MHEA(float fElevation, float fW_Dsgn_T, float fS_Dsgn_T, float *Q50, float *effht, float *effcl);

#endif  //_M_PRE_CALCS_H