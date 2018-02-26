/*
 * sequence.h
 *
 *  Created on: 11 Feb 2018
 *      Author: jason
 */

#ifndef SEQUENCE_H_
#define SEQUENCE_H_

#include "seq_clock.h"

typedef struct {
	byte note;	// MIDI note
	byte vel;	// MIDI velocity
} SEQ_NOTE;

typedef struct {
	byte value;
} SEQ_MOD;


#define SEQ_MAX_STEPS 32


class CSequence {
public:
	enum {
		NO_NOTE = 255,
		NO_POS = -1
	};
	SEQ_NOTE notes[SEQ_MAX_STEPS];
	SEQ_MOD mod[SEQ_MAX_STEPS];
	int m_play_pos;
	int m_loop_from;
	int m_loop_to;
	byte m_last_note;

	CSeqClock::TICKS_TYPE m_next_tick;
	CSeqClock::TICKS_TYPE m_rate;
	CSequence() {
		clear();
		m_play_pos = 0;
		m_loop_from = 0;
		m_loop_to = 16;
		m_last_note = NO_NOTE;
		m_next_tick = 0;
		m_rate =CSeqClock::RATE_16;
	}
	void clear() {
		for(int i=0; i<SEQ_MAX_STEPS; ++i) {
			notes[i].note = NO_NOTE;
			notes[i].vel = 0;
			mod[i].value= 0;
		}
	}
	void set_loop_start(int pos) {
		if(pos <= m_loop_to) {
			m_loop_from = pos;
			m_play_pos = pos;
		}
	}
	void set_loop_end(int pos) {
		if(pos >= m_loop_from) {
			m_loop_to = pos;
		}
	}
	void set_pos(int pos) {
		m_play_pos = pos;
	}

	// tell the sequencer to move to the next step
	// return value is nonzero if the end of the loop
	// has been reached
	byte step() {
		++m_play_pos;
		if(m_play_pos < m_loop_from) {
			m_play_pos = m_loop_from;
		}
		else if(m_play_pos > m_loop_to) {
			m_play_pos = m_loop_from;
		}
		if(m_last_note != NO_NOTE) {
			fire_note(m_last_note, 0);
			m_last_note = NO_NOTE;
		}
		if(notes[m_play_pos].note != NO_NOTE) {
			m_last_note = notes[m_play_pos].note;
			fire_note(m_last_note, 127);
		}
	}
	void run() {

	}
	void test() {
		notes[0].note = 45;
		notes[1].note = 45;
		notes[4].note = 46;
		notes[5].note = 48;
		notes[8].note = 50;
		notes[9].note = 50;
		notes[12].note = 52;
	}
};

#endif /* SEQUENCE_H_ */
