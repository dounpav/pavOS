/*
 * pavos_task.h
 *
 *  Created on: Sep 10, 2019
 *      Author: pavel
 */

#ifndef PAVOS_TASK_H_
#define PAVOS_TASK_H_

#include "pavos_list.h"
#include "pavos_config.h"
#include "pavos_types.h"


typedef enum{

	TASK_RUNNING,
	TASK_READY,
	TASK_BLOCKED
}task_state;

/*Task control block structure for task*/
typedef struct tcb{

	uint32_t	     *stack_ptr;
	task_state		  state;
	uint32_t	timeslice_ticks;
	uint32_t	    sleep_ticks;
	struct _item		   self;
}task_t;


/*
 * Creates a new task using static allocation.
 * Function initializes context and stack for the new task and adds it to a ready queue.
 * All parameters to this function should be provided by the user and should
 * be allocated at compile time
 *
 * - tcb:          address of user allocated task's task control block (tcb)
 * - stack:        a pointer to user allocated stack
 * - stack_size:   a size of the user allocated stack
 * - return:	   nothing
 * */
void task_create( void (*task_function)(void), struct tcb *tcb,
		uint32_t *stack,
		uint32_t stack_size,
		uint8_t priority);



/*
 * Finds runnable task with top (highest runnable) priority
 * - return: task with top priority
 * */
struct tcb *get_top_prio_task(void);


/*
 * Starts the first task
 * - current: task to start first
 * */
__attribute__((naked)) void scheduler_start_task(struct tcb **current);


/*
 * Pends a context switch
 * */
int pend_context_switch(void);


/*
 * Returns currently running task
 * */
struct tcb *get_current_running_task(void);

/*
 * Block currently running task.
 * Task blocks itself and pushes itself to specified wait queue
 *
 * - queue:  queue to which task will be added
 * - return: nothing
 * */
void task_block(struct _list *queue);


/*
 * Unblocks a task
 * Function removes task from wait queue and pushes it to the ready queue
 *
 * - queue:             queue from which task will be popped
 * - return:			a task that has been unblocked
 * */
struct tcb *task_unblock(struct _list *queue);


int _svc_task_sleep(uint32_t ms);
/*
 * Suspends task for ms amount of milliseconds
 * */
int task_sleep(uint32_t ms);


/*
 * Yields currently running task and forces context switch
 * - return: always returns 1 (success)
 * */
int task_yield(void);


/*
 * Starts scheduler and chooses task to run first
 * - return: nothing
 * */
void scheduler_start(void);


#endif /* PAVOS_TASK_H_ */
