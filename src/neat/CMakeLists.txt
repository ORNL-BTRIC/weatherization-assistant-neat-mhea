project(neatlib)

set(SRCS billing.c
         infiltration.c
         neat.c
         report.c
         size.c
         ecms.c
         json.c
         preflight.c
         run_preparation.c
         subs.c)

set(HDRS billing.h
         ecms.h
         json.h
         preflight.h
         subs.h
         constant.h
         enumeration.h
         neat.h
         report.h
         definition.h
         infiltration.h
         output.h
         run_preparation.h
         dwelling.h
         intermediate.h
         postwx.h
         size.h)

add_library(neatlib STATIC ${SRCS} ${HDRS})
