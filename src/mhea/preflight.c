/***************************************************************************
* MODULE:       pre_flight.c            CREATED:     May 31, 2019
*
* AUTHOR:       Mark Fishbaugher (mark@fishbaugher.com)
*
* MDESC:        Apply business rules to the complted MDI input structure
*               to make sure there are no inconsistencies or missing data
****************************************************************************/
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "wa_engine.h"

void mdi_check(MDI *top) {

  ASSERT(top, sprintf(msg, "Must have non null MDI structure to check"));

  ASSERT(strstr(strupr(top->gnl.audit_type), "MHEA"), sprintf(msg, "Audit type needs to be 'MHEA'"));

  ASSERT(top->wth.file, sprintf(msg, "Audit must specify a weather file name"));

  ASSERT(top->inf.post_inf_cfm && top->inf.post_inf_pa, sprintf(msg, "Audit needs post retrofit infiltration CFM and Pa values"));

  {
    int no_htg_duct = (top->htg.duct_location == 0 || top->htg.duct_location == DL_NONE);
    int no_clg_duct = (top->clg.duct_location == 0 || top->clg.duct_location == DL_NONE);
    ASSERT(!(top->inf.evaluate_duct_sealing && no_htg_duct && no_clg_duct),
           sprintf(msg, "Duct sealing selected, but there are no ducts in either htg or clg"));
  }

  // additions must have at least floor, roof, and wall
  if (mdi->afl.length > 1.0 && mdi->afl.width > 1.0) {
    ASSERT(mdi->awl.height_max, sprintf(msg,"Incomplete Addition, need wall information"));         // proxy
    ASSERT(mdi->arf.joist_size, sprintf(msg,"Incomplete Addition, need roof information"));         // proxy
  }

  int num_ducted_systems = 0;
  if (mdi->htg.equip_type == ET_FURNACE || 
    mdi->htg.equip_type == ET_HEATPUMP ||
    mdi->ht2.equip_type == ET_FURNACE || 
    mdi->ht2.equip_type == ET_HEATPUMP)
    num_ducted_systems++;

  if (mdi->clg.equip_type == CE_CENTRALAC || 
    mdi->clg.equip_type == CE_EVAPORATIVE ||
    mdi->clg.equip_type == CE_HEATPUMP ||
    mdi->cl2.equip_type == CE_CENTRALAC || 
    mdi->cl2.equip_type == CE_EVAPORATIVE ||
    mdi->cl2.equip_type == CE_HEATPUMP)
    num_ducted_systems++;

  if (mdi->inf.evaluate_duct_sealing == YES) {
    ASSERT(num_ducted_systems > 0, sprintf(msg, "Must have a ducted heating or cooling system to evaluate duct sealing"));
  }

}