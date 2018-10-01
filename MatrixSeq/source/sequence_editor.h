/*
 * sequence_editor.h
 *
 *  Created on: 30 Sep 2018
 *      Author: jason
 */

#ifndef SEQUENCE_EDITOR_H_
#define SEQUENCE_EDITOR_H_


class CSequenceEditor {
public:

	enum {
		GRID_WIDTH = 32,
		MAX_CURSOR = 31,
		POPUP_MS = 2000
	};
	enum {
		ACTION_NONE,
		ACTION_CREATE_STEP,
		ACTION_EDIT_STEP,
		ACTION_ERASE_STEP,
		ACTION_CLONE_STEP,
		ACTION_SET_LOOP,
		ACTION_DRAG_LOOP,
		ACTION_MOVE_VERT,
		ACTION_MOVE_HORZ
	};


	CSequenceEditor() {
		init_state();
	}

private:
	static const uint32_t c_ruler = 0x88888888U;

	int m_cursor;			// this is the position of the edit cursor
	int m_value;			// value ebign edited in drag mode
	int m_action;

	int m_sel_from;
	int m_sel_to;

	///////////////////////////////////////////////////////////////////////////////
	void init_state() {
		m_cursor = 0;
		m_action = ACTION_NONE;
		m_value = 0;
		m_sel_from = -1;
		m_sel_to = -1;
	}

	///////////////////////////////////////////////////////////////////////////////
	void step_info(CSequenceLayer& layer, CSequenceStep step) {
		byte value = step.m_value;
		switch(layer.get_mode()) {
		case V_SQL_SEQ_MODE_SCALE:
			if(layer.get_mode() == V_SQL_SEQ_MODE_SCALE) {
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
	void scroll(CSequenceLayer& layer, int dir) {
		if(!layer.is_mod_mode()) {
			int scroll_ofs = layer.get_scroll_ofs();
			scroll_ofs += dir;
			if(scroll_ofs < 0) {
				scroll_ofs = 0;
			}
			else if(scroll_ofs > 114) {
				scroll_ofs = 114;
			}
			layer.set_scroll_ofs(scroll_ofs);
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	void set_scroll_for(CSequenceLayer& layer, CSequenceStep step) {
		int v = step.m_value;
		if(v<layer.get_scroll_ofs()) {
			layer.set_scroll_ofs(v);
		}
		else if(v>layer.get_scroll_ofs()+12) {
			layer.set_scroll_ofs(v-12);
		}
	}


	///////////////////////////////////////////////////////////////////////////////
	byte get_graticule(CSequenceLayer& layer, int *baseline, int *spacing) {
		int n;
		switch (layer.get_mode()) {
		case V_SQL_SEQ_MODE_CHROMATIC:
			n = layer.get_scroll_ofs() + 15; // note at top row of screen
			n = 12*(n/12); // C at bottom of that octave
			*baseline = 12 - n + layer.get_scroll_ofs(); // now take scroll offset into account
			*spacing = 12;
			return 1;
		case V_SQL_SEQ_MODE_SCALE:
			n = layer.get_scroll_ofs() + 15; // note at top row of screen
			n = 7*(n/7); // C at bottom of that octave
			*baseline = 12 - n + layer.get_scroll_ofs(); // now take scroll offset into account
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
	void value_action(CSequenceLayer& layer, CSequenceStep& step, ACTION what, byte fine) {
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
	void cursor_action(CSequenceLayer& layer, ACTION what) {
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
		CSequenceStep step = layer.get_step(m_cursor);
		switch(what) {
		////////////////////////////////////////////////
		case ACTION_BEGIN:
			if(!layer.is_note_mode() || step.m_is_data_point) {
				// there is a valid note in this column so we copy it to our
				// paste buffer, scroll it into view and show note name on screen
				set_scroll_for(layer, step);
				if(!layer.is_mod_mode()) {
					step_info(layer, step);
				}
				layer.set_paste_buffer(step);
			}
			break;
		////////////////////////////////////////////////
		case ACTION_HOLD:
			// holding the button down shows the layer id
			g_popup.layer(g_sequencer.get_cur_layer(), layer.get_enabled());
			break;
		////////////////////////////////////////////////
		case ACTION_ENC_LEFT:
		case ACTION_ENC_RIGHT:
			switch(m_edit_keys) {

				// fine edit in mod mode
			case KEY_EDIT|KEY_CLEAR:
				if(layer.is_mod_mode()) {
					// fine adjustment of value. show the new value and copy
					// it to the paste buffer
					value_action(layer, step, what, 1);
					step_info(layer, step);
					layer.set_paste_buffer(step);
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
					value_action(layer, step, what, 0);
					layer.set_paste_buffer(step);
				}
				else {
					// in note mode with no note at current position. we invent one
					// based on scroll offset and insert it into the pattern
					if(layer.is_paste_buffer_empty()) {
						step.reset_all(layer.get_scroll_ofs());
						step.m_is_gate_open = 1;
						step.m_is_trigger = 1;
						layer.set_paste_buffer(step);
					}
				}
				layer.paste_step(m_cursor);
				set_scroll_for(layer, step);
				if(!layer.is_mod_mode()) {
					step_info(layer, step);
				}
				break;
			}
			break;
		////////////////////////////////////////////////
		case ACTION_EDIT_KEYS:
			switch(m_edit_keys) {
			case KEY_EDIT|KEY_PASTE:
				// EDIT + PASTE - advance cursor and copy the current step
				if(!layer.is_paste_buffer_empty()) {
					if(++m_cursor >= CSequenceLayer::MAX_STEPS-1) {
						m_cursor = 0;
					}
					layer.paste_step(m_cursor);
				}
				break;
			case KEY_EDIT|KEY_CLEAR:
				if(layer.is_note_mode()) {
					// EDIT + CLEAR - insert rest and advance cursor
					layer.clear_step_value(m_cursor);
					if(++m_cursor >= CSequenceLayer::MAX_STEPS-1) {
						m_cursor = 0;
					}
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
			layer.paste_step(m_cursor);
			break;
			////////////////////////////////////////////////
		case ACTION_ENC_LEFT:
		case ACTION_ENC_RIGHT:
			// hold-turn pastes multiple notes
			layer.paste_step(m_cursor);
			cursor_action(layer, what);
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
			layer.set_paste_buffer(layer.get_step(m_cursor));
			layer.clear_step_value(m_cursor);
			break;
			////////////////////////////////////////////////
		case ACTION_ENC_LEFT:
		case ACTION_ENC_RIGHT:
			// hold-turn erases multiple notes
			layer.clear_step_value(m_cursor);
			cursor_action(layer, what);
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
			cursor_action(layer, what);
			m_sel_to = m_cursor;
			break;
		////////////////////////////////////////////////
		case ACTION_END:
			if(m_sel_from < 0) {
				layer.set_pos(m_cursor);
			}
			else {
				if(m_sel_to >= m_sel_from) {
					layer.set_loop_from(m_sel_from);
					layer.set_loop_to(m_sel_to);
					layer.set_pos(m_sel_from);
				}
				else {
					layer.set_loop_from(m_sel_to);
					layer.set_loop_to(m_sel_from);
					layer.set_pos(m_sel_to);
				}
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
			scroll(layer, +1);
			break;
		case ACTION_ENC_RIGHT:
			scroll(layer, -1);
			break;
		default:
			break;
		}
	}


	void action(CSequenceLayer& layer, ACTION what) {
		switch(m_action_context) {
		case KEY_EDIT: edit_action(layer, what); break;
		case KEY_PASTE: paste_action(layer, what); break;
		case KEY_CLEAR: clear_action(layer, what); break;
		case KEY_GATE: gate_action(layer, what); break;
		case KEY_LOOP: loop_action(layer, what); break;
		case KEY_MENU: menu_action(layer, what); break;
		}
	}

public:

	/////////////////////////////////////////////////////////////////////////////////////////////
	void repaint() {
		CSequenceLayer& layer = g_sequencer.cur_layer();
		int i;
		uint32_t mask;

		// Clear the display
		g_ui.clear();

		// insert the "graticule", which provides the grid lines to in the background
		// of note based modes
		int graticule_row = 0;
		int graticule_spacing = 0;
		if(get_graticule(layer, &graticule_row, &graticule_spacing)) {
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
		mask = g_ui.bit(layer.get_pos());
		g_ui.raster(15) |= mask;
		g_ui.hilite(15) |= mask;

		// if this is velocity mode then we simply show some text
		if(layer.get_mode() == V_SQL_SEQ_MODE_VELOCITY) {
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
			ruler_from = layer.get_loop_from();
			ruler_to = layer.get_loop_to();
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

			CSequenceStep step = layer.get_step(i);

			// Display the sequencer steps
			byte show_step = 1;
			switch(layer.get_mode()) {
			case V_SQL_SEQ_MODE_CHROMATIC:
			case V_SQL_SEQ_MODE_SCALE:
				show_step = step.m_is_data_point;
				// fall thru
			case V_SQL_SEQ_MODE_TRANSPOSE_ALL:
			case V_SQL_SEQ_MODE_TRANSPOSE_LOCK:
				if(show_step) {
					n = step.m_value;
					n = 12 - n + layer.get_scroll_ofs();
					if(n >= 0 && n <= 12) {
						if(i == layer.get_pos() && g_sequencer.is_running()) {
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
				if(i == layer.get_pos() && g_sequencer.is_running()) {
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

	void event(int evt, uint32_t param) {
		CSequenceLayer& layer = g_sequencer.cur_layer();
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
					action(layer, ACTION_BEGIN);
				}
			}
			else if(m_action_context == KEY_EDIT) {
				m_edit_keys = param;
				action(layer, ACTION_EDIT_KEYS);
			}
			break;
		case EV_KEY_RELEASE:
			if(param & m_edit_keys) {
				m_edit_keys = 0;
			}
			if(param == m_action_context) {
				action(layer, ACTION_END);
				m_action_context = 0;
			}
			break;
		case EV_KEY_HOLD:
			if(param == m_action_context) {
				action(layer, ACTION_HOLD);
			}
			break;
		case EV_KEY_CLICK:
			if(param == m_action_context) {
				action(layer, ACTION_CLICK);
			}
			break;
		case EV_ENCODER:
			if(m_action_context) {
				if((int)param<0) {
					action(layer, ACTION_ENC_LEFT);
				}
				else {
					action(layer, ACTION_ENC_RIGHT);
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

};
CSequenceEditor g_sequence_editor;


#endif /* SEQUENCE_EDITOR_H_ */
