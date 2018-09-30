/*
 * sequence_step.h
 *
 *  Created on: 30 Sep 2018
 *      Author: jason
 */

#ifndef SEQUENCE_STEP_H_
#define SEQUENCE_STEP_H_


class CStep {
public:
	byte m_is_data_point:1; // is this a "user" data point rather than an automatic one?
	byte m_is_accent:1;
	byte m_is_gate_open:1;
	byte m_is_trigger:1;
	byte m_value;

	void copy_data_point(CStep &other) {
		m_value = other.m_value;
		m_is_data_point = other.m_is_data_point;
	}

	// clear data point and gates
	void reset_all(byte value = 0) {
		m_is_data_point = 0;
		m_is_accent = 0;
		m_is_gate_open = 0;
		m_is_trigger = 0;
		m_value = value;
	}

	// clear data point but preserve gate
	void reset_data_point(byte value = 0) {
		m_is_data_point = 0;
		m_value = value;
	}

	// clear gate info but preserve data point
	void reset_gate() {
		m_is_accent = 0;
		m_is_gate_open = 0;
		m_is_trigger = 0;

	}
};



#endif /* SEQUENCE_STEP_H_ */
