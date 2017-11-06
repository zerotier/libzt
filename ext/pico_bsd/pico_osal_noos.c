/*
 * pico_osal.h
 *
 *  Created on: December 2014
 *      Author: Maxime Vincent
 * Description: OS Abstraction Layer between PicoTCP and No Operating System
 *
 */

/* PicoTCP includes */
#include "pico_defines.h"
#include "pico_config.h"
#include "pico_stack.h"
#include "pico_osal.h"

#define osal_dbg(...)
//#define osal_dbg(...) printf(__VA_ARGS__)

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/* ============= */
/* == MUTEXES == */
/* ============= */

struct osal_mutex {
    volatile int want_to_take; /* for ISR safety, basically a mutex for the mutex */
    volatile int mutex;
    int idx; /* only to keep track of the amount/idx, no real function .. */
};

static uint8_t mtx_number = 0;

void * pico_mutex_init(void)
{
    struct osal_mutex * mutex;
    mutex = pico_zalloc(sizeof(struct osal_mutex));
    osal_dbg("mi: %p for %p\n", mutex, __builtin_return_address(0));
    if (!mutex)
        return NULL;
    mutex->mutex = 1;
    mutex->idx = mtx_number++;
    return mutex;
}

void pico_mutex_deinit(void * mutex)
{
    struct osal_mutex * mtx = mutex;
    pico_free(mutex);
}

int pico_mutex_lock_timeout(void * mutex, int timeout)
{
    int retval = 0;
	if(mutex != NULL)
	{
        struct osal_mutex * mtx = mutex;
        pico_time timestamp = PICO_TIME_MS();
        while (mtx->mutex == 0)
        {
            pico_stack_tick();
            #ifdef _POSIX_VERSION
              usleep(500);
            #endif

            /* break on timeout unless infinite timeout */
            if ((timeout != -1) && (PICO_TIME_MS() > (timestamp + timeout)))
                break;
        }
        if (mtx->mutex == 1)
        {
            mtx->mutex = 0; /* take the mutex */
        }
        else
        {
            retval = -1; /* timeout */
        }
	}
    return retval;
}

void pico_mutex_lock(void * mutex)
{
    pico_mutex_lock_timeout(mutex, -1);
}

void pico_mutex_unlock(void * mutex)
{
	if(mutex != NULL)
    {
        struct osal_mutex * mtx = mutex;
        mtx->mutex = 1;
    }
}

void pico_mutex_unlock_ISR(void * mutex)
{
	if(mutex != NULL)
    {
        struct osal_mutex * mtx = mutex;
        // tricky stuff needed or not?
        mtx->mutex = 1;
    }
}

/* ============= */
/* == SIGNALS == */
/* ============= */

void * pico_signal_init(void)
{
    void * signal = pico_mutex_init();
    pico_mutex_lock(signal);
    return signal;
}

void pico_signal_deinit(void * signal)
{
    pico_mutex_deinit(signal);
}

void pico_signal_wait(void * signal)
{
    pico_signal_wait_timeout(signal, -1);
}

int pico_signal_wait_timeout(void * signal, int timeout)
{
    return pico_mutex_lock_timeout(signal, timeout);
}

void pico_signal_send(void * signal)
{
    pico_mutex_unlock(signal);
}

void pico_signal_send_ISR(void * signal)
{
    pico_mutex_unlock_ISR(signal);
}


/* ============= */
/* == THREADS == */
/* ============= */

pico_thread_t pico_thread_create(pico_thread_fn thread, void *arg, int stack_size, int prio)
{
    (void)thread;
    (void)arg;
    (void)stack_size;
    (void)prio;
    return NULL;
}

void pico_thread_destroy(pico_thread_t t)
{
    return;
}

void pico_msleep(int ms)
{
    pico_time now = PICO_TIME_MS();
    while ((pico_time)(now + ms) < PICO_TIME_MS());
}
