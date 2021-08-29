#ifndef COMMON_INCLUDES
#define COMMON_INCLUDES

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "list.h/list.h"
#include "string.h/string.h"
#include "memdebug.h/memdebug.h"
#include "threadpool.h/threadpool.h"

/*******************/
/* LIBRARY INTEROP */
/*******************/

// Provide conversion between strings and lists

LIST_DEFINE(char);
LIST_DEFINE(List_char);
LIST_DEFINE(String);
LIST_DEFINE(List_String);
LIST_DEFINE_MONAD(char, char);
LIST_DEFINE_MONAD(String, String);
LIST_DEFINE_MONAD(char, String);
LIST_DEFINE_MONAD(String, char);

// Destroys the string passed.
static inline List_char String_to_charList(String str) {
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

#endif // COMMON_INCLUDES