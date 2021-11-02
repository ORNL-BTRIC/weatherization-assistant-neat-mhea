/***************************************************************************
* MODULE:       definition.h            CREATED:      March 19, 1999
*
* AUTHOR:       Mark Fishbaugher
*
* MDESC:        defined constants used in both NEAT snd MHEA
****************************************************************************/
#ifndef _DEFINITION_H
#define _DEFINITION_H

#define NEAT_FLG 0
#define MHEA_FLG 1

// some string defines for various file input output treatments

#define NO_OUTPUT "no_output"
#define STD_OUTPUT "std_out"
#define STD_INPUT "std_in"

#define SYSTEM_DIR "./sys/"
#define ESCALATION_DIR SYSTEM_DIR "fuel_escalation/"
#define WEATHER_DIR SYSTEM_DIR "weather/"
#define SCHEMA_DIR SYSTEM_DIR "json_schema/"

#define JSON_SCHEMA_VALIDATION_MESSAGE_FILE SCHEMA_DIR "validation.txt"
#define JSON_INPUT_SCHEMA_FAIL_MESSAGE "JSON input schema validation failure"
#define JSON_OUTPUT_SCHEMA_FAIL_MESSAGE "JSON output schema validation failure"

#define COMMA ","
#define COMMA_CHAR ','

#define FALSE 0
#define TRUE 1

// Array subscripts when used for specific meanings

#define PRE_RETROFIT 0
#define POST_RETROFIT 1

#define TOTAL 0         // often the zeroth element of a base 1 array is used as a total for the non-zero-index array elements
#define AVG 0           // sometimes the zeroth element is used as the average of the other non-zero-index array elements

#define PRE_HEATING 0   // utility billing groups
#define PRE_COOLING 1
#define POST_HEATING 2
#define POST_COOLING 3

#define BILL_UNITS_MISSING -1
#define BILL_UNITS_THERMS 0
#define BILL_UNITS_KWH 1

// Yep, these are binary flags controlling debug diagnostic output
#define D_SILENT 0                      // there should be NO output from the engine other than errors that get echoed to the stdout/JSON in json form
#define D_NORMAL 1                      // normal debug output, lots of history and some formatted tabular output (production/default)
#define D_FUEL_ESCALATION_RATES 2       // for studying the escaltion rate calculations and comparison to EIA tables
#define D_JSON_INPUT_DETAIL 4           // detail echo inside JSON input 
#define D_MHEA_ENERGY_DETAIL 8          // detailed debug information from the mhea_energy_use(() call
#define D_MHEA_SIR_DETAIL 16            // detailed debug information from the MHEA mhea_measure_sir() call
#define D_MHEA_WIN_DETAIL 64            // detailed debug information from the MHEA window measures
#define D_NEAT_HTG_DETAIL 128           // detailed debug information from the NEAT heating replacement measure
#define D_INFILTRATION_DETAIL 256       // detailed infiltration calculations
#define D_NEAT_ENERGY_DETAIL_BASE 512   // detailed debug information from the neat_energy_use base case call
#define D_NEAT_ENERGY_DETAIL_ALL 1024   // detailed debug information from all calls to neat_energy_use 
#define D_MEASURE_EXCLUSION 2048        // show the measure exclusion matrix
#define D_WEATHER_DATA_ECHO 4096        // show summary of the weather data used

// here are our string lengths. Every defined constant that is used
// to dimension a character array has a _LEN (length) suffix

#define MAX_ASSERT_MESSAGE_LEN 1024

#define MAX_COMPONENTS_LEN 1024

#define PATH_LEN 254            // longest possible file pathname length 
#define SHORT_NAME_LEN 80       // size of a short file name, including extension

#define CITYNAME_LEN 30         // max chars for city name in WX file

#define CODE_LEN 20             // sizes of various string data in structures, changed from 4->20 2/20/01
#define MESSAGE_LEN 512         // program message length
#define STRING_LEN 255          // utility long-ish string length
#define UDMAT_LEN 80            // changed from 19 to 80 characters MJF 6/05

#define MEASURENAME_LEN 80      // added MJF 7/19
#define MATERIAL_LEN 80         // changed from 19 to 80 characters MJF 6/05
#define TYPE_LEN 50             // changed from 30 to 50 MJF 8/08
#define RETRO_UNIT_LEN 20       // changed from 6  to 20 MJF 7/07

#define N_CMS_NAME_LEN 80
#define N_CMS_UNITS_LEN 20

#define LRG_STR 60              // various large strings
#define DATE_LEN 12             // length of date in mm/dd/yyyy format
#define STATE_LEN 3             // for weather file
#define CITY_LEN 25             // for weather file

#define REGIONS 5               // 4 census regions plus one more for us average
#define FUEL_LEN 12             // length of fuel type names

#define FUEL_TYPES 8            // Our enumeration goes goes past this, but all there types map the first group of 8
#define NUM_FUEL_RATES 31       // The number of escalation factors for each fuel, was 26 but increased to match MulTEA 2019
#define MAXMLIFE 50             // Lifetimes can exceed the number of fuel rates with year 30 thru 50 using the year 30 factor

#define MAXMESSAGE 30           // Max number of special intereste messages in pgmmsg array
#define MAXMANUALJ 120          // Max number of manual J components

// IMPORTANT NOTE:  Make sure these maximums match the schema.json

#define MAX_HVAC 10      // #303  Heating and cooling systems
#define MAX_DUCT 6       // #303  Both supply and returns (3 possible pairs of ducts)

#define MAX_LTG 20 // lighting systems
#define MIN_LTG 0

#define MAX_ITC 60 // itemized costs
#define MIN_ITC 0

#define MAX_UBI 4 // utility billing periods (heating, cooling x before, after)
#define MIN_UBI 0

#define MAXECMS 200 // The maximum number of individual measures considered  both NEAT and MHEA

#endif // _DEFINITION_H
