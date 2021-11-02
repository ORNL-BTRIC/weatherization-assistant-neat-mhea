/*******************  MODULE NAME:  UA_ROF.C  ****************************/
/**       DATE: 3/14/93                                                             **/
/**          BY:    SLF                                                                 **/
/** DESCRIPTION:    Reads values from form data structures and contains     **/
/**                 equations for 5 UA calculations.                                **/
/** REVISIONS:  10/7/93 SLF - Updated comments and calculations         **/
/**                 12/30/93 SLF - Updated calcs for Additions              **/
/**                 11/16/94 SLF - Reordered mir->flgRetrofits.                   **/
/**                 9/8/95   NLW - Major restructure to single roof model   **/
// NW, 9/8/95: Roof calculations are simplified in the version of UA_ROF
//      to reuse flat roof model with variations for bowstring and pitched roofs.
//      Additions section has not been revised.
//      This restructuring was prompted by unexpectedly high roof UAs for bowstring
//      and pitched roofs and unexpectedly high savings for insulation retrofits.
/**               12/19/98 MJF - elimination of GetData -- now used mdi **/
/**                              extensive revisions to all variable    **/
/**                              references                             **/
/*************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wa_engine.h"

// here is the contents of the rvalred.dat file as a simple
// static array in memory -- it only takes 200 bytes so why NOT!
// MJF 10/7/98

// static int rvalred[] = {20,   //      0
//                         26,   //      1
//                         32,   //      2
//                         36,   //      3
//                         38,   //      4
//                         41,   //      5
//                         43,   //      6
//                         45,   //      7
//                         46,   //      8
//                         48,   //      9
//                         49,   //     10
//                         51,   //     11
//                         52,   //     12
//                         53,   //     13
//                         54,   //     14
//                         55,   //     15
//                         57,   //     16
//                         58,   //     17
//                         59,   //     18
//                         59,   //     19
//                         60,   //     20
//                         61,   //     21
//                         62,   //     22
//                         63,   //     23
//                         64,   //     24
//                         65,   //     25
//                         65,   //     26
//                         66,   //     27
//                         67,   //     28
//                         68,   //     29
//                         68,   //     30
//                         69,   //     31
//                         70,   //     32
//                         70,   //     33
//                         71,   //     34
//                         72,   //     35
//                         72,   //     36
//                         73,   //     37
//                         74,   //     38
//                         74,   //     39
//                         75,   //     40
//                         75,   //     41
//                         76,   //     42
//                         76,   //     43
//                         77,   //     44
//                         78,   //     45
//                         78,   //     46
//                         79,   //     47
//                         79,   //     48
//                         80,   //     49
//                         80,   //     50
//                         81,   //     51
//                         81,   //     52
//                         82,   //     53
//                         82,   //     54
//                         83,   //     55
//                         83,   //     56
//                         84,   //     57
//                         84,   //     58
//                         84,   //     59
//                         85,   //     60
//                         85,   //     61
//                         86,   //     62
//                         86,   //     63
//                         87,   //     64
//                         87,   //     65
//                         88,   //     66
//                         88,   //     67
//                         88,   //     68
//                         89,   //     69
//                         89,   //     70
//                         90,   //     71
//                         90,   //     72
//                         90,   //     73
//                         91,   //     74
//                         91,   //     75
//                         92,   //     76
//                         92,   //     77
//                         92,   //     78
//                         93,   //     79
//                         93,   //     80
//                         93,   //     81
//                         94,   //     82
//                         94,   //     83
//                         95,   //     84
//                         95,   //     85
//                         95,   //     86
//                         96,   //     87
//                         96,   //     88
//                         96,   //     89
//                         97,   //     90
//                         97,   //     91
//                         97,   //     92
//                         98,   //     93
//                         98,   //     94
//                         98,   //     95
//                         99,   //     96
//                         99,   //     97
//                         99,   //     98
//                         100,  //     99
//                         100}; //    100

/*************************************************************************/
/*******************  MODULE NAME:  UA_ROF     ***************************/
/*************************************************************************/

#define INCHESTOFEET 0.08333

/*******************  FUNCTION NAME:  air_space_r_value_rof  ******************/
/**         DATE:  03/08/93                                                         **/
/**           BY:    SLF/JLG                                                                **/
/**  DESCRIPTION:    See page 39 in Notes by Sheila Hayter.                 **/
/*************************************************************************/
float air_space_r_value_rof(float fAirSpaceHeight, int iSeason) {
  float fRAirSpace;

  fRAirSpace = 0.0;

  if (fAirSpaceHeight > 0.0) {
    if (iSeason == 1)
      fRAirSpace = (float)(1.0398 * pow(fAirSpaceHeight, 0.14602));
    if (iSeason == 2)
      fRAirSpace = (float)(0.7816 * pow(fAirSpaceHeight, 0.0577));
  }
  return (fRAirSpace);
}

/*******  FUNCTION NAME:  r_value_reduction_from_compression  ************/
/**         DATE:  02/93                                                **/
/**           BY:    SLF                                                **/
/**  DESCRIPTION:    See pages 26 - 38 in Notes by Sheila Hayter.       **/
/**                                                                     **/
/**  CRAZY! Previous version of this function were doing FILEIO to read **/
/**  an array of 100 numbers from an external file. I just declared     **/
/**  a static array containing the data and return a simple array       **/
/**  element                                                            **/
/*************************************************************************/
// int r_value_reduction_from_compression(int iPercentCompressed) {
//   if (iPercentCompressed >= 0 && iPercentCompressed <= 100)
//     return (rvalred[iPercentCompressed]);
//   else
//     return (100);
// }

/*******************  FUNCTION NAME:  ua_roof  ***************************/
/**         DATE:  03/93        08/17/93                                                **/
/**           BY:    SLF            SLF                                                 **/
/**  DESCRIPTION:    See pages 21 - 30 in Notes by Sheila Hayter.           **/
/**                                                                                         **/
/**  9/7/95, NLW  This function has been significantly simplified           **/
/**  to resolve unreasonable savings estimates as written originally        **/
/**                                                                                         **/
/*************************************************************************/
void ua_roof(float fHeatCapacity, float *fVolumeCathCeiling, float *fUA_ROF_S, float *fUA_ROF_W, float *fSA_ROF_S,
             float *fSA_ROF_W) {
  int i; /* Counting variable */

  /*****************************************************************/
  /*  Variable used in Solar Appeture (SA) Calc. for Roof/Ceiling  */
  /*****************************************************************/

  float fColorRoof; /* Color of Roof, used for solar gain,  */
  /*  = (short wave absorp.)/(film coef.) */

  /*******************************************************/
  /*  Variables used in UA Calculation for Roof/Ceiling  */
  /*******************************************************/

  float fLengthHome; /* Length of Home (ft.) */
  float fLengthAddn;
  float fWidthHome, /* Width of Home (ft.) */
      fWidthAddn;
  float fAreaWHC; /* Effective Area of Water Heater Closet */
  /*  (sq. ft.) */
  float fAreaSkylight = 0.0; /* Area of Skylight window(s) (sq. ft.) */

  float fAreaCeilingTotal;   /* Area of Ceiling (sq. ft.) */
  float fAreaCeilingFlat;    /* Area of Flat Ceiling (sq. ft.) */
  float fAreaCeilingCath;    /* Area of Cathedral Ceiling (sq. ft.) */
  float fPercentCeilingCath; /* Percentage of Ceiling that is Cathedral */
  float fWidthCeilingCath;   /* Actual Width of Cathedral Ceiling (ft.) */
  float fPitchRoof;          /* Pitch of Roof (in./in.) */
  float fFactorRoofPitch;    /* Factor for intermediate calculation */
  /*  using fPitchRoof         */
  float fHeightRoofPeak; /* Height of Peak of pitched Roof (in.) */
  float fHeightHeel;     /* Height of roof Heel (in.) */

  float fRTypeInsM = 0.0; /* R-value for Mineral fiber Type of */
  /*  Insulation (per inch)    */
  float fRTypeInsL = 0.0; /* R-value for Loose fill fiberglass */
  /*  Insulation (per inch)    */
  float fRTypeInsC = 0.0; /* R-value for Loose fill cellulose  */
  /*  Insulation (per inch)    */
  float fRTypeInsR = 0.0; /* R-value for Rigid Type of         */
  /*  Insulation (per inch)    */
  float fThicknessInsM = 0.0; /* Thickness of Mineral fiber        */
  /*  Insulation (in.)         */
  float fThicknessInsL = 0.0; /* Thickness of Loose fill fiberglass */
  /*  Insulation (in.)         */
  float fThicknessInsC = 0.0; /* Thickness of Loose fill cellulose */
  /*  Insulation (in.)         */
  float fThicknessInsR = 0.0; /* Thickness of Rigid                */
  /*  Insulation (in.)         */
  float fThicknessInsTotal = 0.0; /* Thickness of Total Existing Insulation (in.) */
  float fThicknessInsAdd = 0.0;   /* User inputted thickness of insulation to add  */
  /*  to a pitched roof (in.)   */
  float fThicknessInsJoist = 0.0; /* Depth of insulation in joist path */
  /*  2/28/08 */
  float fThicknessInsSpace; /* Thickness of Insuatable air Space */
  /*  available (in.)                          */
  int flgRTypeInsNone = 0; /* Flag for No Insulation */
  float fRInsRigid = 0.0;  /* R-value for total Rigid Insulation */
  float fRInsTotal = 0.0;  /* R-value of Total Insulation */
  float fRInsFrame = 0.0;  /* R-value of Insulation above the truss */
  /* in the addition added to the frame */
  float fRInsJoist = 0.0; /* R-value of Insulation above the truss */
  /* in the MH added to joist path 2/28/08 */
  float fRInsEff = 0.0; /* R-value of Effective Insulation */

  float fRInsEffU = 0.0; /* R-value of Effective Insulation in  */
  /*  Unrestricted area of non-flat Roof */

  float fRFraming;       /* R-value of roof Framing */
  float fThicknessTruss; /* Actual Truss Thickness (in.) */
  //    float fFractionCompressed;      /* Fraction that insulation is Compressed */
  //    int iPercentCompressed;         /* Percent that insulation is Compressed */
  //    int iPercentInsRed;             /* Percent Insulation R-value if Reduced */
  /*  by compression                            */
  float fFractionInsRedR = 1.0; /* Fraction Insulation R-value is Reduced */
  /*  by compression in Restricted section  */
  float fFractionInsRedU = 1.0; /* Fraction Insulation R-value is Reduced  */
  /*  by compression in Unrestricted section */
  float fRAirSpaceS = 0.0; /* R-value for roof Air Space in Summer */
  float fRAirSpaceW = 0.0; /* R-value for roof Air Space in Winter */
  float fRInsideCeilingS;  /* Effective R-value for Inside Ceiling */
  /*  in Summer, = avg. value for gypsum  */
  /*  board + inside air                       */
  float fRInsideCeilingW; /* Effective R-value for Inside Ceiling */
  /*  in Winter, = avg. value for gypsum  */
  /*  board + inside air                       */
  float fROutsideRoofS; /* Effective R-value for Outside Roof     */
  /*  in Summer, = avg. value for exterior */
  /*  roofing + outside air                     */
  float fROutsideRoofW; /* Effective R-value for Outside Roof     */
  /*  in Winter, = avg. value for exterior */
  /*  roofing + outside air                     */
  float fROutsideRoofSIns; /* Effective R-value for Outside Roof     */
  /*  in Summer, = avg. value for exterior */
  /*  roofing + outside air                     */
  float fROutsideRoofWIns; /* Effective R-value for Outside Roof     */
  /*  in Winter, = avg. value for exterior */
  /*  roofing + outside air                     */
  float fROutsideRoofSFrm; /* Effective R-value for Outside Roof     */
  /*  in Summer, = avg. value for exterior */
  /*  roofing + outside air                     */
  float fROutsideRoofWFrm; /* Effective R-value for Outside Roof     */
  /*  in Winter, = avg. value for exterior */
  /*  roofing + outside air                     */

  float fRateVentRoof = 0.1f; /* Roof Ventilation Rate (per sq. ft.) */

  float fU_CavityU_S; /* U-value for Unrestricted roof Cavity */
  /*  in Summer                                    */
  float fU_CavityU_W; /* U-value for Unrestricted roof Cavity */
  /*  in Winter                                    */
  float fU_Frame_S; /* U-value for roof Frame in Summer */
  float fU_Frame_W; /* U-value for roof Frame in Winter */
  float fU_C_S;     /* U-value for Ceiling in Summer */
  float fU_C_W;     /* U-value for Ceiling in Winter */
  float fU_R_S;     /* U-value for Roof in Summer */
  float fU_R_W;     /* U-value for Roof in Winter */
  float fU_RC_S;    /* U-value for Roof/Ceiling in Summer */
  float fU_RC_W;    /* U-value for Roof/Ceiling in Winter */
  float fU_RC_Sc;   /* U-value for Roof/Ceiling in Summer */
  /*  for Cathedral ceiling             */
  float fU_RC_Wc; /* U-value for Roof/Ceiling in Winter */
  /*  for Cathedral ceiling             */
  float temp, temp2; /* Temporary values used in calculations
                                                     *  for pitched roofs */
  float Densexist;   /* Density of existing insulation */

  /********************************************************
  R-Value added to compensate for cupboards, furniture, etc.
  *********************************************************/
  float fRAdded_roof = 1.0f;

  /**  Home Length & Width   **/
  fLengthHome = mdi->gnl.length;
  fWidthHome = mdi->gnl.width;

  /**  Outside Water Heater Closet  **/
  if (mdi->gnl.water_heater_closet == YES)
    fAreaWHC = 6.25;
  else
    fAreaWHC = 0.0;

  /***************************/
  /*  Window data structure  */
  /***************************/ /**  Skylight info  **/

  for (i = 0; i < mdi->num_win; i++) {
    if (mdi->win[i].window_type == SKYLIGHTMHEA)
      fAreaSkylight += (float)((mdi->win[i].height * INCHESTOFEET) * (mdi->win[i].width * INCHESTOFEET) *
                               (mdi->win[i].num_n + mdi->win[i].num_s + mdi->win[i].num_e + mdi->win[i].num_w));
  }

  if (cmds.debug_level & D_MHEA_ENERGY_DETAIL)
    fprintf(stderr, "----UA OF ROOF----\n");

  /************************/
  /*  Total Ceiling Area  */
  /************************/
  fAreaCeilingTotal = (fLengthHome * fWidthHome) - fAreaSkylight - fAreaWHC;
  if (mir->flgWhichPass == BASE_CASE)
    mir->fAreaCeilingTotal = fAreaCeilingTotal;

  /****************************/
  /*  Insulation Thicknesses  */
  /****************************/

  if (mdi->rof.mineral_insl > 0.0) {
    fThicknessInsM = mdi->rof.mineral_insl;
  }
  if (mdi->rof.loose_insl > 0.0) {
    fThicknessInsL = mdi->rof.loose_insl;
  }
  if (mdi->rof.rigid_insl > 0.0) {
    fThicknessInsR = mdi->rof.rigid_insl;
  }
  if ((fThicknessInsM + fThicknessInsL + fThicknessInsR) <= 0.0)
    flgRTypeInsNone = 1;
  fThicknessInsC = 0.0f;

  if (mir->flgWhichPass == BASE_CASE) {
    mir->fRofThicknessBattIns = fThicknessInsM;
    mir->fRofThicknessLFGIns = fThicknessInsL;
  }

  /***********************************
  Modify existing insulation depths to account for compression after
  roof insulation measures are installed. For existing insulation density
  use the density of the thickest existing insulation. Compression is
  assumed for all but the pitched roof.  MBG 7/03
  ***********************************/

  if (mdi->rof.roof_type != RT_PITCHED) {
    if (mir->fRofThicknessBattIns > mir->fRofThicknessLFGIns)
      Densexist = mir->fDensExistBatInsul;
    else
      Densexist = DENSITY_EXIST_FG_INSUL;

    if (mir->flgRetrofits[M_CMS_ROOF_CELLULOSE_LOOSE_INSL]) {
      fThicknessInsM = mir->fRofThicknessBattIns * Densexist / mdi->key.density_of_loose_cellulose_insulation;

      fThicknessInsL = mir->fRofThicknessLFGIns * Densexist / mdi->key.density_of_loose_cellulose_insulation;

      fThicknessInsC = mdi->rof.loose_insl - fThicknessInsL;
    }

    else if (mir->flgRetrofits[M_CMS_ROOF_FIBERGLASS_LOOSE_INSL]) {
      fThicknessInsM = mir->fRofThicknessBattIns * Densexist / mdi->key.density_of_loose_fiberglass_insulation;
    }
  } else {
    if (mir->flgRetrofits[M_CMS_ROOF_CELLULOSE_LOOSE_INSL]) {
      fThicknessInsL = mir->fRofThicknessLFGIns;
      fThicknessInsC = mdi->rof.loose_insl - fThicknessInsL;
    }
  }

  fThicknessInsTotal = fThicknessInsM + fThicknessInsL + fThicknessInsC;

  if (mir->flgWhichPass == BASE_CASE) {
    mir->fThicknessCeilIns = fThicknessInsTotal;
  }

  /*****************************/
  /*  Cathedral Ceiling Stuff  */
  /*****************************/

  fPercentCeilingCath = (float)(mdi->rof.cathedral_ceiling / 100.0);    // as factor

  if (mdi->rof.roof_type != RT_BOWSTRING)
  // Allow cathedral ceiling on flat roof. MBG 5/29/03
  {
    fHeightHeel = 2.5f;
    fAreaCeilingFlat = (float)(fAreaCeilingTotal * (1.0 - fPercentCeilingCath));

    //  Compute standard roof pitch based on width

    if (fWidthHome <= 20.0)
      fPitchRoof = (float)(3.0 / 12.0);
    else /* fWidthHome > 20.0  (double wide) */
      fPitchRoof = (float)(2.0 / 12.0);

    fHeightRoofPeak = (float)(0.5 * fWidthHome * fPitchRoof * 12.0);    // inches

    // width of the sloped area of 1/2 of the cathedral ceiling
    fWidthCeilingCath = (float)(sqrt(pow(fHeightRoofPeak / 12.0, 2) + pow((0.5 * fWidthHome), 2)));   // ft
    // Surface area of cathedral ceiling section
    fAreaCeilingCath = (float)(2.0 * fPercentCeilingCath * fLengthHome * fWidthCeilingCath);    // sqft
    *fVolumeCathCeiling = (float)(fAreaCeilingCath * (0.5 * fHeightRoofPeak / 12.0));     // cubic feet
  } else {
    fAreaCeilingFlat = fAreaCeilingTotal;
    fAreaCeilingCath = 0.0;
    *fVolumeCathCeiling = 0.0;  // for bowstring roof cathedral is not distinguished from remainder of roof
  }

  /**  Pitched Roof  **/
  if (mdi->rof.roof_type == RT_PITCHED) {
    //  NW, 9/7/95: If roof is pitched peak height etc. is calculated along
    //  cathedral area and volume.
    // This calculation is in units of inches

    fHeightRoofPeak = (float)((0.5 * fWidthHome * 12.0 * fPitchRoof) + fHeightHeel);

    fThicknessTruss = 3.5;
    //      fRFraming           = 1.23 * (2.0 * fThicknessTruss);
    fRFraming = (float)(1.23 * fThicknessTruss);
    // Eliminate R-value of second rafter (at roof line) since is
    // likely surrounded by air or insulation.  MBG 2/28/08.

    fThicknessInsSpace = (float)((0.5 * (fHeightRoofPeak - fHeightHeel)) + fHeightHeel);

    if (mir->flgWhichPass == BASE_CASE) {

      //  2/21/03 - New variable added to pitched roof screen to allow user
      // to specify the depth of added insulation to be evaluated.  MJF

      fThicknessInsAdd = mdi->rof.pitch_add_insl;

      // Compute the effective depth of added insulation such that when
      // multiplied by the home width gives the volume of added insulation.
      // Uses actual roof geometry assuming standard pitch.  MBG 3/24/03

      ASSERT(fWidthHome != 0, sprintf(msg, "Assertion Failure"));
      ASSERT(fPitchRoof != 0, sprintf(msg, "Assertion Failure"));

      if (fThicknessInsTotal >= fHeightHeel) // Existing insulation above heel
        mir->fRofInsAirSpace = 1.0f / fPitchRoof * fThicknessInsAdd / fWidthHome / 12.0f *
                             (2.0f * (fHeightRoofPeak - fThicknessInsTotal) - fThicknessInsAdd);

      else if ((fThicknessInsTotal + fThicknessInsAdd) <= fHeightHeel)
        // Existing + added still less than heel height
        mir->fRofInsAirSpace = fThicknessInsAdd;

      else { // Existing below heel, but added will take above heel height
        temp = fHeightRoofPeak - fThicknessInsTotal - fThicknessInsAdd;
        temp2 = (fHeightRoofPeak + fHeightHeel - 2.0f * fThicknessInsTotal) / 2.0f;
        mir->fRofInsAirSpace = temp2 - temp * temp / fPitchRoof / 12.0f / fWidthHome;
      }
      // NW, 9/8/95:  insulatable space constrained for pitched roof airspace
      // MBG 2/20/03 Further constrain to maximum of 12 in.

      if (mir->fRofInsAirSpace < 0.0)
        mir->fRofInsAirSpace = 0.0;
      if (mir->fRofInsAirSpace > 12.0)
        mir->fRofInsAirSpace = 12.0;

    } // end if mir->flgWhichPass = BASE_CASE

  } // end of pitched roof section

  /**  Bowstring Roof  **/

  if (mdi->rof.roof_type == RT_BOWSTRING) {
    fHeightHeel = 2.5;

    fHeightRoofPeak = mdi->rof.ceiling_height; // really height of peak from ceiling in inches

    ASSERT(fWidthHome != 0, sprintf(msg, "Assertion Failure"));

    if (fHeightRoofPeak < fHeightHeel)
      fHeightRoofPeak = fHeightHeel;
    fPitchRoof = (float)(fHeightHeel / (0.5 * fWidthHome * 12.0));

    fThicknessTruss = 3.5;
    //      fRFraming           = 1.23 * (2.0 * fThicknessTruss);
    fRFraming = (float)(1.23 * fThicknessTruss);
    // Eliminate R-value of second rafter (at roof line) since is
    // likely surrounded by air or insulation.  MBG 2/28/08.

    fThicknessInsSpace = (float)((0.5 * (fHeightRoofPeak - fHeightHeel)) + fHeightHeel);

    if (mir->flgWhichPass == BASE_CASE) {
      /*** NW, 9/8/95: constrain insulatable airspace for bowstring roof ****/
      mir->fRofInsAirSpace = (float)((fThicknessInsSpace - fThicknessInsTotal) * 1.00);
      if (mir->fRofInsAirSpace < 0.0)
        mir->fRofInsAirSpace = 0.0;
    }

  } // end of bowstring section

  /**  Flat Roof  **/

  if (mdi->rof.roof_type == RT_FLAT) {

    // added joist thickness to the flat roof UA calculation
    // where previously we were assuming 2x6 construction MJF 6/06

    switch (mdi->rof.joist_size) {
    case JS_TWOXFOUR:
      fThicknessTruss = 3.5;
      break;
    case JS_TWOXSIX:
      fThicknessTruss = 5.5;
      break;
    case JS_TWOXEIGHT:
      fThicknessTruss = 7.5;
      break;
    default:
      fThicknessTruss = 5.5;
      break;
    }

    // fThicknessTruss = 5.5;

    fRFraming = (float)(1.23 * fThicknessTruss);

    fThicknessInsSpace = fThicknessTruss;

    if (mir->flgWhichPass == BASE_CASE) {
      mir->fRofInsAirSpace = fThicknessInsSpace - fThicknessInsTotal;
      if (mir->fRofInsAirSpace < 0.0)
        mir->fRofInsAirSpace = 0.0;
    }

  } // end of flat roof section

  /*******************************************************************/
  /* R-value for inside ceiling components (same for all roof types) */
  /*******************************************************************/
  fRInsideCeilingS = mdi->key.interior_ceiling_r_value_summer;
  fRInsideCeilingW = mdi->key.interior_ceiling_r_value_winter;

  /*********************************************************************/
  /* R-value for outside ceiling components (same for all roof types)  */
  /* Value changed to constant 1.4 representing 0.62 for plywood, 0.33 */
  /* for asphalt/shigles, and outside film.  3/08.                     */
  /*********************************************************************/
  //      if( flgRTypeInsNone )
  //          {
  //          fROutsideRoofSIns   = 0.92;
  //          fROutsideRoofWIns = 0.61;
  //          }
  //      else        /* some type(s) of insulation */
  //          {
  //          fROutsideRoofSIns   = 1.13;
  //          fROutsideRoofWIns   = 1.05;
  //          }
  //      fROutsideRoofSFrm   = 1.13;
  //      fROutsideRoofWFrm   = 1.05;

  fROutsideRoofSIns = fROutsideRoofWIns = fROutsideRoofSFrm = fROutsideRoofWFrm = 1.4f;

  /**************************************************
  Beginning of common calculations for each roof type
  ***************************************************/

  /**********************************************************
  Compute the R-value of insulation using a compressed R/inch
  for loose-fill insulation if it fills a confined space.
  Otherwise, use an unconfined R/inch.
  **********************************************************/

  fRTypeInsR = mdi->key.rigid_insulation_r_value_per_inch;
  fRInsRigid = fRTypeInsR * fThicknessInsR;

  /********************************************************
  Used compressed R's/inch for all roof types except pitched
  whenever the roof has been retrofitted by either measure.  MBG 7/03
  *********************************************************/

  if (mdi->rof.roof_type != RT_PITCHED &&
      (mir->flgRetrofits[M_CMS_ROOF_FIBERGLASS_LOOSE_INSL] || mir->flgRetrofits[M_CMS_ROOF_CELLULOSE_LOOSE_INSL])) {
    fRTypeInsM = mir->fRinCompBatInsul;
    fRTypeInsL = mir->fRinFGCompressed;
    fRTypeInsC = mir->fRinCompCelInsul;
  } else {
    fRTypeInsM = mdi->key.batt_blanket_insulation_r_value_per_inch;
    fRTypeInsL = mdi->key.loose_insulation_r_value_per_inch;
    fRTypeInsC = mir->fRinExistCelInsul;
  }

  // Do we account for the R value of any air spaces say greater than a 10th of an inch  (MJF #86)
  if ((fThicknessInsSpace - fThicknessInsTotal) > 0.1f) {
    fRAirSpaceS = air_space_r_value_rof((fThicknessInsSpace - fThicknessInsTotal), 1);
    fRAirSpaceW = air_space_r_value_rof((fThicknessInsSpace - fThicknessInsTotal), 2);
  } else
    fRAirSpaceS = fRAirSpaceW = 0.0;

  //      if( fThicknessInsTotal > fThicknessInsSpace )
  //          {
  //          ASSERT(fThicknessInsTotal != 0);
  //          fFractionCompressed = fThicknessInsSpace / fThicknessInsTotal;
  //          iPercentCompressed  = (int)( fFractionCompressed * 100.0 );
  //          iPercentInsRed          = r_value_reduction_from_compression( iPercentCompressed );
  //          fFractionInsRedR        = ( (float)iPercentInsRed ) / 100.0;
  //          }
  //      else
  fFractionInsRedR = 1.0;

  fRInsTotal = (fRTypeInsM * fThicknessInsM) + (fRTypeInsL * fThicknessInsL) + (fRTypeInsC * fThicknessInsC);

  fRInsEff = fRInsTotal * fFractionInsRedR;

  /***************************************************************
  Determine if any of the insulation is within the joist path.
  Assume it is of type retrofitted, else the type whose thickness
  is greatest. Exclude batt/blanket.
  ***************************************************************/

  fRInsJoist = 0.0f;
  fThicknessInsJoist = fThicknessInsTotal - fThicknessTruss;

  if (mdi->rof.roof_type != RT_FLAT) {
    if (fThicknessInsJoist > 0.0) {

      if (mir->flgRetrofits[M_CMS_ROOF_FIBERGLASS_LOOSE_INSL])
        fRInsJoist = fThicknessInsJoist * fRTypeInsL;

      else if (mir->flgRetrofits[M_CMS_ROOF_CELLULOSE_LOOSE_INSL])
        fRInsJoist = fThicknessInsJoist * fRTypeInsC;

      else if (fThicknessInsL > fThicknessInsC)
        fRInsJoist = fThicknessInsJoist * fRTypeInsL;

      else if (fThicknessInsC > 0.0f)
        fRInsJoist = fThicknessInsJoist * fRTypeInsC;
    } else {

      /*********************************************************
      Part of joist exposed, reducing its effective R-value
      *********************************************************/

      fRFraming = 1.23f * fThicknessInsTotal;
    }
  }

  if (flgRTypeInsNone) {
    /*************************************************************
    Added air space R-value, fRAirSpaceS/W, to paths. MBG 3/18/08
    Remove air space R-value from framing paths.  MBG 9/23/08
    *************************************************************/

    ASSERT((fRInsideCeilingS + fROutsideRoofSIns) != 0, sprintf(msg, "Assertion Failure"));

    fU_RC_S = (float)((0.9 * (1.0 / (fRInsideCeilingS + fRAirSpaceS + fROutsideRoofSIns))) +
                      (0.1 * (1.0 / (fRInsideCeilingS + fRFraming + fROutsideRoofSFrm))));
    fU_RC_W = (float)((0.9 * (1.0 / (fRInsideCeilingW + fRAirSpaceW + fROutsideRoofWIns))) +
                      (0.1 * (1.0 / (fRInsideCeilingW + fRFraming + fROutsideRoofWFrm))));
  } else {
    fU_RC_S = (float)((0.9 * (1.0 / (fRInsideCeilingS + fRInsEff + fRAirSpaceS + fRInsRigid + fROutsideRoofSIns))) +
                      (0.1 * (1.0 / (fRInsideCeilingS + fRFraming + fRInsJoist + fRInsRigid + fROutsideRoofSFrm))));
    fU_RC_W = (float)((0.9 * (1.0 / (fRInsideCeilingW + fRInsEff + fRAirSpaceW + fRInsRigid + fROutsideRoofWIns))) +
                      (0.1 * (1.0 / (fRInsideCeilingW + fRFraming + +fRInsJoist + fRInsRigid + fROutsideRoofWFrm))));
  }

  /*******************
  Modify U to reflect cupboards, etc. by adding fRAdded_roof Rs.
  4/08
  ********************/
  ASSERT(fU_RC_S != 0, sprintf(msg, "Assertion Failure"));
  ASSERT(fU_RC_W != 0, sprintf(msg, "Assertion Failure"));
  fU_RC_S = 1.0f / (1.0f / fU_RC_S + fRAdded_roof);
  fU_RC_W = 1.0f / (1.0f / fU_RC_W + fRAdded_roof);

  *fUA_ROF_S = fU_RC_S * (fAreaCeilingFlat + fAreaCeilingCath);
  *fUA_ROF_W = fU_RC_W * (fAreaCeilingFlat + fAreaCeilingCath);

  /**********************/
  /*  Solar Apperature  */
  /**********************/

  switch(mdi->rof.roof_color) {
    case RC_LIGHT:
      fColorRoof = 0.15f;
      break;
    default:
      fColorRoof = 0.30f;
      break;
  }
  
  // if (mdi->rof.roof_color == RC_LIGHT)
  //   fColorRoof = 0.15f;
  // if (mdi->rof.roof_color == RC_DARK)
  //   fColorRoof = 0.30f;

  *fSA_ROF_S = fColorRoof * (*fUA_ROF_S);
  *fSA_ROF_W = fColorRoof * (*fUA_ROF_W);

  if (cmds.debug_level & D_MHEA_ENERGY_DETAIL) {
    fprintf(stderr, "fRInsideCeilingS = %8.2f\n", fRInsideCeilingS);
    fprintf(stderr, "fRInsEff = %8.2f\n", fRInsEff);
    fprintf(stderr, "fRAirSpaceS = %8.2f\n", fRAirSpaceS);
    fprintf(stderr, "fRInsRigid = %8.2f\n", fRInsRigid);
    fprintf(stderr, "fROutsideRoofSIns = %8.2f\n", fROutsideRoofSIns);
    fprintf(stderr, "fThicknessInsTotal = %8.2f\n", fThicknessInsTotal);
    fprintf(stderr, "fThicknessInsSpace = %8.2f\n", fThicknessInsSpace);
    fprintf(stderr, "fAreaCeilingTotal = %8.2f\n", fAreaCeilingTotal);
    fprintf(stderr, "fAreaCeilingFlat = %8.2f\n", fAreaCeilingFlat);
    fprintf(stderr, "fAreaCeilingCath = %8.2f\n", fAreaCeilingCath);
    fprintf(stderr, "*fVolumeCathCeiling = %8.2f\n", *fVolumeCathCeiling);
    fprintf(stderr, "fThicknessInsSpace = %8.2f\n", fThicknessInsSpace);
    fprintf(stderr, "mir->fRofInsAirSpace = %8.2f\n", mir->fRofInsAirSpace);

    fprintf(stderr, "fFractionInsRedR = %8.2f\n", fFractionInsRedR);
    fprintf(stderr, "fRInsEff = %8.2f\n", fRInsEff);
    fprintf(stderr, "fU_RC_S = %8.2f\n", fU_RC_S);
    fprintf(stderr, "fU_RC_W = %8.2f\n", fU_RC_W);
    fprintf(stderr, "*fUA_ROF_S = %8.2f\n", *fUA_ROF_S);
    fprintf(stderr, "*fUA_ROF_W = %8.2f\n", *fUA_ROF_W);
    fprintf(stderr, "*fSA_ROF_S = %8.2f\n", *fSA_ROF_S);
    fprintf(stderr, "*fSA_ROF_W = %8.2f\n", *fSA_ROF_W);
  }

  /*****************************************************************
  ******************************************************************
  Addition Roof UA, SA  (follows Roof UA calcs for flat & pitched)
  ******************************************************************
  *****************************************************************/

  /**  Home Length & Width   **/
  fLengthAddn = mdi->afl.length;
  fWidthAddn = mdi->afl.width;

  if (fLengthAddn <= 0.0 || fWidthAddn <= 0.0)
    return;

  if (mdi->arf.roof_color == RC_LIGHT)
    fColorRoof = 0.15f;
  if (mdi->arf.roof_color == RC_DARK)
    fColorRoof = 0.30f;

  /************************************/
  /*  Addition Window data structure  */
  /************************************/ /**  Skylight info  **/
  for (i = 0; i < mdi->num_awn; i++) {
    if (mdi->awn[i].window_type == SKYLIGHTMHEA)
      fAreaSkylight += (float)((mdi->awn[i].height * INCHESTOFEET) * (mdi->awn[i].width * INCHESTOFEET) *
                               (mdi->awn[i].num_n + mdi->awn[i].num_s + mdi->awn[i].num_e + mdi->awn[i].num_w));
  }

  /************************/
  /*  Total Ceiling Area  */
  /************************/

  fAreaCeilingTotal = (fLengthAddn * fWidthAddn) - fAreaSkylight;

  // we need to reset these insluation thickness values to zero
  // otherwise the addition gets the same roof insulation values
  // as the rest of the house -- bug fix 4/02 MJF

  fThicknessInsM = 0;
  fThicknessInsL = 0;
  fThicknessInsR = 0;
  fThicknessInsC = 0;

  flgRTypeInsNone = 0; //  Bug fix, 3/18/08

  /****************************/
  /*  Insulation Thicknesses  */
  /****************************/

  if (mdi->arf.mineral_insl > 0.0) {
    fThicknessInsM = mdi->arf.mineral_insl;
  }
  if (mdi->arf.loose_insl > 0.0) {
    fThicknessInsL = mdi->arf.loose_insl;
  }
  if (mdi->arf.rigid_insl > 0.0) {
    fThicknessInsR = mdi->arf.rigid_insl;
  }
  if ((fThicknessInsM + fThicknessInsL + fThicknessInsR) <= 0.0)
    flgRTypeInsNone = 1;

  if (mir->flgWhichPass == BASE_CASE) {
    mir->fRofThicknessBattIns_Add = fThicknessInsM;
    mir->fRofThicknessLFGIns_Add = fThicknessInsL;
  }

  // added joist thickness to the flat roof UA calculation

  switch (mdi->arf.joist_size) {
  case JS_TWOXFOUR:
    fThicknessTruss = 3.5;
    break;
  case JS_TWOXSIX:
    fThicknessTruss = 5.5;
    break;
  case JS_TWOXEIGHT:
    fThicknessTruss = 7.5;
    break;
  default:
    fThicknessTruss = 5.5;
    break;
  }

  /*******************************************************************/
  /* R-value for inside ceiling components (same for all roof types) */
  /*******************************************************************/
  fRInsideCeilingS = mdi->key.interior_ceiling_r_value_summer;
  fRInsideCeilingW = mdi->key.interior_ceiling_r_value_winter;

  fRTypeInsR = mdi->key.rigid_insulation_r_value_per_inch;
  fRInsRigid = fRTypeInsR * fThicknessInsR;

  /**********************/
  /**  Flat Roof       **/
  /**********************/

  if (mdi->awl.wall_config == WC_FLAT) {
    fAreaCeilingFlat = fAreaCeilingTotal;

    /***********************************
    Modify existing insulation depths to account for compression after
    roof insulation measures are installed. For existing insulation density
    use the density of the thickest existing insulation. Compression is
    assumed for all but the pitched roof.  MBG 7/03
    ***********************************/

    if (mir->fRofThicknessBattIns_Add > mir->fRofThicknessLFGIns_Add)
      Densexist = mir->fDensExistBatInsul;
    else
      Densexist = DENSITY_EXIST_FG_INSUL;

    if (mir->flgRetrofits[M_CMS_ROOF_CELLULOSE_LOOSE_INSL_ADD]) {
      fThicknessInsM = mir->fRofThicknessBattIns_Add * Densexist / mdi->key.density_of_loose_cellulose_insulation;

      fThicknessInsL = mir->fRofThicknessLFGIns_Add * Densexist / mdi->key.density_of_loose_cellulose_insulation;

      fThicknessInsC = mdi->arf.loose_insl - fThicknessInsL;
    }

    else if (mir->flgRetrofits[M_CMS_ROOF_FIBERGLASS_LOOSE_INSL_ADD]) {
      fThicknessInsM = mir->fRofThicknessBattIns_Add * Densexist / mdi->key.density_of_loose_fiberglass_insulation;
    }

    fThicknessInsTotal = fThicknessInsM + fThicknessInsL + fThicknessInsC;
    if (mir->flgWhichPass == BASE_CASE) {
      mir->fThicknessCeilIns_Add = fThicknessInsTotal;
    }

    //    fThicknessTruss = 5.5;  Now given as input  3/08

    fRFraming = 1.23f * fThicknessTruss;

    fThicknessInsSpace = fThicknessTruss;

    if (mir->flgWhichPass == BASE_CASE) {
      mir->fARFInsAirSpace = fThicknessInsSpace - fThicknessInsTotal;
      if (mir->fARFInsAirSpace < 0.0)
        mir->fARFInsAirSpace = 0.0;
    }

    /*********************************************************************/
    /* R-value for outside ceiling components                            */
    /* Value changed to constant 1.4 representing 0.62 for plywood, 0.33 */
    /* for asphalt/shigles, and outside film.  3/08.                     */
    /*********************************************************************/

    //      if( flgRTypeInsNone )
    //          {
    //          fROutsideRoofSIns   = 0.92;
    //          fROutsideRoofWIns = 0.61;
    //          }
    //      else        /* some type(s) of insulation */
    //          {
    //          fROutsideRoofSIns   = 1.13;
    //          fROutsideRoofWIns   = 1.05;
    //          }
    //      fROutsideRoofSFrm   = 1.13;
    //      fROutsideRoofWFrm   = 1.05;

    fROutsideRoofSIns = fROutsideRoofWIns = fROutsideRoofSFrm = fROutsideRoofWFrm = 1.4f;

    /********************************************************
    Used compressed R's/inch for flat roofs whenever the roof
    has been retrofitted by either measure. Otherwise, use an
    unconfined R/inch.  MBG 7/03
    *********************************************************/

    if (mir->flgRetrofits[M_CMS_ROOF_FIBERGLASS_LOOSE_INSL_ADD] || mir->flgRetrofits[M_CMS_ROOF_CELLULOSE_LOOSE_INSL_ADD]) {
      fRTypeInsM = mir->fRinCompBatInsul;
      fRTypeInsL = mir->fRinFGCompressed;
      fRTypeInsC = mir->fRinCompCelInsul;
    } else {
      fRTypeInsM = mdi->key.batt_blanket_insulation_r_value_per_inch;
      fRTypeInsL = mdi->key.loose_insulation_r_value_per_inch;
      fRTypeInsC = mir->fRinExistCelInsul;
    }

    if (fThicknessInsTotal < fThicknessInsSpace) {
      fRAirSpaceS = air_space_r_value_rof((fThicknessInsSpace - fThicknessInsTotal), 1);
      fRAirSpaceW = air_space_r_value_rof((fThicknessInsSpace - fThicknessInsTotal), 2);
    } else
      fRAirSpaceS = fRAirSpaceW = 0.0;

    fFractionInsRedR = 1.0;

    fRInsTotal = (fRTypeInsM * fThicknessInsM) + (fRTypeInsL * fThicknessInsL) + (fRTypeInsC * fThicknessInsC);

    fRInsEff = fRInsTotal * fFractionInsRedR;

    if (flgRTypeInsNone) {
      /*************************************************************
      Added air space R-value, fRAirSpaceS/W, to paths. MBG 3/18/08
      Remove air space R-value from framing paths.  MBG 9/23/08
      *************************************************************/

      ASSERT((fRInsideCeilingS + fROutsideRoofSIns) != 0, sprintf(msg, "Assertion Failure"));

      fU_RC_S = (float)((0.9 * (1.0 / (fRInsideCeilingS + fRAirSpaceS + fROutsideRoofSIns))) +
                        (0.1 * (1.0 / (fRInsideCeilingS + fRFraming + fROutsideRoofSFrm))));
      fU_RC_W = (float)((0.9 * (1.0 / (fRInsideCeilingW + fRAirSpaceW + fROutsideRoofWIns))) +
                        (0.1 * (1.0 / (fRInsideCeilingW + fRFraming + fROutsideRoofWFrm))));
    } else {
      fU_RC_S = (float)((0.9 * (1.0 / (fRInsideCeilingS + fRInsEff + fRAirSpaceS + fRInsRigid + fROutsideRoofSIns))) +
                        (0.1 * (1.0 / (fRInsideCeilingS + fRFraming + fRInsJoist + fRInsRigid + fROutsideRoofSFrm))));
      fU_RC_W = (float)((0.9 * (1.0 / (fRInsideCeilingW + fRInsEff + fRAirSpaceW + fRInsRigid + fROutsideRoofWIns))) +
                        (0.1 * (1.0 / (fRInsideCeilingW + fRFraming + fRInsJoist + fRInsRigid + fROutsideRoofWFrm))));
    }

    /*******************
    Modify U to reflect cupboards, etc. by adding fRAdded_roof Rs.
    4/08
    ********************/
    ASSERT(fU_RC_S != 0, sprintf(msg, "Assertion Failure"));
    ASSERT(fU_RC_W != 0, sprintf(msg, "Assertion Failure"));
    fU_RC_S = 1.0f / (1.0f / fU_RC_S + fRAdded_roof);
    fU_RC_W = 1.0f / (1.0f / fU_RC_W + fRAdded_roof);

    *fUA_ROF_S += fU_RC_S * fAreaCeilingFlat;
    *fUA_ROF_W += fU_RC_W * fAreaCeilingFlat;

    /**  Solar Aperture  **/

    *fSA_ROF_S += fColorRoof * (fU_RC_S * fAreaCeilingFlat);
    *fSA_ROF_W += fColorRoof * (fU_RC_W * fAreaCeilingFlat);

  } // end of flat roof section

  /**********************/
  /**  Pitched Roof  **/
  /**********************/

  else {

    if (mir->flgRetrofits[M_CMS_ROOF_CELLULOSE_LOOSE_INSL_ADD]) {
      fThicknessInsL = mir->fRofThicknessLFGIns;
      fThicknessInsC = mdi->arf.loose_insl - fThicknessInsL;
    }

    fThicknessInsTotal = fThicknessInsM + fThicknessInsL + fThicknessInsC;
    if (mir->flgWhichPass == BASE_CASE) {
      mir->fThicknessCeilIns_Add = fThicknessInsTotal;
    }

    fAreaCeilingFlat = 0.0;

    //    fThicknessTruss  = 3.5;  Now given as input  3/08

    //    fRFraming        = 1.23 * (2.0 * fThicknessTruss);
    fRFraming = 1.23f * (fThicknessTruss);
    // Eliminate R-value of second rafter (at roof line) since is
    // likely surrounded by air or insulation.  MBG 2/28/08.

    fHeightHeel = fThicknessTruss;

    fHeightRoofPeak = (float)(12.0 * (mdi->awl.height_max - mdi->awl.height_min));

    if (mdi->awl.wall_config == WC_INTERIOR) {
      fPitchRoof = (mdi->awl.height_max - mdi->awl.height_min) / fWidthAddn;
      fWidthCeilingCath = (float)(sqrt(pow(fHeightRoofPeak / 12.0, 2) + pow(fWidthAddn, 2)));
      fAreaCeilingCath = fLengthAddn * fWidthCeilingCath;
    }

    // By definition the 'length' measurement for the addtion in this case is 
    // parallel to the main home length and the peak of the cathedral addition
    // is perpendicular to the main home. This differs from cathedral ceilings
    // in the main home running parallel to the length of the main home #360

    if (mdi->awl.wall_config == WC_CENTER) {
      fPitchRoof = (float)((mdi->awl.height_max - mdi->awl.height_min) / (0.5 * fLengthAddn));
      fWidthCeilingCath = (float)(sqrt(pow(fHeightRoofPeak / 12.0, 2) + pow((0.5 * fLengthAddn), 2)));
      fAreaCeilingCath = (float)(2.0 * fWidthAddn * fWidthCeilingCath);
    }

    //   fWidthCeilingCath = (float)(sqrt(pow(fHeightRoofPeak / 12.0, 2) + pow((0.5 * fLengthAddn), 2)));  old broken code
    // if (mdi->awl.wall_config == WC_CENTER) {
    //   fPitchRoof = (float)((mdi->awl.height_max - mdi->awl.height_min) / (0.5 * fWidthAddn));
    //   fWidthCeilingCath = (float)(sqrt(pow(fHeightRoofPeak / 12.0, 2) + pow((0.5 * fWidthAddn), 2)));
    //   fAreaCeilingCath = (float)(2.0 * fLengthAddn * fWidthCeilingCath);
    // }
    // fprintf(stderr, "\n\nfLengthAddn = %8.2f\n", fLengthAddn);
    // fprintf(stderr, "fWidthAddn = %8.2f\n", fWidthAddn);
    // fprintf(stderr, "fHeightRoofPeak = %8.2f\n", fHeightRoofPeak);
    // fprintf(stderr, "WidthCeilingCath = %8.2f\n", fWidthCeilingCath);
    // fprintf(stderr, "fAreaCeilingCath = %8.2f\n\n", fAreaCeilingCath);

    *fVolumeCathCeiling += (float)(fAreaCeilingCath * (0.5 * fHeightRoofPeak));

    fROutsideRoofS = 1.13f;
    fROutsideRoofW = 1.05f;

    fThicknessInsSpace = (float)((0.5 * (fHeightRoofPeak - fHeightHeel)) + fHeightHeel);

    if (mir->flgWhichPass == BASE_CASE) {
      //          mir->fARFInsAirSpace  = fHeightRoofPeak - fThicknessInsTotal;
      mir->fARFInsAirSpace = (float)((fHeightRoofPeak - fThicknessInsTotal) / 2.0);
      if (mir->fARFInsAirSpace < 0.0)
        mir->fARFInsAirSpace = 0.0;
    }

    //   Use uncompressed R/inch for pitched roof

    fRTypeInsM = mdi->key.batt_blanket_insulation_r_value_per_inch;
    fRTypeInsL = mdi->key.loose_insulation_r_value_per_inch;
    fRTypeInsC = mir->fRinExistCelInsul;

    if (fThicknessInsTotal < fThicknessInsSpace) {
      fRAirSpaceS = air_space_r_value_rof((fThicknessInsSpace - fThicknessInsTotal), 1);
      fRAirSpaceW = air_space_r_value_rof((fThicknessInsSpace - fThicknessInsTotal), 2);
    }

    //      if( fThicknessInsTotal > fThicknessInsSpace )
    //          {
    //          fFractionCompressed = fThicknessInsSpace / fThicknessInsTotal;
    //          iPercentCompressed  = (int)( fFractionCompressed * 100.0 );
    //          iPercentInsRed          = r_value_reduction_from_compression( iPercentCompressed );
    //          fFractionInsRedU        = ( (float)iPercentInsRed ) / 100.0;
    //          }
    //      else /* fThicknessInsTotal < fThicknessInsSpace */
    fFractionInsRedU = 1.0;

    fRInsTotal = (fRTypeInsM * fThicknessInsM) + (fRTypeInsL * fThicknessInsL) + (fRTypeInsC * fThicknessInsC);

    fRInsEffU = fRInsTotal * fFractionInsRedU;

    /***************************************************************
    Determine if any of the insulation is within the joist path.
    Assume it is of type retrofitted, else the type whose thickness
    is greatest. Exclude batt/blanket.
    ***************************************************************/

    fRInsFrame = 0.0f;
    fThicknessInsJoist = fThicknessInsTotal - fThicknessTruss;
    if (fThicknessInsJoist > 0.0f) {

      if (mir->flgRetrofits[M_CMS_ROOF_FIBERGLASS_LOOSE_INSL_ADD])
        fRInsFrame = fThicknessInsJoist * fRTypeInsL;

      else if (mir->flgRetrofits[M_CMS_ROOF_CELLULOSE_LOOSE_INSL_ADD])
        fRInsFrame = fThicknessInsJoist * fRTypeInsC;

      else if (fThicknessInsL > fThicknessInsC)
        fRInsFrame = fThicknessInsJoist * fRTypeInsL;

      else if (fThicknessInsC > 0.0f)
        fRInsFrame = fThicknessInsJoist * fRTypeInsC;
    }

    else {

      /*********************************************************
      Part of joist exposed, reducing its effective R-value
      *********************************************************/

      fRFraming = 1.23f * fThicknessInsTotal;
    }

    fU_CavityU_S = 1.0f / (fRInsideCeilingS + fRInsEffU + fRAirSpaceS);
    fU_CavityU_W = 1.0f / (fRInsideCeilingW + fRInsEffU + fRAirSpaceW);

    fU_Frame_S = 1.0f / (fRInsideCeilingS + fRFraming + fRInsFrame);
    fU_Frame_W = 1.0f / (fRInsideCeilingW + fRFraming + fRInsFrame);

    fU_C_S = (fU_CavityU_S * 0.90f) + (fU_Frame_S * 0.10f);
    fU_C_W = (fU_CavityU_W * 0.90f) + (fU_Frame_W * 0.10f);

    fU_R_S = 1.0f / (fROutsideRoofS + fRInsRigid);
    fU_R_W = 1.0f / (fROutsideRoofW + fRInsRigid);

    fFactorRoofPitch = (float)(sqrt(1.0 + pow((fPitchRoof), 2)));

    //  Following statements no longer used for pitched Addition roof
    //
    //      fU_RC_S = 1.0 / ( (1.0/fU_C_S) +
    //                          (1.0/( (fU_R_S * fFactorRoofPitch) +
    //                                       (fHeatCapacity * fRateVentRoof) )) );
    //      fU_RC_W = 1.0 / ( (1.0/fU_C_W) +
    //                          (1.0/( (fU_R_W * fFactorRoofPitch) +
    //                                        (fHeatCapacity * fRateVentRoof) )) );

    fU_RC_Sc = (float)(1.0 / (((1.0 / fU_C_S) * (1.0 / fFactorRoofPitch)) +
                              (1.0 / ((fU_R_S * fFactorRoofPitch) + (fHeatCapacity * fRateVentRoof)))));
    fU_RC_Wc = (float)(1.0 / (((1.0 / fU_C_W) * (1.0 / fFactorRoofPitch)) +
                              (1.0 / ((fU_R_W * fFactorRoofPitch) + (fHeatCapacity * fRateVentRoof)))));

    /*******************
    Modify U to reflect cupboards, etc. by adding fRAdded_roof Rs.
    4/08
    ********************/
    ASSERT(fU_RC_Sc != 0, sprintf(msg, "Assertion Failure"));
    ASSERT(fU_RC_Wc != 0, sprintf(msg, "Assertion Failure"));
    fU_RC_Sc = 1.0f / (1.0f / fU_RC_Sc + fRAdded_roof);
    fU_RC_Wc = 1.0f / (1.0f / fU_RC_Wc + fRAdded_roof);

    *fUA_ROF_S += (fU_RC_Sc * fAreaCeilingCath);
    *fUA_ROF_W += (fU_RC_Wc * fAreaCeilingCath);
    *fSA_ROF_S += fColorRoof * (fU_RC_Sc * fAreaCeilingCath);
    *fSA_ROF_W += fColorRoof * (fU_RC_Wc * fAreaCeilingCath);
  }

  if (cmds.debug_level & D_MHEA_ENERGY_DETAIL) {
    // GKA buffer too long for mir->sMsg
    sprintf(mir->sMsg, "Addition parameters \n"
                     "ThicknessInsTotal = %8.2f \n"
                     "fThicknessInsSpace = %8.2f \n"
                     "fAreaCeilingTotal = %8.2f \n"
                     "fAreaCeilingFlat = %8.2f \n"
                     "fAreaCeilingCath = %8.2f \n"
                     "*fVolumeCathCeiling = %8.2f \n"
                     "fThicknessInsSpace = %8.3f \n"
                     "mir->fARFInsAirSpace = %8.3f \n",
            fThicknessInsTotal, fThicknessInsSpace, fAreaCeilingTotal, fAreaCeilingFlat, fAreaCeilingCath, *fVolumeCathCeiling,
            fThicknessInsSpace, mir->fARFInsAirSpace);
    fprintf(stderr, "%s", mir->sMsg);
    sprintf(mir->sMsg, "fFractionInsRedR = %8.3f \n"
                     "fRInsEff = %8.3f \n"
                     "fU_RC_S = %8.3f \n"
                     "fU_RC_W = %8.3f \n"
                     "*fUA_ROF_S = %8.3f \n"
                     "*fUA_ROF_W = %8.3f \n"
                     "*fSA_ROF_S = %8.3f \n"
                     "*fSA_ROF_W = %8.3f \n",
            fFractionInsRedR, fRInsEff, fU_RC_S, fU_RC_W, *fUA_ROF_S, *fUA_ROF_W, *fSA_ROF_S, *fSA_ROF_W);
    fprintf(stderr, "%s", mir->sMsg);
  }

  return;
}
