/*
 * pavos_semphr.c
 *
 *  Created on: Sep 12, 2019
 *      Author: pavel
 */

#include"pavos_semphr.h"
#include"pavos_task.h"

void semaphore_count_create(semaphore_t *sem, uint32_t init, uint32_t limit){

	LIST_INIT(sem->wait_queue);
	sem->count = init;
	sem->limit = limit;
	sem->holder = NULL;
}

void semaphore_bin_create(semaphore_t *sem, uint32_t init){

	return semaphore_count_create(sem, init, 1);
}

void mutex_create(semaphore_t *mtx){
	
    return semaphore_bin_create(mtx, 1);
}


void semaphore_take(semaphore_t *sem){

    __asm__ __volatile__("svc #0x3\n");
}
void ksemaphore_take(semaphore_t *sem){
    
    sem->count--;
	if(sem->count < 0){
        task_block( &(sem->wait_queue) );
	}
}


void semaphore_give(semaphore_t *sem){
    
    __asm__ __volatile__("svc #0x4\n");
}
void ksemaphore_give(semaphore_t *sem){
    
    sem->count++;
	if(sem->count > sem->limit){
		sem->count = sem->limit;
	}
	if(sem->count <= 0){
		task_unblock( &(sem->wait_queue) );
	}
}


void mutex_lock(semaphore_t *mtx){

    __asm__ __volatile__( "svc #0x5\n" );
}
void kmutex_lock(semaphore_t *mtx){
    
    struct tcb *cur = get_current_running_task();
    
    mtx->count--;
    if(mtx->count == 0){
        mtx->holder = cur;
    }
    else{
        task_block( &(mtx->wait_queue) );
    }
}


void mutex_unlock(semaphore_t *mtx){

    __asm__ __volatile__( "svc #0x6\n");

}
void kmutex_unlock(semaphore_t *mtx){

	INTERRUPTS_DISABLE;
	{
		struct tcb *cur = get_current_running_task();

		if(mtx->holder == cur){
			mtx->count++;
			task_unblock( &(mtx->wait_queue) );
		}
	}
	INTERRUPTS_ENABLE;
}





