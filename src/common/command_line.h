/***************************************************************************
* MODULE:       cmd.h            CREATED:     6/14/2019
*
* AUTHOR:       Mark Fishbaugher (mark@fishbaugher.com)
*
* MDESC:        Command line parsing shared by NEAT and MHEA
****************************************************************************/
#ifndef _COMMAND_LINE_H
#define _COMMAND_LINE_H

typedef struct {
  int run_neat;
  int run_mhea;
  int do_input_validation;
  int do_output_validation;
  int debug_level;
  char *input_file_path;
  char *output_file_path;
  int cjson_echo_input;
  char *run_identifier;
  int format_json_output;
  char *input_echo_file_path;
  char *neat_compare_file_path;
  char *neat_measure_file_path;
  char *mhea_compare_file_path;
  char *mhea_measure_file_path;
  int regression_test;

} WA_COMMAND_LINE_ARGS;

WA_COMMAND_LINE_ARGS cmds;

void process_command_line(int argc, char **argv);

#else

extern WA_COMMAND_LINE_ARGS cmds;

#endif // _COMMAND_LINE_H
