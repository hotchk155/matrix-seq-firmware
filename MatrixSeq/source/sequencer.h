///////////////////////////////////////////////////////////////////////////////
// MATRIX SEQUENCER
// Sixty four pixels Ltd	March 2018
//
// SEQUENCER

#ifndef SEQUENCER_H_
#define SEQUENCER_H_




///////////////////////////////////////////////////////////////////////////////
// SEQUENCER CLASS
class CSequencer {
	byte m_is_running;
public:

	enum {
		NUM_LAYERS = 4,	// number of layers in the sequence
		GRID_WIDTH = 32,
		MAX_CURSOR = 31,
		POPUP_MS = 2000
	};
	enum {
		ACTION_NONE,
		ACTION_CREATE_STEP,
		ACTION_EDIT_STEP,
		//ACTION_EDIT_TRIG,
		ACTION_ERASE_STEP,
		ACTION_CLONE_STEP,
		ACTION_SET_LOOP,
		ACTION_DRAG_LOOP,
		ACTION_MOVE_VERT,
		ACTION_MOVE_HORZ
	};

	static const uint32_t c_ruler = 0x88888888U;

	// Config info that forms part of the patch
	typedef struct {
		V_SQL_SCALE_TYPE scale_type;
		V_SQL_SCALE_ROOT scale_root;
		byte 			m_midi_vel_accent;
		byte 			m_midi_vel;
	} CONFIG;
	CONFIG m_cfg;

	CSequenceLayer m_layers[NUM_LAYERS];
	CSequencer() {
		init_config();
		init_state();
	}

	int m_layer;			// this is the current layer being viewed/edited
	int m_cursor;			// this is the position of the edit cursor
	//int m_row;
	int m_value;			// value ebign edited in drag mode
	int m_action;
	byte m_flags;
	byte m_scale[11];
	byte m_scale_size;

	int m_sel_from;
	int m_sel_to;


	///////////////////////////////////////////////////////////////////////////////
	void init_config() {
		m_cfg.scale_type = V_SQL_SCALE_TYPE_IONIAN;
		m_cfg.scale_root = V_SQL_SCALE_ROOT_C;
		m_cfg.m_midi_vel_accent = 127;
		m_cfg.m_midi_vel = 100;
	}

	///////////////////////////////////////////////////////////////////////////////
	void init_state() {
		m_cursor = 0;
		//m_row = 0;
		m_flags = 0;
		m_action = ACTION_NONE;
		//m_popup = 0;
		//m_copy_step = 0;
		m_value = 0;
		m_scale_size = 0;
		m_is_running = 0;
		m_sel_from = -1;
		m_sel_to = -1;
	}

	///////////////////////////////////////////////////////////////////////////////
	void set(PARAM_ID param, int value) {
		switch(param) {
		case P_SQL_SCALE_TYPE: m_cfg.scale_type = (V_SQL_SCALE_TYPE)value; break;
		case P_SQL_SCALE_ROOT: m_cfg.scale_root = (V_SQL_SCALE_ROOT)value; break;
		case P_SQL_MIDI_VEL_ACCENT: m_cfg.m_midi_vel_accent = value; break;
		case P_SQL_MIDI_VEL: m_cfg.m_midi_vel = value; break;
		default: m_layers[m_layer].set(param,value);
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	int get(PARAM_ID param) {
		switch(param) {
		case P_SQL_SCALE_TYPE: return m_cfg.scale_type;
		case P_SQL_SCALE_ROOT: return m_cfg.scale_root;
		case P_SQL_MIDI_VEL_ACCENT: return m_cfg.m_midi_vel_accent;
		case P_SQL_MIDI_VEL: return m_cfg.m_midi_vel;
		default: return m_layers[m_layer].get(param);
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	int is_valid_param(PARAM_ID param) {
		switch(param) {
		case P_SQL_SCALE_TYPE:
		case P_SQL_SCALE_ROOT:
			switch(m_layers[m_layer].m_cfg.m_mode) {
			case V_SQL_SEQ_MODE_CHROMATIC:
			case V_SQL_SEQ_MODE_TRANSPOSE_ALL:
			case V_SQL_SEQ_MODE_TRANSPOSE_LOCK:
				return (m_layers[m_layer].m_cfg.m_force_scale == V_SQL_FORCE_SCALE_ON);
			case V_SQL_SEQ_MODE_SCALE:
				return 1;
			case V_SQL_SEQ_MODE_MOD:
			case V_SQL_SEQ_MODE_VELOCITY:
			case V_SQL_SEQ_MODE_MAX:
				return 0;
			default:
				break;
			}
			break;
		case P_SQL_MIDI_VEL:
		case P_SQL_MIDI_VEL_ACCENT:
			switch(m_layers[m_layer].m_cfg.m_mode) {
			case V_SQL_SEQ_MODE_CHROMATIC:
			case V_SQL_SEQ_MODE_TRANSPOSE_ALL:
			case V_SQL_SEQ_MODE_TRANSPOSE_LOCK:
			case V_SQL_SEQ_MODE_SCALE:
				return 1;
			case V_SQL_SEQ_MODE_MOD:
			case V_SQL_SEQ_MODE_VELOCITY:
			case V_SQL_SEQ_MODE_MAX:
				return 0;
			default:
				break;
			}
			break;
		default:
			break;
		}
		return m_layers[m_layer].is_valid_param(param);
	}

	void start() {
		m_is_running = 1;
		for(int i=0; i<NUM_LAYERS; ++i) {
			m_layers[i].start(g_clock.get_ticks(), g_clock.get_part_ticks());
		}
	}
	void stop() {
		m_is_running = 0;
		for(int i=0; i<NUM_LAYERS; ++i) {
			m_layers[i].stop_all_notes();
		}
	}
	void reset() {
		for(int i=0; i<NUM_LAYERS; ++i) {
			m_layers[i].reset();
		}
	}
	byte is_running() {
		return m_is_running;
	}

	byte is_layer_enabled(byte layer) {
		return m_layers[layer].m_cfg.m_enabled;
	}
	void enable_layer(byte layer, byte enable) {
		m_layers[layer].m_cfg.m_enabled = enable;
	}

	///////////////////////////////////////////////////////////////////////////////
	void force_repaint() {

	}
	///////////////////////////////////////////////////////////////////////////////
	void activate() {

	}

	///////////////////////////////////////////////////////////////////////////////
	void set_active_layer(int l) {
		m_layer = l;
	}

	void copy_layer(int from, int to) {
		m_layers[to].copy_from(m_layers[from]);
	}

	///////////////////////////////////////////////////////////////////////////////
	void step_info(CStep step, CSequenceLayer& layer) {
		byte value = step.m_value;
		switch(layer.m_cfg.m_mode) {
		case V_SQL_SEQ_MODE_SCALE:
			if(layer.m_cfg.m_mode == V_SQL_SEQ_MODE_SCALE) {
				value = g_scale.index_to_note(value);
			}
			// fall thru
		case V_SQL_SEQ_MODE_CHROMATIC:
			g_popup.note_name(value);
			break;
		case V_SQL_SEQ_MODE_TRANSPOSE_ALL:
		case V_SQL_SEQ_MODE_TRANSPOSE_LOCK:
			g_popup.show_offset(((int)value)-64);
			break;
		case V_SQL_SEQ_MODE_MOD:
			g_popup.num3digits(value);
			break;
		case V_SQL_SEQ_MODE_VELOCITY:
		case V_SQL_SEQ_MODE_MAX:
			break;
		}
		g_popup.avoid(m_cursor);
	}

	///////////////////////////////////////////////////////////////////////////////
	void scroll(int dir) {
		CSequenceLayer& layer = m_layers[m_layer];
		if(!layer.is_mod_mode()) {
			int scroll_ofs = m_layers[m_layer].m_state.m_scroll_ofs + dir;
			if(scroll_ofs < 0) {
				scroll_ofs = 0;
			}
			else if(scroll_ofs > 114) {
				scroll_ofs = 114;
			}
			m_layers[m_layer].m_state.m_scroll_ofs = scroll_ofs;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	void set_scroll_for(CStep step, CSequenceLayer& layer) {
		int v = step.m_value;
		if(v<layer.m_state.m_scroll_ofs) {
			layer.m_state.m_scroll_ofs = v;
		}
		else if(v>layer.m_state.m_scroll_ofs+12) {
			layer.m_state.m_scroll_ofs = v-12;
		}
	}

	//
	//
	//
	//
	//  button down
	//  encoder move
	//  encoder increment
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//	//
	/////////////////////////////////////////////////////////////////////////////////////////////

typedef enum:byte {
	ACTION_BEGIN,
	ACTION_ENC_LEFT,
	ACTION_ENC_RIGHT,
	ACTION_HOLD,
	ACTION_CLICK,
	ACTION_EDIT_KEYS,
	ACTION_END
} ACTION;

	uint32_t m_action_context = 0;
	byte m_encoder_moved = 0;
	uint32_t m_edit_keys = 0;


	///////////////////////////////////////////////////////////////////////////////
	// change step value based on encode event
	void value_action(CSequenceLayer& layer, CStep& step, ACTION what, byte fine) {
		switch(what) {
		case ACTION_ENC_LEFT:
			layer.inc_step_value(step, -1, fine);
			break;
		case ACTION_ENC_RIGHT:
			layer.inc_step_value(step, +1, fine);
			break;
		default:
			break;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	// Move cursor left / right for encoder event
	void cursor_action(ACTION what) {
		switch(what) {
		case ACTION_ENC_RIGHT:
			if(m_cursor < CSequenceLayer::MAX_STEPS-1) {
				++m_cursor;
			}
			break;
		case ACTION_ENC_LEFT:
			if(m_cursor > 0) {
				--m_cursor;
			}
			break;
		default:
			break;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	// STUFF WHAT THE EDIT BUTTON DOES...
	byte edit_action(CSequenceLayer& layer, ACTION what) {
		CStep step = layer.get_step(m_cursor);
		switch(what) {
		////////////////////////////////////////////////
		case ACTION_BEGIN:
			if(!layer.is_note_mode() || step.m_is_data_point) {
				// there is a valid note in this column so we copy it to our
				// paste buffer, scroll it into view and show note name on screen
				layer.m_state.m_copy_step = step;
				set_scroll_for(layer.m_state.m_copy_step, layer);
				if(!layer.is_mod_mode()) {
					step_info(layer.m_state.m_copy_step, layer);
				}
			}
			break;
		////////////////////////////////////////////////
		case ACTION_HOLD:
			// holding the button down shows the layer id
			g_popup.layer(m_layer, layer.m_cfg.m_enabled);
			break;
		////////////////////////////////////////////////
		case ACTION_ENC_LEFT:
		case ACTION_ENC_RIGHT:
			switch(m_edit_keys) {

				// fine edit in mod mode
			case KEY_EDIT|KEY_CLEAR:
				if(layer.is_mod_mode()) {
					value_action(layer, layer.m_state.m_copy_step, what, 1);
					layer.paste_step_value(m_cursor, layer.m_state.m_copy_step);
					step_info(layer.m_state.m_copy_step, layer);
				}
				break;
			case KEY_EDIT|KEY_GATE:
				// action to shift all points up or down
				if(what == ACTION_ENC_LEFT) {
					if(layer.shift_vertical(-1)) {
						--m_value;
					}
				}
				else {
					if(layer.shift_vertical(+1)) {
						++m_value;
					}
				}
				g_popup.show_offset(m_value);
				break;
			case KEY_EDIT|KEY_LOOP:
				// action to shift all points left or right
				if(what == ACTION_ENC_LEFT) {
					if(--m_value <= -31) {
						m_value = 0;
					}
					layer.shift_horizontal(-1);
				}
				else {
					if(++m_value >= 31) {
						m_value = 0;
					}
					layer.shift_horizontal(+1);
				}
				g_popup.show_offset(m_value);
				break;
			default:
				// encoder moved while the button is held
				if(!layer.is_note_mode() || step.m_is_data_point) {
					// we are dragging a note around
					value_action(layer, layer.m_state.m_copy_step, what, 0);

				}
				else {
					// no note in copy buffer so we invent one based on scroll offset
					// and insert it into the pattern
					if(!layer.m_state.m_copy_step.m_is_data_point) {
						layer.default_data_point(layer.m_state.m_copy_step);
					}
				}
				layer.paste_step_value(m_cursor, layer.m_state.m_copy_step);
				set_scroll_for(layer.m_state.m_copy_step, layer);
				if(!layer.is_mod_mode()) {
					step_info(layer.m_state.m_copy_step, layer);
				}
				break;
			}
			break;
		////////////////////////////////////////////////
		case ACTION_EDIT_KEYS:
			switch(m_edit_keys) {
			case KEY_EDIT|KEY_PASTE:
				// EDIT + PASTE - advance cursor and copy the current step
				if(layer.m_state.m_copy_step.m_is_data_point) {
					if(++m_cursor >= CSequenceLayer::MAX_STEPS-1) {
						m_cursor = 0;
					}
					layer.paste_step_value(m_cursor, layer.m_state.m_copy_step);
				}
				break;
			case KEY_EDIT|KEY_CLEAR:
				if(layer.is_note_mode()) {
					// EDIT + CLEAR - insert rest and advance cursor
					layer.clear_step_value(m_cursor);
					if(++m_cursor >= CSequenceLayer::MAX_STEPS-1) {
						m_cursor = 0;
					}
					layer.paste_step_value(m_cursor, layer.m_state.m_copy_step);
				}
				break;
			case KEY_EDIT|KEY_GATE:
				m_value = 0;
				g_popup.text("VERT", 4);
				g_popup.align(CPopup::ALIGN_RIGHT);
				break;
			case KEY_EDIT|KEY_LOOP:
				m_value = 0;
				g_popup.text("HORZ", 4);
				g_popup.align(CPopup::ALIGN_RIGHT);
				break;
			}
			break;
		default:
			break;
		}
		return 0;
	}

	///////////////////////////////////////////////////////////////////////////////
	// PASTE BUTTON
	void paste_action(CSequenceLayer& layer, ACTION what) {
		switch(what) {
		////////////////////////////////////////////////
		case ACTION_CLICK:
			// a click pastes note to new step
			if(layer.m_state.m_copy_step.m_is_data_point) {
				layer.paste_step_value(m_cursor, layer.m_state.m_copy_step);
			}
			break;
			////////////////////////////////////////////////
		case ACTION_ENC_LEFT:
		case ACTION_ENC_RIGHT:
			// hold-turn pastes multiple notes
			if(!layer.is_note_mode() || (layer.m_state.m_copy_step.m_is_data_point)) {
				layer.paste_step_value(m_cursor, layer.m_state.m_copy_step);
			}
			cursor_action(what);
			break;
		default:
			break;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	// CLEAR BUTTON
	void clear_action(CSequenceLayer& layer, ACTION what) {
		switch(what) {
		////////////////////////////////////////////////
		case ACTION_CLICK:
			// a click erases a step, copying it to paste buffer
			layer.m_state.m_copy_step = layer.get_step(m_cursor);
			layer.clear_step_value(m_cursor);
			break;
			////////////////////////////////////////////////
		case ACTION_ENC_LEFT:
		case ACTION_ENC_RIGHT:
			// hold-turn erases multiple notes
			layer.clear_step_value(m_cursor);
			cursor_action(what);
			break;
		default:
			break;
		}
	}


	///////////////////////////////////////////////////////////////////////////////
	// GATE BUTTON
	void gate_action(CSequenceLayer& layer, ACTION what) {
		switch(what) {
		////////////////////////////////////////////////
		case ACTION_ENC_LEFT:
			layer.dec_gate(m_cursor);
			break;
		////////////////////////////////////////////////
		case ACTION_ENC_RIGHT:
			layer.inc_gate(m_cursor);
			break;
		////////////////////////////////////////////////
		case ACTION_CLICK:
			layer.toggle_gate(m_cursor);
			break;
		default:
			break;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	void loop_action(CSequenceLayer& layer, ACTION what) {
		switch(what) {
		////////////////////////////////////////////////
		case ACTION_ENC_LEFT:
		case ACTION_ENC_RIGHT:
			if(m_sel_from < 0) {
				m_sel_from = m_cursor;
			}
			cursor_action(what);
			m_sel_to = m_cursor;
			break;
		////////////////////////////////////////////////
		case ACTION_END:
			if(m_sel_from < 0) {
				layer.set_pos(m_cursor);
			}
			else {
				if(m_sel_to >= m_sel_from) {
					layer.m_cfg.m_loop_from = m_sel_from;
					layer.m_cfg.m_loop_to = m_sel_to;
				}
				else {
					layer.m_cfg.m_loop_from = m_sel_to;
					layer.m_cfg.m_loop_to = m_sel_from;
				}
				layer.set_pos(layer.m_cfg.m_loop_from);
				m_sel_to = -1;
				m_sel_from = -1;
			}
			break;
		default:
			break;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	// MENU BUTTON
	void menu_action(CSequenceLayer& layer, ACTION what) {
		switch(what) {
			////////////////////////////////////////////////
		case ACTION_ENC_LEFT:
			scroll(+1);
			break;
		case ACTION_ENC_RIGHT:
			scroll(-1);
			break;
		default:
			break;
		}
	}


	void action(ACTION what) {
		CSequenceLayer& layer(m_layers[m_layer]);
		switch(m_action_context) {
		case KEY_EDIT: edit_action(layer,what); break;
		case KEY_PASTE: paste_action(layer,what); break;
		case KEY_CLEAR: clear_action(layer,what); break;
		case KEY_GATE: gate_action(layer,what); break;
		case KEY_LOOP: loop_action(layer,what); break;
		case KEY_MENU: menu_action(layer,what); break;
		}
	}

	void event(int evt, uint32_t param) {
		switch(evt) {
		case EV_KEY_PRESS:
			if(!m_action_context) {
				switch(param) {
				case KEY_EDIT:
				case KEY_PASTE:
				case KEY_CLEAR:
				case KEY_GATE:
				case KEY_LOOP:
				case KEY_MENU:
					m_action_context = param;
					m_encoder_moved = 0;
					action(ACTION_BEGIN);
				}
			}
			else if(m_action_context == KEY_EDIT) {
				m_edit_keys = param;
				action(ACTION_EDIT_KEYS);
			}
			break;
		case EV_KEY_RELEASE:
			if(param & m_edit_keys) {
				m_edit_keys = 0;
			}
			if(param == m_action_context) {
				action(ACTION_END);
				m_action_context = 0;
			}
			break;
		case EV_KEY_HOLD:
			if(param == m_action_context) {
				action(ACTION_HOLD);
			}
			break;
		case EV_KEY_CLICK:
			if(param == m_action_context) {
				action(ACTION_CLICK);
			}
			break;
		case EV_ENCODER:
			if(m_action_context) {
				if((int)param<0) {
					action(ACTION_ENC_LEFT);
				}
				else {
					action(ACTION_ENC_RIGHT);
				}
				m_encoder_moved = 1;
			}
			else {
				if((int)param < 0) {
					if(m_cursor > 0) {
						--m_cursor;
					}
				}
				else {
					if(++m_cursor >=  MAX_CURSOR) {
						m_cursor = MAX_CURSOR;
					}
				}
				g_popup.avoid(m_cursor);
			}
			break;
		}
	}




	/////////////////////////////////////////////////////////////////////////////////////////////
	void repaint() {
		int i;
		uint32_t mask;
		CSequenceLayer *layer = &m_layers[m_layer];

		// Clear the display
		g_ui.clear();

		// insert the "graticule", which provides the grid lines to in the background
		// of note based modes
		int graticule_row = 0;
		int graticule_spacing = 0;
		if(layer->get_graticule(&graticule_row, &graticule_spacing)) {
			while(graticule_row < 16) {
				if(graticule_row>= 0 && graticule_row<=12) {
					g_ui.hilite(graticule_row) = 0x11111111U;
				}
				if(graticule_spacing > 0) {
					graticule_row += graticule_spacing;
				}
				else {
					break;
				}
			}
		}

		// displaying the cursor
		mask = g_ui.bit(m_cursor);
		for(i=0; i<=12; ++i) {
			g_ui.raster(i) &= ~mask;
			g_ui.hilite(i) |= mask;
		}

		// show the play position on the lowest row
		mask = g_ui.bit(layer->m_state.m_play_pos);
		g_ui.raster(15) |= mask;
		g_ui.hilite(15) |= mask;

		// if this is velocity mode then we simply show some text
		if(layer->m_cfg.m_mode == V_SQL_SEQ_MODE_VELOCITY) {
			g_ui.print_text("VEL",5,2,g_ui.RASTER);
		}

		mask = g_ui.bit(0);
		int c = 0;
		int n;

		// determine where the "ruler" will be drawn
		int ruler_from;
		int ruler_to;
		if(m_sel_from >= 0) {
			if(m_sel_to >= m_sel_from) {
				ruler_from = m_sel_from;
				ruler_to = m_sel_to;
			}
			else {
				ruler_to = m_sel_from;
				ruler_from = m_sel_to;
			}
		}
		else {
			ruler_from = layer->m_cfg.m_loop_from;
			ruler_to = layer->m_cfg.m_loop_to;
		}


		// scan over the full 32 columns
		for(i=0; i<32; ++i) {

			// show the "ruler" at the bottom of screen
			if(i >= ruler_from && i <= ruler_to) {
				if(!(c & 0x03)) { // steps 0, 4, 8 etc
					g_ui.raster(15) |= mask;
				}
				else {
					g_ui.hilite(15) |= mask;
				}
				++c;
			}

			CStep step = layer->get_step(i);

			// Display the sequencer steps
			byte show_step = 1;
			switch(layer->m_cfg.m_mode) {
			case V_SQL_SEQ_MODE_CHROMATIC:
			case V_SQL_SEQ_MODE_SCALE:
				show_step = step.m_is_data_point;
				// fall thru
			case V_SQL_SEQ_MODE_TRANSPOSE_ALL:
			case V_SQL_SEQ_MODE_TRANSPOSE_LOCK:
				if(show_step) {
					n = step.m_value;
					n = 12 - n + layer->m_state.m_scroll_ofs;
					if(n >= 0 && n <= 12) {
						if(i == layer->m_state.m_play_pos && m_is_running) {
							g_ui.hilite(n) |= mask;
							g_ui.raster(n) |= mask;
						}
						else {
							g_ui.raster(n) |= mask;
							g_ui.hilite(n) &= ~mask;
						}
					}
				}
				break;
			case V_SQL_SEQ_MODE_MOD:
				n = 12 - step.m_value/10;
				if(n<0) {
					n=0;
				}
				if(i == layer->m_state.m_play_pos && m_is_running) {
					g_ui.raster(n) |= mask;
					g_ui.hilite(n) |= mask;
				}
				else if(step.m_is_data_point) {
					g_ui.raster(n) |= mask;
					g_ui.hilite(n) &= ~mask;

				}
				else {
					g_ui.hilite(n) |= mask;
					g_ui.raster(n) &= ~mask;
				}
				break;
			case V_SQL_SEQ_MODE_VELOCITY:
			case V_SQL_SEQ_MODE_MAX:
				break;
			}


			// DISPLAY THE GATE INFO
			if(step.m_is_accent) {
				g_ui.raster(14) |= mask;
				g_ui.hilite(14) |= mask;
			}
			else if(step.m_is_trigger) {
				g_ui.raster(14) |= mask;
			}
			else if(step.m_is_gate_open) {
				g_ui.hilite(14) |= mask;
			}

			mask>>=1;
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////
	void tick(uint32_t ticks, byte parts_tick) {


		// ensure the sequencer is running
		if(m_is_running) {

			// tick each layer
			for(int i=0; i<NUM_LAYERS; ++i) {
				m_layers[i].tick(ticks, parts_tick);
			}


			// process each layer
			for(int i=0; i<NUM_LAYERS; ++i) {
				CSequenceLayer& layer = m_layers[i];
				if(layer.m_state.m_stepped && layer.m_cfg.m_enabled) {
					switch(layer.m_cfg.m_mode) {

						//////////////////////////////////////////////////
						case V_SQL_SEQ_MODE_MOD:
							layer.action_step_mod(i);
							layer.action_step_gate(i);
							break;

						//////////////////////////////////////////////////
						case V_SQL_SEQ_MODE_TRANSPOSE_ALL:
						case V_SQL_SEQ_MODE_TRANSPOSE_LOCK:
						case V_SQL_SEQ_MODE_VELOCITY:
							layer.action_step_gate(i);
							break;

						//////////////////////////////////////////////////
						case V_SQL_SEQ_MODE_CHROMATIC:
						case V_SQL_SEQ_MODE_SCALE:
							layer.action_step_note(
									i,
									CStep(),
									m_cfg.m_midi_vel_accent,
									m_cfg.m_midi_vel,
									1
							);

							// note layers pass note information to subsequent transpose/velocity layers
							for(int j=i+1; j<NUM_LAYERS; ++j) {
								CSequenceLayer& other_layer = m_layers[j];
								if(other_layer.m_cfg.m_mode == V_SQL_SEQ_MODE_SCALE ||
									other_layer.m_cfg.m_mode == V_SQL_SEQ_MODE_CHROMATIC) {
									// another note layer found - stops current note layer providing any
									// info to further layers
									break;
								}
								else if(other_layer.m_cfg.m_mode == V_SQL_SEQ_MODE_TRANSPOSE_ALL ||
										other_layer.m_cfg.m_mode == V_SQL_SEQ_MODE_TRANSPOSE_LOCK) {
									// transpose layer, action as a note layer passing in the
									// note from the active layer to be transposed
									other_layer.action_step_note(
											j,
											layer.m_state.m_step_value,
											m_cfg.m_midi_vel_accent,
											m_cfg.m_midi_vel,
											0
									);
								}
								else if(other_layer.m_cfg.m_mode == V_SQL_SEQ_MODE_VELOCITY) {
									g_cv_gate.mod_cv(j, layer.m_state.m_last_velocity, other_layer.m_cfg.m_cv_range,0,0);
								}
							}

							break;


						default:
							break;
					}
				}
				layer.manage(i, ticks);
			}
	/*
			for(int i=0; i<NUM_LAYERS; ++i) {
				CSequenceLayer& layer = m_layers[i];
				if(layer.m_stepped) {
					CSequenceLayer::STEP_TYPE step = layer.get_step(i);
					switch(layer.m_cfg.m_mode)
					{
					////////////////////////////////////////////////////////////////////
					case V_SQL_SEQ_MODE_CHROMATIC:
						if(step & CSequenceLayer::IS_ACTIVE) {
							layer.
						}
						break;
					}
				}
			}*/

		}
	}


};

CSequencer g_sequencer;

#endif /* SEQUENCER_H_ */
