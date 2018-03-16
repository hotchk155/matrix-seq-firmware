///////////////////////////////////////////////////////////////////////////////
// MATRIX SEQUENCER
// Sixty four pixels Ltd	March 2018
//
// SEQUENCER

#ifndef SEQUENCER_H_
#define SEQUENCER_H_

#include "matrix_seq.h"


///////////////////////////////////////////////////////////////////////////////
// SEQUENCER CLASS
class CSequencer {
public:

	enum {
		NUM_LAYERS = 4,	// number of layers in the sequence
		GRID_WIDTH = 32,
		MAX_CURSOR = 31,
		POPUP_MS = 2000
	};
	enum {
		ACTION_NONE,
		ACTION_EDIT_STEP,
		ACTION_ERASE_STEP,
		ACTION_CLONE_STEP,
		ACTION_SET_LOOP,
		ACTION_DRAG_LOOP
	};

	static const uint32_t c_ruler = 0x88888888U;

	// Config info that forms part of the patch
	typedef struct {
		V_SQL_SCALE_TYPE scale_type;
	} CONFIG;
	CONFIG m_cfg;

	CSequenceLayer m_layers[NUM_LAYERS];
	CSequencer() {
		init_config();
		init_state();
	}

	int m_layer;			// this is the current layer being viewed/edited
	int m_cursor;			// this is the position of the edit cursor
	int m_row;
	int m_action;
	byte m_flags;
	CSequenceLayer::STEP_TYPE m_copy_step;


	///////////////////////////////////////////////////////////////////////////////
	void init_config() {
		m_cfg.scale_type = V_SQL_SCALE_TYPE_IONIAN;
	}

	///////////////////////////////////////////////////////////////////////////////
	void init_state() {
		m_cursor = 0;
		m_row = 0;
		m_flags = 0;
		m_action = ACTION_NONE;
		//m_popup = 0;
		m_copy_step = 0;
	}

	///////////////////////////////////////////////////////////////////////////////
	void set(PARAM_ID param, int value) {
		CSequenceLayer::CONFIG& cfg = m_layers[m_layer].m_cfg;
		switch(param) {
		case P_SQL_SEQ_MODE: cfg.m_mode = (V_SQL_SEQ_MODE)value; break;
		case P_SQL_STEP_RATE: cfg.m_step_rate = (V_SQL_STEP_RATE)value; break;
		case P_SQL_MIDI_CHAN: cfg.m_midi_channel = (V_SQL_MIDI_CHAN)value; break;
		case P_SQL_MIDI_CC: cfg.m_midi_cc = value; break;
		case P_SQL_SCALE_TYPE: m_cfg.scale_type = (V_SQL_SCALE_TYPE)value; break;
		default: break;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	int get(PARAM_ID param) {
		CSequenceLayer::CONFIG& cfg = m_layers[m_layer].m_cfg;
		switch(param) {
		case P_SQL_SEQ_MODE: return cfg.m_mode;
		case P_SQL_STEP_RATE: return cfg.m_step_rate;
		case P_SQL_MIDI_CHAN: return cfg.m_midi_channel;
		case P_SQL_MIDI_CC: return cfg.m_midi_cc;
		case P_SQL_SCALE_TYPE: return m_cfg.scale_type;
		default:return 0;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	void activate() {

	}

	///////////////////////////////////////////////////////////////////////////////
	void set_active_layer(int l) {
		m_layer = l;
	}

	///////////////////////////////////////////////////////////////////////////////
	void step_info(CSequenceLayer::STEP_TYPE step, CSequenceLayer *layer) {
		if(layer->is_note_mode()) {
			g_popup.note_name(STEP_VALUE(step));
			g_popup.avoid(m_cursor);
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	void set_scroll_for(CSequenceLayer::STEP_TYPE step, CSequenceLayer *layer) {
		int v = STEP_VALUE(step);
		if(v<layer->m_scroll_ofs) {
			layer->m_scroll_ofs = v;
		}
		else if(v>layer->m_scroll_ofs+12) {
			layer->m_scroll_ofs = v-12;
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
			if(m_action == ACTION_EDIT_STEP) {
				m_copy_step = layer->get_step(m_cursor);
				layer->inc_step(&m_copy_step, (int)param);
				layer->set_step(m_cursor, m_copy_step);
				set_scroll_for(m_copy_step, layer);
				step_info(m_copy_step, layer);
			}
			else {
				if(m_action == ACTION_SET_LOOP) {
					layer->set_loop_start(m_cursor);
					m_action = ACTION_DRAG_LOOP;
				}
				if(m_action == ACTION_ERASE_STEP) {
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
			if(m_action == ACTION_EDIT_STEP && param == KEY_B2) {
				step = layer->get_step(m_cursor);
				if(!layer->is_note_mode() || (step & CSequenceLayer::IS_ACTIVE)) {
					layer->set_step(m_cursor, step ^ CSequenceLayer::IS_TRIG);
				}
			}
			if(m_action == ACTION_NONE) {
				switch(param) {

				//////////////////////////////////////////////////////////
				// STEP EDIT
				case KEY_B1:
					// check if there is a step in this column
					step = layer->get_step(m_cursor);
					if(layer->is_note_mode() && !(step & CSequenceLayer::IS_ACTIVE)) {

						// no active note step - do we have a previous step to copy?
						if(!m_copy_step) {
							// nope so simply create a step at the bottom of the active range
							m_copy_step = layer->m_scroll_ofs | CSequenceLayer::IS_ACTIVE | CSequenceLayer::IS_TRIG;
						}

						// store the step
						layer->set_step(m_cursor, m_copy_step);
						step = m_copy_step;
					}

					set_scroll_for(step, layer);
					step_info(step, layer);
					// we are now in edit step mode!
					m_action = ACTION_EDIT_STEP;
					break;


				case KEY_B2:
					step = layer->get_step(m_cursor);
					if(m_action == ACTION_EDIT_STEP && (!layer->is_note_mode() || (step & CSequenceLayer::IS_ACTIVE))) {
						layer->set_step(m_cursor, step ^ CSequenceLayer::IS_TRIG);
					}

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
			if(m_action == ACTION_EDIT_STEP && param == KEY_B2) {
				break;
			}
			m_action = ACTION_NONE;
			break;
                                                                                         		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////
	void repaint() {
		int i;
		uint32_t mask;
		CSequenceLayer *layer = &m_layers[m_layer];

		CRenderBuf::clear();

		// displaying the cursor
		if(layer->m_cfg.m_mode) {
			mask = CRenderBuf::bit(m_cursor);
			for(i=0; i<14; ++i) {
				CRenderBuf::raster(i) &= ~mask;
				CRenderBuf::hilite(i) |= mask;
			}
		}
		else {
			mask = CRenderBuf::bit(m_cursor);
			for(i=0; i<13; ++i) {
				CRenderBuf::raster(i) &= ~mask;
				CRenderBuf::hilite(i) |= mask;
			}
		}


		mask = CRenderBuf::bit(layer->m_play_pos);
		CRenderBuf::raster(15) |= mask;
		CRenderBuf::hilite(15) |= mask;

		mask = CRenderBuf::bit(0);
		int c = 0;
		for(i=0; i<32; ++i) {
			if(i >= layer->m_cfg.m_loop_from && i <= layer->m_cfg.m_loop_to) {
				if(!(c & 0x03)) {
					CRenderBuf::raster(15) |= mask;
				}
				else {
					CRenderBuf::hilite(15) |= mask;
				}
				++c;
			}
			CSequenceLayer::STEP_TYPE step = layer->get_step(i);
			if(step & CSequenceLayer::IS_ACTIVE) {
				int n = STEP_VALUE(step);
				n = 12 - n + layer->m_scroll_ofs;
				if(n >= 0 && n <= 12) {
					CRenderBuf::raster(n) |= mask;
					if(i == layer->m_play_pos) {
						CRenderBuf::set_hilite(n, mask);
					}
					else {
						CRenderBuf::clear_hilite(n, mask);
					}
				}
				if(step & CSequenceLayer::IS_TRIG) {
					CRenderBuf::set_raster(14, mask); // trig
				}
				else {
					CRenderBuf::set_hilite(14, mask); // legato
				}
			}
			mask>>=1;
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////
	void tick(uint32_t ticks, byte parts_tick) {

		//byte midi_velocity = m_midi_velocity;
		//uint16_t midi_note_action[NUM_LAYERS];
		int note_layer = -1;
		int velocity_layer = -1;
		byte stepped = 0;

		// run the clock on each sequencer layer
		for(int i=0; i<NUM_LAYERS; ++i) {
//			if(V_SQL_SEQ_MODE_VEL == m_layers[i].m_cfg.m_mode) {
				//velocity_layer = i; // remember if there is a velocity layer (only 1 allowed)
			//}
			//if(V_SQL_SEQ_MODE_NOTE == m_layers[i].m_cfg.m_mode) {
				//note_layer = i; // remember last note layer
			//}
			stepped |= m_layers[i].tick(ticks, parts_tick);
		}
		if(!stepped) {
			// if nothing has changed, no need to go further
			return;
		}
/*
		CCVGate::TRANSACTION txn;
		for(int i=0; i<NUM_LAYERS; ++i) {
			CSequenceLayer& layer = m_layers[i];
			txn.flags[i] = 0;
			if(V_SQL_SEQ_MODE_NOTE == m_layers[i].m_cfg.m_mode) {
				note_layer = i; // preceding note layer for a transpose layer
			}

			// has there been a step on this sequence in this millisecond?
			if(layer.m_stepped) {

				// is this a trigger step?
				if(layer.m_value & CSequenceLayer::IS_TRIG) {
					txn.flags[i] |= CCVGate::TXN_TRIG;
				}
				// is this an active (gate remain open) step?
				else if(layer.m_value & CSequenceLayer::IS_ACTIVE) {
					txn.flags[i] |= CCVGate::TXN_GATE;
				}

				// what type of layer?
				switch(layer.m_cfg.m_mode)
				{
					////////////////////////////////////////////////////////////////////
					case V_SQL_SEQ_MODE_NOTE:
					//case V_SQL_SEQ_MODE_TRANSPOSE:
					{
						int midi_note;
						if(layer.m_cfg.m_mode == V_SQL_SEQ_MODE_TRANSPOSE) {
							if(note_layer >= 0) {
								// get note from the note layer and transpose it
								midi_note = (byte)m_layers[note_layer].m_value;
								midi_note += ((int)((byte)layer.m_value)-64);
								midi_note = layer.force_to_scale(midi_note);
							}
							else {
								// if there is no note layer then there will be no
								// CV or MIDI from a transpose layer...
								break;
							}
						}
						else {
							midi_note = (byte)layer.m_value;
						}

						// decide MIDI velocity
						byte midi_velocity;
						if(velocity_layer >= 0) {
							midi_velocity = (byte)m_layers[velocity_layer].m_value;
						}
						else {
							midi_velocity = m_midi_velocity;
						}

						// gonna send us some CV...
						txn.flags[i] |= CCVGate::TXN_PITCH;
						txn.cv[i] = ((uint16_t)(midi_note))<<8;

						// Now sort out MIDI...
						// is the current step a RETRIG?
						if(layer.m_value & CSequenceLayer::IS_TRIG) {
							if(layer.m_last_midi_note) {
								// stop previous note
								g_midi.send_note(layer.m_cfg.m_midi_channel, layer.m_last_midi_note, 0);
							}
							// play new note and remember it
							layer.m_last_midi_note = (byte)midi_note;
							g_midi.send_note(layer.m_cfg.m_midi_channel, layer.m_last_midi_note, midi_velocity);
						}
						// is the current step LEGATO?
						else if(layer.m_value & CSequenceLayer::IS_ACTIVE) {
							if(layer.m_last_midi_note != (byte)midi_note) {
								g_midi.send_note(layer.m_cfg.m_midi_channel, (byte)midi_note, midi_velocity);
								if(layer.m_last_midi_note) {
									g_midi.send_note(layer.m_cfg.m_midi_channel, layer.m_last_midi_note, 0);
								}
								layer.m_last_midi_note = (byte)midi_note;
							}
						}
						// current step is MUTED
						else {
							if(layer.m_last_midi_note) {
								g_midi.send_note(layer.m_cfg.m_midi_channel, layer.m_last_midi_note, 0);
								layer.m_last_midi_note = 0;
							}
						}
						break;
					}

					////////////////////////////////////////////////////////////////////
					//case V_SQL_SEQ_MODE_VEL:
					//	// A velocity layer sends no MIDI out by itself
					//	txn.flags[i] |= CCVGate::TXN_CV;
					//	txn.cv[i] = (layer.m_value & 0x7F)<<8;
					//	break;

					////////////////////////////////////////////////////////////////////
					case V_SQL_SEQ_MODE_MOD:
						txn.flags[i] |= CCVGate::TXN_CV;
						txn.cv[i] = (layer.m_value & 0x7F)<<8;
						break;

					default:
						break;
				}
			}
		}

		// if there are any actions then
		for(int i=0; i<NUM_LAYERS; ++i) {
			if(txn.flags[i]) {
				//
				g_cv_gate.set(txn);
				break;
			}
		}*/
	}

};

extern CSequencer g_sequencer;
#ifdef MAIN_INCLUDE
	CSequencer g_sequencer;
#endif

#endif /* SEQUENCER_H_ */
