/*
 * pavos_semphr.c
 *
 *  Created on: Sep 12, 2019
 *      Author: pavel
 */

#include"pavos_semphr.h"


void semaphore_count_create(semaphore *sem, uint32_t init, uint32_t limit){

	sem->count = init;
	sem->limit = limit;
}

void semaphore_bin_create(semaphore *sem, uint32_t init){

	sem->count = init;
	sem->limit = 1;
}

void semaphore_take(semaphore *sem){

	sem->count--;
	if(sem->count < 0){

        task_block( &(sem->wait) );
	}
}

void semaphore_give(semaphore *sem){

	sem->count++;
	if(sem->count > sem->limit){
		sem->count = sem->limit;
	}
	if(sem->count <= 0){

        // semaphore wakes task from waiting queue
        task_unblock( &(sem->wait) );
	}
}

