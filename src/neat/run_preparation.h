/***************************************************************************
* MODULE:	neat_prep.h            CREATED:	April 7, 1999 (renamed MJF 11/2018)
*
* AUTHOR:	Mark Fishbaugher  Fishbaugher and Associates  1999
*
* MDESC:	prototype declarations for neat_prep.c (previously named data22.c)
****************************************************************************/
#ifndef _NEAT_PREP_H
#define _NEAT_PREP_H

void translate_parms(void);
void translate_ndi(void);

void initialize_neat_measure_exclusion(void);
void initialize_billing(void);

void neat_preparation(void);

#endif /* _NEAT_PREP_H */
