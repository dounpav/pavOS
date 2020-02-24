/*
 * pavos_semphr.c
 *
 *  Created on: Sep 12, 2019
 *      Author: pavel
 */

#include"pavos_semphr.h"
#include"pavos_task.h"

void semaphore_count_create(semaphore *sem, uint32_t init, uint32_t limit){

	LIST_INIT(sem->wait);
	sem->count = init;
	sem->limit = limit;
}

void semaphore_bin_create(semaphore *sem, uint32_t init){

	return semaphore_count_create(sem, init, 1);
}

void semaphore_take(semaphore *sem){

	INTERRUPTS_DISABLE;
	{
		sem->count--;
		if(sem->count < 0){

			task_block( &(sem->wait) );
		}
	}
	INTERRUPTS_ENABLE;
}

void semaphore_give(semaphore *sem){

	INTERRUPTS_DISABLE;
	{
		sem->count++;
		if(sem->count > sem->limit){
			sem->count = sem->limit;
		}
		if(sem->count <= 0){

			// semaphore wakes task from waiting queue
			task_unblock( &(sem->wait) );
		}
	}
	INTERRUPTS_ENABLE;
}

