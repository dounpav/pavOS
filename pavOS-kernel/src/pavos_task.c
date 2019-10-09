/*
 * pavos_task.c
 *
 *  Created on: Sep 5, 2019
 *      Author: pavel
 */

#include"pavos_task.h"

tcb *current_running_task = NULL;
static uint8_t readyq_prio_bmap = 0;


static tcb idle_tcb;							// task control block for the idle task
static uint32_t idle_stack[STACK_SIZE_MIN];			// stack for idle task
static uint32_t *sp_kernel;

task_queue ready_queues[TASK_PRIORITY_CNT];
uint8_t runnable_queue_prios = 0;


void task_create(void (*task_function)(void),
				tcb *tcb,
				uint32_t *stack,
				uint32_t stack_size,
				uint8_t priority)
{
	/* create stack frame as it would be created by context switch*/

	tcb->sp = &stack[ stack_size - (uint32_t)1 ];		// stack start address
	*(tcb->sp) = (uint32_t) task_function;				// LR
	(tcb->sp) -= 8;										// R11, R10, R8, R7, R6, R5, R4

	tcb->prio = priority;
	tcb->state = TASK_READY;

	// push newly created task to ready queue
	ready_queue_push(priority, tcb);

	// set ready queue with priority as runnable
	set_runnable_prio(priority);
}

__attribute__((naked))void task_context_switch(uint32_t **sp1, uint32_t **sp2){

	__asm__ __volatile__(

			"	push {lr, r4-r11}			\n"			// save context
			"	str sp, [r0]				\n"
			"	ldr	sp, [r1]				\n"			// context switch
			"	pop {lr, r4-r11}			\n"			// restore context
			"	bx lr						\n"
	);
}

void task_queue_push(task_queue *queue, tcb *tcb){

	if(queue->size == 0){

		queue->tail = tcb;
		queue->head = tcb;
	}
	else{

		struct tcb *old_tail = queue->tail;
		old_tail->next = tcb;
		queue->tail = tcb;
		old_tail = NULL;
	}
	queue->size++;
}

tcb *task_queue_pop(task_queue *queue){

	struct tcb *old_head = queue->head;
	tcb *new_head = old_head->next;
	old_head->next = NULL;
	queue->head = new_head;

	if(new_head == NULL){
		queue->tail = NULL;
	}

	queue->size--;

	return old_head;
}

tcb *get_top_prio_task(void){

	uint8_t prio = get_highest_runnable_prio;
	tcb *top_task = ready_queue_pop(prio);

	if(ready_queues[prio].size == 0){
		unset_runnable_prio(prio);
	}

	return top_task;
}


void task_block_self(task_queue *wait){

	tcb *current_task = current_running_task;
	current_task->state = TASK_BLOCKED;

	task_queue_push(wait, current_task);
}

tcb *task_unblock_one(task_queue *wait){

	tcb *task = task_queue_pop(wait);
	task->state = TASK_READY;

	ready_queue_push(task->prio, task);
	set_runnable_prio(task->prio);

	return task;
}

void task_yield(void){

	// if current task is the only one running dont yield
	if(runnable_queue_prios == 0) return;

	/*
	 * if current task has higher priority than highest runnable priority
	 * dont yield the task
	 * */
	if(current_running_task->state == TASK_RUNNING){

		if(current_running_task->prio <= get_highest_runnable_prio){
			current_running_task->state = TASK_READY;
			ready_queue_push(current_running_task->prio, current_running_task);
		}
		else return;
	}

	// schedule new task to run
	tcb *old_task = current_running_task;
	tcb *new_task = get_top_prio_task();
	new_task->state = TASK_RUNNING;
	current_running_task = new_task;

	task_context_switch( &(old_task->sp), &(new_task->sp) );
}


void task_yield_old(void){

	// if current task is the only task return immediately
	if(runnable_queue_prios == 0) return;

	// get highest runnable priority from ready queues
	int highest_prio = get_highest_runnable_prio;

	/* if highest runnable queue priority is lower that running tasks priority
	 * and it is not blocked dont perform context switch*/
	if(current_running_task->state != TASK_BLOCKED){

		if(current_running_task->prio <= highest_prio){
			// push task to its ready queue according to its priority
			current_running_task->state = TASK_READY;
			ready_queue_push(current_running_task->prio, current_running_task);
		}
		else{
			// dont perform context switch: return immediately
			return;
		}
	}

	// choose next task to run
	tcb *old_task = current_running_task;
	// pop a task from ready queue with highest runnable priority
	current_running_task = ready_queue_pop(highest_prio);
	current_running_task->state = TASK_RUNNING;

	// if queue became empty unset the runnable priority
	if(ready_queues[highest_prio].size == 0){
		unset_runnable_prio(highest_prio);
	}

	// perform context switch
	task_context_switch( &(old_task->sp), &(current_running_task->sp) );
}

void idle_task(void){

	while(1) task_yield();
}

void scheduler_start(void){

	// create the idle task
	task_create(idle_task, &idle_tcb, idle_stack, STACK_SIZE_MIN, TASK_PRIORITY_IDLE);

	// first task to run
	current_running_task = get_top_prio_task();

	// start first task by switching context with kernel
	task_context_switch( &sp_kernel, &(current_running_task->sp) );
}


