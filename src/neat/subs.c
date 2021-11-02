/***************************************************************************
* MODULE:       subs.c            CREATED:      February 24, 1999
*
* AUTHOR:       Mike Gettings       Oak Ridge National Laboratory
*           Mark Fishbaugher    Fishbaugher and Associates
*
* MDESC:        set of functions called from the neat.c module
****************************************************************************/
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wa_engine.h"

// global variables static to this module

// extern float wall_insrval[];
// extern int uatt_instype[];
extern int dbgflg;
// extern char cityname[];

// all variables starting with bs... are used to backup the basecase

static float bsrcuvlclc[NEAT_MAX_UAS], bsrcinsclinp[NEAT_MAX_UAS], bsrcuvlclj[NEAT_MAX_UAS], bsflrcavr[NEAT_MAX_FND],
    bssbuvalfl[NEAT_MAX_FND], bswlucavty[NEAT_MAX_WAL], bswnuvalue[NEAT_MAX_WIN], bswnsgfsum[NEAT_MAX_WIN],
    bswnsgfwin[NEAT_MAX_WIN], bsrdbrdldc, bsrdbrdldh, bsshade[2][NEAT_MAX_WIN], bsprsys_seaseff, bswnleakcoef[NEAT_MAX_WIN],
    bswncfm[NEAT_MAX_WIN][MONTHS + 1], bswncfmtot[MONTHS + 1], bssysht_seaseff, bswnlatload[MONTHS + 1], bshtgmengy[MONTHS + 1],
    bsclgmengy[MONTHS + 1], bsacseer[NEAT_MAX_CLG], bsseer, bsblc[13], bsfreeheat_day[13], bsfreeheat_night[13], bsteffn[13],
    bsuavwl[NEAT_MAX_WAL], bsuavrc[NEAT_MAX_UAS], bsuaeff[NEAT_MAX_FND], bsuabsmt[NEAT_MAX_FND], bsnight_setback_temperature[13],
    bsuasill[NEAT_MAX_FND], bswlinsrval[NEAT_MAX_WAL], bsuainsdpth[NEAT_MAX_UAS], bssbflrinsr[NEAT_MAX_FND],
    bssbwlinsr[NEAT_MAX_FND], bssbuvalwla[NEAT_MAX_FND], bssbuvalwlb[NEAT_MAX_FND], bsFductht, bsperac, bsuainsrv[NEAT_MAX_UAS],
    bstotsol[MONTHS + 1], bsuarcabsorp[NEAT_MAX_UAS], bswnsunscrn[2][NEAT_MAX_WIN], bsuvalwlbtot[NEAT_MAX_FND];
static int bswnglazg[NEAT_MAX_WIN], bsuainstype[NEAT_MAX_UAS], htg0fueltype;
static float bsdruvalue[NEAT_MAX_DOR], bsdrleakcoef[NEAT_MAX_DOR], bsdrcfm[NEAT_MAX_DOR][MONTHS + 1], bsdrcfmtot[MONTHS + 1],
    bsdrlatload[MONTHS + 1]; // Added V89, 12/11
                             /* static float bsnpv[MAXECMS]; */
                             /* static int bsindex_by_sir[MAXECMS]; */

char *comp_group_name(enum MEASURE_COMPONENT_GROUP_TYPE type) {
  switch (type)  {
  case MG_NONE:
    return "";
  case MG_WALL:
    return "Wall";
  case MG_FOUNDATION_FLOOR:
    return "FoundationFloor";
  case MG_FOUNDATION_SILL:
    return "FoundationSill";
  case MG_FOUNDATION_WALL:
    return "FoundationWall";
  case MG_ATTIC_RESTRICTED:
    return "UnfinishedAtticRestricted";
  case MG_ATTIC_UNRESTRICTED:
    return "UnfinishedAtticUnrestricted";
  case MG_ATTIC_SPECIFIED_REQUIRED:
    return "UnfinishedAtticSpecified";
  case MG_ATTIC_KNEEWALL:
    return "FinishedAtticKneewall";
  case MG_ATTIC_COLLAR_BEAM_RESTRICTED:
    return "FinishedAtticCollarBeamRestricted";
  case MG_ATTIC_COLLAR_BEAM_UNRESTRICTED:
    return "FinishedAtticCollarBeamUnRestricted";
  case MG_ATTIC_COLLAR_BEAM_SPECIFIED_REQUIRED:
    return "FinishedAtticCollarBeamSpecified";
  case MG_ATTIC_ROOF_RAFTER_RESTRICTED:
    return "FinishedAtticRoofRafterRestricted";
  case MG_ATTIC_ROOF_RAFTER_UNRESTRICTED:
    return "FinishedAtticRoofRafterUnRestricted";
  case MG_ATTIC_ROOF_RAFTER_SPECIFIED_REQUIRED:
    return "FinishedAtticRoofRafterSpecified";
  case MG_ATTIC_OUTER_CEILING_JOIST_RESTRICTED:
    return "FinishedAtticOuterCeilingJoistRestricted";
  case MG_ATTIC_OUTER_CEILING_JOIST_UNRESTRICTED:
    return "FinishedAtticCeilingJoistUnRestricted";
  case MG_ATTIC_OUTER_CEILING_JOIST_SPECIFIED_REQUIRED:
    return "FinishedAtticCeilingJoistSpecified";
  default:
    ASSERT(FALSE, sprintf(msg, "Unrecognized measure component grouping type: %d", type));
  }
}

char *comp_group_name_short(enum MEASURE_COMPONENT_GROUP_TYPE type) {
  switch (type)  {
  case MG_NONE:
    return "";
  case MG_WALL:
    return "WAL";
  case MG_FOUNDATION_FLOOR:
    return "FFL";
  case MG_FOUNDATION_SILL:
    return "FSL";
  case MG_FOUNDATION_WALL:
    return "FWL";
  case MG_ATTIC_RESTRICTED:
    return "UAR";
  case MG_ATTIC_UNRESTRICTED:
    return "UAU";
  case MG_ATTIC_SPECIFIED_REQUIRED:
    return "UAS";
  case MG_ATTIC_KNEEWALL:
    return "AKW";
  case MG_ATTIC_COLLAR_BEAM_RESTRICTED:
    return "ACR";
  case MG_ATTIC_COLLAR_BEAM_UNRESTRICTED:
    return "ACU";
  case MG_ATTIC_COLLAR_BEAM_SPECIFIED_REQUIRED:
    return "ACR";
  case MG_ATTIC_ROOF_RAFTER_RESTRICTED:
    return "ARR";
  case MG_ATTIC_ROOF_RAFTER_UNRESTRICTED:
    return "ARU";
  case MG_ATTIC_ROOF_RAFTER_SPECIFIED_REQUIRED:
    return "ARS";
  case MG_ATTIC_OUTER_CEILING_JOIST_RESTRICTED:
    return "OJR";
  case MG_ATTIC_OUTER_CEILING_JOIST_UNRESTRICTED:
    return "OJU";
  case MG_ATTIC_OUTER_CEILING_JOIST_SPECIFIED_REQUIRED:
    return "OJS";
  default:
    ASSERT(FALSE, sprintf(msg, "Unrecognized measure component grouping type: %d", type));
  }
}

/*******************************************
 * Function name orientation_index
 * Date:  3/17/2019
 * Author: MJF
 *
 * Description:  The orientation is used as an array index in a certain
 * order, this routine converts the orientation enumeration
 * to the solar orientation index used in arrays
 * ***************************************/
enum SOLAR_ORIENTATION solar_orientation(enum ORIENTATION orient_enum) {
  switch (orient_enum) {
  case NORTH:
    return SOLAR_NORTH;
  case EAST:
    return SOLAR_EAST;
  case SOUTH:
    return SOLAR_SOUTH;
  case WEST:
    return SOLAR_WEST;
  default: // fall through to assert
    break;
  }
  ASSERT(FALSE, sprintf(msg, "Unrecognized orientation: %d", orient_enum));
}

/*******************************************
 * Function name wall_stud_depth
 * Date:  3/16/2019
 * Author: MJF
 *
 * Description:  The actual stud depth in inches for wall[i]
 * ***************************************/
float wall_stud_depth(int wall_i) {
  if (wall_i < ndi->num_wal) {
    switch (ndi->wal[wall_i].stud_size) // wall framing thickness
    {
    case SS_TWOXTWO:
      return 1.5;
      break;
    case SS_TWOXTHREE:
      return 2.5;
      break;
    case SS_TWOXFOUR:
      return 3.5;
      break;
    case SS_TWOXSIX:
      return 5.5;
      break;
    case SS_TWOXEIGHT:
      return 7.5;
      break;
    default:
      return 3.5;
      break;
    }
  }
  return 3.5;
}

char *wall_stud_size_name(enum STUD_SIZE stud_size) {
  switch (stud_size)  {
  case SS_TWOXTWO:
    return "2x2";
  case SS_TWOXTHREE:
    return "2x3";
  case SS_TWOXFOUR:
    return "2x4";
  case SS_TWOXSIX:
    return "2x6";
  case SS_TWOXEIGHT:
    return "2x8";
  default:
    ASSERT(FALSE, sprintf(msg, "Unrecognized stud size: %d", stud_size));
  }
}

int wall_add_insulation_material(enum WALLADDINSTYPE  ins_type) {
  switch (ins_type) {
  case AW_CELLULOSE_BLOWN:
    return N_MAT_WALL_INSULATION_CELL;
  case AW_TYPE2:
    return N_MAT_WALL_INSULATION_UT2;
  case AW_TYPE3:
    return N_MAT_WALL_INSULATION_UT3;
  case AW_TYPE4:
    return N_MAT_WALL_INSULATION_UT4;
  case AW_TYPE5:
    return N_MAT_WALL_INSULATION_UT5;
  case AW_TYPE6:
    return N_MAT_WALL_INSULATION_UT6;
  case AW_NONE:
  default:
    ASSERT(FALSE, sprintf(msg, "Unrecognized wall add insulation type: %d", ins_type));
  }
}

int attic_add_insulation_material(enum ATTICADDINSTYPE ins_type, int cms_index) {
  switch(ins_type) {
  case AA_CELLULOSE_BLOWN:
    switch(cms_index) {
    case N_CMS_ATTIC_INSULATION_R11:
      return N_MAT_ATTIC_INSULATION_R11_CELL;
      break;
    case N_CMS_ATTIC_INSULATION_R19:
      return N_MAT_ATTIC_INSULATION_R19_CELL;
      break;
    case N_CMS_ATTIC_INSULATION_R30:
      return N_MAT_ATTIC_INSULATION_R30_CELL;
      break;
    case N_CMS_ATTIC_INSULATION_R38:
      return N_MAT_ATTIC_INSULATION_R38_CELL;
      break;
    case N_CMS_ATTIC_INSULATION_R49:
      return N_MAT_ATTIC_INSULATION_R49_CELL;
      break;
    default:
      ASSERT(0, sprintf(msg, "Unrecognized cms measure: %d", cms_index));
    }
  case AA_FIBERGLASS_BLOWN:
    switch(cms_index) {
    case N_CMS_ATTIC_INSULATION_R11:
      return N_MAT_ATTIC_INSULATION_R11_FIBER;
      break;
    case N_CMS_ATTIC_INSULATION_R19:
      return N_MAT_ATTIC_INSULATION_R19_FIBER;
      break;
    case N_CMS_ATTIC_INSULATION_R30:
      return N_MAT_ATTIC_INSULATION_R30_FIBER;
      break;
    case N_CMS_ATTIC_INSULATION_R38:
      return N_MAT_ATTIC_INSULATION_R38_FIBER;
      break;
    case N_CMS_ATTIC_INSULATION_R49:
      return N_MAT_ATTIC_INSULATION_R49_FIBER;
      break;
    default:
      ASSERT(0, sprintf(msg, "Unrecognized cms measure: %d", cms_index));
    }
  case AA_TYPE3:
    switch(cms_index) {
    case N_CMS_ATTIC_INSULATION_R11:
      return N_MAT_ATTIC_INSULATION_R11_UT3;
      break;
    case N_CMS_ATTIC_INSULATION_R19:
      return N_MAT_ATTIC_INSULATION_R19_UT3;
      break;
    case N_CMS_ATTIC_INSULATION_R30:
      return N_MAT_ATTIC_INSULATION_R30_UT3;
      break;
    case N_CMS_ATTIC_INSULATION_R38:
      return N_MAT_ATTIC_INSULATION_R38_UT3;
      break;
    case N_CMS_ATTIC_INSULATION_R49:
      return N_MAT_ATTIC_INSULATION_R49_UT3;
      break;
    default:
      ASSERT(0, sprintf(msg, "Unrecognized cms measure: %d", cms_index));
    }
  case AA_TYPE4:
    switch(cms_index) {
    case N_CMS_ATTIC_INSULATION_R11:
      return N_MAT_ATTIC_INSULATION_R11_UT4;
      break;
    case N_CMS_ATTIC_INSULATION_R19:
      return N_MAT_ATTIC_INSULATION_R19_UT4;
      break;
    case N_CMS_ATTIC_INSULATION_R30:
      return N_MAT_ATTIC_INSULATION_R30_UT4;
      break;
    case N_CMS_ATTIC_INSULATION_R38:
      return N_MAT_ATTIC_INSULATION_R38_UT4;
      break;
    case N_CMS_ATTIC_INSULATION_R49:
      return N_MAT_ATTIC_INSULATION_R49_UT4;
      break;
    default:
      ASSERT(0, sprintf(msg, "Unrecognized cms measure: %d", cms_index));
    }
  case AA_TYPE5:
    switch(cms_index) {
    case N_CMS_ATTIC_INSULATION_R11:
      return N_MAT_ATTIC_INSULATION_R11_UT5;
      break;
    case N_CMS_ATTIC_INSULATION_R19:
      return N_MAT_ATTIC_INSULATION_R19_UT5;
      break;
    case N_CMS_ATTIC_INSULATION_R30:
      return N_MAT_ATTIC_INSULATION_R30_UT5;
      break;
    case N_CMS_ATTIC_INSULATION_R38:
      return N_MAT_ATTIC_INSULATION_R38_UT5;
      break;
    case N_CMS_ATTIC_INSULATION_R49:
      return N_MAT_ATTIC_INSULATION_R49_UT5;
      break;
    default:
      ASSERT(0, sprintf(msg, "Unrecognized cms measure: %d", cms_index));
    }
  case AA_TYPE6:
    switch(cms_index) {
    case N_CMS_ATTIC_INSULATION_R11:
      return N_MAT_ATTIC_INSULATION_R11_UT6;
      break;
    case N_CMS_ATTIC_INSULATION_R19:
      return N_MAT_ATTIC_INSULATION_R19_UT6;
      break;
    case N_CMS_ATTIC_INSULATION_R30:
      return N_MAT_ATTIC_INSULATION_R30_UT6;
      break;
    case N_CMS_ATTIC_INSULATION_R38:
      return N_MAT_ATTIC_INSULATION_R38_UT6;
      break;
    case N_CMS_ATTIC_INSULATION_R49:
      return N_MAT_ATTIC_INSULATION_R49_UT6;
      break;
    default:
      ASSERT(0, sprintf(msg, "Unrecognized cms measure: %d", cms_index));
    }
  case AA_NONE:
  default:
    ASSERT(0, sprintf(msg, "Unrecognized added attic insulation type: %d", ins_type));
  }
}

int attic_kneewall_add_insulation_material(enum KNEEWALLADDINSTYPE  ins_type) {
  switch (ins_type) {
  case AK_FIBERGLASS_BATT:
    return N_MAT_KNEEWALL_INSULATION;
  case AK_TYPE2:
    return N_MAT_KNEEWALL_INSULATION_UT2;
  case AK_TYPE3:
    return N_MAT_KNEEWALL_INSULATION_UT3;
  case AK_TYPE4:
    return N_MAT_KNEEWALL_INSULATION_UT4;
  case AK_TYPE5:
    return N_MAT_KNEEWALL_INSULATION_UT5;
  case AK_TYPE6:
    return N_MAT_KNEEWALL_INSULATION_UT6;
  case AK_NONE:
  default:
    ASSERT(FALSE, sprintf(msg, "Unrecognized wall add insulation type: %d", ins_type));
  }
}

int floor_add_insulation_material(enum FLOORADDINSTYPE ins_type, int cms_index) {
  switch(ins_type) {
  case AL_FIBERGLASS_BATT:
    switch(cms_index) {
    case N_CMS_FLOOR_INSULATION_R11:
      return N_MAT_FLOOR_INSULATION_R11;
      break;
    case N_CMS_FLOOR_INSULATION_R19:
      return N_MAT_FLOOR_INSULATION_R19;
      break;
    case N_CMS_FLOOR_INSULATION_R30:
      return N_MAT_FLOOR_INSULATION_R30;
      break;
    case N_CMS_FLOOR_INSULATION_R38:
      return N_MAT_FLOOR_INSULATION_R38;
      break;
    default:
      ASSERT(0, sprintf(msg, "Unrecognized cms measure: %d", cms_index));
    }

  case AL_CELLULOSE_BLOWN:
    return N_MAT_FLOOR_FILL_CELLULOSE;
  case AL_FIBERGLASS_BLOWN:
    return N_MAT_FLOOR_FILL_FIBERGLASS;

  case AL_TYPE4:
    switch(cms_index) {
    case N_CMS_FLOOR_INSULATION_R11:
      return N_MAT_FLOOR_INSULATION_R11_UT4;
      break;
    case N_CMS_FLOOR_INSULATION_R19:
      return N_MAT_FLOOR_INSULATION_R19_UT4;
      break;
    case N_CMS_FLOOR_INSULATION_R30:
      return N_MAT_FLOOR_INSULATION_R30_UT4;
      break;
    case N_CMS_FLOOR_INSULATION_R38:
      return N_MAT_FLOOR_INSULATION_R38_UT4;
      break;
    case N_CMS_FILL_FLOOR_CAVITY:
      return N_MAT_FLOOR_FILL_UT4;
      break;
    default:
      ASSERT(0, sprintf(msg, "Unrecognized cms measure: %d", cms_index));
    }
  case AL_TYPE5:
    switch(cms_index) {
    case N_CMS_FLOOR_INSULATION_R11:
      return N_MAT_FLOOR_INSULATION_R11_UT5;
      break;
    case N_CMS_FLOOR_INSULATION_R19:
      return N_MAT_FLOOR_INSULATION_R19_UT5;
      break;
    case N_CMS_FLOOR_INSULATION_R30:
      return N_MAT_FLOOR_INSULATION_R30_UT5;
      break;
    case N_CMS_FLOOR_INSULATION_R38:
      return N_MAT_FLOOR_INSULATION_R38_UT5;
      break;
    case N_CMS_FILL_FLOOR_CAVITY:
      return N_MAT_FLOOR_FILL_UT5;
      break;
    default:
      ASSERT(0, sprintf(msg, "Unrecognized cms measure: %d", cms_index));
    }
  case AL_TYPE6:
    switch(cms_index) {
    case N_CMS_FLOOR_INSULATION_R11:
      return N_MAT_FLOOR_INSULATION_R11_UT6;
      break;
    case N_CMS_FLOOR_INSULATION_R19:
      return N_MAT_FLOOR_INSULATION_R19_UT6;
      break;
    case N_CMS_FLOOR_INSULATION_R30:
      return N_MAT_FLOOR_INSULATION_R30_UT6;
      break;
    case N_CMS_FLOOR_INSULATION_R38:
      return N_MAT_FLOOR_INSULATION_R38_UT6;
      break;
    case N_CMS_FILL_FLOOR_CAVITY:
      return N_MAT_FLOOR_FILL_UT6;
      break;
    default:
      ASSERT(0, sprintf(msg, "Unrecognized cms measure: %d", cms_index));
    }
  case AL_NONE:
  default:
    ASSERT(0, sprintf(msg, "Unrecognized added floor insulation type: %d", ins_type));
  }
}

int foundation_wall_add_insulation_material(enum FOUNDATIONADDINSTYPE ins_type) {
  switch (ins_type) {
  case AF_RIGID_BOARD:
    return N_MAT_FOUNDATION_WALL_INSULATION;
  case AF_TYPE2:
    return N_MAT_FOUNDATION_WALL_INSULATION_UT2;
  case AF_TYPE3:
    return N_MAT_FOUNDATION_WALL_INSULATION_UT3;
  case AF_TYPE4:
    return N_MAT_FOUNDATION_WALL_INSULATION_UT4;
  case AF_TYPE5:
    return N_MAT_FOUNDATION_WALL_INSULATION_UT5;
  case AF_TYPE6:
    return N_MAT_FOUNDATION_WALL_INSULATION_UT6;
  case AF_NONE:
  default:
    ASSERT(FALSE, sprintf(msg, "Unrecognized foundation wall add insulation type: %d", ins_type));
  }
}

int foundation_sill_add_insulation_material(enum SILLADDINSTYPE ins_type) {
  switch (ins_type) {
  case AS_FIBERGLASS_BATT:
    return N_MAT_SILLBOX_INSULATION;
  case AS_TYPE2:
    return N_MAT_SILLBOX_INSULATION_UT2;
  case AS_TYPE3:
    return N_MAT_SILLBOX_INSULATION_UT3;
  case AS_TYPE4:
    return N_MAT_SILLBOX_INSULATION_UT4;
  case AS_TYPE5:
    return N_MAT_SILLBOX_INSULATION_UT5;
  case AS_TYPE6:
    return N_MAT_SILLBOX_INSULATION_UT6;
  case AS_NONE:
  default:
    ASSERT(FALSE, sprintf(msg, "Unrecognized foundation wall add insulation type: %d", ins_type));
  }
}

float foundation_sill_ua_value(int nc) {
  float areasill = ndi->fnd[nc].sill_perimeter * ndi->fnd[nc].joist_height / 12.0f;
  if (ndi->fnd[nc].sill_r > 0.0)
    return  1.0f / (1.0f / SILL_U_VALUE + ndi->fnd[nc].sill_r) * areasill;
  else
    return SILL_U_VALUE * areasill;
}

/*******************************************
* Function name window_u_value
* Date:  3/18/2019
* Author: Gettings/MJF
*
* Description:  The window u values by frame and glazing type
* Originally, this was a two dimensional array that was enumeration
* order dependent.  In this form it is a bit more transparant, and order independent
* From Eng Manual: The following table lists the R-values and solar heat gain coefficients used
* for various types of existing windows. The values are taken mostly from the 2001 ASHRAE
* Handbook of Fundamentals and the Efficient Windows Collaborative (www.efficientwindows.org).
* The u values listed below include both interior and exterior film resistances (ASHRAE only)
* The exterior film resistances have been modified to reflect average rather than design conditions.
*
* MJF Added the Manual J window u-values for the same set of frame and glazing types 3/2019 removing
* a second two dimensional array named wnmju which was indexed in reverse order to the other window u value array.
*
* NOTE the u-values for Manual J Cooling calcs are solely based on glazing, not frame types, hence the repeated values.
* ***************************************/
// clang-format off
float window_u_value(enum FRAME_TYPE frame, enum GLAZINGTYPE glazing, enum LOAD_CALCULATION_TYPE calc) {
  switch (frame) {

  case FT_WOOD_VINYL:
    switch (glazing) {
    case SINGLE:
      switch (calc) { case ASHRAE: return 0.78f; case MANUAL_J_HEAT: return 0.990f; case MANUAL_J_COOL: return 0.8f; } 
    case SINGLE_W_WOOD_STORM:
      switch (calc) { case ASHRAE: return 0.46f; case MANUAL_J_HEAT: return 0.475f; case MANUAL_J_COOL: return 0.4f; } 
    case SINGLE_W_METAL_STORM:
      switch (calc) { case ASHRAE: return 0.61f; case MANUAL_J_HEAT: return 0.475f; case MANUAL_J_COOL: return 0.4f; } 
    case SINGLE_W_BAD_STORM:
      switch (calc) { case ASHRAE: return 0.78f; case MANUAL_J_HEAT: return 0.990f; case MANUAL_J_COOL: return 0.8f; } 
    case DOUBLE_GLAZED:
      switch (calc) { case ASHRAE: return 0.49f; case MANUAL_J_HEAT: return 0.551f; case MANUAL_J_COOL: return 0.4f; } 
    case DOUBLE_GLAZED_LOWE:
      switch (calc) { case ASHRAE: return 0.42f; case MANUAL_J_HEAT: return 0.361f; case MANUAL_J_COOL: return 0.2f; }
    case DOUBLE_GLAZED_W_STORM:
      switch (calc) { 
        case ASHRAE: 
            ASSERT(FALSE, sprintf(msg, "No U-value for glazing type: %d", glazing));
        case MANUAL_J_HEAT: 
          return 0.341f;
        case MANUAL_J_COOL: 
          return 0.2f; 
      }
    }

  case FT_METAL:
    switch (glazing) {
    case SINGLE:
      switch (calc) { case ASHRAE: return 1.08f; case MANUAL_J_HEAT: return 1.155f;  case MANUAL_J_COOL: return 0.8f; } 
    case SINGLE_W_WOOD_STORM:
      switch (calc) { case ASHRAE: return 0.55f; case MANUAL_J_HEAT: return 0.650f;  case MANUAL_J_COOL: return 0.4f; } 
    case SINGLE_W_METAL_STORM:
      switch (calc) { case ASHRAE: return 0.78f; case MANUAL_J_HEAT: return 0.650f;  case MANUAL_J_COOL: return 0.4f; } 
    case SINGLE_W_BAD_STORM:
      switch (calc) { case ASHRAE: return 1.08f; case MANUAL_J_HEAT: return 1.155f;  case MANUAL_J_COOL: return 0.8f; } 
    case DOUBLE_GLAZED:
      switch (calc) { case ASHRAE: return 0.76f; case MANUAL_J_HEAT: return 0.725f;  case MANUAL_J_COOL: return 0.4f; } 
    case DOUBLE_GLAZED_LOWE:
      switch (calc) { case ASHRAE: return 0.66f; case MANUAL_J_HEAT: return 0.475f;  case MANUAL_J_COOL: return 0.2f; }
    case DOUBLE_GLAZED_W_STORM:
      switch (calc) { 
        case ASHRAE: 
            ASSERT(FALSE, sprintf(msg, "No U-value for glazing type: %d", glazing));
        case MANUAL_J_HEAT: 
          return 0.490f;
        case MANUAL_J_COOL: 
          return 0.2f;
      }
    }

  case FT_IMPROVED_METAL:
    switch (glazing) { 
    case SINGLE:
      switch (calc) { case ASHRAE: return 0.93f; case MANUAL_J_HEAT: return 1.045f; case MANUAL_J_COOL: return 0.8f; } 
    case SINGLE_W_WOOD_STORM:
      switch (calc) { case ASHRAE: return 0.51f; case MANUAL_J_HEAT: return 0.525f; case MANUAL_J_COOL: return 0.4f; } 
    case SINGLE_W_METAL_STORM:
      switch (calc) { case ASHRAE: return 0.69f; case MANUAL_J_HEAT: return 0.525f; case MANUAL_J_COOL: return 0.4f; } 
    case SINGLE_W_BAD_STORM:
      switch (calc) { case ASHRAE: return 0.93f; case MANUAL_J_HEAT: return 1.045f; case MANUAL_J_COOL: return 0.8f; } 
    case DOUBLE_GLAZED:
      switch (calc) { case ASHRAE: return 0.57f; case MANUAL_J_HEAT: return 0.609f; case MANUAL_J_COOL: return 0.4f; } 
    case DOUBLE_GLAZED_LOWE:
      switch (calc) { case ASHRAE: return 0.49f; case MANUAL_J_HEAT: return 0.399f; case MANUAL_J_COOL: return 0.2f; }
    case DOUBLE_GLAZED_W_STORM:
      switch (calc) { 
        case ASHRAE: 
            ASSERT(FALSE, sprintf(msg, "No U-value for glazing type: %d", glazing));
        case MANUAL_J_HEAT: 
          return 0.385f;
        case MANUAL_J_COOL: 
          return 0.2f;
      }
    }

  default:
    ASSERT(FALSE, sprintf(msg, "No uvalue for frame type: %d", frame));
  }
}
// clang-format on

/*******************************************
 * Function name window_shgc
 * Date:  3/17/2019
 * Author: MJF
 *
 * Description:  This was an index static array
 * but this removes the magic numbers and clearly
 * associates the coefficients with the framing and glazsing enumerations
 *
 * Since these factors are applied to the area_gross of the window, the
 * solar gain through the different opaque framing materials explains the
 * different values by frame type  MJF 3/2019
 * ***************************************/
float window_shgc(enum FRAME_TYPE frame, enum GLAZINGTYPE glazing) {
  switch (frame) {

  case FT_WOOD_VINYL:
    switch (glazing) {
    case SINGLE:
      return 0.64f;
    case SINGLE_W_WOOD_STORM:
    case SINGLE_W_METAL_STORM:
    case SINGLE_W_BAD_STORM:
    case DOUBLE_GLAZED:
      return 0.56f;
    case DOUBLE_GLAZED_LOWE:
      return 0.42f;
    default:
      ASSERT(FALSE, sprintf(msg, "No SHGC for glazing type: %d", glazing));
    }

  case FT_METAL:
    switch (glazing) {
    case SINGLE:
      return 0.76f;
    case SINGLE_W_WOOD_STORM:
    case SINGLE_W_METAL_STORM:
    case SINGLE_W_BAD_STORM:
    case DOUBLE_GLAZED:
      return 0.68f;
    case DOUBLE_GLAZED_LOWE:
      return 0.53f;
    default:
      ASSERT(FALSE, sprintf(msg, "No SHGC for glazing type: %d", glazing));
    }

  case FT_IMPROVED_METAL:
    switch (glazing) {
    case SINGLE:
      return 0.70f;
    case SINGLE_W_WOOD_STORM:
    case SINGLE_W_METAL_STORM:
    case SINGLE_W_BAD_STORM:
    case DOUBLE_GLAZED:
      return 0.62f;
    case DOUBLE_GLAZED_LOWE:
      return 0.44f;
    default:
      ASSERT(FALSE, sprintf(msg, "No SHGC for glazing type: %d", glazing));
    }

  default:
    ASSERT(FALSE, sprintf(msg, "No SHGC for frame type: %d", frame));
  }
}

float window_treatment_shgc(int treatment) {
  switch (treatment) {
  case N_CMS_SUN_SCREEN_FABRIC:
    return 0.34f;
  case N_CMS_SUN_SCREEN_LOUVERED:
    return  0.11f;
  case N_CMS_WINDOW_FILM:
    return 0.26f;
  }
  ASSERT(FALSE, sprintf(msg, "Unrecognized window treatment: %d", treatment));
}

/*******************************************
 * Function name window_not_suitable_for_shading_retrofit
 * Date:  3/19/2019
 * Author: MJF
 *
 * Description:  This bit of logic is repeated
 * for all the shading retrofits so deserves it's own
 * bit of clear logic
 * ***************************************/
int window_not_suitable_for_shading_retrofit(int nc) {
  return ndi->win[nc].solar_orient == SOLAR_NORTH || ndi->win[nc].shade_factor_summer < 0.5f ? 1 : 0;
}

/*******************************************
 * Function name door_u_value
 * Date:  3/25/2019
 * Author: MJF
 *
 * Description:  The door u-values based on type
 * condition and calculation type
 * ***************************************/
float door_u_value(enum DOOR_TYPE door, enum STORM_DOOR_INFO storm, enum LOAD_CALCULATION_TYPE calc) {
  // ASHRAE calcs credit R1 to adequate storm door
  switch (door) {

  case WOOD_HOLLOW_CORE:
    switch (storm) {
    case DR_ADEQUATE:
      switch (calc) {
      case ASHRAE:
        return 0.315f;
      case MANUAL_J_HEAT:
      case MANUAL_J_COOL:
        return 0.345f;
      }
    case DR_DETERIORATED:
    case DR_NONE:
      switch (calc) {
      case ASHRAE:
        return 0.460f;
      case MANUAL_J_HEAT:
      case MANUAL_J_COOL:
        return 0.560f;
      }
    }

  case WOOD_SOLID_CORE:
    switch (storm) {
    case DR_ADEQUATE:
      switch (calc) {
      case ASHRAE:
        return 0.286f;
      case MANUAL_J_HEAT:
      case MANUAL_J_COOL:
        return 0.305f;
      }
    case DR_DETERIORATED:
    case DR_NONE:
      switch (calc) {
      case ASHRAE:
        return 0.400f;
      case MANUAL_J_HEAT:
      case MANUAL_J_COOL:
        return 0.460f;
      }
    }

  case STEEL_INSULATED:
    switch (storm) {
    case DR_ADEQUATE:
      switch (calc) {
      case ASHRAE:
        return 0.286f;
      case MANUAL_J_HEAT:
      case MANUAL_J_COOL:
        return 0.342f;
      }
    case DR_DETERIORATED:
    case DR_NONE:
      switch (calc) {
      case ASHRAE:
        return 0.400f;
      case MANUAL_J_HEAT:
      case MANUAL_J_COOL:
        return 0.530f;
      }
    }

  case SINGLE_SLIDING_GLASS:
    switch (storm) {
    case DR_ADEQUATE:
      switch (calc) {
      case ASHRAE:
        return 0.482f;
      case MANUAL_J_HEAT:
      case MANUAL_J_COOL:
        return 0.525f;
      }
    case DR_DETERIORATED:
    case DR_NONE:
      switch (calc) {
      case ASHRAE:
        return 0.930f;
      case MANUAL_J_HEAT:
      case MANUAL_J_COOL:
        return 1.045f;
      }
    }

  case DOUBLE_SLIDING_GLASS:
    switch (storm) {
    case DR_ADEQUATE:
      switch (calc) {
      case ASHRAE:
        return 0.363f;
      case MANUAL_J_HEAT:
      case MANUAL_J_COOL:
        return 0.385f;
      }
    case DR_DETERIORATED:
    case DR_NONE:
      switch (calc) {
      case ASHRAE:
        return 0.570f;
      case MANUAL_J_HEAT:
      case MANUAL_J_COOL:
        return 0.609f;
      }
    }
  }
  ASSERT(FALSE, sprintf(msg, "Unrecognized door type: %d, storm: %d, calc: %d", door, storm, calc));
}

/*******************************************
 * Function name attic_ins_exist_rpi
 * Date:  4/4/2019
 * Author: MJF
 *
 * Description:  Removes the legacy rpinceil[]
 * array in favor of a function removing order
 * dependence and making the R value per
 * inch values clearly associated with the
 * insulation enumerated type
 * ***************************************/
// clang-format off
float attic_ins_exist_rpi(int nc) {
  switch (ndi->uas[nc].exist_insulation) {
    case AE_NONE:             return 0.0f;
    case AE_CELLULOSE_BLOWN:  return 3.75f;
    case AE_FIBERGLASS_BLOWN: return 3.09f;
    case AE_ROCKWOOL_BLOWN:   return 3.09f;
    case AE_FIBERGLASS_BATT:  return 3.33f;
    case AE_OTHER:            return 3.09f;
  }
  ASSERT(FALSE, sprintf(msg, "Unrecognized attic exist_insulation: %d", ndi->uas[nc].exist_insulation));
}
// clang-format on

/*******************************************
 * Function name attic_new_rpi
 * Date:  4/5/2019
 * Author: MJF
 *
 * Description: The new insulation r value
 * per inch (rpi)
 * ***************************************/
float attic_ins_added_rpi(int nc) {
  ASSERT(ndi->ins_attic[ndi->uas[nc].added_insulation].value > 0.0f, sprintf(msg, "Missing insulation R per inch value for attic: %s", ndi->uas[nc].code));
  return ndi->ins_attic[ndi->uas[nc].added_insulation].value; // insulation values are imported by their enumeration indexes
}

// returns true if measures i and j are mutually exclusive

int mutually_exclusive_measures(int i, int j) {
  int m;
  for (m = 0; m < nir->measure_exclusion_count[i]; m++) {
    if (nir->measure_exclusion[i][m] == j)
      return (1);
  }
  return (0);
}

/*****************************************
 *****************************************
 Rank measures(items) according to decreasing B/C (ranking parameter)  */

int rankms(float elc[], int npt, int kwhc[]) {
  int np, ip, is, i, j;
  for (i = 0; i < MAXECMS; i++)
    kwhc[i] = 0;

  /*  Rank one item at a time after assuming the first item has the greatest
        ranking parameter  */

  for (np = 1; np <= npt; np++) {

    /*  Starting with the item with smallest ranking parameter, find the first
        item previously ranked whose ranking paramter is greater than the item
        currently being ranked (the test item) */

    for (ip = np - 1; ip >= -1; ip--) {
      if (ip == -1) {
        for (j = np - 1; j >= ip + 1; j--)
          is = kwhc[j], kwhc[j + 1] = is;
        kwhc[ip + 1] = np;
        goto laba;
      } else if (elc[np] < elc[kwhc[ip]]) {

        /*  Increase by one the rankings of all items with ranking parameters less
            than the test item  */

        for (j = np - 1; j >= ip + 1; j--)
          is = kwhc[j], kwhc[j + 1] = is;

        /*  Set the rank of the test item at the level determined   */

        kwhc[ip + 1] = np;
        goto laba;
      }
    }
  laba:;
  }
  /*
        fprintf(stderr, "\n    I   elc        I   kwhc  elc\n") ;
        for(i=0;i<=npt;i++) fprintf(stderr, "%5d%7.2f%8d%5d%7.2f\n",
           i,elc[i],i,kwhc[i],elc[kwhc[i]]);
  */
  return (0);
}

/******************
 * Get a line from a file with file pointer fln, terminating on CR, or
 * more than "max" chars entered.  DOES NOT include the ending newline.
 * Returns EOF if end of file detected; otherwise, the number of characters
 * read (not counting trialing NULL).  String produced terminates with null
 * character.
 */

int getf_line(char line[], int max, FILE *fln) {
  int i, c; /* current char, count of chars  */

  /* read until max chars; ignore all chars after max until CR. */

  i = 0;
  while ((c = getc(fln)) != '\n' && c != EOF)
    if (i < max - 1)
      line[i++] = c;
  line[i] = '\0'; /* terminate with null */
  if (c == EOF)
    return EOF;
  else
    return i;
}

/************************
 ************************
 Skip to the lines'th line after last character read
 eg. for lines = 1, the read skips to the next line, skipping none  */

int skipl(FILE *file, int lines) {
  int i;
  for (i = 1; i <= lines; i++)
    while (fgetc(file) != '\n')
      ;
  return (0);
}

/***********************
 ***********************/

int skipc(FILE *file, int nc) {
  int i;
  char blank = ' ';
  for (i = 1; i <= nc; i++)
    fprintf(file, "%c", blank);
  return (0);
}

/***********************
 ***********************/
int savebase(void) {

  for (int m = 1; m <= MONTHS; m++) {
    bswncfmtot[m] = nir->wn_cfm_tot[m];
    bswnlatload[m] = nir->wn_lat_load[m];
    bsdrcfmtot[m] = nir->dr_cfm_tot[m];
    bsdrlatload[m] = nir->dr_lat_load[m];
    bshtgmengy[m] = nir->htgmengy[1][m];
    bsclgmengy[m] = nir->clgmengy[1][m];
  }
  for (int m = 0; m < MONTHS + 1; m++)
    bstotsol[m] = nir->totsol[m];
  for (int nc = 0; nc < NEAT_MAX_UAS; nc++) {
    bsrcuvlclc[nc] = ndi->uas[nc].u_value;
    bsrcinsclinp[nc] = ndi->uas[nc].ins_depth;
    bsrcuvlclj[nc] = ndi->uas[nc].joist_u_value;
    bsuavrc[nc] = ndi->uas[nc].ua_value;
    bsuainsdpth[nc] = ndi->uas[nc].ins_depth;
    bsuainstype[nc] = ndi->uas[nc].exist_insulation;
    bsuainsrv[nc] = ndi->uas[nc].r_value;
    bsuarcabsorp[nc] = ndi->uas[nc].roof_absorptance;
  }
  for (int nc = 0; nc < NEAT_MAX_FND; nc++) {
    bsflrcavr[nc] = ndi->fnd[nc].floor_cavity_r;
    bssbuvalfl[nc] = ndi->fnd[nc].floor_u_value;
    bsuaeff[nc] = ndi->fnd[nc].ua_value_basement_effective;
    bsuabsmt[nc] = ndi->fnd[nc].ua_value_basement;
    bsuasill[nc] = ndi->fnd[nc].ua_value_basement_sill;
    bssbflrinsr[nc] = ndi->fnd[nc].flr_ins_r;
    bssbwlinsr[nc] = ndi->fnd[nc].wall_ins_r;
    bssbuvalwla[nc] = ndi->fnd[nc].above_grade_wall_u_value;
    bssbuvalwlb[nc] = ndi->fnd[nc].below_grade_wall_u_value;
    bsuvalwlbtot[nc] = ndi->fnd[nc].u_value_basement_wall_total;
  }
  for (int nc = 0; nc < NEAT_MAX_WAL; nc++) {
    bswlucavty[nc] = ndi->wal[nc].u_cavity;
    bsuavwl[nc] = ndi->wal[nc].ua_value;
    bswlinsrval[nc] = ndi->wal[nc].exist_r;
  }
  for (int nc = 0; nc < NEAT_MAX_WIN; nc++) {
    bswnuvalue[nc] = ndi->win[nc].u_value;
    bswnsgfsum[nc] = ndi->win[nc].shgc_summer;
    bswnsgfwin[nc] = ndi->win[nc].shgc_winter;
    bsshade[0][nc] = ndi->win[nc].shade_factor_winter;
    bsshade[1][nc] = ndi->win[nc].shade_factor_summer;
    bswnglazg[nc] = ndi->win[nc].glazing_type;
    bswnleakcoef[nc] = ndi->win[nc].leak_coef;
    bswnsunscrn[HEATING][nc] = nir->wn_sunscrn[HEATING][nc];
    bswnsunscrn[COOLING][nc] = nir->wn_sunscrn[COOLING][nc];
    for (int m = 1; m <= MONTHS; m++)
      bswncfm[nc][m] = nir->wn_leak_cfm[nc][m];
  }
  for (int nc = 0; nc < NEAT_MAX_DOR; nc++) {
    bsdruvalue[nc] = ndi->dor[nc].u_value;
    bsdrleakcoef[nc] = ndi->dor[nc].leak_coef;
    for (int m = 1; m <= MONTHS; m++)
      bsdrcfm[nc][m] = nir->dr_leak_cfm[nc][m];
  }
  for (int nc = 0; nc < NEAT_MAX_CLG; nc++)
    bsacseer[nc] = ndi->clg[nc].seer;
  for (int m = 0; m <= MONTHS; m++) {
    bsblc[m] = nir->building_load_coeff[m];
    bsfreeheat_night[m] = nir->internal_gain_night[m];
    bsfreeheat_day[m] = nir->internal_gain_day[m];
    bsteffn[m] = nir->teffn[m];
    bsnight_setback_temperature[m] = nir->night_setback_temperature[m];
  }
  /*   for(i=0;i<MAXECMS;i++) { bsindex_by_sir[i]=index_by_sir[i]; bsnpv[i] = npv[i]; } */
  bsrdbrdldc = nir->rdbrdldc, bsrdbrdldh = nir->rdbrdldh;
  bsprsys_seaseff = ndi->htg[PRIMARY].delivered_eff;
  bssysht_seaseff = nir->sysht_seaseff;
  bsseer = ndi->clgs.avg_seer;
  bsFductht = ndi->inf.pre_duct_seal_efficiency;
  bsperac = ndi->clgs.fraction_cooled;
  htg0fueltype = ndi->htg[PRIMARY].fuel_type;

  return (0);
}

/********************
 ********************/

int restorebase(void) {

  for (int m = 1; m <= MONTHS; m++) {
    nir->wn_cfm_tot[m] = bswncfmtot[m];
    nir->wn_lat_load[m] = bswnlatload[m];
    nir->dr_cfm_tot[m] = bsdrcfmtot[m];
    nir->dr_lat_load[m] = bsdrlatload[m];
    nir->htgmengy[1][m] = bshtgmengy[m];
    nir->clgmengy[1][m] = bsclgmengy[m];
  }
  for (int m = 0; m <= MONTHS; m++)
    nir->totsol[m] = bstotsol[m];
  for (int nc = 0; nc < NEAT_MAX_UAS; nc++) {
    ndi->uas[nc].u_value = bsrcuvlclc[nc];
    ndi->uas[nc].ins_depth = bsrcinsclinp[nc];
    ndi->uas[nc].joist_u_value = bsrcuvlclj[nc];
    ndi->uas[nc].ua_value = bsuavrc[nc];
    ndi->uas[nc].ins_depth = bsuainsdpth[nc];
    ndi->uas[nc].exist_insulation = bsuainstype[nc];
    ndi->uas[nc].r_value = bsuainsrv[nc];
    ndi->uas[nc].roof_absorptance = bsuarcabsorp[nc];
  }
  for (int nc = 0; nc < NEAT_MAX_FND; nc++) {
    ndi->fnd[nc].floor_cavity_r = bsflrcavr[nc];
    ndi->fnd[nc].floor_u_value = bssbuvalfl[nc];
    ndi->fnd[nc].ua_value_basement_effective = bsuaeff[nc];
    ndi->fnd[nc].ua_value_basement = bsuabsmt[nc];
    ndi->fnd[nc].ua_value_basement_sill = bsuasill[nc];
    ndi->fnd[nc].flr_ins_r = bssbflrinsr[nc];
    ndi->fnd[nc].wall_ins_r = bssbwlinsr[nc];
    ndi->fnd[nc].above_grade_wall_u_value = bssbuvalwla[nc];
    ndi->fnd[nc].below_grade_wall_u_value = bssbuvalwlb[nc];
    ndi->fnd[nc].u_value_basement_wall_total = bsuvalwlbtot[nc];
  }
  for (int nc = 0; nc < NEAT_MAX_WAL; nc++) {
    ndi->wal[nc].u_cavity = bswlucavty[nc];
    ndi->wal[nc].ua_value = bsuavwl[nc];
    ndi->wal[nc].exist_r = bswlinsrval[nc];
  }
  for (int nc = 0; nc < NEAT_MAX_WIN; nc++) {
    ndi->win[nc].u_value = bswnuvalue[nc];
    ndi->win[nc].shgc_summer = bswnsgfsum[nc];
    ndi->win[nc].shgc_winter = bswnsgfwin[nc];
    ndi->win[nc].shade_factor_winter = bsshade[0][nc];
    ndi->win[nc].shade_factor_summer = bsshade[1][nc];
    ndi->win[nc].glazing_type = bswnglazg[nc];
    ndi->win[nc].leak_coef = bswnleakcoef[nc];
    nir->wn_sunscrn[HEATING][nc] = bswnsunscrn[HEATING][nc];
    nir->wn_sunscrn[COOLING][nc] = bswnsunscrn[COOLING][nc];
    for (int m = 1; m <= MONTHS; m++)
      nir->wn_leak_cfm[nc][m] = bswncfm[nc][m];
  }
  for (int nc = 0; nc < NEAT_MAX_DOR; nc++) {
    ndi->dor[nc].u_value = bsdruvalue[nc];
    ndi->dor[nc].leak_coef = bsdrleakcoef[nc];
    for (int m = 1; m <= MONTHS; m++)
      nir->dr_leak_cfm[nc][m] = bsdrcfm[nc][m];
  }
  for (int nc = 0; nc < NEAT_MAX_CLG; nc++)
    ndi->clg[nc].seer = bsacseer[nc];
  for (int m = 0; m <= MONTHS; m++) {
    nir->building_load_coeff[m] = bsblc[m];
    nir->internal_gain_night[m] = bsfreeheat_night[m];
    nir->internal_gain_day[m] = bsfreeheat_day[m];
    nir->teffn[m] = bsteffn[m];
    nir->night_setback_temperature[m] = bsnight_setback_temperature[m];
  }
  /*    for(i=0;i<MAXECMS;i++) { index_by_sir[i]=bsindex_by_sir[i]; npv[i] = bsnpv[i]; } */
  nir->rdbrdldc = bsrdbrdldc;
  nir->rdbrdldh = bsrdbrdldh;
  ndi->htg[PRIMARY].delivered_eff = bsprsys_seaseff;
  nir->sysht_seaseff = bssysht_seaseff;
  ndi->clgs.avg_seer = bsseer;
  ndi->inf.pre_duct_seal_efficiency = bsFductht;
  ndi->clgs.fraction_cooled = bsperac;
  ndi->htg[PRIMARY].fuel_type = htg0fueltype;

  return (0);
}

/*********************
 *********************
 This routine computes the composite heating fuel cost ($/MMBtu) for multiple
 heating systems. It replaces the macro COMPFLCOST which handled only two
 systems.*/

float CompFuelCost(void) {
  int i;
  float num = 0.0f, den = 0.0f, compfuelcost;

  for (i = 0; i < ndi->num_htg; i++) {
    num += fuel_cost_per_mmbtu(ndi->htg[i].fuel_type) * ndi->htg[i].percent_heat_supplied / ndi->htg[i].delivered_eff;
    den += ndi->htg[i].percent_heat_supplied / ndi->htg[i].delivered_eff;
  }

  compfuelcost = num / den;
  return (compfuelcost);
}

/************************
 ************************
 This routine computes the composite discounted heating fuel cost savings
 ($/first year MMBtu savings) over the life of a measure for multiple
 heating systems. It replaces the macro DCOMPFLCOST previously used
 for only two systems. */

float DCompFuelCost(int life) {
  int i;
  float num = 0.0f, den = 0.0f, dcompfuelcost;

  for (i = 0; i < ndi->num_htg; i++) {
    num += pw_fuel_cost(ndi->htg[i].fuel_type, life) * ndi->htg[i].percent_heat_supplied / ndi->htg[i].delivered_eff;
    den += ndi->htg[i].percent_heat_supplied / ndi->htg[i].delivered_eff;
  }

  dcompfuelcost = num / den;
  return (dcompfuelcost);
}
