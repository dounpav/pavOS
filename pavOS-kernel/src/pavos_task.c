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

/*
 * sleep task queue
 *
 * Holds tcb's of tasks that are currently sleeping
 * */
struct list sleep_task_queue = LIST_INITIAL_CONTENT;


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
	tcb->sleep_ticks = 0;

	//LIST_ITEM_INIT(tcb->self, tcb);

	tcb->self.next = NULL;
	tcb->self.prev = NULL;
	tcb->self.holder = tcb;

	// push newly created task to ready queue
	list_insert_back(&ready_task_queue, &tcb->self);

}

__attribute__((naked))void task_context_switch(uint32_t **sp_st, uint32_t **sp_ld){

	__asm__ __volatile__(

			"   cpsid i						\n"			// disable interrupts
			"								\n"
			"	push {r4-r11, lr}			\n"			// save context
			"	str sp, [r0]				\n"         // store old stack pointer
			"	ldr	sp, [r1]				\n"			// restore new stack pointer
			"	pop {r4-r11, lr}			\n"			// restore context
			"								\n"
			"   cpsie i						\n"			// enable interrupts
			"								\n"
			"	bx lr						\n"
	);
}

/*
struct tcb *schedule_new_task(void){

	struct list_item *item = list_remove_front(&ready_task_queue);
	return LIST_ITEM_HOLDER(struct tcb*, item);
}
 */

struct tcb *get_top_prio_task(void){

	struct list_item *item = list_remove_front(&ready_task_queue);
	return LIST_ITEM_HOLDER(struct tcb*, item);
}


void task_block(struct list *wait){

	struct tcb *current_task = current_running_task;
	current_task->state = TASK_BLOCKED;

	list_insert_back(wait, &(current_task->self));

	task_yield();
}

struct tcb *task_unblock(struct list *wait){

	struct list_item *item = list_remove_front(wait);
	struct tcb *task = LIST_ITEM_HOLDER(struct tcb*, item);

	task->state = TASK_READY;
	list_insert_back(&ready_task_queue, &task->self);

	return task;
}

void task_sleep(uint32_t ms){

	INTERRUPTS_DISABLE;
	{
		struct tcb *current_task = current_running_task;
		current_task->sleep_ticks = ms;
		current_task->state = TASK_BLOCKED;

		list_insert_back(&sleep_task_queue, &current_task->self);
	}
	INTERRUPTS_ENABLE;

	task_yield();
}

void task_yield(void){

	INTERRUPTS_DISABLE;

	/*
	 * if ready task queue is empty do not schedule new task
	 * */
	if(LIST_IS_EMPTY(ready_task_queue)){

		INTERRUPTS_ENABLE;
		return;
	}

	struct tcb *old_task = current_running_task;
	struct list_item *temp = list_remove_front(&ready_task_queue);
	struct tcb *new_task = LIST_ITEM_HOLDER(struct tcb*, temp);

	if(old_task->state != TASK_BLOCKED){
		list_insert_back(&ready_task_queue, &old_task->self);
	}
	new_task->state = TASK_RUNNING;
	current_running_task = new_task;

	INTERRUPTS_ENABLE;

	task_context_switch( &(old_task->stack_ptr), &(new_task->stack_ptr) );
}


void idle_task(void){

	while(1) task_yield();
}

void scheduler_start(void){

	INTERRUPTS_DISABLE;

	// create the idle task
	task_create(idle_task, &idle_tcb, idle_stack, STACK_SIZE_MIN, TASK_PRIORITY_IDLE);

	// first task to run
	current_running_task = get_top_prio_task();

	// Initalize systick
	SysTick_Config(CPU_CLOCK_RATE_HZ/1000);
	NVIC_SetPriority (SysTick_IRQn, 255);

	// start first task by switching context with kernel
	task_context_switch( &sp_kernel, &(current_running_task->stack_ptr) );
}

extern void SysTick_Handler(void){

	INTERRUPTS_DISABLE;
	{
		struct list_item *cur_item = sleep_task_queue.head;
		struct tcb *cur_tcb;

		while(cur_item != NULL){

			cur_tcb = LIST_ITEM_HOLDER(struct tcb*, cur_item);
			if(--cur_tcb->sleep_ticks == 0){

				struct list_item *item = list_remove_front(&sleep_task_queue);
				struct tcb *ready_task = LIST_ITEM_HOLDER(struct tcb*, item);
				list_insert_back(&ready_task_queue, &ready_task->self);
			}
			cur_item = cur_item->next;
		}
	}
	INTERRUPTS_ENABLE;

}




