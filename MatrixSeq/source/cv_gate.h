/*
 * cv.h
 *
 *  Created on: 14 Feb 2018
 *      Author: jason
 */

#ifndef CV_GATE_H_
#define CV_GATE_H_


#define BIT_GATE1		MK_GPIOA_BIT(PORTD_BASE, 2)
#define BIT_GATE2		MK_GPIOA_BIT(PORTD_BASE, 3)
#define BIT_GATE3		MK_GPIOA_BIT(PORTD_BASE, 4)
#define BIT_GATE4		MK_GPIOA_BIT(PORTA_BASE, 6)


class CCVGate {

	enum {
		MAX_CV = 4,
		MAX_GATE = 4
	};
	uint16_t m_dac[MAX_CV];
	uint16_t m_gate[MAX_GATE];


	byte m_pending;
public:
	enum {
		TXN_TRIG	= 0x01,	// generate rising edge trig
		TXN_GATE	= 0x02, // maintain gate
		TXN_CV		= 0x04,  // update CV
		TXN_PITCH	= 0x08  // interpret CV as a pitch value
	};
	typedef struct {
		byte flags[4];
		uint16_t cv[4];	// midi note * 256 with fractional part OR 16 bit scaled to full output V
	} TRANSACTION;


	// pitch is defined a 256 * midi note number + fractional part
	typedef uint16_t PITCH_TYPE;

	// gate type is defined as milliseconds. zero is infinite
	typedef uint16_t GATE_TYPE;

	// cv type is a 16 bit value to be mapped to the full voltage range
	typedef uint16_t CV_TYPE;

	typedef uint16_t DAC_TYPE;

	typedef struct {
		byte cv_range[4];
		byte cv_scale[4];
	} CONFIG;
	CONFIG m_cfg;

	CCVGate() {
		memset((byte*)m_dac,0,sizeof m_dac);
		memset((byte*)m_gate,0,sizeof m_gate);
		memset((byte*)m_cfg.cv_range,5,sizeof m_cfg.cv_range);
		memset((byte*)m_cfg.cv_scale,V_CVGATE_VSCALE_1VOCT,sizeof m_cfg.cv_scale);
		m_pending = 0;
	}





	void set(int which, PARAM_ID param, int value) {
		switch(param) {
			case P_CVGATE_VSCALE: m_cfg.cv_scale[which] = value; break;
			case P_CVGATE_VRANGE: m_cfg.cv_range[which] = value; break;
			default: break;
		}
	}
	int get(int which, PARAM_ID param) {
		switch(param) {
			case P_CVGATE_VSCALE: return m_cfg.cv_scale[which];
			case P_CVGATE_VRANGE: return m_cfg.cv_range[which];
			default: return 0;
		}
	}


	void dac_send() {
		if(m_pending && !g_i2c_bus.busy()) {
			g_i2c_bus.dac_write(m_dac);
			m_pending = 0;
		}
	}
	void dac_out(byte which, DAC_TYPE dac) {
		m_dac[which] = dac;
		m_pending = 1;
		dac_send();
	}

	void cv_out(byte which, CV_TYPE cv) {
	//	dac_out(which, CV_2_DAC(cv));
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
	void gate_on(byte which, GATE_TYPE dur) {
		gate_on(which);
		m_gate[which] = dur;
	}

	void note_out(byte which, PITCH_TYPE pitch, GATE_TYPE gate) {
//		dac_out(which, PITCH_2_DAC(pitch));
		// TODO SYNC
		gate_on(which, gate);
	}
	void run() {
		dac_send();
		for(int i=0; i<MAX_GATE; ++i) {
			if(m_gate[i]) {
				if(!--m_gate[i]) {
					gate_off(i);
				}
			}
		}
	}

	void set(TRANSACTION& txn) {

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
