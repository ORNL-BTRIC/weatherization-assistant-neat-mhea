/***************************************************************************
 * MODULE:  json_helper.h    CREATED:    November 2, 2018
 *
 * AUTHOR:  Mark Fishbaugher (mark@fishbaugher.com)
 *
 * MDESC:   Prototype functions for JSON helpers
 ****************************************************************************/
#ifndef _JSON_HELPER_H
#define _JSON_HELPER_H

#define MAX_FIELDNAME_LEN 80

char *strlwr(char *str);
char *strupr(char *str);
char *replace_char(char *str, char find, char replace);

void json_schema_validate_input(char *schema_file);
void json_schema_validate_output(char *schema_file);
void assert_fail_json_output(char *fail_message, char *fail_location);

int str_assign(size_t target_size, char *target, cJSON *jitem, char *section, char *fieldname);
int boo_assign(int *target, cJSON *jitem, char *section, char *fieldname);
void WA_AddBoolToObject(cJSON *const object, const char *const name, enum LOGICAL boolean);
void WA_AddNumToObjectNoZero(cJSON * const object, const char * const name, const double number);
void WA_AddStrToObjectNoNull(cJSON * const object, const char * const name, char *str);
int enu_assign(int *target, cJSON *jitem, char *section, char *fieldname, cJSON *jschema);
int int_assign(int *target, cJSON *jitem, char *section, char *fieldname);
int lng_assign(long *target, cJSON *jitem, char *section, char *fieldname);
int flt_assign(float *target, cJSON *jitem, char *section, char *fieldname);

cJSON *parse_json_file(const char *filename);
const char *get_filename_ext(const char *filename);
void write_results_to_file(char *output);
void write_json_echo_to_file(char *output);

// clang-format off   so our braces line up in the editor

// macros for JSON parsing into our ndi and mdi structures

// strings
#define STR_ASSIGN(json_section, target, fieldname) \
  ({ \
    if (str_assign(sizeof((target)), (target), jleaf, #json_section, #fieldname)) \
  continue; \
  })
#define  J_STR_ASSIGN(json_section, struct_section, fieldname) STR_ASSIGN(json_section, (top->struct_section.fieldname), fieldname);
#define JI_STR_ASSIGN(json_section, struct_section, fieldname) STR_ASSIGN(json_section, (top->struct_section[i].fieldname), fieldname);


// boolean values
#define BOO_ASSIGN(json_section, target, fieldname) \
  ({ \
    if (boo_assign((int *)(target), jleaf, #json_section, #fieldname)) \
  continue; \
  })
#define  J_BOO_ASSIGN(json_section, struct_section, fieldname) BOO_ASSIGN(json_section, (int *)&(top->struct_section.fieldname), fieldname)
#define JI_BOO_ASSIGN(json_section, struct_section, fieldname) BOO_ASSIGN(json_section, (int *)&(top->struct_section[i].fieldname), fieldname)


// integer values (not enumerators or boolean)
#define INT_ASSIGN(json_section, target, fieldname) \
  ({ \
    if (int_assign((target), jleaf, #json_section, #fieldname)) \
  continue; \
  })
#define  J_INT_ASSIGN(json_section, struct_section, fieldname) INT_ASSIGN(json_section, &(top->struct_section.fieldname), fieldname);
#define JI_INT_ASSIGN(json_section, struct_section, fieldname) INT_ASSIGN(json_section, &(top->struct_section[i].fieldname), fieldname);


// enumeration selctions with cast to integer values
#define ENU_ASSIGN(json_section, target, fieldname) \
  ({ \
    if (enu_assign((int *)(target), jleaf, #json_section, #fieldname, jschema)) \
  continue; \
  })
#define  J_ENU_ASSIGN(json_section, struct_section, fieldname) ENU_ASSIGN(json_section, (int *)&(top->struct_section.fieldname), fieldname);
#define JI_ENU_ASSIGN(json_section, struct_section, fieldname) ENU_ASSIGN(json_section, (int *)&(top->struct_section[i].fieldname), fieldname);


// long integer values
#define LNG_ASSIGN(json_section, target, fieldname) \
  ({ \
    if (lng_assign((target), jleaf, #json_section, #fieldname)) \
  continue; \
  })
#define  J_LNG_ASSIGN(json_section, struct_section, fieldname) LNG_ASSIGN(json_section, &(top->struct_section.fieldname), fieldname);
#define JI_LNG_ASSIGN(json_section, struct_section, fieldname) LNG_ASSIGN(json_section, &(top->struct_section[i].fieldname), fieldname);


// float values
#define FLT_ASSIGN(json_section, target, fieldname) \
  ({ \
    if (flt_assign((target), jleaf, #json_section, #fieldname)) \
  continue; \
  })
#define  J_FLT_ASSIGN(json_section, struct_section, fieldname) FLT_ASSIGN(json_section, &(top->struct_section.fieldname), fieldname);
#define JI_FLT_ASSIGN(json_section, struct_section, fieldname) FLT_ASSIGN(json_section, &(top->struct_section[i].fieldname), fieldname);


// Matched sets here for balance of braces.  This is for singleton sections/objects in the JSON
#define J_SEC_BEG(section_name) \
  { \
    char section[40]; \
    STRCPY(section, #section_name); \
    if (strcmp(strlwr(jbranch->string), #section_name) == 0) { \
      ASSERT(cJSON_IsObject(jbranch), sprintf(msg, "Section: %s is not a cJSON object", #section_name)); \
      cJSON_ArrayForEach(jleaf, jbranch) {

#define J_SEC_END() \
    if (cmds.debug_level & D_NORMAL || \
      cmds.debug_level & D_JSON_INPUT_DETAIL) \
      fprintf(stderr, "\nNotice, Extra JSON field: %s FOUND IN SECTION: %s", jleaf->string, section); \
  } \
  } \
  }

// Another matched set Beg/End for arrays of object elements in the JSON
#define JI_ARR_BEG(section_name, record_count) \
  { \
    char section[40]; \
    STRCPY(section, #section_name); \
    if (strcmp(strlwr(jbranch->string), section) == 0) { \
      ASSERT(cJSON_IsArray(jbranch), sprintf(msg, "JSON section: %s is not an array", section)); \
      record_count = cJSON_GetArraySize(jbranch); \
      int i = 0; \
      cJSON_ArrayForEach(jbranch2, jbranch) { \
        cJSON_ArrayForEach(jleaf, jbranch2) {

#define JI_ARR_END() \
    if (cmds.debug_level & D_NORMAL || \
      cmds.debug_level & D_JSON_INPUT_DETAIL) \
      fprintf(stderr, "\nNotice, Extra JSON field: %s FOUND IN SECTION: %s:%d", jleaf->string, section, i); \
    } \
  i++; \
  } \
  } \
  }

// clang-format on

#endif // _JSON_HELPER_H
