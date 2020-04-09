/*
 * pavos_semphr.c
 *
 *  Created on: Sep 12, 2019
 *      Author: pavel
 */

#include"pavos_semphr.h"
#include"pavos_task.h"

#define SVC_SEM_TAKE    3
#define SVC_SEM_GIVE    4
#define SVC_MTX_LOCK    5
#define SVC_MTX_UNLOCK  6

#define svc_sem_take()   syscall(SVC_SEM_TAKE)
#define svc_sem_give()   syscall(SVC_SEM_GIVE)
#define svc_mtx_lock()   syscall(SVC_MTX_LOCK)
#define svc_mtx_unlock() syscall(SVC_MTX_UNLOCK)

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


int semaphore_take(semaphore_t *sem){

    svc_sem_take();
}
int ksemaphore_take(semaphore_t *sem){

    sem->count--;
	if(sem->count < 0){
        task_block( &(sem->wait_queue) );
	}
    return PAVOS_ERR_SUCC;
}


int semaphore_give(semaphore_t *sem){

    svc_sem_give();
}
int ksemaphore_give(semaphore_t *sem){

    int err;
    
    sem->count++;
	if(sem->count > sem->limit){

        /*todo: kassert check if wait queue was actually empty*/

		sem->count = sem->limit;

        /* no task was waiting for semaphore*/
        err = PAVOS_ERR_FAIL;
	}
	if(sem->count <= 0){
		task_unblock( &(sem->wait_queue) );

        /* todo: kassert if function returned NULL*/

        /* semaphore was successfully incremented and waiting task unblocked */
        err = PAVOS_ERR_SUCC;
	}

    return err;
}


int mutex_lock(semaphore_t *mtx){

    svc_mtx_lock();
}
int kmutex_lock(semaphore_t *mtx){
    
    struct tcb *cur = get_current_running_task();
    
    mtx->count--;
    if(mtx->count == 0){
        mtx->holder = cur;
    }
    else{
        task_block( &(mtx->wait_queue) );
    }

    /* function should always return success code*/
    return PAVOS_ERR_SUCC;
}


int mutex_unlock(semaphore_t *mtx){

    svc_mtx_unlock();
}
int kmutex_unlock(semaphore_t *mtx){

    int err;
    struct tcb *cur = get_current_running_task();

	if(mtx->holder == cur){
        mtx->count++;
        task_unblock( &(mtx->wait_queue) );
        err = PAVOS_ERR_SUCC;
    }
    else{
        err = PAVOS_ERR_FAIL;
    }

    return err;
}





