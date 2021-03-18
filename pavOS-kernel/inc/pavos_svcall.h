/*
 * pavos_svcall.h
 *
 *  Created on: Apr 16, 2020
 *      Author: pavel
 */

#ifndef PAVOS_SYSCALL_H_
#define PAVOS_SYSCALL_H_

#include<stdint.h>

#define E_SUCC		0
#define E_FAIL		1

#define SVC_SCHED_START 0
#define SVC_TASK_YIELD  1
#define SVC_TASK_SLEEP  2
#define SVC_SEM_TAKE    3
#define SVC_SEM_TTAKE   4
#define SVC_SEM_GIVE    5
#define SVC_MTX_LOCK    6
#define SVC_MTX_TLOCK   7
#define SVC_MTX_UNLOCK  8
#define SVC_MBOX_SEND	9
#define SVC_MBOX_TSEND	10
#define SVC_MBOX_RECV	11
#define SVC_MBOX_TRECV	12


__attribute__((naked)) uint32_t svcall(uint8_t n, void *p1, void *p2, void *p3);


#endif /* PAVOS_SYSCALL_H_ */
