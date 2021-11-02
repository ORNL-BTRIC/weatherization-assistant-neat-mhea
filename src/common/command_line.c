/***************************************************************************
* MODULE:       command_line.c            CREATED:     6/14/2019
*
* AUTHOR:       Mark Fishbaugher (mark@fishbaugher.com)
*
* MDESC:        Shared NEAT and MHEA command line processing
****************************************************************************/
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include "../getopt/getopt.h"
#else
#include <unistd.h>
#endif

#include "wa_engine.h"

void process_command_line(int argc, char **argv) {
  extern char *optarg;
  extern int optind;
  int opt;

  // Default command line arguments
  // clang-format off
  cmds.run_neat                   = FALSE;        // n
  cmds.run_mhea                   = FALSE;        // m
  cmds.do_input_validation        = FALSE;        // s
  cmds.do_output_validation       = FALSE;        // v
  cmds.debug_level                = D_SILENT;     // d
  cmds.input_file_path            = STD_INPUT;    // o
  cmds.output_file_path           = STD_OUTPUT;   // i
  cmds.cjson_echo_input           = FALSE;        // j
  cmds.run_identifier             = "";           // r
  cmds.format_json_output         = FALSE;        // j
  cmds.input_echo_file_path       = NO_OUTPUT;    // e
  cmds.neat_compare_file_path     = NO_OUTPUT;    // c
  cmds.neat_measure_file_path     = NO_OUTPUT;    // u
  cmds.mhea_compare_file_path     = NO_OUTPUT;    // x
  cmds.mhea_measure_file_path     = NO_OUTPUT;    // y
  cmds.regression_test            = FALSE;        // z

  static char usage[] = "usage: %s -nm[sjfz] [-d LEVEL] [-ioecuxy  FILE] [-r STRING]\n\n"
    WA_DESCRIPTION "\n"
    "Version: " WA_VERSION "\n"
    "Contact: " WA_CONTACT_EMAIL "\n\n"
    "  Cmd   Arg       Description (default setting)\n"
    "  --------------------------------------------------------------------------------------\n"
    "  -n              Run the NEAT site built engine\n"
    "  -m              Run the MHEA manufactured housing engine\n"
    "  -s              Do the input JSON validation against the schema\n"
    "  -v              Do the output JSON validation against the schema\n"
    "  -d   LEVEL      Debug level. NOTE: use >2filepath to redirect stderr to filepath (silent)\n"
    "  -i   FILE       JSON Input from FILE instead of standard input (stdin)\n"
    "  -o   FILE       JSON Output to FILE instead of standard output (stdout)\n"
    "  -j              Echo the cJSON input file to stderr in JSON format (false)\n"
    "  -r   STRING1    A run_identifier string used to tag both echoed input and results (blank)\n"
    "  -f              Format the JSON output with whitespace (false)\n"
    "  -e   FILE       Echo internal audit input structure as json prior to run (no output)\n"
    "  -c   FILE       Create a formated sizing and bill comparison output file, NEAT extra/legacy (no output)\n"
    "  -u   FILE       Create a formated recommended measure text report, NEAT extra/legacy (no output)\n"
    "  -x   FILE       Create a formated sizing and bill comparison output file, MHEA extra/legacy (no output)\n"
    "  -y   FILE       Create a formated recommended measure text report, MHEA extra/legacy (no output)\n"
    "  -z              Skip items in JSON output to aid in regression testing (false)\n"
    "  -h              Show this command line usage help message (no help message)\n";

  // list of command letters followed by : if the command takes an arg
  while ((opt = getopt(argc, argv, "nmsvd:i:o:jr:fe:c:u:x:y:zh")) != -1){

    switch (opt) {
    case 'n':
      cmds.run_neat = TRUE;
      break;
    case 'm':
      cmds.run_mhea = TRUE;
      break;
    case 's':
      cmds.do_input_validation = TRUE;
      break;
    case 'v':
      cmds.do_output_validation = TRUE;
      break;
    case 'd':
      cmds.debug_level = atoi(optarg);
      break;
    case 'i':
      cmds.input_file_path = optarg;
      break;
    case 'o':
      cmds.output_file_path = optarg;
      break;
    case 'j':
      cmds.cjson_echo_input = TRUE;
      break;
    case 'r':
      cmds.run_identifier = optarg;
      break;
    case 'f':
      cmds.format_json_output = TRUE;
      break;
    case 'e':
      cmds.input_echo_file_path = optarg;
      break;
    case 'c':
      cmds.neat_compare_file_path = optarg;
      break;
    case 'u':
      cmds.neat_measure_file_path = optarg;
      break;
    case 'x':
      cmds.mhea_compare_file_path = optarg;
      break;
    case 'y':
      cmds.mhea_measure_file_path = optarg;
      break;
    case 'z':
      cmds.regression_test = TRUE;
      break;

    case 'h':
    case '?':
      fprintf(stderr, usage, argv[0]);
      exit(EXIT_FAILURE);
      break;
    }
  }
  // clang-format on

  // Show usage notes if errors found in command input
  if (optind < argc ||
     (cmds.run_neat == FALSE && cmds.run_mhea == FALSE) ||
     (cmds.run_neat == TRUE && cmds.run_mhea == TRUE)) {
    fprintf(stderr, usage, argv[0]);
    fprintf(stderr, "\n\noptind:%d argc:%d", optind, argc);
    fprintf(stderr, "\nrun_neat:%d run_mhea:%d", cmds.run_neat, cmds.run_mhea);
    exit(EXIT_FAILURE);
  }
  return;
}

