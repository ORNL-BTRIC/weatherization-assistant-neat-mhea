project(commonlib)

set(SRCS command_line.c
         fuels.c
         hvac_2.c
         infiltration.c
         json.c
         utility.c
         weather.c
         ../cjson/cjson.c)

set(HDRS command_line.h
         constant.h
         definition.h
         dwelling.h
         enumeration.h
         fuels.h
         hvac_2.h
         infiltration.h
         json.h
         macro.h
         output.h
         utility.h
         version.h
         wa_engine.h
         weather.h
         ../cjson/cjson.h)

if(WIN32)
  list(APPEND SRCS ../getopt/getopt.c)
  list(APPEND HDRS ../getopt/getopt.h)
endif(WIN32)

add_library(commonlib STATIC ${SRCS} ${HDRS})
