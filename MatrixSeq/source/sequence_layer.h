/*
 * sequence.h
 *
 *  Created on: 11 Feb 2018
 *      Author: jason
 */

#ifndef SEQUENCE_LAYER_H_
#define SEQUENCE_LAYER_H_

#include "matrix_seq.h"

#define SEQ_STEP(a) ((a) & 0x7F)
#define SEQ_GATE(a) ((a) & 0x80)

// This class defines a single layer of a sequence
class CSequenceLayer {

public:
/*	enum {
		NOTE_SEQUENCE,	// The sequence contains notes
		MOD_SEQUENCE,	// The sequence contains modulation
		VELOCITY_SEQUENCE,	// The sequence contains velocity information
		TRANSPOSE_SEQUENCE	// The sequence contains transposition information
	};*/
	enum {
		NO_POS = -1,
		MAX_STEPS = 32,
		IS_ACTIVE = (uint16_t)0x10000,	// is step active
		IS_TRIG = (uint16_t)0x20000 	// is step a trigger
	};

	// information that gets storedin the patch
	typedef struct {
		byte m_mode;
		uint16_t m_step[MAX_STEPS];	// note step
		byte m_step_rate;
		byte m_midi_channel;
		byte m_midi_cc;
		byte m_loop_from;
		byte m_loop_to;
	} CONFIG;

	static const byte c_tick_rates[V_SQL_STEP_RATE_MAX];

	CONFIG m_cfg;

	uint16_t m_value;			// the last value output by sequencer
	byte m_stepped;
	int m_play_pos;
	byte m_last_midi_note;
	uint32_t m_next_step_millis;

	uint32_t m_next_tick;

	CSequenceLayer() {
		clear();
		m_play_pos = 0;
		m_cfg.m_loop_from = 0;
		m_cfg.m_loop_to = 15;
		m_cfg.m_midi_channel = 0;
		m_cfg.m_midi_cc = 1;
		m_last_midi_note = 0;
		m_value = 0;
		m_next_step_millis = 0;
		m_cfg.m_mode = V_SQL_SEQ_MODE_NOTE;
		m_stepped = 0;
		m_next_tick = 0;
		m_cfg.m_step_rate = V_SQL_STEP_RATE_16;
	}

	void clear() {
		for(int i=0; i<MAX_STEPS; ++i) {
			m_cfg.m_step[i] = 0;
		}
	}
	void set_loop_start(int pos) {
		if(pos <= m_cfg.m_loop_to) {
			m_cfg.m_loop_from = pos;
			m_play_pos = pos;
		}
	}
	void set_loop_end(int pos) {
		if(pos >= m_cfg.m_loop_from) {
			m_cfg.m_loop_to = pos;
		}
	}
	void set_pos(int pos) {
		m_play_pos = pos;
	}

	byte tick(uint32_t ticks, byte parts_tick) {
		if(ticks >= m_next_tick) {
			m_next_tick += c_tick_rates[m_cfg.m_step_rate];
			++m_play_pos;
			if(m_play_pos < m_cfg.m_loop_from) {
				m_play_pos = m_cfg.m_loop_from;
			}
			else if(m_play_pos > m_cfg.m_loop_to) {
				m_play_pos = m_cfg.m_loop_from;
			}
			m_value = m_cfg.m_step[m_play_pos];
			m_stepped = 1;
		}
		else {
			m_stepped = 0;
		}
		return m_stepped;
	}
	void clear_step(int index) {
		m_cfg.m_step[index] = 0;
	}
	uint16_t get_step(int index) {
		return m_cfg.m_step[index];
	}
	void set_step(int index, uint16_t step) {
		m_cfg.m_step[index] =step;
	}
	/*byte get_gate(int index) {
		return m_step[index] & GATE_BIT;
	}
	void set_gate(int index, int gate) {
		if(gate) {
			m_step[index] |= GATE_BIT;
		}
		else {
			m_step[index] &= ~GATE_BIT;
		}
	}*/
	void run() {

	}
	byte force_to_scale(int note) {
		// TODO
		while(note < 0) note += 12;
		while(note > 127) note -= 12;
		return note;
	}
	void test() {
		m_cfg.m_step[0] = 45|IS_TRIG;
		m_cfg.m_step[1] = 45|IS_TRIG;
		m_cfg.m_step[4] = 46|IS_TRIG;
		m_cfg.m_step[5] = 48|IS_TRIG;
		m_cfg.m_step[8] = 50|IS_TRIG;
		m_cfg.m_step[9] = 50|IS_TRIG;
		m_cfg.m_step[12] = 52|IS_TRIG;
	}
};



#ifdef MAIN_INCLUDE
const byte CSequenceLayer::c_tick_rates[V_SQL_STEP_RATE_MAX] = {96,72,48,36,32,24,18,16,12,9,8,6,4,3};
#endif

#endif /* SEQUENCE_LAYER_H_ */
