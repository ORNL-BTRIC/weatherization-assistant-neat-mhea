/*******************  MODULE NAME:  BILLING.C  ***************************/
/**			DATE: October 1994
 * **/
/**			  BY: SLF
 * **/
/**  DESCRIPTION:	Edited version of original ORNL NEAT utility.c code.	**/
/**					BillAdj() is called from the Run() function (if the	**/
/**					selects "Run w/ Billing Data" from the menus) and 		**/
/**					writes the Billing Comparison output file.				**/
/**    REVISIONS:
 * **/
/**		7/14/95, NW, revisions to menus.c to send correct monthly		**/
/**					values to BillAdj function
 * **/
/**		7/28/95, NW, notice added for ambiguous baseload					**/
/**		 8/3/95, NW, notice added for insufficient billing data			**/
/**		 8/3/95, NW, initialization of adj factor moved outside loop	**/
/**    04/15/97, NW, added 14th element to idom to avoid out-of-bounds **/
/**     				array reference, updated debug message handling **/
/**    08/06/02, MJF re-write to use NEAT data structures and integrate **/
/**                  into mheaex.c code which re-runs the cumulative    **/
/**                  pass after billing adjustments to energy           **/
/*************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <math.h>

#include "wa_engine.h"

// Utility Billing information static variables, same naming convention
// as in NEAT -- but here the variables are global just to this module

int nble[MAX_UBI];             // number of billing records [0]pre heating, [1]pre cooling, [2]post heating, [3]post cooling
int fbyf[MAX_UBI][MONTHS + 1]; // year of each bill
int fbmf[MAX_UBI][MONTHS + 1]; // month number of each bill
int fbdf[MAX_UBI][MONTHS + 1]; // day of the month
int ndfper[MAX_UBI];           // number of days in first billing period
int fbcn[MAX_UBI][MONTHS + 1]; // consumption of each bill
int fbdd[MAX_UBI][MONTHS + 1]; // degree days (optional) for each bill
float ddbase[MAX_UBI];         // heating degree day base temperature, deg F
float baseldinpMHEA[MAX_UBI] = // base load in consumption units
    {-1., -1., -1., -1.};      // logic needs to see -1 if not read from file
char adjbill;                  // Do billing adjustments, Y/N
int iuoMHEA[MAX_UBI] = {-1, -1, -1, -1}; // input units flag -1 missing, 0=therms, 1=kWh
float tbal[DAY + 1][COOLING + 1][MONTHS + 1];

static int mhea_billing_adjustment(float *adj, int is, int iu, int runflg, FILE *outstream);

/********************
NW, 7/14/95, Monthly use generated in mhea_energy_use function - calling sequence was
changed to insure that values passed to BillAdj corresponded to basecase values
*********************/

/***************************************************************************
 ** Function Name: translate_mhea_bil
 **          Date: March 31, 1999
 **     Author(s): Mark Fishbaugher
 **
 **  DESCRIPTION: used the MDI data structure to fill in all of the variables
 **  associated with utility billing information (contents of the .bil
 **  file in versions 6.1 and earlier.  Adapted from function of the same
 **  name in NEAT ioconv.c
 **************************************************************************/
void translate_mhea_bil(void) {
  int j, count;

  // HEATING
  ndfper[HEATING] = mdi->ubh.period_days;      // number of days in first billing period
  ddbase[HEATING] = mdi->ubh.base_temp;        // heating degree day base temperature, deg F
  baseldinpMHEA[HEATING] = mdi->ubh.base_load; // base load in consumption units
  if (mdi->ubh.usage_units == THERMS)
    iuoMHEA[HEATING] = 0;
  else
    iuoMHEA[HEATING] = 1;

  count = 0;
  for (j = 0; j < mdi->num_urh; j++) // loop through all billing records
  {
    count++;                                       // following arrays ARE base 1
    fbyf[HEATING][count] = mdi->urh[j].year;             // year of bill (yyyy)
    fbmf[HEATING][count] = mdi->urh[j].month;            // month of bill (1-12)
    fbdf[HEATING][count] = mdi->urh[j].day;              // day of the month for bill (1-31)
    fbcn[HEATING][count] = (int)mdi->urh[j].usage;       // consumption in specified units
    fbdd[HEATING][count] = (int)mdi->urh[j].degree_days; // heating or cooling degree days
  }
  nble[HEATING] = count;

  // COOLING
  ndfper[COOLING] = mdi->ubc.period_days;      // number of days in first billing period
  ddbase[COOLING] = mdi->ubc.base_temp;        // heating degree day base temperature, deg F
  baseldinpMHEA[COOLING] = mdi->ubc.base_load; // base load in consumption units
  if (mdi->ubc.usage_units == THERMS)
    iuoMHEA[COOLING] = 0;
  else
    iuoMHEA[COOLING] = 1;

  count = 0;
  for (j = 0; j < mdi->num_urc; j++) // loop through all billing records
  {
    count++;                                       // following arrays ARE base 1
    fbyf[COOLING][count] = mdi->urc[j].year;             // year of bill (yyyy)
    fbmf[COOLING][count] = mdi->urc[j].month;            // month of bill (1-12)
    fbdf[COOLING][count] = mdi->urc[j].day;              // day of the month for bill (1-31)
    fbcn[COOLING][count] = (int)mdi->urc[j].usage;       // consumption in specified units
    fbdd[COOLING][count] = (int)mdi->urc[j].degree_days; // heating or cooling degree days
  }
  nble[COOLING] = count;
}

//int manage_mhea_billing_adjustments(WeatherData) {
void manage_mhea_billing_adjustments(void) {

  // char sOutputFile[M_FILENAME_LEN];
  FILE *outstream = NULL;
  int iunit;
  float fHtgAdj, fClgAdj;

  mir->fAdj_Htg = 1.0; // starting point -- unity adjustments
  mir->fAdj_Clg = 1.0;

  translate_mhea_bil(); // get data into our static variables

  if (strcmp(cmds.mhea_compare_file_path, NO_OUTPUT) != 0) {
    outstream = fopen(cmds.mhea_compare_file_path, "w");
    ASSERT(outstream, sprintf(msg, "Couldn't open MHEA report output file: %s code:%d:%s", cmds.mhea_compare_file_path, errno, strerror(errno)));
  }

  if (nble[HEATING] > 0) // there are heating billing records
  {
    iunit = iuoMHEA[HEATING];
    //mhea_billing_adjustment(WeatherData, &fHtgAdj, 0, iunit, 1, outstream);
    mhea_billing_adjustment(&fHtgAdj, HEATING, iunit, 1, outstream);
    if (outstream)
      fprintf(outstream, "\n\n");
    mir->fAdj_Htg = fHtgAdj;
  }
  if (nble[COOLING] > 0) // there are cooling billing records
  {
    iunit = iuoMHEA[COOLING];
    //mhea_billing_adjustment(WeatherData, &fClgAdj, 1, iunit, 1, outstream);
    mhea_billing_adjustment(&fClgAdj, COOLING, iunit, 1, outstream);
    if (outstream)
      fprintf(outstream, "\n\n");
    mir->fAdj_Clg = fClgAdj;
  }
  if (outstream)
    fclose(outstream);
  //return (OK);
  return;
}

static float prcn[MONTHS + 1], prdd[MONTHS + 1];

//int mhea_billing_adjustment(WEATHER *WeatherData, float *adj, int is, int iu, int runflg, FILE *outstream) {
int mhea_billing_adjustment(float *adj, int is, int iu, int runflg, FILE *outstream) {
  float acmin, acmax, avdcn[MONTHS + 2];
  float fbhtgcn[MONTHS + 1], pradcn[MONTHS + 1], totactual, totpred;
  float totddpred = 0., totddact, fnd, unitconv[2], totdif, totddif;
  //char *slabl[2], *slab2[2], *slab3[2];
  char *slab3[2];
  //int l, m, col[6], wid[6], lnno, ndip[MONTHS + 1];
  int l, m, lnno, ndip[MONTHS + 1];
  int mi, mf, mfe, ma, inday, enday, totndays;
  static int idop[MONTHS + 1], fdop[MONTHS + 1];
  static float avdghrs[2][MONTHS + 1];

  slab3[0] = "(Therms)";
  slab3[1] = " (kWh)  ";

  // unitconv[0]=0.10f;  unitconv[1]=0.003413f;
  //   /* .10 converts MMBtu to Therms;  .003413 converts MMBtu to kWh  */

  /* 0.10 converts MMBtu to Therms */
  /* 100000.0 converts Btu to Therms */
  unitconv[0] = BTU_PER_THERM;
  /* 0.003413 converts MMBtu to kWh  */
  //unitconv[1] = MMBTU_PER_KWH;
  unitconv[1] = BTU_PER_KWH;        // #352, MHEA internal loads computed in unit of Btu rather than MMBtu

  /* Determine last acceptable period, ie., nonzero dates, and number of days
   * in period  */

  ndip[1] = ndfper[is];
  fdop[1] = doy(fbmf[is][1], fbdf[is][1]);
  idop[1] = fdop[1] - ndip[1] + 1;
  if (idop[1] <= 0)
    idop[1] += 365;
  for (l = 2; l < 13; l++) {
    fdop[l] = doy(fbmf[is][l], fbdf[is][l]);
    ndip[l] = fdop[l] - fdop[l - 1];
    if (ndip[l] <= 0)
      ndip[l] += 365;
    idop[l] = fdop[l] - ndip[l];
    if (idop[l] <= 0)
      idop[l] += 365;
    if (fbmf[is][l] == 0 || fbdf[is][l] == 0)
      break;
  }
  lnno = l - 1;
  avdcn[0] = avdcn[lnno + 1] = 1.e20f;

  /* Determine period of least and greatest average daily consumption. */

  ASSERT((float)ndip[1] != 0, sprintf(msg, "Assertion Failure"));
  avdcn[1] = (float)fbcn[is][1] / (float)ndip[1];
  acmin = acmax = avdcn[1];
  for (l = 2; l <= lnno; l++) {

    ASSERT((float)ndip[l], sprintf(msg, "Assertion Failure"));
    avdcn[l] = (float)fbcn[is][l] / (float)ndip[l];
    if (avdcn[l] < acmin) {
      acmin = avdcn[l];
    }
    if (avdcn[l] > acmax) {
      acmax = avdcn[l];
    }
  }

  /* Initialize parameters.
   * Set baseload to acmin if no value has been read in. */

  for (m = 1; m <= MONTHS; m++) {
    tbal[NIGHT][HEATING][m] = tbal[NIGHT][COOLING][m] = tbal[DAY][HEATING][m] = tbal[DAY][COOLING][m] = ddbase[is];
    prcn[m] = prdd[m] = 0.;
  }
  totactual = totpred = totddpred = totddact = 0.;
  totndays = 0;
  if (baseldinpMHEA[is] < 0.0f)
    baseldinpMHEA[is] = acmin * 30.0f;

  /* Determine the interpolated daily degree-days for each month */

  adjusted_monthly_degree_hours(tbal, avdghrs);

  /* Subtract base load from billing data and determine predicted consumption
   * corresponding to billing period. */
  if (outstream) {
    if(cmds.regression_test)
      fprintf(outstream, "%s\n", "MHEA ");
    else
      fprintf(outstream, "%s\n", "MHEA " WA_VERSION);
  }

  if (is == HEATING) {
    if (outstream) {
      fprintf(outstream, "\n                   HEATING ENERGY CONSUMPTION COMPARISON \n\n");
      fprintf(outstream, "     P e r i o d         C o n s u m p t i o n       D e g r e e  D a y s\n");
      fprintf(outstream, "   End     Days in             %8.8s                  (Base  %2d F)\n", slab3[iu], (int)(ddbase[is]));
    }
    // optional structure output

    STRCPY(mor->heat_comp_units, slab3[iu]);
    mor->heat_dd_base = (int)ddbase[is];
    
  } else {
    if (outstream) {
      fprintf(outstream, "\n                   COOLING ENERGY CONSUMPTION COMPARISON \n\n");
      fprintf(outstream, "     P e r i o d         C o n s u m p t i o n       D e g r e e  D a y s\n");
      fprintf(outstream, "   End     Days in             %8.8s                   (Base  %2d F)\n", slab3[iu], (int)(ddbase[is]));
    }
    // optional structure output

    STRCPY(mor->cool_comp_units, slab3[iu]);
    mor->cool_dd_base = (int)ddbase[is];

  }
  if (outstream)
    fprintf(outstream, "     Date       Period       Actual      Predicted        Actual     Predicted\n");

  for (m = 1; m <= MONTHS; m++) {

    ASSERT(cwd->days_in_month[m] != 0, sprintf(msg, "Assertion Failure"));
    avdghrs[is][m] /= (float)(cwd->days_in_month[m]);

    ASSERT(unitconv[iu] != 0, sprintf(msg, "Assertion Failure"));
    if (is == HEATING)
      pradcn[m] = mir->fMonthlyHtgUse[m - 1] / unitconv[iu] / (float)(cwd->days_in_month[m]);
    else
      pradcn[m] = mir->fMonthlyClgUse[m - 1] / unitconv[iu] / (float)cwd->days_in_month[m];
  }

  for (l = 1; l <= lnno; l++) { /* Step through periods */
    fbhtgcn[l] = (float)fbcn[is][l] - (float)ndip[l] * baseldinpMHEA[is] / 30.0f;
    if (fbhtgcn[l] < 0.)
      fbhtgcn[l] = 0.;
    m = 0;
    do
      m++;
    while (idop[l] >= cwd->days_in_year_for_month[m]);
    mi = m - 1;
    m = 0;
    do
      m++;
    while (fdop[l] >= cwd->days_in_year_for_month[m]);
    mf = m - 1;
    mfe = mf;
    if (idop[l] > fdop[l])
      mfe = mf + 12;
    for (m = mi; m < mfe; m++) {
      ma = m;
      if (m > 12)
        ma = m - 12;
      inday = cwd->days_in_year_for_month[ma];
      if (m == mi)
        inday = idop[l];
      if (ma < 12)
        enday = cwd->days_in_year_for_month[ma + 1];
      else
        enday = 366;
      fnd = (float)(enday - inday);
      prcn[l] += pradcn[ma] * fnd;
      prdd[l] += avdghrs[is][ma] * fnd / 24.0f;
    }
    if (mi != mfe)
      fnd = (float)(fdop[l] - cwd->days_in_year_for_month[mf] + 1);
    else
      fnd = (float)(fdop[l] - idop[l] + 1);
    prcn[l] += pradcn[mf] * fnd;
    prdd[l] += avdghrs[is][mf] * fnd / 24.0f;
    totpred += prcn[l];
    totactual += fbhtgcn[l];
    totndays += ndip[l];
    totddpred += prdd[l];
    totddact += fbdd[is][l];
    if (runflg < 2) {
      if (outstream) {
        fprintf(outstream, "\n  %4d/%2d/%2d%9d ", fbyf[is][l], fbmf[is][l], fbdf[is][l], ndip[l]);
        fprintf(outstream, "      %7d      %7d", (int)(fbhtgcn[l] + .5), (int)(prcn[l] + .5));
        fprintf(outstream, "         %7d      %7d", fbdd[is][l], (int)(prdd[l] + .5));
      }

      // optional structure output

      if (mor) {
        int cnt;
        if (is == HEATING) {
          cnt = mor->num_heat_comp;
          mor->heat_comp[cnt].index = cnt;
          mor->heat_comp[cnt].year = fbyf[is][l];
          mor->heat_comp[cnt].month = fbmf[is][l];
          mor->heat_comp[cnt].day = fbdf[is][l];
          mor->heat_comp[cnt].period_days = ndip[l];
          mor->heat_comp[cnt].consump_act = (int)(fbhtgcn[l] + .5);
          mor->heat_comp[cnt].consump_pred = (int)(prcn[l] + .5);
          mor->heat_comp[cnt].dd_act = fbdd[is][l];
          mor->heat_comp[cnt].dd_pred = (int)(prdd[l] + .5);
          mor->num_heat_comp++;
        } else {
          cnt = mor->num_cool_comp;
          mor->cool_comp[cnt].index = cnt;
          mor->cool_comp[cnt].year = fbyf[is][l];
          mor->cool_comp[cnt].month = fbmf[is][l];
          mor->cool_comp[cnt].day = fbdf[is][l];
          mor->cool_comp[cnt].period_days = ndip[l];
          mor->cool_comp[cnt].consump_act = (int)(fbhtgcn[l] + .5);
          mor->cool_comp[cnt].consump_pred = (int)(prcn[l] + .5);
          mor->cool_comp[cnt].dd_act = fbdd[is][l];
          mor->cool_comp[cnt].dd_pred = (int)(prdd[l] + .5);
          mor->num_cool_comp++;
        }
      }
    }
  }

  if (runflg < 2) {
    if (outstream) {
      fprintf(outstream, "\n\n  Total     %4d       %7ld      %7ld", totndays, (long)(totactual + .5), (long)(totpred + .5));
      fprintf(outstream, "         %7d      %7d", (int)(totddact + .5), (int)(totddpred + .5));
    }
    if (totactual > .1) {
      ASSERT(totactual != 0, sprintf(msg, "Assertion Failure"));
      totdif = -(totactual - totpred) / totactual * 100.0f;
      if (outstream)
        fprintf(outstream, "\n%% Difference                 %7ld%%", (long)(totdif + 0.5));
    }
    if (totddact > .1) {

      ASSERT(totddact != 0, sprintf(msg, "Assertion Failure"));
      totddif = -(totddact - totddpred) / totddact * 100.0f;
      if (outstream)
        fprintf(outstream, "                     %7ld%%", (long)(totddif + 0.5));
    }
  }

  if (totpred > .0001) {
    *adj = totactual / totpred;
  }
  nble[is] = lnno;
  return (0);
}

