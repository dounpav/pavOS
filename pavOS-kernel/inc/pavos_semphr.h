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
	task_queue  wait;	// semaphore wait queue
}semaphore;

// deprected functions do not use
#define semaphore_wait_push(sem, tcb)				task_queue_push( &(sem->wait), tcb )
#define semaphore_wait_pop(sem)						task_queue_pop( &(sem->wait) )


/*
 * @brief:      initializes counting semaphore
 * @param sem:  pointer to semaphore
 * @param init: inital count value for semaphore
 * @param limit:limit for semaphore count value
 * @return:     nothing
 * */
void semaphore_count_create(semaphore *sem, uint32_t init, uint32_t limit);


/*
 * @biref:      initializes binary semaphore
 * @param sem:  pointer to semaphore
 * @param init: inital value
 * @return      nothing
 * */
void semaphore_bin_create(semaphore *sem, uint32_t init);


/* @brief:  tries to take counting semaphore
 * @sem:    pointer to a semaphore
 * @return  nothing
 * */
void semaphore_take(semaphore *sem);


/*
 * @breif:  gives the semaphore
 * @sem:    pointer to a semaphore
 * @return: nothing
 * */
void semaphore_give(semaphore *sem);



#endif /* PAVOS_SEMPHR_H_ */

