# memdebug.h
A drop-in replacement for malloc that overrides malloc(), realloc(), and free() for the remainder of the translation unit for easy memory debugging.


# Example usage: 
```c
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
```

## Output
```
apaz@apaz-laptop:~/git/memdebug.h$ gcc test.c
apaz@apaz-laptop:~/git/memdebug.h$ ./a.out
malloc(1) -> 0x56120d0a7260 on line 19 of main() in test.c.
realloc(0x56120d0a7260, 10) -> 0x56120d0a7260 on line 20 of main() in test.c.
free(0x56120d0a7260) on line 21 of main() in test.c.
malloc(20) -> 0x56120d0a7260 on line 24 of main() in test.c.
malloc(25) -> 0x56120d0a7690 on line 25 of main() in test.c.

*************
* HEAP DUMP *
*************
1 pointer has been allocated totalling 20 bytes in file: test.c in function: main on line: 24.
1 pointer has been allocated totalling 25 bytes in file: test.c in function: main on line: 25.

Total Heap size in bytes: 45
Total number of heap allocations: 2



MEMORY PANIC: Tried to free() an invalid pointer.
Pointer: 0x1
On line: 34
In function: main()
In file: test.c
Aborted.
```
