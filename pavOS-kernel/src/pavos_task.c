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
static tcb *current_running_task = NULL;

/*
 * @ready_queues
 *
 * Ready queues. Ready queues are used to hold tasks that are ready to run.
 * Scheduler picks next task to run only from these queues.
 * Each queue is defined with its own unique priority, thus there cannot be two
 * ready queues with same priority.
 * Each queue should only contain tasks with same scheduling priority.
 * Number of queues is determined always at compile time by TASK_PRIORITY_CNT macro
 *
 * Ready queue with priority 0 is used for idle task, but nothing
 * restricts user to add any other task to that priority.
 * */
static task_queue ready_queues[TASK_PRIORITY_CNT];


/*
 * @ready_queue_prio_bmap
 *
 * Bitmap variable that tells which of the ready queues are ready to run
 * Each bit corresponds to a one queue priority that is ready to run
 * When nth bit is set, then queue with nth priority is ready to run
 * Queue is ready to run when it contains tasks that are in ready state
 *
 * Bitmap will be equal to 2^PRIORITY_CNT-1 when all queues are ready to run
 * Bitmap will be equal to zero if no queues are ready to run
 *
 * @note:
 * Bitmap will be always greater than zero, because idle task will be always scheduled to run
 * as default.
 * */
static uint8_t ready_queue_prio_bmap = 0;


/*helper functions*/
#define ready_queue_push(prio, tcb)				task_queue_push( &(ready_queues[prio]), tcb)
#define ready_queue_pop(prio)					task_queue_pop( &(ready_queues[prio]) )
#define set_ready_prio(prio)					ready_queue_prio_bmap = ready_queue_prio_bmap | (1 << prio)
#define unset_ready_prio(prio)					ready_queue_prio_bmap = ready_queue_prio_bmap ^ (1 << prio)
#define get_highest_runnable_prio				find_msb(ready_queue_prio_bmap)


static tcb idle_tcb;								// task control block for the idle task
static uint32_t idle_stack[STACK_SIZE_MIN];			// stack for idle task
static uint32_t *sp_kernel;



/*void task_stack_init(void *(entry)(void), tcb *tcb, uint32_t *stack, uint32_t stack_size){

    uint32_t *temp = &stack[ stack_size - (uint32_t)1];
    temp = (uint32_t) entry;
    temp -= 8;
    tcb->sp = temp;

    tcb->end = &stack[4];
    stack[0] = 0xfeedbeef;
    stack[1] = 0xfeedbeef;
    stack[2] = 0xfeedbeef;
    stack[3] = 0xfeedbeef;
}
*/

void task_create(void (*task_function)(void),
				tcb *tcb,
				uint32_t *stack,
				uint32_t stack_size,
				uint8_t priority)
{
	/* create stack frame as it would be created by context switch*/

	tcb->sp = &stack[ stack_size - (uint32_t)1 ];		// stack start address
	*(tcb->sp) = (uint32_t) task_function;				// LR
	(tcb->sp) -= 8;										// R11, R10, R9, R8, R7, R6, R5, R4

	/*check that given priority is valid*/
	if(priority > TASK_PRIORITY_MAX){
		priority = TASK_PRIORITY_MAX;
	}
	if(priority < TASK_PRIORITY_IDLE){
		priority = TASK_PRIORITY_IDLE;
	}

	tcb->prio = priority;
	tcb->state = TASK_READY;

	// push newly created task to ready queue
	ready_queue_push(priority, tcb);

	// set ready queue with priority as runnable
	set_ready_prio(priority);
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
		unset_ready_prio(prio);
	}

	return top_task;
}


void task_block(task_queue *wait){

	tcb *current_task = current_running_task;
	current_task->state = TASK_BLOCKED;

	task_queue_push(wait, current_task);

	task_yield();
}

tcb *task_unblock(task_queue *wait){

	tcb *task = task_queue_pop(wait);
	task->state = TASK_READY;

	ready_queue_push(task->prio, task);
	set_ready_prio(task->prio);

	return task;
}

void task_yield(void){

	// if current task is the only one running dont yield
	if(ready_queue_prio_bmap == 0) return;

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


