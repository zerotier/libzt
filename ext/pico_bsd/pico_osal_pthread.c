/* Pthread osal implementation, for testing purposes */
#include <pthread.h>
#include "pico_defines.h"
#include "pico_stack.h"
#include "pico_osal.h"
#include <semaphore.h>
#include <time.h>

#define BILLION 1000000000

void * pico_mutex_init(void) {
    pthread_mutex_t *mutex = pico_zalloc(sizeof(pthread_mutex_t));
    if (!mutex)
        return NULL;
    if (pthread_mutex_init(mutex, NULL) == 0 )
        return mutex;
    pico_free(mutex);
    return NULL;
}
void pico_mutex_deinit(void * mutex)
{
    pthread_mutex_destroy((pthread_mutex_t *)mutex);
    pico_free(mutex);
}

void pico_mutex_lock(void * mutex)
{
    pthread_mutex_lock((pthread_mutex_t *)mutex);
}

int pico_mutex_lock_timeout(void *mutex, int timeout)
{
    if (timeout < 0) {
        return pthread_mutex_lock((pthread_mutex_t *)mutex);

    } else {
        struct timespec ts = { timeout / 1000, (timeout % 1000) * 1000000 };
        return pthread_mutex_timedlock((pthread_mutex_t *)mutex, &ts);
    }

}

void pico_mutex_unlock(void * mutex)
{
    pthread_mutex_unlock((pthread_mutex_t *)mutex);
}


void * pico_signal_init(void)
{
    sem_t *sem = pico_zalloc(sizeof(pthread_mutex_t));
    if (!sem)
        return NULL;
    if (sem_init(sem, 0, 0) == 0)
        return sem;
    pico_free(sem);
    return NULL; 
}

void pico_signal_deinit(void * signal)
{
    sem_destroy((sem_t *) signal);
}

void pico_signal_wait(void * signal)
{
    sem_wait((sem_t *) signal);
}

int pico_signal_wait_timeout(void * signal, int timeout)
{
    if (timeout < 0) {
        return sem_wait((sem_t *) signal);
    } else {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += (timeout / 1000);
        ts.tv_nsec += ((timeout % 1000) * 1000000);
        if (ts.tv_nsec >= BILLION) {
            ts.tv_nsec -= BILLION;
            ts.tv_sec++;
        }
        return sem_timedwait((sem_t *) signal, &ts);
    }
}

void pico_signal_send(void * signal)
{
    sem_post((sem_t *) signal);
}

pico_thread_t pico_thread_create(pico_thread_fn thread, void *arg, int stack_size, int prio)
{
    pico_thread_t t = PICO_ZALLOC(sizeof(pthread_t));
    if (!t)
        return NULL;
    (void)stack_size;
    (void)prio;
    pthread_create((pthread_t *)t, NULL, thread, arg);
    pthread_detach(*((pthread_t *)t));
}

void pico_thread_destroy(pico_thread_t t)
{
    pthread_cancel(*((pthread_t *)t));
    PICO_FREE(t);
}

void pico_msleep(int ms)
{
    struct timespec ts = { ms / 1000, (ms % 1000) * 1000000 };
    nanosleep(&ts, NULL);
}

void pico_threads_schedule(void)
{
    while (1 < 2)
        pico_msleep(1000);
}
