#define MEMDEBUG 1
#define PRINT_MEMALLOCS 1
#include <apaz-libc.h>

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
