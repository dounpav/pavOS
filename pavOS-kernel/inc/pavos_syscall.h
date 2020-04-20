/*
 * pavos_syscall.h
 *
 *  Created on: Apr 16, 2020
 *      Author: pavel
 */

#ifndef PAVOS_SYSCALL_H_
#define PAVOS_SYSCALL_H_

#include<stdint.h>

#define SYS_SCHED_START 0
#define SYS_TASK_YIELD  1
#define SYS_TASK_SLEEP  2
#define SYS_SEM_TAKE    3
#define SYS_SEM_TTAKE   4
#define SYS_SEM_GIVE    5
#define SYS_MTX_LOCK    6
#define SYS_MTX_TLOCK   7
#define SYS_MTX_UNLOCK  8


__attribute__((naked)) uint32_t sys_call(uint8_t n, void *param);


#endif /* PAVOS_SYSCALL_H_ */
