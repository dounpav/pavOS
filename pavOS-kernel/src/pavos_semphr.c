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

void semaphore_block_one(semaphore *sem, tcb *task){

	current_running_task->state = TASK_BLOCKED;
	semaphore_wait_push(sem, current_running_task);
}

tcb *semaphore_wake_one(semaphore *sem){

	/*
	 * pop a task from wait queue and set it as ready
	 * */
	tcb *task = semaphore_wait_pop(sem);
	task->state = TASK_READY;

	/*
	 * update ready queue of task's priority if needed
	 * */
	if(ready_queues[task->prio].size == 0){
		set_runnable_prio(task->prio);
	}

	// push task to ready queue
	ready_queue_push(task->prio, task);

	return task;
}


void semaphore_take(semaphore *sem){

	sem->count--;
	if(sem->count < 0){
		semaphore_block_one(sem, current_running_task);
		task_yield();
	}
}

void semaphore_give(semaphore *sem){

	sem->count++;
	if(sem->count > sem->limit){
		sem->count = sem->limit;
	}
	if(sem->count <= 0){
		semaphore_wake_one(sem);
	}
}

