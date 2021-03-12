/*
 * pavos_config.h
 *
 *  Created on: Sep 10, 2019
 *      Author: pavel
 */

#ifndef PAVOS_CONFIG_H_
#define PAVOS_CONFIG_H_

#define CPU_CLOCK_RATE_HZ 				72000000
#define SYSTICK_RATE_HZ 				1000

#define STACK_SIZE_MIN					200

#define TASK_PRIORITY_CNT				8
#define TASK_PRIORITY_MAX				TASK_PRIORITY_CNT -1
#define TASK_PRIORITY_IDLE				0

#endif /* PAVOS_CONFIG_H_ */
