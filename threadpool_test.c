#include <apaz-libc/threadpool.h>
#include <stdio.h>

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

        if (!Threadpool_exectask(&pool, say_hello, intptr)) {
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