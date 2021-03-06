/*
 * pavos_task.c
 *
 *  Created on: Sep 5, 2019
 *      Author: pavel
 */

#include"pavos_svcall.h"
#include"pavos_task.h"

/* NVIC Interrupt Control State Register */
#define NVIC_INT_CTRL_ST_REG			(*((volatile uint32_t*) 0xE000ED04))
/* Bit position to pend PendSV exception */
#define NVIC_PENDSVSET_BIT			(1 << 28)
/* Reset value for xPSR register */
#define xPSR_RESET_VAL				(0x01000000)
/* Link register reset value*/
#define LR_RESET_VAL				(0xffffffff)
/* NVIC lowest interrupt priority*/
#define NVIC_PRIO_LOWEST			(255)

#define SCHED_RR_TIMESLICE			(5)

/* svcall helper functions */
#define m_svcall_task_yield()			svcall(SVC_TASK_YIELD, NULL, NULL, NULL)
#define m_svcall_task_sleep(ms)			svcall(SVC_TASK_SLEEP, ms, NULL, NULL)

/*
 * current_running_task
 * Pointer that holds address of currently executing/running task.
 * Pointer should always point to a valid task control block
 */
static struct _tcb *current_running_task = NULL;


/*
 * ready_task_queue
 * Holds tasks that are ready to be scheduled.
 * Scheduler pick next task to run only from this queue
 */
static struct _list ready_task_queue = m_list_initial_content;


/*
 * sleep task queue
 * Holds tcb's of tasks that are currently sleeping
 */
static struct _list sleep_task_queue = m_list_initial_content;


static struct _tcb idle_tcb;			// task control block for the idle task
static uint32_t idle_stack[STACK_SIZE_MIN];	// stack for idle task


void task_create(void (*task_function)(void), task_t *task,
		uint32_t *stack,
		uint32_t stack_size,
		uint8_t priority)
{
	struct _tcb *tcb = (struct _tcb *)task;
	/* create stack frame as it would be created by context switch */

	/* locate stack start address */
	tcb->stack_ptr = &stack[ stack_size - (uint32_t)1 ];
	/* Leave a space to avoid possible memory corruption when returning
	 * from exception */
	tcb->stack_ptr--;
	*(tcb->stack_ptr) = xPSR_RESET_VAL;		// xPSR
	tcb->stack_ptr--;
	*(tcb->stack_ptr) = (uint32_t) task_function;	// PC
	tcb->stack_ptr--;
	*(tcb->stack_ptr) = LR_RESET_VAL;               // LR
	(tcb->stack_ptr) -= 13;	 // r12, r4, r3, r2, r1, r0, r11, r10, r9, r8, r7, r6, r5

	/* set task to ready state */
	tcb->state = TASK_READY;
	tcb->timeslice_ticks = SCHED_RR_TIMESLICE;
	tcb->sleep_ticks = 0;

	/* initalize tcb as an item of list */
	m_item_init(tcb->self, tcb);
	/* push created task to ready queue */
	struct _item *item = &(tcb->self);
	_list_insert_back(&ready_task_queue, item);

}

__attribute__((naked)) static void _init_kernel_stack(void)
{
	__asm__ __volatile__(

		" ldr   r0, =0xe000ed08	\n" /* locating the offset of vector table */
		" ldr   r0, [r0]	\n"
		" ldr   r0, [r0]	\n" /* locate the kernel stack */
		" msr   msp, r0		\n" /* now main stack pointer points to a kernel stack  */
		" dsb			\n" /* required to use after msr instruction with stack pointer*/
		" isb			\n" /* required to use after msr instruction with stack pointer*/
		" cpsie i               \n" /* enable interrupts */
		" svc #0                \n" /* start first task by restoring a context */
	);
}

__attribute__((naked)) void _schd_start_task(struct _tcb **current)
{
	__asm__ __volatile__(

		" cpsid i		\n" /* disable interrupts*/
		" ldr r0, [r0]  	\n"
		" ldr r0, [r0]		\n" /* first entry in tcb is the stack pointer*/
		" ldmia r0!, {r4-r11}	\n" /* restore context */
		" msr psp, r0		\n" /* update process stack pointer*/
		" dsb			\n"
		" isb                   \n"
		" mov lr, 0xfffffffd	\n" /* modify exc_return value to return using process stack pointer*/
		" cpsie i               \n" /* enable interrupts*/
		" bx lr			\n"
	);
}

int _schd_pend_context_switch(void)
{
	int ret = E_SUCC;

	/*if ready queue is empty do not pend context switch*/
	if( !m_list_is_empty(ready_task_queue) ){
		NVIC_INT_CTRL_ST_REG |= NVIC_PENDSVSET_BIT;
	}
	else{
		ret = -E_FAIL;
	}
	return ret;
}

void _schd_schedule_task(void)
{
	struct _tcb *cur = current_running_task;
	struct _item *item = _list_remove_front(&ready_task_queue);
	struct _tcb *next = m_item_parent(struct _tcb*, item);

	if(cur->state != TASK_BLOCKED){

		/* assign task with new time slice if task's time slice
		 * expired last time
		 * */
		if(cur->timeslice_ticks == 0){
			cur->timeslice_ticks = SCHED_RR_TIMESLICE;
		}
		_list_insert_back(&ready_task_queue, &cur->self);
	}
	next->state = TASK_RUNNING;
	current_running_task = next;
}

__attribute__((naked)) extern void PendSV_Handler(void)
{
	__asm__ __volatile__(

		" cpsid i                       \n"  /* disable interrupts*/
		" mrs r0, psp                   \n"
		" dsb                           \n"
		" isb                           \n"
		" ldr r1, =current_running_task \n"
		"                               \n"
		" ldr r2, [r1]                  \n"
		" stmdb r0!, {r4-r11}           \n"  /* push registers r4-r11 to stack*/
		" str r0, [r2]                  \n"  /* save context into current tcb*/
		"                               \n"
		" push {lr, r1}                 \n"
		" bl _schd_schedule_task        \n"  /* contex switch*/
		" pop  {lr, r1}                 \n"
		"                               \n"
		" ldr r2, [r1]                  \n"
		" ldr r2, [r2]                  \n"  /* load new context*/
		" ldmia r2!, {r4-r11}           \n"  /* pop registers from new stack*/
		"                               \n"
		" msr psp, r2                   \n"  /* update process stack pointer */
		" dsb                           \n"
		" isb                           \n"
		" cpsie i                       \n"
		" bx lr                         \n"

	);
}


struct _tcb *get_top_prio_task(void)
{
	struct _item *item = _list_remove_front(&ready_task_queue);
	return m_item_parent(struct _tcb*, item);
}

struct _tcb *_schd_current_running_task(void)
{
	return current_running_task;
}

static void _schd_suspend_task(struct _tcb *tsk, struct _list *list, uint32_t ticks)
{
	if(ticks > 0){
		tsk->sleep_ticks = ticks;
	}
	tsk->state = TASK_BLOCKED;
	_list_insert_back(list, &(tsk->self));
	_schd_pend_context_switch();
}

static struct _tcb *_schd_resume_task(struct _list *list)
{
	struct _item *item = _list_remove_front(list);
	struct _tcb *tsk = NULL;

	/*
	 * Usually when removing task from queue, we should expect
	 * task to be valid
	 * */
	if(item != NULL){

		struct _tcb *tsk = m_item_parent(struct _tcb*, item);
		tsk->state = TASK_READY;
		_list_insert_back(&ready_task_queue, &tsk->self);
	}

	return tsk;
}


void _schd_block_task(struct _list *list)
{
	struct _tcb *tsk = current_running_task;
	return _schd_suspend_task(tsk, list, 0);
}

struct _tcb *_schd_unblock_task(struct _list *list)
{
	return _schd_resume_task(list);
}

int task_sleep(uint32_t ms)
{
	return m_svcall_task_sleep((void*)ms);
}
int _svc_task_sleep(uint32_t ms)
{
	struct _tcb *cur = current_running_task;
	_schd_suspend_task(cur, &sleep_task_queue, ms);

	return E_SUCC;
}

/*
void task_block(struct _list *wait)
{
	struct _tcb *cur = current_running_task;
	cur->state = TASK_BLOCKED;
	_list_insert_back(wait, &(cur->self));
	_schd_pend_context_switch();
}

struct _tcb *task_unblock(struct _list *wait)
{
	struct _item *item = _list_remove_front(wait);
	struct _tcb *task = NULL;

	if(item){

		struct _tcb *task = m_item_parent(struct _tcb*, item);
		task->state = TASK_READY;
		_list_insert_back(&ready_task_queue, &task->self);
	}

	return task;
}

int _svc_task_sleep(uint32_t ms)
{
	struct _tcb *cur = current_running_task;
	cur->sleep_ticks = ms;
	cur->state = TASK_BLOCKED;
	_list_insert_back(&sleep_task_queue, &(cur->self));
	_schd_pend_context_switch();

	return E_SUCC;
}
*/

int task_yield(void)
{
	return m_svcall_task_yield();
}

int _svc_task_yield(void)
{
	return _schd_pend_context_switch();
}


static void idle_task(void)
{
	while(1){
		task_yield();
	}
}

void scheduler_start(void)
{
	INTERRUPTS_DISABLE();

	// create the idle task
	task_create(idle_task, &idle_tcb, idle_stack, STACK_SIZE_MIN, TASK_PRIORITY_IDLE);
	idle_tcb.timeslice_ticks = 1;

	// first task to run
	current_running_task = get_top_prio_task();

	// Initalize systick
	SysTick_Config(CPU_CLOCK_RATE_HZ/1000);

	// Initialize interrupt priorities
	NVIC_SetPriority(SysTick_IRQn, NVIC_PRIO_LOWEST);
	NVIC_SetPriority(PendSV_IRQn, NVIC_PRIO_LOWEST);
	NVIC_SetPriority(SVCall_IRQn, NVIC_PRIO_LOWEST);

	/*start first task by initalizing the kernel stack*/
	_init_kernel_stack();
}

extern void SysTick_Handler(void)
{
	INTERRUPTS_DISABLE();
	{
		/*
		 * Round Robin Scheduling:
		 * if task's time slice is drained to zero pend context switch
		 * */
		struct _tcb *cur = current_running_task;
		if( (--cur->timeslice_ticks) == 0 ){

			/* if current task is the only task possible to run
			 * renew it's timeslice */
			if(_schd_pend_context_switch() == E_FAIL){
				cur->timeslice_ticks = SCHED_RR_TIMESLICE;
			}
		}

		struct _item *cur_item = sleep_task_queue.head;
		struct _tcb *cur_tcb;

		while(cur_item != NULL){

			cur_tcb = m_item_parent(struct _tcb*, cur_item);
			if(--cur_tcb->sleep_ticks == 0){

				struct _item *item = _list_remove(&sleep_task_queue, cur_item);
				struct _tcb *ready_task = m_item_parent(struct _tcb*, item);
				_list_insert_back(&ready_task_queue, &ready_task->self);
			}
			cur_item = cur_item->next;
		}
	}
	INTERRUPTS_ENABLE();

}






