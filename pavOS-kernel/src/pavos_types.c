/*
 * pavos_types.c
 *
 *  Created on: Sep 20, 2019
 *      Author: pavel
 */

#include"pavos_types.h"


__attribute__ ((naked)) uint32_t find_msb(uint32_t value){

	__asm__ __volatile__(

			"	clz r1, r0				\n"
			"	mov r2, #31				\n"
			"	sub r2, r1				\n"
			"	mov r0, r2				\n"
			"	bx lr					\n"
	);
}


