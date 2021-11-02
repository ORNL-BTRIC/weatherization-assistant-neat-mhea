/***************************************************************************
 * MODULE:  json_m.h            CREATED:    November 2, 2018
 *
 * AUTHOR:  Mark Fishbaugher (mark@fishbaugher.com)
 *
 * MDESC:   Prototype functions for JSON helpers
 ****************************************************************************/
#ifndef _JSON_M_H
#define _JSON_M_H

void mhea_json_read(MDI *, cJSON *jtree, cJSON *jschema);
void mhea_json_echo_write(MDI *);
void mhea_json_result_write(MDI *, MOR *);

#endif /* _JSON_M_H */
