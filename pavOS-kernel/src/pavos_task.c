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

static struct _list ready_task_queue[TASK_PRIORITY_CNT];

/*
 * Ready queue priority bitmap that tells which of the raeady queues are
 * ready to run. Each bit corresponds to a one task priority.
 *
 * When nth bit is set, then queue with nth priority is ready to run, which
 * means that ready queue contains at least one task ready to run.
 *
 * Bitmap will be equal to 2^PRIORITY_CNT-1 when all queues are ready to run
 * Bitmap will be equal to zero if no queues are ready to run, the current
 * running task is the only task that can be ran at that time.
 */
static uint8_t rq_prio_bmap = 0;

#define m_set_runnable_prio(prio)	rq_prio_bmap = rq_prio_bmap | (1 << prio)
#define m_unset_runnable_prio(prio)	rq_prio_bmap = rq_prio_bmap ^ (1 << prio)
#define m_top_runnable_prio		msb_uint32(rq_prio_bmap)


/*
 * sleep task queue
 * Holds tcb's of tasks that are currently sleeping
 */
static struct _list sleep_task_queue = m_list_initial_content;

static struct _tcb idle_tcb;			// task control block for the idle task
static uint32_t idle_stack[STACK_SIZE_MIN];	// stack for idle task


__attribute__ ((naked)) uint32_t msb_uint32(uint32_t value)
{
	__asm__ __volatile__(

		"	clz r1, r0				\n"
		"	mov r2, #31				\n"
		"	sub r2, r1				\n"
		"	mov r0, r2				\n"
		"	bx lr					\n"
	);
}


static void _schd_insert_ready_queue(struct _tcb *tsk)
{
	_list_insert_back( &ready_task_queue[tsk->prio], &tsk->self);
	m_set_runnable_prio( tsk->prio );
}

static struct _tcb *_schd_remove_ready_queue(uint8_t prio)
{
	struct _item *item = _list_remove_front(&ready_task_queue[prio]);
	struct _tcb *tsk = m_item_parent(struct _tcb*, item);

	if( m_list_is_empty(ready_task_queue[prio]) )
	{
		m_unset_runnable_prio(prio);
	}

	return tsk;
}

static struct _tcb *_schd_top_prio_task()
{
	uint8_t prio = m_top_runnable_prio;
	return _schd_remove_ready_queue(prio);
}


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
	tcb->prio = priority;
	tcb->sv_prio = priority;
	tcb->msg_ptr = NULL;

	/* initalize tcb as an item of list */
	m_item_init(tcb->self, tcb);
	/* insert created task to ready queue */
	_schd_insert_ready_queue(tcb);
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
	NVIC_INT_CTRL_ST_REG |= NVIC_PENDSVSET_BIT;
	return E_SUCC;
}

void _schd_schedule_task(void)
{
	struct _tcb *cur = current_running_task;

	if(cur->state != TASK_BLOCKED){

		/* assign task with new time slice if task's time slice
		 * expired last time
		 * */
		if(cur->timeslice_ticks == 0){
			cur->timeslice_ticks = SCHED_RR_TIMESLICE;
		}
		_schd_insert_ready_queue(cur);
	}
	struct _tcb *next = _schd_top_prio_task();
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

		tsk = m_item_parent(struct _tcb*, item);
		tsk->state = TASK_READY;
		_schd_insert_ready_queue(tsk);
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

int task_yield(void)
{
	return m_svcall_task_yield();
}

int _svc_task_yield(void)
{
	/* if there are no runnable ready queues do not 
	 * pend context switch
	 * */
	if( rq_prio_bmap != 0 ){
		return _schd_pend_context_switch();
	}
	else{
		return E_FAIL;
	}
}


static void idle_task(void)
{
	while(1){
		task_yield();
	}
}

void scheduler_start(void)
{
	m_arch_intr_disable();

	// create the idle task
	task_create(idle_task, &idle_tcb, idle_stack, STACK_SIZE_MIN, TASK_PRIORITY_IDLE);
	idle_tcb.timeslice_ticks = 1;

	// first task to run
	current_running_task = _schd_top_prio_task();

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
	m_arch_intr_disable();
	{
		/*
		 * Round Robin Scheduling:
		 * if task's time slice is drained to zero pend context switch
		 * */
		struct _tcb *cur = current_running_task;
		if( (--cur->timeslice_ticks) == 0 ){

			/* if current task is the only task possible to run
			 * renew it's timeslice */
			if( rq_prio_bmap != 0 )
			{
				_schd_pend_context_switch();
			}
			else{
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

				_schd_insert_ready_queue(ready_task);
			}
			cur_item = cur_item->next;
		}
	}
	m_arch_intr_enable();

}






