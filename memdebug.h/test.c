#include <stdio.h>
#include <stdlib.h>

// #define PRINT_MEMALLOCS to zero to disable debug messages on every allocation.
// Allocations will still be tracked, and print_heap() will still be available.

// #define PRINT_MEMALLOCS 0

// #define MEMDEBUG to zero to disable all wrapping and features.
// No allocations will be tracked, and print_heap() and any additional debugging
// features that are added will be defined to nothing and will be optimized out
// completely by the compiler.

// #define MEMDEBUG 0
#include "memdebug.h"

int main() {
    // Print debug messages on allocation/free
    void* ptr = malloc(1);
    ptr = realloc(ptr, 10);
    free(ptr);

    // Find memory leaks
    malloc(20);
    malloc(25);

    print_heap();

    // Catch out of memory errors
    // malloc(9223372036854775807);

    // Explode gracefully
    void* invalid_ref = (void*)0x1;
    free(invalid_ref);
}