/*
 * pavos_svcall.c
 *
 *  Created on: Mar 14, 2020
 *      Author: pavel
 */

#include"pavos_task.h"
#include"pavos_semphr.h"


static void C_SVC_Handler(uint32_t *svc_args){

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
	switch(svc_number)
	{
	case 0: 
        /* empty dummy statement */ ;
        struct tcb *cur = get_current_running_task();
		scheduler_start_task(&cur);
		break;
	case 1:
		pend_context_switch();
		break;
    case 2:
        ktask_sleep( (uint32_t) svc_args[0] );
        break;
	case 3:
        ksemaphore_take( (struct semaphore *)svc_args[0] );
		break;
	case 4:
        ksemaphore_give( (struct semaphore *)svc_args[0] );
		break;
	case 5:
        kmutex_lock( (struct semaphore *)svc_args[0] );
		break;
	case 6:
        kmutex_unlock( (struct semaphore *)svc_args[0] );
		break;
	default:
		break;
	}
	INTERRUPTS_ENABLE;
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



