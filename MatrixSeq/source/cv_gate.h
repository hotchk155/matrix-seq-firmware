/*
 * cv.h
 *
 *  Created on: 14 Feb 2018
 *      Author: jason
 */

#ifndef CV_GATE_H_
#define CV_GATE_H_

#define PORTD_BIT_GATE4 4
#define PORTD_BIT_GATE2 3
#define PORTD_BIT_GATE3 2
#define PORTA_BIT_GATE1 6

#define BIT_GATE4		MK_GPIOA_BIT(PORTD_BASE, PORTD_BIT_GATE4)
#define BIT_GATE2		MK_GPIOA_BIT(PORTD_BASE, PORTD_BIT_GATE2)
#define BIT_GATE3		MK_GPIOA_BIT(PORTD_BASE, PORTD_BIT_GATE3)
#define BIT_GATE1		MK_GPIOA_BIT(PORTA_BASE, PORTA_BIT_GATE1)

// These definitions are used to initialise the port
CDigitalOut<kGPIO_PORTA, PORTA_BIT_GATE1> g_gate_1;
CDigitalOut<kGPIO_PORTD, PORTD_BIT_GATE2> g_gate_2;
CDigitalOut<kGPIO_PORTD, PORTD_BIT_GATE3> g_gate_3;
CDigitalOut<kGPIO_PORTD, PORTD_BIT_GATE4> g_gate_4;


/////////////////////////////////////////////////////////////////////////////////
class CCVGate {
public:
	typedef enum:byte  {
		GATE_CLOSED,
		GATE_OPEN,
		GATE_RETRIG,
	} GATE_STATE;
	enum {
		MAX_CV = 4,
		MAX_GATE = 4
	};

	typedef struct {
		int pitch;			// 32-bit current pitch value (dac << 16)
		int target;  		// 32-bit current target value (dac << 16)
		int glide_rate;  	// glide rate applied per ms to the pitch
	} CV_STATE;



public:
	CV_STATE m_cv[MAX_CV];
	uint16_t m_dac[MAX_CV];
	GATE_STATE m_gate[MAX_GATE];
	byte m_cv_pending;
	byte m_gate_pending;

	/////////////////////////////////////////////////////////////////////////////////
	void gate_on(byte which) {
		switch(which) {
		case 0: SET_GPIOA(BIT_GATE1); break;
		case 1: SET_GPIOA(BIT_GATE2); break;
		case 2: SET_GPIOA(BIT_GATE3); break;
		case 3: SET_GPIOA(BIT_GATE4); break;
		}
	}

	/////////////////////////////////////////////////////////////////////////////////
	void gate_off(byte which) {
		switch(which) {
		case 0: CLR_GPIOA(BIT_GATE1); break;
		case 1: CLR_GPIOA(BIT_GATE2); break;
		case 2: CLR_GPIOA(BIT_GATE3); break;
		case 3: CLR_GPIOA(BIT_GATE4); break;
		}
	}

public:

	// pitch is defined a 256 * midi note number + fractional part
	typedef uint16_t PITCH_TYPE;

	// gate type is defined as milliseconds. zero is infinite
	typedef uint16_t GATE_TYPE;

	// cv type is a 16 bit value to be mapped to the full voltage range
	typedef uint16_t CV_TYPE;

	typedef uint16_t DAC_TYPE;


	/////////////////////////////////////////////////////////////////////////////////
	CCVGate() {
		memset((byte*)m_cv,0,sizeof m_cv);
		memset((byte*)m_dac,0,sizeof m_dac);
		memset((byte*)m_gate,0,sizeof m_gate);
		m_cv_pending = 0;
		m_gate_pending = 0;
	}


	/////////////////////////////////////////////////////////////////////////////////
	void gate(byte which, GATE_STATE gate) {
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
					g_gate_led.blink(g_gate_led.MEDIUM_BLINK);
					break;
				case GATE_OPEN:
					m_gate[which] = GATE_OPEN;
					m_gate_pending = 1;
					g_gate_led.blink(g_gate_led.MEDIUM_BLINK);
					break;
			}
		}
	}

	/////////////////////////////////////////////////////////////////////////////////
	void pitch_cv(int which, int note, V_SQL_CVSCALE scaling) {

		// convert the note to DAC value
		// TODO support other note/cv mappings
		int dac = (500 * (int)note)/12;
		set_cv(which, dac);
	}

	void set_cv(int which, int dac)
	{
		// get the appropriate offset into the DAC structure
		// for the target CV output
		int ofs;
		switch(which) {
		case 1: ofs = 1; break;
		case 2: ofs = 2; break;
		case 3: ofs = 0; break;
		default: ofs = 3; break;
		}

		// if the output has changed then load the new one
		if(m_dac[ofs] != dac) {
			g_cv_led.blink(g_cv_led.MEDIUM_BLINK);
			m_dac[ofs] = dac;
			m_cv_pending = 1;
		}
	}

	/////////////////////////////////////////////////////////////////////////////////
	void mod_cv(int which, int value, int volt_range, int value2, int sweep_time) {
		m_cv[which].pitch = (500*volt_range*value)<<9; // divide by 128 then left shift by 16
		if(sweep_time) {
			m_cv[which].target = (500*volt_range*value2)<<9;
			m_cv[which].glide_rate = (m_cv[which].target - m_cv[which].pitch)/sweep_time;
		}
		else {
			m_cv[which].glide_rate = 0;
		}

		int dac = m_cv[which].pitch>>16;
		set_cv(which, dac);
	}

	/////////////////////////////////////////////////////////////////////////////////
	void service() {

		// if we have CV data to send and the bus is clear
		// then send out the CV data to DAC
		if(!g_i2c_bus.busy()) {
			if(m_cv_pending) {
				g_i2c_bus.dac_write(m_dac);
				m_cv_pending = 0;
			}
		}
		if(m_gate_pending) { // rising edge pending?
			for(int i=0; i<4; ++i) {
				if(m_gate[i] == GATE_OPEN) {
					gate_on(i);
				}
			}
			m_gate_pending = 0;
		}
	//TODO gate sync with CV in note mode
	}


	/////////////////////////////////////////////////////////////////////////////////
	// called once per ms
	void run() {
		for(int i=0; i<MAX_CV; ++i) {
			if(m_cv[i].glide_rate) {
				m_cv[i].pitch += m_cv[i].glide_rate;
				if(m_cv[i].glide_rate < 0) {
					if(m_cv[i].pitch <= m_cv[i].target) {
						m_cv[i].pitch = m_cv[i].target;
						m_cv[i].glide_rate = 0;
					}
				}
				else {
					if(m_cv[i].pitch >= m_cv[i].target) {
						m_cv[i].pitch = m_cv[i].target;
						m_cv[i].glide_rate = 0;
					}
				}
				int dac = m_cv[i].pitch>>16;
				set_cv(i, dac);
			}
		}
	}

};

// define global instance of the CV/Gate controller
CCVGate g_cv_gate;


#endif /* CV_GATE_H_ */
