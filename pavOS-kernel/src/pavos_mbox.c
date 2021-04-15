
/* file: pavos_mbox.c*/

#include "pavos_svcall.h"
#include "pavos_task.h"
#include "pavos_mbox.h"
#include <string.h>

#define m_svcall_mbox_send(mbox, msg)		svcall(SVC_MBOX_SEND, mbox, msg, NULL)
#define m_svcall_mbox_try_send(mbox, msg)	svcall(SVC_MBOX_TSEND, mbox, msg, NULL)
#define m_svcall_mbox_recv(mbox, msg)		svcall(SVC_MBOX_RECV, mbox, msg, NULL)
#define m_svcall_mbox_try_recv(mbox, msg)	svcall(SVC_MBOX_RECV, mbox, msg, NULL)

void mailbox_create(mailbox_t *mbox, uint8_t *buffer, uint32_t size)
{
	m_list_init(mbox->send_queue);
	m_list_init(mbox->recv_queue);

	mbox->msg_storg = buffer;
	mbox->msg_sz = size;
	mbox->dirty = 0;
}

int mailbox_send(mailbox_t *mbox, void *src)
{
	return m_svcall_mbox_send(mbox, src);
}

int mailbox_try_send(mailbox_t *mbox, void *src)
{
	return m_svcall_mbox_try_send(mbox, src);
}

int mailbox_recv(mailbox_t *mbox, void *dest)
{
	return m_svcall_mbox_recv(mbox, dest);
}

int mailbox_try_recv(mailbox_t *mbox, void *dest)
{
	return m_svcall_mbox_try_recv(mbox, dest);
}

int _svc_mbox_send(struct _mbox *mbox, void *src, bool try)
{
	struct _tcb *cur = _schd_current_running_task();
	int ret;

	cur->msg_ptr = src;

	/*
	 * If the mailbox contains a message that is not yet recieved by other
	 * task, block calling task until the unread message is received or
	 * return immedately
	 * */
	if(mbox->dirty == 1)
	{
		if(try){
			ret = E_FAIL;
		}
		else{
			cur->msg_ptr = src;
			_schd_block_task( &(mbox->send_queue) );
			ret = E_SUCC;
		}
	}
	/*
	 * If mailbox is does not contain any unread/dirty messages, then write
	 * new message to the mailbox.
	 * */
	else{
		memcpy(mbox->msg_storg, src, mbox->msg_sz);
		/*
		 * Unblock any waiting task and copy the message from the
		 * mailbox to task's destination messgage buffer
		 * */
		if( !m_list_is_empty(mbox->recv_queue) )
		{
			struct _tcb *tsk = _schd_unblock_task( &(mbox->recv_queue) );
			memcpy(tsk->msg_ptr, mbox->msg_storg, mbox->msg_sz);
			ret = E_SUCC;
		}
		/*
		 * If no task is waiting for the message, block current task
		 * until the message is received or return immediately
		 * */
		else{
			mbox->dirty = 1;
			if(try){
				ret = E_FAIL;
			}
			else{
				_schd_block_task( &(mbox->send_queue) );
				ret = E_SUCC;
			}
		}
	}

	return ret;
}

int _svc_mbox_recv(struct _mbox *mbox, void *dest, bool try)
{
	struct _tcb *cur = _schd_current_running_task();
	int ret;

	cur->msg_ptr = dest;

	/*
	 * If mailbox is empty(does not contain unread message) block task
	 * until new message is sent or return immediately
	 * */
	if(mbox->dirty == 0){

		if(try)
		{
			ret = E_FAIL;
		}
		else{
			_schd_block_task( &(mbox->recv_queue) );
			ret = E_SUCC;
		}
	}
	/*
	 * If mailbox contains unread message then copy the message to
	 * task's destination buffer
	 * */
	else{
		memcpy(dest, mbox->msg_storg, mbox->msg_sz);
		/*
		 * if send queue contains a task, unblock it and copy its
		 * message to the mailbox. Otherwise set mailbox to clean.
		 * */
		if( !m_list_is_empty(mbox->send_queue) )
		{
			struct _tcb *tsk = _schd_unblock_task( &(mbox->send_queue) );
			memcpy(mbox->msg_storg, tsk->msg_ptr, mbox->msg_sz);
		}
		else{
			mbox->dirty = 0;
			ret = E_SUCC;
		}
	}

	return ret;
}

