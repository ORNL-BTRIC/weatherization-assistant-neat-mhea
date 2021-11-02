/***************************************************************************
* MODULE:       json_helper.c            CREATED:     November 2, 2018
*
* AUTHOR:       Mark Fishbaugher (mark@fishbaugher.com)
*
* MDESC:        Helper functions for JSON input and output
****************************************************************************/
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "wa_engine.h"

/// Convert string to lower case.  Not standard in all compilers, so we define our own version.

char *strlwr(char *str) {
  unsigned char *p = (unsigned char *)str;

  while (*p) {
    *p = tolower((unsigned char)*p);
    p++;
  }
  return str;
}

/// Convert string to upper case.  Not standard in all compilers, so we define our own version.

char *strupr(char *str) {
  unsigned char *p = (unsigned char *)str;

  while (*p) {
    *p = toupper((unsigned char)*p);
    p++;
  }
  return str;
}

char *replace_char(char *str, char find, char replace){
  char *current_pos = strchr(str, find);
  while (current_pos){
    *current_pos = replace;
    current_pos = strchr(current_pos, find);
  }
  return str;
}

void json_schema_validate_input(char *schema_file) {
    char ajv_cmd[1024];
    // run the validator ajv-cli redirecting both stdout and stderr to our message file
    sprintf(ajv_cmd, "ajv -s %s -d %s --errors=text --spec=draft7 --strict=false > " JSON_SCHEMA_VALIDATION_MESSAGE_FILE " 2>&1", schema_file, cmds.input_file_path);
    //The following line produces JSON output which we don't want
    //sprintf(ajv_cmd, "ajv -s %s -d %s > " JSON_SCHEMA_VALIDATION_MESSAGE_FILE " 2>&1", schema_file, cmds.input_file_path);
    if (cmds.debug_level & D_NORMAL) fprintf(stderr, "\nValidate JSON INPUT with:%s", ajv_cmd);
    remove(JSON_SCHEMA_VALIDATION_MESSAGE_FILE);
    ASSERT(system(ajv_cmd) == 0, sprintf(msg, JSON_INPUT_SCHEMA_FAIL_MESSAGE));
    ASSERT(remove(JSON_SCHEMA_VALIDATION_MESSAGE_FILE) == 0, sprintf(msg, "Failed to clean up:%s", JSON_SCHEMA_VALIDATION_MESSAGE_FILE));
}

void json_schema_validate_output(char *schema_file) {
    char ajv_cmd[1024];

    if (strcmp(cmds.output_file_path, STD_OUTPUT) == 0) {
      fprintf(stderr, "\nCan not validate schema for the standard output");
    } else {
      // run the validator ajv-cli redirecting both stdout and stderr to our message file
      sprintf(ajv_cmd, "ajv -s %s -d %s --errors=text --spec=draft7 --strict=false > " JSON_SCHEMA_VALIDATION_MESSAGE_FILE " 2>&1", schema_file, cmds.output_file_path);
      //The following line produces JSON output which we don't want
      //sprintf(ajv_cmd, "ajv -s %s -d %s > " JSON_SCHEMA_VALIDATION_MESSAGE_FILE " 2>&1", schema_file, cmds.input_file_path);
      if (cmds.debug_level & D_NORMAL) fprintf(stderr, "\nValidate JSON OUTPUT with:%s", ajv_cmd);
      remove(JSON_SCHEMA_VALIDATION_MESSAGE_FILE);
      ASSERT(system(ajv_cmd) == 0, sprintf(msg, JSON_OUTPUT_SCHEMA_FAIL_MESSAGE));
      ASSERT(remove(JSON_SCHEMA_VALIDATION_MESSAGE_FILE) == 0, sprintf(msg, "Failed to clean up:%s", JSON_SCHEMA_VALIDATION_MESSAGE_FILE));
    }

}

// All program exits other than return(EXIT_SUCCESS) should be done through
// the ASSERT() macro, which calls this function to spit out the
// JSON structured failure information,  See #34 also #121
void assert_fail_json_output(char *fail_message, char *fail_location) {
  cJSON *jroot = NULL; // the JSON tree echo of our NEAT_RESULTS structure
  //cJSON *jdata = NULL;
  cJSON *jarray = NULL;
  cJSON *jitem = NULL;
  FILE *out_file = NULL;

  if (strcmp(cmds.output_file_path, STD_OUTPUT) == 0) {
    out_file = stdout;
  } else {
    out_file = fopen(cmds.output_file_path, "ab");    // append any existing output
    assert((out_file));      // NOTE, this is called by ASSERT() so avoid race condition
  }

  jroot = cJSON_CreateObject();

  // Only return a top level JSON object that can be packaged however the calling process decides #121 
  //cJSON_AddFalseToObject(jroot, "success");
  //cJSON_AddItemToObject(jroot, "data", jdata = cJSON_CreateObject());

  if (cmds.run_neat) {
    cJSON_AddStringToObject(jroot, "audit_type", "NEAT");
  } else if (cmds.run_mhea) {
    cJSON_AddStringToObject(jroot, "audit_type", "MHEA");
  } else {
    cJSON_AddStringToObject(jroot, "audit_type", "UNKNOWN");
  }

  // here is a batch of run meta data to tag onto the top of the JSON
  // clang-format off
  if (strlen(cmds.run_identifier)) cJSON_AddStringToObject(jroot,    "run_identifier",     cmds.run_identifier);
  cJSON_AddStringToObject(jroot,    "message",       fail_message);
  cJSON_AddStringToObject(jroot,    "location",      fail_location);

  // add to our JSON failure message output if we have failed on JSON INPUT schema validation
  if (strstr(fail_message, JSON_INPUT_SCHEMA_FAIL_MESSAGE)) {
    char line[1024];    // hopefully no single lines bigger than this

    FILE *json_fail_messages_file = NULL;
    json_fail_messages_file = fopen(JSON_SCHEMA_VALIDATION_MESSAGE_FILE, "rb");
    assert(json_fail_messages_file);   // NOTE, this is called by ASSERT() so avoid race condition

    cJSON_AddItemToObject(jroot,   "input_schema_fail_messages",     jarray = cJSON_CreateArray());

    while (fgets(line, sizeof(line), json_fail_messages_file)) {
      line[strcspn(line, "\r\n")] = 0;    // get rid of trailing cr lf crlf
      cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
      cJSON_AddStringToObject(jitem, "message", line);
    }
    fclose(json_fail_messages_file);
  }

  // add to our JSON failure message output if we have failed on JSON OUTPUT schema validation
  if (strstr(fail_message, JSON_OUTPUT_SCHEMA_FAIL_MESSAGE)) {
    char line[1024];    // hopefully no single lines bigger than this

    FILE *json_fail_messages_file = NULL;
    json_fail_messages_file = fopen(JSON_SCHEMA_VALIDATION_MESSAGE_FILE, "rb");
    assert(json_fail_messages_file);   // NOTE, this is called by ASSERT() so avoid race condition

    cJSON_AddItemToObject(jroot,   "output_schema_fail_messages",     jarray = cJSON_CreateArray());

    while (fgets(line, sizeof(line), json_fail_messages_file)) {
      line[strcspn(line, "\r\n")] = 0;    // get rid of trailing cr lf crlf
      cJSON_AddItemToArray(jarray, jitem = cJSON_CreateObject());
      cJSON_AddStringToObject(jitem, "message", line);
    }
    fclose(json_fail_messages_file);
  }

  time_t rawtime;
  struct tm *info;
  char time_buffer[80];
  time(&rawtime);
  info = localtime(&rawtime);
  strftime(time_buffer, 80, "%c", info);
  cJSON_AddStringToObject(jroot, "run_timestamp", time_buffer);

  //clang-format on

  char *out = NULL;
  if (cmds.format_json_output)
    out = cJSON_Print(jroot); // allocates the formatted JSON output string and returns it
  else
    out = cJSON_PrintUnformatted(jroot); // allocates the un-formatted JSON output string and returns it
  fprintf(out_file, "%s\n", out);        // the whole structure recurses and goes out stdout
  fclose(out_file);
  if (out) free(out);             // done with monster out string
  if (jroot) cJSON_Delete(jroot); // pretty sure this cleans up the sub cJSON objects
}

/// Assigns cJSON string to a target string

int str_assign(size_t target_size, char *target, cJSON *jitem, char *section, char *fieldname) {
  char lfn[MAX_FIELDNAME_LEN];
  STRCPY(lfn, fieldname);
  if (strcmp(strlwr(jitem->string), strlwr(lfn)) == 0) // in cJSON, the string field matches the key name
  {
    ASSERT(!cJSON_IsInvalid(jitem), sprintf(msg, "%s:%s is not valid JSON: %s", section, lfn, jitem->string));
    if (cmds.debug_level & D_JSON_INPUT_DETAIL) fprintf(stderr, "\nstr_assign:%s:%s", section, lfn);
    if (!cJSON_IsNull(jitem))     // otherwise the target string stays a null with [0] = "\0"
    {
      ASSERT(cJSON_IsString(jitem), sprintf(msg, "%s:%s is not a string", section, lfn));
      char *strsource = cJSON_GetStringValue(jitem);
      // note the target_size is the result of a sizeof() operator on a string which
      // returns the array size including the room for the null terminator, so sizeof(char[4]) = 4 regardless of contents
      //fprintf(stderr, "\nSection: %s Field:%s SizeT:%lu SizeS:%lu", section, fieldname, target_size, strlen(strsource));
      target_size--;            // passed from sizeof() operator which DOES include space for null terminator in target string
      strncpy(target, strsource, target_size);      // upto the target size minus space for null terminator
      target[target_size] = '\0';                   // always necessary when using STRNCPY()
      if (cmds.debug_level & D_JSON_INPUT_DETAIL) fprintf(stderr, " = %s", target);
    } else {
      if (cmds.debug_level & D_JSON_INPUT_DETAIL) fprintf(stderr, " = NULL");
    }
    return 1;
  }
  return 0;
}

/// Assigns cJSON string to a target boolean.  The boolean is an integer enum LOGICAL that matches the cJSON boolean
/// with the addition of the NA enumeration to indicate the boolean field was received as a null or anything other 
/// than a true or false value

int boo_assign(int *target, cJSON *jitem, char *section, char *fieldname) {
  char lfn[MAX_FIELDNAME_LEN];
  STRCPY(lfn, fieldname);
  if (strcmp(strlwr(jitem->string), strlwr(lfn)) == 0) // in cJSON, the string field matches the key name
  {
    if (cmds.debug_level & D_JSON_INPUT_DETAIL) fprintf(stderr, "\nboo_assign:%s:%s", section, lfn);
    if (cJSON_IsTrue(jitem)) {
      *target = YES;
      if (cmds.debug_level & D_JSON_INPUT_DETAIL) fprintf(stderr, " = YES");
    } else if (cJSON_IsFalse(jitem)) {
      *target = NO;
      if (cmds.debug_level & D_JSON_INPUT_DETAIL) fprintf(stderr, " = NO");
    } else if (cJSON_IsNull(jitem)) {
      *target = NA;           // does not satisfy a required boolean field
      if (cmds.debug_level & D_JSON_INPUT_DETAIL) fprintf(stderr, " = NA");
    }else {
      ASSERT( 0, sprintf(msg, "%s:%s Broken Boolean input: %s", section, lfn, jitem->string));
    }
    return 1;
  }
  return 0;
}

// outputs the approproate cJSON object based on the WA LOGICAL enumeration
// the case on this function name matches the standard in cjson for better readability
void WA_AddBoolToObject(cJSON * const object, const char * const name, enum LOGICAL boolean) {
  switch (boolean){
    case NO:
      cJSON_AddFalseToObject(object, name);
      break;
    case YES:
      cJSON_AddTrueToObject(object, name);
      break;
    case NA:
      cJSON_AddNullToObject(object, name);
      break;
    default:
      ASSERT(0, sprintf(msg, "Broken boolean on cJSON add"));
  }
}

// conditionally outputs a number only if non zero
// the case on this function name matches the standard in cjson for better readability
void WA_AddNumToObjectNoZero(cJSON * const object, const char * const name, const double number) {
  if (number)
    cJSON_AddNumberToObject(object, name, number);
}

// conditionally outputs a string only if non null
// the case on this function name matches the standard in cjson for better readability
void WA_AddStrToObjectNoNull(cJSON * const object, const char * const name, char *str) {
  if (strlen(str))
    cJSON_AddStringToObject(object, name, str);
}

/// Lookup the enumeration id for the given schema, section, and field name
static int enu_schema_id_lookup(cJSON *jschema, char *section, char *fieldname, char *look_for) {
  int index = 0;
  char key[80];
  cJSON *jenum;
  cJSON *jfield;
  cJSON *jleaf;

  sprintf(key, "#/%s/%s", section, fieldname);
  if (cmds.debug_level & D_JSON_INPUT_DETAIL) fprintf(stderr, "\nenu_lookup:%s:%s", key, look_for);
  jfield = cJSON_GetParentObjectContainingString(jschema, key);
  ASSERT(jfield, sprintf(msg, "No identifier %s found in schema", key));  
  jenum = cJSON_GetObjectItem(jfield, "enum");

  ASSERT(jenum, sprintf(msg, "No enum element found in schema for %s", key));
  ASSERT(cJSON_IsArray(jenum), sprintf(msg, "Schema Enumeration:%s is not an array", key));
  cJSON_ArrayForEach(jleaf, jenum) {
    if (cJSON_IsNumber(jleaf)) {
      index = (int) jleaf->valuedouble;
    }
    if (cJSON_IsString(jleaf)) {
      if (cmds.debug_level & D_JSON_INPUT_DETAIL) fprintf(stderr, "\nLooking at %s", cJSON_GetStringValue(jleaf));
      if (strcmp(strlwr(cJSON_GetStringValue(jleaf)), strlwr(look_for)) == 0) {
        if (cmds.debug_level & D_JSON_INPUT_DETAIL) fprintf(stderr, "\nFound enumeration :%d", index);
        ASSERT(index, sprintf(msg, "No numeric enum element found in schema for %s", key));
        return index;   // assumes that the index integer just preceedes the associated string
      }
    }
  }
  ASSERT(0, sprintf(msg, "Enumeration lookup %s:%s failed", key, look_for));
}

/// Assigns cJSON string to a target integer

int enu_assign(int *target, cJSON *jitem, char *section, char *fieldname, cJSON *jschema) {
  char lfn[MAX_FIELDNAME_LEN];
  STRCPY(lfn, fieldname);
  if (strcmp(strlwr(jitem->string), strlwr(lfn)) == 0) // in cJSON, the string field matches the key name
  {
    if (cmds.debug_level & D_JSON_INPUT_DETAIL) fprintf(stderr, "\nenu_assign:%s:%s", section, lfn);
    if (!cJSON_IsNull(jitem)) {
      if (cJSON_IsNumber(jitem)) {
        *target = (int)jitem->valuedouble;
        if (cmds.debug_level & D_JSON_INPUT_DETAIL) fprintf(stderr, " = %d", (int)jitem->valuedouble);
      } else if (cJSON_IsString(jitem)) {
        if (cmds.debug_level & D_JSON_INPUT_DETAIL) fprintf(stderr, " = %s", cJSON_GetStringValue(jitem));
        *target = enu_schema_id_lookup(jschema, section, lfn, cJSON_GetStringValue(jitem));
        if (cmds.debug_level & D_JSON_INPUT_DETAIL) fprintf(stderr, " = %d", *target);
      } else {
        ASSERT( 0, sprintf(msg, "%s:%s enumeration is not a number or string", section, lfn));
      }
    } else {
      if (cmds.debug_level & D_JSON_INPUT_DETAIL) fprintf(stderr, " = NULL");
    }
    return 1;
  }
  return 0;
}

int int_assign(int *target, cJSON *jitem, char *section, char *fieldname) {
  char lfn[MAX_FIELDNAME_LEN];
  STRCPY(lfn, fieldname);
  if (strcmp(strlwr(jitem->string), strlwr(lfn)) == 0) // in cJSON, the string field matches the key name
  {
    if (cmds.debug_level & D_JSON_INPUT_DETAIL) fprintf(stderr, "\nint_assign:%s:%s", section, lfn);
    if (!cJSON_IsNull(jitem)) {
      ASSERT(cJSON_IsNumber(jitem), sprintf(msg, "%s:%s is not a number", section, lfn));
      int value = (int)jitem->valuedouble;
      *target = value;
      if (cmds.debug_level & D_JSON_INPUT_DETAIL) fprintf(stderr, " = %d", value);
    } else {
      if (cmds.debug_level & D_JSON_INPUT_DETAIL) fprintf(stderr, " = NULL");
    }
    return 1;
  }
  return 0;
}

/// Assigns cJSON string to a target long

int lng_assign(long *target, cJSON *jitem, char *section, char *fieldname) {
  char lfn[MAX_FIELDNAME_LEN];
  STRCPY(lfn, fieldname);
  if (strcmp(strlwr(jitem->string), strlwr(lfn)) == 0) // in cJSON, the string field matches the key name
  {
    if (cmds.debug_level & D_JSON_INPUT_DETAIL) fprintf(stderr, "\nlng_assign:%s:%s", section, lfn);
    if (!cJSON_IsNull(jitem)) {
      ASSERT(cJSON_IsNumber(jitem), sprintf(msg, "%s:%s is not a number", section, lfn));
      long value = (long)jitem->valuedouble;
      *target = value;
      if (cmds.debug_level & D_JSON_INPUT_DETAIL) fprintf(stderr, " = %ld", value);
    } else {
      if (cmds.debug_level & D_JSON_INPUT_DETAIL) fprintf(stderr, " = NULL");
    }
    return 1;
  }
  return 0;
}

/// Assigns cJSON string to a target long

int flt_assign(float *target, cJSON *jitem, char *section, char *fieldname) {
  char lfn[MAX_FIELDNAME_LEN];
  STRCPY(lfn, fieldname);
  if (strcmp(strlwr(jitem->string), strlwr(lfn)) == 0) // in cJSON, the string field matches the key name
  {
    if (cmds.debug_level & D_JSON_INPUT_DETAIL) fprintf(stderr, "\nflt_assign:%s:%s", section, lfn);
    if (!cJSON_IsNull(jitem)) {
      ASSERT(cJSON_IsNumber(jitem), sprintf(msg, "%s:%s is not a number", section, lfn));
      float value = (float)jitem->valuedouble;
      *target = value;
      if (cmds.debug_level & D_JSON_INPUT_DETAIL) fprintf(stderr, " = %f", value);
    } else {
      if (cmds.debug_level & D_JSON_INPUT_DETAIL) fprintf(stderr, " = NULL");
    }
    return 1;
  }
  return 0;
}

/// Read the JSON file returning an array of characters.  Uses a simple fread() along with various asserts. Allocates space for
/// the returned content string which the calling function should free()

static char *read_json_file(const char *filepath) {
  FILE *in_file = NULL;
  long length = 0;
  char *content = NULL;
  size_t read_chars = 0;

  if (strcmp(filepath, STD_INPUT) == 0) {
    in_file = stdin;
  } else {
    in_file = fopen(filepath, "rb");
    ASSERT(in_file, sprintf(msg, "Failed to open the input json file: %s code:%d:%s", filepath, errno, strerror(errno)));
  }

  ASSERT(fseek(in_file, 0, SEEK_END) == 0, sprintf(msg, "Input file did not seek to end in file: %s", filepath));
  length = ftell(in_file);
  ASSERT(length >= 0, sprintf(msg, "Input file length using ftell() failed on file: %s", filepath));
  ASSERT(fseek(in_file, 0, SEEK_SET) == 0, sprintf(msg, "Input file did not seek to beginning on file: %s", filepath));
  ASSERT((content = (char *)malloc((size_t)length + sizeof(""))),
         sprintf(msg, "Failed to allocate memory for input file buffer"));

  // read the file into memory
  read_chars = fread(content, sizeof(char), (size_t)length, in_file);
  if ((long)read_chars != length) {
    if (content)
      free(content);
    if (in_file)
      fclose(in_file);
    ASSERT(FALSE, sprintf(msg, "Failed to read whole file: %s", filepath));
  } else {
    content[read_chars] = '\0'; // tag the end of content
    if (in_file)
      fclose(in_file);
    return content;
  }
}

// Send the json echo output to a file
void write_json_echo_to_file(char *output) {
  FILE *out_file;

  ASSERT(cmds.input_echo_file_path, sprintf(msg, "Must have an output JSON results file path specified"));

  if (strcmp(cmds.input_echo_file_path, STD_OUTPUT) == 0) {
    out_file = stdout;
  } else {
    out_file = fopen(cmds.input_echo_file_path, "wb");
    ASSERT(out_file, sprintf(msg, "Failed to open the input echo json file:%s code:%d:%s", cmds.input_echo_file_path, errno, strerror(errno)));
  }
  fprintf(out_file, "%s\n", output);         // the whole structure recurses and goes to the specified out_file
  fclose(out_file);
}

// Send the JSON output string of the results structure to a file
void write_results_to_file(char *output){
  FILE *out_file = NULL;

  ASSERT(cmds.output_file_path, sprintf(msg, "Must have an output JSON results file path specified"));

  if (strcmp(cmds.output_file_path, STD_OUTPUT) == 0) {
    out_file = stdout;
  } else {
    out_file = fopen(cmds.output_file_path, "wb");
    ASSERT(out_file, sprintf(msg, "Failed to open the json results output file: %s code:%d:%s", cmds.output_file_path, errno, strerror(errno)));
  }
  fprintf(out_file, "%s\n", output);        // the whole structure recurses and goes out stdout
  fclose(out_file);
}

/// Reads in a JSON file and returns the parsed cJSON linked list structure. The calling function must cleanup
/// the parsed structure.

cJSON *parse_json_file(const char *filepath) {
  cJSON *parsed = NULL;

  ASSERT(filepath, sprintf(msg, "Must have an input JSON file path"));

  char *content = read_json_file(filepath);     // allocates the content local buffer

  // fprintf(stderr, "FILE CONTENT: %s\n", content);

  parsed = cJSON_Parse(content); // cJSON workhorse, should return fully populated CJSON tree, calling routine needs to free

  if (!parsed) {
    // TODO make sure our abort messages has some pointer into the json file error that is useful
    const char *error_ptr = cJSON_GetErrorPtr();
    if (error_ptr)
      fprintf(stderr, "\nJSON Input error before: %s\n", strlen(error_ptr) ? error_ptr : "No Input");
    ASSERT(FALSE, sprintf(msg, "JSON Input parsing failed on file: %s", filepath));
  }

  if (cmds.cjson_echo_input) {
    char *jstring = NULL;
    jstring = cJSON_Print(parsed); // another workhorse to create a string from the CJSON tree
    ASSERT(jstring, sprintf(msg, "JSON Input file echo string not built for file: %s", filepath));
    fprintf(stderr, "\n--- Input JSON START ---\n%s\n", jstring);
    fprintf(stderr, "--- Input JSON END ---\n");
    if (jstring)
      free(jstring);
  }

  if (content)
    free(content); // done with file memory, all in parsed at this point
  return parsed;
}

/// Returns a pointer to the file extension.  It searches for the dot separator from the right side
/// of the string.

const char *get_filename_ext(const char *filename) {
  const char *dot = strrchr(filename, '.');
  if (!dot || dot == filename)
    return "";
  return dot + 1;
}
