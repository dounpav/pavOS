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
	struct list_item *temp = &(tcb->self);
	list_insert_back(&ready_task_queue, temp);

}

__attribute__((naked)) static void init_kernel_stack(void){

	__asm__ __volatile__(

			" ldr   r0, =0xE000ED08             \n"     /* locating the offset of vector table */
			" ldr   r0, [r0]                    \n" 
			" ldr   r0, [r0]                    \n"     /* locate the kernel stack */
			" msr   msp, r0                     \n"     /* now main stack pointer points to a kernel stack  */
			" dsb                               \n"     /* required to use after msr instruction with stack pointer*/
			" isb                               \n"     /* required to use after msr instruction with stack pointer*/
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
		NVIC_INT_CTRL_ST_REG |= NVIC_PENDSVSET_BIT;
	}
}


__attribute__((naked)) void context_store(struct tcb **task){

	__asm__ __volatile__(

            " mrs r1, psp                   \n"
            " isb                           \n"
			" ldr   r0, [r0]                \n"
			" stmdb r1!, {r4-r11}           \n"         /* push registers r4-r11 to task's stack*/
            " str   r1, [r0]                \n"         /* update tcb with new stack pointer  */
			" dsb                           \n"
			" bx lr                         \n"
	);
}

__attribute__((naked)) void context_restore(struct tcb **task){

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

static void schedule_task(void){

    struct tcb *cur = current_running_task;
	struct list_item *item = list_remove_front(&ready_task_queue);
	struct tcb *next = LIST_ITEM_HOLDER(struct tcb*, item);

	if(cur->state != TASK_BLOCKED){
		list_insert_back(&ready_task_queue, &cur->self);
	}
	next->state = TASK_RUNNING;
	current_running_task = next;
}

__attribute__((naked)) extern void PendSV_Handler(void){

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
            " bl schedule_task              \n"  /* contex switch*/
            " pop {lr, r1}                  \n"
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

extern void _PendSV_Handler(void){

	INTERRUPTS_DISABLE;

	/*store current task's context*/
	context_store(&current_running_task);

	/*schedule next task*/
     schedule_task();

	/*restore new task's context*/
	context_restore(&current_running_task);

	return;
}

struct tcb *get_top_prio_task(void){

	struct list_item *item = list_remove_front(&ready_task_queue);
	return LIST_ITEM_HOLDER(struct tcb*, item);
}

struct tcb *get_current_running_task(void){
	return current_running_task;
}

void task_block(struct list *wait){

	struct tcb *cur = current_running_task;
	cur->state = TASK_BLOCKED;

	list_insert_back(wait, &(cur->self));
	pend_context_switch();
}

struct tcb *task_unblock(struct list *wait){

	struct list_item *item = list_remove_front(wait);
	struct tcb *task = LIST_ITEM_HOLDER(struct tcb*, item);

	task->state = TASK_READY;
	list_insert_back(&ready_task_queue, &task->self);

	return task;
}

void task_sleep(uint32_t ms){
    __asm__ __volatile__("svc #0x2\n");
}
void ktask_sleep(uint32_t ms){

	//INTERRUPTS_DISABLE;
	//{
		struct tcb *cur = current_running_task;
		cur->sleep_ticks = ms;
		cur->state = TASK_BLOCKED;

        /* insert task to the back of the sleep queue */
		list_insert_back(&sleep_task_queue, &(cur->self));
        pend_context_switch();
	//}
    //INTERRUPTS_ENABLE;
}

void task_yield(void){ TASK_YIELD; }


void idle_task(void){

	while(1){
		task_yield();
	}
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

	/*start first task by initalizing the kernel stack*/
	init_kernel_stack();
}

extern void SysTick_Handler(void){

	INTERRUPTS_DISABLE;
	{
		struct list_item *cur_item = sleep_task_queue.head;
		struct tcb *cur_tcb;

		while(cur_item != NULL){

			cur_tcb = LIST_ITEM_HOLDER(struct tcb*, cur_item);
			if(--cur_tcb->sleep_ticks == 0){

				struct list_item *item = list_remove(&sleep_task_queue, cur_item);
				struct tcb *ready_task = LIST_ITEM_HOLDER(struct tcb*, item);
				list_insert_back(&ready_task_queue, &ready_task->self);
			}
			cur_item = cur_item->next;
		}
	}
	INTERRUPTS_ENABLE;

}






