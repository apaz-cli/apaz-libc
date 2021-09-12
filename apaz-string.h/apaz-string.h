
#ifndef STRUTIL_INCLUDE
#define STRUTIL_INCLUDE

#include "variadic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

typedef char *String;

static inline String String_new(size_t len) {
  void *ptr = malloc(sizeof(size_t) + len + 1);
  *((size_t *)ptr) = len;
  String data = ((String)ptr) + sizeof(size_t);
  data[len] = '\0';
  return data;
}

static inline String String_new_of(char *cstr, size_t len) {
  String nstr = String_new(len);
  for (int i = 0; i < len; i++)
    nstr[i] = cstr[i];
  return nstr;
}

static inline String String_new_of_strlen(char *cstr) {
  size_t len = 0;
  while (cstr[len] != '\0')
    len++;

  String nstr = String_new(len);
  for (int i = 0; i < len; i++)
    nstr[i] = cstr[i];
  return nstr;
}

static inline String String_resize(String str, size_t new_size) {
  void *ptr = str - sizeof(size_t);
  ptr = realloc(ptr, sizeof(size_t) + new_size + 1);
  *((size_t *)ptr) = new_size;
  String data = ((String)ptr) + sizeof(size_t);
  data[new_size] = '\0';
  return data;
}

static inline void String_destroy(String str) { free(str - sizeof(size_t)); }

#define STRING_DESTROY_(str) free(str - sizeof(size_t));
#define STRING_DESTROY(...) __EVAL(__MAP(STRING_DESTROY_, __VA_ARGS__))

static inline size_t String_len(String str) {
  return *((size_t *)(str - sizeof(size_t)));
}

static inline String String_add(String str1, String str2) {
  size_t sl1 = String_len(str1), sl2 = String_len(str2);
  size_t sl3 = sl1 + sl2;
  String ns = String_new(sl3);
  size_t i = 0, j = 0;
  for (; i < sl1; i++)
    ns[i] = str1[i];
  for (; j < sl2; i++, j++)
    ns[i] = str2[j];
  ns[sl3] = '\0';
  return ns;
}

// Returns str1+str2, destroying both and returning a new string.
// Probably will re-use str1's memory.
static inline String String_add_destroy(String base, String to_append) {
  size_t blen = String_len(base), alen = String_len(to_append);
  size_t nlen = blen + alen;
  base = String_resize(base, nlen);
  for (size_t i = 0; i < nlen; i++)
    base[blen + i] = to_append[i];
  return base;
}

static inline String String_copy(String source, String dest) {
  size_t slen = String_len(source);
  size_t dlen = String_len(dest);
  size_t minlen = MIN(slen, dlen);
  for (size_t i = 0; i < minlen; i++)
    dest[i] = source[i];
  return source;
}

static inline String String_clone(String to_clone) {
  String ns = String_new(String_len(to_clone));
  String_copy(to_clone, ns);
  return ns;
}

static inline String String_substring_copy(String str, size_t start,
                                           size_t end) {
  String ns = String_new(end - start);
  String_copy(str, ns);
  return ns;
}

static inline void String_print(String str) {
  printf("%s", str);
  fflush(stdout);
}

static inline void String_println(String str) {
  printf("%s\n", str);
  fflush(stdout);
}

static inline size_t apaz_strlen(char *str) {
  register const char *s;
  for (s = str; *s; ++s)
    ;
  return (s - str);
}

static inline bool apaz_str_equals(char *str, char *seq) {
  // Compare until mismatch or on at least one null terminator.
  while (*str && (*str == *seq)) {
    str++;
    seq++;
  }
  // If one is null and not the other, return false.
  // Otherwise, return true.
  return *str == *seq;
}

static inline bool apaz_str_startsWith(char *str, char *prefix) {
  while (*prefix) {
    if (*prefix++ != *str++)
      return false;
  }
  return true;
}

static inline char *apaz_strstr(char *str, char *subseq) {
  while (*str) {
    if (!apaz_str_equals(str, subseq))
      return str;
    ++str;
  }
  return NULL;
}


static inline bool apaz_str_contains(char *str, char *subseq) {
  return apaz_strstr(str, subseq) == NULL ? false : true;
}

static inline bool String_equals(String str, char *seq) {
  return apaz_str_equals(str, seq);
}

static inline bool String_startsWith(String str, char *prefix) {
  return apaz_str_startsWith(str, prefix);
}

static inline bool String_endsWith(String str, char *suffix) {
  size_t len1 = String_len(str);
  size_t len2 = apaz_strlen(suffix);
  if (len1 < len2)
    return 0;
  str += (len1 - len2);
  return String_equals(str, suffix);
}

#endif // STRUTIL_INCLUDE