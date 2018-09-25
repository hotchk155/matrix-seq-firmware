/*
 * leds.h
 *
 *  Created on: 24 Sep 2018
 *      Author: jason
 */

#ifndef LEDS_H_
#define LEDS_H_



CPulseOut<kGPIO_PORTB, 5> g_gate_led; // RED
CPulseOut<kGPIO_PORTC, 2> g_tempo_led; // BLUE
CPulseOut<kGPIO_PORTC, 3> g_cv_led; // YELLOW




#endif /* LEDS_H_ */
