/***************************************************************************
* MODULE:       billing.c            CREATED:      June 8, 2020
*
* AUTHOR:   Mike Gettings       Oak Ridge National Laboratory
*           Mark Fishbaugher    Fishbaugher and Associates
*
* MDESC:
****************************************************************************/

#include "wa_engine.h"

// Manage the calls to the billing adjustment routines for heating or cooling

void manage_neat_billing_adjustments(int *flg) {
  int iunit;
  // extern int buff1[];

  *flg = 0;

  if (nir->bill_consumption_units[PRE_HEATING] == BILL_UNITS_MISSING) {
    if (ndi->htg[PRIMARY].fuel_type != ELECTRIC)
      iunit = BILL_UNITS_THERMS;
    else
      iunit = BILL_UNITS_KWH;
  } else
    iunit = nir->bill_consumption_units[PRE_HEATING];

  /* for autoexecute mode, compute nir->bladj[] only, thus 4th arg = 2 */

  if (ndi->gnl.do_billing_adjust == YES) {
    if (nir->billing_record_count[HEATING] > 0) {
      neat_billing_adjustment(nir->bladj, PRE_HEATING, iunit, SECOND_BILL_ADJUST, nir->blapflg, (FILE *)NULL); 
      *flg += nir->blapflg[PRE_HEATING];
    }
    if (nir->billing_record_count[COOLING] > 0) {
      neat_billing_adjustment(nir->bladj, PRE_COOLING, BILL_UNITS_KWH, SECOND_BILL_ADJUST, nir->blapflg, (FILE *)NULL);
      *flg += nir->blapflg[PRE_COOLING];
    }
  }
}

/***************************************************************************
 ** Function Name: translate_bil
 **          Date: March 31, 1999
 **     Author(s): Mark Fishbaugher
 **
 **  DESCRIPTION: used the dwelling data structure to fill in all of the variables
 **  associated with utility billing information (contents of the .bil
 **  file in versions 6.1 and earlier
 **************************************************************************/
void translate_neat_bil(void) {
  // int i, j, n = -1, count;
  int j, count;

  // HEATING

  nir->bill_first_period_days[PRE_HEATING] = ndi->ubh.period_days;  // number of days in first billing period
  nir->bill_degree_day_base[PRE_HEATING] = ndi->ubh.base_temp;    // heating degree day base temperature, deg F
  nir->bill_base_load_consumption[PRE_HEATING] = ndi->ubh.base_load; // base load in consumption units
  if (ndi->ubh.usage_units == THERMS)
    nir->bill_consumption_units[PRE_HEATING] = BILL_UNITS_THERMS;
  else
    nir->bill_consumption_units[PRE_HEATING] = BILL_UNITS_KWH;

  count = 0;
  for (j = 0; j < ndi->num_urh; j++) // loop through all billing records
  {
    count++;                                       // following arrays ARE base 1
    nir->bill_year[PRE_HEATING][count] = ndi->urh[j].year;             // year of bill (yyyy)
    nir->bill_month[PRE_HEATING][count] = ndi->urh[j].month;            // month of bill (1-12)
    nir->bill_day[PRE_HEATING][count] = ndi->urh[j].day;              // day of the month for bill (1-31)
    nir->bill_consumption[PRE_HEATING][count] = (int)ndi->urh[j].usage;       // consumption in specified units
    nir->bill_degree_days[PRE_HEATING][count] = (int)ndi->urh[j].degree_days; // heating or cooling degree days
  }
  nir->billing_record_count[PRE_HEATING] = count;

  // COOLING

  nir->bill_first_period_days[PRE_COOLING] = ndi->ubc.period_days;  // number of days in first billing period
  nir->bill_degree_day_base[PRE_COOLING] = ndi->ubc.base_temp;    // heating degree day base temperature, deg F
  nir->bill_base_load_consumption[PRE_COOLING] = ndi->ubc.base_load; // base load in consumption units
  if (ndi->ubc.usage_units == THERMS)
    nir->bill_consumption_units[PRE_COOLING] = BILL_UNITS_THERMS;
  else
    nir->bill_consumption_units[PRE_COOLING] = BILL_UNITS_KWH;

  count = 0;
  for (j = 0; j < ndi->num_urc; j++) // loop through all billing records
  {
    count++;                                       // following arrays ARE base 1
    nir->bill_year[PRE_COOLING][count] = ndi->urc[j].year;             // yeR of bill (yyyy)
    nir->bill_month[PRE_COOLING][count] = ndi->urc[j].month;            // month of bill (1-12)
    nir->bill_day[PRE_COOLING][count] = ndi->urc[j].day;              // day of the month for bill (1-31)
    nir->bill_consumption[PRE_COOLING][count] = (int)ndi->urc[j].usage;       // consumption in specified units
    nir->bill_degree_days[PRE_COOLING][count] = (int)ndi->urc[j].degree_days; // heating or cooling degree days
  }
  nir->billing_record_count[PRE_COOLING] = count;
}

static float prcn[MONTHS + 1], prdd[MONTHS + 1];

void neat_billing_adjustment(float adj[], int is, int units, int pass, int blapflg[], FILE *fp) {
  float acmin, acmax, avdcn[MONTHS + 2];
  float fbhtgcn[MONTHS + 1], pradcn[MONTHS + 1], totactual, totpred;
  float totddpred = 0., totddact, fnd, totdif, totddif;
  char *slab3[BILL_UNITS_KWH + 1];
  float unitconv[BILL_UNITS_KWH + 1];
  int l, m, lnno, ndip[MONTHS + 1];
  int mi, mf, mfe, ma, inday, enday, totndays;
  static int idop[MONTHS + 1];
  static int fdop[MONTHS + 1];
  static float degree_hours[COOLING + 1][MONTHS + 1];

  slab3[BILL_UNITS_THERMS] = "(Therms)";
  slab3[BILL_UNITS_KWH] = " (kWh)  ";

  unitconv[BILL_UNITS_THERMS] = MMBTU_PER_THERM;
  unitconv[BILL_UNITS_KWH] = MMBTU_PER_KWH;

  /* Determine last acceptable period, ie., nonzero dates, and number of days
   * in period  */

  ndip[JANUARY] = nir->bill_first_period_days[is];
  fdop[JANUARY] = doy(nir->bill_month[is][JANUARY], nir->bill_day[is][JANUARY]);
  idop[JANUARY] = fdop[JANUARY] - ndip[JANUARY] + 1;
  if (idop[JANUARY] <= 0)
    idop[JANUARY] += 365;
  for (l = 2; l <= MONTHS; l++) {
    fdop[l] = doy(nir->bill_month[is][l], nir->bill_day[is][l]);
    ndip[l] = fdop[l] - fdop[l - 1];
    if (ndip[l] <= 0)
      ndip[l] += 365;
    idop[l] = fdop[l] - ndip[l];
    if (idop[l] <= 0)
      idop[l] += 365;
    if (nir->bill_month[is][l] == 0 || nir->bill_day[is][l] == 0)
      break;
  }
  lnno = l - 1;
  avdcn[0] = avdcn[lnno + 1] = 1.e20f;

  /* Determine period of least and greatest average daily consumption. */

  ASSERT((float)ndip[1], sprintf(msg, "Need non zero factor"));
  avdcn[JANUARY] = (float)nir->bill_consumption[is][JANUARY] / (float)ndip[JANUARY];
  acmin = acmax = avdcn[JANUARY];
  for (l = 2; l <= lnno; l++) {

    ASSERT((float)ndip[l], sprintf(msg, "Need non zero factor"));
    avdcn[l] = (float)nir->bill_consumption[is][l] / (float)ndip[l];
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
    nir->balance_point_temp[NIGHT][HEATING][m] = 
    nir->balance_point_temp[NIGHT][COOLING][m] = 
    nir->balance_point_temp[DAY][HEATING][m] = 
    nir->balance_point_temp[DAY][COOLING][m] = nir->bill_degree_day_base[is];
    prcn[m] = prdd[m] = 0.;
  }
  totactual = totpred = totddpred = totddact = 0.;
  totndays = 0;
  if (nir->bill_base_load_consumption[is] < 0.0f)
    nir->bill_base_load_consumption[is] = acmin * 30.0f;

  /* Determine the average daily degree-days for each month */

  if (pass == FIRST_BILL_ADJUST)
    adjusted_monthly_degree_hours(nir->balance_point_temp, degree_hours);

  /* Subtract base load from billing data and determine predicted consumption
   * corresponding to billing period. */

  if (pass == FIRST_BILL_ADJUST) {

    if (is == PRE_HEATING) {
      if (fp) {
        fprintf(fp, "\n                   HEATING ENERGY CONSUMPTION COMPARISON \n\n");
        fprintf(fp, "     P e r i o d         C o n s u m p t i o n       D e g r e e  D a y s\n");
        fprintf(fp, "   End     Days in             %8.8s                  (Base  %2d F)\n", slab3[units], (int)(nir->bill_degree_day_base[is]));
      }
      // optional structure output

      if (nor) {
        STRCPY(nor->heat_comp_units, slab3[units]);
        nor->heat_dd_base = (int)nir->bill_degree_day_base[is];
      }
    } else {
      if (fp) {
        fprintf(fp, "\n                   COOLING ENERGY CONSUMPTION COMPARISON \n\n");
        fprintf(fp, "     P e r i o d         C o n s u m p t i o n       D e g r e e  D a y s\n");
        fprintf(fp, "   End     Days in             %8.8s                   (Base  %2d F)\n", slab3[units], (int)(nir->bill_degree_day_base[is]));
      }
      // optional structure output

      if (nor) {
        STRCPY(nor->cool_comp_units, slab3[units]);
        nor->cool_dd_base = (int)nir->bill_degree_day_base[is];
      }
    }
    if (fp)
      fprintf(fp, "     Date       Period       Actual      Predicted        Actual     Predicted\n");
  }

  for (m = 1; m <= MONTHS; m++) {

    ASSERT(cwd->days_in_month[m], sprintf(msg, "Need non zero factor"));
    degree_hours[is][m] /= (float)(cwd->days_in_month[m]);

    ASSERT(unitconv[units], sprintf(msg, "Need non zero factor"));
    if (is == PRE_HEATING)
      pradcn[m] = nir->htgmengy[PRE_RETROFIT][m] / unitconv[units] / (float)(cwd->days_in_month[m]);
    else
      pradcn[m] = nir->clgmengy[PRE_RETROFIT][m] / unitconv[units] / (float)(cwd->days_in_month[m]);
  }

  for (l = 1; l <= lnno; l++) { /* Step through periods */
    fbhtgcn[l] = (float)nir->bill_consumption[is][l] - (float)ndip[l] * nir->bill_base_load_consumption[is] / 30.0f;
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
      mfe = mf + MONTHS;
    for (m = mi; m < mfe; m++) {
      ma = m;
      if (m > MONTHS)
        ma = m - MONTHS;
      inday = cwd->days_in_year_for_month[ma];
      if (m == mi)
        inday = idop[l];
      if (ma < MONTHS)
        enday = cwd->days_in_year_for_month[ma + 1];
      else
        enday = 366;
      fnd = (float)(enday - inday);
      prcn[l] += pradcn[ma] * fnd;
      prdd[l] += degree_hours[is][ma] * fnd / 24.0f;
    }
    if (mi != mfe)
      fnd = (float)(fdop[l] - cwd->days_in_year_for_month[mf] + 1);
    else
      fnd = (float)(fdop[l] - idop[l] + 1);
    prcn[l] += pradcn[mf] * fnd;
    prdd[l] += degree_hours[is][mf] * fnd / 24.0f;
    totpred += prcn[l];
    totactual += fbhtgcn[l];
    totndays += ndip[l];
    totddpred += prdd[l];
    totddact += nir->bill_degree_days[is][l];
    if (pass == FIRST_BILL_ADJUST) {
      if (fp)
        fprintf(fp, "\n  %4d%2d/%2d%9d ", nir->bill_year[is][l], nir->bill_month[is][l], nir->bill_day[is][l], ndip[l]);
      if (fp)
        fprintf(fp, "      %7d      %7d", (int)(fbhtgcn[l] + .5), (int)(prcn[l] + .5));
      if (fp)
        fprintf(fp, "         %7d      %7d", nir->bill_degree_days[is][l], (int)(prdd[l] + .5));

      // optional structure output

      if (nor) {
        int cnt;
        if (is == PRE_HEATING) {
          cnt = nor->num_heat_comp;
          nor->heat_comp[cnt].index = cnt;
          nor->heat_comp[cnt].year = nir->bill_year[is][l];
          nor->heat_comp[cnt].month = nir->bill_month[is][l];
          nor->heat_comp[cnt].day = nir->bill_day[is][l];
          nor->heat_comp[cnt].period_days = ndip[l];
          nor->heat_comp[cnt].consump_act = (int)(fbhtgcn[l] + .5);
          nor->heat_comp[cnt].consump_pred = (int)(prcn[l] + .5);
          nor->heat_comp[cnt].dd_act = nir->bill_degree_days[is][l];
          nor->heat_comp[cnt].dd_pred = (int)(prdd[l] + .5);
          nor->num_heat_comp++;
        } else {
          cnt = nor->num_cool_comp;
          nor->cool_comp[cnt].index = cnt;
          nor->cool_comp[cnt].year = nir->bill_year[is][l];
          nor->cool_comp[cnt].month = nir->bill_month[is][l];
          nor->cool_comp[cnt].day = nir->bill_day[is][l];
          nor->cool_comp[cnt].period_days = ndip[l];
          nor->cool_comp[cnt].consump_act = (int)(fbhtgcn[l] + .5);
          nor->cool_comp[cnt].consump_pred = (int)(prcn[l] + .5);
          nor->cool_comp[cnt].dd_act = nir->bill_degree_days[is][l];
          nor->cool_comp[cnt].dd_pred = (int)(prdd[l] + .5);
          nor->num_cool_comp++;
        }
      }
    }
  }

  if (pass == FIRST_BILL_ADJUST) {
    if (fp)
      fprintf(fp, "\n\n  Total     %4d       %7ld      %7ld", totndays, (long)(totactual + .5), (long)(totpred + .5));
    if (fp)
      fprintf(fp, "         %7d      %7d", (int)(totddact + .5), (int)(totddpred + .5));
    if (totactual > .1) {

      ASSERT(totactual, sprintf(msg, "Need non zero factor"));
      totdif = -(totactual - totpred) / totactual * 100.0f;
      if (fp)
        fprintf(fp, "\n%% Difference                 %7ld%%", (long)(totdif + 0.5));
    }
    if (totddact > .1) {

      ASSERT(totddact, sprintf(msg, "Need non zero factor"));
      totddif = -(totddact - totddpred) / totddact * 100.0f;
      if (fp)
        fprintf(fp, "                     %7ld%%", (long)(totddif + 0.5));
    }
  }

  if (totpred > .0001) {
    adj[is] = totactual / totpred;
    if (pass == SECOND_BILL_ADJUST)
      blapflg[is]++;
  }
  nir->billing_record_count[is] = lnno;
  return;
}
