/*
 * pavos_semphr.c
 *
 *  Created on: Sep 12, 2019
 *      Author: pavel
 */

#include"pavos_svcall.h"
#include"pavos_semphr.h"
#include"pavos_task.h"

#define m_svcall_semphr_take(sem)		svcall(SVC_SEM_TAKE, sem, NULL, NULL)
#define m_svcall_semphr_try_take(sem)		svcall(SVC_SEM_TTAKE, sem, NULL, NULL)
#define m_svcall_semphr_give(sem)		svcall(SVC_SEM_GIVE, sem, NULL, NULL)
#define m_svcall_mutex_lock(mtx)		svcall(SVC_MTX_LOCK, mtx, NULL, NULL)
#define m_svcall_mutex_try_lock(mtx)		svcall(SVC_MTX_TLOCK, mtx, NULL, NULL)
#define m_svcall_mutex_unlock(mtx)		svcall(SVC_MTX_UNLOCK, mtx, NULL, NULL)

void semaphore_create_cnt(semphr_t *sem, uint32_t init, uint32_t limit)
{
	m_list_init(sem->wait_queue);
	sem->count = init;
	sem->limit = limit;
	sem->holder = NULL;
}

void semaphore_create_bin(semphr_t *sem, uint32_t init)
{
	return semaphore_create_cnt(sem, init, 1);
}

void mutex_create(semphr_t *mtx)
{
	return semaphore_create_bin(mtx, 1);
}

int semaphore_take(semphr_t *sem)
{
	return m_svcall_semphr_take(sem);
}

int _svc_semphr_take(struct _semphr *sem)
{
	sem->count--;
	if(sem->count < 0)
	{
		task_block( &(sem->wait_queue) );
	}
	return PAVOS_ERR_SUCC;
}


int semaphore_try_take(semphr_t *sem)
{
	return m_svcall_semphr_try_take(sem);
}
int _svc_semphr_try_take(struct _semphr *sem)
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


int semaphore_give(semphr_t *sem)
{
	return m_svcall_semphr_give(sem);
}
int _svc_semphr_give(struct _semphr *sem)
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


int mutex_lock(semphr_t *mtx)
{
	return m_svcall_mutex_lock(mtx);
}
int _svc_mutex_lock(struct _semphr *mtx)
{
	struct _tcb *cur = get_current_running_task();

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


int mutex_try_lock(semphr_t *mtx)
{
	return m_svcall_mutex_try_lock(mtx);
}
int _svc_mutex_try_lock(struct _semphr *mtx)
{
	int ret = PAVOS_ERR_SUCC;
	struct _tcb *cur = get_current_running_task();

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

int mutex_unlock(semphr_t *mtx)
{
	return m_svcall_mutex_unlock(mtx);
}
int _svc_mutex_unlock(struct _semphr *mtx)
{
	int ret;
	struct _tcb *cur = get_current_running_task();
	struct _tcb *tsk = NULL;

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





