/*
 * cv.h
 *
 *  Created on: 14 Feb 2018
 *      Author: jason
 */

#ifndef CV_GATE_H_
#define CV_GATE_H_

class CCVGate {
	enum {
		MAX_CV = 4
	};
	uint16_t m_cv[MAX_CV];
	byte m_pending;
public:
	CCVGate() {
		memset((byte*)m_cv,0,sizeof m_cv);
		m_pending = 0;
	}
	void run() {
		if(m_pending && !g_i2c_bus.busy()) {
			g_i2c_bus.dac_write(m_cv);
			m_pending = 0;
		}
	}
	void write(byte index, uint16_t value) {
		m_cv[index] = value;
		m_pending = 1;
	}
};



#endif /* CV_GATE_H_ */
