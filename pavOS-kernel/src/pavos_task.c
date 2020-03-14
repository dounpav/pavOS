/*
 * pavos_task.c
 *
 *  Created on: Sep 5, 2019
 *      Author: pavel
 */


#include"pavos_task.h"

/* NVIC Interrupt Control State Register */
#define NVIC_INT_CTRL_ST_REG		             (*((volatile uint32_t*) 0xE000ED04))
/* Bit position to pend PendSV exception */
#define NVIC_PENDSVSET_BIT			             (1 << 28)
/* Reset value for xPSR register */
#define xPSR_RESET_VAL				             (0x01000000)
/* Link register reset value*/
#define LR_RESET_VAL				             (0xffffffff)
/* NVIC lowest interrupt priority*/
#define NVIC_PRIO_LOWEST                         (255)



/*
 * current_running_task
 * Pointer that holds address of currently executing/running task.
 * Pointer should always point to a valid task control block
 */
static struct tcb *current_running_task = NULL;


/*
 * ready_task_queue
 * Holds tasks that are ready to be scheduled.
 * Scheduler pick next task to run only from this queue
 */
static struct list ready_task_queue = LIST_INITIAL_CONTENT;


/*
 * sleep task queue
 * Holds tcb's of tasks that are currently sleeping
 */
static struct list sleep_task_queue = LIST_INITIAL_CONTENT;


static struct tcb idle_tcb;								// task control block for the idle task
static uint32_t idle_stack[STACK_SIZE_MIN];				// stack for idle task
// static uint32_t *sp_kernel;


void task_create(void (*task_function)(void), struct tcb *tcb,
		                                      uint32_t *stack,
		                                  uint32_t stack_size,
		                                     uint8_t priority)
{
	/* create stack frame as it would be created by context switch */

	/* locate stack start address */
	tcb->stack_ptr = &stack[ stack_size - (uint32_t)1 ];
	/* Leave a space to avoid possible memory corruption when returning
	 * from exception */
	tcb->stack_ptr--;
	*(tcb->stack_ptr) = xPSR_RESET_VAL;                         // xPSR
	tcb->stack_ptr--;
	*(tcb->stack_ptr) = (uint32_t) task_function;				// PC
	tcb->stack_ptr--;
	*(tcb->stack_ptr) = LR_RESET_VAL;                           // LR
	(tcb->stack_ptr) -= 13;										// r12, r4, r3, r2, r1, r0, r11, r10, r9, r8, r7, r6, r5

	/* set task to ready state */
	tcb->state = TASK_READY;
	tcb->sleep_ticks = 0;

	/* initalize tcb as an item of list */
	LIST_ITEM_INIT(tcb->self, tcb);
	/* push created task to ready queue */
	list_insert_back(&ready_task_queue, &tcb->self);

}

/*
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
*/

__attribute__((naked)) static void init_kernel_stack(void){

	__asm__ __volatile__(

			" ldr   r0, =0xE000ED08             \n"     /* locating the offset of vector table */
			" ldr   r0, [r0]                    \n" 
			" ldr   r0, [r0]                    \n"     /* locate the kernel stack */
			" msr   msp, r0                     \n"     /* now main stack pointer points to a kernel stack  */
			" isb                               \n"     /* required to use after msr instruction with stack pointer*/
			" dsb                               \n"     /* required to use after msr instruction with stack pointer*/
			" cpsie i                           \n"     /* enable interrupts */
			" svc #0                            \n"     /* start first task by restoring a context */
	);
}

__attribute__((naked)) void scheduler_start_task(struct tcb **current){

	__asm__ __volatile__(

			"   cpsid i                         \n"     /* disable interrupts*/
			"	ldr r0, [r0]  					\n"
			"	ldr r0, [r0]					\n"     /* first entry in tcb is the stack pointer*/
			"	ldmia r0!, {r4-r11}				\n"     /* restore context */
			"	msr psp, r0						\n"     /* update process stack pointer*/
			"	isb								\n"
			"   dsb                             \n"
			"	mov lr, 0xfffffffd				\n"     /* modify exc_return value to return using process stack pointer*/
			"   cpsie i                         \n"     /* enable interrupts*/
			"	bx lr							\n"
	);
}

void pend_context_switch(void){
    
    /*if ready queue is empty do not pend context switch*/
	if( !LIST_IS_EMPTY(ready_task_queue) ){
		NVIC_INT_CTRL_ST_REG = NVIC_PENDSVSET_BIT;
	}
}


//static void C_SVC_Handler(uint32_t *svc_args){

	//uint8_t svc_number;

	/*
	 * r0 - svc_args[0]
	 * r1 - svc_args[1]
	 * r2 - svc_args[2]
	 * r3 - svc_args[3]
	 */

	/*
	 * Retrieve svc number from the svc instruction itself
	 * by subtracting 2 bytes from pc bytes because instruction is 16bits long
	 * */
	//svc_number = ( ( uint8_t * )svc_args[ 6 ] )[ -2 ] ;
/*
	switch(svc_number)
	{
	case 0:
		scheduler_start_task(&current_running_task);
		break;
	case 1:
		pend_context_switch();
		break;
	case 2:
		// semaphore take
		break;
	case 3:
		// semaphore give
		break;
	case 4:
		// mutex lock
		break;
	case 5:
		// mutex release
		break;
	default:
		break;
	}
	INTERRUPTS_ENABLE;
}
*/

//__attribute__((naked)) extern void SVC_Handler(void){

//	__asm__ __volatile__(

//			" cpsid i                           \n"     /* disable inerrupts*/
//			" tst	lr, #4				        \n"     /* test which stack pointer was used to stack*/             
//			" ite	eq					        \n"
//			" mrseq r0, msp				        \n"
//			" mrsne r0, psp				        \n"
//			" b C_SVC_Handler			        \n"     /* call c svc handler to process svcall*/
//	);
//}

__attribute__((naked)) static void context_store(struct tcb **task){

	__asm__ __volatile__(

            " mrs r1, psp                   \n"
            " isb                           \n"
            "                               \n"
			" ldr   r0, [r0]                \n"
			" stmdb r1!, {r4-r11}           \n"         /* push registers r4-r11 to task's stack*/
            " str   r1, [r0]                \n"         /* update tcb with new stack pointer  */
			" dsb                           \n"
			" bx lr                         \n"
	);
}

__attribute__((naked)) static void context_restore(struct tcb **task){

	__asm__ __volatile__(

			" ldr    r0, [r0]                \n"
			" ldr    r0, [r0]                \n"        /* first entry in tcb is stack pointer */
			" ldmia  r0!, {r4-r11}           \n"        /* pop registers form the stack  */
			" msr    psp, r0                 \n"        /* update proces stack pointer */
			" isb                            \n"
			" dsb                            \n"
			" cpsie i                        \n"        /* enable interrupts */
			" bx lr                          \n"
	);

}

static void *schedule_task(void){

    struct tcb *cur = current_running_task;
	struct list_item *item = list_remove_front(&ready_task_queue);
	struct tcb *next = LIST_ITEM_HOLDER(struct tcb*, item);

	if(cur->state != TASK_BLOCKED){
		list_insert_back(&ready_task_queue, &cur->self);
	}
	next->state = TASK_RUNNING;
	current_running_task = next;
}

extern void PendSV_Handler(void){

	INTERRUPTS_DISABLE;

	/*store current task's context*/
	context_store(&current_running_task);

	/*schedule next task*/
	 current_running_task = schedule_task();
     schedule_task();

	/*restore new task's context*/
	context_restore(&current_running_task);
}

struct tcb *get_top_prio_task(void){

	struct list_item *item = list_remove_front(&ready_task_queue);
	return LIST_ITEM_HOLDER(struct tcb*, item);
}

struct tcb *get_current_running_task(void){
	return current_running_task;
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

	while(1) TASK_YIELD;
}

void scheduler_start(void){

	INTERRUPTS_DISABLE;

	// create the idle task
	task_create(idle_task, &idle_tcb, idle_stack, STACK_SIZE_MIN, TASK_PRIORITY_IDLE);

	// first task to run
	current_running_task = get_top_prio_task();

	// Initalize systick
	SysTick_Config(CPU_CLOCK_RATE_HZ/1000);

	// Initialize interrupt priorities
	NVIC_SetPriority(SysTick_IRQn, NVIC_PRIO_LOWEST);
	NVIC_SetPriority(PendSV_IRQn, NVIC_PRIO_LOWEST);
	NVIC_SetPriority(SVCall_IRQn, NVIC_PRIO_LOWEST);

	/*start first task by initalizing first the kernel stack*/
	init_kernel_stack();

	// start first task by switching context with kernel
	// task_context_switch( &sp_kernel, &(current_running_task->stack_ptr) );
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






