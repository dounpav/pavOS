/*
 * pavos_semphr.h
 *
 *  Created on: Sep 12, 2019
 *      Author: pavel
 */

#ifndef PAVOS_SEMPHR_H_
#define PAVOS_SEMPHR_H_

#include "pavos_list.h"
#include <stdbool.h>

typedef struct _semphr{

	int32_t			count;	 // semaphore count value
	int32_t			limit;	 // semaphore limit value
	struct _list	   wait_queue;	 // semaphore wait queue
	struct _tcb	      *holder;	 // current holder of the mutex lock
}semphr_t;


/*
 * Initializes counting semaphore
 *	- sem:		pointer to semaphore
 *	- init:		inital count value for semaphore
 *	- limit:	limit for semaphore count value
 *	- return:	nothing
 * */
void semaphore_create_cnt(semphr_t *sem, uint32_t init, uint32_t limit);


/*
 * Initializes binary semaphore
 *	- sem:		pointer to semaphore
 *	- init:		inital value
 *	- return:	nothing
 * */
void semaphore_create_bin(semphr_t *sem, uint32_t init);


/*
 * Initalizes mutex
 *	- mtx:		pointer to a mutex structure
 *	- return:	nothing
 * */
void mutex_create(semphr_t *mtx);


/* 
 * Blocking system call to take/decrement semaphore
 *	- sem:		pointer to a semaphore structure
 *	- return:	E_SUCC
 * */
int semaphore_take(semphr_t *sem);


/* Non-blocking system call to take/decrement semaphore
 *	- sem:		pointer to a semaphore structure
 *	- return:	E_SUCC if operation succeeded
 *			E_FAIL if operation failed
 * */
int semaphore_try_take(semphr_t *sem);


/*
 * System call to increment/give semapgore
 *	- sem:		pointer to a semaphore structure
 *	- return:	E_SUCC operation succeeded
 *			E_FAIL if no task was waiting for the given
 *			semaphore
 * */
int semaphore_give(semphr_t *sem);


/*
 * Blocking system call to lock a mutex. If lock is not available the task
 * trying to take the lock is suspended until the lock is available
 *	- mtx:		pointer to a mutex semaphore
 *	- return:	E_SUCC
 * */
int mutex_lock(semphr_t *mtx);


/*
 * Nonblocking system call to lock mutex. If mutex is locked by some other task
 * the function immediately return without suspneding the calling task.
 *	- mtx:		pointer to a mutex semaphore structure
 *	- return:	E_SUCC if mutex was locked
 *			E_FAIL if mutex is locked by other
 * */
int mutex_try_lock(semphr_t *mtx);


/*
 * Release the mutex lock.
 *	- mtx:		pointer to a mutex to lock
 *	- return:	E_SUCC lock was released
 *			E_FAIL releasing the lock failed because task is not
 *			owner of the lock
 * */
int mutex_unlock(semphr_t *mtx);


int _svc_semphr_take(struct _semphr *sem, bool try);
int _svc_semphr_give(struct _semphr *sem);
int _svc_mutex_lock(struct _semphr *mtx, bool try);
int _svc_mutex_unlock(struct _semphr *mtx);




#endif /* PAVOS_SEMPHR_H_ */

