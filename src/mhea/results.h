/***************************************************************************
* MODULE:       results.h            CREATED:      9/7/01
*
* AUTHOR:       Mark Fishbaugher
*
* MDESC:        Report writer prototype
****************************************************************************/
#ifndef _RESULTS_H
#define _RESULTS_H

// int mhea_results(int iflgBillAdj, float fBaseUse_Htg, float fBaseUse_Clg, float fBaseUse_Elc, float fEndUse_Htg,
//                  float fEndUse_Clg, float fEndUse_Elc, SIZING HtgDsgn[], float fW_Dsgn_T);
void mhea_results(int iflgBillAdj);
void add_mhea_message(char *msg);

#endif
