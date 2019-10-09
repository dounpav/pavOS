/*
 * pavos_types.h
 *
 *  Created on: Sep 5, 2019
 *      Author: pavel
 */

#ifndef PAVOS_TYPES_H_
#define PAVOS_TYPES_H_

#include<stddef.h>
#include<stdbool.h>
#include<stdint.h>
#include<assert.h>



/*
 * @brief: finds most significant bit from 32-bit value
 * @return: position of most significant bit
 * */
__attribute__((naked)) uint32_t find_msb(uint32_t value);



#endif /* PAVOS_TYPES_H_ */
