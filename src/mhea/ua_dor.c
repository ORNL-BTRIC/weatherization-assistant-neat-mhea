/*******************  MODULE NAME:  UA_DOR.C  ****************************/
/**       DATE: 3/14/93                                                             **/
/**          BY:    SLF                                                                 **/
/** DESCRIPTION:    Reads values from form data structures and contains     **/
/**                 equations for 5 UA calculations.                                **/
/**   REVISIONS:  12/30/93 SLF - Updated calcs for Additions                **/
/**                 11/16/94 SLF - Reordered mir->flgRetrofits.                   **/
/*************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wa_engine.h"

/*************************************************************************/
/*******************  MODULE NAME:  UA_DOR    ****************************/
/*************************************************************************/

#define SOLARAPPCONSTANT 0.15
#define INCHESTOFEET 0.08333

float door_perimeter(M_DOR *dor) {
  return 2.0f * ((dor->width + dor->height) * INCHESTOFEET) * (dor->num_n + dor->num_s + dor->num_e + dor->num_w);
}

/*******************  FUNCTION NAME:  ua_door  ***************************/
/**         DATE:  03/93        07/26/93                                                **/
/**           BY:    JG         SLF                                                 **/
/**  DESCRIPTION:    See Notes by Sheila Hayter.                                    **/
/*************************************************************************/
void ua_door(float *fUA_DOR_S, float *fUA_DOR_W, float *fSA_DOR_S_N, float *fSA_DOR_S_S, float *fSA_DOR_S_E, float *fSA_DOR_S_W,
             float *fSA_DOR_W_N, float *fSA_DOR_W_S, float *fSA_DOR_W_E, float *fSA_DOR_W_W) {
  float fUADoor;
  float fUDoor, fUDoorTotal;
  float RStormDoor;
  float fADoorWin, fADoorWinN = 0.0, fADoorWinS = 0.0, fADoorWinE = 0.0, fADoorWinW = 0.0;
  float fADoor, fADoorTotal, fADoorN = 0.0, fADoorS = 0.0, fADoorE = 0.0, fADoorW = 0.0;
  int iSeason, i, j, k; /* loop counters */
  M_DOR *workingDOR;
  //int d_count, ndx;
  int d_count;

  if (cmds.debug_level & D_MHEA_ENERGY_DETAIL)
    fprintf(stderr, "----UA OF DOORS----\n");

  for (k = 0; k < 2; k++) // home plus addition
  {
    if (k == 0) /* Home Doors */
    {
      /***************************
      For every record (window description), calculate
      the door window area and read the next.
      **************************/
      for (j = 0; j < mdi->num_win; j++) {
        /********************
        The total door window area for each orientation is calculated.
        *********************/
        if (mdi->win[j].window_type == DOORWINMHEA) {
          fADoorWin = (float)((mdi->win[j].height * INCHESTOFEET) * (mdi->win[j].width * INCHESTOFEET));
          fADoorWinN += fADoorWin * mdi->win[j].num_n;
          fADoorWinS += fADoorWin * mdi->win[j].num_s;
          fADoorWinE += fADoorWin * mdi->win[j].num_e;
          fADoorWinW += fADoorWin * mdi->win[j].num_w;
        }
      }
      d_count = mdi->num_dor;
    }

    if (k == 1) /* Addition Doors */
    {
      /***************************
      For every record (window description), calculate
      the door window area and read the next.
      **************************/
      for (j = 0; j < mdi->num_awn; j++) {
        /********************
        The total door window area for each orientation is calculated.
        *********************/
        if (mdi->awn[j].window_type == DOORWINMHEA) {
          fADoorWin = (float)((mdi->awn[j].height * INCHESTOFEET) * (mdi->awn[j].width * INCHESTOFEET));
          fADoorWinN += fADoorWin * mdi->awn[j].num_n;
          fADoorWinS += fADoorWin * mdi->awn[j].num_s;
          fADoorWinE += fADoorWin * mdi->awn[j].num_e;
          fADoorWinW += fADoorWin * mdi->awn[j].num_w;
        }
      }
      d_count = mdi->num_adr;
    }

    /***************************
    For every record (door description), calculate the UA value,
    summing UA values at the end of each loop, and read the next.
    **************************/

    for (j = 0; j < d_count; j++) {

      if (k == 0)
        workingDOR = &mdi->dor[j]; // MJF 12/98
      if (k == 1)
        //workingDOR = (M_DOR *)(&mdi->adr[j]);
        workingDOR = &mdi->adr[j];

      /*********************
      For the current record (door description), get the door area
      and subtract the average door window area for each orientation.
      (If there is 1 or less doors use the else equation to avoid
      divide by zero errors.)
      **********************/
      fADoor = (float)((workingDOR->width * INCHESTOFEET) * (workingDOR->height * INCHESTOFEET));
      if (workingDOR->num_n > 1)
        fADoorN = (fADoor * workingDOR->num_n) - (fADoorWinN / workingDOR->num_n);
      else
        fADoorN = (fADoor * workingDOR->num_n) - fADoorWinN;
      if (workingDOR->num_s > 1)
        fADoorS = (fADoor * workingDOR->num_s) - (fADoorWinS / workingDOR->num_s);
      else
        fADoorS = (fADoor * workingDOR->num_s) - fADoorWinS;
      if (workingDOR->num_e > 1)
        fADoorE = (fADoor * workingDOR->num_e) - (fADoorWinE / workingDOR->num_e);
      else
        fADoorE = (fADoor * workingDOR->num_e) - fADoorWinE;
      if (workingDOR->num_w > 1)
        fADoorW = (fADoor * workingDOR->num_w) - (fADoorWinW / workingDOR->num_w);
      else
        fADoorW = (fADoor * workingDOR->num_w) - fADoorWinW;
      fADoorTotal = fADoorN + fADoorS + fADoorE + fADoorW;

      /**********************
      Loop through calculations twice, for summer and winter.
      **********************/
      iSeason = 1; /* Summer */
      for (i = 0; i < 2; i++) {
        /*********************
        Check which kind of door has been chosen.  Issue #156 make sure fUDoor is not garbage
        *********************/

        switch (workingDOR->door_type) {
          case SOLIDWOOD:
            fUDoor = mdi->key.door_u_value_wood_with_solid_core;
            break;
          case HOLLOWWOOD:
            fUDoor = mdi->key.door_u_value_wood_with_hollow_core;
            break;
          case METAL:
            fUDoor = mdi->key.door_u_value_standard_mfg_home_door;
            break;
          default:    // INSUL_STEEL || REPLACEMENT)
            fUDoor = mdi->key.u_value_of_replacement_door;
            break;
        }

        // if (workingDOR->door_type == SOLIDWOOD)
        //   fUDoor = mdi->key.door_u_value_wood_with_solid_core;

        // else if (workingDOR->door_type == HOLLOWWOOD)
        //   fUDoor = mdi->key.door_u_value_wood_with_hollow_core;
        // else if (workingDOR->door_type == METAL)
        //   fUDoor = mdi->key.door_u_value_standard_mfg_home_door;

        // else if (workingDOR->door_type == INSUL_STEEL || workingDOR->door_type == REPLACEMENT)
        //   fUDoor = mdi->key.u_value_of_replacement_door;

        if (cmds.debug_level & D_MHEA_ENERGY_DETAIL){
          fprintf(stderr, "\nDoor:%d",j);
          fprintf(stderr, "\nDoor Type: %d", workingDOR->door_type);
          fprintf(stderr, "\nfUDoor: %8.3f", fUDoor);
        }

        /************************
        Calculate the door U value w/ inside & outside air film R values.
        ***********************
        UADoor = fADoor * (1.0 / ( (1.0/UDoorType) + RStormDoor
                             + Rinside + Routside ))
        Rinside = 0.68 for both summer and winter.
        Routside = 0.25 for summer and = 0.17 for winter
        ***********************/

        if (workingDOR->storm == YES)
          RStormDoor = 1.0;
        else
          RStormDoor = 0.0;

        if (iSeason) /* Summer */
          fUDoorTotal = (float)(1.0 / ((1.0 / fUDoor) + RStormDoor + 0.25 + 0.68));
        else /* Winter */
          fUDoorTotal = (float)(1.0 / ((1.0 / fUDoor) + RStormDoor + 0.17 + 0.68));

        fUADoor = fUDoorTotal * fADoorTotal;

        if (cmds.debug_level & D_MHEA_ENERGY_DETAIL){
          fprintf(stderr, "\nj:%d fUDoorTotal: %8.3f", j, fUDoorTotal);
          fprintf(stderr, "\nj:%d fADoorTotal: %8.3f\n", j, fADoorTotal);
        }

        /*************************
        Set the summer and winter U values.
        **************************/
        if (iSeason) {
          *fUA_DOR_S += fUADoor;
          *fSA_DOR_S_N += (float)(fUDoor * SOLARAPPCONSTANT * fADoorN);
          *fSA_DOR_S_S += (float)(fUDoor * SOLARAPPCONSTANT * fADoorS);
          *fSA_DOR_S_E += (float)(fUDoor * SOLARAPPCONSTANT * fADoorE);
          *fSA_DOR_S_W += (float)(fUDoor * SOLARAPPCONSTANT * fADoorW);
        } else {
          *fUA_DOR_W += fUADoor;
          *fSA_DOR_W_N += (float)(fUDoor * SOLARAPPCONSTANT * fADoorN);
          *fSA_DOR_W_S += (float)(fUDoor * SOLARAPPCONSTANT * fADoorS);
          *fSA_DOR_W_E += (float)(fUDoor * SOLARAPPCONSTANT * fADoorE);
          *fSA_DOR_W_W += (float)(fUDoor * SOLARAPPCONSTANT * fADoorW);
        }

        iSeason = 0; /* Winter */

      } /* End of for( i = 0; ...) loop. */

    } // End for on d_count
  }

  return;

} // End of void ua_door( ) user function.
