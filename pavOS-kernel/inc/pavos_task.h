/*
 * pavos_task.h
 *
 *  Created on: Sep 10, 2019
 *      Author: pavel
 */

#ifndef PAVOS_TASK_H_
#define PAVOS_TASK_H_

#include"pavos_config.h"
#include"pavos_assert.h"
#include"pavos_types.h"
#include"list.h"

typedef enum{

	TASK_RUNNING,
	TASK_READY,
	TASK_BLOCKED
}task_state;

/*Task control block structure for task*/
struct tcb{

	uint32_t        *stack_ptr;
	task_state    		 state;
    struct list_item      self;
};

//typedef struct tcb task_t;

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
void task_create(		void (*task_function)(void),
						struct tcb *tcb,
						uint32_t *stack,
						uint32_t stack_size,
						uint8_t priority);


/*
 * Performs context switch.
 * Function performs context switch using two stack pointers.
 * Upon returning from this function stack pointer register will be
 * using stack pointer of restored task
 *
 * - sp_st:	 Stack pointer of a task whose context will be stored
 * - sp_ld:	 Stack pointer of a task whose context will be loaded
 * - return: nothing
 * */
__attribute__((naked))void task_context_switch(uint32_t **sp_st, uint32_t **sp_ld);


/*
 * Finds runnable task with top (highest runnable) priority
 * - return: task with top priority
 * */
struct tcb *get_top_prio_task(void);


/*
 * Block currently running task.
 * Task blocks itself and pushes itself to specified wait queue
 *
 * - queue:  queue to which task will be added
 * - return: nothing
 * */
void task_block(struct list *queue);


/*
 * Unblocks a task
 * Function removes task from wait queue and pushes it to the ready queue
 *
 * - queue:             queue from which task will be popped
 * - return:			a task that has been unblocked
 * */
struct tcb *task_unblock(struct list *queue);


/*
 * Yields currently running task and forces context switch
 * - return: nothing
 * */
void task_yield(void);


/*
 * Idle task that runs when no other task is ready to run
 * - return: nothing
 * - note: 	 this task should be always in ready queue
 * */
void idle_task(void);


/*
 * Start scheduler and choose task to run first
 * - return: nothing
 * */
void scheduler_start(void);


#endif /* PAVOS_TASK_H_ */
