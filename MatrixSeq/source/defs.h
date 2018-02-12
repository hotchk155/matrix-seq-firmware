/*
 * defs.h
 *
 *  Created on: 4 Feb 2018
 *      Author: jason
 */

#ifndef DEFS_H_
#define DEFS_H_

#include "fsl_common.h"

enum {
	EV_NONE,
	EV_ENCODER,
	EV_KEY_PRESS,
	EV_KEY_RELEASE
};

typedef unsigned char byte;

#define PORTA_BASE 0
#define PORTB_BASE 8
#define PORTC_BASE 16
#define PORTD_BASE 24

#define MK_GPIOA_BIT(port, bit) (((uint32_t)1) << ((port) + (bit)))
#define SET_GPIOA(mask) ((GPIO_Type *)GPIOA_BASE)->PSOR = (mask)
#define CLR_GPIOA(mask) ((GPIO_Type *)GPIOA_BASE)->PCOR = (mask)
#define READ_GPIOA(mask) (((GPIO_Type *)GPIOA_BASE)->PDIR & (mask))


extern void fire_event(int event, uint32_t param);

extern const uint32_t char4x5[];
#define CHAR4X5_ALPHA		0
#define CHAR4X5_HASH		26
#define CHAR4X5_NUMERIC		32
#define CHAR4X5_MINUS		42


#endif /* DEFS_H_ */
