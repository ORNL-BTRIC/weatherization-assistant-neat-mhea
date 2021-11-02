/***************************************************************************
 * MODULE:       neat.h            CREATED:    April 23, 1999 (Nov 5,2018 migration)
 * UPDATED  MJF 1/2020
 *
 * AUTHOR:       Mike Gettings
 *               Mark Fishbaugher  (mark@fishbaugher.com)
 *
 * MDESC:        External function prototypes for this module. Previously
 *               Named load37.h
 ****************************************************************************/
#ifndef _NEAT_H
#define _NEAT_H

void run_neat(void);    // main NEAT call

void neat_energy_use(char *run_title, int phase);
float get_latent_infil_load(float tdb, float twb, float cfm);
void neat_solar_storage(float, float, float, float, float, float *);

#endif /* _NEAT_H */
