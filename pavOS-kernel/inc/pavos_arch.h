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

#define m_arch_intr_disable()		__disable_irq()
#define m_arch_intr_enable()		__enable_irq()

#define E_SUCC						0
#define E_FAIL						1


#endif /* PAVOS_TYPES_H_ */
