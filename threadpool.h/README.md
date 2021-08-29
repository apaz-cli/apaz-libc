# threadpool.h
A minimal threadpool implemented from scratch in C using pthreads.

# Example Usage:
```c
#include "threadpool.h"

#include <stdio.h>

void nothing(void* voidptr) {
    printf("Hello from task #%i.\n", *((int*)voidptr));
    free(voidptr);
}

int main() {
    Threadpool pool;

    Threadpool_create(&pool, 8);

    for (int i = 0; i < 5000; i++) {
        // Executes a task in the form of a function pointer accepting one
        // void* argument and returning nothing. You can use this pointer
        // to provide args, or to return a value, or both.
        int* intptr = (int*)malloc(sizeof(int));
        *intptr = i;

        if (!Threadpool_exectask(&pool, nothing, intptr)) {
            // If the pool returns 0 it rejected the work because the pool was already shut down.
            // It can never happen in this example, but if it's at all possible (such as if you
            // could shut down and then add tasks, or if shutting down and adding tasks are
            // happening on different threads) you should check this.

            // The easiest solution is just to do the task yourself, like so.
            nothing(intptr);
        }
    }

    Threadpool_destroy(&pool);
}
```

# Output
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
