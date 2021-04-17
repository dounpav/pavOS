
/*
 * pavos_mbox.h
 * */

#ifndef PAVOS_MBOX_H_
#define PAVOS_MBOX_H_

#include "pavos_list.h"
#include <stdbool.h>

typedef struct _mbox{

	uint8_t		*msg_storg;
	uint8_t		dirty;
	uint32_t	msg_sz;
	struct _list	send_queue;
	struct _list	recv_queue;
}mailbox_t;

void mailbox_create(mailbox_t *mbox, uint8_t *msg_buff, uint32_t sz);

int mailbox_send(mailbox_t *mbox, void *src);
int mailbox_try_send(mailbox_t *mbox, void *src);

int mailbox_recv(mailbox_t *mbox, void *dest);

int _svc_mbox_send(struct _mbox *mbox, void *src, bool try);
int _svc_mbox_recv(struct _mbox *mbox, void *dest, bool try);

#endif /* PAVOS_MBOX_H_*/
