#ifndef COMMON_INCLUDES
#define COMMON_INCLUDES

#include <sys/types.h>
#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef APAZ_HANDLE_UNLIKELY_ERRORS
#define APAZ_HANDLE_UNLIKELY_ERRORS 1
#endif

#include "memdebug.h/memdebug.h"

#include "threadpool.h/threadpool.h"

#include "list.h/list.h"

#include "apaz-string.h/apaz-string.h"

#include "arena.h/arena.h"

#include "apaz-utf8.h/apaz-utf8.h"

/*******************/
/* LIBRARY INTEROP */
/*******************/

#define ARR_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

/******************************************************************************/
#define TYPE_DECLARE(type)                                                     \
  struct type;                                                                 \
  typedef struct type type;                                                    \
  typedef type *type##Ptr;                                                     \
  LIST_DECLARE(type)                                                           \
  /****************************************************************************/

LIST_DEFINE(char);
LIST_DEFINE(List_char);
LIST_DEFINE(String);
LIST_DEFINE(List_String);
LIST_DEFINE_MONAD(char, char);
LIST_DEFINE_MONAD(String, String);
LIST_DEFINE_MONAD(char, String);
LIST_DEFINE_MONAD(String, char);

// Destroys the string passed.
static inline List_char String_to_List_char(String str) {
  size_t n = String_len(str);
  List_char strl = List_char_new_of(str, n, n);
  String_destroy(str);
  return strl;
}

// Destroys the list passed
static inline String List_char_to_String(List_char list) {
  String s = String_new_of(list, List_char_len(list));
  List_char_destroy(list);
  return s;
}

static inline void List_String_print(List_String strlist) {
  size_t len = List_String_len(strlist);
  putchar('[');
  if (len) {
    printf("%s", strlist[0]);
    for (size_t i = 1; i < len; i++) {
      printf(", %s", strlist[i]);
    }
  }
  puts("]\n");
}

// Doesn't destroy the string passed.
// The list returned must be destroyed by List_destroy.
static inline List_String String_split(String str, char *delim) {
  size_t delim_len = apaz_strlen(delim);

  // First make the list disregarding length information.
  // Remember that the first one actually has it already.
  List_String ret = List_String_new_cap(50);
  ret = List_String_addeq(ret, str);

  // Pull out matches
  char *match = str;
  while (true) {
    // Find the next delimiter until we're done
    match = apaz_strstr(match, delim);
    if (!match)
      break;

    // Skip after the delimiter
    match += delim_len;

    // Add the result. Note this may result in a string of
    // length zero to be added. This is intentional.
    ret = List_String_addeq(ret, match);
  };

  // Create strings of the right length with the contents.
  // Convert all but the last one. Then do the last one.
  // There's always a last one.
  size_t lastIdx = List_String_len(ret) - 1;
  for (size_t i = 0; i < lastIdx; i++)
    ret[i] = String_new_of(ret[i], (ret[i + 1] - delim_len) - ret[i]);
  ret[lastIdx] = String_new_of_strlen(ret[lastIdx]);

  return ret;
}

#ifdef __cplusplus
}
#endif
#endif // COMMON_INCLUDES