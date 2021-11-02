project(mhealib)

set(SRCS balance.c
         billing.c
         consmptn.c
         energyuse.c
         json.c
         loads.c
         measure.c
         mhea.c
         precalcs.c
         preflight.c
         results.c
         retrofit.c
         ua_dor.c
         ua_flr.c
         ua_rof.c
         ua_wal.c
         ua_win.c)

set(HDRS billing.h
         calcs.h
         constant.h
         definition.h
         dwelling.h
         energyuse.h
         enumeration.h
         intermediate.h
         json.h
         measure.h
         mhea.h
         output.h
         precalcs.h
         preflight.h
         results.h
         retrofit.h)

add_library(mhealib STATIC ${SRCS} ${HDRS})
