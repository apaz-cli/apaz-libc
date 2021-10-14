#ifndef UTF8_INCLUDE
#define UTF8_INCLUDE

#include "../apaz-string.h/apaz-string.h"

#include <inttypes.h>
#include <limits.h>
#include <locale.h>
#include <stdint.h>

#define UTF8_END -1
#define UTF8_ERR -2

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
  utf8_t *content;
  size_t len;
};
typedef struct UTF8FileContent UTF8FileContent;

/**********/
/* DECODE */
/**********/

/* Get the next byte. It returns UTF8_END if there are no more bytes. */
static inline int _utf8_cont_8(UTF8State *state) {
  if (state->idx >= state->len)
    return UTF8_END;
  int c = state->inp[state->idx] & 0xFF;
  state->idx += 1;
  return c;
}

/*
 * Get the 6-bit payload of the next continuation byte.
 * Return UTF8_ERR if it is not a contination byte.
 */
static inline int _utf8_cont_6(UTF8State *state) {
  int c = _utf8_cont_8(state);
  return ((c & 0xC0) == 0x80) ? (c & 0x3F) : UTF8_ERR;
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
static inline int utf8_atByte(UTF8State *state) { return state->byte; }

/*
 * Get the current character offset. This is generally used in error reporting.
 * The character offset matches the byte offset if the text is strictly ASCII.
 */
static inline int utf8_atCharacter(UTF8State *state) {
  return (state->chr > 0) ? state->chr - 1 : 0;
}

/*
 * Extract the next unicode code point.
 * Returns the character, or UTF8_END, or UTF8_ERR.
 */
static inline utf8_t utf8_decodeNext(UTF8State *state) {
  int c;    /* the first byte of the character */
  int c1;   /* the first continuation character */
  int c2;   /* the second continuation character */
  int c3;   /* the third continuation character */
  utf8_t r; /* the result */

  if (state->idx >= state->len)
    return state->idx == state->len ? UTF8_END : UTF8_ERR;

  state->byte = state->idx;
  state->chr += 1;
  c = _utf8_cont_8(state);

  /* Zero continuation (0 to 127) */
  if ((c & 0x80) == 0)
    return c;

  /* One continuation (128 to 2047) */
  if ((c & 0xE0) == 0xC0) {
    c1 = _utf8_cont_6(state);
    if (c1 >= 0) {
      r = ((c & 0x1F) << 6) | c1;
      if (r >= 128)
        return r;
    }
    /* Two continuations (2048 to 55295 and 57344 to 65535) */
  } else if ((c & 0xF0) == 0xE0) {
    c1 = _utf8_cont_6(state);
    c2 = _utf8_cont_6(state);
    if ((c1 | c2) >= 0) {
      r = ((c & 0x0F) << 12) | (c1 << 6) | c2;
      if (r >= 2048 && (r < 55296 || r > 57343))
        return r;
    }
    /* Three continuations (65536 to 1114111) */
  } else if ((c & 0xF8) == 0xF0) {
    c1 = _utf8_cont_6(state);
    c2 = _utf8_cont_6(state);
    c3 = _utf8_cont_6(state);
    if ((c1 | c2 | c3) >= 0) {
      r = ((c & 0x07) << 18) | (c1 << 12) | (c2 << 6) | c3;
      if (r >= 65536 && r <= 1114111)
        return r;
    }
  }
  return UTF8_ERR;
}

/**********/
/* ENCODE */
/**********/

/*
 * Encodes the given utf8 code point into the given buffer.
 * Returns the number of characters in the buffer used.
 */
static inline size_t utf8_encode_codepoint(uint32_t codepoint, char *buf4) {
  if (codepoint <= 0x7F) {
    buf4[0] = (char)codepoint;
    return 1;
  } else if (codepoint <= 0x07FF) {
    buf4[0] = (char)(((codepoint >> 6) & 0x1F) | 0xC0);
    buf4[1] = (char)(((codepoint >> 0) & 0x3F) | 0x80);
    return 2;
  } else if (codepoint <= 0xFFFF) {
    buf4[0] = (char)(((codepoint >> 12) & 0x0F) | 0xE0);
    buf4[1] = (char)(((codepoint >> 6) & 0x3F) | 0x80);
    buf4[2] = (char)(((codepoint >> 0) & 0x3F) | 0x80);
    return 3;
  } else if (codepoint <= 0x10FFFF) {
    buf4[0] = (char)(((codepoint >> 18) & 0x07) | 0xF0);
    buf4[1] = (char)(((codepoint >> 12) & 0x3F) | 0x80);
    buf4[2] = (char)(((codepoint >> 6) & 0x3F) | 0x80);
    buf4[3] = (char)(((codepoint >> 0) & 0x3F) | 0x80);
    return 4;
  }
  return 0;
}

#define utf8_encode(codepoints, len)                                           \
  _utf8_encode(codepoints, len, __LINE__, __func__, __FILE__)
static inline FileContent _utf8_encode(utf8_t *codepoints, size_t len,
                                       size_t line, const char *func,
                                       const char *file) {
  // Allocate at least enough memory.
  char buf4[4];
  char *out_buf =
      (char *)memdebug_malloc(len * sizeof(utf8_t) + 1, line, func, file);
  if (!out_buf) {
    FileContent c = {.content = NULL, .len = 0};
    return c;
  }
  size_t characters_used = 0;

  // For each unicode rune
  for (size_t i = 0; i < len; i++) {
    // Decode it, handle error
    size_t used = utf8_encode_codepoint(codepoints[i], buf4);
    if (!used) {
      FileContent c = {.content = NULL, .len = 0};
      return c;
    }

    // Copy the result onto the end of the buffer
    for (size_t j = 0; j < used; j++)
      out_buf[characters_used++] = buf4[j];
  }

  // Add the null terminator, shrink to size
  out_buf[characters_used] = '\0';
  out_buf =
      (char *)memdebug_realloc(out_buf, characters_used + 1, line, func, file);

  // Return the result
  FileContent c = {.content = out_buf, .len = characters_used};
  return c;
}

static inline FileContent utf8_encode_len(utf8_t *codepoints) {
  // strlen()
  const utf8_t *s;
  for (s = codepoints; *s; ++s)
    ;
  size_t len = (s - codepoints);
  return utf8_encode(codepoints, len);
}

static inline FileContent utf8_encode_content(UTF8FileContent content) {
  return utf8_encode(content.content, content.len);
}

/*************/
/* READ FILE */
/*************/

#define utf8_readFile(filePath)                                                \
  _utf8_readFile(filePath, __LINE__, __func__, __FILE__)
static inline UTF8FileContent _utf8_readFile(char *filePath, size_t line,
                                             const char *func,
                                             const char *file) {

  /* Open file, get length. */
  apaz_FileInfo info = apaz_openSeekFile(filePath);
  if (!info.fptr) {
    UTF8FileContent ret = {.content = NULL, .len = 0};
    return ret;
  }

  /* Allocate memory for the file contents. */
  char *buffer = (char *)memdebug_malloc(info.fileLen + 1, line, func, file);
  if (!buffer) {
    fclose(info.fptr);
    UTF8FileContent ret = {.content = NULL, .len = 0};
    return ret;
  }

  /* Copy file contents into memory, close file. */
  if (apaz_readCloseFile(buffer, info)) {
    memdebug_free(buffer, line, func, file); // File already closed
    UTF8FileContent ret = {.content = NULL, .len = 0};
    return ret;
  }

  /* Write null terminator */
  buffer[info.fileLen] = '\0';

  /* Convert from utf8 to unicode code points */

  /*
   * Allocate memory for the utf8 string buffer.
   * If the file contains multibyte characters,
   * this is more memory than is required.
   */
  utf8_t *utf8_buffer = (utf8_t *)memdebug_malloc(
      (sizeof(utf8_t) * info.fileLen) + sizeof(utf8_t), line, func, file);

  /* Initialize the parser state */
  UTF8State state;
  utf8_t utfchar = ~UTF8_END;
  size_t num_chars = 0;
  utf8_decode_init(&state, buffer, info.fileLen);

  /* Parse unicode code points from the file. */
  while (true) {
    utfchar = utf8_decodeNext(&state);
    if (utfchar == UTF8_ERR) {
      memdebug_free(buffer, line, func, file);
      memdebug_free(utf8_buffer, line, func, file);
      UTF8FileContent ret = {.content = NULL, .len = 0};
      return ret;
    } else if (utfchar == UTF8_END) {
      break;
    }
    utf8_buffer[num_chars++] = utfchar;
  }
  utf8_buffer[num_chars] = '\0';
  utf8_buffer = (utf8_t *)memdebug_realloc(
      utf8_buffer, sizeof(utf8_t) * (num_chars + 1), line, func, file);

  /* Delete the buffer we read the file into. */
  memdebug_free(buffer, line, func, file);

  UTF8FileContent ret = {.content = utf8_buffer, .len = num_chars};
  return ret;
}

#endif // UTF8_INCLUDE