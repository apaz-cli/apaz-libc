#include "threadpool.h"

#include <stdio.h>

void nothing(void* voidptr) {
    printf("Hello from task #%i.\n", *((int*)voidptr));
    free(voidptr);
}

int main() {
    Threadpool pool;

    POOL_create(&pool, 8);

    for (int i = 0; i < 5000; i++) {
        // Executes a task in the form of a function pointer accepting one
        // void* argument and returning nothing. You can use this pointer
        // to provide args, or to return a value, or both.
        int* intptr = (int*)malloc(sizeof(int));
        *intptr = i;

        if (!POOL_exectask(&pool, nothing, intptr)) {
            // If the pool returns 0 it rejected the work because the pool was already shut down.
            // It can never happen in this example, but if it's at all possible (such as if you
            // could shut down and then add tasks, or if shutting down and adding tasks are
            // happening on different threads) you should check this.

            // The easiest solution is just to do the task yourself, like so.
            nothing(intptr);
        }
    }

    POOL_destroy(&pool);
}