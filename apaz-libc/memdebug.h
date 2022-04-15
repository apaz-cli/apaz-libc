#ifndef MEMDEBUG_INCLUDE
#define MEMDEBUG_INCLUDE

#include "mutex.h"

/*******************************/
/* Pretty ANSI Terminal Colors */
/*******************************/

#ifndef ANSI_TERMINAL

#ifdef _WIN32
#define ANSI_TERMINAL 0
#else
#define ANSI_TERMINAL 1
#endif

#endif

#if ANSI_TERMINAL
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"
#else
#define ANSI_COLOR_RED ""
#define ANSI_COLOR_GREEN ""
#define ANSI_COLOR_YELLOW ""
#define ANSI_COLOR_BLUE ""
#define ANSI_COLOR_MAGENTA ""
#define ANSI_COLOR_CYAN ""
#define ANSI_COLOR_RESET ""
#endif

#define ANSI_COLOR_HEAD ANSI_COLOR_RED
#define ANSI_COLOR_PNIC ANSI_COLOR_RED
#define ANSI_COLOR_PNTR ANSI_COLOR_MAGENTA
#define ANSI_COLOR_BYTE ANSI_COLOR_BLUE
#define ANSI_COLOR_FILE ANSI_COLOR_GREEN
#define ANSI_COLOR_FUNC ANSI_COLOR_YELLOW
#define ANSI_COLOR_LINE ANSI_COLOR_CYAN


#ifndef MEMDEBUG
#define MEMDEBUG 0
#endif

// PRINT_MEMALLOCS is used to control debug error messages for every allocation.
// Still wraps malloc() and tracks allocations for print_heap() if this is off.
#if MEMDEBUG
#ifndef PRINT_MEMALLOCS
#define PRINT_MEMALLOCS 0
#endif
#endif

#if MEMDEBUG
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline void low_mem_print_heap();
static inline void print_heap();

/******************************************/
/* Void Pointer Hash Function For Hashmap */
/******************************************/

static inline size_t
log_base_2(const size_t num) {
    if (num == 1)
        return 0;
    return 1 + log_base_2(num / 2);
}

/* 
 * Note that, by all accounts, this is a bad idea. 
 * How ptr_hash behaves is entirely implementation specific because how uintptr_t is implementation specific. 
 * However, it behaves in the sane way that you'd expect across most popular compilers.
 */
#define MAP_BUF_SIZE 100000
static inline size_t
ptr_hash(void* val) {
    size_t logsize = log_base_2(1 + sizeof(void*));
    size_t shifted = (size_t)((uintptr_t)val) >> logsize;
    size_t other = (size_t)((uintptr_t)val) << (8 - logsize);
    size_t xed = shifted ^ other;
    size_t ans = xed % MAP_BUF_SIZE;
    return ans;
}

/**************************************/
/* Global Allocation Tracking Hashmap */
/**************************************/

// Mutex to guard the allocation structure.
static mutex_t alloc_mutex = MUTEX_INITIALIZER;
#define MEMDEBUG_LOCK_MUTEX mutex_lock(&alloc_mutex);
#define MEMDEBUG_UNLOCK_MUTEX mutex_unlock(&alloc_mutex);

struct MemAlloc;
typedef struct MemAlloc MemAlloc;
struct MemAlloc {
    void* ptr;
    size_t size;
    size_t line;
    const char* func;
    const char* file;
};

static inline bool
compare_memallocs(MemAlloc a1, MemAlloc a2) {
    // First by file
    int cmp = strcmp(a1.file, a2.file);
    if (!cmp) {
        // Then by line
        return a1.line > a2.line;
    } else {
        return (cmp > 0) ? true : false;
    }
}

static inline void
sort_memallocs(MemAlloc* allocs, size_t n) {
    // Sort the buffer. This is a shellsort.
    size_t interval, i, j;
    MemAlloc temp;
    for (interval = n / 2; interval > 0; interval /= 2) {
        for (i = interval; i < n; i += 1) {
            temp = allocs[i];
            for (j = i; j >= interval && compare_memallocs(allocs[j - interval], temp); j -= interval) {
                allocs[j] = allocs[j - interval];
            }
            allocs[j] = temp;
        }
    }
}

struct MapMember;
typedef struct MapMember MapMember;
struct MapMember {
    MemAlloc alloc;
    MapMember* next;
};

// Global alloc hash map
static MapMember allocs[MAP_BUF_SIZE];
static bool memallocs_initialized = false;
static size_t num_allocs = 0;

/***************/
/* Map Methods */
/***************/
static inline void OOM(size_t line, const char* func, const char* file, size_t num_bytes);

static inline void
memallocs_init() {
    for (size_t i = 0; i < MAP_BUF_SIZE; i++) {
        allocs[i].alloc.ptr = NULL;
        allocs[i].next = NULL;
    }
    memallocs_initialized = true;
}

static inline void
alloc_add(MemAlloc alloc) {
    if (!memallocs_initialized)
        memallocs_init();

    num_allocs++;

    // Travel to the bucket to put this allocation into.
    MapMember* bucket = (MapMember*)allocs + ptr_hash(alloc.ptr);

    // If we can insert into the bucket directly, do so.
    if (bucket->alloc.ptr == NULL) {
        bucket->alloc = alloc;
        return;
    }

    // Otherwise, traverse the linked list until you find the end
    while (bucket->next != NULL) {
        bucket = bucket->next;
    }

    // Create a new LL node off the previous for the allocation
    bucket->next = (MapMember*)malloc(sizeof(MapMember));
    if (!bucket->next) OOM(__LINE__ - 1, __func__, __FILE__, sizeof(MapMember));
    bucket = bucket->next;

    // Put the allocation into it.
    bucket->alloc = alloc;
    bucket->next = NULL;
}

// returns the pointer, or NULL if not found.
static inline bool
alloc_remove(void* ptr) {
    if (!memallocs_initialized) {
        memallocs_init();
        return false;
    }

    MapMember* previous = NULL;
    MapMember* bucket = (MapMember*)allocs + ptr_hash(ptr);

    // Traverse the bucket looking for the pointer
    while (bucket) {
        if (bucket->alloc.ptr == ptr) {
            // Remove this bucket node from the linked list
            if (!previous) {
                // Copy from the next node into the original array.
                if (bucket->next) {
                    MapMember* to_free = bucket->next;
                    bucket->alloc = to_free->alloc;
                    bucket->next = to_free->next;
                    free(to_free);
                } else {
                    bucket->alloc.ptr = NULL;
                    bucket->next = NULL;
                }
            } else {
                // Point the previous allocation at the next allocation.
                // Then free the bucket.
                previous->next = bucket->next;
                free(bucket);
            }
            num_allocs--;
            return true;
        } else {
            previous = bucket;
            bucket = bucket->next;
        }
    }

    return false;
}

/****************/
/* Memory Panic */
/****************/
#ifndef MEMPANIC_EXIT_STATUS
#define MEMPANIC_EXIT_STATUS 10
#endif
#ifndef OOM_EXIT_STATUS
#define OOM_EXIT_STATUS 11
#endif

static inline void
mempanic(void* badptr, const char* message, size_t line, const char* func, const char* file) {
    printf(ANSI_COLOR_PNIC "\nMEMORY PANIC: %s\n" ANSI_COLOR_RESET
               ANSI_COLOR_PNTR "Pointer: %p\n" ANSI_COLOR_RESET
                   ANSI_COLOR_LINE "On line: %zu\n" ANSI_COLOR_RESET
                       ANSI_COLOR_FUNC "In function: %s()\n" ANSI_COLOR_RESET
                           ANSI_COLOR_FILE "In file: %s\n" ANSI_COLOR_RESET
                               ANSI_COLOR_PNIC "Aborted.\n" ANSI_COLOR_RESET,
           message, badptr, line, func, file);
    fflush(stdout);
    exit(MEMPANIC_EXIT_STATUS);
}

static inline void
OOM(size_t line, const char* func, const char* file, size_t num_bytes) {
    if (strcmp(file, "memdebug.h") == 0) {
        printf(ANSI_COLOR_PNIC
               "\nIronically, this program has run out of memory keeping track of or printing memory allocations."
               "\nThe error did not happen inside your program, but it was likely caused by it because you allocated too much memory." ANSI_COLOR_RESET);
    }
    printf(
        ANSI_COLOR_PNIC
        "\n*****************"
        "\n* Out of Memory *"
        "\n*****************\n" ANSI_COLOR_RESET
            ANSI_COLOR_FILE "In file: %s\n" ANSI_COLOR_RESET
                ANSI_COLOR_FUNC "In function: %s()\n" ANSI_COLOR_RESET
                    ANSI_COLOR_LINE "On line: %zu\n" ANSI_COLOR_RESET
                        ANSI_COLOR_BYTE "Could not allocate %zu bytes.\n" ANSI_COLOR_RESET,
        file, func, line, num_bytes);

    low_mem_print_heap();
    exit(OOM_EXIT_STATUS);
}

/**************************/
/* Print Helper Functions */
/**************************/

static inline void
print_alloc_summary(size_t total_ptrs_at_location, size_t total_bytes_at_location, char* location_file, char* location_func, size_t location_line) {
    if (total_ptrs_at_location == 1) {
        printf(
            ANSI_COLOR_PNTR "%zu pointer has been allocated" ANSI_COLOR_RESET
                ANSI_COLOR_BYTE " totalling %zu bytes" ANSI_COLOR_RESET
                    ANSI_COLOR_FILE " in file: %s" ANSI_COLOR_RESET
                        ANSI_COLOR_FUNC " in function: %s()" ANSI_COLOR_RESET
                            ANSI_COLOR_LINE " on line: %zu.\n" ANSI_COLOR_RESET,
            total_ptrs_at_location, total_bytes_at_location, location_file, location_func, location_line);
    } else {
        printf(
            ANSI_COLOR_PNTR "%zu pointers have been allocated" ANSI_COLOR_RESET
                ANSI_COLOR_BYTE " totalling %zu bytes" ANSI_COLOR_RESET
                    ANSI_COLOR_FILE " in file: %s" ANSI_COLOR_RESET
                        ANSI_COLOR_FUNC " in function: %s()" ANSI_COLOR_RESET
                            ANSI_COLOR_LINE " on line: %zu.\n" ANSI_COLOR_RESET,
            total_ptrs_at_location, total_bytes_at_location, location_file, location_func, location_line);
    }
}

static inline void
print_heap_summary_totals(size_t total_allocated, size_t num_allocs) {
    printf(
        "\nTotal size in bytes: %zu"
        "\nTotal number of allocations: %zu"
        "\n\n\n",
        total_allocated, num_allocs);
    fflush(stdout);
}

static inline void
print_heap_dump_header() {
    printf(ANSI_COLOR_HEAD "\n*************\n* HEAP DUMP *\n*************\n" ANSI_COLOR_RESET);
}

/**********************/
/* Externally Visible */
/**********************/

// Print all of the memory allocations of this program.
static inline void 
print_heap() {
    size_t total_allocated = 0;
    size_t allocs_idx = 0;
    MemAlloc* all_allocs = (MemAlloc*)malloc(sizeof(MemAlloc) * num_allocs);
    if (!all_allocs) OOM(__LINE__ - 1, __func__, __FILE__, sizeof(MemAlloc) * num_allocs);

    MEMDEBUG_LOCK_MUTEX;
    if (!memallocs_initialized)
        memallocs_init();

    // Pack the buffer
    for (size_t i = 0; i < MAP_BUF_SIZE; i++) {
        MapMember* bucket = (MapMember*)(allocs + i);
        while (bucket->alloc.ptr != NULL) {
            all_allocs[allocs_idx++] = bucket->alloc;
            total_allocated += bucket->alloc.size;
            if (bucket->next) {
                bucket = bucket->next;
            } else {
                break;
            }
        }
    }
    MEMDEBUG_UNLOCK_MUTEX;

    // Sort the buffer
    sort_memallocs(all_allocs, allocs_idx);

    // Print the formatted results
    print_heap_dump_header();
    if (allocs_idx) {
        char* location_file = NULL;
        char* location_func;
        size_t location_line;

        size_t total_bytes_at_location;
        size_t total_ptrs_at_location;

        for (size_t i = 0; i < allocs_idx; i++) {
            MemAlloc alloc = all_allocs[i];

            // If the current allocation is not the same as the last, print the summary of that location in the code.
            if ((alloc.line != location_line) || (alloc.func != location_func) || (alloc.file != location_file)) {
                if (location_file != NULL) {
                    print_alloc_summary(total_ptrs_at_location, total_bytes_at_location, location_file, location_func, location_line);
                }

                location_file = (char*)alloc.file;
                location_func = (char*)alloc.func;
                location_line = alloc.line;
                total_bytes_at_location = alloc.size;
                total_ptrs_at_location = 1;
            } else {
                total_bytes_at_location += alloc.size;
                total_ptrs_at_location++;
            }
        }

        print_alloc_summary(total_ptrs_at_location, total_bytes_at_location, location_file, location_func, location_line);
    }

    print_heap_summary_totals(total_allocated, num_allocs);

    free(all_allocs);
}

// This is the same as print_heap() except it doesn't sort
// because it's meant to be called when the program is out of memory.
static inline void
low_mem_print_heap() {
    size_t total_allocated = 0;

    MEMDEBUG_LOCK_MUTEX;

    if (!memallocs_initialized)
        memallocs_init();

    // For each bucket, traverse over each and print all the allocations
    print_heap_dump_header();
    for (size_t i = 0; i < MAP_BUF_SIZE; i++) {
        MapMember* bucket = (MapMember*)(allocs + i);
        while (bucket->alloc.ptr != NULL) {
            MemAlloc alloc = bucket->alloc;
            printf(
                ANSI_COLOR_PNTR "Heap ptr: %p" ANSI_COLOR_RESET
                    ANSI_COLOR_BYTE " of size: %zu" ANSI_COLOR_RESET
                        ANSI_COLOR_FILE " Allocated in file: %s" ANSI_COLOR_RESET
                            ANSI_COLOR_LINE " On line: %zu\n" ANSI_COLOR_RESET,
                alloc.ptr, alloc.size, alloc.file, alloc.line);
            total_allocated += alloc.size;

            if (bucket->next) {
                bucket = bucket->next;
            } else {
                break;
            }
        }
    }

    MEMDEBUG_UNLOCK_MUTEX;

    print_heap_summary_totals(total_allocated, num_allocs);
}

static inline size_t 
get_num_allocs() {
    return num_allocs;
}

/*********************************************/
/* malloc(), realloc(), free() Redefinitions */
/*********************************************/

static inline void*
memdebug_malloc(size_t n, size_t line, const char* func, const char* file) {
    // Call malloc()
    void* ptr = malloc(n);
    if (!ptr) OOM(line, func, file, n);

#if PRINT_MEMALLOCS
    // Print message
    printf(ANSI_COLOR_FUNC "malloc(" ANSI_COLOR_RESET
               ANSI_COLOR_BYTE "%zu" ANSI_COLOR_RESET
                   ANSI_COLOR_FUNC ")" ANSI_COLOR_RESET
                           " -> " ANSI_COLOR_PNTR "%p" ANSI_COLOR_RESET
                           " on line " ANSI_COLOR_LINE "%zu" ANSI_COLOR_RESET
                           " of " ANSI_COLOR_FUNC "%s()" ANSI_COLOR_RESET
                           " in " ANSI_COLOR_FILE "%s" ANSI_COLOR_RESET
                           ".\n",
           n, ptr, line, func, file);
    fflush(stdout);
#endif

    // Keep a record of it
    MemAlloc newalloc;
    newalloc.ptr = ptr;
    newalloc.size = n;
    newalloc.line = line;
    newalloc.func = func;
    newalloc.file = file;

    MEMDEBUG_LOCK_MUTEX;
    alloc_add(newalloc);
    MEMDEBUG_UNLOCK_MUTEX;
    return ptr;
}

static inline void*
memdebug_realloc(void* ptr, size_t n, size_t line, const char* func, const char* file) {
    MEMDEBUG_LOCK_MUTEX;

    // Check to make sure the allocation exists, and keep track of the location
    if (ptr != NULL){
        bool removed = alloc_remove(ptr);
        if (!removed) {
            mempanic(ptr, "Tried to realloc() an invalid pointer.", line, func, file);
        }
    }

    // Call realloc()
    void* newptr = realloc(ptr, n);
    if (!newptr) OOM(line, func, file, n);

#if PRINT_MEMALLOCS
    // Print message
    printf(
        ANSI_COLOR_FUNC "realloc(" ANSI_COLOR_RESET
            ANSI_COLOR_PNTR "%p" ANSI_COLOR_RESET
                ANSI_COLOR_FUNC ", " ANSI_COLOR_RESET
                    ANSI_COLOR_BYTE "%zu" ANSI_COLOR_RESET
                        ANSI_COLOR_FUNC ")" ANSI_COLOR_RESET
                        " -> " ANSI_COLOR_PNTR "%p" ANSI_COLOR_RESET
                        " on line " ANSI_COLOR_LINE "%zu" ANSI_COLOR_RESET
                        " of " ANSI_COLOR_FUNC "%s()" ANSI_COLOR_RESET
                        " in " ANSI_COLOR_FUNC "%s" ANSI_COLOR_RESET
                        ".\n",
        ptr, n, newptr, line, func, file);
    fflush(stdout);
#endif

    // Update the record of allocations
    MemAlloc newalloc;
    newalloc.ptr = newptr;
    newalloc.size = n;
    newalloc.line = line;
    newalloc.func = func;
    newalloc.file = file;
    alloc_add(newalloc);

    MEMDEBUG_UNLOCK_MUTEX;

    return newptr;
}

static inline void
memdebug_free(void* ptr, size_t line, const char* func, const char* file) {
    MEMDEBUG_LOCK_MUTEX;

    // Check to make sure the allocation exists, and keep track of the location
    bool removed = alloc_remove(ptr);
    if (ptr != NULL && !removed) {
        mempanic(ptr, "Tried to free() an invalid pointer.", line, func, file);
    }

    MEMDEBUG_UNLOCK_MUTEX;

    // Call free()
    free(ptr);

#if PRINT_MEMALLOCS
    // Print message
    printf(
        ANSI_COLOR_FUNC "free(" ANSI_COLOR_RESET
            ANSI_COLOR_PNTR "%p" ANSI_COLOR_RESET
                ANSI_COLOR_FUNC ")" ANSI_COLOR_RESET
                        " on line " ANSI_COLOR_LINE "%zu" ANSI_COLOR_RESET
                        " of " ANSI_COLOR_FUNC "%s()" ANSI_COLOR_RESET
                        " in " ANSI_COLOR_FILE "%s" ANSI_COLOR_RESET
                        ".\n",
        ptr, line, func, file);
    fflush(stdout);
#endif
}

// Wrap malloc(), realloc(), free() with the new functionality
static inline void*  original_malloc(size_t n) { return malloc(n); }
static inline void*  original_realloc(void* ptr, size_t n) { return realloc(ptr, n); }
static inline void   original_free(void* ptr) { free(ptr); }
#define malloc(n)       memdebug_malloc(      n, __LINE__, __func__, __FILE__)
#define realloc(ptr, n) memdebug_realloc(ptr, n, __LINE__, __func__, __FILE__)
#define free(ptr)       memdebug_free(   ptr,    __LINE__, __func__, __FILE__)

#else  // MEMDEBUG flag is disabled
/*************************************************************************************/
/* Define externally visible functions to do nothing when debugging flag is disabled */
/*************************************************************************************/
#include <stdlib.h>
static inline void*  original_malloc(size_t n) { return malloc(n); }
static inline void*  original_realloc(void* ptr, size_t n) { return realloc(ptr, n); }
static inline void   original_free(void* ptr) { free(ptr); }
static inline void*  memdebug_malloc(size_t n, size_t line, const char* func, const char* file) { (void)func; (void)file; (void)line; return malloc(n); }
static inline void*  memdebug_realloc(void* ptr, size_t n, size_t line, const char* func, const char* file) { (void)func; (void)file; (void)line; return realloc(ptr, n); }
static inline void   memdebug_free(void* ptr, size_t line, const char* func, const char* file) { (void)func; (void)file; (void)line; free(ptr); }
static inline void   print_heap() {}
static inline void   low_mem_print_heap() {}
static inline size_t get_num_allocs() { return 0; }
#endif
#endif  // MEMDEBUG_INCLUDE