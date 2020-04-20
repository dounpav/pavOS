/*
 * pavos_svcall.c
 *
 *  Created on: Mar 14, 2020
 *      Author: pavel
 */

#include"pavos_syscall.h"
#include"pavos_task.h"
#include"pavos_semphr.h"


__attribute__((naked)) uint32_t sys_call(uint8_t n, void *param){

    __asm__ __volatile__(

            " svc #0 \n"
            " bx lr \n"
    );
}


static void C_SVC_Handler(uint32_t *svc_args){

    /*
     * r0 - svc_args[0] contains the system call number
     * r1 - svc_args[1] contains the system call parameters
     * */

    uint8_t syscall_n = svc_args[0];
    int ret;

	switch(syscall_n)
	{
	case SYS_SCHED_START: 
        /* empty dummy statement */ ;
        struct tcb *cur = get_current_running_task();
		scheduler_start_task(&cur);
		break;
	case SYS_TASK_YIELD:
		ret = pend_context_switch();
		break;
    case SYS_TASK_SLEEP:
        ret = ktask_sleep( (uint32_t) svc_args[0] );
        break;
	case SYS_SEM_TAKE:
        ret = ksemaphore_take( (struct semaphore *)svc_args[0] );
		break;
    case SYS_SEM_TTAKE:
        ret = ksemaphore_try_take( (struct semaphore *)svc_args[0] );
	case SYS_SEM_GIVE:
        ret = ksemaphore_give( (struct semaphore *)svc_args[0] );
		break;
	case SYS_MTX_LOCK:
        ret = kmutex_lock( (struct semaphore *)svc_args[0] );
		break;
    case SYS_MTX_TLOCK:
        ret = kmutex_try_lock( (struct semaphore *)svc_args[0] );
        break;
    case SYS_MTX_UNLOCK:
        ret = kmutex_unlock( (struct semaphore *)svc_args[0] );
		break;
	default:
		break;
	}
    // modify r0 in stack frame for return value
    svc_args[0] = ret;

	INTERRUPTS_ENABLE();


}


static void C_SVC_Handler_old(uint32_t *svc_args){

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
	uint8_t svc_number = ((uint8_t *)svc_args[6])[-2];
    int ret;

	switch(svc_number)
	{
	case 0: 
        /* empty dummy statement */ ;
        struct tcb *cur = get_current_running_task();
		scheduler_start_task(&cur);
		break;
	case 1:
		ret = pend_context_switch();
		break;
    case 2:
        ret = ktask_sleep( (uint32_t) svc_args[0] );
        break;
	case 3:
        ret = ksemaphore_take( (struct semaphore *)svc_args[0] );
		break;
    case 4:
        ret = ksemaphore_try_take( (struct semaphore *)svc_args[0] );
	case 5:
        ret = ksemaphore_give( (struct semaphore *)svc_args[0] );
		break;
	case 6:
        ret = kmutex_lock( (struct semaphore *)svc_args[0] );
		break;
	case 7:
        ret = kmutex_unlock( (struct semaphore *)svc_args[0] );
		break;
    case 8:
        ret = kmutex_try_lock( (struct semaphore *)svc_args[0] );
	default:
		break;
	}
    // modify r0 in stack frame for return value
    svc_args[0] = ret;

	INTERRUPTS_ENABLE();
}


__attribute__((naked)) extern void SVC_Handler(void){

	__asm__ __volatile__(

			" cpsid i                           \n"     /* disable inerrupts*/
			" tst	lr, #4				        \n"     /* test which stack pointer was used to stack*/             
			" ite	eq					        \n"
			" mrseq r0, msp				        \n"
			" mrsne r0, psp				        \n"
			" b C_SVC_Handler			        \n"     /* call c svc handler to process svcall*/
	);
}



