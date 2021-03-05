/*
 * pavos_semphr.h
 *
 *  Created on: Sep 12, 2019
 *      Author: pavel
 */

#ifndef PAVOS_SEMPHR_H_
#define PAVOS_SEMPHR_H_

#include "pavos_list.h"

typedef struct _semphr{

	int32_t			count;	 // semaphore count value
	int32_t			limit;	 // semaphore limit value
	struct _list	   wait_queue;	 // semaphore wait queue
	struct _tcb	      *holder;	 // current holder of the mutex lock
}semphr_t;

/*
 * Initializes counting semaphore
 * - sem:    pointer to semaphore
 * - init:   inital count value for semaphore
 * - limit:  limit for semaphore count value
 * - return: nothing
 * */
void semaphore_create_cnt(semphr_t *sem, uint32_t init, uint32_t limit);


/*
 * Initializes binary semaphore
 * - sem:    pointer to semaphore
 * - init:   inital value
 * - return: nothing
 * */
void semaphore_create_bin(semphr_t *sem, uint32_t init);


/*
 * Initalizes mutex
 * - mtx:	 pointer to a mutex structure
 * - return: nothing
 *
 * */
void mutex_create(semphr_t *mtx);


/* 
 * Kernel function to decrement semaphore. 
 * If semaphore is not available suspend requesting task.
 * Must be used and called only by the kernel.
 *
 * - sem:    pointer to a semaphore object
 * - return: nothing
 * */
int _svc_semphr_take(struct _semphr *sem);
/* 
 * Systemcall(svc) to take semaphore with blocking. 
 * If semaphore is not available, task requesting
 * the semaphore will be suspended and resumed
 * when semaphore becomes available again.
 * 
 * - sem:    pointer to a semaphore object
 * - return: PAVOS_ERR_SUCC
 * */
int semaphore_take(semphr_t *sem);


/*
 * Kernel function to increment semaphore without blocking.
 *
 * - return:
 *   PAVOS_ERR_SUCC if taking semaphore succeeded
 *   PAVOS_ERR_FAIL if semaphore was not available
 * */
int _svc_semphr_try_take(struct _semphr *sem);

/* Systemcall to take semaphore without blocking.
 * If semaphore is not available function returns
 * and task will be not suspended.
 * - return: 
 *   PAVOS_ERR_SUCC if taking semaphore succeeded
 *   PAVOS_ERR_FAIL if semaphore was not available
 * */
int semaphore_try_take(semphr_t *sem);


/*
 * Increment semaphore inside kernel.
 * - sem: pointer to a semaphore object
 * */
int _svc_semphr_give(struct _semphr *sem);
/*
 * Gives the semaphore (system call)
 * - sem:    pointer to a semaphore
 * - return: 
 *           PAVOS_ERR_SUCC if giving semaphore was successfull
 *           PAVOS_ERR_FAIL if no task was waiting for semaphore
 * */
int semaphore_give(semphr_t *sem);


/*
 * Locks mutex. If lock is not available the task
 * trying to take the lock is suspended until the lock is available
 * - mtx:   pointer to a mutex semaphore
 * - return nothing 
 * */
int mutex_lock(semphr_t *mtx);
int _svc_mutex_lock(struct _semphr *mtx);

/*
 * Nonblocking call to lock mutex.
 * If mutex is already locked function returns.
 * */
int mutex_try_lock(semphr_t *mtx);
int _svc_mutex_try_lock(struct _semphr *mtx);

int _svc_mutex_unlock(struct _semphr *mtx);
/*
 * Releases the mutex lock.
 * - mtx: pointer to a mutex to lock
 * */
int mutex_unlock(semphr_t *mtx);



#endif /* PAVOS_SEMPHR_H_ */

