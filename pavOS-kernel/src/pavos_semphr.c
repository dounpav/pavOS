/*
 * pavos_semphr.c
 *
 *  Created on: Sep 12, 2019
 *      Author: pavel
 */

#include"pavos_svcall.h"
#include"pavos_semphr.h"
#include"pavos_task.h"


void semaphore_count_create(semaphore_t *sem, uint32_t init, uint32_t limit)
{
	LIST_INIT(sem->wait_queue);
	sem->count = init;
	sem->limit = limit;
	sem->holder = NULL;
}

void semaphore_bin_create(semaphore_t *sem, uint32_t init)
{
	return semaphore_count_create(sem, init, 1);
}

void mutex_create(semaphore_t *mtx)
{
	return semaphore_bin_create(mtx, 1);
}


int semaphore_take(semaphore_t *sem)
{
	return svcall(SVC_SEM_TAKE, sem);
}
int ksemaphore_take(semaphore_t *sem)
{
	sem->count--;
	if(sem->count < 0)
	{
		task_block( &(sem->wait_queue) );
	}
	return PAVOS_ERR_SUCC;
}


int semaphore_try_take(semaphore_t *sem)
{
	return svcall(SVC_SEM_TTAKE, sem);
}
int ksemaphore_try_take(semaphore_t *sem)
{
	int ret = PAVOS_ERR_SUCC;

	if(sem->count > 0)
	{
		sem->count--;
	}
	else
	{
		ret = PAVOS_ERR_FAIL;
	}

	return ret;
}


int semaphore_give(semaphore_t *sem)
{
	return svcall(SVC_SEM_GIVE, sem);
}
int ksemaphore_give(semaphore_t *sem)
{
	int ret;

	sem->count++;
	if(sem->count > sem->limit)
	{
		/*todo: kassert check if wait queue was actually empty*/

		sem->count = sem->limit;

		/* no task was waiting for semaphore*/
		ret = PAVOS_ERR_FAIL;
	}
	if(sem->count <= 0)
	{
		task_unblock( &(sem->wait_queue) );

		/* todo: kassert if function returned NULL*/

		/* semaphore was successfully incremented and waiting task unblocked */
		ret = PAVOS_ERR_SUCC;
	}

	return ret;
}


int mutex_lock(semaphore_t *mtx)
{
	return svcall(SVC_MTX_LOCK, mtx);
}
int kmutex_lock(semaphore_t *mtx)
{
	struct tcb *cur = get_current_running_task();

	mtx->count--;
	if(mtx->count == 0)
	{
		mtx->holder = cur;
	}
	else
	{
		task_block( &(mtx->wait_queue) );
	}

	/* function should always return success code*/
	return PAVOS_ERR_SUCC;
}


int mutex_try_lock(semaphore_t *mtx)
{
	return svcall(SVC_MTX_TLOCK, mtx);
}
int kmutex_try_lock(semaphore_t *mtx)
{
	int ret = PAVOS_ERR_SUCC;
	struct tcb *cur = get_current_running_task();

    /*
     * does not support recursive mutexes yet
     
    if(mtx->holder == cur) 
        return PAVOS_ERR_SUCC;
    */

	if(mtx->count == 1)
	{
		mtx->holder = cur;
		mtx->count--;
	}
	else
	{
		ret = PAVOS_ERR_FAIL;
	}

    return ret;
}

/*
static int kmutex_try_locknew(semaphore_t *mtx){

    struct tcb *cur = get_current_running_task();
    int ret = PAVOS_ERR_FAIL;

    if(mtx->holder == NULL){
        mtx->holder = cur;
        ret = PAVOS_ERR_SUCC;
    }
    else{
        if(mtx->holder == cur){
            ret = PAVOS_ERR_SUCC;
        }
    }

    return ret;
}

		if(!LIST_IS_EMPTY(mtx->wait_queue)){
			tsk = task_unblock( &(mtx->wait_queue) );
		}

*/

int mutex_unlock(semaphore_t *mtx)
{
	return svcall(SVC_MTX_UNLOCK, mtx);
}
int kmutex_unlock(semaphore_t *mtx)
{
	int ret;
	struct tcb *cur = get_current_running_task();
	struct tcb *tsk = NULL;

	if(mtx->holder == cur)
	{
		mtx->count++;
		/* unblock any task that from waiting queue*/
		if(mtx->count <= 0)
		{
			tsk = task_unblock( &(mtx->wait_queue) );
			mtx->holder = tsk;
		}
		else
		{
			mtx->holder = NULL;
		}
		ret = PAVOS_ERR_SUCC;
	}
	else{
		ret = PAVOS_ERR_FAIL;
	}

	return ret;
}





