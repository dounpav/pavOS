/*
 * pavos_svcall.h
 *
 *  Created on: Apr 16, 2020
 *      Author: pavel
 */

#ifndef PAVOS_SYSCALL_H_
#define PAVOS_SYSCALL_H_

#include<stdint.h>

#define SVC_SCHED_START 0
#define SVC_TASK_YIELD  1
#define SVC_TASK_SLEEP  2
#define SVC_SEM_TAKE    3
#define SVC_SEM_TTAKE   4
#define SVC_SEM_GIVE    5
#define SVC_MTX_LOCK    6
#define SVC_MTX_TLOCK   7
#define SVC_MTX_UNLOCK  8


__attribute__((naked)) uint32_t svcall(uint8_t n, void *params);


#endif /* PAVOS_SYSCALL_H_ */
