/*
 * pavos_types.h
 *
 *  Created on: Sep 5, 2019
 *      Author: pavel
 */

#ifndef PAVOS_TYPES_H_
#define PAVOS_TYPES_H_

#include"cmsis.h"
#include"core_cm3.h"
#include"core_cmFunc.h"
#include<stddef.h>
#include<stdbool.h>
#include<stdint.h>
#include<assert.h>

#define INTERRUPTS_DISABLE()       __disable_irq()
#define INTERRUPTS_ENABLE()	    	__enable_irq()

#define PAVOS_ERR_SUCC                           0
#define PAVOS_ERR_FAIL                          -1

#define SVC(n) __asm__ __volatile__("svc %0" :: "I"(n))

#define _SVC(n)  __asm__ __volatile__("svc %0 \n" :: "I"(n));      \
                __asm__ __volatile__(" mov %0, r0 \n" : "=r"(rc))

#define syscall(n){                                      \
    int ret;                                             \
    SVC(n);                                              \
    __asm__ __volatile__(" mov %0, r0 \n" : "=r"(ret));  \
    return ret;                                          \
}                                                        




/*
 * @brief: finds most significant bit from 32-bit value
 * @return: position of most significant bit
 * */
__attribute__((naked)) uint32_t find_msb(uint32_t value);



#endif /* PAVOS_TYPES_H_ */
