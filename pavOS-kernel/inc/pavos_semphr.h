/*
 * pavos_semphr.h
 *
 *  Created on: Sep 12, 2019
 *      Author: pavel
 */

#ifndef PAVOS_SEMPHR_H_
#define PAVOS_SEMPHR_H_

#include"pavos_task.h"


typedef struct{

	int32_t     count;	// semaphore count value
	int32_t     limit;	// semaphore limit value
	struct list  wait;	// semaphore wait queue
}semaphore;


/*
 * Initializes counting semaphore
 * - sem:    pointer to semaphore
 * - init:   inital count value for semaphore
 * - limit:  limit for semaphore count value
 * - return: nothing
 * */
void semaphore_count_create(semaphore *sem, uint32_t init, uint32_t limit);


/*
 * Initializes binary semaphore
 * - sem:    pointer to semaphore
 * - init:   inital value
 * - return: nothing
 * */
void semaphore_bin_create(semaphore *sem, uint32_t init);


/* Take counting semaphore
 * - sem:    pointer to a semaphore
 * - return  nothing
 * */
void semaphore_take(semaphore *sem);


/*
 * Gives the semaphore
 * - sem:    pointer to a semaphore
 * - return: nothing
 * */
void semaphore_give(semaphore *sem);



#endif /* PAVOS_SEMPHR_H_ */

