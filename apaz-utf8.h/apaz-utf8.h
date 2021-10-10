#ifndef UTF8_INCLUDE
#define UTF8_INCLUDE

#include "../apaz-string.h/apaz-string.h"

#include <inttypes.h>
#include <limits.h>
#include <locale.h>
#include <stdint.h>

#define UTF8_END -1
#define UTF8_ERROR -2

typedef int utf8_t;

struct UTF8State {
  int idx;
  int len;
  int chr;
  int byte;
  char *inp;
};
typedef struct UTF8State UTF8State;

struct UTF8FileContent {
  size_t len;
  utf8_t *content;
};
typedef struct UTF8FileContent UTF8FileContent;

/* Get the next byte. It returns UTF8_END if there are no more bytes. */
static inline int _utf8_cont_8(UTF8State *state) {
  int c;
  if (state->idx >= state->len)
    return UTF8_END;
  c = state->inp[state->idx] & 0xFF;
  state->idx += 1;
  return c;
}

/*
 * Get the 6-bit payload of the next continuation byte.
 * Return UTF8_ERROR if it is not a contination byte.
 */
static inline int _utf8_cont_6(UTF8State *state) {
  int c = _utf8_cont_8(state);
  return ((c & 0xC0) == 0x80) ? (c & 0x3F) : UTF8_ERROR;
}

/* Initialize the UTF-8 decoder. The decoder is not reentrant. */
static inline void utf8_decode_init(UTF8State *state, char *str, int len) {
  state->idx = 0;
  state->inp = str;
  state->len = len;
  state->chr = 0;
  state->byte = 0;
}

/* Get the current byte offset. This is generally used in error reporting. */
static inline int utf8_at_byte(UTF8State *state) { return state->byte; }

/*
 * Get the current character offset. This is generally used in error reporting.
 * The character offset matches the byte offset if the text is strictly ASCII.
 */
static inline int utf8_at_character(UTF8State *state) {
  return (state->chr > 0) ? state->chr - 1 : 0;
}

/*
    Extract the next character.
    Returns: the character (between 0 and 1114111)
         or  UTF8_END   (the end)
         or  UTF8_ERROR (error)
*/
static inline utf8_t utf8_decode_next(UTF8State *state) {
  int c;    /* the first byte of the character */
  int c1;   /* the first continuation character */
  int c2;   /* the second continuation character */
  int c3;   /* the third continuation character */
  utf8_t r; /* the result */

  if (state->idx >= state->len)
    return state->idx == state->len ? UTF8_END : UTF8_ERROR;

  state->byte = state->idx;
  state->chr += 1;
  c = _utf8_cont_8(state);
  /* Zero continuation (0 to 127) */
  if ((c & 0x80) == 0) {
    return c;
  }
  /* One continuation (128 to 2047) */
  if ((c & 0xE0) == 0xC0) {
    c1 = _utf8_cont_6(state);
    if (c1 >= 0) {
      r = ((c & 0x1F) << 6) | c1;
      if (r >= 128) {
        return r;
      }
    }
    /* Two continuations (2048 to 55295 and 57344 to 65535) */
  } else if ((c & 0xF0) == 0xE0) {
    c1 = _utf8_cont_6(state);
    c2 = _utf8_cont_6(state);
    if ((c1 | c2) >= 0) {
      r = ((c & 0x0F) << 12) | (c1 << 6) | c2;
      if (r >= 2048 && (r < 55296 || r > 57343)) {
        return r;
      }
    }
    /* Three continuations (65536 to 1114111) */
  } else if ((c & 0xF8) == 0xF0) {
    c1 = _utf8_cont_6(state);
    c2 = _utf8_cont_6(state);
    c3 = _utf8_cont_6(state);
    if ((c1 | c2 | c3) >= 0) {
      r = ((c & 0x07) << 18) | (c1 << 12) | (c2 << 6) | c3;
      if (r >= 65536 && r <= 1114111) {
        return r;
      }
    }
  }
  return UTF8_ERROR;
}

#define UTF8ReadFile(filepath)                                                 \
  _UTF8ReadFile(filePath, __LINE__, __func__, __FILE__)
static inline utf8_t *_UTF8ReadFile(char *filePath, size_t line,
                                    const char *func, const char *file) {
  /* Open file, get length. */
  apaz_FileInfo info = apaz_openSeekFile(filePath);
  if (!info.fptr)
    return NULL;

  /* Allocate memory. */
  char *buffer = (char *)memdebug_malloc(info.fileLen + 1, line, func, file);
  if (!buffer) {
    fclose(info.fptr);
    HANDLE_OOM(buffer);
  }

  /* Copy buffer, close file. */
  if (apaz_readCloseFile(buffer, info))
    return NULL;

  /* Write null terminator */
  buffer[info.fileLen] = '\0';

  /* Allocate memory for the utf8 string buffer */
  utf8_t *utf8_buffer = (utf8_t *)memdebug_malloc(
      (sizeof(utf8_t) * info.fileLen) + sizeof(utf8_t), line, func, file);

  /* Copy into it the file that's been read. */
  UTF8State state;
  utf8_decode_init(&state, buffer, info.fileLen);
  for (size_t i = 0; i < info.fileLen; i++)
    utf8_buffer[i] = utf8_decode_next(&state);
  utf8_buffer[info.fileLen] = '\0';

  /* Delete the buffer we read the file into. */
  free(buffer); 

  return utf8_buffer;
}

#endif // UTF8_INCLUDE