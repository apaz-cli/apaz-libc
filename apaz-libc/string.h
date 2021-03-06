#ifndef STRUTIL_INCLUDE
#define STRUTIL_INCLUDE

#include "arena.h"
#include "list.h"
#include "memdebug.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

#ifndef APAZ_HANDLE_UNLIKELY_ERRORS
#define APAZ_HANDLE_UNLIKELY_ERRORS 1
#endif

#if APAZ_HANDLE_UNLIKELY_ERRORS
#if MEMDEBUG
#define HANDLE_OOM(ptr)                                                        \
  if (!ptr) {                                                                  \
    printf("Out of memory on line %zu of %s.\n", __LINE__, __FILE__);          \
    exit(5);                                                                   \
    return NULL;                                                               \
  }
#else
#define HANDLE_OOM(ptr)                                                        \
  if (!ptr) {                                                                  \
    return NULL;                                                               \
  }
#endif
#else // APAZ_HANDLE_UNLIKELY_ERRORS == 0
#define HANDLE_OOM(ptr) ;
#endif

/****************************/
/* Charptr String Functions */
/****************************/

static inline size_t apaz_strlen(char *str) {
  const char *s;
  for (s = str; *s; ++s)
    ;
  return (s - str);
}

static inline bool apaz_str_equals(char *str, char *seq) {
  while (*str && (*str == *seq)) {
    str++;
    seq++;
  }
  return *str == *seq;
}

static inline int apaz_strcmp(char *s1, char *s2) {
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
  return *(unsigned char *)s1 - *(unsigned char *)s2;
}

static inline bool apaz_str_startsWith(char *str, char *prefix) {
  if (!str | !prefix)
    return false;
  if (!*prefix | !*str)
    return false;
  while (*prefix)
    if (*prefix++ != *str++)
      return false;
  return true;
}

/*
 * Returns the index of the first match, or NULL if none found.
 */
static inline char *apaz_strstr(char *str, const char *subseq) {
  while (*str) {
    if (apaz_str_startsWith(str, (char *)subseq))
      return str;
    else
      str++;
  }
  return NULL;
}

static inline bool apaz_str_contains(char *str, char *subseq) {
  return apaz_strstr(str, subseq) == NULL ? false : true;
}

/*********************/
/* Type Declarations */
/*********************/

typedef char *String;

/***************************************/
/* Fatptr String Function Declarations */
/***************************************/

// Wrap functions that modify memory in macros so that memdebug.h doesn't get
// confused. Furthermore, pass the line/function/file tracking through if one of
// these functions calls another. We want the allocations to appear as though
// they come from user code, not internal String code.

// Don't worry about efficiency here. Starting at -O2, both GCC and Clang will
// optimize out all the additional parameters if memdebug functions don't use
// them (which is to say if not MEMDEBUG). This is a benefit of everything being
// in the same translation unit. Otherwise, they would be unable to optimize for
// us without breaking ABI.

#define String_new(len) _String_new(len, __LINE__, __func__, __FILE__)
#define String_new_of(cstr, len)                                               \
  _String_new_of(cstr, len, __LINE__, __func__, __FILE__)
#define String_new_of_strlen(cstr)                                             \
  _String_new_of_strlen(cstr, __LINE__, __func__, __FILE__)
#define String_new_fromFile(filePath)                                          \
  _String_new_fromFile(filePath, __LINE__, __func__, __FILE__)
#define String_resize(str, new_size)                                           \
  _String_resize(str, new_size, __LINE__, __func__, __FILE__)
#define String_destroy(str) _String_destroy(str, __LINE__, __func__, __FILE__)
static inline size_t String_len(String str);
#define String_add(str1, str2)                                                 \
  _String_add(str1, str2, __LINE__, __func__, __FILE__)
#define String_add_destroy(base, to_append)                                    \
  _String_add_destroy(base, to_append, __LINE__, __func__, __FILE__);
static inline void String_copy(String source, String dest);
#define String_clone(to_clone)                                                 \
  _String_clone(to_clone, __LINE__, __func__, __FILE__)
#define String_substring(str, start, end)                                      \
  _String_substring(str, start, end, __LINE__, __func__, __FILE__)
static inline void String_print(String str);
static inline void String_println(String str);
static inline bool String_equals(String str, char *seq);
static inline bool String_startsWith(String str, char *prefix);
static inline bool String_endsWith(String str, char *suffix);
static inline uint64_t String_hash(String str);
static inline int String_compareTo(String str1, String str2);
static inline bool String_contains(String str, char *subseq);
static inline String String_toUpper(String str);
static inline String String_toLower(String str);

// #define                String_regexCompile(expr)
// static inline bool     String_regexMatches(String str, CompiledRegEx regex);

#include "variadic.h"
#define STRING_DESTROY_(str) _String_destroy(str, __LINE__, __func__, __FILE__);
#define STRING_DESTROY(...) __EVAL(__MAP(STRING_DESTROY_, __VA_ARGS__))

static inline String _String_new(size_t len, size_t line, const char *func,
                                 const char *file) {
  void *ptr = memdebug_malloc(sizeof(size_t) + len + 1, line, func, file);
  HANDLE_OOM(ptr);
  *((size_t *)ptr) = len;
  String data = ((String)ptr) + sizeof(size_t);
  data[len] = '\0';
  return data;
}

static inline String _String_new_of(char *cstr, size_t len, size_t line,
                                    const char *func, const char *file) {
  String nstr = _String_new(len, line, func, file);
  HANDLE_OOM(nstr);
  for (int i = 0; i < len; i++)
    nstr[i] = cstr[i];
  return nstr;
}

static inline String _String_new_of_strlen(char *cstr, size_t line,
                                           const char *func, const char *file) {
  size_t len = 0;
  while (cstr[len] != '\0')
    len++;

  String nstr = _String_new(len, line, func, file);
  HANDLE_OOM(nstr);
  for (int i = 0; i < len; i++)
    nstr[i] = cstr[i];
  return nstr;
}

static inline String _String_resize(String str, size_t new_size, size_t line,
                                    const char *func, const char *file) {
  void *ptr = str - sizeof(size_t);
  ptr = memdebug_realloc(ptr, sizeof(size_t) + new_size + 1, line, func, file);
  HANDLE_OOM(ptr);
  *((size_t *)ptr) = new_size;
  String data = ((String)ptr) + sizeof(size_t);
  data[new_size] = '\0';
  return data;
}

static inline void _String_setlen_internal(String str, size_t new_len) {
  size_t *st = ((size_t *)str) - 1;
  *st = new_len;
  str[new_len] = '\0';
}

static inline void _String_destroy(String str, size_t line, const char *func,
                                   const char *file) {
  memdebug_free(str - sizeof(size_t), line, func, file);
}

static inline size_t String_len(String str) {
  return *((size_t *)(str - sizeof(size_t)));
}

// Returns str1str2, destroying neither and returning a new string.
static inline String _String_add(String str1, String str2, size_t line,
                                 const char *func, const char *file) {
  size_t sl1 = String_len(str1), sl2 = String_len(str2);
  size_t sl3 = sl1 + sl2;
  String ns = _String_new(sl3, line, func, file);
  HANDLE_OOM(ns);
  size_t i = 0, j = 0;
  for (; i < sl1; i++)
    ns[i] = str1[i];
  for (; j < sl2; i++, j++)
    ns[i] = str2[j];
  return ns;
}

// Returns str1str2, destroying both and returning a new string. (Re-uses memory
// where possible.)
static inline String _String_add_destroy(String base, String to_append,
                                         size_t line, const char *func,
                                         const char *file) {
  size_t blen = String_len(base), alen = String_len(to_append);
  size_t nlen = blen + alen;
  base = _String_resize(base, nlen, line, func, file);
  HANDLE_OOM(base);
  for (size_t i = 0; i < nlen; i++)
    base[blen + i] = to_append[i];
  _String_destroy(to_append, line, func, file);
  return base;
}

static inline void String_copy(String source, String dest) {
  size_t slen = String_len(source);
  size_t dlen = String_len(dest);
  size_t minlen = MIN(slen, dlen);
  for (size_t i = 0; i < minlen; i++)
    dest[i] = source[i];
}

static inline String _String_clone(String to_clone, size_t line,
                                   const char *func, const char *file) {
  String ns = _String_new(String_len(to_clone), line, func, file);
  HANDLE_OOM(ns);
  String_copy(to_clone, ns);
  return ns;
}

static inline String _String_substring(String str, size_t start, size_t end,
                                       size_t line, const char *func,
                                       const char *file) {
  String ns = _String_new(end - start, line, func, file);
  HANDLE_OOM(ns);
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

static inline uint64_t String_hash(String str) {
  // djb2
  char c;
  uint64_t h = 5381;
  while ((c = *str++))
    h = ((h << 5) + h) + c;
  return h;
}

static inline int String_compareTo(String str1, String str2) {
  return apaz_strcmp(str1, str2);
}

static inline bool String_contains(String str, char *subseq) {
  return apaz_str_contains(str, subseq);
}

static inline String String_toUpper(String str) {
  String original = str;
  for (; *str; ++str)
    *str = toupper(*str);
  return original;
}

static inline String String_toLower(String str) {
  String original = str;
  for (; *str; ++str)
    *str = tolower(*str);
  return original;
}

/****************/
/* File Reading */
/****************/

struct FileContent {
  char *content;
  size_t len;
};
typedef struct FileContent FileContent;

struct apaz_FileInfo {
  FILE *fptr;
  long fileLen;
};
typedef struct apaz_FileInfo apaz_FileInfo;
static inline apaz_FileInfo apaz_openSeekFile(char *filePath) {
  apaz_FileInfo ret = {.fptr = NULL, .fileLen = 0};
  ret.fptr = fopen(filePath, "rb");
  if (!ret.fptr) // Error is likely
    return ret;

    /* Get file length */
#if APAZ_HANDLE_UNLIKELY_ERRORS
  if (fseek(ret.fptr, 0, SEEK_END) == -1) {
    apaz_FileInfo nret = {.fptr = NULL, .fileLen = 0};
    return nret;
  }
  if ((ret.fileLen = ftell(ret.fptr)) == -1) {
    apaz_FileInfo nret = {.fptr = NULL, .fileLen = 0};
    return nret;
  }
  if (fseek(ret.fptr, 0, SEEK_SET) == -1) {
    apaz_FileInfo nret = {.fptr = NULL, .fileLen = 0};
    return nret;
  }
#else
  fseek(ret.fptr, 0, SEEK_END);
  ret.fileLen = ftell(ret.fptr);
  fseek(ret.fptr, 0, SEEK_SET);
#endif

  return ret;
}

// returns true on error.
static inline bool apaz_readCloseFile(char *buffer, apaz_FileInfo info) {
  bool ret = 0;
/* This could be faster with mmap, but fread is more portable. */
#if APAZ_HANDLE_UNLIKELY_ERRORS
  if (fread(buffer, info.fileLen, 1, info.fptr) != 1)
    ret = 1;
#else
  fread(buffer, info.fileLen, 1, info.fptr);
#endif
  fclose(info.fptr);
  return ret;
}

#define apaz_str_readFile(filePath)                                            \
  _apaz_str_readFile(filePath, __LINE__, __func__, __FILE__)
static inline FileContent _apaz_str_readFile(char *filePath, size_t line,
                                             const char *func,
                                             const char *file) {
  /* Open file, get length. */
  apaz_FileInfo info = apaz_openSeekFile(filePath);
  if (!info.fptr) {
    FileContent c = {.content = NULL, .len = 0};
    return c;
  }

  /* Allocate memory. */
  char *buffer = (char *)memdebug_malloc(info.fileLen + 1, line, func, file);
  if (!buffer) {
    fclose(info.fptr);
    FileContent c = {.content = NULL, .len = 0};
    return c;
  }

  /* Copy buffer, close file. */
  if (apaz_readCloseFile(buffer, info)) {
    FileContent c = {.content = NULL, .len = 0};
    return c;
  }

  /* Write null terminator */
  buffer[info.fileLen] = '\0';

  FileContent c = {.content = buffer, .len = (size_t)info.fileLen};
  return c;
}

#define String_new_fromFile(filePath)                                          \
  _String_new_fromFile(filePath, __LINE__, __func__, __FILE__)
static inline String _String_new_fromFile(char *filePath, size_t line,
                                          const char *func, const char *file) {
  /* Open file, get length. */
  apaz_FileInfo info = apaz_openSeekFile(filePath);
  if (!info.fptr)
    return NULL;

  /* Allocate a String to hold the contents. Null terminator already present. */
  String buffer = _String_new(info.fileLen, line, func, file);
  if (!buffer) {
    fclose(info.fptr);
    HANDLE_OOM(buffer);
  }

  /* Copy buffer, close file. */
  if (apaz_readCloseFile(buffer, info))
    return NULL;

  return buffer;
}

#endif // STRUTIL_INCLUDE
