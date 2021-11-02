/*******************  MODULE NAME:  UA_FLR.C  ****************************/
/**       DATE: 3/14/93                                                 **/
/**          BY:    SLF                                                 **/
/** DESCRIPTION:    Reads values from form data structures and contains **/
/**                 equations for 5 UA calculations.                    **/
/** REVISIONS:  09/01/93 SLF - Took out code for variable flag          **/
/**                   flgWingInsulationLocation.drapedBelow to match frf**/
/**                 10/07/93 SLF - Updated comments and calculations    **/
/**                 10/26/93 SLF - Updated calculations and comments    **/
/**                 11/01/93 SLF - Updated calculations and comments    **/
/**                 12/30/93 SLF - Updated calcs for Additions          **/
/**                 11/16/94 SLF - Reordered mir->flgRetrofits.           **/
/**                7/15/95 NLW - Belly airspace adjustments             **/
/**                 9/1/95 NLW - More belly airspace adjustments        **/
/**               12/19/98 MJF - elimination of GetData -- now used mdi **/
/**                              extensive revisions to all variable    **/
/**                              references                             **/
/*************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wa_engine.h"

/*************************************************************************/
/*******************  MODULE NAME:  UA_FLR    ****************************/
/*************************************************************************/
// UNITS
#define RFRAME 1.23           // R/inch
#define JOISTFACTOR 0.10      // unitless
#define CAVITYFACTOR_FLR 0.90 // unitless
#define TWOXFOUR 3.5          // inches
#define TWOXSIX 5.5           // inches
#define TWOXEIGHT 7.5         // inches

/*******************  FUNCTION NAME:  air_space_r_value_flr  ******************/
/**         DATE:  03/08/93                                                         **/
/**           BY:    SLF/JLG                                                                **/
/**  DESCRIPTION:    See page 39 in Notes by Sheila Hayter.                 **/
/*************************************************************************/
float air_space_r_value_flr(float airSpaceHeight, int iSeason) {
  float fRAirSpace;

  fRAirSpace = 0.0;

  if (airSpaceHeight > 0.0) {
    if (iSeason == 2)
      fRAirSpace = (float)(1.0398 * pow(airSpaceHeight, 0.14602));
    if (iSeason == 1)
      fRAirSpace = (float)(0.7816 * pow(airSpaceHeight, 0.0577));
  }
  return (fRAirSpace);

}

/*******************  FUNCTION NAME:  ua_floor  **************************/
/**         DATE:  03/93                                                                **/
/**           BY:    JG                                                                 **/
/**  DESCRIPTION:    See Notes by Sheila Hayter.                                    **/
/*************************************************************************/
void ua_floor(float *fUA_FLR_S, float *fUA_FLR_W) {
  /*************************************
  General R-values that apply to both wing and belly calcs.
  **************************************/
  float fUFloor = 0.0;
  float fUATotal;
  float fROutsideAir;
  float fRIntFlr;
  //float fRStillAirFlr; // No longer used

  /***********************************
  General size and flag values that apply to both wing and belly calcs.
  ***********************************/
  float fAFloor;
  float fATotal;
  float fWidth;
  float fLength;
  float fAreaWHC = 0.0;   // feet squared
  float fLengthWHC = 0.0; // feet
  int i, iSummer, iSeason;
  float fBellyCondDerate = 1.0; // how much to derate Rvalues in belly due to general condition

  /*********************************
  Belly R- and U-values.
  ********************************/
  float fUFloorBelly = 0.0;
  float fRInsLoose = 0.0;
  float fRBellyInsulation = 0.0;
  float fRBellyMineral = 0.0;
  float fRBellyLooseJoist = 0.0;  // Rvalue of loose insul under joist
  float fRBellyLooseCavity = 0.0; // Rvalue of loose insul between joists
  float fRBellyFraming = 0.0;
  float fRBellyAirCavity = 0.0;
  float fRBellyAirJoist = 0.0;

  /*********************************
  Belly sizes & flags.
  ********************************/
  float fBellyInsulDepth = 0.0;
  float fBellyMinDepth = 0.0;        // Batt/Blanket Mineral insul depth
  float fBellyLooseDepth = 0.0;      // Loose insulation total depth
  float fBellyLooseJoistDepth = 0.0; // Loose insul depth under joist
  float fBellyAirSpaceJoist = 0.0;   // Air space under joists
  float fBellyAirSpaceCavity = 0.0;  // Air space between joists
  int iBellyInsulAtFlr = 0;
  int iBellyInsulAtJoist = 0;
  int iBellyInsulUnderJoist = 0;
  int iBellyInsulDraped = 0;
  int iBellySquare = 1;
  float fBellyJoistSize = 0.0;
  float fBellyDepth;
  //  float       ftempdepth = 0.0;

  /******************************
  Wing R- and U-values.
  ******************************/
  float fRWngInsulation = 0.0;
  float fRWngAirSpace = 0.0;
  float fRWngFraming = 0.0;
  float fUFloorWng;

  /******************************
  Wing sizes and flags.
  ******************************/
  float fWngInsulDepth = 0.0;
  float fWngMinDepth = 0.0;
  float fWngCelDepth = 0.0;
  float fWngLooseDepth = 0.0;
  float fWngAirSpace = 0.0;
  float fWngJoistSize = 0.0;                 // GKA initialize this value
  float fDensExist;                          // Density of existing insulation
  static float fsWngExistMinDepth = 0.0;     // Existing depth of batt insulation in wing
  static float fsWngExistLooseDepth = 0.0;   // Existing depth of loose FG insulation in wing
  //static float fsBellyExistMinDepth = 0.0;   // Existing depth of batt insulation in belly
  static float fsBellyExistLooseDepth = 0.0; // Existing depth of loose FG insulation in belly
  int iWngInsulAtFlr = 0;
  int iWngInsulAtJoist = 0;
  int iWngInsulUnderJoist = 0;

  /****************************
  Bandjoist R- and U-value, size, and flag variables.
  ***************************/
  float fRBjoist;
  float fUBjoist;
  //float fRStillAirBjoist;
  float fABjoist = 0.0;
  //float fBjoistHeight;
  float fBjoistThickness = 1.5f;

  /*************************************
  General R-values that apply to addition calcs.
  **************************************/
  float fUFloorTotalAddn = 0.0;
  float fUATotalAddn;

  /***********************************
  General size and flag values that apply to addition calcs.
  ***********************************/
  float fAFloorAddn;
  float fATotalAddn;

  /******************************
  Addition R- and U-values.
  ******************************/
  float fRAddnInsulation = 0.0;
  float fRAddnAirSpace = 0.0;
  float fRAddnFraming = 0.0;
  float fUFloorAddn;
  float fRinchFlr;

  /******************************
  Addition sizes and flags.
  ******************************/
  float fAddnInsulDepth = 0.0;
  float fAddnMinDepth = 0.0;
  float fAddnLooseDepth = 0.0;
  float fAddnAirSpace = 0.0;
  float fAddnJoistSize;
  int iAddnInsulAtFlr = 0;
  int iAddnInsulAtJoist = 0;
  int iAddnInsulUnderJoist = 0;
  int iflgFlrMeas = FALSE;

  /****************************
  Addition Bandjoist R- and U-value, size, and flag variables.
  ***************************/
  float fRBjoistAddn;
  float fUBjoistAddn;
  //float fRStillAirBjoistAddn;
  float fABjoistAddn = 0.0;
  //float fBjoistAddnHeight;

  /********************************************************
  R-Value added to compensate for cupboards, furniture, etc.
  *********************************************************/
  float fRAdded_floor = 1.0f;

  /***********************************
  General Building Information
  ************************************/
  fWidth = mdi->gnl.width;
  fLength = mdi->gnl.length;

  /* Outside Water Heater Closet (area to subtract) */
  if (mdi->gnl.water_heater_closet == YES) {
    fAreaWHC = 6.25;  // feet squared
    fLengthWHC = 2.5; // feet
  }

  if (cmds.debug_level & D_MHEA_ENERGY_DETAIL)
    fprintf(stderr, "----UA OF FLOORS----\n");

  /**********************************
  General Floor Information
  ************************************/

  // Here is the general Rvalue derating factor from
  // belly condition (MJF 5/1/02).  We might want to
  // make these factors programmable, or add the factor
  // as an input

  switch (mdi->flr.belly_condition) {
  case BC_GOOD:
    fBellyCondDerate = 1.0f;
    break;
  case BC_AVERAGE:
    fBellyCondDerate = 0.93f;
    break;
  case BC_POOR:
    fBellyCondDerate = 0.85f;
    break;
  default:
    fBellyCondDerate = 1.0f;
    break;
  }

  /**************************
  Belly U-value Calculations  (season-independent)
  **************************/

  // Assume existing loose insulation is fiberglass

  if (mir->flgWhichPass == BASE_CASE) {
    //fsBellyExistMinDepth = mdi->flr.belly_mineral_insl;
    fsBellyExistLooseDepth = mdi->flr.belly_loose_insl;
  }

  // If either insulation measure is installed assume insulation is moderately compressed

  if (mir->flgRetrofits[M_CMS_BELLY_FIBERGLASS_LOOSE_INSL] || mir->flgRetrofits[M_CMS_BELLY_CELLULOSE_LOOSE_INSL])
    fRInsLoose = mir->fRinBellyLFGInsul;
  else
    fRInsLoose = mdi->key.loose_insulation_r_value_per_inch;

  if (mdi->flr.belly_joist_size == JS_TWOXFOUR)
    fBellyJoistSize = TWOXFOUR;
  else if (mdi->flr.belly_joist_size == JS_TWOXSIX)
    fBellyJoistSize = TWOXSIX;
  else if (mdi->flr.belly_joist_size == JS_TWOXEIGHT)
    fBellyJoistSize = TWOXEIGHT;

  fRBellyFraming = (float)(fBellyJoistSize * RFRAME);
  fBellyMinDepth = mdi->flr.belly_mineral_insl;
  fBellyLooseDepth = mdi->flr.belly_loose_insl;

  if (mdi->flr.belly_cavity == BC_SQUARE)
    iBellySquare = 1;
  else
    iBellySquare = 0; /* Belly is Rounded or Flat */

  fBellyDepth = mdi->flr.belly_depth;

  if (cmds.debug_level & D_MHEA_ENERGY_DETAIL) {
    sprintf(mir->sMsg, "Depth, LooseDepth = %8.2f %8.2f \n", fBellyDepth, fBellyLooseDepth);
    fprintf(stderr, "%s", mir->sMsg);
  }

  if (mdi->flr.belly_insl_location == DRAPEDBELOW)
  /***************************
  If the insulation is draped below, the total insulation value for
  the belly will generally be the R for the batt/blanket mineral
  insulation plus the R for the loose fill insulation.  Assume that
  the batt/blanket insulation is located at the bottom of the drape.
  ****************************/
  {
    iBellyInsulDraped = 1;

    if (fBellyMinDepth > 0.0)
      fRBellyMineral = mdi->key.batt_blanket_insulation_r_value_per_inch * fBellyMinDepth;

    // ???? is this a logic mistake from a careless replacement MJF 12/98

    if (fBellyLooseDepth < 0.0)
      fBellyLooseDepth = 0.0;
    //      if( fBellyLooseDepth > 0.0 )
    /*************************
    Calculate the thickness of loose insulation and air space
    underneath the joists (Joist) and between the joists (Cavity).
    ***************************/
    {
      /**************
      Square Belly
      **************/
      if (iBellySquare) {
        if (fBellyLooseDepth >= (fBellyDepth - fBellyJoistSize - fBellyMinDepth))
        /******************
        The loose insul fills the space under the joist.

        NW, 9/1/95: Future versions may wish to consider
        insulation compression here
        ******************/
        {
          fBellyLooseJoistDepth = fBellyDepth - fBellyJoistSize - fBellyMinDepth;
          fBellyAirSpaceJoist = 0.0;
          fBellyAirSpaceCavity = fBellyDepth - fBellyLooseDepth - fBellyMinDepth;
          if (fBellyAirSpaceCavity < 0.0)
            fBellyAirSpaceCavity = 0.0;
        } else
        /******************
        The loose insul does not fill up to joists
        ******************/
        {
          fBellyLooseJoistDepth = fBellyLooseDepth;
          fBellyAirSpaceJoist = fBellyDepth - fBellyJoistSize - fBellyMinDepth - fBellyLooseDepth;
          if (mir->flgWhichPass == BASE_CASE)
            mir->fJoistAirSpace = fBellyAirSpaceJoist;
          fBellyAirSpaceCavity = fBellyAirSpaceJoist + fBellyJoistSize;
        }
      }
      /**************
      Round Belly or Flat
      **************/
      else {
        if (fBellyLooseDepth >= (fBellyDepth - fBellyJoistSize - fBellyMinDepth))
        /******************
        The loose insul fills the space under the joist
        ******************/
        {
          fBellyLooseJoistDepth = (float)(0.5 * (fBellyDepth - fBellyJoistSize - fBellyMinDepth));
          fBellyAirSpaceJoist = 0.0;
          // NW,9/5/95: Don't reduce cavity depth for rounding at a level above the
          //                  bottom of the floor joists
          //                  fBellyAirSpaceCavity = 0.5 * (fBellyDepth - fBellyLooseDepth -
          //                                                          fBellyMinDepth);
          fBellyAirSpaceCavity = (fBellyDepth - fBellyLooseDepth - fBellyMinDepth);
          if (fBellyAirSpaceCavity < 0.0)
            fBellyAirSpaceCavity = 0.0;
        } else
        /******************
        The loose insul does not fill up to joists
        ******************/
        {
          // NW,9/5/95: Correction to insulation and airspace depth calculations
          //                  fBellyLooseJoistDepth = 0.5 * fBellyLooseDepth;
          // Don't need to reduce insulation thickness beneath joists
          fBellyLooseJoistDepth = fBellyLooseDepth; // NW,9/5/95
          fBellyAirSpaceJoist = (float)(0.5 * (fBellyDepth - fBellyJoistSize - fBellyMinDepth - fBellyLooseDepth));
          if (mir->flgWhichPass == BASE_CASE)
            mir->fJoistAirSpace = fBellyAirSpaceJoist;
          // Don't need to reduce cavity depth for rounded belly between the joists
          //                  fBellyAirSpaceCavity = 0.5 * (fBellyAirSpaceJoist +
          //                                                          fBellyJoistSize);
          fBellyAirSpaceCavity = (fBellyAirSpaceJoist + fBellyJoistSize); // NW,9/5/95
        }
      }

      // If the cellulose measure has not been implemented, assume all
      // loose insulation is FG.  Else, split the loose FG from the added
      // loose cellulose. MBG 7/03

      if (!mir->flgRetrofits[M_CMS_BELLY_CELLULOSE_LOOSE_INSL]) {

        /* R-value of loose insulation for depth UNDER the Joist */
        fRBellyLooseJoist = fRInsLoose * fBellyLooseJoistDepth;

        /* R-value of loose insulation for total belly depth */
        fRBellyLooseCavity = fRInsLoose * fBellyLooseDepth;
      }

      else {
        if (fBellyLooseJoistDepth - fsBellyExistLooseDepth > 0.0f) {
          fRBellyLooseJoist =
              fRInsLoose * fsBellyExistLooseDepth + mir->fRinBellyCelInsul * (fBellyLooseJoistDepth - fsBellyExistLooseDepth);
        } else
          fRBellyLooseJoist = mir->fRinBellyCelInsul * fBellyLooseJoistDepth;

        if (fBellyLooseDepth - fsBellyExistLooseDepth > 0.0f) {
          fRBellyLooseCavity =
              fRInsLoose * fsBellyExistLooseDepth + mir->fRinBellyCelInsul * (fBellyLooseDepth - fsBellyExistLooseDepth);
        } else
          fRBellyLooseCavity = mir->fRinBellyCelInsul * fBellyLooseDepth;
      }

      // Save insulation depth for compression calcs in measure.c
      if (mir->flgWhichPass == BASE_CASE)
        mir->fBellyInsDepth = fBellyLooseDepth;

    } // End if Square Belly
  }   // End if DRAPEDBELOW
  else



  /********************************
  The insulation is not draped.  There may be layering of the
  loose and batt insulation.  The insulation is attached to the
  floor, attached to the joists, or there is no insulation at all.
  *******************************/
  {
    if (mdi->flr.belly_insl_location == ATTACHEDFLOOR)
      iBellyInsulAtFlr = 1;
    else if (mdi->flr.belly_insl_location == BETWEENJOISTS)
      iBellyInsulAtJoist = 1;
    else if (mdi->flr.belly_insl_location == ATTACHEDUNDER)
      iBellyInsulUnderJoist = 1;

    if (fBellyMinDepth > 0.0) {
      fBellyInsulDepth = fBellyMinDepth;
      fRBellyInsulation = mdi->key.batt_blanket_insulation_r_value_per_inch * fBellyMinDepth;

      /***************************
      If the floor joists are perpendicular to the direction
      of batt/blanket insulation, then the insulating
      value of the insulation is reduced by 12.5%.
      ***************************/
      //          if( iJoistLengthWise != iInsulLengthWise )
      if (mdi->flr.belly_insl_location == ATTACHEDUNDER)
        fRBellyInsulation *= 0.875;
    }

    if (fBellyLooseDepth > 0.0) {
      fBellyInsulDepth += fBellyLooseDepth;

      // If the cellulose measure has not been implemented, assume all
      // loose insulation is FG.  Else, split the loose FG from the added
      // loose cellulose.  MBG 7/03

      if (!mir->flgRetrofits[M_CMS_BELLY_CELLULOSE_LOOSE_INSL])
        fRBellyInsulation += fRInsLoose * fBellyLooseDepth;
      else {

        if (fBellyLooseDepth - fsBellyExistLooseDepth > 0.0f) {
          fRBellyInsulation +=
              fRInsLoose * fsBellyExistLooseDepth + mir->fRinBellyCelInsul * (fBellyLooseDepth - fsBellyExistLooseDepth);
        } else
          fRBellyInsulation += mir->fRinBellyCelInsul * fBellyLooseDepth;
      }
    }

    if (mdi->flr.belly_cavity == BC_FLAT && (fBellyLooseDepth > 0.0) && (fBellyMinDepth <= 0.0))
      /********************
      If there is a flat belly with only loose fill insulation
      on top of the tar wrap, it will be attached to the joist.
      ********************/
      iBellyInsulAtJoist = 1;

    if ((fBellyJoistSize >= fBellyInsulDepth) && iBellyInsulAtFlr)
    /*****************************
    The bottom of the joist is sticking out below the insulation.
    This section of the joist will not be treated, thus the size
    of the joist is made equal to the depth of the insulation and
    the R-value of the joist is adjusted accordingly.
    *******************************/
    {
      if (mdi->flr.belly_cavity != BC_FLAT)
      /*****************************
      If belly is flat, assume rodent barrier is attached to bottom
      of joist and assume separate heat flow path. Thus don't adjust
      effective joist depth to equal the insulation depth. 1/08 MBG
      ******************************/
      {
        fBellyJoistSize = fBellyInsulDepth;
      }
      fRBellyFraming = (float)(fBellyJoistSize * RFRAME);
      /** NLW, 9/1/95, Adjusted belly airspace calc for cavity def.**/
      fBellyAirSpaceCavity = fBellyDepth - fBellyInsulDepth;
    }

    if ((fBellyJoistSize > fBellyInsulDepth) && iBellyInsulAtJoist)
      /************************************
      There is an air space between the insulation and the floor in
      the joist cavity.  This air space thickness is calculated here
      and the corresponding R-value is calculated later.
      *************************************/
      /************************************
      Adjust definition of insulatable belly
      airspace to refer to space below insulation at joist rather
      than space above.
        See NREL notes 92.5 NLW, ERG, 7/15/94
      *************************************/
      //          fBellyAirSpaceCavity = fBellyJoistSize - fBellyInsulDepth;
      /************************************
      NW, 9/1/95; Adjust definition of insulatable belly
      airspace to refer to any space in the belly
      per conversation w/ Ron Judkoff.
      Typical retrofit inserts a fill tube through the
      existing batt/blanket to generally fill all available
      space.
      *************************************/
      //          fBellyAirSpaceCavity = fBellyDepth - fBellyJoistSize -
      //                      fBellyInsulDepth;
      fBellyAirSpaceCavity = fBellyDepth - fBellyInsulDepth;

    if (!iBellyInsulAtJoist && !iBellyInsulAtFlr)
      fBellyAirSpaceCavity = fBellyDepth - fBellyInsulDepth;

    if (iBellyInsulUnderJoist && mdi->flr.belly_cavity == BC_FLAT)
    /*************************
    Insulation bulges into cavity to about 1/2 its depth. Adjust
    the cavity air space accordingly. 1/08
    *************************/
    {
      fBellyAirSpaceCavity = fBellyJoistSize - 0.5f * fBellyInsulDepth;
      if (mir->flgWhichPass == BASE_CASE)
        mir->fBellyAirSpace = fBellyAirSpaceCavity;

      if (cmds.debug_level & D_MHEA_ENERGY_DETAIL) {
        fprintf(stderr, "fBellyJoistSize:%8.3f\n", fBellyJoistSize);
        fprintf(stderr, "BellyInsulDepth:%8.3f\n", fBellyInsulDepth);
        fprintf(stderr, "BellyAirSpaceCavity:%8.3f\n", fBellyAirSpaceCavity);
        fprintf(stderr, "mir->fBellyAirSpace:%8.3f\n", mir->fBellyAirSpace);
      }

    }

    // Save insulation depth for compression calcs in measure.c
    if (mir->flgWhichPass == BASE_CASE)
      mir->fBellyInsDepth = fBellyInsulDepth;

  } // End if-else for flgBellyInsulationLocation

  /**************************
  Wing U-value Calculations  (season-independent)
  **************************/

  if (mdi->flr.wing_joist_size == JS_TWOXFOUR)
    fWngJoistSize = TWOXFOUR;
  else if (mdi->flr.wing_joist_size == JS_TWOXSIX)
    fWngJoistSize = TWOXSIX;
  else if (mdi->flr.wing_joist_size == JS_TWOXEIGHT)
    fWngJoistSize = TWOXEIGHT;

  /*******************
  The bandjoist height equals the wing joist size only when the wing
  section is uninsulated.  Otherwise the bandjoist height equals the
  air space height if insulation is attached to joist.  If insulation
  is attached to flooring, then there is no uninsulated bandjoist height
  (see other occurances of fRBjoist).

  Validation Note: This calculation is from PNL and not EEDO.

  Above modified 1/08 to model horizontal heat flow through band joist
  from belly. Insulation attached to the floor effects the overall band
  joist R-value while insulation between the joists at the bottom of the
  joists effects the area through which the heat flows.
  *******************/

  //fBjoistHeight = fWngJoistSize;
  //  fRBjoist    = fBjoistHeight * RFRAME;
  fRBjoist = (float)(fBjoistThickness * RFRAME);

  fRWngFraming = (float)(fWngJoistSize * RFRAME);
  fWngMinDepth = mdi->flr.wing_mineral_insl;
  fWngLooseDepth = mdi->flr.wing_loose_insl;

  if (mir->flgWhichPass == BASE_CASE) {
    fsWngExistMinDepth = fWngMinDepth;
    fsWngExistLooseDepth = fWngLooseDepth;
  }

  /********************************
  The insulation is not draped and there may be layering of the
  loose and batt insulation.  The insulation is attached to the
  floor, attached to or under the joists, or there is no
  insulation at all.
  *******************************/
  if (mdi->flr.wing_insl_location == ATTACHEDFLOOR)
    iWngInsulAtFlr = 1;
  else if (mdi->flr.wing_insl_location == BETWEENJOISTS)
    iWngInsulAtJoist = 1;
  else if (mdi->flr.wing_insl_location == ATTACHEDUNDER)
    iWngInsulUnderJoist = 1;

  /**************************************************************
  Modify insulation depths to account for compression after floor
  insulation measures are installed. For the existing density
  use the density of the thickest existing insulation.
  **************************************************************/

  if (fsWngExistMinDepth > fsWngExistLooseDepth)
    fDensExist = mir->fDensExistBatInsul;
  else
    fDensExist = DENSITY_EXIST_FG_INSUL;

  if (mir->flgRetrofits[M_CMS_BELLY_CELLULOSE_LOOSE_INSL]) {
    fWngMinDepth = fsWngExistMinDepth * fDensExist / mdi->key.density_of_loose_cellulose_insulation;
    fWngLooseDepth = fsWngExistLooseDepth * fDensExist / mdi->key.density_of_loose_cellulose_insulation;
    fWngCelDepth = mdi->flr.wing_loose_insl;
  } else if (mir->flgRetrofits[M_CMS_BELLY_FIBERGLASS_LOOSE_INSL]) {
    if (mdi->key.density_of_loose_fiberglass_insulation > fDensExist)
      fWngMinDepth = fsWngExistMinDepth * fDensExist / mdi->key.density_of_loose_fiberglass_insulation;
  }

  fWngInsulDepth = fWngMinDepth + fWngLooseDepth + fWngCelDepth;
  if (mir->flgWhichPass == BASE_CASE)
    mir->fWingInsDepth = fWngInsulDepth;

  /********************************************************
  If the floor joists are perpendicular to the direction
  of the batt/blanket insulation, then the insulating
  value of the insulation is reduced by 12.5%.
  *********************************************************/

  //  if( iJoistLengthWise != iInsulLengthWise )
  if (mdi->flr.wing_insl_location == ATTACHEDUNDER)
    fWngMinDepth *= 0.875;

  if (mir->flgRetrofits[M_CMS_BELLY_FIBERGLASS_LOOSE_INSL] || mir->flgRetrofits[M_CMS_BELLY_CELLULOSE_LOOSE_INSL]) {
    fRWngInsulation =
        mir->fRinCompBatInsul * fWngMinDepth + mir->fRinFGCompressed * fWngLooseDepth + mir->fRinCompCelInsul * fWngCelDepth;
  } else {
    fRWngInsulation = mdi->key.batt_blanket_insulation_r_value_per_inch * fWngMinDepth +
                      mdi->key.loose_insulation_r_value_per_inch * fWngLooseDepth +
                      mir->fRinExistCelInsul * fWngCelDepth;
  }

  if ((fWngLooseDepth > 0.0) && (fWngMinDepth <= 0.0))
    /********************
    If there only loose fill insulation on top of the tar wrap,
    it will be attached to the joist.
    ********************/
    iWngInsulAtJoist = 1;

  /* SLF 12/13/94 */
  if (fWngInsulDepth == 0.0) {
    fWngAirSpace = fWngJoistSize;
    if (mir->flgWhichPass == BASE_CASE)
      mir->fWngAirSpace = fWngAirSpace;
  } else if ((fWngJoistSize >= 1.01f * fWngInsulDepth) && iWngInsulAtFlr)
  //  if( (fWngJoistSize >= fWngInsulDepth) && iWngInsulAtFlr )
  /*****************************
  The bottom of the floor wing joist sticks out below the insulation.
  This section of the joist will not be treated, thus the size
  of the joist is made equal to the depth of the insulation and
  the R-value of the joist is adjusted accordingly.
  *******************************
  The above assumption reversed 1/08 MBG. Assume that rodent barrier
  is attached to bottom of joist in wing section creating separate
  heat flow path from cavity.
  ******************************
  Since the bandjoist is covered up to the bottom of the floor, the
  U-value of the bandjoist is not considered in the calculation
  of the total floor UA value.
  *******************************/
  {

    fWngAirSpace = fWngJoistSize - fWngInsulDepth;
    if (mir->flgWhichPass == BASE_CASE)
      mir->fWngAirSpace = fWngAirSpace;

    /*******************
    The questioned statements below are replaced by the statements
    immediately above which allow insulating a wing section
    eventhough the insulatable space is between existing insulation
    attached to the floor (seldom seen) and the tar wrap at the bottom
    of the joists.
    MBG 3/38/03
    *******************/

    //      fWngJoistSize = fWngInsulDepth;  (See 1/08 MBG comment above)

    fRWngFraming = (float)(fWngJoistSize * RFRAME);

    //      fRBjoist = 0;
    //      Modify above to eliminate discontinuity as insulation level
    //      attached to floor goes to 0.  1/08 MBG
    fRBjoist += fRWngInsulation;

    /* SLF 12/13/94 ?? This may not be correct. */
    //      if( mir->flgWhichPass == BASE_CASE )
    //          mir->fWngAirSpace = 0.0;
  } else if ((fWngJoistSize > fWngInsulDepth) && iWngInsulAtJoist)
  //  if( (fWngJoistSize > fWngInsulDepth) && iWngInsulAtJoist )
  /************************************
  There is an air space between the insulation and the floor in
  the joist cavity.  Part of the bandjoist is also uninsulated so
  that an additional calculation must be made for the bandjoist
  U-value.  The air space thickness is calculated here and the
  corresponding R-value is calculated later.

  The air space dimension, fWngAirSpace, is also considered
  the bandjoist width for the area calculations.
  *************************************/
  {
    fWngAirSpace = fWngJoistSize - fWngInsulDepth;
    if (mir->flgWhichPass == BASE_CASE)
      mir->fWngAirSpace = fWngAirSpace;
    //      fRBjoist = fWngAirSpace * RFRAME;
  } else if (iWngInsulUnderJoist)
  /*************************
  Insulation bulges into cavity to about 1/2 its depth. Adjust
  the cavity air space accordingly. 1/08
  *************************/
  {
    fWngAirSpace = fWngJoistSize - 0.5f * fWngInsulDepth;
    if (mir->flgWhichPass == BASE_CASE)
      mir->fWngAirSpace = fWngAirSpace;
    //      fRBjoist = fWngAirSpace * RFRAME;
  }

  /****************************
  Floor and Bandjoist Areas
  ****************************/
  //  fAFloor = fWidth * fLength;
  //  fATotal = (( fWidth  + (2.0 * ( fWngAirSpace / 12.0 )) ) *
  //                ( fLength + (2.0 * ( fWngAirSpace / 12.0 )) )) - fAreaWHC;
  //  if( fWngAirSpace != 0 )
  //      fABjoist = fATotal - fAFloor - fAreaWHC -
  //                    ( fLengthWHC * (fWngAirSpace / 12.0) );
  //  else
  //      fABjoist = 0.0;

  /******************************
  More accurate and straight forward area computations  MBG 1/08
  ******************************/
  fAFloor = fWidth * fLength - fAreaWHC;
  if (iWngInsulUnderJoist)
    fABjoist = 2.0f * fWngJoistSize / 12.0f * (fWidth + fLength + fLengthWHC);
  /****************************
  If insulation attached under joist, despite insulation bulging into
  cavity, the band joist is totally exposed.  1/08
  *****************************/
  else if (fWngAirSpace != 0)
    fABjoist = 2.0f * fWngAirSpace / 12.0f * (fWidth + fLength + fLengthWHC);
  else
    fABjoist = 0.0;
  fATotal = fAFloor + fABjoist;

  /*********************************
  Floor U-value Calculations  (season dependent)
  *********************************/

  iSummer = 1;
  for (i = 0; i < 2; i++) {
    //fRStillAirBjoist = 0.68f;

    /************************
    R-values dependent on season for bandjoist, wing, & belly calculations.
    Values changed 3/08 to represent average not design conditions and to
      reflect that differences in heat flow up and down cancel on upper
      and lower surfaces.
    ************************/
    if (iSummer) {
      fRIntFlr = mdi->key.interior_floor_r_value_summer;
      //fRStillAirFlr = 0.61f; // No longer used.
      if (mdi->flr.skirt == YES)
        //              fROutsideAir = 0.61f;
        fROutsideAir = 0.765f;
      else
        //              fROutsideAir = 0.25f;
        fROutsideAir = 0.488f;
    } else /* Winter */
    {
      fRIntFlr = mdi->key.interior_floor_r_value_winter;
      //fRStillAirFlr = 0.92f; // No longer used.
      if (mdi->flr.skirt == YES)
        //              fROutsideAir = 0.92f;
        fROutsideAir = 0.765f;
      else
        //              fROutsideAir = 0.17f;
        fROutsideAir = 0.488f;
    }

    /***********************************
    If there is an air space between the insulation and the floor,
    fRBellyAirSpace needs to be calculated.
    **************************************/
    if (fBellyAirSpaceCavity > 0.0) {
      if (iSummer)
        iSeason = 1;
      else /* Winter */
        iSeason = 2;

      fRBellyAirCavity = air_space_r_value_flr(fBellyAirSpaceCavity, iSeason);
      if (mir->flgWhichPass == BASE_CASE) {
        mir->fBellyAirSpace = fBellyAirSpaceCavity;
        if (cmds.debug_level & D_MHEA_ENERGY_DETAIL) {
          fprintf(stderr, "mir->fBellyAirSpace 1:%8.3f\n", mir->fBellyAirSpace);
        }
      }

    } else {
      if (mir->flgWhichPass == BASE_CASE){
        mir->fBellyAirSpace = 0.0;
        if (cmds.debug_level & D_MHEA_ENERGY_DETAIL) {
          fprintf(stderr, "mir->fBellyAirSpace 2:%8.3f\n", mir->fBellyAirSpace);
        }
      }

    }
    if (fBellyAirSpaceJoist > 0.0) {
      if (iSummer)
        iSeason = 1;
      else /* Winter */
        iSeason = 2;

      fRBellyAirJoist = air_space_r_value_flr(fBellyAirSpaceJoist, iSeason);
    }

    /***********************
    For draped insulation, the U-value calculation has
    additional parameters, so the U-value calculation for
    a draped belly is separate from a non-draped belly.
    **********************/

    if (iBellyInsulDraped) {

      // replaced with a single calculation that
      // uses derating factors based on belly condition.  Note that
      // wing Rvalue is not effected. (MJF 5/1/02)

      fUFloorBelly =
          (float)(((1.0 / (fRIntFlr + fRBellyFraming + fRBellyAirJoist + fRBellyLooseJoist + fRBellyMineral * fBellyCondDerate +
                           fROutsideAir)) *
                   JOISTFACTOR) +
                  ((1.0 / (fRIntFlr + fRBellyAirCavity + fRBellyLooseCavity + fRBellyMineral * fBellyCondDerate + fROutsideAir)) *
                   CAVITYFACTOR_FLR));

    } else
      /************************************
      The insulation is attached to the floor, attached to
      the joists, or there is no insulation at all.
      ***********************************/
      fUFloorBelly = (float)(((1.0 / (fRIntFlr + fRBellyFraming + fROutsideAir)) * JOISTFACTOR) +
                             ((1.0 / (fRIntFlr + fRBellyAirCavity + fRBellyInsulation * fBellyCondDerate + fROutsideAir)) *
                              CAVITYFACTOR_FLR));

    if (cmds.debug_level & D_MHEA_ENERGY_DETAIL) {
      sprintf(mir->sMsg, "RBellyAir, RBellyIns = %8.2f %8.2f \n", fRBellyAirCavity, fRBellyInsulation);
      fprintf(stderr, "%s", mir->sMsg);
    }

    /***********************************
    If there is an air space between the insulation and the floor,
    fRWngAirSpace needs to be calculated.
    **************************************/
    if (fWngAirSpace > 0.0) {
      if (iSummer)
        iSeason = 1;
      else /* Winter */
        iSeason = 2;

      fRWngAirSpace = air_space_r_value_flr(fWngAirSpace, iSeason);
    }

    // Added the belly condition de-rate factor to the
    // wing section as well assuming that the condition of the
    // wing insulation follows that of the belly.  MJF 4/1/03

    fUFloorWng =
        (float)(((1.0 / (fRIntFlr + fRWngFraming + fROutsideAir)) * JOISTFACTOR) +
                ((1.0 / (fRIntFlr + fRWngInsulation * fBellyCondDerate + fRWngAirSpace + fROutsideAir)) * CAVITYFACTOR_FLR));

    if (fRBjoist > 0.0)
      /*********************
      Part of the bandjoist is exposed, thus the U-value of the exposed
      bandjoist area needs to be included in the total floor UA value.
      **********************/
      //          fUBjoist = 1.0 / ( fRIntFlr + fRStillAirFlr + fRWngAirSpace +
      //                                 fRStillAirBjoist + fRBjoist + fROutsideAir );
      fUBjoist = (float)(1.0 / (fRIntFlr + fRWngAirSpace + fRBjoist + fROutsideAir));
    else
      /*******************
      Do not include fUBjoist in the total floor UA calcs.
      ********************/
      fUBjoist = 0.0;

    /********************
    UFloor = component U-values for the wing, belly, & bandjoist regions.
    Assume 50/50 weighting of wing and belly U-values for the floor.
    *********************/

    ASSERT(fATotal != 0, sprintf(msg, "Assertion Failure"));

    fUFloor = (float)((fUFloorWng * (fAFloor / (2.0 * fATotal))) + (fUFloorBelly * (fAFloor / (2.0 * fATotal))) +
                      (fUBjoist * (fABjoist / fATotal)));

    /*******************
    UA total = U for the entire floor times the total floor area.
    First modify U to reflect cupboards, etc. by adding fRAdded_floor Rs.
    4/08
    ********************/
    ASSERT(fUFloor != 0, sprintf(msg, "Assertion Failure"));
    fUFloor = 1.0f / (1.0f / fUFloor + fRAdded_floor);
    fUATotal = fUFloor * fATotal;

    /***********************
    Assign the pointers the fUATotal values for summer and winter.
    ************************/
    if (iSummer)
      *fUA_FLR_S = fUATotal;
    else /* Winter */
      *fUA_FLR_W = fUATotal;

    iSummer = 0; /* Winter */

  } // End of for( i = 0; ... ) loop.

  if (cmds.debug_level & D_MHEA_ENERGY_DETAIL) {
    sprintf(mir->sMsg, "UWng, UBelly, UBandJ = %8.4f %8.4f %8.4f \n", fUFloorWng, fUFloorBelly, fUBjoist);
    fprintf(stderr, "%s", mir->sMsg);
  }

  /*********************************
  only look at the addition if there is a non-zero
  floor area  MJF Debug 1/99
  *********************************/

  if (mdi->afl.length * mdi->afl.width == 0)
    return;

  /********************************
  This code added 1/99 -- previous calculations
  did not reset the width and length variables and thus
  computed the addition UA based on the area for the WHOLE HOME!
  MJF 1/99
  *********************************/
  fWidth = mdi->afl.width;
  fLength = mdi->afl.length;

  /*****************************
  Code added 3/05 to handle slab-on-grade addition floor.
  UA value taken from NEAT calculations.
  ******************************/

  if (mdi->afl.add_floor_type == SLABONGRADE) {
    float fUSlab = 0.23f;
    // U-Value of slab,[Btu/hr-F] assumed insulated, else = 0.424
    float fCSoil = 0.86f;
    // Soil thermal conductivity [Btu/hr-sqft-F]
    float fPerimeter;               // Slab perimeter [ft]
    float fAreaSlab;                // Area of slab [sqft]
    float fSlabThickness = 0.4167f; // Slab thickness [ft]
    float fXsl;                     // Dimensionless factor
    float fFcsl;                    // Shape correction factor
    float fFact1, fFact2, fFact3;   // Temporary interim values
    float fRsl;                     // Modified soil thermal resistance [sqft-hr-F/Btu]
    float fUAFloor;                 // Effective UA value of slab

    fAreaSlab = fWidth * fLength;
    fPerimeter = 2.0f * (fWidth + fLength);
    ASSERT(fPerimeter != 0.0f, sprintf(msg, "Assertion Failure"));
    fXsl = 16.0f * fAreaSlab / fPerimeter / fPerimeter;
    fFcsl = 0.0904f + 1.1115f * fXsl - 0.2038f * fXsl * fXsl;
    ASSERT(fCSoil != 0.0f, sprintf(msg, "Assertion Failure"));
    ASSERT(fUSlab != 0.0f, sprintf(msg, "Assertion Failure"));
    fFact1 = fPerimeter * fFcsl / fCSoil;
    fFact2 = (float)log((double)(fCSoil / fPerimeter / fUSlab));
    fFact3 = fSlabThickness / fPerimeter;
    fRsl = fFact1 * (0.1208f + 0.0195f * fFact2 + 0.0011f * fFact2 * fFact2 + 0.2347f * fFact3 - 20.336f * fFact3 * fFact3 -
                     0.1421f * fFact3 * fFact2);

    fUAFloor = fAreaSlab / (fRsl + 1.0f / fUSlab);
    *fUA_FLR_S += fUAFloor;
    *fUA_FLR_W += fUAFloor;
    return;
  }

  //  End of slab-on-grade UA calculations

  /**********************************
  General Floor Information
  ************************************/

  /**************************
  Addition U-value Calculations  (season-independent)
  **************************/

  // Use the switch instead to avoid the uninitialized variable warning from compiler
  // if( mdi->afl.joist_size == JS_TWOXFOUR )
  //    fAddnJoistSize = TWOXFOUR;
  // else if( mdi->afl.joist_size == JS_TWOXSIX )
  //    fAddnJoistSize = TWOXSIX;
  // else if( mdi->afl.joist_size == JS_TWOXEIGHT )
  //    fAddnJoistSize = TWOXEIGHT;

  switch (mdi->afl.joist_size) {
  case JS_TWOXFOUR:
    fAddnJoistSize = TWOXFOUR;
    break;
  case JS_TWOXSIX:
    fAddnJoistSize = TWOXSIX;
    break;
  case JS_TWOXEIGHT:
    fAddnJoistSize = TWOXEIGHT;
    break;
  default:
    fAddnJoistSize = TWOXSIX;
  }

  /*******************
  The bandjoist height equals the addition joist size only when the addition
  section is uninsulated.  Otherwise the bandjoist height equals the
  air space height if insulation is attached to joist.  If insulation
  is attached to flooring, then there is no uninsulated bandjoist height
  (see other occurances of fRBjoistAddn).

  Validation Note: This calculation is from PNL and not EEDO.

  Above modified 1/08 to model horizontal heat flow through band joist
  from crawl space. Insulation attached to the floor effects the overall
  band joist R-value while insulation between the joists at the bottom
  of the joists effects the area through which the heat flows.
  *******************/
  //fBjoistAddnHeight = fAddnJoistSize;
  //  fRBjoistAddn    = fBjoistAddnHeight * RFRAME;
  fRBjoistAddn = (float)(fBjoistThickness * RFRAME);

  fRAddnFraming = (float)(fAddnJoistSize * RFRAME);
  fAddnMinDepth = mdi->afl.mineral_insl;
  fAddnLooseDepth = mdi->afl.loose_insl;

  /******************************
  Check to see if addition floor insulation measure has been installed
  so that compressed R/in can be used.
  ******************************/

  iflgFlrMeas = FALSE;
  if (mir->flgRetrofits[M_CMS_BELLY_FIBERGLASS_LOOSE_INSL_ADD] || mir->flgRetrofits[M_CMS_BELLY_CELLULOSE_LOOSE_INSL_ADD])
    iflgFlrMeas = TRUE;

  /********************************
  The insulation is not draped and there may be layering of the
  loose and batt insulation.  The insulation is attached to the
  floor, attached to or under the joists, or there is no
  insulation at all.
  *******************************/
  if (mdi->afl.insl_location == ATTACHEDFLOOR)
    iAddnInsulAtFlr = 1;
  else if (mdi->afl.insl_location == BETWEENJOISTS)
    iAddnInsulAtJoist = 1;
  else if (mdi->afl.insl_location == ATTACHEDUNDER)
    iAddnInsulUnderJoist = 1;

  if (mdi->afl.mineral_insl > 0.0) {
    fAddnInsulDepth = fAddnMinDepth;
    fRinchFlr = mdi->key.batt_blanket_insulation_r_value_per_inch;
    if (iflgFlrMeas == TRUE)
      fRinchFlr = mir->fRinCompBatInsul;
    fRAddnInsulation = fRinchFlr * fAddnMinDepth;

    /***************************
    If the floor joists are perpendicular to the direction
    of the batt/blanket insulation, then the insulating
    value of the insulation is reduced by 12.5%.
    ***************************/
    //      if( iJoistLengthWise != iInsulLengthWise )
    if (mdi->afl.insl_location == ATTACHEDUNDER)
      fRAddnInsulation *= 0.875;
  }
  if (mdi->afl.loose_insl > 0.0) {
    fAddnInsulDepth += fAddnLooseDepth;
    if (mir->flgRetrofits[M_CMS_BELLY_FIBERGLASS_LOOSE_INSL_ADD])
      fRInsLoose = mir->fRinFGCompressed;
    else if (mir->flgRetrofits[M_CMS_BELLY_CELLULOSE_LOOSE_INSL_ADD])
      fRInsLoose = mir->fRinCompCelInsul;
    else
      fRInsLoose = mdi->key.loose_insulation_r_value_per_inch;
    fRAddnInsulation += fRInsLoose * fAddnLooseDepth;
  }
  if (mir->flgWhichPass == BASE_CASE)
    mir->fAFloorInsDepth = fAddnInsulDepth;

  /********************
  If there is only loose fill insulation on top of the tar wrap,
  it will be attached to the joist.
  ********************/
  if ((fAddnLooseDepth > 0.0) && (fAddnMinDepth <= 0.0))
    iAddnInsulAtJoist = 1;

  /*****************************
  The bottom of the floor addition joist sticks out below the insulation.
  This section of the joist will not be treated, thus the size
  of the joist is made equal to the depth of the insulation and
  the R-value of the joist is adjusted accordingly.  Also, since
  the bandjoist is covered up to the bottom of the floor, the
  U-value of the bandjoist is not considered in the calculation
  of the total floor UA value.

  The above is NOT modified as in the case of the wing section of the MH
  due to the absence of a rodent barrier at the bottom of the joists in
  the Addition.
  *******************************/
  if ((fAddnJoistSize >= fAddnInsulDepth) && iAddnInsulAtFlr) {
    fAddnJoistSize = fAddnInsulDepth;
    fRAddnFraming = (float)(fAddnJoistSize * RFRAME);
    fRBjoistAddn = 0;
  }

  /************************************
  There is an air space between the insulation and the floor in
  the joist cavity.  Part of the bandjoist is also uninsulated so
  that an additional calculation must be made for the bandjoist
  U-value.  The air space thickness is calculated here and the
  corresponding R-value is calculated later.

  The air space dimension, fAddnAirSpace, is also considered
  the bandjoist width for the area calculations.
  *************************************/
  if (fAddnJoistSize > fAddnInsulDepth) {
    if (iAddnInsulAtJoist) {
      fAddnAirSpace = fAddnJoistSize - fAddnInsulDepth;
    }
    //        fRBjoistAddn = fAddnAirSpace * RFRAME; }
    else if (iAddnInsulUnderJoist) {
      /*************************
      Insulation bulges into cavity to about 1/2 its depth. Adjust
      the cavity air space accordingly. 1/08
      *************************/
      fAddnAirSpace = fAddnJoistSize - 0.5f * fAddnInsulDepth;
    }
  }

  /****************************
  Floor and Bandjoist Areas
  ****************************/
  //  fAFloorAddn = fWidth * fLength;
  //  fATotalAddn = ( fWidth  + ( fAddnAirSpace / 12.0 ) ) *
  //                   ( fLength + (2.0 * ( fAddnAirSpace / 12.0 )) );
  //  if( fAddnAirSpace != 0 )
  //      fABjoistAddn = fATotalAddn - fAFloorAddn;
  //  else
  //      fABjoistAddn = 0.0;

  /******************************
  More accurate and straight forward area computations  MBG 1/08
  Retain assumption that one long side of addition is against MH,
  eliminating heat loss through the band joist along that side.
  ******************************/
  fAFloorAddn = fWidth * fLength;

  if (fAddnAirSpace != 0) {
    if (iAddnInsulUnderJoist)
      fABjoistAddn = fAddnJoistSize / 12.0f * (2.0f * fWidth + fLength);
    /****************************
    If insulation attached under joist, despite insulation bulging into
    cavity, the band joist is totally exposed.  1/08
    *****************************/

    else
      fABjoistAddn = fAddnAirSpace / 12.0f * (2.0f * fWidth + fLength);
  } else
    fABjoistAddn = 0.0;

  fATotalAddn = fAFloorAddn + fABjoistAddn;

  /*********************************
  Floor U-value Calculations  (season dependent)
  *********************************/
  iSummer = 1;
  for (i = 0; i < 2; i++) {
    //fRStillAirBjoistAddn = 0.68f;

    /************************
    R-values dependent on season for addition & bandjoist calculations.
    Values changed 3/08 to represent average not design conditions and to
      reflect that differences in heat flow up and down cancel on upper
      and lower surfaces.
    ************************/
    if (iSummer)
      fRIntFlr = mdi->key.interior_floor_r_value_summer;
    else /* Winter */
      fRIntFlr = mdi->key.interior_floor_r_value_winter;

    if (mdi->afl.add_floor_type != EXPOSEDFLOOR)
      fROutsideAir = 0.765f;
    else
      fROutsideAir = 0.488f;

    /***********************************
    If there is an air space between the insulation and the floor,
    fRAddnAirSpace needs to be calculated.
    **************************************/
    if (fAddnAirSpace > 0.0) {
      if (iSummer)
        iSeason = 1;
      else /* Winter */
        iSeason = 2;

      fRAddnAirSpace = air_space_r_value_flr(fAddnAirSpace, iSeason);
    }

    fUFloorAddn = (float)(((1.0 / (fRIntFlr + fRAddnFraming + fROutsideAir)) * JOISTFACTOR) +
                          ((1.0 / (fRIntFlr + fRAddnInsulation + fRAddnAirSpace + fROutsideAir)) * CAVITYFACTOR_FLR));

    if (fRBjoistAddn > 0.0)
      /*********************
      Part of the bandjoist is exposed, thus the U-value of the exposed
      bandjoist area needs to be included in the total floor UA value.
      **********************/
      //          fUBjoistAddn = 1.0 / ( fRIntFlr + fRStillAirFlr + fRAddnAirSpace +
      //                             fRStillAirBjoistAddn + fRBjoistAddn + fROutsideAir );
      fUBjoistAddn = (float)(1.0 / (fRIntFlr + fRAddnAirSpace + +fRBjoistAddn + fROutsideAir));
    else
      /*******************
      Do not include fUBjoistAddn in the total floor UA calcs.
      ********************/
      fUBjoistAddn = 0.0;

    /********************
    UFloor = component U-values for the addition & bandjoist regions.
    *********************/

    ASSERT(fATotalAddn != 0, sprintf(msg, "Assertion Failure"));

    fUFloorTotalAddn = (fUFloorAddn * (fAFloorAddn / fATotalAddn)) + (fUBjoistAddn * (fABjoistAddn / fATotalAddn));

    /*******************
    UA total = U for the entire floor times the total floor area.
    First modify U to reflect cupboards, etc. by adding fRAdded_floor Rs.
    4/08
    ********************/
    ASSERT(fUFloorTotalAddn != 0, sprintf(msg, "Assertion Failure"));
    fUFloorTotalAddn = 1.0f / (1.0f / fUFloorTotalAddn + fRAdded_floor);
    fUATotalAddn = fUFloorTotalAddn * fATotalAddn;

    /***********************
    Assign the pointers the fUATotalAddn values for summer and winter.
    ************************/
    if (iSummer)
      *fUA_FLR_S += fUATotalAddn;
    else /* Winter */
      *fUA_FLR_W += fUATotalAddn;

    iSummer = 0; /* Winter */

  } // End of for( i = 0; ... ) loop.

  return;

} // End of ua_floor() user function.
