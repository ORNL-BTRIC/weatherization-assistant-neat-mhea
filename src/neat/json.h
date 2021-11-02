/***************************************************************************
 * MODULE:  json_n.h            CREATED:    November 2, 2018
 *
 * AUTHOR:  Mark Fishbaugher (mark@fishbaugher.com)
 *
 * MDESC:   Prototype functions for JSON helpers
 ****************************************************************************/
#ifndef _JSON_N_H
#define _JSON_N_H

void neat_json_read(NDI *, cJSON *jtree, cJSON *jschema);
void neat_json_echo_write(NDI *);
void neat_json_result_write(NDI *, NOR *);

#endif /* _JSON_N_H */
