#ifndef ARENA_INCLUDE
#define ARENA_INCLUDE

#include "../list.h/list.h"
#include "../memdebug.h/memdebug.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#ifndef __cplusplus
#define ALIGNOF(type) _Alignof(type)
#else
#define ALIGNOF(type) alignof(type)
#endif

#ifndef ARENA_ERRCHECK
#define ARENA_ERRCHECK 1
#endif

#define ROUND_TO_ALIGNMENT(n, alignment)
static inline size_t _roundToAlignment(size_t n, size_t alignment) {
  return (n + alignment - 1) / alignment * alignment;
}

#define ARENA_SIZE (4096 * 128)


#if MEMDEBUG
LIST_DEFINE(MemAlloc);
#endif

struct Arena;
typedef struct Arena Arena;
struct Arena {
  char *name;
  Arena *next;

  void *buffer;
  size_t buf_size;
  size_t buf_cap;

#if MEMDEBUG
  List_MemAlloc given;
#endif
};

static inline Arena *Arena_init(Arena *arena, char *name, void *buffer,
                                size_t buf_cap) {
  arena->name = name;
  arena->next = NULL;

  arena->buffer = buffer;
  arena->buf_size = 0;
  arena->buf_cap = buf_cap; // yes

#if MEMDEBUG
  arena->given = List_MemAlloc_new_cap(50);
#endif

  return arena;
}

#define Arena_new(name) _Arena_new(name, __LINE__, __func__, __FILE__)
static inline Arena _Arena_new(char *name, size_t line, const char *func,
                               const char *file) {
  Arena arena;
  void *buffer = memdebug_malloc(ARENA_SIZE, line, func, file);

#if APAZ_HANDLE_UNLIKELY_ERRORS && !MEMDEBUG
  if (!buffer) {
    printf("Could not initialize arena %s. Passed a null buffer.\n");
    exit(1);
  }
#endif

  Arena_init(&arena, name, buffer, ARENA_SIZE);

  return arena;
}

static inline Arena *_Arena_new_on(Arena *current) {

  // Allocate a new Arena.
  Arena *new_arena = (Arena *)malloc(sizeof(Arena));
  size_t reserved = _roundToAlignment(sizeof(Arena), ALIGNOF(Arena));
  void *buffer = ((char *)new_arena) + reserved;
#if APAZ_HANDLE_UNLIKELY_ERRORS && !MEMDEBUG
  if (!buffer | !new_arena) {
    printf("Out of memory allocating arena %s.\n");
    exit(1);
  }
#endif
  Arena_init(new_arena, current->name, buffer, ARENA_SIZE - reserved);

  // Throw it into the LL.
  while (current->next)
    current = current->next;
  current->next = new_arena;

  return new_arena;
}

static inline void Arena_destroy(Arena *arena, bool free_arena_ptr,
                                 bool free_buffer_ptr) {

  // The first arena could be allocated in any way, hence the args. The
  // remaining ones are always allocated by Arena_new_on().

  // Destroy the first one.
  Arena *next = arena->next;
  if (free_buffer_ptr)
    free(arena->buffer);
  arena->name = NULL;
  arena->next = NULL;
  arena->buffer = NULL;
  arena->buf_cap = 0;
  arena->buf_size = SIZE_MAX;
#if MEMDEBUG
  List_MemAlloc_destroy(arena->given);
#endif
  if (free_arena_ptr)
    free(arena);

  // Destroy the rest.
  arena = next;
  while (arena) {
    Arena *next = arena->next;
#if MEMDEBUG
    List_MemAlloc_destroy(arena->given);
#endif
    free(arena);
    arena = next;
  }
}

#define Arena_malloc(arena, bytes)                                             \
  _Arena_malloc(arena, bytes, __LINE__, __func__, __FILE__)
#define Arena_malloc_of(arena, type)                                           \
  _Arena_malloc(arena, sizeof(type), __LINE__, __func__, __FILE__)
static inline void *_Arena_malloc(Arena *arena, size_t num_bytes, size_t line,
                                  const char *func, const char *file) {
  // Align
  num_bytes = _roundToAlignment(num_bytes, ALIGNOF(max_align_t));

#if MEMDEBUG
  if (num_bytes > arena->buf_cap) {
    fprintf(stdout,
            ANSI_COLOR_RESET
            "Impossible to allocate " ANSI_COLOR_BYTE "%zu" ANSI_COLOR_RESET
            " bytes on arena: %s. Error inside " ANSI_COLOR_FUNC
            "%s()" ANSI_COLOR_RESET " on line " ANSI_COLOR_LINE
            "%zu" ANSI_COLOR_RESET " in " ANSI_COLOR_FILE "%s" ANSI_COLOR_RESET
            ".\n",
            num_bytes, arena->name, func, line, file);
    exit(1);
  }
#endif

#if MEMDEBUG && PRINT_MEMALLOCS
  Arena *original = arena;
  size_t prev_size = original->buf_size;
  char *name = original.name;
#endif

  // Build another arena on this one if it's full, and use it instead.
  if (arena->buf_size + num_bytes >= arena->buf_cap)
    arena = _Arena_new_on(arena);

  // Claim some memory.
  void *ptr = ((char *)arena->buffer) + arena->buf_size;
  arena->buf_size = arena->buf_size + num_bytes;

#if MEMDEBUG
  // Keep a record of it
  MemAlloc newalloc;
  newalloc.ptr = ptr;
  newalloc.size = num_bytes;
  newalloc.line = line;
  newalloc.func = func;
  newalloc.file = file;
  arena->given = List_MemAlloc_addeq(arena->given, newalloc);

#if PRINT_MEMALLOCS
  // Print message
  printf(ANSI_COLOR_FUNC
         "Arena_malloc(" ANSI_COLOR_RESET ANSI_COLOR_HEAD "%s" ANSI_COLOR_RESET
         ", " ANSI_COLOR_BYTE "%zu" ANSI_COLOR_RESET ANSI_COLOR_FUNC
         ")" ANSI_COLOR_RESET " -> " ANSI_COLOR_PNTR "%p" ANSI_COLOR_RESET
         " (offset: " ANSI_COLOR_BYTE "%zu" ANSI_COLOR_RESET
         " -> " ANSI_COLOR_BYTE "%zu" ANSI_COLOR_RESET
         ") on line " ANSI_COLOR_LINE "%zu" ANSI_COLOR_RESET
         " of " ANSI_COLOR_FUNC "%s()" ANSI_COLOR_RESET " in " ANSI_COLOR_FILE
         "%s" ANSI_COLOR_RESET ".\n",
         name, num_bytes, ptr, prev_size, arena->buf_size, line, func, file);
  fflush(stdout);
#endif
#endif

  return ptr;
}

#define Arena_pop(arena, bytes)                                                \
  _Arena_pop(arena, _roundToAlignment(bytes, ALIGNOF(max_align_t)), __LINE__,  \
             __func__, __FILE__)
#define Arena_pop_of(arena, type)                                              \
  _Arena_pop(arena, _roundToAlignment(sizeof(type), ALIGNOF(max_align_t)),     \
             __LINE__, __func__, __FILE__)
static inline void _Arena_pop(Arena *arena, size_t n, size_t line,
                              const char *func, const char *file) {
#if MEMDEBUG && PRINT_MEMALLOCS
  size_t prev_size = arena->buf_size;
#endif

  // Handle underflow
  arena->buf_size = arena->buf_size > n ? arena->buf_size - n : 0;

#if MEMDEBUG
#if PRINT_MEMALLOCS

  // Print message
  printf(ANSI_COLOR_FUNC
         "Arena_pop(" ANSI_COLOR_RESET ANSI_COLOR_HEAD "%s" ANSI_COLOR_RESET
         ", " ANSI_COLOR_BYTE "%zu" ANSI_COLOR_RESET ANSI_COLOR_FUNC
         ")" ANSI_COLOR_RESET " (offset: " ANSI_COLOR_BYTE
         "%zu" ANSI_COLOR_RESET " -> " ANSI_COLOR_BYTE "%zu" ANSI_COLOR_RESET
         ")" ANSI_COLOR_RESET " on line " ANSI_COLOR_LINE "%zu" ANSI_COLOR_RESET
         " of " ANSI_COLOR_FUNC "%s()" ANSI_COLOR_RESET " in " ANSI_COLOR_FILE
         "%s" ANSI_COLOR_RESET ".\n",
         arena->name, n, prev_size, arena->buf_size, line, func, file);
  fflush(stdout);
#endif
#endif
}

static inline void Arena_print_memallocs(Arena *arena) {
#if MEMDEBUG

  // Create one big list that's a copy of all the allocations stored.
  // At the same time, get the total number of bytes allocated.
  List_MemAlloc all_arena_allocs = List_MemAlloc_new_cap(1000);
  size_t total_bytes = 0;
  do {
    all_arena_allocs = List_MemAlloc_addAlleq(all_arena_allocs, arena->given);
    total_bytes += arena->buf_size;
  } while ((arena = arena->next));

  // Sort the list of all allocations. This does not modify the order of the
  // original lists.
  size_t num_arena_allocs = List_MemAlloc_len(all_arena_allocs);
  sort_memallocs(all_arena_allocs, num_arena_allocs);

  // Print the results.
  // This is mostly copy/paste from memdebug.h.
  print_heap_dump_header();
  if (num_arena_allocs) {
    char *location_file = NULL;
    char *location_func;
    size_t location_line;

    size_t total_bytes_at_location;
    size_t total_ptrs_at_location;

    for (size_t i = 0; i < num_arena_allocs; i++) {
      MemAlloc alloc = all_arena_allocs[i];

      // If the current allocation is not the same as the last, print the
      // summary of that location in the code.
      if ((alloc.line != location_line) || (alloc.func != location_func) ||
          (alloc.file != location_file)) {
        if (location_file != NULL) {
          print_alloc_summary(total_ptrs_at_location, total_bytes_at_location,
                              location_file, location_func, location_line);
        }

        location_file = (char *)alloc.file;
        location_func = (char *)alloc.func;
        location_line = alloc.line;
        total_bytes_at_location = alloc.size;
        total_ptrs_at_location = 1;
      } else {
        total_bytes_at_location += alloc.size;
        total_ptrs_at_location++;
      }
    }

    print_alloc_summary(total_ptrs_at_location, total_bytes_at_location,
                        location_file, location_func, location_line);
  }
  print_heap_summary_totals(total_bytes, num_arena_allocs);
  List_MemAlloc_destroy(all_arena_allocs);
#endif
}

#endif // ARENA_INCLUDE