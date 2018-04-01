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
		ACTION_EDIT_TRIG,
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
		byte 			m_midi_vel_hi;
		byte 			m_midi_vel_med;
		byte 			m_midi_vel_lo;
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
	CSequenceLayer::STEP_TYPE m_copy_step;



	///////////////////////////////////////////////////////////////////////////////
	void init_config() {
		m_cfg.scale_type = V_SQL_SCALE_TYPE_IONIAN;
		m_cfg.scale_root = V_SQL_SCALE_ROOT_C;
		m_cfg.m_midi_vel_hi = 127;
		m_cfg.m_midi_vel_med = 100;
		m_cfg.m_midi_vel_lo = 50;
	}

	///////////////////////////////////////////////////////////////////////////////
	void init_state() {
		m_cursor = 0;
		//m_row = 0;
		m_flags = 0;
		m_action = ACTION_NONE;
		//m_popup = 0;
		m_copy_step = 0;
		m_value = 0;
		m_scale_size = 0;
		m_is_running = 0;
	}

	///////////////////////////////////////////////////////////////////////////////
	void set(PARAM_ID param, int value) {
		switch(param) {
		case P_SQL_SCALE_TYPE: m_cfg.scale_type = (V_SQL_SCALE_TYPE)value; break;
		case P_SQL_SCALE_ROOT: m_cfg.scale_root = (V_SQL_SCALE_ROOT)value; break;
		case P_SQL_MIDI_VEL_HI: m_cfg.m_midi_vel_hi = value; break;
		case P_SQL_MIDI_VEL_MED: m_cfg.m_midi_vel_med = value; break;
		case P_SQL_MIDI_VEL_LO: m_cfg.m_midi_vel_lo = value; break;
		default: m_layers[m_layer].set(param,value);
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	int get(PARAM_ID param) {
		switch(param) {
		case P_SQL_SCALE_TYPE: return m_cfg.scale_type;
		case P_SQL_SCALE_ROOT: return m_cfg.scale_root;
		case P_SQL_MIDI_VEL_HI: return m_cfg.m_midi_vel_hi;
		case P_SQL_MIDI_VEL_MED: return m_cfg.m_midi_vel_med;
		case P_SQL_MIDI_VEL_LO: return m_cfg.m_midi_vel_lo;
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
			case V_SQL_SEQ_MODE_TRANSPOSE:
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
		case P_SQL_MIDI_VEL_HI:
		case P_SQL_MIDI_VEL_MED:
		case P_SQL_MIDI_VEL_LO:
			switch(m_layers[m_layer].m_cfg.m_mode) {
			case V_SQL_SEQ_MODE_CHROMATIC:
			case V_SQL_SEQ_MODE_TRANSPOSE:
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
	void step_info(CSequenceLayer::STEP_TYPE step, CSequenceLayer *layer) {
		byte value = STEP_VALUE(step);
		switch(layer->m_cfg.m_mode) {
		case V_SQL_SEQ_MODE_SCALE:
			if(layer->m_cfg.m_mode == V_SQL_SEQ_MODE_SCALE) {
				value = g_scale.index_to_note(value);
			}
			// fall thru
		case V_SQL_SEQ_MODE_CHROMATIC:
			g_popup.note_name(value);
			break;
		case V_SQL_SEQ_MODE_TRANSPOSE:
			g_popup.show_offset(((int)value)-64);
			break;
		case V_SQL_SEQ_MODE_MOD:
		case V_SQL_SEQ_MODE_VELOCITY:
		case V_SQL_SEQ_MODE_MAX:
			break;
		}
		g_popup.avoid(m_cursor);
	}

	///////////////////////////////////////////////////////////////////////////////
	void scroll(int dist) {
		int scroll_ofs = m_layers[m_layer].m_state.m_scroll_ofs + dist;
		if(scroll_ofs < 0) {
			scroll_ofs = 0;
		}
		else if(scroll_ofs > 114) {
//TODO: for each type of layer
			scroll_ofs = 114;
		}
		m_layers[m_layer].m_state.m_scroll_ofs = scroll_ofs;
	}

	///////////////////////////////////////////////////////////////////////////////
	void set_scroll_for(CSequenceLayer::STEP_TYPE step, CSequenceLayer *layer) {
		int v = STEP_VALUE(step);
		if(v<layer->m_state.m_scroll_ofs) {
			layer->m_state.m_scroll_ofs = v;
		}
		else if(v>layer->m_state.m_scroll_ofs+12) {
			layer->m_state.m_scroll_ofs = v-12;
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////
	void event(int evt, uint32_t param) {
		CSequenceLayer *layer = &m_layers[m_layer];
		CSequenceLayer::STEP_TYPE step;
		switch(evt) {
		/////////////////////////////////////////////////////////////////////////////////////////////
		// ENCODER INC/DEC
		case EV_ENCODER:
			if(m_action == ACTION_CREATE_STEP) {
				m_copy_step = layer->get_step(m_cursor) | CSequenceLayer::IS_ACTIVE;
				layer->set_step(m_cursor, m_copy_step);
				set_scroll_for(m_copy_step, layer);
				step_info(m_copy_step, layer);
				m_action = ACTION_EDIT_STEP;
			}
			else if(m_action == ACTION_EDIT_STEP) {
				m_copy_step = layer->get_step(m_cursor);
				layer->inc_step(&m_copy_step, (int)param);
				layer->set_step(m_cursor, m_copy_step);
				set_scroll_for(m_copy_step, layer);
				step_info(m_copy_step, layer);
			}
			else if(m_action == ACTION_EDIT_TRIG) {
				CSequenceLayer::STEP_TYPE step = layer->get_step(m_cursor);
				int vel = CSequenceLayer::get_velocity(step);
				if(vel > CSequenceLayer::VELOCITY_OFF && ((int)param) < 0) {
					CSequenceLayer::set_velocity(step, vel-1);
				}
				else if(vel < CSequenceLayer::VELOCITY_HIGH && ((int)param) > 0) {
					CSequenceLayer::set_velocity(step, vel+1);
				}
				layer->set_step(m_cursor, step);
			}
			else if(m_action == ACTION_MOVE_VERT) {
				if(layer->vertical_move((int)param)) {
					m_value += (int)param;
				}
				g_popup.show_offset(m_value);
			}
			else if(m_action == ACTION_MOVE_HORZ) {
				if((int)param < 0 && m_value > -31) {
					layer->shift_left();
					--m_value;
				}
				else if((int)param > 0 && m_value < 31) {
					layer->shift_right();
					++m_value;
				}
				g_popup.show_offset(m_value);
			}
			else {
				if(m_action == ACTION_SET_LOOP) {
					layer->set_loop_start(m_cursor);
					m_action = ACTION_DRAG_LOOP;
				}
				else if(m_action == ACTION_ERASE_STEP) {
					layer->clear_step(m_cursor);
				}
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
				if(m_action == ACTION_CLONE_STEP) {
					layer->set_step(m_cursor, m_copy_step);
				}
				else if(m_action == ACTION_DRAG_LOOP) {
					layer->set_loop_end(m_cursor);
				}
			}
			break;
		/////////////////////////////////////////////////////////////////////////////////////////////
		// BUTTON PRESS
		case EV_KEY_PRESS:
			if(m_action == ACTION_EDIT_STEP || m_action == ACTION_CREATE_STEP) {
				switch(param) {
				case KEY_B2:
//					step = layer->get_step(m_cursor);
//					if(!layer->is_note_mode() || (step & CSequenceLayer::IS_ACTIVE)) {
//						layer->set_step(m_cursor, step ^ CSequenceLayer::IS_TRIG);
//					}
					m_action = ACTION_EDIT_TRIG;
					break;
				case KEY_B3:
					m_value = 0;
					g_popup.show_offset(m_value);
					m_action = ACTION_MOVE_VERT;
					break;
				case KEY_B4:
					m_value = 0;
					g_popup.show_offset(m_value);
					m_action = ACTION_MOVE_HORZ;
					break;
				}
			}
			else if(m_action == ACTION_NONE) {
				switch(param) {

				//////////////////////////////////////////////////////////
				// STEP EDIT
				case KEY_B1:
					// check if there is a step in this column
					step = layer->get_step(m_cursor);
					if(step & CSequenceLayer::IS_ACTIVE) {
						set_scroll_for(step, layer);
						step_info(step, layer);
						m_action = ACTION_EDIT_STEP;
					}
					else {
						// no active note step - do we have a previous step to copy?
						if(m_copy_step) {
							layer->set_step(m_cursor, STEP_VALUE(m_copy_step)|CSequenceLayer::IS_VEL1);
						}
						else {
							layer->set_step(m_cursor, layer->m_state.m_scroll_ofs|CSequenceLayer::IS_VEL1);
						}
						m_action = ACTION_CREATE_STEP;
					}
					break;


				case KEY_B2:
					step = layer->get_step(m_cursor);
					m_copy_step = layer->get_step(m_cursor);
					if(!layer->is_note_mode() || (step & CSequenceLayer::IS_ACTIVE)) {
						m_action = ACTION_CLONE_STEP;
					}
					break;
				case KEY_B3:
					layer->clear_step(m_cursor);
					m_action = ACTION_ERASE_STEP;
					break;
				case KEY_B4:
					layer->set_pos(m_cursor);
					m_action = ACTION_SET_LOOP;
					break;
				}

			} // if m_action == ACTION_NONE
			break;

		/////////////////////////////////////////////////////////////////////////////////////////////
		// BUTTON RELEASE
		case EV_KEY_RELEASE:
			if(m_action == ACTION_EDIT_TRIG && param == KEY_B2) {
				m_action = ACTION_EDIT_STEP;
			}
			else {
				m_action = ACTION_NONE;
			}
			break;
                                                                                         		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////
	void repaint() {
		int i;
		uint32_t mask;
		CSequenceLayer *layer = &m_layers[m_layer];

		CRenderBuf::clear();

		int max_row = (m_action == ACTION_EDIT_TRIG)? 11 : 12;

		int graticule_row = 0;
		int graticule_spacing = 0;
		if(layer->get_graticule(&graticule_row, &graticule_spacing)) {
			while(graticule_row < 16) {
				if(graticule_row>= 0 && graticule_row<=max_row) {
					CRenderBuf::hilite(graticule_row) = 0x11111111U;
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
		mask = CRenderBuf::bit(m_cursor);
		for(i=0; i<=max_row; ++i) {
			CRenderBuf::raster(i) &= ~mask;
			CRenderBuf::hilite(i) |= mask;
		}

		mask = CRenderBuf::bit(layer->m_state.m_play_pos);
		CRenderBuf::raster(15) |= mask;
		CRenderBuf::hilite(15) |= mask;

		// if this is velocity mode then we simply show some text
		if(layer->m_cfg.m_mode == V_SQL_SEQ_MODE_VELOCITY) {
			CRenderBuf::print_text("VEL",5,2);
		}

		mask = CRenderBuf::bit(0);
		int c = 0;
		int n;

		for(i=0; i<32; ++i) {

			// show the "ruler" at the bottom of screen
			if(i >= layer->m_cfg.m_loop_from && i <= layer->m_cfg.m_loop_to) {
				if(!(c & 0x03)) { // steps 0, 4, 8 etc
					CRenderBuf::raster(15) |= mask;
				}
				else {
					CRenderBuf::hilite(15) |= mask;
				}
				++c;
			}

			CSequenceLayer::STEP_TYPE step = layer->get_step(i);

			// Display the sequencer steps
			byte show_step = 1;
			switch(layer->m_cfg.m_mode) {
			case V_SQL_SEQ_MODE_CHROMATIC:
			case V_SQL_SEQ_MODE_SCALE:
				show_step = !!(step & CSequenceLayer::IS_ACTIVE);	// note nodes hide the note unless a trigger present
				// fall thru
			case V_SQL_SEQ_MODE_TRANSPOSE:
				if(show_step) {
					n = STEP_VALUE(step);
					n = 12 - n + layer->m_state.m_scroll_ofs;
					if(n >= 0 && n <= max_row) {
						CRenderBuf::raster(n) |= mask;
						if(i == layer->m_state.m_play_pos && m_is_running) {
							CRenderBuf::set_hilite(n, mask);
						}
						else {
							CRenderBuf::clear_hilite(n, mask);
						}
					}
				}
				break;
			case V_SQL_SEQ_MODE_MOD:
				n = STEP_VALUE(step);
				if(n>CSequenceLayer::MAX_MOD_VALUE) {
					n=0;
				}
				else {
					n=CSequenceLayer::MAX_MOD_VALUE-n;
				}
//				for(int j=n; j<=13; ++j) {
//					CRenderBuf::raster(j) &= ~mask;
//					CRenderBuf::hilite(j) |= mask;
//				}
				if(i == layer->m_state.m_play_pos && m_is_running) {
					CRenderBuf::raster(n) |= mask;
					CRenderBuf::hilite(n) |= mask;
				}
				else {
					CRenderBuf::raster(n) |= mask;
					CRenderBuf::hilite(n) &= ~mask;

				}
				break;
			case V_SQL_SEQ_MODE_VELOCITY:
			case V_SQL_SEQ_MODE_MAX:
				break;
			}

			// Display the sequencer steps
			int vel;
			switch(layer->m_cfg.m_mode) {
			case V_SQL_SEQ_MODE_CHROMATIC:
			case V_SQL_SEQ_MODE_SCALE:
			case V_SQL_SEQ_MODE_MOD:
				if(step & CSequenceLayer::IS_ACTIVE) {
					vel = CSequenceLayer::get_velocity(step);
					if(m_action == ACTION_EDIT_TRIG) {
						if(vel == CSequenceLayer::VELOCITY_HIGH) {
							CRenderBuf::set_raster(12, mask);
						}
						else {
							CRenderBuf::set_hilite(12, mask);
						}
						if(vel == CSequenceLayer::VELOCITY_MEDIUM) {
							CRenderBuf::set_raster(13, mask);
						}
						else {
							CRenderBuf::set_hilite(13, mask);
						}
						if(vel == CSequenceLayer::VELOCITY_LOW) {
							CRenderBuf::set_raster(14, mask);
						}
						else {
							CRenderBuf::set_hilite(14, mask);
						}
					}
					else {
						if(vel >= CSequenceLayer::VELOCITY_LOW) {
							CRenderBuf::set_raster(14, mask);
						}
						else {
							CRenderBuf::set_hilite(14, mask);
						}
					}
				}
				break;
			default:
				break;
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
						case V_SQL_SEQ_MODE_TRANSPOSE:
						case V_SQL_SEQ_MODE_VELOCITY:
							layer.action_step_gate(i);
							break;

						//////////////////////////////////////////////////
						case V_SQL_SEQ_MODE_CHROMATIC:
						case V_SQL_SEQ_MODE_SCALE:
							layer.action_step_note(
									i,
									0,
									m_cfg.m_midi_vel_hi,
									m_cfg.m_midi_vel_med,
									m_cfg.m_midi_vel_lo,
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
								else if(other_layer.m_cfg.m_mode == V_SQL_SEQ_MODE_TRANSPOSE) {
									// transpose layer, action as a note layer passing in the
									// note from the active layer to be transposed
									other_layer.action_step_note(
											j,
											layer.m_state.m_step_value,
											m_cfg.m_midi_vel_hi,
											m_cfg.m_midi_vel_med,
											m_cfg.m_midi_vel_lo,
											0
									);
								}
								else if(other_layer.m_cfg.m_mode == V_SQL_SEQ_MODE_VELOCITY) {
									g_cv_gate.mod_cv(j, layer.m_state.m_last_velocity, other_layer.m_cfg.m_cv_range);
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

extern CSequencer g_sequencer;
#ifdef MAIN_INCLUDE
	CSequencer g_sequencer;
#endif

#endif /* SEQUENCER_H_ */
