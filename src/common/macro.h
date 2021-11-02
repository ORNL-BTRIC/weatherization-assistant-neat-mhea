/***************************************************************************
 * MODULE:       macro.h            CREATED:     October 22, 2018
 *
 * AUTHOR:       Mark Fishbaugher (mark@fishbaugher.com)
 *
 * MDESC:        a place for common macros for both the neat and mhea engines
 ****************************************************************************/
#ifndef _MACRO_H
#define _MACRO_H

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// #include "def.h"
// #include "enum.h"

// #include "cjson.h" // get the case of the file name right to avoid compiler warning
// #include "json_helper.h"

// clang-format off   so our braces line up in the editor

// I tried variadic macro, but clang does not support zero args for __VA_ARGS__
// https://gcc.gnu.org/onlinedocs/cpp/Variadic-Macros.html,  NO GO
// so we resort to a single version that always takes an sprintf(msg, ...) in the call
#define ASSERT(condition, sprintf_statement)                                                                                     \
  ({                                                                                                                             \
    if (!(condition)) {                                                                                                          \
      char fail_message[2 * MAX_ASSERT_MESSAGE_LEN];                                                                                 \
      char fail_location[MAX_ASSERT_MESSAGE_LEN];                                                                                \
      char msg[MAX_ASSERT_MESSAGE_LEN];                                                                                          \
      sprintf_statement;                                                                                                         \
      sprintf(fail_message, "(%s):%s", #condition, msg);                                                                         \
      sprintf(fail_location, "%s:%d", __FILE__, __LINE__);                                                                       \
      assert_fail_json_output(fail_message, fail_location);                                                                      \
      fprintf(stderr, "\n%s\n", fail_message);                                                                                   \
      assert((condition));                                                                                                       \
    }                                                                                                                            \
  })

// NOTE:  These strxx() replacement macros only work for statically declared
// strings since we are using the sizeof() function which returns the size of the
// pointer if you pass a dynamically allocated string  MJF 2019  #52
// we define our own buffer overflow protected string operations and try
// to use only these

// clang-format off

#define STRCPY(dest, source)                                                                                                     \
  ({                                                                                                                             \
    ASSERT(sizeof(dest) != sizeof(char *), sprintf(msg, "STRCPY using pointer: %ld != %ld",sizeof(dest), sizeof(char *)));       \
    ASSERT(sizeof(dest) > strlen(source), sprintf(msg, "STRCPY overflow %ld < %ld (%s) %s:%d", sizeof(dest), strlen(source), source, __FILE__, __LINE__)); \
    strcpy(dest, source);                                                                                                        \
  })

#define STRCAT(dest, source)                                                                                                     \
  ({                                                                                                                             \
    ASSERT(sizeof(dest) != sizeof(char *), sprintf(msg, "STRCAT using pointer"));                                                \
    ASSERT(sizeof(dest) > strlen(dest) + strlen(source), sprintf(msg, "STRCAT overflow %ld < %ld (%s) %s:%d", sizeof(dest), strlen(dest) + strlen(source), source, __FILE__, __LINE__)); \
    strcat(dest, source);                                                                                                        \
  })

// note that we make sure the dest is null terminate having made sure we have space for the null
#define STRNCAT(dest, source, size)                                                                                              \
  ({                                                                                                                             \
    ASSERT(sizeof(dest) != sizeof(char *), sprintf(msg, "STRNCAT using pointer"));                                                \
    ASSERT(sizeof(dest) > strlen(dest) + size, sprintf(msg, "STRNCAT overflow %ld < %ld (%s) %s:%d", sizeof(dest), strlen(dest) + (size_t)size, source, __FILE__, __LINE__));  \
    strncat(dest, source, size);                                                                                                 \
    dest[strlen(dest)] = '\0';                                                                                                   \
  })

// we have no assert in this version because we simply truncate the src but this
// protects the dest string with null terminator in cases where strlen(source) = size_of(dest)
#define STRNCPY(dest, source, size)                                                                                              \
  ({                                                                                                                             \
    ASSERT(sizeof(dest) != sizeof(char *), sprintf(msg, "STRNCPY using pointer"));                                                \
    ASSERT(sizeof(dest) > size, sprintf(msg, "STRNCPY overflow %ld:%ld < %ld (%s) %s:%d", sizeof(dest)-1, (size_t)size, strlen(source), source, __FILE__, __LINE__));       \
    strncpy(dest, source, size);                                                                                                 \
    dest[size] = '\0';                                                                                                       \
  })

// clang-format on

// Fixed number of decimals in output, works for any numeric value
#define WA_DBL_FMT(value, decimals) round((double)value * pow(10, decimals)) / pow(10, decimals)

// some other common macro definitions

#define EXPC(x) (exp((double)x))
#define POWC(x, y) (pow((double)x, (double)y))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

/** rounding rather than truncation macro **/

#define ROUND(A) (((A) - (int)((A))) >= 0.5F ? (int)((A) + 1) : (int)((A)))
  
// clang-format on

#endif // _MACRO_H
