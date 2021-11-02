/***************************************************************************
* MODULE:       pre_flight.c            CREATED:     May 31, 2019
*
* AUTHOR:       Mark Fishbaugher (mark@fishbaugher.com)
*
* MDESC:        Apply business rules to the complted BLD input structure
*               to make sure there are no inconsistencies or missing data
****************************************************************************/
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "wa_engine.h"

void ndi_check(NDI *top) {

  int num_primary_heating = 0;
  int num_ducted_systems = 0;
  float sum_percent_heated = 0.0f;

  for (int i = 0; i < top->num_htg; i++) {
    sum_percent_heated += top->htg[i].percent_heat_supplied;
    if (top->htg[i].primary == YES) {
      num_primary_heating++;
      if (top->htg[i].system_type == HE_HEAT_PUMP)
        ASSERT(top->htg[i].hspf || top->htg[i].year_manufactured, sprintf(msg, "Audit needs either HSPF or Year for primary heat pump"));
    }
  }
  ASSERT(sum_percent_heated <= 100.0f, sprintf(msg, "More than 100 percent heat supplied"));
  ASSERT(num_primary_heating == 1, sprintf(msg, "Need exactly one primary heating system, have %d", num_primary_heating));
  ASSERT(top->htg[PRIMARY].primary == YES, sprintf(msg, "The first heating system must be the primary system"));

  for (int nc = 0; nc < ndi->num_htg; nc++) {
    if (top->htg[nc].system_type == HE_FORCED_AIR_FURNACE || 
      top->htg[nc].system_type == HE_GRAVITY_FURNACE ||
      top->htg[nc].system_type == HE_OTHER ||
      top->htg[nc].system_type == HE_HEAT_PUMP)
      num_ducted_systems++;
  }

  for (int nc = 0; nc < top->num_clg; nc++) {
    if (top->clg[nc].system_type == CET_CENTRAL || 
      top->clg[nc].system_type == CET_EVAPORATIVE ||
      top->clg[nc].system_type == CET_HEATPUMP)
      num_ducted_systems++;
  }

  if (top->inf.evaluate_duct_sealing == YES) {
    ASSERT(num_ducted_systems > 0, sprintf(msg, "Must have a ducted heating or cooling system to evaluate duct sealing"));
  }

  // TODO other completness checks, including data dependent required fields.  In the
  // JSON parsing, only the lowest common set of required fields is checked.  If a field is
  // required ONLY IF another field has a certain value, then that biz logic needs to be added
  // here, otherwise we are depending on deeper level asserts()

  // TODO  if billing adjust, assert either heating or cooling billing data

  // TODO make sure sum of cooling system area is < = to floor area

  // TODO compare year_manufactured fields to CURRENT YEAR + 1 max
}