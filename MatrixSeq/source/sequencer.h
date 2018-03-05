/*
 * sequence.h
 *
 *  Created on: 4 Mar 2018
 *      Author: jason
 */

#ifndef SEQUENCER_H_
#define SEQUENCER_H_

#include <clock.h>
#include "cv_gate.h"
#include "sequence_layer.h"
#include "midi.h"
class CSequencer {
public:

	int m_layer;
	int m_cursor;
	int m_row;
	int m_base_note;
	int m_action;
	int m_popup;
	//byte m_layer;
	byte m_flags;
	uint16_t m_copy_mod;
	char *m_msg;
	static const uint32_t c_ruler = 0x88888888U;

	enum {
		NUM_LAYERS = 4
	};
	CSequenceLayer m_layers[NUM_LAYERS];
	byte m_midi_velocity = 127;
	CSequencer() {
		m_midi_velocity = 127;
		m_layer = 0;
		init();
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
			if(CSequenceLayer::VELOCITY_SEQUENCE == m_layers[i].m_mode) {
				velocity_layer = i; // remember if there is a velocity layer (only 1 allowed)
			}
			if(CSequenceLayer::NOTE_SEQUENCE == m_layers[i].m_mode) {
				note_layer = i; // remember last note layer
			}
			stepped |= m_layers[i].tick(ticks, parts_tick);
		}
		if(!stepped) {
			// if nothing has changed, no need to go further
			return;
		}

		CCVGate::TRANSACTION txn;
		for(int i=0; i<NUM_LAYERS; ++i) {
			CSequenceLayer& layer = m_layers[i];
			txn.flags[i] = 0;
			if(CSequenceLayer::NOTE_SEQUENCE == m_layers[i].m_mode) {
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
				switch(layer.m_mode)
				{
					////////////////////////////////////////////////////////////////////
					case CSequenceLayer::NOTE_SEQUENCE:
					case CSequenceLayer::TRANSPOSE_SEQUENCE:
					{
						int midi_note;
						if(layer.m_mode == CSequenceLayer::TRANSPOSE_SEQUENCE) {
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
								g_midi.send_note(layer.m_midi_channel, layer.m_last_midi_note, 0);
							}
							// play new note and remember it
							layer.m_last_midi_note = (byte)midi_note;
							g_midi.send_note(layer.m_midi_channel, layer.m_last_midi_note, midi_velocity);
						}
						// is the current step LEGATO?
						else if(layer.m_value & CSequenceLayer::IS_ACTIVE) {
							if(layer.m_last_midi_note != (byte)midi_note) {
								g_midi.send_note(layer.m_midi_channel, (byte)midi_note, midi_velocity);
								if(layer.m_last_midi_note) {
									g_midi.send_note(layer.m_midi_channel, layer.m_last_midi_note, 0);
								}
								layer.m_last_midi_note = (byte)midi_note;
							}
						}
						// current step is MUTED
						else {
							if(layer.m_last_midi_note) {
								g_midi.send_note(layer.m_midi_channel, layer.m_last_midi_note, 0);
								layer.m_last_midi_note = 0;
							}
						}
						break;
					}

					////////////////////////////////////////////////////////////////////
					case CSequenceLayer::VELOCITY_SEQUENCE:
						// A velocity layer sends no MIDI out by itself
						txn.flags[i] |= CCVGate::TXN_CV;
						txn.cv[i] = (layer.m_value & 0x7F)<<8;
						break;

					////////////////////////////////////////////////////////////////////
					case CSequenceLayer::MOD_SEQUENCE:
						txn.flags[i] |= CCVGate::TXN_CV;
						txn.cv[i] = (layer.m_value & 0x7F)<<8;
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
		}
	}



public:
	enum {
		GRID_WIDTH = 32,
		MAX_CURSOR = 31,
		POPUP_MS = 2000
	};
	/*enum {
		LAYER_0,
		LAYER_1,
		LAYER_2,
		LAYER_3,
		LAYER_MOD = 0x80
	};*/
	enum {
		ACTION_NONE,
		ACTION_NOTE_DRAG,
		ACTION_MOD_DRAG,
		ACTION_ERASE,
		ACTION_MOD_CLONE,
		ACTION_NOTE_CLONE,
		ACTION_SET_LOOP,
		ACTION_DRAG_LOOP
	};
	void init() {
		m_cursor = 0;
		m_row = 0;
		m_flags = 0;
		m_base_note = 48;
		m_action = ACTION_NONE;
		m_popup = 0;
		m_copy_mod = 0;
		m_msg = nullptr;
	}
	void set_active_layer(int l) {

		char buf[5] = {'1' + l, '-'};
		m_layer = l;
		switch(m_layers[m_layer].m_mode) {
			case CSequenceLayer::NOTE_SEQUENCE: buf[2] = 'N'; buf[3] = 'O'; buf[4] = 'T'; break;
			case CSequenceLayer::MOD_SEQUENCE: buf[2] = 'M'; buf[3] = 'O'; buf[4] = 'D'; break;
			case CSequenceLayer::VELOCITY_SEQUENCE: buf[2] = 'V'; buf[3] = 'E'; buf[4] = 'L'; break;
			case CSequenceLayer::TRANSPOSE_SEQUENCE: buf[2] = 'T'; buf[3] = 'R'; buf[4] = 'N'; break;
		}
		g_popup.align(CPopup::ALIGN_RIGHT);
		g_popup.text(buf,5);
	}
	void event(int evt, uint32_t param) {
		CSequenceLayer *layer = &m_layers[m_layer];
		switch(evt) {
		case EV_ENCODER:
			if(m_action == ACTION_NOTE_DRAG) {
				m_row += (int)param;
				if(m_row < 0) {
					m_row = 0;
				}
				if(m_row > 12) {
					m_row = 12;
				}
				g_popup.note_name(m_base_note + m_row);
				layer->set_step(m_cursor, m_base_note + m_row);
				m_popup = POPUP_MS;
			}
			else if(m_action == ACTION_MOD_DRAG) {
				m_copy_mod += 10 * (int)param;
				if(m_copy_mod < 0) {
					m_copy_mod = 0;
				}
				if(m_copy_mod > 127) {
					m_copy_mod = 127;
				}
				layer->set_step(m_cursor, m_copy_mod);
				m_popup = 0;
			}
			else {
				if(m_action == ACTION_SET_LOOP) {
					layer->set_loop_start(m_cursor);
					m_action = ACTION_DRAG_LOOP;
				}
				if(m_action == ACTION_ERASE) {
					layer->clear_step(m_cursor);
					m_popup = 0;
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
				if(m_action == ACTION_MOD_CLONE) {
					layer->set_step(m_cursor, m_copy_mod);
				}
				else if(m_action == ACTION_NOTE_CLONE) {
					layer->set_step(m_cursor, m_base_note + m_row);
					m_popup = POPUP_MS;
				}
				else if(m_action == ACTION_DRAG_LOOP) {
					layer->set_loop_end(m_cursor);
					m_popup = POPUP_MS;
				}
			}
			break;
		case EV_KEY_PRESS:
			if(m_action == ACTION_NONE) {
				switch(param) {
				case KEY_B1:
					switch(layer->m_mode) {
						case CSequenceLayer::NOTE_SEQUENCE:
							if(!layer->get_step(m_cursor)) {
								layer->set_step(m_cursor, m_base_note + m_row);
							}
							else {
								m_row = layer->get_step(m_cursor) - m_base_note;
							}
							g_popup.avoid(m_cursor);
							g_popup.note_name(m_base_note + m_row);
							m_action = ACTION_NOTE_DRAG;
							break;
						default:
							m_copy_mod = layer->get_step(m_cursor);
							m_popup = 0;
							m_action = ACTION_MOD_DRAG;
					}
					break;
				case KEY_B2:
					switch(layer->m_mode) {
						case CSequenceLayer::NOTE_SEQUENCE:
							m_row = layer->get_step(m_cursor) - m_base_note;
							m_action = ACTION_NOTE_CLONE;
							m_popup = POPUP_MS;
							break;
						default:
							m_copy_mod = layer->get_step(m_cursor);
							m_action = ACTION_MOD_CLONE;
							m_popup = 0;
							break;
					}
					break;
				case KEY_B3:
					layer->clear_step(m_cursor);
					m_action = ACTION_ERASE;
					break;
				case KEY_B6:
					layer->set_pos(m_cursor);
					m_action = ACTION_SET_LOOP;
					break;
				case KEY_L5:
					set_active_layer(0);
					break;
				case KEY_L6:
					set_active_layer(1);
					break;
				case KEY_L7:
					set_active_layer(2);
					break;
				case KEY_L8:
					set_active_layer(3);
					break;
				}
			}
			break;
		case EV_KEY_RELEASE:
			m_action = ACTION_NONE;
			break;
		}
	}
	void repaint() {
		int i;
		uint32_t mask;
		CSequenceLayer *layer = &m_layers[m_layer];

		//CRenderBuf::lock();
		//CRenderBuf::clear();

		// displaying the cursor
		switch(layer->m_mode) {
			case CSequenceLayer::NOTE_SEQUENCE:
				mask = CRenderBuf::bit(m_cursor);
				for(i=0; i<13; ++i) {
					CRenderBuf::raster(i) &= ~mask;
					CRenderBuf::hilite(i) |= mask;
				}
				break;
			default:
				mask = CRenderBuf::bit(m_cursor);
				for(i=0; i<14; ++i) {
					CRenderBuf::raster(i) &= ~mask;
					CRenderBuf::hilite(i) |= mask;
				}
		}

		// the loop points
		mask = CRenderBuf::make_mask(layer->m_loop_from, layer->m_loop_to + 1);

		CRenderBuf::raster(15) |= (c_ruler & mask);
  		CRenderBuf::hilite(15) |= (~c_ruler & mask);
		mask = CRenderBuf::bit(layer->m_play_pos);
		CRenderBuf::raster(15) |= mask;
		CRenderBuf::hilite(15) |= mask;

		// displaying the cursor
		switch(layer->m_mode) {
			case CSequenceLayer::NOTE_SEQUENCE:
				mask = CRenderBuf::bit(0);
				for(i=0; i<32; ++i) {
					int n = SEQ_STEP(layer->m_step[i]);
					if(n) {
						n = 12 - n + m_base_note;
						if(n >= 0 && n <= 12) {
							CRenderBuf::raster(n) |= mask;
							if(i == layer->m_play_pos) {
								CRenderBuf::set_hilite(n, mask);
							}
							else {
								CRenderBuf::clear_hilite(n, mask);
							}
						}

						if(SEQ_GATE(layer->m_step[i])) {
							CRenderBuf::set_raster(14, mask); // trig
						}
						else {
							CRenderBuf::set_hilite(14, mask); // legato
						}
					}
					mask>>=1;
				}
				break;
			default:
				mask = CRenderBuf::bit(0);
				for(i=0; i<32; ++i) {
					int n = (5+SEQ_STEP(layer->m_step[i]))/10;
					if(n>0) {
						CRenderBuf::raster(13-n) |= mask;
					}
					else {
						CRenderBuf::hilite(13) |= mask;
					}
					mask>>=1;
				}
				break;
		}

		/*
		if(m_popup) {
			int flags = 0;
			if(m_cursor > 16) {
				flags = CRenderBuf::POPUP_LEFT;
			}
			switch(m_action) {
			case ACTION_NOTE_DRAG:
			case ACTION_NOTE_CLONE:
				CRenderBuf::print_note_name(m_row + m_base_note,flags);
				break;
			case ACTION_DRAG_LOOP:
				CRenderBuf::print_number2(1 + layer->m_loop_to -layer->m_loop_from,flags);
				break;
			default:
				if(m_msg) {
					CRenderBuf::print_text(m_msg,flags);
				}
				break;
			}
		}
*/

		//CRenderBuf::unlock();
	}
	/*
	void run() {
		repaint();
		if(m_popup) {
			--m_popup;
		}
	}
*/
};

extern CSequencer g_sequencer;
#ifdef MAIN_INCLUDE
	CSequencer g_sequencer;
#endif

#endif /* SEQUENCER_H_ */
