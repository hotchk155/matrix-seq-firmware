///////////////////////////////////////////////////////////////////////////////
// MATRIX SEQUENCER
// Sixty four pixels Ltd	March 2018
//
// SEQUENCER LAYER
#ifndef SEQUENCE_LAYER_H_
#define SEQUENCE_LAYER_H_

#include "matrix_seq.h"

#define STEP_VALUE(s) ((byte)(s))

///////////////////////////////////////////////////////////////////////////////
// This class defines a single layer of a sequence
class CSequenceLayer {

public:
	typedef uint16_t STEP_TYPE;
	enum {
		NO_POS = -1,
		MAX_STEPS = 32,					// number of steps in the layer
		IS_ACTIVE = (uint16_t)0x100,	// is step active
		IS_TRIG = (uint16_t)0x200 	// is step a trigger
	};

	// look up table of tick rates
	static const byte c_tick_rates[V_SQL_STEP_RATE_MAX];

	// step word contains both gate and CV info
	// CV info is 1 byte. Interpretation depends on mode. Always 1 unit per grid cell
	//
	// V_SQL_SEQ_MODE_NOTE 			- 0-127 MIDI note
	// V_SQL_SEQ_MODE_NOTE_SCALE	- 0-127 scale increment
	// V_SQL_SEQ_MODE_MOD 			- 0-13 coarse CC value
	// V_SQL_SEQ_MODE_MOD_FINE		- 0-127 CC value



	// This structure holds the layer information that gets saved with the patch
	typedef struct {
		V_SQL_SEQ_MODE 	m_mode;				// the mode for this layer (note, mod etc)
		STEP_TYPE		m_step[MAX_STEPS];	// data value and gate for each step
		V_SQL_STEP_RATE m_step_rate;		// step rate setting
		byte 			m_loop_from;		// loop start point
		byte 			m_loop_to;			// loop end point
		char			m_transpose;		// manual transpose amount for the layer
		V_SQL_TRANSPOSE_MOD	m_transpose_mod;	// automatic transpose source for the layer

		byte 			m_midi_vel;			// MIDI velocity
		V_SQL_VEL_MOD	m_midi_vel_mod;		// MIDI velocity modulation
		V_SQL_MIDI_CHAN m_midi_channel;		// MIDI channel
		byte 			m_midi_cc;			// MIDI CC

	} CONFIG;


	CONFIG m_cfg;				// instance of config
	byte m_scroll_ofs;			// lowest step value shown on grid
	uint16_t m_value;			// the last value output by sequencer
	byte m_stepped;				// stepped flag
	int m_play_pos;
	byte m_last_midi_note;
	uint32_t m_next_step_millis;
	uint32_t m_next_tick;

	///////////////////////////////////////////////////////////////////////////////
	CSequenceLayer() {
		init_config();
		init_state();
	}

	///////////////////////////////////////////////////////////////////////////////
	void init_config() {
		for(int i=0; i<MAX_STEPS; ++i) {
			m_cfg.m_step[i] = 0;
		}
		m_cfg.m_mode 		= V_SQL_SEQ_MODE_CHROMATIC;
		m_cfg.m_step_rate	= V_SQL_STEP_RATE_16;
		m_cfg.m_loop_from	= 0;
		m_cfg.m_loop_to		= 15;
		m_cfg.m_transpose	= 0;
		m_cfg.m_transpose_mod = V_SQL_TRANSPOSE_MOD_OFF;
		m_cfg.m_midi_vel	= 100;
		m_cfg.m_midi_vel_mod = V_SQL_VEL_MOD_OFF;
		m_cfg.m_midi_channel = V_SQL_MIDI_CHAN_1;
		m_cfg.m_midi_cc = 1;
	}

	///////////////////////////////////////////////////////////////////////////////
	void init_state() {
		m_value = 0;
		m_stepped = 0;
		m_play_pos = 0;
		m_last_midi_note = 0;
		m_next_step_millis = 0;
		m_next_tick = 0;
		m_scroll_ofs = 48;
	}

	///////////////////////////////////////////////////////////////////////////////
	void set_loop_start(int pos) {
		if(pos <= m_cfg.m_loop_to) {
			m_cfg.m_loop_from = pos;
			m_play_pos = pos;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	void set_loop_end(int pos) {
		if(pos >= m_cfg.m_loop_from) {
			m_cfg.m_loop_to = pos;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	void set_pos(int pos) {
		m_play_pos = pos;
	}

	///////////////////////////////////////////////////////////////////////////////
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

	void inc_step(STEP_TYPE *value, int delta) {
		int v = (byte)(*value) + delta;
		switch(m_cfg.m_mode) {
		case V_SQL_SEQ_MODE_MOD:
			if(v < 0) {
				v = 0;
			}
			else if(v > 13) {
				v = 13;
			}
			break;
		case V_SQL_SEQ_MODE_CHROMATIC:
		case V_SQL_SEQ_MODE_CHROMATIC_FORCED:
		case V_SQL_SEQ_MODE_MOD_FINE:
		default:
			if(v < 0) {
				v = 0;
			}
			else if(v > 127) {
				v = 127;
			}
			break;
		}
		*value &= 0xFF00;
		*value |= v;
	}
	///////////////////////////////////////////////////////////////////////////////
	inline byte is_mod_mode() {
		return(m_cfg.m_mode == V_SQL_SEQ_MODE_MOD || m_cfg.m_mode == V_SQL_SEQ_MODE_MOD_FINE);
	}

	///////////////////////////////////////////////////////////////////////////////
	inline byte is_note_mode() {
		return(m_cfg.m_mode == V_SQL_SEQ_MODE_CHROMATIC || m_cfg.m_mode == V_SQL_SEQ_MODE_CHROMATIC_FORCED);
	}

	///////////////////////////////////////////////////////////////////////////////
	inline void clear_step(int index) {
		m_cfg.m_step[index] = 0;
	}

	///////////////////////////////////////////////////////////////////////////////
	inline uint16_t get_step(int index) {
		return m_cfg.m_step[index];
	}

	///////////////////////////////////////////////////////////////////////////////
	inline void set_step(int index, uint16_t step) {
		m_cfg.m_step[index] =step;
	}

	///////////////////////////////////////////////////////////////////////////////
	void test() {
		m_cfg.m_step[0] = 45|IS_TRIG|IS_ACTIVE;
		m_cfg.m_step[1] = 45|IS_TRIG|IS_ACTIVE;
		m_cfg.m_step[4] = 46|IS_TRIG|IS_ACTIVE;
		m_cfg.m_step[5] = 48|IS_TRIG|IS_ACTIVE;
		m_cfg.m_step[8] = 50|IS_TRIG|IS_ACTIVE;
		m_cfg.m_step[9] = 50|IS_TRIG|IS_ACTIVE;
		m_cfg.m_step[12] = 52|IS_TRIG|IS_ACTIVE;
	}
};



#ifdef MAIN_INCLUDE
const byte CSequenceLayer::c_tick_rates[V_SQL_STEP_RATE_MAX] = {96,72,48,36,32,24,18,16,12,9,8,6,4,3};
#endif

#endif /* SEQUENCE_LAYER_H_ */
