/*
 * pavos_semphr.h
 *
 *  Created on: Sep 12, 2019
 *      Author: pavel
 */

#ifndef PAVOS_SEMPHR_H_
#define PAVOS_SEMPHR_H_

#include "pavos_list.h"

typedef struct semaphore{

	int32_t          count;	 // semaphore count value
	int32_t          limit;	 // semaphore limit value
	struct list       wait;	 // semaphore wait queue
	struct tcb  *owner;  // current owner of the mutex lock
}semaphore_t;

/*
 * Initializes counting semaphore
 * - sem:    pointer to semaphore
 * - init:   inital count value for semaphore
 * - limit:  limit for semaphore count value
 * - return: nothing
 * */
void semaphore_count_create(semaphore_t *sem, uint32_t init, uint32_t limit);


/*
 * Initializes binary semaphore
 * - sem:    pointer to semaphore
 * - init:   inital value
 * - return: nothing
 * */
void semaphore_bin_create(semaphore_t *sem, uint32_t init);


/*
 * Initalizes mutex
 * - mtx:	 pointer to a mutex structure
 * - return: nothing
 *
 * */
void mutex_create(semaphore_t *sem);


/* 
 * Take semaphore inside kernel. 
 * Must be used and called only by the kernel.
 * - sem:    pointer to a semaphore
 * - return: nothing
 * */
void ksemaphore_take(semaphore_t *sem);
/* 
 * Take semaphore. Uses supervisor call(svc)
 * - sem:    pointer to a semaphore
 * - return  nothing
 * */
void semaphore_take(semaphore_t *sem);


/*
 * Give semaphore inside kernel.
 * - sem: pointer to a semaphore
 * */
void ksemaphore_give(semaphore_t *sem);
/*
 * Gives the semaphore
 * - sem:    pointer to a semaphore
 * - return: nothing
 * */
void semaphore_give(semaphore_t *sem);


void kmutex_lock(semaphore_t *mtx);
/*
 * Takes the mutex lock. If lock is not available the task
 * trying to take the lock is suspended until the lock is available
 * - mtx:   pointer to a mutex semaphore
 * - return nothing 
 * */
void mutex_lock(semaphore_t *mtx);


void kmutex_release(semaphore_t *mtx);
/*
 * Releases the mutex lock.
 * - mtx: pointer to a mutex
 * - returns nothing
 * */
void mutex_release(semaphore_t *mtx);



#endif /* PAVOS_SEMPHR_H_ */

