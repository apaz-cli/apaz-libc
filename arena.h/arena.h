#ifndef ARENA_INCLUDE
#define ARENA_INCLUDE

#include "../list.h/list.h"
#include "../memdebug.h/memdebug.h"
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

#ifndef ARENA_ERRCHECK
#define ARENA_ERRCHECK 1
#endif

static size_t _arena_size = 0;
static inline size_t arena_get_page_size() {
#define ARENA_PAGES 128
  if (!_arena_size)
    _arena_size = (size_t)sysconf(_SC_PAGESIZE) * ARENA_PAGES;
  return _arena_size;
}

static inline size_t roundToAlignment(size_t n, size_t alignment) {
  return (n + alignment - 1) / alignment * alignment;
}

#if MEMDEBUG
LIST_DEFINE(MemAlloc);
#endif

struct Arena;
typedef struct Arena Arena;
struct Arena {
  char *name;
  Arena *next;

#if MEMDEBUG
  List_MemAlloc given;
#endif

  size_t buf_cap;
  size_t buf_size;
  char buffer[]; // FAM
};

static inline Arena *Arena_new(char *name) {
  size_t page_size = arena_get_page_size();
  size_t buffer_cap = ARENA_PAGES * page_size;

  Arena *arena = (Arena *)malloc(sizeof(Arena));
  arena->name = name;
  arena->next = NULL;
  arena->buf_cap = buffer_cap - sizeof(Arena);
  arena->buf_size = 0;

#if MEMDEBUG
  arena->given = List_MemAlloc_new_cap(50);
#endif

  return arena;
}

static inline Arena *Arena_new_on(Arena *current) {
  Arena *new_arena = Arena_new(current->name);
#if MEMDEBUG
  new_arena->given = List_MemAlloc_new_cap(50);
#endif

  while (current->next)
    current = current->next;
  current->next = new_arena;

  return new_arena;
}

static inline void Arena_destroy(Arena *arena) {
  if (!arena)
    return;

  free(arena->buffer);
#if MEMDEBUG
  List_MemAlloc_destroy(arena->given);
#endif
  Arena_destroy(arena->next);
  free(arena);
}

// #define Arena_malloc(arena, bytes) _Arena_malloc(arena, bytes,
// _Alignof(max_align_t), __LINE__, __func__, __FILE__)
#define Arena_malloc(arena, bytes)                                             \
  _Arena_malloc(arena, roundToAlignment(bytes, _Alignof(max_align_t)),         \
                __LINE__, __func__, __FILE__)
#define Arena_malloc_of(arena, type)                                           \
  _Arena_malloc(arena, roundToAlignment(sizeof(type), _Alignof(max_align_t)),  \
                __LINE__, __func__, __FILE__)
static inline void *_Arena_malloc(Arena *arena, size_t n, size_t line,
                                  const char *func, const char *file) {
#if MEMDEBUG
  if (n > arena->buf_cap) {
    fprintf(stdout,
            ANSI_COLOR_RESET
            "Cannot allocate " ANSI_COLOR_BYTE "%zu" ANSI_COLOR_RESET
            " bytes on arena: %s. Error inside " ANSI_COLOR_FUNC
            "%s()" ANSI_COLOR_RESET " on line " ANSI_COLOR_LINE
            "%zu" ANSI_COLOR_RESET " in " ANSI_COLOR_FILE "%s" ANSI_COLOR_RESET
            ".\n",
            n, arena->name, func, line, file);
    exit(1);
  }
#endif

// Build another arena on this one if it's full.
#if MEMDEBUG
#if PRINT_MEMALLOCS
  Arena *original = arena;
  size_t prev_size = original->buf_size;
#endif
#endif

  if (arena->buf_size + n >= arena->buf_cap)
    arena = Arena_new_on(arena);

  // Claim some memory.
  void *ptr = (void *)(arena->buffer + arena->buf_size);
  arena->buf_size += n;

#if MEMDEBUG
  // Keep a record of it
  MemAlloc newalloc;
  newalloc.ptr = ptr;
  newalloc.size = n;
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
         original->name, n, ptr, prev_size, arena->buf_size, line, func, file);
  fflush(stdout);
#endif
#endif
  return ptr;
}

#define Arena_pop(arena, bytes)                                                \
  _Arena_pop(arena, roundToAlignment(bytes, _Alignof(max_align_t)), __LINE__,  \
             __func__, __FILE__)
#define Arena_pop_of(arena, type)                                              \
  _Arena_pop(arena, roundToAlignment(sizeof(type), _Alignof(max_align_t)),     \
             __LINE__, __func__, __FILE__)
static inline void _Arena_pop(Arena *arena, size_t n, size_t line,
                              const char *func, const char *file) {
#if MEMDEBUG
#if PRINT_MEMALLOCS
  size_t prev_size = arena->buf_size;
#endif
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