#ifndef PROFILE_INCLUDE
#define PROFILE_INCLUDE

#include "../memdebug.h/memdebug.h"
#include <stdio.h>
#include <stdlib.h>

#if 1
static inline void
debug_printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    fflush(stdout);
}

#else
static inline void
debug_printf(const char* fmt, ...) { (void)fmt; }
#endif


#define APAZ_PROFILE 1
/* Backtraces */
#ifdef __has_include
#if __has_include(<execinfo.h>)
#include <execinfo.h>
/* Obtain a backtrace and print it to stdout. */
static inline void print_trace() {
  const int max_frames = 1000;
  void *array[max_frames];
  char **strings;
  int size, i;
  size = backtrace(array, max_frames);
  strings = backtrace_symbols(array, size);
  if (strings != NULL) {
    printf("Obtained %d stack frames.\n", size);
    for (i = 0; i < size; i++)
      printf("%s\n", strings[i]);
  }
  original_free(strings);
}
#else
static inline void print_trace() {
  fprintf(stderr, "The include file <execinfo.h> could not be found on your "
                  "system. Backtraces not supported.\n");
  exit(1);
}
#endif
#else
static inline void print_trace() {
  fprintf(stderr, "Could not look for <execinfo.h>, because __has_include() is "
                  "not defined. Backtraces not supported.\n");
  exit(1);
}
#endif

/* Profiling */

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
#include <time.h>

#define STOPWATCH_HOURS (CLOCKS_PER_SEC * 60 * 60)
#define STOPWATCH_MINUTES (CLOCKS_PER_SEC * 60)
#define STOPWATCH_SECONDS (CLOCKS_PER_SEC)
#define STOPWATCH_MILLISECONDS (CLOCKS_PER_SEC / 1000.0)
#define STOPWATCH_MICROSECONDS (CLOCKS_PER_SEC / 1000000.0)

#if APAZ_PROFILE

static clock_t __stopwatch_timer;
static size_t __stopwatch_laps;
static double __stopwatch_resolution;
static char *__stopwatch_tstr;

static clock_t __stopwatch_start;
static clock_t __stopwatch_stop;

#define STOPWATCH_INIT(resolution)                                             \
  do {                                                                         \
    __stopwatch_timer = 0;                                                     \
    __stopwatch_laps = 0;                                                      \
    __stopwatch_resolution = resolution;                                       \
    if (resolution == STOPWATCH_HOURS)                                         \
      __stopwatch_tstr = "hours";                                              \
    if (resolution == STOPWATCH_MINUTES)                                       \
      __stopwatch_tstr = "min";                                                \
    else if (resolution == STOPWATCH_SECONDS)                                  \
      __stopwatch_tstr = "s";                                                  \
    else if (resolution == STOPWATCH_MILLISECONDS)                             \
      __stopwatch_tstr = "ms";                                                 \
    else if (resolution == STOPWATCH_MICROSECONDS)                             \
      __stopwatch_tstr = "us";                                                 \
    else {                                                                     \
      fprintf(stdout,                                                          \
              "Please provide a proper argument to STOPWATCH_INIT().\n");      \
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
    __stopwatch_laps += 1;                                                     \
  } while (0);
#define STOPWATCH_READ()                                                       \
  do {                                                                         \
    double __time_converted =                                                  \
        (double)__stopwatch_timer / __stopwatch_resolution;                    \
    double __avg_time = __time_converted / __stopwatch_laps;                   \
    /* TODO figure out how to handle format and resolution. Probably with      \
     * preprocessor magic. Currently this only works on my machine. */         \
    if (__stopwatch_resolution != STOPWATCH_MICROSECONDS) {                    \
      printf(ANSI_COLOR_YELLOW                                                 \
             "Stopwatch laps: " ANSI_COLOR_RESET ANSI_COLOR_RED                \
             "%zu" ANSI_COLOR_RESET "\n" ANSI_COLOR_YELLOW                     \
             "Total Time: " ANSI_COLOR_RESET ANSI_COLOR_RED                    \
             "%.2f %s" ANSI_COLOR_RESET "\n" ANSI_COLOR_YELLOW                 \
             "Average Time: " ANSI_COLOR_RESET ANSI_COLOR_RED                  \
             "%.2f %s" ANSI_COLOR_RESET "\n",                                  \
             __stopwatch_laps, __time_converted, __stopwatch_tstr, __avg_time, \
             __stopwatch_tstr);                                                \
    } else {                                                                   \
      printf(ANSI_COLOR_YELLOW                                                 \
             "Stopwatch laps: " ANSI_COLOR_RESET ANSI_COLOR_RED                \
             "%zu" ANSI_COLOR_RESET "\n" ANSI_COLOR_YELLOW                     \
             "Total Time: " ANSI_COLOR_RESET ANSI_COLOR_RED                    \
             "%.0f %s" ANSI_COLOR_RESET "\n" ANSI_COLOR_YELLOW                 \
             "Average Time: " ANSI_COLOR_RESET ANSI_COLOR_RED                  \
             "%.2f %s" ANSI_COLOR_RESET "\n",                                  \
             __stopwatch_laps, __time_converted, __stopwatch_tstr, __avg_time, \
             __stopwatch_tstr);                                                \
    }                                                                          \
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
    fprintf(stderr, "Profiling is disabled. Please recompile with "            \
                    "APAZ_PROFILE = 1 and MEMDEBUG = 0.\n");                   \
    exit(1);                                                                   \
  }
#endif // APAZ_PROFILE

#endif // PROFILE_INCLUDE