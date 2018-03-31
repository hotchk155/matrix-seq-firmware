///////////////////////////////////////////////////////////////////////////////
// MATRIX SEQUENCER
// Sixty four pixels Ltd	March 2018
//
// SEQUENCER LAYER
#ifndef SEQUENCE_LAYER_H_
#define SEQUENCE_LAYER_H_

#include "matrix_seq.h"

#define STEP_VALUE(s) ((byte)(s))

class CSequencer;
///////////////////////////////////////////////////////////////////////////////
// This class defines a single layer of a sequence
class CSequenceLayer {

public:
	typedef uint16_t STEP_TYPE;
	enum {
		NO_POS = -1,
		MAX_STEPS = 32,					// number of steps in the layer
		IS_ACTIVE = (uint16_t)0x100,	// is step active
		IS_VEL0 = (uint16_t)0x200, 	// velocity type bit 0
		IS_VEL1 = (uint16_t)0x400, 	// velocity type bit 1
		MAX_PLAYING_NOTES = 8,
		REFERENCE_NOTE = 48,
		DEFAULT_NOTE = 36,
		MAX_MOD_VALUE = 12
	};

	enum {
		VELOCITY_OFF,
		VELOCITY_LEGATO,
		VELOCITY_LOW,
		VELOCITY_MEDIUM,
		VELOCITY_HIGH
	};

	// look up table of tick rates
	static const byte c_tick_rates[V_SQL_STEP_RATE_MAX];
	static const byte c_step_duration[V_SQL_STEP_DUR_MAX];

	static const STEP_TYPE VEL_MASK = (IS_ACTIVE|IS_VEL0|IS_VEL1);
	static const STEP_TYPE VEL_LEGATO = (IS_ACTIVE);
	static const STEP_TYPE VEL_LOW = (IS_ACTIVE|IS_VEL0);
	static const STEP_TYPE VEL_MEDIUM = (IS_ACTIVE|IS_VEL1);
	static const STEP_TYPE VEL_HIGH = (IS_ACTIVE|IS_VEL0|IS_VEL1);

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
		V_SQL_FORCE_SCALE	m_force_scale;	// force to scale
		STEP_TYPE		m_step[MAX_STEPS];	// data value and gate for each step
		V_SQL_STEP_RATE m_step_rate;		// step rate setting
		byte 			m_loop_from;		// loop start point
		byte 			m_loop_to;			// loop end point
		char			m_transpose;		// manual transpose amount for the layer
		V_SQL_TRANSPOSE_MOD	m_transpose_mod;	// automatic transpose source for the layer
		V_SQL_STEP_DUR	m_note_dur;
//		V_SQL_VEL_MOD	m_midi_vel_mod;		// MIDI velocity modulation
		byte 			m_enabled;
		V_SQL_MIDI_CHAN m_midi_channel;		// MIDI channel
		byte 			m_midi_cc;			// MIDI CC
	} CONFIG;


	typedef struct {
		byte note;
		byte count;
	} PLAYING_NOTE;



	typedef struct {
		byte m_scroll_ofs;			// lowest step value shown on grid
		STEP_TYPE m_step_value;			// the last value output by sequencer
		byte m_stepped;				// stepped flag
		int m_play_pos;
		byte m_last_note; // last midi note played on channel - and output to CV
		byte m_last_velocity;
		uint32_t m_next_tick;
		byte m_last_tick_lsb;
		PLAYING_NOTE m_playing[MAX_PLAYING_NOTES];
		byte m_gate;
//		byte m_reference;
	} STATE;

	CONFIG m_cfg;				// instance of config
	STATE m_state;
	///////////////////////////////////////////////////////////////////////////////
	CSequenceLayer() {
		init_config();
		init_state();
	}

	///////////////////////////////////////////////////////////////////////////////
	void init_config() {
		for(int i=0; i<MAX_STEPS; ++i) {
			m_cfg.m_step[i] = DEFAULT_NOTE;
		}
		m_cfg.m_mode 		= V_SQL_SEQ_MODE_SCALE;
		m_cfg.m_force_scale = V_SQL_FORCE_SCALE_OFF;
		m_cfg.m_step_rate	= V_SQL_STEP_RATE_16;
		m_cfg.m_note_dur	= V_SQL_STEP_DUR_FULL;
		m_cfg.m_loop_from	= 0;
		m_cfg.m_loop_to		= 15;
		m_cfg.m_transpose	= 0;
		m_cfg.m_transpose_mod = V_SQL_TRANSPOSE_MOD_OFF;
		m_cfg.m_midi_channel 	= V_SQL_MIDI_CHAN_1;
		m_cfg.m_midi_cc = 1;
		m_cfg.m_enabled = 1;
	}

	///////////////////////////////////////////////////////////////////////////////
	void init_state() {
		m_state.m_scroll_ofs = 48;
		m_state.m_last_tick_lsb = 0;
		m_state.m_last_note = 0;
		m_state.m_gate = CCVGate::GATE_CLOSED;
		//m_state.m_reference = 0;
		memset(m_state.m_playing,0,sizeof(m_state.m_playing));
		reset();
	}
	void reset() {
		m_state.m_step_value = 0;
		m_state.m_stepped = 0;
		m_state.m_play_pos = 0;
		m_state.m_next_tick = 0;
	}

	void copy_from(const CSequenceLayer& layer) {
		CONFIG cfg = m_cfg;

		m_cfg = layer.m_cfg;
		m_cfg.m_enabled = cfg.m_enabled;
		m_cfg.m_midi_channel = cfg.m_midi_channel;
		m_cfg.m_midi_cc = cfg.m_midi_cc;

		m_state = layer.m_state;
		m_state.m_last_note = 0;
		memset(m_state.m_playing,0,sizeof(m_state.m_playing));


	}

	///////////////////////////////////////////////////////////////////////////////
	void set(PARAM_ID param, int value) {
		switch(param) {
		case P_SQL_SEQ_MODE: set_mode((V_SQL_SEQ_MODE)value); break;
		case P_SQL_FORCE_SCALE: m_cfg.m_force_scale = (V_SQL_FORCE_SCALE)value; break;
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
		case P_SQL_FORCE_SCALE: return m_cfg.m_force_scale;
		case P_SQL_STEP_RATE: return m_cfg.m_step_rate;
		case P_SQL_STEP_DUR: return m_cfg.m_note_dur;
		case P_SQL_MIDI_CHAN: return m_cfg.m_midi_channel;
		case P_SQL_MIDI_CC: return m_cfg.m_midi_cc;
		default:return 0;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	int is_valid_param(PARAM_ID param) {
		switch(param) {
		case P_SQL_FORCE_SCALE: return !(m_cfg.m_mode == V_SQL_SEQ_MODE_MOD);
		case P_SQL_MIDI_CC: return !!(m_cfg.m_mode == V_SQL_SEQ_MODE_MOD);
		default: return 1;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	// preserve trigs but clear all data
	void reset_values(byte value) {
		for(int i=0; i<MAX_STEPS; ++i) {
			m_cfg.m_step[i] &= 0xFF00;
			m_cfg.m_step[i] |= value;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	void set_mode(V_SQL_SEQ_MODE value) {
		switch (value) {
		case V_SQL_SEQ_MODE_CHROMATIC:
			if(m_cfg.m_mode == V_SQL_SEQ_MODE_SCALE) {
				// changing from short scale to chromatic mode...
				for(int i=0; i<MAX_STEPS; ++i) {
					byte value = m_cfg.m_step[i] & 0xFF;
					value = g_scale.index_to_note(value);
					m_cfg.m_step[i] &= 0xFF00;
					m_cfg.m_step[i] |= value;
				}
				m_state.m_scroll_ofs = g_scale.index_to_note(m_state.m_scroll_ofs);
			}
			else {
				reset_values(DEFAULT_NOTE);
				m_state.m_scroll_ofs = DEFAULT_NOTE - 5;
			}
			break;

		case V_SQL_SEQ_MODE_SCALE:
			if(m_cfg.m_mode == V_SQL_SEQ_MODE_CHROMATIC) {
				// changing from chromatic to short scale mode...
				for(int i=0; i<MAX_STEPS; ++i) {
					byte value = m_cfg.m_step[i] & 0xFF;
					value = g_scale.note_to_index(value);
					m_cfg.m_step[i] &= 0xFF00;
					m_cfg.m_step[i] |= value;
				}
				m_state.m_scroll_ofs = g_scale.note_to_index(m_state.m_scroll_ofs);
			}
			else {
				int default_index = g_scale.note_to_index(DEFAULT_NOTE);
				reset_values(default_index);
				m_state.m_scroll_ofs = default_index - 5;
			}
			break;

		case V_SQL_SEQ_MODE_TRANSPOSE:
			reset_values(64);
			m_state.m_scroll_ofs = 60;
			break;

		case V_SQL_SEQ_MODE_MOD:
			reset_values(0);
			break;
		default:
			break;
		}
		m_cfg.m_mode = value;
	}

	///////////////////////////////////////////////////////////////////////////////
	void set_loop_start(int pos) {
		if(pos <= m_cfg.m_loop_to) {
			m_cfg.m_loop_from = pos;
			m_state.m_play_pos = pos;
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
		m_state.m_play_pos = pos;
	}

	void start(uint32_t ticks, byte parts_tick) {
		m_state.m_next_tick = ticks;
	}

	///////////////////////////////////////////////////////////////////////////////
	void tick(uint32_t ticks, byte parts_tick) {
		if(ticks >= m_state.m_next_tick) {
			m_state.m_next_tick += c_tick_rates[m_cfg.m_step_rate];
			if(m_state.m_play_pos == m_cfg.m_loop_to) {
				m_state.m_play_pos = m_cfg.m_loop_from;
			}
			else {
				if(++m_state.m_play_pos > MAX_STEPS-1) {
					m_state.m_play_pos = 0;
				}
			}

			/*
			if(m_state.m_play_pos < m_cfg.m_loop_from) {
				m_state.m_play_pos = m_cfg.m_loop_from;
			}
			else if(m_state.m_play_pos > m_cfg.m_loop_to) {
				m_state.m_play_pos = m_cfg.m_loop_from;
			}*/
			m_state.m_step_value = m_cfg.m_step[m_state.m_play_pos];
			m_state.m_stepped = 1;
		}
		else {
			m_state.m_stepped = 0;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	void manage(uint32_t ticks) {
		// has a tick expired?
		if(m_state.m_last_tick_lsb != (byte)ticks) {
			m_state.m_last_tick_lsb = (byte)ticks;
			for(int i=0; i<MAX_PLAYING_NOTES;++i) {
				if(m_state.m_playing[i].count) {
					if(!--m_state.m_playing[i].count) {
						if(m_state.m_playing[i].note == m_state.m_last_note) {
							// close the gate
							m_state.m_gate = CCVGate::GATE_CLOSED;
						}
						send_midi_note(m_state.m_playing[i].note, 0);
						m_state.m_playing[i].note = 0;
					}
				}
			}
		}
	}


	///////////////////////////////////////////////////////////////////////////////
	void send_midi_note(byte note, byte velocity) {
		if(m_cfg.m_midi_channel > V_SQL_MIDI_CHAN_NONE) {
			g_midi.send_note(m_cfg.m_midi_channel-V_SQL_MIDI_CHAN_1, note, velocity);
		}
	}


	///////////////////////////////////////////////////////////////////////////////
	void stop_all_notes() {
		for(int i=0; i<MAX_PLAYING_NOTES;++i) {
			if(m_state.m_playing[i].count) {
				send_midi_note(m_state.m_playing[i].note, 0);
				m_state.m_playing[i].note = 0;
				m_state.m_playing[i].count = 0;
			}
		}
	}


	static int get_velocity(const STEP_TYPE& step) {
		switch(step & VEL_MASK) {
		case VEL_LEGATO: return VELOCITY_LEGATO;
		case VEL_LOW: return VELOCITY_LOW;
		case VEL_MEDIUM: return VELOCITY_MEDIUM;
		case VEL_HIGH: return VELOCITY_HIGH;
		}
		return VELOCITY_OFF;
	}
	static void set_velocity(STEP_TYPE& step, int value) {
		step &= ~VEL_MASK;
		switch(value) {
		case VELOCITY_OFF: break;
		case VELOCITY_LEGATO: step |= VEL_LEGATO; break;
		case VELOCITY_LOW: step |= VEL_LOW; break;
		case VELOCITY_MEDIUM: step |= VEL_MEDIUM; break;
		case VELOCITY_HIGH: step |= VEL_HIGH; break;
		}
	}

	void inc_step(STEP_TYPE *value, int delta) {
		int v = (byte)(*value) + delta;
		int max_note = 127;
		switch(m_cfg.m_mode) {
		case V_SQL_SEQ_MODE_MOD:
			if(v < 0) {
				v = 0;
			}
			else if(v > MAX_MOD_VALUE) {
				v = MAX_MOD_VALUE;
			}
			break;
		case V_SQL_SEQ_MODE_TRANSPOSE:
			if(v < 0) {
				v = 0;
			}
			else if(v > 127) {
				v = 127;
			}
			break;
		case V_SQL_SEQ_MODE_SCALE:
			max_note = g_scale.max_index();
		case V_SQL_SEQ_MODE_CHROMATIC:
		default:
			if(v < 0) {
				v = 0;
			}
			else if(v > max_note) {
				v = max_note;
			}
			break;
		}
		*value &= 0xFF00;
		*value |= v;
	}
	///////////////////////////////////////////////////////////////////////////////
	inline byte is_mod_mode() {
		return(m_cfg.m_mode == V_SQL_SEQ_MODE_MOD);
	}

	///////////////////////////////////////////////////////////////////////////////
	inline byte is_note_mode() {
		return(m_cfg.m_mode == V_SQL_SEQ_MODE_CHROMATIC || m_cfg.m_mode == V_SQL_SEQ_MODE_SCALE);
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

	byte vertical_move(int offset) {
		for(int i = 0; i<MAX_STEPS; ++i) {
			STEP_TYPE step = m_cfg.m_step[i];
			if(step & IS_ACTIVE) {
				int v = (int)STEP_VALUE(step) + offset;
				if(v < 0 || v > 127) {
					return 0;
				}
			}
		}
		for(int i = 0; i<MAX_STEPS; ++i) {
			STEP_TYPE step = m_cfg.m_step[i];
			if(step & IS_ACTIVE) {
				m_cfg.m_step[i] = (step & 0xFF00) | (STEP_VALUE(step) + offset);
			}
		}
		return 1;
	}
	void shift_left() {
		STEP_TYPE step = m_cfg.m_step[0];
		for(int i = 0; i<MAX_STEPS-1; ++i) {
			m_cfg.m_step[i] = m_cfg.m_step[i+1];
		}
		m_cfg.m_step[MAX_STEPS-1] = step;
	}
	void shift_right() {
		STEP_TYPE step = m_cfg.m_step[MAX_STEPS-1];
		for(int i = MAX_STEPS-1; i>0; --i) {
			m_cfg.m_step[i] = m_cfg.m_step[i-1];
		}
		m_cfg.m_step[0] = step;
	}

	///////////////////////////////////////////////////////////////////////////////
	byte get_graticule(int *baseline, int *spacing) {
		int n;
		switch (m_cfg.m_mode) {
		case V_SQL_SEQ_MODE_CHROMATIC:
			n = m_state.m_scroll_ofs + 15; // note at top row of screen
			n = 12*(n/12); // C at bottom of that octave
			*baseline = 12 - n + m_state.m_scroll_ofs; // now take scroll offset into account
			*spacing = 12;
			return 1;
		case V_SQL_SEQ_MODE_SCALE:
			n = m_state.m_scroll_ofs + 15; // note at top row of screen
			n = 7*(n/7); // C at bottom of that octave
			*baseline = 12 - n + m_state.m_scroll_ofs; // now take scroll offset into account
			*spacing = 7;
			return 1;
		case V_SQL_SEQ_MODE_TRANSPOSE:
			*baseline = 64;
			*spacing = 0;
			return 1;
		case V_SQL_SEQ_MODE_MOD:
		case V_SQL_SEQ_MODE_VELOCITY:
		case V_SQL_SEQ_MODE_MAX:
			break;
		}
		return 0;
	}

	///////////////////////////////////////////////////////////////////////////////
	// Play a step for a note mode
	void play_note_step(STEP_TYPE step_for_transpose, byte note_for_transpose, byte midi_vel_hi, byte midi_vel_med, byte midi_vel_lo) {
		STEP_TYPE step = 0;
		if(m_cfg.m_mode == V_SQL_SEQ_MODE_TRANSPOSE) {
			int transposed = (int)STEP_VALUE(m_state.m_step_value) + note_for_transpose - 64;
			if(transposed >= 0 && transposed < 128) {
				transposed = g_scale.force_to_scale(transposed);
				step = (step_for_transpose & 0xFF00) | transposed;
			}
		}
		else {
			step = m_state.m_step_value;
		}


		// Get step type: active / legato / velocity level
		byte legato = 0;
		byte velocity = 0;
		byte active = 1;
		switch(get_velocity(step)) {
		case VELOCITY_LOW: velocity = midi_vel_lo; break;
		case VELOCITY_MEDIUM: velocity = midi_vel_med; break;
		case VELOCITY_HIGH: velocity = midi_vel_hi; break;
		case VELOCITY_LEGATO: legato = 1; break; // legato only set for active step
		case VELOCITY_OFF: active = 0; break;
		}

		// Kill "open" notes which are timed to step boundaries rather than by a timeout
		byte kill_open_notes = 1;
		if(m_cfg.m_note_dur == V_SQL_STEP_DUR_STEP) {
			kill_open_notes = !legato;
		}
		else if(m_cfg.m_note_dur == V_SQL_STEP_DUR_FULL) {
			kill_open_notes = active && !legato;
		}
		if(kill_open_notes) {
			for(int i=0; i<MAX_PLAYING_NOTES;++i) {
				if(m_state.m_playing[i].note && !m_state.m_playing[i].count) {
					if(m_state.m_playing[i].note == m_state.m_last_note) {
						m_state.m_gate = CCVGate::GATE_CLOSED;
					}
					send_midi_note(m_state.m_playing[i].note,0);
					m_state.m_playing[i].note = 0;
				}
			}
		}


		// is there anything going on at this step
		if(active) {

			// force to scale if needed
			byte note = STEP_VALUE(step);
			if(m_cfg.m_mode == V_SQL_SEQ_MODE_SCALE) {
				note = g_scale.index_to_note(note);
			}
			else if(m_cfg.m_force_scale == V_SQL_FORCE_SCALE_ON) {
				note = g_scale.force_to_scale(note);
			}

			if(note) {
				// decide duration. a duration 0 is open ended
				byte duration = 0;
				if(m_cfg.m_note_dur >= V_SQL_STEP_DUR_32) {
					duration = c_step_duration[m_cfg.m_note_dur - V_SQL_STEP_DUR_32];
				}

				// scan list of playing note slots to find a usable slot where
				// we can track this note
				int free = -1;
				int same = -1;
				int last = -1;
				int steal = -1;
				int steal_count = -1;
				for(int i=0; i<MAX_PLAYING_NOTES;++i) {
					if(!m_state.m_playing[i].note) {
						// we have a free slot
						free = i;
					}
					else if(m_state.m_playing[i].note == note) {
						// the same note is already playing. we will use this slot so
						// there is no need to carry on looking
						same = i;
						break;
					}
					else if(m_state.m_playing[i].note == m_state.m_last_note) {
						// this slot is playing the last note output from this channel
						// (and it is still playing!)
						last = i;
					}
					else if(m_state.m_playing[i].count < steal_count || steal_count == -1) {
						// this is the note that has the least time remaining to play
						// so we might steal it's slot if we have to...
						steal_count = m_state.m_playing[i].count;
						steal = i;
					}
				}

				// For legato steps there is no velocity information... we use the
				// velocity for the last played note
				if(velocity) {
					m_state.m_last_velocity = velocity;
				}

				// decide which of the possible slots we're going to use
				int slot = -1;
				if(same>=0) {
					// slot for same note will always be reused
					if(!legato) {
						// retrigger the note
						send_midi_note(note,0);
						send_midi_note(note,m_state.m_last_velocity);
						m_state.m_gate = CCVGate::GATE_RETRIG;
					}
					slot = same;
				}
				else if(last>=0 && legato) {
					// the last played note is still sounding, in legato mode we will
					// replace that note, but only stop it after the new note starts
					send_midi_note(note,m_state.m_last_velocity);
					send_midi_note(m_state.m_playing[last].note,0);
					m_state.m_gate = CCVGate::GATE_OPEN;
					slot = last;
				}
				else if(free>=0) {
					// else a free slot will be used if available
					send_midi_note(note,m_state.m_last_velocity);
					m_state.m_gate = legato? CCVGate::GATE_OPEN : CCVGate::GATE_RETRIG;
					slot = free;
				}
				else if(steal>=0) {
					// final option we'll steal a slot from another note, so we need to stop that note playing
					send_midi_note(m_state.m_playing[steal].note,0);
					send_midi_note(note,m_state.m_last_velocity);
					m_state.m_gate = legato? CCVGate::GATE_OPEN : CCVGate::GATE_RETRIG;
					slot = steal;
				}

				if(slot >= 0) {
					m_state.m_last_note = note;
					m_state.m_playing[slot].note = note;
					m_state.m_playing[slot].count = duration;
				}
			}
		}
	}


	///////////////////////////////////////////////////////////////////////////////
	void test() {
		m_cfg.m_step[0] = 45|IS_VEL1|IS_ACTIVE;
		m_cfg.m_step[1] = 45|IS_VEL1|IS_ACTIVE;
		m_cfg.m_step[4] = 46|IS_VEL1|IS_ACTIVE;
		m_cfg.m_step[5] = 48|IS_VEL1|IS_ACTIVE;
		m_cfg.m_step[8] = 50|IS_VEL1|IS_ACTIVE;
		m_cfg.m_step[9] = 50|IS_VEL1|IS_ACTIVE;
		m_cfg.m_step[12] = 52|IS_VEL1|IS_ACTIVE;
	}

	/*
	///////////////////////////////////////////////////////////////////////////////
	void start_midi_note(byte chan, byte note, byte velocity, byte duration, byte legato) {

		// scan list of playing note slots to find candidate slots
		int free = -1;
		int same = -1;
		int last = -1;
		int steal = -1;
		byte steal_count = 255;
		for(int i=0; i<MAX_PLAYING_NOTES;++i) {
			if(!m_state.m_playing[i].note) {
				// we have a free slot
				free = i;
			}
			else if(m_state.m_playing[i].note == note) {
				// the same note is already playing. we will use this slot so
				// there is no need to carry on looking
				same = i;
				break;
			}
			else if(m_state.m_playing[i].note == m_state.m_last_note) {
				// this slot is playing the last note output from this channel
				// (and it is still playing!)
				last = i;
			}
			else if(m_state.m_playing[i].count < steal_count) {
				// this is the note that has the least time remaining to play
				// so we might steal it's slot if we have to...
				steal_count = m_state.m_playing[i].count;
				steal = i;
			}
		}


		// decide which of the possible slots we're going to use
		byte legato_note = 0;
		int slot = -1;
		if(same>=0) {
			// slot for same note will always be reused
			slot = same;
			if(legato) {
				// for legato playing we just extend this note, do not retrigger it!
				m_state.m_playing[same].count = duration;
				m_state.m_last_note = note;
				return;
			}
		}
		else if(last>=0 && legato) {
			// the last played note is still sounding, in legato mode we will
			// replace that note, but only stop it after the new note starts
			slot = last;
			velocity = m_state.m_last_velocity;
			legato_note = m_state.m_playing[last].note;
		}
		else if(free>=0) {
			// else a free slot will be used if available
			slot = free;
		}
		else if(steal>=0) {
			// final option we'll steal a slot from another note, so we need to stop that note playing
			g_midi.send_note(chan, m_state.m_playing[steal].note, 0);
			slot = steal;
		}

		if(slot>=0) {
			// play the the new note
			m_state.m_playing[slot].note = note;
			m_state.m_playing[slot].count = duration;
			m_state.m_last_note = note;
			m_state.m_last_velocity = velocity;
			g_midi.send_note(chan, note, velocity);
		}
		if(legato_note) {
			// if there is a "legato" note to stop only after the new note
			// has been started then stop it now
			g_midi.send_note(chan, legato_note, 0);
		}
	}*/

};


#ifdef MAIN_INCLUDE
const byte CSequenceLayer::c_tick_rates[V_SQL_STEP_RATE_MAX] = {96,72,48,36,32,24,18,16,12,9,8,6,4,3};
const byte CSequenceLayer::c_step_duration[] = {3,4,6,8,9,12,16,18,24,32,36,48,72,96};
#endif

#endif /* SEQUENCE_LAYER_H_ */
