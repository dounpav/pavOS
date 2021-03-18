/*
 * pavos_semphr.c
 *
 *  Created on: Sep 12, 2019
 *      Author: pavel
 */

#include"pavos_svcall.h"
#include"pavos_semphr.h"
#include"pavos_task.h"
#include <stdbool.h>

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

int _svc_semphr_take(struct _semphr *sem, bool try)
{
	int ret = E_SUCC;

	if(sem->count == 0){

		ret = E_FAIL;
		if(!try){
			_schd_block_task( &(sem->wait_queue) );
			ret = E_SUCC;
		}
	}
	else{
		sem->count--;
	}

	return ret;
}

int semaphore_try_take(semphr_t *sem)
{
	return m_svcall_semphr_try_take(sem);
}

int semaphore_give(semphr_t *sem)
{
	return m_svcall_semphr_give(sem);
}

int _svc_semphr_give(struct _semphr *sem)
{
	int ret = E_SUCC;

	if( sem->count >= 0 && sem->count != sem->limit ){
		/* If no task was waiting for semaphore just increment
		 * the counter */
		if( m_list_is_empty(sem->wait_queue) ){
			sem->count++;
		}
		else{
			_schd_unblock_task(&sem->wait_queue);
		}
	}
	else{
		ret = E_FAIL;
	}

	return ret;
}

int mutex_lock(semphr_t *mtx)
{
	return m_svcall_mutex_lock(mtx);
}


int _svc_mutex_lock(struct _semphr *mtx, bool try)
{
	int ret = E_SUCC;
	struct _tcb *cur = _schd_current_running_task();
	struct _tcb *holder = mtx->holder;

	if(holder == cur){
		return E_SUCC;
	}

	if(mtx->count != 1){

		ret = E_FAIL;

		/* Prevent unbounded priority inversion:
		 * If holder of the mutex has lower priority
		 * than current runnning task, increase
		 * holder task's priority to the same as the
		 * currently running task*/
		if(holder->prio < cur->prio){
			holder->sv_prio = holder->prio;
			holder->prio = cur->prio;
		}

		if(!try)
		{
			_schd_block_task( &(mtx->wait_queue) );
			ret = E_SUCC;
		}
	}
	else{
		mtx->count = 0;
		mtx->holder = cur;
	}

	return ret;
}

int mutex_try_lock(semphr_t *mtx)
{
	return m_svcall_mutex_try_lock(mtx);
}

int mutex_unlock(semphr_t *mtx)
{
	return m_svcall_mutex_unlock(mtx);
}

int _svc_mutex_unlock(struct _semphr *mtx)
{
	int ret = E_SUCC;
	struct _tcb *cur = _schd_current_running_task();
	struct _tcb *tsk = NULL;

	if(mtx->holder == cur)
	{
		/* unblock any task from waiting queue*/
		if( !m_list_is_empty(mtx->wait_queue) )
		{
			/* if task's priority was changed due
			 * to priority inversion, change the task's priority to
			 * saved original prioity.
			 * */
			if(cur->prio != cur->sv_prio)
			{
				cur->prio = cur->sv_prio;
			}
			tsk = _schd_unblock_task( &(mtx->wait_queue) );
			mtx->holder = tsk;
		}
		else
		{
			mtx->count = 1;
			mtx->holder = NULL;
		}
	}
	else{
		ret = E_FAIL;
	}

	return ret;
}





