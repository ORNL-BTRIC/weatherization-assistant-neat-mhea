//******************  MODULE NAME:  UA_WAL.C  ****************************/
/**       DATE: 3/14/93                                                             **/
/**          BY:    SLF                                                                 **/
/** DESCRIPTION:    Reads values from form data structures and contains     **/
/**                 equations for 5 UA calculations.                                **/
/**   REVISIONS:    12/30/93 SLF - Updated calcs for Additions              **/
/**                 11/16/94 SLF - Reordered mir->flgRetrofits.                   **/
/*************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wa_engine.h"

/*************************************************************************/
/*******************  MODULE NAME:  UA_WAL    ****************************/
/*************************************************************************/

#define RFRAME 1.23
//  #define STUDFACTOR          0.15  Modified 2/5/08 MBG
//  #define CAVITYFACTOR_WAL    0.85
#define STUDFACTOR 0.20
#define CAVITYFACTOR_WAL 0.80
#define SOLARAPPCONSTANT 0.15
#define INCHESTOFEET 0.08333

/*******************  FUNCTION NAME:  air_space_r_value_wal  *****************/
/**         DATE:  03/10/93                                                         **/
/**           BY:    JLG                                                                    **/
/**  DESCRIPTION:    See page 39 in Notes by Sheila Hayter.                 **/
/*************************************************************************/
float air_space_r_value_wal(float fAirSpaceDepth) {
  float fRAirSpace;

  // Here is an exponential function that constrains the returned
  // R-value to those found in the ASHRAE handbook of fundamentals
  // but is a monotonically decreasing function of gap width
  // fAirSpaceDepth is assumed to be in units of inches, MJF 2/2003

  if (fAirSpaceDepth <= 0.0)
    fRAirSpace = 0.0;
  else
    fRAirSpace = (float)(1.01 - exp(-4.434 * fAirSpaceDepth));
  return (fRAirSpace);
}

/*******************  FUNCTION NAME:  ua_wall  ***************************/
/**         DATE:  03/93        07/26/93                                                **/
/**           BY:    JG         SLF                                                 **/
/**  DESCRIPTION:    See Notes by Sheila Hayter.                                    **/
/**         NOTE:  Wall Component R-values are temporarily "hard-wired" **/
/**                and will need to be modified for the user to change  **/
/**                them in the "Setup" menu.                            **/
/*************************************************************************/
void ua_wall(float *fVolumeAddition, float *fUA_WAL_S, float *fUA_WAL_W, float *fSA_WAL_S_N, float *fSA_WAL_S_S,
             float *fSA_WAL_S_E, float *fSA_WAL_S_W, float *fSA_WAL_W_N, float *fSA_WAL_W_S, float *fSA_WAL_W_E,
             float *fSA_WAL_W_W, float *fShadingRatioL, float *fShadingRatioW2H) {
  float fUWall = 0.0, fUAWall = 0.0;
  float fAreaWallShort = 0.0, fAreaWallLong = 0.0;
  float fAreaWallCommon, fAreaWallCommonN = 0.0, fAreaWallCommonS = 0.0, fAreaWallCommonE = 0.0, fAreaWallCommonW = 0.0;
  float fAreaWallN = 0.0, fAreaWallS = 0.0, /* Net Area of North/South Walls */
      fAreaWallE = 0.0, fAreaWallW = 0.0;   /* Net Area of East/West Walls */
  float fAreaNetWall = 0.0;                 /* Total Net Area of Walls */
  float fAreaStepWall = 0.0;                /* Area of step wall */
  float fHeightStepWall = 3.0f;             /* Height of step wall */
  //          fAreaGrossWall;                 /* Total Gross Area of Walls */
  float fAddnLength = 0.0, fAddnWidth = 0.0;
  float fRInsulation = 0.0;
  float fCompBatt = 0.0; /* compressed batt thickness */
  float fStudDim = 0.0;
  float fInsulDepth = 0.0, fInsulDepthTotal = 0.0, fRIxInsulDepth = 0.0;
  float fCF;
  //    float   fCFfoam;
  float fRWallFrame, fRWallCavity;
  float fRAirSpace = 0.0;
  float fAirSpaceDepth;

  float fRExterior, fRExtAir, fRInterior, fRIntAir;
  float fRFoamCore = 0.0;

  static float fUnInsulR;    // Uninsulatable insulation R-value of wall
  static float fUnInsulAirR; // UnInsulatable air space R-value
  float fRWallCavityUnins;   // Un-retrofitted cavity R-value
  float fUWallUnins;         // Un-retrofitted wall U-value

  int iflgWallMeas = FALSE; //  wall measure applied

  int iSeason, i;

  /********************************
  Door and window area variables.
  *******************************/
  float fAreaDoor, fAreaDoorT = 0.0, fAreaDoorN = 0.0, fAreaDoorS = 0.0, fAreaDoorE = 0.0, fAreaDoorW = 0.0;
  float fAreaWindow, fAreaWindowT = 0.0, fAreaWindowN = 0.0, fAreaWindowS = 0.0, fAreaWindowE = 0.0, fAreaWindowW = 0.0;

  /********************************************************
  R-Value added to compensate for cupboards, furniture, etc.
  *********************************************************/
  float fRAdded_walls = 1.0f;

  /*****************************************************************
  ******************************************************************
  Addition Wall UA, SA  (follows Wall UA calculation)
  The addition wall calcs are done first so that the addition
  "common" wall area can be subtracted from home wall area
  ******************************************************************
  *****************************************************************/

  /*************
  Get the total addition door area.
  **************/

  if (mdi->num_adr > 0) {

    /***************************
    For every record (addition door description) in the data file,
    calculate the area and read the next.
    **************************/

    for (i = 0; i < mdi->num_adr; i++) {
      /***************************
      For each record read in, there can
      be multiple doors and orientations.
      ***************************/

      fAreaDoor = (float)((mdi->adr[i].height * INCHESTOFEET) * (mdi->adr[i].width * INCHESTOFEET));
      fAreaDoorN += fAreaDoor * mdi->adr[i].num_n;
      fAreaDoorS += fAreaDoor * mdi->adr[i].num_s;
      fAreaDoorE += fAreaDoor * mdi->adr[i].num_e;
      fAreaDoorW += fAreaDoor * mdi->adr[i].num_w;
      fAreaDoorT += fAreaDoorN + fAreaDoorS + fAreaDoorE + fAreaDoorW;
    }
  }

  /*************
  Get the total addition window area.
  **************/

  if (mdi->num_awn > 0) {

    /***************************
    For every record (addition window description) in the data file,
    calculate the area and read the next.
    **************************/

    for (i = 0; i < mdi->num_awn; i++) {
      /***************************
      For each record read in, there can
      be  multiple windows and orientations.
      ***************************/

      if (mdi->awn[i].window_type != DOORWINMHEA && mdi->awn[i].window_type != SKYLIGHTMHEA) {
        fAreaWindow = (float)((mdi->awn[i].height * INCHESTOFEET) * (mdi->awn[i].width * INCHESTOFEET));
        fAreaWindowN += fAreaWindow * mdi->awn[i].num_n;
        fAreaWindowS += fAreaWindow * mdi->awn[i].num_s;
        fAreaWindowE += fAreaWindow * mdi->awn[i].num_e;
        fAreaWindowW += fAreaWindow * mdi->awn[i].num_w;
        fAreaWindowT += fAreaWindowN + fAreaWindowS + fAreaWindowE + fAreaWindowW;
      }
    }
  }

  /******************************
  Get the addition floor data structure.
  ******************************/

  fAddnWidth = mdi->afl.width;
  fAddnLength = mdi->afl.length;

  /******************************
  Get the wall data structure.
  ******************************/

  if (mdi->awl.orientation != 0 && mdi->awl.wall_config != 0) {
    /*********************************
    Calculate the gross wall area.
    **********************************/
    if (mdi->awl.wall_config == WC_INTERIOR) {
      fAreaWallShort = (float)(fAddnWidth * (mdi->awl.height_min + (0.5 * (mdi->awl.height_max - mdi->awl.height_min))));
      fAreaWallLong = fAddnLength * mdi->awl.height_min;
      fAreaWallCommon = fAddnLength * mdi->awl.height_max;
    } else if (mdi->awl.wall_config == WC_CENTER) {
      fAreaWallShort = fAddnWidth * mdi->awl.height_min;
      fAreaWallLong = (float)(fAddnLength * (mdi->awl.height_min + (0.5 * (mdi->awl.height_max - mdi->awl.height_min))));
      fAreaWallCommon = fAreaWallLong;
    } else /* mdi->awl.wall_config == WC_FLAT */
    {
      fAreaWallShort = fAddnWidth * mdi->awl.height_max;
      fAreaWallLong = fAddnLength * mdi->awl.height_max;
      fAreaWallCommon = fAreaWallLong;
    }

    //      fAreaGrossWall = fAreaWallLong + (fAreaWallShort * 2.0);

    /*********************
    Addition Volume (volume for an addition cathedral
    ceiling is accounted for in ua_rof().)
    ********************/
    *fVolumeAddition = fAddnLength * fAddnWidth * mdi->awl.height_min;

    /*********************************
    Calculate the net wall area.
    **********************************/
    if (mdi->awl.orientation == NORTH) {
      fAreaWallN = fAreaWallLong - (fAreaDoorN + fAreaWindowN);
      fAreaWallS = 0.0;
      fAreaWallCommonS = fAreaWallCommon;
      fAreaWallE = fAreaWallShort - (fAreaDoorE + fAreaWindowE);
      fAreaWallW = fAreaWallShort - (fAreaDoorW + fAreaWindowW);
    }
    if (mdi->awl.orientation == SOUTH) {
      fAreaWallN = 0.0;
      fAreaWallCommonN = fAreaWallCommon;
      fAreaWallS = fAreaWallLong - (fAreaDoorS + fAreaWindowS);
      fAreaWallE = fAreaWallShort - (fAreaDoorE + fAreaWindowE);
      fAreaWallW = fAreaWallShort - (fAreaDoorW + fAreaWindowW);
    }
    if (mdi->awl.orientation == EAST) {
      fAreaWallN = fAreaWallShort - (fAreaDoorN + fAreaWindowN);
      fAreaWallS = fAreaWallShort - (fAreaDoorS + fAreaWindowS);
      fAreaWallE = fAreaWallLong - (fAreaDoorE + fAreaWindowE);
      fAreaWallW = 0.0;
      fAreaWallCommonW = fAreaWallCommon;
    }
    if (mdi->awl.orientation == WEST) {
      fAreaWallN = fAreaWallShort - (fAreaDoorN + fAreaWindowN);
      fAreaWallS = fAreaWallShort - (fAreaDoorS + fAreaWindowS);
      fAreaWallE = 0.0;
      fAreaWallCommonE = fAreaWallCommon;
      fAreaWallW = fAreaWallLong - (fAreaDoorW + fAreaWindowW);
    }

    //      fAreaNetWall = fAreaGrossWall - (fAreaDoorT + fAreaWindowT);
    fAreaNetWall = fAreaWallN + fAreaWallS + fAreaWallE + fAreaWallW;
    if (mir->flgWhichPass == BASE_CASE)
      mir->fNetAWLArea = fAreaNetWall;

    if (mdi->awl.stud_size == SS_TWOXTWOMHEA)
      fStudDim = 1.75;
    if (mdi->awl.stud_size == SS_TWOXTHREEMHEA)
      fStudDim = 2.5;
    if (mdi->awl.stud_size == SS_TWOXFOURMHEA)
      fStudDim = 3.5;
    if (mdi->awl.stud_size == SS_TWOXSIXMHEA)
      fStudDim = 5.5;

    /***********************
    Check to see if the wall is intentionally ventilated.
    Changed the derate factor for ventilated walls from .25 to
    .50 on 10/02 MJF.  Changed to 0.75 on 1/21/03 MBG.
    ***********************/
    if (mdi->awl.wall_vent == WV_VENT)
      fCF = 0.75;
    else /* mdi->awl.wall_vent == WV_NOTVENT */
      fCF = 1.00;

    /***********************
    Check to see if any wall insulation measure has been applied.
    if so, will use compressed R's/inch.
    ***********************/

    iflgWallMeas = FALSE;
    if (mir->flgRetrofits[M_CMS_WALL_CELLULOSE_LOOSE_INSL_ADD] || mir->flgRetrofits[M_CMS_WALL_FIBERGLASS_LOOSE_INSL_ADD] ||
        mir->flgRetrofits[M_CMS_WALL_FIBERGLASS_BATT_INSL_ADD])
      iflgWallMeas = TRUE;
    /*****************************
    All three types of insulation are allowed for a single wall
    (layering is allowed).  Thus, check for all three types and
    sum the R value for each occurance.
    ***************************/
    if (mdi->awl.batt_insl > 0.0) {
      fRInsulation = mdi->key.batt_blanket_insulation_r_value_per_inch;
      if (iflgWallMeas == TRUE)
        fRInsulation = mir->fRinCompBatInsul;
      fInsulDepth = mdi->awl.batt_insl;
      fInsulDepthTotal = fInsulDepth;
      fRIxInsulDepth = fRInsulation * fInsulDepth * fCF;
    }
    if (mdi->awl.loose_insl > 0.0) {
      fRInsulation = mdi->key.loose_insulation_r_value_per_inch;

      // NW, 9/8/95: Disable these R-value overrides
      /*********************************
      if( mir->flgRetrofits[4] )
      fRInsulation = 3.7;
      if( mir->flgRetrofits[5] )
      fRInsulation = 4.5;
      ************************************/
      /* Re-institute overrides with global parameters. Continue to
      assume all insulation is at the added insulation R/inch.  Error
      shouldn't be significant.  MBG 7/03
      *************************************/

      if (mir->flgRetrofits[M_CMS_WALL_CELLULOSE_LOOSE_INSL_ADD])
        fRInsulation = mir->fRinCompCelInsul;
      else if (mir->flgRetrofits[M_CMS_WALL_FIBERGLASS_LOOSE_INSL_ADD])
        fRInsulation = mir->fRinFGCompressed;

      fInsulDepth = mdi->awl.loose_insl;
      fInsulDepthTotal += fInsulDepth;
      fRIxInsulDepth += fRInsulation * fInsulDepth;
    }
    if (mir->flgWhichPass == BASE_CASE)
      mir->fAWallInsDepth = fInsulDepthTotal;
    if (mdi->awl.foam_insl > 0.0) {
      fRInsulation = mdi->key.foamcore_insulation_r_value_per_inch;
      fInsulDepth = mdi->awl.foam_insl;

      if (fInsulDepthTotal <= 0.0)
        /***********************
        If foam core is the only existing wall insulation,
        then assume the wall to be ventilated.
        ***********************/
        fCF = 0.75;
      else
        fCF = 1.0;

      fInsulDepthTotal += fInsulDepth;
      fRIxInsulDepth += fRInsulation * fInsulDepth * fCF;

      /*****************************
      Additional variable "fRFoamCore" to be added to stud
      section of wall since it goes over the studs.
      ******************************/
      fRFoamCore = mdi->key.foamcore_insulation_r_value_per_inch * fInsulDepth;
    }

    /*****************************
    Check to see if the total insulation thickness or depth is
    less than the stud thickness.  If it is, then there is an
    air space and it must be accounted for. (Page 34, Hayter notes)
    ******************************/
    if (fInsulDepthTotal < fStudDim) {
      fAirSpaceDepth = fStudDim - fInsulDepthTotal;
      fRAirSpace = air_space_r_value_wal(fAirSpaceDepth);
      if (mir->flgWhichPass == BASE_CASE)
        mir->fAWLInsAirSpace = fAirSpaceDepth;
    } else {
      if (mir->flgWhichPass == BASE_CASE)
        mir->fAWLInsAirSpace = 0.0;
    }

    iSeason = 1; /* Summer */
    for (i = 0; i < 2; i++) {
      /**********************
      Set the air film R-values.
      *********************/
      if (iSeason) /* Summer */
      {
        fRExtAir = mdi->key.outside_wall_r_value_summer;
        fRExterior = 1.32f; // Added exterior sheathing
        fRInterior = mdi->key.interior_wall_r_value_summer;
        fRIntAir = 0.68f;
      } else /* Winter */
      {
        fRExtAir = mdi->key.outside_wall_r_value_winter;
        fRExterior = 1.32f; // Added exterior sheathing
        fRInterior = mdi->key.interior_wall_r_value_winter;
        fRIntAir = 0.68f;
      }

      fRWallFrame = (float)((fStudDim * RFRAME) + fRFoamCore + fRExtAir + fRExterior + fRIntAir + fRInterior);

      //          if( fInsulDepthTotal <= 0.0 )
      //              /***************
      //              If no insulation, assume that space between
      //              interior and exterior walls is unconditioned
      //              (see p. 33 of Sheila Hayter's Notes).
      //              ***************/
      //              fRWallCavity = fRIntAir + fRInterior + 0.68;
      //          else
      //          Elimination of air-space and skin not believed. Gives
      //          unreasonable savings for wall insulation. Concurrence
      //          with Mark Ternes.  MBG 3/08.

      fRWallCavity = fRIxInsulDepth + fRAirSpace + fRExtAir + fRExterior + fRIntAir + fRInterior;

      ASSERT(fRWallFrame != 0, sprintf(msg, "Assertion Failure"));
      ASSERT(fRWallCavity != 0, sprintf(msg, "Assertion Failure"));

      fUWall = (float)(((1.0 / fRWallFrame) * STUDFACTOR) + ((1.0 / fRWallCavity) * CAVITYFACTOR_WAL));

      /*******************
      Modify U to reflect cupboards, etc. by adding fRAdded_walls Rs.
      4/08
      ********************/
      ASSERT(fUWall != 0, sprintf(msg, "Assertion Failure"));
      fUWall = 1.0f / (1.0f / fUWall + fRAdded_walls);

      fUAWall = fUWall * fAreaNetWall;

      /********************
      Assume NO Overhang Modifier
      *********************/

      if (iSeason) {
        *fUA_WAL_S = fUAWall;
        *fSA_WAL_S_N = (float)(fUWall * SOLARAPPCONSTANT * fAreaWallN);
        *fSA_WAL_S_S = (float)(fUWall * SOLARAPPCONSTANT * fAreaWallS);
        *fSA_WAL_S_E = (float)(fUWall * SOLARAPPCONSTANT * fAreaWallE);
        *fSA_WAL_S_W = (float)(fUWall * SOLARAPPCONSTANT * fAreaWallW);
      } else {
        *fUA_WAL_W = fUAWall;
        *fSA_WAL_W_N = (float)(fUWall * SOLARAPPCONSTANT * fAreaWallN);
        *fSA_WAL_W_S = (float)(fUWall * SOLARAPPCONSTANT * fAreaWallS);
        *fSA_WAL_W_E = (float)(fUWall * SOLARAPPCONSTANT * fAreaWallE);
        *fSA_WAL_W_W = (float)(fUWall * SOLARAPPCONSTANT * fAreaWallW);
      }

      iSeason = 0; /* Winter */

    } /* End if( iSeason ) loop. */
  }

  /*****************************************************************
  ******************************************************************
  Home Wall UA, SA
  ******************************************************************
  *****************************************************************/

  /********************************
  Reset insulation caculation variables.
  *******************************/
  fRIxInsulDepth = fRAirSpace = 0.0;

  /********************************
  Reset door and window area variables.
  *******************************/
  fAreaDoorT = fAreaDoorN = fAreaDoorS = fAreaDoorE = fAreaDoorW = 0.0;
  fAreaWindowT = fAreaWindowN = fAreaWindowS = fAreaWindowE = fAreaWindowW = 0.0;

  /*************
  Get the total door area by orientation and a total area
  **************/

  if (mdi->num_dor > 0) // there are doors in mdi
  {
    for (i = 0; i < mdi->num_dor; i++) {
      /***************************
      For each record read in, there can
      be multiple doors and orientations.
      ***************************
      Changed 7/23/93 SLF - This may affect calculation results.
      ***************************/

      fAreaDoor = (float)((mdi->dor[i].height * INCHESTOFEET) * (mdi->dor[i].width * INCHESTOFEET));
      fAreaDoorN += fAreaDoor * mdi->dor[i].num_n;
      fAreaDoorS += fAreaDoor * mdi->dor[i].num_s;
      fAreaDoorE += fAreaDoor * mdi->dor[i].num_e;
      fAreaDoorW += fAreaDoor * mdi->dor[i].num_w;
      fAreaDoorT += fAreaDoorN + fAreaDoorS + fAreaDoorE + fAreaDoorW;
    }
  }

  /*************
  Get the total window area.
  **************/

  if (mdi->num_win > 0) // there are doors in mdi
  {
    for (i = 0; i < mdi->num_win; i++) {
      /***************************
      For each record read in, there can
      be  multiple windows and orientations.
      ***************************
      Changed 7/20/93 SLF - This will affect calculation results
      since the accumulation of window area wasn't correct before.
      ***************************/
      if (mdi->win[i].window_type != DOORWINMHEA && mdi->win[i].window_type != SKYLIGHTMHEA) {
        fAreaWindow = (float)((mdi->win[i].height * INCHESTOFEET) * (mdi->win[i].width * INCHESTOFEET));
        fAreaWindowN += fAreaWindow * mdi->win[i].num_n;
        fAreaWindowS += fAreaWindow * mdi->win[i].num_s;
        fAreaWindowE += fAreaWindow * mdi->win[i].num_e;
        fAreaWindowW += fAreaWindow * mdi->win[i].num_w;
        fAreaWindowT += fAreaWindowN + fAreaWindowS + fAreaWindowE + fAreaWindowW;
      }
    }
  }

  /*********************************
  Calculate the gross wall area.
  **********************************/
  fAreaWallLong = mdi->gnl.length * mdi->gnl.height;
  fAreaWallShort = mdi->gnl.width * mdi->gnl.height;

  /*********************************
  Calculate the net wall area.
  **********************************/
  if (mdi->wal.home_orientation == NORTH || mdi->wal.home_orientation == SOUTH) {
    fAreaWallN = fAreaWallLong - fAreaWallCommonS - (fAreaDoorN + fAreaWindowN);
    fAreaWallS = fAreaWallLong - fAreaWallCommonN - (fAreaDoorS + fAreaWindowS);
    fAreaWallE = fAreaWallShort - fAreaWallCommonW - (fAreaDoorE + fAreaWindowE);
    fAreaWallW = fAreaWallShort - fAreaWallCommonE - (fAreaDoorW + fAreaWindowW);
  } else if (mdi->wal.home_orientation == EAST || mdi->wal.home_orientation == WEST) {
    fAreaWallN = fAreaWallShort - fAreaWallCommonS - (fAreaDoorN + fAreaWindowN);
    fAreaWallS = fAreaWallShort - fAreaWallCommonN - (fAreaDoorS + fAreaWindowS);
    fAreaWallE = fAreaWallLong - fAreaWallCommonW - (fAreaDoorE + fAreaWindowE);
    fAreaWallW = fAreaWallLong - fAreaWallCommonE - (fAreaDoorW + fAreaWindowW);
  }

  /************************************************************
  Compute step wall area and add to net wall area.  MBG 5/30/03
  ************************************************************/

  if (mdi->rof.step_wall != NONE && mdi->rof.cathedral_ceiling > 0.0f) {

    fAreaStepWall = mdi->gnl.width * fHeightStepWall;

    if (mdi->rof.step_wall == NORTH)
      fAreaWallN += fAreaStepWall;
    else if (mdi->rof.step_wall == SOUTH)
      fAreaWallS += fAreaStepWall;
    else if (mdi->rof.step_wall == EAST)
      fAreaWallE += fAreaStepWall;
    else if (mdi->rof.step_wall == WEST)
      fAreaWallW += fAreaStepWall;
  }

  /*************************************************
  Insure each orientations wall area is non-negative
  *************************************************/

  if (fAreaWallN < 0.0)
    fAreaWallN = 0.0;
  if (fAreaWallS < 0.0)
    fAreaWallS = 0.0;
  if (fAreaWallE < 0.0)
    fAreaWallE = 0.0;
  if (fAreaWallW < 0.0)
    fAreaWallW = 0.0;

  //  fAreaNetWall = fAreaGrossWall - (fAreaDoorT + fAreaWindowT);
  fAreaNetWall = fAreaWallN + fAreaWallS + fAreaWallE + fAreaWallW;
  if (mir->flgWhichPass == BASE_CASE)
    mir->fNetWalArea = fAreaWallN + fAreaWallS + fAreaWallE + fAreaWallW;

  /***************************************************************
  Compute fraction of wall uninsulatable - NREL used 33% standard.
  Propose reducing to 15%. Add in user-designated additional area.
  6/03 MBG
  ***************************************************************/

  if (mir->flgWhichPass == BASE_CASE) {
    mir->fFractWallUnins = FRACTION_UNINSULATABLE_WALL + mdi->wal.uninsulatable_area / mir->fNetWalArea;
    if (mir->fFractWallUnins > 1.0f)
      mir->fFractWallUnins = 1.0f;
  }

  /*************************************************************
  Determine if wall insulation measure has been applied in order
  to know if un-insulatable area needs to be treated differently
  *************************************************************/

  // #334 bug number 7 fix
  // if (mir->flgRetrofits[M_CMS_WALL_CELLULOSE_LOOSE_INSL] || mir->flgRetrofits[M_CMS_WALL_FIBERGLASS_LOOSE_INSL] ||
  //     mir->flgRetrofits[M_CMS_WALL_FIBERGLASS_BATT_INSL])
  //   // iflgWallMeas = 1;

  if (mdi->wal.stud_size == SS_TWOXTWOMHEA)
    fStudDim = 1.75;
  if (mdi->wal.stud_size == SS_TWOXTHREEMHEA)
    fStudDim = 2.5;
  if (mdi->wal.stud_size == SS_TWOXFOURMHEA)
    fStudDim = 3.5;
  if (mdi->wal.stud_size == SS_TWOXSIXMHEA)
    fStudDim = 5.5;

  /***********************
  Check to see if the wall is intentionally ventilated.
  ***********************/
  if (mdi->wal.wall_vent == WV_VENT)
    fCF = 0.75;
  else /* mdi->wal.wall_vent == WV_NOTVENT */
    fCF = 1.00;

  /***********************************
  Modify existing batt insulation depth to account for
  compression after wall insulation measures are installed.
  ***********************************/

  fCompBatt = mdi->wal.batt_insl;
  if (mir->flgRetrofits[M_CMS_WALL_CELLULOSE_LOOSE_INSL])
    fCompBatt = mdi->wal.batt_insl * mir->fDensExistBatInsul / mdi->key.density_of_loose_cellulose_insulation;
  else if (mir->flgRetrofits[M_CMS_WALL_FIBERGLASS_LOOSE_INSL])
    fCompBatt = mdi->wal.batt_insl * mir->fDensExistBatInsul / mdi->key.density_of_loose_fiberglass_insulation;
  if (fCompBatt > mdi->wal.batt_insl)
    fCompBatt = mdi->wal.batt_insl;

  /******************************************************************
  Determine total cavity insulation depth to determine if cavity is
  filled, in which case compression will be assumed giving fiberglass
  a greater R/inch. Also save the existing depth of loose fill ins
  to allow wall insulation routine to compute its compression.
  If foam core is the only existing wall insulation,
  then assume the wall to be ventilated.
  *******************************************************************/

  fInsulDepthTotal = fCompBatt + mdi->wal.loose_insl;

  if (mir->flgWhichPass == BASE_CASE)
    mir->fWallInsDepth = fInsulDepthTotal;
  //  if(fInsulDepthTotal <= 0.0f) fCFfoam = 0.75f;
  //  else fCFfoam = 1.0f;
  //  Degradation of foam insulation abandoned. See also below. 2/08 MBG

  /*****************************
  All three types of insulation are allowed for a single wall
  (layering is allowed).  Thus, check for all three types and
  sum the R value for each occurance, noting when compressed
  R/in should be used.
  ***************************/

  /***************************************
  Use compressed batt R/in if the cavity is filled with more than just
  batt insulation, or the batt insulation measure has been installed and
  insulation existed prior to the retrofit, or the batt insulation measure
  was installed in a cavity less than 3.5" deep. MBG 7/03
  ***************************************/

  if (mdi->wal.batt_insl > 0.0) {

    fInsulDepth = fCompBatt; // use the compressed value MJF 7/03
    fRInsulation = MAX(mir->fRinFGCompressed, mdi->key.batt_blanket_insulation_r_value_per_inch);

    if ((fInsulDepthTotal >= 0.99f * fStudDim && mdi->wal.loose_insl > 0.0) ||
        (mir->flgRetrofits[M_CMS_WALL_FIBERGLASS_BATT_INSL] && fUnInsulR > 0.0 && fStudDim < 4.0f)) {

      if (fInsulDepth > fStudDim)
        fInsulDepth = fStudDim;
    }
    // Batt insulation measure can give ins depths > stud depth

    else if (fInsulDepth > 1.01f * fStudDim) {
      fInsulDepth = fStudDim;
    }

    else
      fRInsulation = mdi->key.batt_blanket_insulation_r_value_per_inch;
    fRIxInsulDepth += fRInsulation * fInsulDepth * fCF;
  }

  /******************************************
  If insulation doesn't fill cavity, use uncompressed R/in for loose
  insulation. Otherwise assume loose is compressed  MBG 7/03
  ******************************************/

  if (mdi->wal.loose_insl > 0.0) {
    fInsulDepth = mdi->wal.loose_insl;
    if (fInsulDepthTotal < 0.99f * fStudDim) {
      fRInsulation = mdi->key.loose_insulation_r_value_per_inch;
      fRIxInsulDepth += fRInsulation * fInsulDepth;
    } else {
      if (mir->flgRetrofits[M_CMS_WALL_CELLULOSE_LOOSE_INSL])
        fRInsulation = mir->fRinCompCelInsul;
      else
        fRInsulation = mir->fRinFGCompressed;
      fRIxInsulDepth += fRInsulation * fInsulDepth;
    }
  }

  if (mdi->wal.foam_insl > 0.0) {
    fRInsulation = mdi->key.foamcore_insulation_r_value_per_inch;
    fInsulDepth = mdi->wal.foam_insl;
    //      fInsulDepthTotal += fInsulDepth;
    //        Redefine to be cavity insulation depth only. MBG 6/03
    //      fRIxInsulDepth += fRInsulation * fInsulDepth * fCFfoam;
    //        Degradation of foam insulation abandoned. See also above. MBG 2/08
    fRIxInsulDepth += fRInsulation * fInsulDepth;

    /*****************************
    Additional variable "fRFoamCore" to be added to stud
    section of wall since it goes over the studs.
    ******************************/
    fRFoamCore = mdi->key.foamcore_insulation_r_value_per_inch * fInsulDepth;
  }

  /*****************************
  Check to see if the total insulation thickness or depth is
  less than the stud thickness.  If it is, then there is an
  air space and it must be accounted for. (Page 34, Hayter notes)
  ******************************/
  if (fInsulDepthTotal < 0.99f * fStudDim) {
    fAirSpaceDepth = fStudDim - fInsulDepthTotal;
    fRAirSpace = air_space_r_value_wal(fAirSpaceDepth);
    if (mir->flgWhichPass == BASE_CASE)
      mir->fWalInsAirSpace = fAirSpaceDepth;
  } else {
    if (mir->flgWhichPass == BASE_CASE)
      mir->fWalInsAirSpace = 0.0;
  }

  /*************************************************************
  Preserve pre-retrofit insulation and air-space R-values for
  uninsulatable wall area  MBG 6/03
  *************************************************************/
  if (mir->flgWhichPass == BASE_CASE) {
    fUnInsulR = fRIxInsulDepth;
    fUnInsulAirR = fRAirSpace;
  }

  if (cmds.debug_level & D_MHEA_ENERGY_DETAIL)
    fprintf(stderr, "----UA OF WALLS----\n");

  iSeason = 1; /* Summer */
  for (i = 0; i < 2; i++) {
    /**********************
    Set the air films R values.
    Changed 3/08 to use values from setup library.
    *********************/

    if (iSeason) /* Summer */
    {
      fRExtAir = mdi->key.outside_wall_r_value_summer;
      fRExterior = 0.0;
      fRInterior = mdi->key.interior_wall_r_value_summer;
      fRIntAir = 0.68f;
    } else /* Winter */
    {
      fRExtAir = mdi->key.outside_wall_r_value_winter;
      fRExterior = 0.0;
      fRInterior = mdi->key.interior_wall_r_value_winter;
      fRIntAir = 0.68f;
    }

    fRWallFrame = (float)((fStudDim * RFRAME) + fRFoamCore + fRExtAir + fRExterior + fRIntAir + fRInterior);

    //      if( fInsulDepthTotal <= 0.0 )
    //          /***************
    //          If no insulation, assume that space between
    //          interior and exterior walls is unconditioned
    //          (see p. 33 of Sheila Hayter's Notes).
    //          (Note redefinition of fInsulDepthTotal if
    //          above were re-instituted.  MBG 6/30 )
    //          ***************/
    //          fRWallCavity = fRIntAir + fRInterior + 0.68;
    //      else
    //      Elimination of air-space and skin not believed.  Gives
    //      unreasonable savings for wall insulation. Concurrence
    //      with Mark Ternes.  MBG 5/03.

    fRWallCavity = fRIxInsulDepth + fRAirSpace + fRExtAir + fRExterior + fRIntAir + fRInterior;

    fRWallCavityUnins = fUnInsulR + fUnInsulAirR + fRExtAir + fRExterior + fRIntAir + fRInterior;

    ASSERT(fRWallFrame != 0, sprintf(msg, "Assertion Failure"));
    ASSERT(fRWallCavity != 0, sprintf(msg, "Assertion Failure"));
    ASSERT(fRWallCavityUnins != 0, sprintf(msg, "Assertion Failure"));

    fUWall = (float)(((1.0 / fRWallFrame) * STUDFACTOR) + ((1.0 / fRWallCavity) * CAVITYFACTOR_WAL));

    fUWallUnins = (float)(((1.0 / fRWallFrame) * STUDFACTOR) + ((1.0 / fRWallCavityUnins) * CAVITYFACTOR_WAL));

    /*******************
    Modify U to reflect cupboards, etc. by adding fRAdded_walls Rs.
    4/08
    ********************/
    ASSERT(fUWall != 0, sprintf(msg, "Assertion Failure"));
    ASSERT(fUWallUnins != 0, sprintf(msg, "Assertion Failure"));
    fUWall = 1.0f / (1.0f / fUWall + fRAdded_walls);
    fUWallUnins = 1.0f / (1.0f / fUWallUnins + fRAdded_walls);

    fUAWall = fUWall * fAreaNetWall * (1.0f - mir->fFractWallUnins) + fUWallUnins * fAreaNetWall * mir->fFractWallUnins;

    if (cmds.debug_level & D_MHEA_ENERGY_DETAIL) {
      sprintf(mir->sMsg, "fUWall         = %8.3f \n"
                       "fUWallUnins    = %8.2f \n"
                       "fAreaNetWall   = %8.2f \n"
                       "fFractionWallUnins = %8.2f \n"
                       "fUAWall        = %8.2f \n",
              fUWall, fUWallUnins, fAreaNetWall, mir->fFractWallUnins, fUAWall);
      fprintf(stderr, "%s", mir->sMsg);
      // GKA The following string is too long for the buffer.
      // Symptom I'm observing is that the GLBprojectNameMHEA is getting overwritten with junk.
      // However, I suspect this could be any of the Globals, depending on which one has
      // the unfortunate luck of being allocated in the space being assaulted by mir->sMsg content.
      // A simple, short solution for now is to write a smaller string to the buffer.
      // Longer term, the sprintf should be replaced by snprintf, or some other way to
      // safely write these buffers.
      /*sprintf( mir->sMsg, "\nfRIxInsulDepth = %8.2f \n"
      "fUnInsulR      = %8.2f \n"
      "fRAirSpace     = %8.2f \n"
      "fRUnInsulAirR  = %8.2f \n"
      "fRExtAir       = %8.2f \n"
      "fRExterior     = %8.2f \n"
      "fRIntAir       = %8.2f \n"
      "fRInterior     = %8.2f \n"
      "fRWallCavity   = %8.2f \n"
      "fRWallFrame    = %8.2f \n"
      "STUDFACTOR     = %8.2f \n"
      "CAVITYFCTR_WAL = %8.2f \n\n",
      fRIxInsulDepth, fUnInsulR, fRAirSpace, fUnInsulAirR,
      fRExtAir, fRExterior, fRIntAir, fRInterior,
      fRWallCavity, fRWallFrame, STUDFACTOR,
      CAVITYFACTOR_WAL ); */
      sprintf(mir->sMsg, "\nfRIxInsulDepth = %8.2f \n"
                       "fUnInsulR      = %8.2f \n"
                       "fRAirSpace     = %8.2f \n"
                       "fRUnInsulAirR  = %8.2f \n"
                       "fRExtAir       = %8.2f \n"
                       "fRExterior     = %8.2f \n",
              fRIxInsulDepth, fUnInsulR, fRAirSpace, fUnInsulAirR, fRExtAir, fRExterior);
      fprintf(stderr, "%s", mir->sMsg);
      sprintf(mir->sMsg, "\nfRIntAir       = %8.2f \n"
                       "fRInterior     = %8.2f \n"
                       "fRWallCavity   = %8.2f \n"
                       "fRWallFrame    = %8.2f \n"
                       "STUDFACTOR     = %8.2f \n"
                       "CAVITYFCTR_WAL = %8.2f \n\n",
              fRIntAir, fRInterior, fRWallCavity, fRWallFrame, STUDFACTOR, CAVITYFACTOR_WAL);
      fprintf(stderr, "%s", mir->sMsg);
    }

    /********************
    Ratios for Overhang Modifier calculations (part of Solar Gain)
    *********************/
    if (mdi->wal.porch_orientation == SOUTH && (mdi->wal.porch_length > 0.0) && (mdi->wal.porch_width > 0.0)) {

      ASSERT(mdi->gnl.height != 0, sprintf(msg, "Assertion Failure"));
      ASSERT(mdi->gnl.length != 0, sprintf(msg, "Assertion Failure"));

      *fShadingRatioW2H = mdi->wal.porch_width / mdi->gnl.height;
      if ((*fShadingRatioL = mdi->wal.porch_length / mdi->gnl.length) > 1.0)
        *fShadingRatioL = 1.0;
    } else {
      *fShadingRatioL = 0.0;
      *fShadingRatioW2H = 0.0;
    }

    if (iSeason) {
      *fUA_WAL_S += fUAWall;
      *fSA_WAL_S_N += (float)(fUWall * SOLARAPPCONSTANT * fAreaWallN);
      *fSA_WAL_S_S += (float)(fUWall * SOLARAPPCONSTANT * fAreaWallS);
      *fSA_WAL_S_E += (float)(fUWall * SOLARAPPCONSTANT * fAreaWallE);
      *fSA_WAL_S_W += (float)(fUWall * SOLARAPPCONSTANT * fAreaWallW);
    } else {
      *fUA_WAL_W += fUAWall;
      *fSA_WAL_W_N += (float)(fUWall * SOLARAPPCONSTANT * fAreaWallN);
      *fSA_WAL_W_S += (float)(fUWall * SOLARAPPCONSTANT * fAreaWallS);
      *fSA_WAL_W_E += (float)(fUWall * SOLARAPPCONSTANT * fAreaWallE);
      *fSA_WAL_W_W += (float)(fUWall * SOLARAPPCONSTANT * fAreaWallW);
    }

    iSeason = 0; /* Winter */

  } /* End if( iSeason ) loop. */

  return;

} /* End of the ua_wall() user funtion. */
