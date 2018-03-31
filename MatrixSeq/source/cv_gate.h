/*
 * cv.h
 *
 *  Created on: 14 Feb 2018
 *      Author: jason
 */

#ifndef CV_GATE_H_
#define CV_GATE_H_


#define BIT_GATE1		MK_GPIOA_BIT(PORTD_BASE, 3)
#define BIT_GATE2		MK_GPIOA_BIT(PORTD_BASE, 2)
#define BIT_GATE3		MK_GPIOA_BIT(PORTD_BASE, 4)
#define BIT_GATE4		MK_GPIOA_BIT(PORTA_BASE, 6)


class CCVGate {

	enum {
		MAX_CV = 4,
		MAX_GATE = 4
	};
	uint16_t m_dac[MAX_CV];
	byte m_gate[MAX_GATE];
	byte m_cv_pending;
	byte m_gate_pending;


public:
	enum {
		GATE_CLOSED,
		GATE_OPEN,
		GATE_RETRIG,
	};
	// pitch is defined a 256 * midi note number + fractional part
	typedef uint16_t PITCH_TYPE;

	// gate type is defined as milliseconds. zero is infinite
	typedef uint16_t GATE_TYPE;

	// cv type is a 16 bit value to be mapped to the full voltage range
	typedef uint16_t CV_TYPE;

	typedef uint16_t DAC_TYPE;


	CCVGate() {
		memset((byte*)m_dac,0,sizeof m_dac);
		memset((byte*)m_gate,0,sizeof m_gate);
		m_cv_pending = 0;
		m_gate_pending = 0;
	}

	void gate_on(byte which) {
		switch(which) {
		case 0: SET_GPIOA(BIT_GATE1); break;
		case 1: SET_GPIOA(BIT_GATE2); break;
		case 2: SET_GPIOA(BIT_GATE3); break;
		case 3: SET_GPIOA(BIT_GATE4); break;
		}
	}
	void gate_off(byte which) {
		switch(which) {
		case 0: CLR_GPIOA(BIT_GATE1); break;
		case 1: CLR_GPIOA(BIT_GATE2); break;
		case 2: CLR_GPIOA(BIT_GATE3); break;
		case 3: CLR_GPIOA(BIT_GATE4); break;
		}
	}

	void note_gate(int which, int note, int gate) {
		int dac = (500 * (int)note)/12;
		if(m_dac[which] != dac) {
			m_dac[which] = dac;
			m_cv_pending = 1;
		}
		if(m_gate[which] != gate) {
			switch(gate) {
				case GATE_CLOSED:
					gate_off(which);
					m_gate[which] = GATE_CLOSED;
					break;
				case GATE_RETRIG:
					gate_off(which);
					m_gate[which] = GATE_OPEN;
					m_gate_pending = 1;
					break;
				case GATE_OPEN:
					m_gate[which] = GATE_OPEN;
					m_gate_pending = 1;
					break;
			}
		}
	}

	void run() {

		// if we have CV data to send and the bus is clear
		// then send out the CV data to DAC
		if(!g_i2c_bus.busy()) {
			if(m_cv_pending) {
				g_i2c_bus.dac_write(m_dac);
				m_cv_pending = 0;
			}
			else if(m_gate_pending) { // rising edge pending?
				for(int i=0; i<4; ++i) {
					if(m_gate[i] == GATE_OPEN) {
						gate_on(i);
					}
				}
				m_gate_pending = 0;
			}
		}
	}

};

extern CCVGate g_cv_gate;
#ifdef MAIN_INCLUDE
CCVGate g_cv_gate;
CDigitalOut<kGPIO_PORTA, 6> pGate4;
CDigitalOut<kGPIO_PORTD, 4> pGate3;
CDigitalOut<kGPIO_PORTD, 3> pGate2;
CDigitalOut<kGPIO_PORTD, 2> pGate1;
#endif


#endif /* CV_GATE_H_ */
