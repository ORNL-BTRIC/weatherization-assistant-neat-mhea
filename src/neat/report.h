/***************************************************************************
 * MODULE:       report.h            CREATED:    April 23, 1999 (Nov 5,2018 migration)
 *
 * AUTHOR:       Mike Gettings
 *               Mark Fishbaugher  (mark@fishbaugher.com)
 *
 * MDESC:        report routines called from neat.c
 ****************************************************************************/
#ifndef _REPORT_H
#define _REPORT_H

void report_header(FILE *fp, int adjflg);
void neat_results(FILE *fp);
void report_materials(FILE *fp);
void report_energy(FILE *fp);
void report_measure_materials(void);

void add_neat_message(char *msg);
void fill_ceiling_cavity_names(char *material_name, char *type_name, int rmc_index, int nc);

#endif /* _REPORT_H */
