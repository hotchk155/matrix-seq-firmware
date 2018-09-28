///////////////////////////////////////////////////////////////////////////////
// MATRIX SEQUENCER
// Sixty four pixels Ltd	March 2018
//
// SEQUENCER LAYER
#ifndef SEQUENCE_LAYER_H_
#define SEQUENCE_LAYER_H_

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

		IS_VALUE_SET = (uint16_t)0x100,	// is there a user set value at this step?

		IS_GATE = (uint16_t)0x200,		// is the gate open
		IS_TRIG = (uint16_t)0x400,		// is there a trigger at this point
		IS_ACCENT = (uint16_t)0x800,    // is there an accent (MIDI velocity) at this point

		GATE_MASK =	IS_GATE|IS_TRIG|IS_ACCENT,


		MAX_PLAYING_NOTES = 8,
		REFERENCE_NOTE = 48,
		DEFAULT_NOTE = 36,
		//MAX_MOD_VALUE = 127
	};

	// look up table of tick rates
	static const byte c_tick_rates[V_SQL_STEP_RATE_MAX];
	static const byte c_step_duration[V_SQL_STEP_DUR_MAX];

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
		V_SQL_STEP_DUR	m_note_dur;
		byte 			m_enabled;
		V_SQL_MIDI_CHAN m_midi_channel;		// MIDI channel
		byte 			m_midi_cc;			// MIDI CC
		V_SQL_CVSCALE	m_cv_scale;
		byte 			m_cv_range;
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
		CSequenceLayer::STEP_TYPE m_copy_step;
		//byte m_gate;
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
		m_cfg.m_note_dur	= V_SQL_STEP_DUR_16;
		m_cfg.m_loop_from	= 0;
		m_cfg.m_loop_to		= 15;
		m_cfg.m_transpose	= 0;
		//m_cfg.m_transpose_mod = V_SQL_TRANSPOSE_MOD_OFF;
		m_cfg.m_midi_channel 	= V_SQL_MIDI_CHAN_NONE;
		m_cfg.m_midi_cc = 1;
		m_cfg.m_enabled = 1;
		m_cfg.m_cv_scale = V_SQL_CVSCALE_1VOCT;
		m_cfg.m_cv_range = 5;
	}

	///////////////////////////////////////////////////////////////////////////////
	void init_state() {
		m_state.m_scroll_ofs = 48;
		m_state.m_last_tick_lsb = 0;
		m_state.m_last_note = 0;
		m_state.m_copy_step = 0;
		//m_state.m_gate = CCVGate::GATE_CLOSED;
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
		case P_SQL_CVSCALE: m_cfg.m_cv_scale = (V_SQL_CVSCALE)value; break;
		case P_SQL_CVRANGE: m_cfg.m_cv_range = value; break;
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
		case P_SQL_CVSCALE: return m_cfg.m_cv_scale;
		case P_SQL_CVRANGE: return m_cfg.m_cv_range;
		default:return 0;
		}
	}



	///////////////////////////////////////////////////////////////////////////////
	int is_valid_param(PARAM_ID param) {
		switch(param) {
		case P_SQL_MIDI_CHAN: return !(m_cfg.m_mode == V_SQL_SEQ_MODE_VELOCITY);
		case P_SQL_FORCE_SCALE: return !!(m_cfg.m_mode == V_SQL_SEQ_MODE_CHROMATIC||m_cfg.m_mode == V_SQL_SEQ_MODE_TRANSPOSE);
		case P_SQL_MIDI_CC:	return !!(m_cfg.m_mode == V_SQL_SEQ_MODE_MOD);
		case P_SQL_CVRANGE: return !!(m_cfg.m_mode == V_SQL_SEQ_MODE_MOD || m_cfg.m_mode == V_SQL_SEQ_MODE_VELOCITY);
		case P_SQL_CVSCALE: return !(m_cfg.m_mode == V_SQL_SEQ_MODE_MOD || m_cfg.m_mode == V_SQL_SEQ_MODE_VELOCITY);
		default: return 1;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	// preserve trigs but clear all data
	void reset_values(byte value) {
		for(int i=0; i<MAX_STEPS; ++i) {
			m_cfg.m_step[i] &= GATE_MASK;
			m_cfg.m_step[i] |= value;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	void clear_step_value(byte index) {
		switch(m_cfg.m_mode) {
		case V_SQL_SEQ_MODE_SCALE:
		case V_SQL_SEQ_MODE_CHROMATIC:
			m_cfg.m_step[index] = 0; // clear gate and note
			break;
		case V_SQL_SEQ_MODE_TRANSPOSE:
			m_cfg.m_step[index] &= GATE_MASK; // preserve gate into
			m_cfg.m_step[index] |= 64;
			break;
		case V_SQL_SEQ_MODE_MOD:
			m_cfg.m_step[index] &= GATE_MASK; // preserve gate into
			interpolate();
			break;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	void paste_step_value(byte index, STEP_TYPE step) {
		switch(m_cfg.m_mode) {
		case V_SQL_SEQ_MODE_SCALE:
		case V_SQL_SEQ_MODE_CHROMATIC:
			m_cfg.m_step[index] = step |= CSequenceLayer::IS_VALUE_SET;
			break;
		case V_SQL_SEQ_MODE_TRANSPOSE:
			m_cfg.m_step[index] &= GATE_MASK;
			m_cfg.m_step[index] |= CSequenceLayer::IS_VALUE_SET;
			m_cfg.m_step[index] |= (byte)step;
			break;
		case V_SQL_SEQ_MODE_MOD:
			m_cfg.m_step[index] &= GATE_MASK;
			m_cfg.m_step[index] |= CSequenceLayer::IS_VALUE_SET;
			m_cfg.m_step[index] |= (byte)step;
			interpolate();
			break;
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
					m_cfg.m_step[i] &= 0xFF;
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
		m_state.m_copy_step = 0;
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

	///////////////////////////////////////////////////////////////////////////////
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
	void manage(byte index, uint32_t ticks) {
		// has a tick expired?
		if(m_state.m_last_tick_lsb != (byte)ticks) {
			m_state.m_last_tick_lsb = (byte)ticks;
			for(int i=0; i<MAX_PLAYING_NOTES;++i) {
				if(m_state.m_playing[i].count) {
					if(!--m_state.m_playing[i].count) {
						if(m_state.m_playing[i].note == m_state.m_last_note) {
							// close the gate
							g_cv_gate.gate(index, CCVGate::GATE_CLOSED);
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


	///////////////////////////////////////////////////////////////////////////////
	static void inc_gate(STEP_TYPE *value) {
		if(*value & IS_TRIG) {
			*value |= IS_ACCENT;
		}
		else if(*value & IS_GATE) {
			*value |= IS_TRIG;
		}
		else
		{
			*value |= IS_GATE;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	static void dec_gate(STEP_TYPE *value) {
		if(*value & IS_ACCENT) {
			*value &= ~IS_ACCENT;
			*value |= IS_TRIG;
		}
		else if(*value & IS_TRIG) {
			*value &= ~IS_TRIG;
			*value |= IS_GATE;
		}
		else if(*value & IS_GATE) {
			*value &= ~IS_GATE;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	static void toggle_gate(STEP_TYPE *value) {
		if(*value & (IS_GATE|IS_ACCENT|IS_TRIG)) {
			*value &= ~GATE_MASK;
		}
		else {
			*value |= IS_TRIG;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	void constrain_step_value(STEP_TYPE *value) {
		inc_step_value(value, 0, 0);
	}

	///////////////////////////////////////////////////////////////////////////////
	void inc_step_value(STEP_TYPE *value, int delta, byte fine) {
		int v = (byte)(*value);
		int max_value = 127;
		switch(m_cfg.m_mode) {
		case V_SQL_SEQ_MODE_MOD:
			if(fine) {
				v+= delta;
			}
			else {
				v = 10*(v/10 + delta);
			}
			break;
		case V_SQL_SEQ_MODE_SCALE:
			v+=delta;
			max_value = g_scale.max_index();
			break;
		case V_SQL_SEQ_MODE_CHROMATIC:
		case V_SQL_SEQ_MODE_TRANSPOSE:
		default:
			v+=delta;
			break;
		}
		if(v<0) {
			v = 0;
		}
		else if(v>max_value) {
			v = max_value;
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
	inline uint16_t get_step(int index) {
		return m_cfg.m_step[index];
	}

	///////////////////////////////////////////////////////////////////////////////
	inline void set_step(int index, uint16_t step) {
		m_cfg.m_step[index] =step;
	}
	///////////////////////////////////////////////////////////////////////////////
	inline void set_step_value(int index, STEP_TYPE step) {
		if(m_cfg.m_mode == V_SQL_SEQ_MODE_MOD) {
			m_cfg.m_step[index] &= 0xFF00;
			m_cfg.m_step[index] |= (step & 0x00FF);
		}
		else {
			m_cfg.m_step[index] =step;
		}
	}

	byte shift_vertical(int offset) {
		for(int i = 0; i<MAX_STEPS; ++i) {
			STEP_TYPE step = m_cfg.m_step[i];
			if(step & IS_VALUE_SET) {
				int v = (int)STEP_VALUE(step) + offset;
				if(v < 0 || v > 127) {
					return 0;
				}
			}
		}
		for(int i = 0; i<MAX_STEPS; ++i) {
			STEP_TYPE step = m_cfg.m_step[i];
			if(step & IS_VALUE_SET) {
				m_cfg.m_step[i] = (step & 0xFF00) | (STEP_VALUE(step) + offset);
			}
		}
		return 1;
	}
	void shift_left() {
		STEP_TYPE step = m_cfg.m_step[0];
		for(int i = 0; i<MAX_STEPS-1; ++i) {
			set_step_value(i, m_cfg.m_step[i+1]);
		}
		set_step_value(MAX_STEPS-1, step);
	}
	void shift_right() {
		STEP_TYPE step = m_cfg.m_step[MAX_STEPS-1];
		for(int i = MAX_STEPS-1; i>0; --i) {
			set_step_value(i, m_cfg.m_step[i-1]);
		}
		set_step_value(0, step);
	}


	///////////////////////////////////////////////////////////////////////////////
	// Create interpolated points between two waypoints
	void interpolate_section(int pos, int end) {

		// calculate the number of new points that we will need to
		// crate during the interpolation
		int num_points = end - pos;
		if(num_points < 0) {
			num_points += MAX_STEPS;
		}
		if(num_points > 0) {

			// starting point and gradient
			double value =  (byte)m_cfg.m_step[pos];
			double gradient = ((byte)m_cfg.m_step[end] - value)/num_points;
			while(--num_points > 0) {
				// wrap around the column
				if(++pos >= MAX_STEPS) {
					pos = 0;
				}
				value += gradient;
				m_cfg.m_step[pos] = (m_cfg.m_step[pos] & GATE_MASK) | (byte)(value+0.5);
			}
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	// Interpolate all points
	void interpolate() {
		int i;
		int first_waypoint = -1;
		int prev_waypoint = -1;
		for(i=0; i<32; ++i) {
			if(m_cfg.m_step[i] & IS_VALUE_SET) {
				if(prev_waypoint < 0) {
					first_waypoint = i;
				}
				else {
					interpolate_section(prev_waypoint, i);
				}
				prev_waypoint = i;
			}
		}

		if(first_waypoint < 0) {
			// no waypoints defined
			for(i=0; i<32; ++i) {
				m_cfg.m_step[i] &= GATE_MASK; // data is 0
			}
		}
		else if(prev_waypoint == first_waypoint) {
			// only one waypoint defined
			for(i=0; i<32; ++i) {
				if(i!=prev_waypoint) {
					m_cfg.m_step[i] &= GATE_MASK;
					m_cfg.m_step[i] |= (byte)(m_cfg.m_step[first_waypoint]);
				}
			}
		}
		else {
			// multiple waypoints defined
			interpolate_section(prev_waypoint, first_waypoint);
		}
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
	void action_step_note(byte which, STEP_TYPE step_for_transpose, byte midi_vel_accent, byte midi_vel, byte action_gate) {
		STEP_TYPE step = 0;
		if(m_cfg.m_mode == V_SQL_SEQ_MODE_TRANSPOSE) {
			int transposed = (int)STEP_VALUE(step_for_transpose) + (int)STEP_VALUE(m_state.m_step_value) - 64;
			if(transposed >= 0 && transposed < 128) {
				step = (step_for_transpose & 0xFF00) | transposed;
			}
		}
		else {
			step = m_state.m_step_value;
		}


		// Get step type: active / legato / velocity level
		CCVGate::GATE_STATE gate_state = CCVGate::GATE_CLOSED;
		byte legato = 0;
		byte velocity = 0;
		byte active = !!(step & IS_GATE);
		if(step & IS_ACCENT) {
			velocity = midi_vel_accent;
		}
		else if(step & IS_TRIG) {
			velocity = midi_vel;
		}
		else {
			legato = 1;
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
						gate_state = CCVGate::GATE_CLOSED;
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
						gate_state = CCVGate::GATE_RETRIG;
					}
					slot = same;
				}
				else if(last>=0 && legato) {
					// the last played note is still sounding, in legato mode we will
					// replace that note, but only stop it after the new note starts
					send_midi_note(note,m_state.m_last_velocity);
					send_midi_note(m_state.m_playing[last].note,0);
					gate_state = CCVGate::GATE_OPEN;
					slot = last;
				}
				else if(free>=0) {
					// else a free slot will be used if available
					send_midi_note(note,m_state.m_last_velocity);
					gate_state = legato? CCVGate::GATE_OPEN : CCVGate::GATE_RETRIG;
					slot = free;
				}
				else if(steal>=0) {
					// final option we'll steal a slot from another note, so we need to stop that note playing
					send_midi_note(m_state.m_playing[steal].note,0);
					send_midi_note(note,m_state.m_last_velocity);
					gate_state = legato? CCVGate::GATE_OPEN : CCVGate::GATE_RETRIG;
					slot = steal;
				}

				if(slot >= 0) {
					m_state.m_last_note = note;
					m_state.m_playing[slot].note = note;
					m_state.m_playing[slot].count = duration;
					g_cv_gate.pitch_cv(which, note, m_cfg.m_cv_scale);
					if(action_gate) {
						g_cv_gate.gate(which, gate_state);
					}
				}
			}
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	void action_step_mod(byte which) {
		byte value = STEP_VALUE(m_state.m_step_value);
		g_cv_gate.mod_cv(which, value, m_cfg.m_cv_range);
	}

	///////////////////////////////////////////////////////////////////////////////
	// Play the gate for a step
	void action_step_gate(byte which) {
		STEP_TYPE step = m_state.m_step_value;
		if(step & IS_TRIG) {
			g_cv_gate.gate(which, CCVGate::GATE_RETRIG);
		}
		else if(step & IS_GATE) {
			g_cv_gate.gate(which, CCVGate::GATE_OPEN);
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	void test() {
		m_cfg.m_step[0] = 45|IS_GATE|IS_VALUE_SET;
		m_cfg.m_step[1] = 45|IS_GATE|IS_VALUE_SET;
		m_cfg.m_step[4] = 46|IS_GATE|IS_VALUE_SET;
		m_cfg.m_step[5] = 48|IS_GATE|IS_VALUE_SET;
		m_cfg.m_step[8] = 50|IS_GATE|IS_VALUE_SET;
		m_cfg.m_step[9] = 50|IS_GATE|IS_VALUE_SET;
		m_cfg.m_step[12] = 52|IS_GATE|IS_VALUE_SET;
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

//TODO
const byte CSequenceLayer::c_tick_rates[V_SQL_STEP_RATE_MAX] = {96,72,48,36,32,24,18,16,12,9,8,6,4,3};
const byte CSequenceLayer::c_step_duration[] = {3,4,6,8,9,12,16,18,24,32,36,48,72,96};

#endif /* SEQUENCE_LAYER_H_ */
