/*
 * pavos_arch.h
 *
 *  Created on: Sep 5, 2019
 *      Author: pavel
 */

#ifndef PAVOS_ARCH_H_
#define PAVOS_ARCH_H_


#include"cmsis.h"
#include"core_cm3.h"
#include"core_cmFunc.h"

#include<stddef.h>
#include<stdbool.h>
#include<stdint.h>
#include<assert.h>

#define m_arch_intr_disable()		__disable_irq()
#define m_arch_intr_enable()		__enable_irq()

#endif /* PAVOS_ARCH_H_ */
