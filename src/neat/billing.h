/***************************************************************************
 * MODULE:       billing.h            CREATED:    June 8, 2020
 *
 * AUTHOR:       Mike Gettings
 *               Mark Fishbaugher  (mark@fishbaugher.com)
 *
 * MDESC:        
 ****************************************************************************/
#ifndef _N_BILLING_H
#define _N_BILLING_H

void manage_neat_billing_adjustments(int *flg);
void translate_neat_bil(void);
void neat_billing_adjustment(float adj[], int is, int iu, int pass, int blapflg[], FILE *fp);

#define FIRST_BILL_ADJUST 1
#define SECOND_BILL_ADJUST 2

#endif /* _N_BILLING_H */