#ifndef COMMON_INCLUDES
#define COMMON_INCLUDES

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "memdebug.h/memdebug.h"

#include "threadpool.h/threadpool.h"

#include "list.h/list.h"

#include "apaz-string.h/apaz-string.h"

#include "arena.h/arena.h"

/*******************/
/* LIBRARY INTEROP */
/*******************/

// Provide conversion between strings and lists of char.

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

// Does not destroy the string passed.
static inline List_String String_split(String str, char *delim) {
  size_t len = String_len(str);
  size_t delim_len = apaz_strlen(delim);

  List_String sl = List_String_new_cap(10);

  while (true) {
    char *match = apaz_strstr(str, delim);
    if (match) {
      ptrdiff_t diff = match - str;
      sl = List_String_addeq(sl, String_new_of(str, (size_t)diff));
    } else
      break;
  }

  return sl;
}

#ifdef __cplusplus
}
#endif
#endif // COMMON_INCLUDES