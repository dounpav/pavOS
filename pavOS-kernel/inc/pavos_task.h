/*
 * pavos_task.h
 *
 *  Created on: Sep 10, 2019
 *      Author: pavel
 */

#ifndef PAVOS_TASK_H_
#define PAVOS_TASK_H_

#include"pavos_config.h"
#include"pavos_types.h"

typedef enum{

	TASK_RUNNING,
	TASK_READY,
	TASK_BLOCKED
}task_state;

/*Task control block structure for task*/
typedef struct tcb{

	uint32_t 	  *sp;				// stack pointer
	task_state  state;				// task state
	uint8_t		 prio;				// tasks priority
	struct tcb  *next;				// points to next task
}tcb;


/*Task queue that holds tasks*/
typedef struct{

	uint8_t  	size;				// size of the queue
	tcb 	   *head;				// head of the queue
	tcb 	   *tail;				// tail of the queue
}task_queue;


/*
 * @ready_queues
 *
 * Ready queues. Ready queues are used to hold tasks that are ready to run.
 * Scheduler picks next task to run only from these queues.
 * Each queue is defined with its own unique priority, thus there cannot be two
 * ready queues with same priority.
 * Each queue should only contain tasks with same priority.
 * Number of queues is determined always at compile time by TASK_PRIORITY_CNT macro
 *
 * Ready queue with priority 0 is used for idle task, but nothing
 * restricts user to add any other task to that priority.
 * */
extern task_queue ready_queues[TASK_PRIORITY_CNT];


/*
 * @runnable_queue_prios
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
extern uint8_t runnable_queue_prios;



/*helper functions*/
#define ready_queue_push(prio, tcb)				task_queue_push( &(ready_queues[prio]), tcb)
#define ready_queue_pop(prio)					task_queue_pop( &(ready_queues[prio]) )
#define set_runnable_prio(prio)					runnable_queue_prios = runnable_queue_prios | (1 << prio)
#define unset_runnable_prio(prio)				runnable_queue_prios = runnable_queue_prios ^ (1 << prio)
#define get_highest_runnable_prio				find_msb(runnable_queue_prios)


/*
 * @brief:  Creates a new task using static allocation
 *
 * Function initializes context and stack for the new task and adds it to
 * corresponding ready queue with same priority.
 * All parameters to this function should be provided by the user and should
 * be statically allocated
 *
 * @param tcb:          address of user allocated task's task control block (tcb)
 * @param stack:        a pointer to user allocated stack
 * @param stack_size:   a size of the user allocated stack
 * @param prior:        priority of the task
 *
 * @return:				nothing
 * */
void task_create(		void (*task_function)(void),
						tcb *tcb,
						uint32_t *stack,
						uint32_t stack_size,
						uint8_t priority);


/*
 * @brief:  Performs context switch
 *
 * Function performs context switch using two stack pointers.
 * Upon returning from this function stack pointer register will be
 * using restored stack pointer of restored task
 *
 * @param sp_st:	Stack pointer of a task whose context will be stored
 * @param sp_ld:	Stack pointer of a task whose context will be loaded
 * @return: 	    nothing
 * */
__attribute__((naked))void task_context_switch(uint32_t **sp_st, uint32_t **sp_ld);


/*
 * @brief: 	pushes task to a the back of task queue
 * @queue: task queue to which to push
 * @tcb: 	task to push
 * @return: nothing
 * */
void task_queue_push(task_queue *queue, tcb *tcb);

/*
 * @brief: pops a task from the front of the task queue
 * @queue: task queue from which to pop
 * @return: popped task from queue
 * */
tcb* task_queue_pop(task_queue *queue);


/*
 * @brief:	finds runnable task with top (highest runnable) priority
 * @return:	task with top priority
 * */
tcb *get_top_prio_task(void);


/*
 * @brief:  Block currently running task
 *
 * Task blocks itself and pushes itself to specified wait queue
 *
 * @param queue: queue to which task will be added
 * @return:	     nothing
 * */
void task_block_self(task_queue *queue);


/*
 * @brief:  unblocks a task
 *
 * Function pops task from wait queue and pushes it to corresponding
 * ready queue with same priority
 *
 * @param queue:    queue from which task will be popped
 * @return:			a task that has been unblocked
 * */
tcb *task_unblock_one(task_queue *queue);


/*
 * @brief: yields currently running task and forces context switch
 * @return: nothing
 * */
void task_yield(void);


/*
 * @brief: 	idle task that runs when no other task is ready to run
 * @return: nothing
 * @note: 	this task should be always in ready queue
 * */
void idle_task(void);


/*
 * @brief: 	starts scheduler and chooses task to run first
 * @return: nothing
 * */
void scheduler_start(void);


#endif /* PAVOS_TASK_H_ */
