/*
 * pavos_svcall.c
 *
 *  Created on: Mar 14, 2020
 *      Author: pavel
 */

#include"pavos_svcall.h"
#include"pavos_task.h"
#include"pavos_semphr.h"


__attribute__((naked)) uint32_t svcall(uint8_t n, void *params)
{
	__asm__ __volatile__(

		" svc #0 \n"
		" bx lr \n"
	);
}


static void C_SVC_Handler(uint32_t *svc_args)
{
    /*
     * r0 - svc_args[0] contains the system call number
     * r1 - svc_args[1] contains the system call parameters
     * */

    uint8_t syscall_n = svc_args[0];
    int ret;

	switch(syscall_n)
	{
		case SVC_SCHED_START:
			/* empty dummy statement */ ;
			struct tcb *cur = get_current_running_task();
			scheduler_start_task(&cur);
		break;
		case SVC_TASK_YIELD:
			ret = pend_context_switch();
		break;
		case SVC_TASK_SLEEP:
			ret = ktask_sleep( (uint32_t) svc_args[0] );
		break;
		case SVC_SEM_TAKE:
			ret = ksemaphore_take( (struct semaphore *)svc_args[0] );
		break;
		case SVC_SEM_TTAKE:
			ret = ksemaphore_try_take( (struct semaphore *)svc_args[0] );
		case SVC_SEM_GIVE:
			ret = ksemaphore_give( (struct semaphore *)svc_args[0] );
		break;
		case SVC_MTX_LOCK:
			ret = kmutex_lock( (struct semaphore *)svc_args[0] );
		break;
		case SVC_MTX_TLOCK:
			ret = kmutex_try_lock( (struct semaphore *)svc_args[0] );
		break;
		case SVC_MTX_UNLOCK:
			ret = kmutex_unlock( (struct semaphore *)svc_args[0] );
		break;
		default:
		break;
	}
	// modify r0 in stack frame for return value
	svc_args[0] = ret;

	INTERRUPTS_ENABLE();


}

__attribute__((naked)) extern void SVC_Handler(void)
{
	__asm__ __volatile__(

		" cpsid i		\n"     /* disable inerrupts*/
		" tst	lr, #4		\n"     /* test which stack pointer was used to stack*/
		" ite	eq		\n"
		" mrseq r0, msp		\n"
		" mrsne r0, psp		\n"
		" b C_SVC_Handler	\n"     /* call c svc handler to process svcall*/
	);
}



