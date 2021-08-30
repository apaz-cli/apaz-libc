# apaz-libc
## My personal C library.

This library is composed of 4 other libraries, with more to come.

<br/>

### Jump to Library Examples:

* [memdebug.h](#memdebug.h)
* [threadpool.h](#threadpool.h)
* [list.h](#list.h)
* [string.h](#string.h)

<br/>

# memdebug.h <a name="memdebug.h"></a>
A drop-in replacement for `malloc` that wraps `malloc()`, `realloc()`, and `free()` for the remainder of the translation unit for easy memory debugging.


## Example usage: 
```c
// #define MEMDEBUG to 1 to enable wrapping malloc(), realloc(), and free().
// Allocations will be tracked, so they can be printed out with print_heap().
// #define MEMDEBUG to 0, and print_heap() and memory management function 
// wrapping will be defined away and optimized out completely.

// #define PRINT_MEMALLOCS to 1 to print a message on each call to 
// malloc(), realloc(), and free().
// #define PRINT_MEMALLOCS to 0 if you want to still track allocations for
// print_heap(), but don't want to spam the terminal with messages.

#define MEMDEBUG 1
#define PRINT_MEMALLOCS 1
#include "apaz-libc/apaz-libc.h"

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

<br/>

# threadpool.h <a name="threadpool.h"></a>
A minimal threadpool implemented from scratch in C using `pthread`s.

## Example Usage:
```c
#include "apaz-libc/apaz-libc.h"

void say_hello(void* voidptr) {
    printf("Hello from task #%i.\n", *((int*)voidptr));
    free(voidptr);
}

int main() {
    Threadpool pool;
    Threadpool_create(&pool, 8); // 8 tasks to be run at a time

    for (int i = 0; i < 5000; i++) {
        // Executes a task in the form of a function pointer accepting one
        // void* argument and returning nothing. You can use this pointer
        // to provide args, or to return a value, or both.
        int* intptr = (int*)malloc(sizeof(int));
        *intptr = i;

        // The reason to malloc() here is to prevent race conditions.
        // This way each task has its own space for its instructions.
        // As always, if two threads try to access the same variable
        // at the same time without a mutex, bad things happen.

        if (!Threadpool_exectask(&pool, nothing, intptr)) {
            // If exectask returns 0, the pool rejected the work
            // because it is already shut down.
            // It can never happen here, but if it's a possibility
            // you should check this.

            // The easiest solution is usually to do the task 
            // yourself, like so.
            say_hello(intptr);
        }
    }

    Threadpool_destroy(&pool);
}
```

## Output
```c
...
Hello from task #7.
Hello from task #6.
Hello from task #5.
Hello from task #4.
Hello from task #3.
Hello from task #2.
Hello from task #1.
Hello from task #273.
Hello from task #335.
Hello from task #616.
Hello from task #102.
Hello from task #318.
Hello from task #201.
Hello from task #317.
```


Note that the pool uses a stack data structure for its tasks, not a queue. This choice was made after some testing. It's less "fair," but it's faster for cache locality reasons. I decided to go the faster route, but it should be noted that should this be undesireable, it can be changed with some very minor modifications to the code.

<br>

# list.h <a name="list.h"></a>

A monadic list library utilizing fat pointers to retain `[]` syntax.


## Example Usage:
```c
#include "apaz-libc.h"

static inline size_t factorial(size_t x, void *extra_data) {
  return x ? x * factorial(x - 1, extra_data) : 1;
}

static inline void print_size_t(size_t to_print, void *extra_data) {
  printf("%zu\n", to_print);
}

// This macro declares all the functionality of a list of the given type.
LIST_DEFINE(size_t);
// You can easily define lists of lists.
LIST_DEFINE(List_size_t);
// This is the fun part. (original type, map to type)
LIST_DEFINE_MONAD(size_t, size_t);

int main() {
  // The type of the list is List_##type, as provided to LIST_DEFINE. There
  // are a bunch of ways to define a list, they all have _new in their name.
  List_size_t list = List_size_t_new_len(10);

  // It's just a size_t*. You can use it as such.
  for (size_t i = 0; i < List_size_t_len(list); i++)
    list[i] = i;

  // Add new elements whenever you want, and it will grow to meet demand.
  // There are two syntaxes for this, whichever you prefer.
  List_size_t_add(&list, 11);
  list = List_size_t_addeq(list, 12);

  // You can also peek and pop.
  size_t peeked = *List_size_t_peek(list);
  List_size_t_pop(list);

  // You can clone lists too.
  // Lists are heap allocated, so destroy them when you're done with them.
  // Don't try to use normal free() through. Use List_##type##_destroy() instead.
  List_size_t cloned = List_size_t_clone(list);
  List_size_t_destroy(list);

  // Lists also support operations like map(), filter(), flatmap(), and
  // foreach().
  // These operations destroy the list passed to them, and if they return a new
  // list they re-use the space allocated for the old one where possible.
  cloned = List_size_t_map_to_size_t(cloned, factorial, NULL);
  List_size_t_foreach(cloned, print_size_t, NULL);
}
```

## Output
```
1
1
2
6
24
120
720
5040
40320
362880
39916800
```

# string.h <a name="list.h"></a>

A string library that utilizes fat pointers to retain the `[]` syntax for accessing characters, but also stores length information.


## Example Usage: