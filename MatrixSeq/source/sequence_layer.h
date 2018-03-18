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
		IS_TRIG = (uint16_t)0x200, 	// is step a trigger
		MAX_PLAYING_NOTES = 8,
	};

	// look up table of tick rates
	static const byte c_tick_rates[V_SQL_STEP_RATE_MAX];
	static const byte c_step_duration[V_SQL_STEP_DUR_MAX];


	typedef struct {
		byte note;
		byte count;
	} PLAYING_NOTE;
	PLAYING_NOTE m_playing[MAX_PLAYING_NOTES];

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
		V_SQL_STEP_DUR	m_note_dur;
		byte 			m_midi_vel;			// MIDI velocity
		V_SQL_VEL_MOD	m_midi_vel_mod;		// MIDI velocity modulation
		V_SQL_MIDI_CHAN m_midi_channel;		// MIDI channel
		byte 			m_midi_cc;			// MIDI CC

	} CONFIG;



	CONFIG m_cfg;				// instance of config
	byte m_scroll_ofs;			// lowest step value shown on grid
	STEP_TYPE m_step_value;			// the last value output by sequencer
	byte m_stepped;				// stepped flag
	int m_play_pos;
	byte m_last_note_index;
	//uint32_t m_next_step_millis;
	uint32_t m_next_tick;
	byte m_last_tick_lsb;

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
		m_cfg.m_note_dur	= V_SQL_STEP_DUR_FULL;
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
		m_step_value = 0;
		m_stepped = 0;
		m_play_pos = 0;
		//m_last_midi_note = 0;
		//m_next_step_millis = 0;
		m_next_tick = 0;
		m_scroll_ofs = 48;
		m_last_tick_lsb = 0;
		memset(m_playing,0,sizeof(m_playing));
		m_last_note_index = 0xFF;
	}


	///////////////////////////////////////////////////////////////////////////////
	void set(PARAM_ID param, int value) {
		switch(param) {
		case P_SQL_SEQ_MODE: m_cfg.m_mode = (V_SQL_SEQ_MODE)value; break;
		case P_SQL_STEP_RATE: m_cfg.m_step_rate = (V_SQL_STEP_RATE)value; break;
		case P_SQL_STEP_DUR: m_cfg.m_note_dur = (V_SQL_STEP_DUR)value; break;
		case P_SQL_MIDI_CHAN: m_cfg.m_midi_channel = (V_SQL_MIDI_CHAN)value; break;
		case P_SQL_MIDI_CC: m_cfg.m_midi_cc = value; break;
		default: break;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	int get(PARAM_ID param) {
		switch(param) {
		case P_SQL_SEQ_MODE: return m_cfg.m_mode;
		case P_SQL_STEP_RATE: return m_cfg.m_step_rate;
		case P_SQL_STEP_DUR: return m_cfg.m_note_dur;
		case P_SQL_MIDI_CHAN: return m_cfg.m_midi_channel;
		case P_SQL_MIDI_CC: return m_cfg.m_midi_cc;
		default:return 0;
		}
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

	void start(uint32_t ticks, byte parts_tick) {
		m_next_tick = ticks;
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
			m_step_value = m_cfg.m_step[m_play_pos];
			m_stepped = 1;
		}
		else {
			m_stepped = 0;
		}

		// every whole tick, check the playing notes
		if(m_last_tick_lsb != (byte)ticks) {
			m_last_tick_lsb = (byte)ticks;
			for(int i=0; i<MAX_PLAYING_NOTES;++i) {
				if(m_playing[i].count) {
					if(!--m_playing[i].count) {
						g_midi.send_note(m_cfg.m_midi_channel, m_playing[i].note, 0);
					}
				}
			}
		}
		return m_stepped;
	}

	///////////////////////////////////////////////////////////////////////////////
	void start_note(byte note, byte velocity, byte duration, byte legato) {
		// if playing legato we replace the most recent note
		byte stop_note = 0;
		if(legato && m_last_note_index != 0xFF) {
			stop_note = m_playing[m_last_note_index].note;
		}
		else {
			int free = -1;
			int same = -1;
			int steal = -1;
			byte steal_count = 255;
			for(int i=0; i<MAX_PLAYING_NOTES;++i) {
				if(m_playing[i].note == note) {
					same = i;
					break;
				}
				if(!m_playing[i].count) {
					free = i;
					break;
				}
				if(m_playing[i].count < steal_count) {
					steal_count = m_playing[i].count;
					steal = i;
				}
			}
			if(same>=0) {
				m_last_note_index = same;
			}
			else if(free>=0) {
				m_last_note_index = free;
			}
			else if(steal>=0) {
				g_midi.send_note(m_cfg.m_midi_channel, m_playing[steal].note, 0);
				m_last_note_index = steal;
			}
			else {
				return;
			}
		}
		m_playing[m_last_note_index].note = note;
		m_playing[m_last_note_index].count = duration;
		g_midi.send_note(m_cfg.m_midi_channel, note, velocity);
		if(stop_note) {
			g_midi.send_note(m_cfg.m_midi_channel, stop_note, 0);
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	void stop_all_notes() {
		for(int i=0; i<MAX_PLAYING_NOTES;++i) {
			if(m_playing[i].count) {
				g_midi.send_note(m_cfg.m_midi_channel, m_playing[i].note, 0);
				m_playing[i].count = 0;
			}
		}
	}
	///////////////////////////////////////////////////////////////////////////////
	void play_step() {
		switch(m_cfg.m_mode) {
		case V_SQL_SEQ_MODE_CHROMATIC:
			if(m_step_value & IS_ACTIVE) {
				byte dur = (m_cfg.m_note_dur == V_SQL_STEP_DUR_FULL)? c_tick_rates[m_cfg.m_step_rate] : c_step_duration[m_cfg.m_note_dur];
				start_note((byte)m_step_value, m_cfg.m_midi_vel, dur, !(m_step_value & IS_TRIG));
			}
			break;
		default:
			break;
		}
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
const byte CSequenceLayer::c_step_duration[V_SQL_STEP_DUR_MAX] = {0,1,3,4,6,8,9,12,16,18,24,32,36,48,72,96};
#endif

#endif /* SEQUENCE_LAYER_H_ */
