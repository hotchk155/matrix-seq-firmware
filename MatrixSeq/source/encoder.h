/*
 * encoder.h
 *
 *  Created on: 5 Feb 2018
 *      Author: jason
 */

#ifndef ENCODER_H_
#define ENCODER_H_

#include "defs.h"
#include "digital_out.h"
#ifdef MAIN_INCLUDE
CDigitalIn<kGPIO_PORTD, 0> pEncoder1;
CDigitalIn<kGPIO_PORTD, 1> pEncoder2;
#endif

#define BIT_ENCODER1	MK_GPIOA_BIT(PORTD_BASE, 0)
#define BIT_ENCODER2	MK_GPIOA_BIT(PORTD_BASE, 1)

class CEncoder {
	byte m_prev_state[3] = {0};

public:
	CEncoder() {
	}
	int read() {

		int delta = 0;
		// get the state of the two inputs into a 2 bit value
		byte new_state = 0;
		if(!(READ_GPIOA(BIT_ENCODER1))) {
			new_state |= 0b10;
		}
		if(!(READ_GPIOA(BIT_ENCODER2))) {
			new_state |= 0b01;
		}


		// make sure the state has changed and does not match
		// the previous state (which may indicate a bounce)
		if(new_state != m_prev_state[0] && new_state != m_prev_state[1]) {

			if(new_state == 0b11) {
				if( (m_prev_state[0] == 0b10) &&
					(m_prev_state[1] == 0b00) &&
					(m_prev_state[2] == 0b01)) {
					delta = 1;
				}
				else if( (m_prev_state[0] == 0b01) &&
					(m_prev_state[1] == 0b00) &&
					(m_prev_state[2] == 0b10)) {
					delta = -1;
				}
			}

			m_prev_state[2] = m_prev_state[1];
			m_prev_state[1] = m_prev_state[0];
			m_prev_state[0] = new_state;
		}

		return delta;
	}
};

#endif /* ENCODER_H_ */
