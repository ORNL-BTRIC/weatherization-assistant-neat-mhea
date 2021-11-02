/***************************************************************************
 * MODULE:  struct.h            CREATED:    November 5, 1998
 *
 * AUTHOR:  Mark Fishbaugher
 *
 * MDESC:   common definitions for both NEAT and MHEA structures.  mostly
 *           common string lengths, counts and the like
 ****************************************************************************/
#ifndef _RESULT_H
#define _RESULT_H

typedef struct {
  char fuel_name[FUEL_LEN + 1];
  float fuel_cost;
  char fuel_cost_units[FUEL_LEN + 1];
  float fuel_cost_per_mmbtu;
} USED_FUEL;


#endif // _RESULT_H
