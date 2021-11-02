/***************************************************************************
* MODULE:       wa_engine.c            CREATED:     October 22, 2018
*
* AUTHOR:       Mark Fishbaugher (mark@fishbaugher.com)
*
* MDESC:        the main entry point for the Weatherization Assistant
*               analysis engine that reads and writes JSON
****************************************************************************/

// clang-format note on header tidy:  clang-format now reorders the list
// in each grouping (separated by new lines) ordering alphabetically

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wa_engine.h"

// our main entry point
int main(int argc, char **argv) {
  cJSON *json_schema = NULL;     // our input json schema linked list
  cJSON *json_input = NULL;      // our input audit linked list JSON structure allocated by cJSON on parse
  cJSON *json_results = NULL;    // output results linked list JSON struct

  process_command_line(argc, argv);   // fills in our cmds. structure or fails and exits

  if (cmds.debug_level != D_SILENT) {
    ASSERT(sizeof(int) == 4, sprintf(msg, "Debug flags are binary assuming at least 4 byte int variable size"));
  }

  if (cmds.debug_level & D_NORMAL) {
    if (cmds.run_neat) fprintf(stderr, "\nNEAT Engine Run: ");
    if (cmds.run_mhea) fprintf(stderr, "\nMHEA Engine Run: ");
    if (!cmds.regression_test) fprintf(stderr, "\nVersion        : %s", WA_VERSION );
    fprintf(stderr, "\nInput From     : %s", cmds.input_file_path);  // always should have input
    fprintf(stderr, "\nOutput To      : %s", cmds.output_file_path); // and output
  }

  if (cmds.do_input_validation || cmds.do_output_validation) {
    ASSERT(system(NULL), sprintf(msg, "Shell is unavailable on this system.  It is needed to run ajv-cli"));
  }

  // uncomment the following line to test ASSERTion failure JSON output MJF 2/20
  // ASSERT(0, sprintf(msg, "This is a test assertion failure line"));

  // Common Weather Data structure
  ASSERT((cwd = (CWD *)calloc(1, sizeof(CWD))), sprintf(msg, "Out of memory on CWD"));

  // Run just one of the two possible engines

  if (cmds.run_neat) {

    if (cmds.do_input_validation) json_schema_validate_input(NEAT_INPUT_JSON_SCHEMA_FILE);
    json_input = parse_json_file(cmds.input_file_path);
    json_schema = parse_json_file(NEAT_INPUT_JSON_SCHEMA_FILE);

    if (cmds.debug_level & D_NORMAL) {
      // clang-format off
      if (strcmp(cmds.input_echo_file_path, NO_OUTPUT)    != 0) fprintf(stderr, "\nNEAT Echo To          : %s", cmds.input_echo_file_path);
      if (strcmp(cmds.neat_compare_file_path, NO_OUTPUT)  != 0) fprintf(stderr, "\nNEAT Compare Report To: %s", cmds.neat_compare_file_path);
      if (strcmp(cmds.neat_measure_file_path, NO_OUTPUT)  != 0) fprintf(stderr, "\nNEAT Measure Report To: %s", cmds.neat_measure_file_path);
      // clang-format on
    }

    ndi = NULL;          // NEAT dwelling information global pointer
    nir = NULL;          // NEAT intermediate results global pointer
    nor = NULL;          // NEAT output results global pointer

    ASSERT((ndi = (NDI *)calloc(1, sizeof(NDI))), sprintf(msg, "Out of memory on NDI"));
    ASSERT((nir = (NIR *)calloc(1, sizeof(NIR))), sprintf(msg, "Out of memory on NIR"));
    ASSERT((nor = (NOR *)calloc(1, sizeof(NOR))), sprintf(msg, "Out of memory on NOR"));

    neat_json_read(ndi, json_input, json_schema); // cJSON to NDI assignments using schema

    if (strcmp(cmds.input_echo_file_path, NO_OUTPUT) != 0) {
      neat_json_echo_write(ndi); // optional JSON echo for validation
    }

    run_neat(); // <<<<<<<======= NEAT engine WORKHORSE

    neat_json_result_write(ndi, nor); // Output the results structure as a JSON

    if (cmds.do_output_validation) json_schema_validate_output(NEAT_OUTPUT_JSON_SCHEMA_FILE);

    if (ndi) {free(ndi); ndi = NULL;}
    if (nir) {free(nir); nir = NULL;}
    if (nor) {free(nor); nor = NULL;}

  } else if (cmds.run_mhea) {

    if (cmds.do_input_validation) json_schema_validate_input(MHEA_INPUT_JSON_SCHEMA_FILE);
    json_input = parse_json_file(cmds.input_file_path);
    json_schema = parse_json_file(MHEA_INPUT_JSON_SCHEMA_FILE);

    if (cmds.debug_level & D_NORMAL) {
      // clang-format off
      if (strcmp(cmds.input_echo_file_path, NO_OUTPUT)    != 0) fprintf(stderr, "\nMHEA Echo To          : %s", cmds.input_echo_file_path);
      if (strcmp(cmds.mhea_compare_file_path, NO_OUTPUT)  != 0) fprintf(stderr, "\nMHEA Compare Report To: %s", cmds.mhea_compare_file_path);
      if (strcmp(cmds.mhea_measure_file_path, NO_OUTPUT)  != 0) fprintf(stderr, "\nMHEA Measure Report To: %s", cmds.mhea_measure_file_path);
      // clang-format on
    }

    ASSERT((mdi = (MDI *)calloc(1, sizeof(MDI))), sprintf(msg, "Out of memory on MDI"));
    ASSERT((mir = (MIR *)calloc(1, sizeof(MIR))), sprintf(msg, "Out of memory on MIR"));
    ASSERT((mor = (MOR *)calloc(1, sizeof(MOR))), sprintf(msg, "Out of memory on MOR"));

    mhea_json_read(mdi, json_input, json_schema);  // cJSON to MDI assignments

    if (strcmp(cmds.input_echo_file_path, NO_OUTPUT) != 0) {
      mhea_json_echo_write(mdi); // optional JSON echo for validation
    }

    run_mhea(); // <<<<<<<======= MHEA engine WORKHORSE

    mhea_json_result_write(mdi, mor); // Output the results structure as a JSON

    if (cmds.do_output_validation) json_schema_validate_output(MHEA_OUTPUT_JSON_SCHEMA_FILE);

    if (mdi) {free(mdi); mdi = NULL;}
    if (mir) {free(mir); mir = NULL;}
    if (mor) {free(mor); mor = NULL;}
  }

  if (cwd) {free(cwd); cwd = NULL;}

  // Time for cleanup just in case this function someday gets expanded or called separately
  // Normally all these will fall out of scope naturally at the return, but good practice to
  // make the cleanup explicit.

  if (json_schema)
    cJSON_Delete(json_schema);

  if (json_input)
    cJSON_Delete(json_input);

  if (json_results)
    cJSON_Delete(json_results);

  return EXIT_SUCCESS;
}
