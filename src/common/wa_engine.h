/***************************************************************************
 * MODULE:  wa_engine.h            CREATED:    December 11, 2018
 *
 * AUTHOR:  Mark Fishbaugher (mark@fishbaugher.com)
 *
 * MDESC:   Some global macros and our wa command line structure
 ****************************************************************************/
#ifndef _WA_ENGINE_H
#define _WA_ENGINE_H

#include "version.h"
#define WA_VERSION BUILD_VERSION " " BUILD_TIMESTAMP
#define WA_CONTACT_EMAIL "<ternesmp@ornl.gov>"
#define WA_DESCRIPTION "USDOE/ORNL Weatherization Assistant analysis engine"

#include "command_line.h"
#include "../cjson/cjson.h"    // JSON handling c functions

#include "constant.h"          // common constants
#include "definition.h"        // common definitions
#include "enumeration.h"       // common enumerations
#include "dwelling.h"          // common dwelling input structures
#include "hvac_2.h"            // common hvac functions and structs
#include "output.h"            // common output structures
#include "json.h"              // common JSON handling
#include "macro.h"             // common macros
#include "fuels.h"             // common fuel price functions
#include "weather.h"           // common weather functions
#include "utility.h"           // common utility functions

#include "../neat/constant.h"            // NEAT defined constants
#include "../neat/definition.h"          // NEAT defines
#include "../neat/enumeration.h"         // NEAT enumerations
#include "../neat/dwelling.h"            // NEAT NDI structures
#include "../neat/intermediate.h"        // NEAT NIR structures
#include "../neat/output.h"              // NEAT NOR structures

#include "../neat/subs.h"                // NEAT JSON handling
#include "../neat/run_preparation.h"     // NEAT input conversion handling
#include "../neat/preflight.h"           // NEAT dwelling input checks
#include "../neat/billing.h"             // NEAT billing adjustments
#include "../neat/size.h"                // NEAT equipment sizing
#include "../neat/infiltration.h"        // NEAT duct and infiltration
#include "../neat/ecms.h"                // NEAT energy conservation measure functions
#include "../neat/report.h"              // NEAT reporting functions

#include "../neat/json.h"                // NEAT JSON handling

#include "../neat/neat.h"                // NEAT top level

#include "../mhea/constant.h"            // MHEA defined constants
#include "../mhea/definition.h"          // MHEA defines
#include "../mhea/enumeration.h"         // MHEA enumerations
#include "../mhea/dwelling.h"            // MHEA MDI structures
#include "../mhea/intermediate.h"        // MHEA MIR structures
#include "../mhea/output.h"              // MHEA MOR structures

#include "../mhea/calcs.h"               // MHEA Calculations
#include "../mhea/measure.h"             // MHEA ECM prototypes
#include "../mhea/precalcs.h"            // MHEA pre calc 
#include "../mhea/preflight.h"           // MHEA data checks
#include "../mhea/energyuse.h"           // MHEA energy modeling
#include "../mhea/retrofit.h"            // MHEA energy conservation retrofit functions
#include "../mhea/results.h"             // MHEA outpt result writting
#include "../mhea/billing.h"             // MHEA billing adjustments

#include "../mhea/json.h"                // MHEA JSON handling

#include "../mhea/mhea.h"                // MHEA top level

#include "infiltration.h"      // common infiltration and duct leakage calculations

// Our three common top level NEAT global pointers
NDI *ndi;
NIR *nir;
NOR *nor;

// Our three common top level MHEA global pointers
MDI *mdi;
MIR *mir;
MOR *mor;

// Common Weather Data .wx and solar data
CWD *cwd;

#else

// Our three common top level NEAT global pointers
extern NDI *ndi;
extern NIR *nir;
extern NOR *nor;

// Our three common top level MHEA global pointers
extern MDI *mdi;
extern MIR *mir;
extern MOR *mor;

// Common Weather Data .wx and solar data
extern CWD *cwd;

#endif /* _WA_ENGINE_H */
