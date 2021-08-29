/***********/
/* Mutexes */
/***********/
#ifndef __INCLUDED_MUTEX
#define __INCLUDED_MUTEX

// All mutex functions return 0 on success

#ifdef _WIN32
// Use windows.h if compiling for Windows
#include <Windows.h>

#define mutex_t SRWLOCK
#define MUTEX_INITIALIZER SRWLOCK_INIT
int mutex_init(mutex_t* mutex) {
    InitializeSRWLock(mutex);
    return 0;
}
int mutex_lock(mutex_t* mutex) {
    AcquireSRWLockExclusive(mutex);
    return 0;
}
int mutex_unlock(mutex_t* mutex) {
    ReleaseSRWLockExclusive(mutex);
    return 0;
}
int mutex_destroy(mutex_t* mutex) { return 0; }

#else
// On other platforms use <pthread.h>
#include <pthread.h>

#define mutex_t pthread_mutex_t
#define MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
int mutex_init(mutex_t* mutex) { return pthread_mutex_init(mutex, NULL); }
int mutex_lock(mutex_t* mutex) { return pthread_mutex_lock(mutex); }
int mutex_unlock(mutex_t* mutex) { return pthread_mutex_unlock(mutex); }
int mutex_destroy(mutex_t* mutex) { return pthread_mutex_destroy(mutex); }
#endif

#endif  // End mutex include guard

#ifndef THREADPOOL
#define THREADPOOL

#include <stdbool.h>
#include <stdlib.h>

#define Threadpool_CRITICAL_BEGIN mutex_lock(&(pool->pool_mutex));
#define Threadpool_CRITICAL_END mutex_unlock(&(pool->pool_mutex));

/**********************/
/* Struct definitions */
/**********************/

struct TaskStack;
typedef struct TaskStack TaskStack;
struct TaskStack {
    void (*task_fn)(void* args);
    void* task_args;
    TaskStack* next;
};

struct Threadpool {
    mutex_t pool_mutex;
    TaskStack* task_stack;
    size_t num_threads;
    size_t num_threads_running;
    pthread_t* threads;
    bool is_shutdown;
};
typedef struct Threadpool Threadpool;

/**********************/
/* Threadpool methods */
/**********************/

static void* await_and_do_tasks(void* pool_arg);

// Create the pool before you add tasks to it. Then destroy it once you're done.
extern void
Threadpool_create(Threadpool* pool, size_t num_threads) {
    // Initialize the pool
    static mutex_t pmut_init = MUTEX_INITIALIZER;
    pool->pool_mutex = pmut_init;
    pool->task_stack = NULL;
    pool->is_shutdown = false;

    pool->num_threads = num_threads;
    pool->num_threads_running = num_threads;
    pool->threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));

    // Spin threads. They wrap on the spinlock doing work until the pool is
    // shut down. They detach themselves and do not need to be joined.
    for (size_t i = 0; i < num_threads; i++) {
        pthread_create(&(pool->threads[i]), NULL, await_and_do_tasks, pool);
    }
}

// Do not exec tasks in the pool before it is created or after it is destroyed.
// Do not exec a task that will not finish.
// Returns true on success, false on failure. Fails when pool is already shut down.
extern bool
Threadpool_exectask(Threadpool* pool, void (*task_fn)(void* args), void* task_args) {
    // Put the thread in the pool so that it can be consumed.
    Threadpool_CRITICAL_BEGIN;
    {
        // Fails if already shut down
        if (pool->is_shutdown) {
            return 0;
        }

        // Push work onto the TaskStack
        TaskStack* work = (TaskStack*)malloc(sizeof(TaskStack));
        work->task_fn = task_fn;
        work->task_args = task_args;

        // Note that the TaskStack is initialized NULL.
        // Doing it this way makes sure the pool's TaskStack stays null terminated.
        work->next = pool->task_stack;
        pool->task_stack = work;
    }
    Threadpool_CRITICAL_END;

    return 1;
}

static void* await_and_do_tasks(void* pool_arg) {
    Threadpool* pool = (Threadpool*)pool_arg;

    // Detach self
    pthread_detach(pthread_self());

    // Spinlock waiting for work
    while (true) {
        // Try to obtain work
        TaskStack* work = NULL;

        /* Critical section */
        Threadpool_CRITICAL_BEGIN;
        {
            // Join self when the pool shuts down,
            // making sure to end the critical section.
            if (pool->is_shutdown && pool->task_stack == NULL) {
                pool->num_threads_running--;
                Threadpool_CRITICAL_END;
                return NULL;
            }

            // Check for work. If we find some, pop it from the
            // stack and let the threadpool know that we're working on something.

            work = pool->task_stack;
            if (work) {
                pool->task_stack = work->next;
            }
        }
        Threadpool_CRITICAL_END;

        if (work) {
            // Extract the work and args
            void (*task_fn)(void*) = work->task_fn;
            void* task_args = work->task_args;
            free(work);

            task_fn(task_args);
        } else {
            sched_yield();
            continue;
        }
    }
}

// Only destroy once, and not before the threadpool is created.
extern void
Threadpool_destroy(Threadpool* pool) {
    Threadpool_CRITICAL_BEGIN;
    pool->is_shutdown = true;
    Threadpool_CRITICAL_END;

    // Wait for the task stack to be consumed.
    bool waiting = true;
    while (waiting) {
        sched_yield();

        Threadpool_CRITICAL_BEGIN;
        waiting = pool->task_stack;
        Threadpool_CRITICAL_END;
    }

    // Wait for the tasks to finish running
    waiting = true;
    while (waiting) {
        Threadpool_CRITICAL_BEGIN;
        waiting = pool->num_threads_running;
        Threadpool_CRITICAL_END;

        sched_yield();
    }

    // Now the pool is completely finished and all threads are joined.
    // Tear down the last remaining resources we're using.
    mutex_destroy(&(pool->pool_mutex));
    free(pool->threads);
}

#endif