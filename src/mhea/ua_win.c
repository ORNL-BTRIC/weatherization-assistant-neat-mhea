/*******************  MODULE NAME:  UA_WIN.C  ****************************/
/**       DATE: 3/14/93                                                             **/
/**          BY:    SLF                                                                 **/
/** DESCRIPTION:    Reads values from form data structures and contains     **/
/**                 equations for 5 UA calculations.                                **/
/** REVISIONS:  10/7/93 SLF - Updated comments and calculations         **/
/**                 12/30/93 SLF - Updated calcs for Additions              **/
/**                 11/16/94 SLF - Reordered mir->flgRetrofits.                   **/
/**                 5/8/95 NLW - Corrected interior shade calculation       **/
/*************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wa_engine.h"

/*************************************************************************/
/*******************  MODULE NAME:  UA_WIN    ****************************/
/*************************************************************************/

#define CFCOVER 0.67 // window covering use factor
#define INCHESTOFEET 0.08333

float window_perimeter(M_WIN *win) {
  return 2.0f * ((win->width + win->height) * INCHESTOFEET) * (win->num_n + win->num_s + win->num_e + win->num_w);
}

/*******************  FUNCTION NAME:  UA_Window  *************************/
/**         DATE:  03/93        07/23/93                                **/
/**           BY:    JG         SLF                                     **/
/**  DESCRIPTION:    See Notes by Sheila Hayter.                        **/
/** UA_Window calculates the summer and winter UA value for each        **/
/**     window type (single, double, etc.) passed in.  The UA value is  **/
/**     based on the glazing and the interior window covering used.     **/
/*************************************************************************/
void ua_window(float *fUA_WIN_S, float *fUA_WIN_W, float *fSA_WIN_S_N, float *fSA_WIN_S_S, float *fSA_WIN_S_E, float *fSA_WIN_S_W,
               float *fSA_WIN_W_N, float *fSA_WIN_W_S, float *fSA_WIN_W_E, float *fSA_WIN_W_W, float *fShadingRatioL,
               float *fShadingRatioW2H) {
  float fU_Window;
  float fUA_Window;
  float fSGF = 0;               // Solar Gain Factor
  float fTransmissivity = 0;
  float fSC = 0;                // Shading coefficient
  float fRGlass = 0.0;
  float fUGlazing = 0;
  float fRCover = 0.0;
  float fWidth;
  float fHeight;
  float fAreaWindow;
  float fAreaSWinTotal = 0.0, fAreaSWinAwning = 0.0, fAreaSWinPorch = 0.0;
  float fNumWinTotal, fNumWinN, fNumWinS, fNumWinE, fNumWinW;
  int iSeason, i, j, k;

  M_WIN *workingWIN; // scratch place to copy windows stuctures
  int w_count;       // scratch counter through windows

  for (k = 0; k < 2; k++) // windows + addition windows
  {
    /*****************************
    Get the window data structure.
    *****************************/
    if (k == 0) /* Home Windows */
      w_count = mdi->num_win;

    if (k == 1) /* Addition Windows */
      w_count = mdi->num_awn;

    /***************************
    For every record (window description), calculate the UA value, sum the
    total UA values at the end of each loop, and read the next record.
    **************************/

    for (j = 0; j < w_count; j++) {

      if (k == 0)
        workingWIN = &mdi->win[j]; // MJF 12/98
      if (k == 1)
        workingWIN = (M_WIN *)(&mdi->awn[j]);

      /***************************
      Get the height and width of the current window description.
      **************************/
      fWidth = (float)(workingWIN->width * INCHESTOFEET);
      fHeight = (float)(workingWIN->height * INCHESTOFEET);

      fAreaWindow = fWidth * fHeight;

      /***********************
      Get the number of windows for each orientation
      of the current window description.
      **********************/
      fNumWinN = workingWIN->num_n;
      fNumWinS = workingWIN->num_s;
      fNumWinE = workingWIN->num_e;
      fNumWinW = workingWIN->num_w;
      fNumWinTotal = fNumWinN + fNumWinS + fNumWinE + fNumWinW;

      fTransmissivity = 0.87f;    // Issue #156 (1) default to single which makes sure our *= adjustment factors don't operate on random data
      iSeason = 1; /* Summer */
      for (i = 0; i < 2; i++) {
        /**************************
        Check to see if the window type is "skylight".
        The skylight type window has a different set of
        U-values for the different glazing types.
        ***************************/
        if (workingWIN->window_type == SKYLIGHTMHEA) {
          if (iSeason) // Get the skylight U-value for summer.
          {
            if (workingWIN->glazing_type == GT_SINGLE) {
              fUGlazing = mdi->key.skylight_u_value_1_glazing_summer;
              fTransmissivity = 0.87f;
            }
            if (workingWIN->glazing_type == GT_DBL) {
              fUGlazing = mdi->key.skylight_u_value_2_glazing_summer;
              fTransmissivity = 0.77f;
            }
            if (workingWIN->glazing_type == GT_SINGLEGLASS) {
              fUGlazing = mdi->key.skylight_u_value_1_glass_storm_summer;
              fTransmissivity = 0.77f;
            } else if (workingWIN->glazing_type == GT_DOUBLEGLASS) {
              fUGlazing = mdi->key.skylight_u_value_2_glass_storm_summer;
              fTransmissivity = 0.67f;
            } else if (workingWIN->glazing_type == GT_SINGLEPLASTIC) {
              fUGlazing = mdi->key.skylight_u_value_1_plstc_storm_summer;
              fTransmissivity = 0.77f;
            } else if (workingWIN->glazing_type == GT_DOUBLEPLASTIC) {
              fUGlazing = mdi->key.skylight_u_value_2_plstc_storm_summer;
              fTransmissivity = 0.67f;
            }
          } else // Get the skylight U-value for winter.
          {
            if (workingWIN->glazing_type == GT_SINGLE) {
              fUGlazing = mdi->key.skylight_u_value_1_glazing_winter;
              fTransmissivity = 0.87f;
            } else if (workingWIN->glazing_type == GT_DBL) {
              fUGlazing = mdi->key.skylight_u_value_2_glazing_winter;
              fTransmissivity = 0.77f;
            } else if (workingWIN->glazing_type == GT_SINGLEGLASS) {
              fUGlazing = mdi->key.skylight_u_value_1_glass_storm_winter;
              fTransmissivity = 0.77f;
            } else if (workingWIN->glazing_type == GT_DOUBLEGLASS) {
              fUGlazing = mdi->key.skylight_u_value_2_glass_storm_winter;
              fTransmissivity = 0.67f;
            } else if (workingWIN->glazing_type == GT_SINGLEPLASTIC) {
              fUGlazing = mdi->key.skylight_u_value_1_plstc_storm_winter;
              fTransmissivity = 0.77f;
            } else if (workingWIN->glazing_type == GT_DOUBLEPLASTIC) {
              fUGlazing = mdi->key.skylight_u_value_2_plstc_storm_winter;
              fTransmissivity = 0.67f;
            }
          }
        } else {
          /*****************************
          If the window type is other than skylight, the glazing
          U-value will fall under one of the following if's.
          Get the window glazing U-value.
          *****************************/
          if (iSeason) // Get the summer value.
          {
            if (workingWIN->glazing_type == GT_SINGLE) {
              fUGlazing = mdi->key.window_u_value_1_glazing_summer;
              fTransmissivity = 0.87f;
            } else if (workingWIN->glazing_type == GT_DBL) {
              fUGlazing = mdi->key.window_u_value_2_glazing_summer;
              fTransmissivity = 0.77f;
            } else if (workingWIN->glazing_type == GT_SINGLEGLASS) {
              fUGlazing = mdi->key.window_u_value_1_glass_storm_summer;
              fTransmissivity = 0.87f;
            } else if (workingWIN->glazing_type == GT_DOUBLEGLASS) {
              fUGlazing = mdi->key.window_u_value_2_glass_storm_summer;
              fTransmissivity = 0.77f;
            } else if (workingWIN->glazing_type == GT_SINGLEPLASTIC) {
              fUGlazing = mdi->key.window_u_value_1_plastic_storm_summer;
              fTransmissivity = 0.87f;
            } else if (workingWIN->glazing_type == GT_DOUBLEPLASTIC) {
              fUGlazing = mdi->key.window_u_value_2_plastic_storm_summer;
              fTransmissivity = 0.77f;
            }
          } else // Get the winter value
          {
            if (workingWIN->glazing_type == GT_SINGLE) {
              fUGlazing = mdi->key.window_u_value_1_glazing_winter;
              fTransmissivity = 0.87f;
            } else if (workingWIN->glazing_type == GT_DBL) {
              fUGlazing = mdi->key.window_u_value_2_glazing_winter;
              fTransmissivity = 0.77f;
            } else if (workingWIN->glazing_type == GT_SINGLEGLASS) {
              fUGlazing = mdi->key.window_u_value_1_glass_storm_winter;
              fTransmissivity = 0.77f;
            } else if (workingWIN->glazing_type == GT_DOUBLEGLASS) {
              fUGlazing = mdi->key.window_u_value_2_glass_storm_winter;
              fTransmissivity = 0.67f;
            } else if (workingWIN->glazing_type == GT_SINGLEPLASTIC) {
              fUGlazing = mdi->key.window_u_value_1_plastic_storm_winter;
              fTransmissivity = 0.77f;
            } else if (workingWIN->glazing_type == GT_DOUBLEPLASTIC) {
              fUGlazing = mdi->key.window_u_value_2_plastic_storm_winter;
              fTransmissivity = 0.67f;
            }
          }
        }

        // Issue #156 make sure we don't have zeros
        ASSERT(fTransmissivity, sprintf(msg,"Glazing transmissivity missing"));
        ASSERT(fUGlazing, sprintf(msg, "Glazing U-value missing"));

        /*******************************
        Add effects of shade screens and low-e film
        *******************************/

        if ((k == 0 && mir->flgRetrofits[M_CMS_ADD_SHADE_SCREENS]) || (k == 1 && mir->flgRetrofits[M_CMS_ADD_SHADE_SCREENS_ADD]) ||
            (workingWIN->ext_shading == ES_SUNSCREEN)) {
          if (iSeason) /* Summer */
            fTransmissivity *= (float)(mdi->key.sun_screen_solar_trans_reduction_summer / 100.0);
          else /* Winter */
            fTransmissivity *= (float)(mdi->key.sun_screen_solar_trans_reduction_winter / 100.0);
        } else if (workingWIN->ext_shading == ES_LOWEFILM) {
          fTransmissivity *= 0.68f; // Reduces solar to 68%
          // Effect of low-e film on transmissivity could be added to setup
          fUGlazing = 1.0f / (1.0f / fUGlazing + 0.5f); // Adds 0.5R to window
          // Effect of low-e film on U-value could be added to setup
        }

        /****************************
        Get the window covering R-value.  Same for summer and winter.
        Also get the shading coefficient.
        **************************/

        switch (workingWIN->int_shading) {
          case IS_DRAPESMHEA:
            fRCover = mdi->key.window_shading_r_value_drapes;
            fSC = 0.62f;
            break;
          case IS_BLINDSMHEA:
            fRCover = mdi->key.window_shading_r_value_blinds_shades;
            fSC = 0.56f;
            break;
          case IS_DRAPESSHADESMHEA:
            fRCover = mdi->key.window_shading_r_value_drapes_shades;
            fSC = 0.56f;
            break;
          default:
            fRCover = 0.00f;
            fSC = 1.00f;
            break;
        }

        // ASSERT(fSC, sprintf(msg, "Missing shading coefficient"));

        fRGlass = (float)(1.0 / fUGlazing);

        fU_Window = (float)(1.0 / (fRGlass + (fRCover * CFCOVER)));

        fUA_Window = fU_Window * fAreaWindow * fNumWinTotal;

        fSGF = fTransmissivity * fSC;

        /********************
        Total South Window Area with Awning or Carport/Porch Shading
        *********************/
        if (fNumWinS > 0.0) {
          fAreaSWinTotal += fAreaWindow * fNumWinS;
          if (workingWIN->ext_shading == ES_AWNINGMHEA)
            fAreaSWinAwning += fAreaWindow * fNumWinS;
          else if (workingWIN->ext_shading == ES_CARPORT)
            fAreaSWinPorch += fAreaWindow * fNumWinS;
        }

        /******************
        Solar Appeture Calculations for each orientation in each season
        ******************/
        if (iSeason) /* Summer */
        {
          *fSA_WIN_S_N += fSGF * fAreaWindow * fNumWinN;
          *fSA_WIN_S_S += fSGF * fAreaWindow * fNumWinS;
          *fSA_WIN_S_E += fSGF * fAreaWindow * fNumWinE;
          *fSA_WIN_S_W += fSGF * fAreaWindow * fNumWinW;
        } else /* Winter */
        {
          *fSA_WIN_W_N += fSGF * fAreaWindow * fNumWinN;
          *fSA_WIN_W_S += fSGF * fAreaWindow * fNumWinS;
          *fSA_WIN_W_E += fSGF * fAreaWindow * fNumWinE;
          *fSA_WIN_W_W += fSGF * fAreaWindow * fNumWinW;
        }

        if (iSeason) // The summer UA value is...
          *fUA_WIN_S += fUA_Window;
        else // The winter UA value is...
          *fUA_WIN_W += fUA_Window;

        iSeason = 0; /* Winter */

      } // End for( i = 0; ...) user loop

    } // End for on w_count

    /********************
    Ratios for Overhang Modifier (part of Solar Gain calculation)
    *********************/
    if (fAreaSWinTotal > 0.0) {
      *fShadingRatioL = fAreaSWinAwning / fAreaSWinTotal;
      *fShadingRatioW2H = fAreaSWinPorch / fAreaSWinTotal;
      if (*fShadingRatioL > 1.0)
        *fShadingRatioL = 1.0;
      if (*fShadingRatioW2H > 1.0)
        *fShadingRatioW2H = 1.0;
    } else {
      *fShadingRatioL = 0.0;
      *fShadingRatioW2H = 0.0;
    }
  }

  return;

} // End of ua_window() user function.
