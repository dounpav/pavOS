/*
 * pavos_assert.h
 *
 *  Created on: Feb 10, 2020
 *      Author: pavel
 */

#ifndef PAVOS_ASSERT_H_
#define PAVOS_ASSERT_H_

#define PAVOS_ASSERT(expr){				\
	if(!(expr)) 						\
		__asm__(" bkpt \n"); 			\
};

#endif /* PAVOS_ASSERT_H_ */
