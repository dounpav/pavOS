/*
 * pavos_svcall.c
 *
 *  Created on: Mar 14, 2020
 *      Author: pavel
 */

#include"pavos_svcall.h"
#include"pavos_task.h"
#include"pavos_semphr.h"
#include"pavos_mbox.h"


__attribute__((naked)) uint32_t svcall(uint8_t n, void *p1, void *p2, void *p3)
{
	__asm__ __volatile__(

		" svc #0 \n"
		" bx lr \n"
	);
}




extern void C_SVC_Handler(uint32_t *svc_args)
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
			struct _tcb *cur = _schd_current_running_task();
			_schd_start_task(&cur);
			break;
		case SVC_TASK_YIELD:
			ret = _svc_task_yield();
			break;
		case SVC_TASK_SLEEP:
			ret = _svc_task_sleep( svc_args[1] );
			break;
		case SVC_SEM_TAKE:
			ret = _svc_semphr_take( (struct _semphr *)svc_args[1], false);
			break;
		case SVC_SEM_TTAKE:
			ret = _svc_semphr_take( (struct _semphr *)svc_args[1], true);
			break;
		case SVC_SEM_GIVE:
			ret = _svc_semphr_give( (struct _semphr *)svc_args[1] );
			break;
		case SVC_MTX_LOCK:
			ret = _svc_mutex_lock( (struct _semphr *)svc_args[1], false);
			break;
		case SVC_MTX_TLOCK:
			ret = _svc_mutex_lock( (struct _semphr *)svc_args[1], true);
			break;
		case SVC_MTX_UNLOCK:
			ret = _svc_mutex_unlock( (struct _semphr *)svc_args[1] );
			break;
		case SVC_MBOX_SEND:
			ret = _svc_mbox_send( (struct _mbox *)svc_args[1], (void*)svc_args[2], false);
			break;
		case SVC_MBOX_TSEND:
			ret = _svc_mbox_send( (struct _mbox *)svc_args[1], (void*)svc_args[2], true);
			break;
		case SVC_MBOX_RECV:
			ret = _svc_mbox_recv( (struct _mbox *)svc_args[1], (void*)svc_args[2], false);
			break;
		case SVC_MBOX_TRECV:
			ret = _svc_mbox_recv( (struct _mbox *)svc_args[1], (void*)svc_args[2], true);
		default:
			break;
	}
	/* modify r0 in stack frame for return value */
	svc_args[0] = ret;

	/* enable interrupts */
	m_arch_intr_enable();
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



