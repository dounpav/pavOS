/*
 * pavos_semphr.c
 *
 *  Created on: Sep 12, 2019
 *      Author: pavel
 */

#include"pavos_semphr.h"
#include"pavos_task.h"

void semaphore_count_create(semaphore_t *sem, uint32_t init, uint32_t limit){

	LIST_INIT(sem->wait);
	sem->count = init;
	sem->limit = limit;
	sem->owner = NULL;
}

void semaphore_bin_create(semaphore_t *sem, uint32_t init){

	return semaphore_count_create(sem, init, 1);
}


void mutex_create(semaphore_t *mtx){
	
    return semaphore_bin_create(mtx, 1);
}


void semaphore_take(semaphore_t *sem){

	INTERRUPTS_DISABLE;
	{
		sem->count--;
		if(sem->count < 0){

			task_block( &(sem->wait) );
		}
	}
	INTERRUPTS_ENABLE;
}

void semaphore_give(semaphore_t *sem){

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

void mutex_lock(semaphore_t *mtx){

	INTERRUPTS_DISABLE;
	{
		struct tcb *current = get_current_running_task();

		if(mtx->count == mtx->limit){
			mtx->count--;
			mtx->owner = current;
		}
		else{
			task_block( &(mtx->wait) );
		}
	}
	INTERRUPTS_ENABLE;
}


void mutex_release(semaphore_t *mtx){

	INTERRUPTS_DISABLE;
	{
		struct tcb *current = get_current_running_task();

		if(mtx->owner == current){
			mtx->count++;
			task_unblock( &(mtx->wait) );
		}
	}
	INTERRUPTS_ENABLE;
}





