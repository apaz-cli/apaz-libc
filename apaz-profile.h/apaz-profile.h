#ifndef PROFILE_INCLUDE
#define PROFILE_INCLUDE

#include "../memdebug.h/memdebug.h"

#define APAZ_PROFILE 1

#ifndef APAZ_PROFILE_MEMDEBUG
#define APAZ_PROFILE_MEMDEBUG 0
#endif

#if APAZ_PROFILE_MEMDEBUG
#ifndef APAZ_PROFILE
#define APAZ_PROFILE 1
#endif
#else
#ifndef APAZ_PROFILE
#define APAZ_PROFILE 0
#endif
#endif

#if APAZ_PROFILE_MEMDEBUG && !APAZ_PROFILE
_Static_assert(0, "APAZ_PROFILE_MEMDEBUG = 1 is incompatible with APAZ_PROFILE "
                  "= 0. Either use the debugger or don't.");
#endif

// Override the decision that it's not to be used with memdebug.

#include <assert.h>
#include <stdio.h>
#include <time.h>
#define STOPWATCH_MINUTES (CLOCKS_PER_SEC * 60)
#define STOPWATCH_SECONDS (CLOCKS_PER_SEC)
#define STOPWATCH_MILLISECONDS (CLOCKS_PER_SEC / 1000.0)
#define STOPWATCH_MICROSECONDS (CLOCKS_PER_SEC / 1000000.0)

#if APAZ_PROFILE

static clock_t __stopwatch_timer;
static size_t __stopwatch_runs;
static double __stopwatch_resolution;
static char *__stopwatch_tstr;

static clock_t __stopwatch_start;
static clock_t __stopwatch_stop;

#define STOPWATCH_INIT(resolution)                                             \
  do {                                                                         \
    __stopwatch_timer = 0;                                                     \
    __stopwatch_runs = 0;                                                      \
    __stopwatch_resolution = resolution;                                       \
    if (resolution == STOPWATCH_MINUTES)                                       \
      __stopwatch_tstr = "minutes";                                            \
    else if (resolution == STOPWATCH_SECONDS)                                  \
      __stopwatch_tstr = "seconds";                                            \
    else if (resolution == STOPWATCH_MILLISECONDS)                             \
      __stopwatch_tstr = "milliseonds";                                        \
    else if (resolution == STOPWATCH_MICROSECONDS)                             \
      __stopwatch_tstr = "microseconds";                                       \
    else {                                                                     \
      fprintf(stdout,                                                          \
              "Please provide a proper argument to STOPWATCH_INIT().");        \
      exit(1);                                                                 \
    }                                                                          \
  } while (0);
#define STOPWATCH_START_LAP()                                                  \
  do {                                                                         \
    __stopwatch_start = clock();                                               \
  } while (0);
#define STOPWATCH_END_LAP()                                                    \
  do {                                                                         \
    __stopwatch_stop = clock();                                                \
    __stopwatch_timer += (__stopwatch_stop - __stopwatch_start);               \
    __stopwatch_runs += 1                                                      \
  } while (0);
#define STOPWATCH_READ()                                                       \
  do {                                                                         \
    double __time_spent =                                                      \
        (double)(__stopwatch_stop - __stopwatch_start) / CLOCKS_PER_SEC;       \
    printf("Code took %d reads %f");                                           \
  } while (0);

#define MICROBENCH_MAIN(function, times, resolution)                           \
  int main() {                                                                 \
    STOPWATCH_INIT(resolution);                                                \
    for (size_t i = 0; i < (times); i++) {                                     \
      STOPWATCH_START_LAP();                                                   \
      function();                                                              \
      STOPWATCH_END_LAP();                                                     \
    }                                                                          \
    STOPWATCH_READ();                                                          \
  }

#else // APAZ_PROFILE

#define STOPWATCH_INIT(resolution) ;
#define STOPWATCH_START_LAP() ;
#define STOPWATCH_END_LAP() ;
#define STOPWATCH_READ() ;
#define MICROBENCH_MAIN(function, times, resolution)                           \
  int main() {                                                                 \
    fprintf(stderr,                                                            \
            "Profiling is disabled. Please recompile with APAZ_PROFILE.\n");   \
    exit(1);                                                                   \
  }
#endif // APAZ_PROFILE

#endif // PROFILE_INCLUDE