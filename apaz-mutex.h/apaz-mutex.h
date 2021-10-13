#ifndef MUTEX_INCLUDE
#define MUTEX_INCLUDE

// All mutex functions return 0 on success

#ifdef _WIN32
// Use windows.h if compiling for Windows
#include <Windows.h>

#define mutex_t SRWLOCK
#define MUTEX_INITIALIZER SRWLOCK_INIT
static inline int mutex_init(mutex_t *mutex) {
  InitializeSRWLock(mutex);
  return 0;
}
static inline int mutex_lock(mutex_t *mutex) {
  AcquireSRWLockExclusive(mutex);
  return 0;
}
static inline int mutex_unlock(mutex_t *mutex) {
  ReleaseSRWLockExclusive(mutex);
  return 0;
}
static inline int mutex_destroy(mutex_t *mutex) { return 0; }

#else
// On other platforms use <pthread.h>
#include <pthread.h>

#define mutex_t pthread_mutex_t
#define MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
static inline int mutex_init(mutex_t *mutex) { return pthread_mutex_init(mutex, NULL); }
static inline int mutex_lock(mutex_t *mutex) { return pthread_mutex_lock(mutex); }
static inline int mutex_unlock(mutex_t *mutex) { return pthread_mutex_unlock(mutex); }
static inline int mutex_destroy(mutex_t *mutex) { return pthread_mutex_destroy(mutex); }
#endif

#endif // MUTEX_INCLUDE