/*
 * pavos_task.c
 *
 *  Created on: Sep 5, 2019
 *      Author: pavel
 */

#include"pavos_task.h"


/*
 * @current_running_task
 *
 * Pointer that holds address of currently executing/running task.
 * Pointer should always point to a valid task control block
 * */
static struct tcb *current_running_task = NULL;

/*
 * @ready_task_queue
 *
 * Holds tasks that are ready to be scheduled.
 * Scheduler pick next task to run only from this queue
 * */
static struct list ready_task_queue = LIST_INITIAL_CONTENT;


static struct tcb idle_tcb;								// task control block for the idle task
static uint32_t idle_stack[STACK_SIZE_MIN];				// stack for idle task
static uint32_t *sp_kernel;

void task_create(void (*task_function)(void),
		struct tcb *tcb,
		uint32_t *stack,
		uint32_t stack_size,
		uint8_t priority)
{
	/* create stack frame as it would be created by context switch*/

	tcb->stack_ptr = &stack[ stack_size - (uint32_t)1 ];		// stack start address
	*(tcb->stack_ptr) = (uint32_t) task_function;				// LR
	(tcb->stack_ptr) -= 8;										// R11, R10, R8, R7, R6, R5, R4
	tcb->state = TASK_READY;

	// LIST_ITEM_INIT(tcb->self, tcb);

	tcb->self.next = NULL;
	tcb->self.prev = NULL;
	tcb->self.data = tcb;

	// push newly created task to ready queue
	struct list_item* temp = list_insert_back(&ready_task_queue, &tcb->self);
	PAVOS_ASSERT(temp != NULL);

}

__attribute__((naked))void task_context_switch(uint32_t **sp_st, uint32_t **sp_ld){

	__asm__ __volatile__(

			"	push {r4-r11, lr}			\n"			// save context
			"	str sp, [r0]				\n"         // store old stack pointer
			"	ldr	sp, [r1]				\n"			// context switch
			"	pop {r4-r11, lr}			\n"			// restore context
			"	bx lr						\n"
	);
}


struct tcb *get_top_prio_task(void){

	struct list_item *temp = list_remove_front(&ready_task_queue);

	return (struct tcb*)temp->data;
}


void task_block(struct list *wait){

	struct tcb *current_task = current_running_task;
	current_task->state = TASK_BLOCKED;

	list_insert_back(wait, &(current_task->self));

	task_yield();
}

struct tcb *task_unblock(struct list *wait){

	struct list_item *item = list_remove_front(wait);
	struct tcb *task = (struct tcb*)item->data;

	task->state = TASK_READY;
	list_insert_back(&ready_task_queue, &task->self);

	return task;
}

void task_yield(void){

	// schedule new task to run
	struct tcb *old_task = current_running_task;
	struct list_item *temp = list_remove_front(&ready_task_queue);
	struct tcb *new_task = (struct tcb*)temp->data;

	if(old_task->state != TASK_BLOCKED){
		list_insert_back(&ready_task_queue, &old_task->self);
	}
	new_task->state = TASK_RUNNING;
	current_running_task = new_task;

	task_context_switch( &(old_task->stack_ptr), &(new_task->stack_ptr) );
}


void idle_task(void){

	while(1) task_yield();
}

void scheduler_start(void){

	// create the idle task
	//task_create(idle_task, &idle_tcb, idle_stack, STACK_SIZE_MIN, TASK_PRIORITY_IDLE);

	// first task to run
	current_running_task = get_top_prio_task();

	// start first task by switching context with kernel
	task_context_switch( &sp_kernel, &(current_running_task->stack_ptr) );
}


