///////////////////////////////////////////////////////////////////////////////
// MATRIX SEQUENCER
// Sixty four pixels Ltd	March 2018
//
// SEQUENCER LAYER
#ifndef SEQUENCE_LAYER_H_
#define SEQUENCE_LAYER_H_

//#define STEP_VALUE(s) ((byte)(s))

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

class CSequencer;
///////////////////////////////////////////////////////////////////////////////
// This class defines a single layer of a sequence
class CSequenceLayer {


	///////////////////////////////////////////////////////////////////////////////
	void impl_shift_left(byte with_gates) {
		CStep step = m_cfg.m_step[0];
		for(int i = 0; i<MAX_STEPS-1; ++i) {
			if(with_gates) {
				m_cfg.m_step[i] = m_cfg.m_step[i+1];
			}
			else {
				m_cfg.m_step[i].copy_data_point(m_cfg.m_step[i+1]);
			}
		}
		if(with_gates) {
			m_cfg.m_step[MAX_STEPS-1] = step;
		}
		else {
			m_cfg.m_step[MAX_STEPS-1].copy_data_point(step);
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	void impl_shift_right(byte with_gates) {

		CStep step = m_cfg.m_step[MAX_STEPS-1];
		for(int i = 0; i<MAX_STEPS-1; ++i) {
			if(with_gates) {
				m_cfg.m_step[i] = m_cfg.m_step[i-1];
			}
			else {
				m_cfg.m_step[i].copy_data_point(m_cfg.m_step[i-1]);
			}
		}
		if(with_gates) {
			m_cfg.m_step[0] = step;
		}
		else {
			m_cfg.m_step[0].copy_data_point(step);
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	// Create interpolated points between two waypoints
	void impl_interpolate_section(int pos, int end)
	{
		// calculate the number of new points that we will need to
		// crate during the interpolation
		int num_points = end - pos;
		if(num_points < 0) {
			num_points += MAX_STEPS;
		}
		if(num_points > 0) {

			// starting point and gradient
			double value =  m_cfg.m_step[pos].m_value;
			double gradient = (m_cfg.m_step[end].m_value - value)/num_points;
			while(--num_points > 0) {
				// wrap around the column
				if(++pos >= MAX_STEPS) {
					pos = 0;
				}
				value += gradient;
				m_cfg.m_step[pos].m_value = (byte)(value+0.5);
			}
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	void impl_interpolate()
	{
		int i;
		int first_waypoint = -1;
		int prev_waypoint = -1;
		for(i=0; i<32; ++i) {
			if(m_cfg.m_step[i].m_is_data_point) {
				if(prev_waypoint < 0) {
					first_waypoint = i;
				}
				else {
					impl_interpolate_section(prev_waypoint, i);
				}
				prev_waypoint = i;
			}
		}

		if(first_waypoint < 0) {
			// no waypoints defined
			for(i=0; i<32; ++i) {
				m_cfg.m_step[i].m_value = 0;
			}
		}
		else if(prev_waypoint == first_waypoint) {
			// only one waypoint defined
			for(i=0; i<32; ++i) {
				if(i!=prev_waypoint) {
					m_cfg.m_step[i].m_value = m_cfg.m_step[first_waypoint].m_value;
				}
			}
		}
		else {
			// multiple waypoints defined
			impl_interpolate_section(prev_waypoint, first_waypoint);
		}
	}

	int next_step_index(int index) {
		++index;
		if(index > m_cfg.m_loop_to) {
			index -= m_cfg.m_loop_to;
		}
		if(index < m_cfg.m_loop_from) {
			index += m_cfg.m_loop_from;
		}
		return index;
	}


public:

	enum {
		NO_POS = -1,
		MAX_STEPS = 32,					// number of steps in the layer



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
		CStep		m_step[MAX_STEPS];	// data value and gate for each step
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
		V_SQL_CVGLIDE	m_cv_glide;
	} CONFIG;


	typedef struct {
		byte note;
		byte count;
	} PLAYING_NOTE;



	typedef struct {
		byte m_scroll_ofs;			// lowest step value shown on grid
		CStep m_step_value;			// the last value output by sequencer
		byte m_stepped;				// stepped flag
		int m_play_pos;
		byte m_last_note; // last midi note played on channel - and output to CV
		byte m_last_velocity;
		uint32_t m_next_tick;
		byte m_last_tick_lsb;
		PLAYING_NOTE m_playing[MAX_PLAYING_NOTES];
		CStep m_copy_step;
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
			m_cfg.m_step[i].reset_all(DEFAULT_NOTE);
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
		m_cfg.m_cv_glide = V_SQL_CVGLIDE_OFF;
	}

	///////////////////////////////////////////////////////////////////////////////
	void init_state() {
		m_state.m_scroll_ofs = 48;
		m_state.m_last_tick_lsb = 0;
		m_state.m_last_note = 0;
		m_state.m_copy_step.reset_all();
		memset(m_state.m_playing,0,sizeof(m_state.m_playing));
		reset();
	}

	///////////////////////////////////////////////////////////////////////////////
	void reset() {
		m_state.m_step_value.reset_all();
		m_state.m_stepped = 0;
		m_state.m_play_pos = 0;
		m_state.m_next_tick = 0;
	}

	///////////////////////////////////////////////////////////////////////////////
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
		case P_SQL_CVGLIDE: m_cfg.m_cv_glide = (V_SQL_CVGLIDE)value; break;
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
		case P_SQL_CVGLIDE: return m_cfg.m_cv_glide;
		default:return 0;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	int is_valid_param(PARAM_ID param) {
		switch(param) {
		case P_SQL_MIDI_CHAN: return !(m_cfg.m_mode == V_SQL_SEQ_MODE_VELOCITY);
		case P_SQL_FORCE_SCALE: return !!(m_cfg.m_mode == V_SQL_SEQ_MODE_CHROMATIC||m_cfg.m_mode == V_SQL_SEQ_MODE_TRANSPOSE_ALL || m_cfg.m_mode == V_SQL_SEQ_MODE_TRANSPOSE_LOCK);
		case P_SQL_MIDI_CC:	return !!(m_cfg.m_mode == V_SQL_SEQ_MODE_MOD);
		case P_SQL_CVRANGE: return !!(m_cfg.m_mode == V_SQL_SEQ_MODE_MOD || m_cfg.m_mode == V_SQL_SEQ_MODE_VELOCITY);
		case P_SQL_CVSCALE: return !(m_cfg.m_mode == V_SQL_SEQ_MODE_MOD || m_cfg.m_mode == V_SQL_SEQ_MODE_VELOCITY);
		default: return 1;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	// preserve trigs but clear all data
	void reset_data_points(byte value) {
		for(int i=0; i<MAX_STEPS; ++i) {
			m_cfg.m_step[i].reset_data_point(value);
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	// make a default data point
	void default_data_point(CStep& step) {
//TODO default for different types of layer
		step.reset_all(m_state.m_scroll_ofs);
		step.m_is_gate_open = 1;
		step.m_is_trigger = 1;
	}

	///////////////////////////////////////////////////////////////////////////////
// TODO reset data_point
	void clear_step_value(byte index) {
		switch(m_cfg.m_mode) {
		case V_SQL_SEQ_MODE_SCALE:
		case V_SQL_SEQ_MODE_CHROMATIC:
			m_cfg.m_step[index].reset_all(); // clear gate and note
			break;
		case V_SQL_SEQ_MODE_TRANSPOSE_ALL:
		case V_SQL_SEQ_MODE_TRANSPOSE_LOCK:
			m_cfg.m_step[index].reset_data_point(64); // preserve gate into, set data to midpoint
			break;
		case V_SQL_SEQ_MODE_MOD:
			m_cfg.m_step[index].reset_data_point(0); // preserve gate into, set data to zero
			break;
		}
		recalc_data_points();
	}

	///////////////////////////////////////////////////////////////////////////////
// TODO paste_data_point
	void paste_step_value(byte index, CStep& step) {
		switch(m_cfg.m_mode) {
		case V_SQL_SEQ_MODE_SCALE:
		case V_SQL_SEQ_MODE_CHROMATIC:
			m_cfg.m_step[index] = step;	// paste all data
			break;
		case V_SQL_SEQ_MODE_TRANSPOSE_ALL:
		case V_SQL_SEQ_MODE_TRANSPOSE_LOCK:
		case V_SQL_SEQ_MODE_MOD:
			m_cfg.m_step[index].copy_data_point(step);	// paste data point, leave gates unaffected
			break;
		}
		step.m_is_data_point = 1;
		recalc_data_points();
	}

	///////////////////////////////////////////////////////////////////////////////
	// change the mode
	void set_mode(V_SQL_SEQ_MODE value) {
		switch (value) {
		case V_SQL_SEQ_MODE_CHROMATIC:
			if(m_cfg.m_mode == V_SQL_SEQ_MODE_SCALE) {
				// changing from short scale to chromatic mode...
				for(int i=0; i<MAX_STEPS; ++i) {
					if(m_cfg.m_step[i].m_is_data_point) {
						m_cfg.m_step[i].m_value = g_scale.index_to_note(m_cfg.m_step[i].m_value);
					}
				}
				m_state.m_scroll_ofs = g_scale.index_to_note(m_state.m_scroll_ofs);
			}
			else {
				reset_data_points(DEFAULT_NOTE);
				m_state.m_scroll_ofs = DEFAULT_NOTE - 5;
			}
			break;

		case V_SQL_SEQ_MODE_SCALE:
			if(m_cfg.m_mode == V_SQL_SEQ_MODE_CHROMATIC) {
				// changing from chromatic to short scale mode...
				for(int i=0; i<MAX_STEPS; ++i) {
					if(m_cfg.m_step[i].m_is_data_point) {
						m_cfg.m_step[i].m_value = g_scale.note_to_index(m_cfg.m_step[i].m_value);
					}
				}
				m_state.m_scroll_ofs = g_scale.note_to_index(m_state.m_scroll_ofs);
			}
			else {
				int default_index = g_scale.note_to_index(DEFAULT_NOTE);
				reset_data_points(default_index);
				m_state.m_scroll_ofs = default_index - 5;
			}
			break;

		case V_SQL_SEQ_MODE_TRANSPOSE_ALL:
		case V_SQL_SEQ_MODE_TRANSPOSE_LOCK:
			if(m_cfg.m_mode != V_SQL_SEQ_MODE_TRANSPOSE_ALL &&
				m_cfg.m_mode != V_SQL_SEQ_MODE_TRANSPOSE_LOCK) {
				reset_data_points(64);
				m_state.m_scroll_ofs = 60;
			}
			break;

		case V_SQL_SEQ_MODE_MOD:
			reset_data_points(0);
			break;
		default:
			break;
		}
		m_cfg.m_mode = value;

		// clear the paste buffer, since the data type has changed
		m_state.m_copy_step.reset_all();
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
	void inc_gate(int index) {
		if(!m_cfg.m_step[index].m_is_gate_open) {
			m_cfg.m_step[index].m_is_gate_open = 1;
		}
		else if(!m_cfg.m_step[index].m_is_trigger) {
			m_cfg.m_step[index].m_is_trigger = 1;
		}
		else if(!m_cfg.m_step[index].m_is_accent) {
			m_cfg.m_step[index].m_is_accent = 1;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	void dec_gate(int index) {
		if(m_cfg.m_step[index].m_is_accent) {
			m_cfg.m_step[index].m_is_accent = 0;
		}
		else if(m_cfg.m_step[index].m_is_trigger) {
			m_cfg.m_step[index].m_is_trigger = 0;
		}
		else if(m_cfg.m_step[index].m_is_gate_open) {
			m_cfg.m_step[index].m_is_gate_open = 0;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	void toggle_gate(int index) {
		if(m_cfg.m_step[index].m_is_gate_open) {
			m_cfg.m_step[index].reset_gate();
		}
		else {
			m_cfg.m_step[index].m_is_gate_open = 1;
			m_cfg.m_step[index].m_is_trigger = 1;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	void inc_step_value(CStep& step, int delta, byte fine) {
		int value = step.m_value;
		int max_value = 127;
		switch(m_cfg.m_mode) {
		case V_SQL_SEQ_MODE_MOD:
			if(fine) {
				value += delta;
			}
			else {
				value = 10*(value/10 + delta);
			}
			break;
		case V_SQL_SEQ_MODE_SCALE:
			value += delta;
			max_value = g_scale.max_index();
			break;
		case V_SQL_SEQ_MODE_CHROMATIC:
		case V_SQL_SEQ_MODE_TRANSPOSE_ALL:
		case V_SQL_SEQ_MODE_TRANSPOSE_LOCK:
		default:
			value += delta;
			break;
		}
		if(value<0) {
			value = 0;
		}
		else if(value>max_value) {
			value = max_value;
		}
		step.m_value = value;
		step.m_is_data_point = 1;	// the data point has been set by user
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
	inline CStep get_step(int index) {
		return m_cfg.m_step[index];
	}

	///////////////////////////////////////////////////////////////////////////////
	// shift pattern vertically up or down by one space
	byte shift_vertical(int dir) {
		// first make sure that the shift will not exceed the bounds at
		// the top of the bottom of the pattern
		for(int i = 0; i<MAX_STEPS; ++i) {
			CStep step = m_cfg.m_step[i];
			if(step.m_is_data_point) {
				int new_value = (int)step.m_value + dir;
				if(new_value < 0 || new_value > 127) {
					return 0;
				}
			}
		}

		// perform the actual shift of all the data points
		for(int i = 0; i<MAX_STEPS; ++i) {
			CStep step = m_cfg.m_step[i];
			if(step.m_is_data_point) {
				m_cfg.m_step[i].m_value += dir;
			}
		}

		// perform any recalculation needed in the pattern
		recalc_data_points();
		return 1;
	}


	///////////////////////////////////////////////////////////////////////////////
	// shift pattern horizontally by one step
	void shift_horizontal(int dir) {
		if(dir<0) {
			impl_shift_left(is_note_mode());
		}
		else {
			impl_shift_right(is_note_mode());
		}
		recalc_data_points();
	}



	///////////////////////////////////////////////////////////////////////////////
	// Interpolate all points
	void recalc_data_points() {
		if(m_cfg.m_mode == V_SQL_SEQ_MODE_MOD) {
			impl_interpolate();
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
		case V_SQL_SEQ_MODE_TRANSPOSE_ALL:
		case V_SQL_SEQ_MODE_TRANSPOSE_LOCK:
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


/*
	///////////////////////////////////////////////////////////////////////////////
	// Play a step for a note mode
	void action_step_note(byte which, STEP_TYPE step_for_transpose, byte midi_vel_accent, byte midi_vel, byte action_gate) {
		STEP_TYPE step = 0;
		if(m_cfg.m_mode == V_SQL_SEQ_MODE_TRANSPOSE_ALL) {
			int transposed = (int)STEP_VALUE(step_for_transpose) + (int)STEP_VALUE(m_state.m_step_value) - 64;
			if(transposed >= 0 && transposed < 128) {
				step = (step_for_transpose & 0xFF00) | transposed;
			}
		}
		else if((m_cfg.m_mode == V_SQL_SEQ_MODE_TRANSPOSE_LOCK)) {
			 if(!(step_for_transpose & IS_ACCENT)) {
				 int transposed = (int)STEP_VALUE(step_for_transpose) + (int)STEP_VALUE(m_state.m_step_value) - 64;
				 if(transposed >= 0 && transposed < 128) {
					 step = (step_for_transpose & 0xFF00) | transposed;
				 }
			 }
			 else {
				 step = step_for_transpose & ~IS_ACCENT;
			 }
		}
		else {
			step = m_state.m_step_value;
		}


		// Get step type: active / legato / velocity level
		int glide_time = 0;
		CCVGate::GATE_STATE gate_state = CCVGate::GATE_CLOSED;
		byte legato = 0;
		byte velocity = 0;
		byte active = !!(step & IS_DATA_POINT);
		if(step & IS_ACCENT) {
			velocity = midi_vel_accent;
		}
		else if(step & IS_TRIG) {
			velocity = midi_vel;
		}
		else {
			// active step with no trigger or no gate
			legato = 1;
			if(m_cfg.m_cv_glide == V_SQL_CVGLIDE_ON) {
				glide_time = g_clock.get_ms_for_measure(m_cfg.m_step_rate);
			}
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
					g_cv_gate.pitch_cv(which, note, m_cfg.m_cv_scale, glide_time);
					if(action_gate) {
						g_cv_gate.gate(which, gate_state);
					}
				}
			}
		}
	}
*/

	///////////////////////////////////////////////////////////////////////////////
	// Play a step for a note mode
	void action_step_note(byte which, CStep step_for_transpose, byte midi_vel_accent, byte midi_vel, byte action_gate) {


		// determine the step value to be processed, which may come from a different layer
		// in the case of transposition
		CStep step;
		step.reset_all();
		int transposed;
		switch(m_cfg.m_mode) {
		case V_SQL_SEQ_MODE_TRANSPOSE_ALL:
			transposed = (int)step_for_transpose.m_value + m_state.m_step_value.m_value - 64;
			if(transposed >= 0 && transposed < 128) {
				step = step_for_transpose;
				step.m_value = transposed;
			}
			break;
		case V_SQL_SEQ_MODE_TRANSPOSE_LOCK:
			 if(step_for_transpose.m_is_accent) {
				 transposed = (int)step_for_transpose.m_value + m_state.m_step_value.m_value - 64;
				 if(transposed >= 0 && transposed < 128) {
					step = step_for_transpose;
					step.m_value = transposed;
				 }
			 }
			 else {
				step = step_for_transpose;
				step.m_is_accent = 0;
			 }
			 break;
		default:
			step = m_state.m_step_value;
		}


		// Get step type: active / legato / velocity level
		int glide_time = 0;
		CCVGate::GATE_STATE gate_state = CCVGate::GATE_CLOSED;
		byte legato = 0;
		byte velocity = 0;
		byte active = step.m_is_data_point;
		if(step.m_is_accent) {
			velocity = midi_vel_accent;
		}
		else if(step.m_is_trigger) {
			velocity = midi_vel;
		}
		else {
			// active step with no trigger or no gate
			legato = 1;
			if(m_cfg.m_cv_glide == V_SQL_CVGLIDE_ON) {
				glide_time = g_clock.get_ms_for_measure(m_cfg.m_step_rate);
			}
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
			byte note = step.m_value;
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
					g_cv_gate.pitch_cv(which, note, m_cfg.m_cv_scale, glide_time);
					if(action_gate) {
						g_cv_gate.gate(which, gate_state);
					}
				}
			}
		}
	}


	///////////////////////////////////////////////////////////////////////////////
	void action_step_mod(byte which) {
		byte value1 = m_cfg.m_step[m_state.m_play_pos].m_value;
		if(m_cfg.m_cv_glide == V_SQL_CVGLIDE_ON) {
			int next = next_step_index(m_state.m_play_pos);
			byte value2 = m_cfg.m_step[next].m_value;
			int ms = g_clock.get_ms_for_measure(m_cfg.m_step_rate);
			g_cv_gate.mod_cv(which, value1, m_cfg.m_cv_range, value2, ms);
		}
		else {
			g_cv_gate.mod_cv(which, value1, m_cfg.m_cv_range, 0, 0);
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	// Play the gate for a step
	void action_step_gate(byte which) {
		CStep step = m_state.m_step_value;
		if(step.m_is_trigger) {
			g_cv_gate.gate(which, CCVGate::GATE_RETRIG);
		}
		else if(step.m_is_gate_open) {
			g_cv_gate.gate(which, CCVGate::GATE_OPEN);
		}
		else {
			g_cv_gate.gate(which, CCVGate::GATE_CLOSED);
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	void test() {
/*		m_cfg.m_step[0] = 45|IS_GATE|IS_DATA_POINT;
		m_cfg.m_step[1] = 45|IS_GATE|IS_DATA_POINT;
		m_cfg.m_step[4] = 46|IS_GATE|IS_DATA_POINT;
		m_cfg.m_step[5] = 48|IS_GATE|IS_DATA_POINT;
		m_cfg.m_step[8] = 50|IS_GATE|IS_DATA_POINT;
		m_cfg.m_step[9] = 50|IS_GATE|IS_DATA_POINT;
		m_cfg.m_step[12] = 52|IS_GATE|IS_DATA_POINT;*/
	}

};

//TODO
const byte CSequenceLayer::c_tick_rates[V_SQL_STEP_RATE_MAX] = {96,72,48,36,32,24,18,16,12,9,8,6,4,3};
const byte CSequenceLayer::c_step_duration[] = {3,4,6,8,9,12,16,18,24,32,36,48,72,96};

#endif /* SEQUENCE_LAYER_H_ */
