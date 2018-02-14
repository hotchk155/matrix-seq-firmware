/*
 * sequence.h
 *
 *  Created on: 11 Feb 2018
 *      Author: jason
 */

#ifndef SEQUENCE_H_
#define SEQUENCE_H_


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
	CSequence() {
		clear();
		m_play_pos = 0;
		m_loop_from = 0;
		m_loop_to = 16;
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
	void tick() {
		static int c = 0;
		if(++c >= 100) {
			c = 0;
			++m_play_pos;
			if(m_play_pos < m_loop_from) {
				m_play_pos = m_loop_from;
			}
			else if(m_play_pos > m_loop_to) {
				m_play_pos = m_loop_from;
			}
		}
	}
	void test() {
		notes[0].note = 5;
		notes[1].note = 5;
		notes[4].note = 6;
		notes[5].note = 8;
		notes[8].note = 0;
		notes[9].note = 0;
		notes[12].note = 12;
	}
};

#endif /* SEQUENCE_H_ */
