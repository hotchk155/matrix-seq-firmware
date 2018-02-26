#ifndef GRID_H_
#define GRID_H_

class CGrid {
	CSequence &m_seq;
	int m_cursor;
	int m_row;
	int m_base_note;
	int m_action;
	int m_popup;
	byte m_layer;
	byte m_flags;
	byte m_copy_mod;
	const uint32_t c_ruler = 0x88888888U;
public:
	enum {
		GRID_WIDTH = 32,
		MAX_CURSOR = 31,
		POPUP_MS = 2000
	};
	enum {
		LAYER_0,
		LAYER_1,
		LAYER_2,
		LAYER_3,
		LAYER_MOD = 0x80
	};
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
	CGrid(CSequence& seq) : m_seq(seq) {
		init();

	}
	void init() {
		m_cursor = 0;
		m_row = 0;
		m_flags = 0;
		m_base_note = 48;
		m_action = ACTION_NONE;
		m_popup = 0;
		m_layer = LAYER_0;
		m_copy_mod = 0;
	}
	void event(int evt, uint32_t param) {
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
				m_seq.notes[m_cursor].note = m_base_note + m_row;
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
				m_seq.notes[m_cursor].mod = m_copy_mod;
				m_popup = 0;
			}
			else {
				if(m_action == ACTION_SET_LOOP) {
					m_seq.set_loop_start(m_cursor);
					m_action = ACTION_DRAG_LOOP;
				}
				if(m_action == ACTION_ERASE) {
					if(m_layer & LAYER_MOD) {
						m_seq.notes[m_cursor].mod = 0;
					}
					else {
						m_seq.notes[m_cursor].note = CSequence::NO_NOTE;
					}
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
				if(m_action == ACTION_MOD_CLONE) {
					m_seq.notes[m_cursor].mod = m_copy_mod;
				}
				else if(m_action == ACTION_NOTE_CLONE) {
					m_seq.notes[m_cursor].note = m_base_note + m_row;
					m_popup = POPUP_MS;
				}
				else if(m_action == ACTION_DRAG_LOOP) {
					m_seq.set_loop_end(m_cursor);
					m_popup = POPUP_MS;
				}
			}
			break;
		case EV_KEY_PRESS:
			switch(param) {
			case KEY_B1:
				if(m_action == ACTION_NONE) {
					if(m_layer & LAYER_MOD) {
						m_copy_mod = m_seq.notes[m_cursor].mod;
						m_popup = 0;
						m_action = ACTION_MOD_DRAG;
					}
					else {
						if(m_seq.notes[m_cursor].note == CSequence::NO_NOTE) {
							m_seq.notes[m_cursor].note = m_base_note + m_row;
						}
						else {
							m_row = m_seq.notes[m_cursor].note - m_base_note;
						}
						m_popup = POPUP_MS;
						m_action = ACTION_NOTE_DRAG;
					}
				}
				break;
			case KEY_B2:
				if(m_action == ACTION_NONE) {
					if(m_layer & LAYER_MOD) {
						m_copy_mod = m_seq.notes[m_cursor].mod;
						m_action = ACTION_MOD_CLONE;
						m_popup = 0;
					}
					else {
						if(m_seq.notes[m_cursor].note != CSequence::NO_NOTE) {
							m_row = m_seq.notes[m_cursor].note - m_base_note;
							m_action = ACTION_NOTE_CLONE;
							m_popup = POPUP_MS;
						}
					}
				}
				break;
			case KEY_B3:
				if(m_action == ACTION_NONE) {
					m_seq.notes[m_cursor].note = CSequence::NO_NOTE;
					m_action = ACTION_ERASE;
				}
				break;
			case KEY_B6:
				if(m_action == ACTION_NONE) {
					m_seq.set_pos(m_cursor);
					m_action = ACTION_SET_LOOP;
				}
				break;
			case KEY_B8:
				if(m_action == ACTION_NONE) {
					m_layer ^= LAYER_MOD;
				}
				break;
			}
			break;
		case EV_KEY_RELEASE:
			m_action = ACTION_NONE;
			break;
		}
	}
	void repaint() {
		int i,j;
		uint32_t mask;

		CRenderBuf::lock();
		CRenderBuf::clear();

		// displaying the cursor
		if(m_layer & LAYER_MOD) {
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

		// the loop points
		mask = CRenderBuf::make_mask(m_seq.m_loop_from, m_seq.m_loop_to + 1);

		CRenderBuf::raster(15) |= (c_ruler & mask);
  		CRenderBuf::hilite(15) |= (~c_ruler & mask);
		mask = CRenderBuf::bit(m_seq.m_play_pos);
		CRenderBuf::raster(15) |= mask;
		CRenderBuf::hilite(15) |= mask;


		if(m_layer & LAYER_MOD) {
			mask = CRenderBuf::bit(0);
			for(i=0; i<32; ++i) {
				int n = (5+m_seq.notes[i].mod)/10;
				if(n>0) {
					CRenderBuf::raster(13-n) |= mask;
				}
				else {
					CRenderBuf::hilite(13) |= mask;
				}

				mask>>=1;
			}
		}
		else {
			mask = CRenderBuf::bit(0);
			for(i=0; i<32; ++i) {
				int n = m_seq.notes[i].note;
				if(n != CSequence::NO_NOTE) {
					n = 12 - n + m_base_note;
					if(n >= 0 && n <= 12) {
						CRenderBuf::raster(n) |= mask;
						if(i == m_seq.m_play_pos) {
							CRenderBuf::set_hilite(n, mask);
						}
						else {
							CRenderBuf::clear_hilite(n, mask);
						}
					}

					if(m_seq.notes[i].mod) {
						CRenderBuf::set_raster(14, mask); // trig
					}
					else {
						CRenderBuf::set_hilite(14, mask); // legato
					}
				}
				mask>>=1;
			}
		}

		if(m_popup) {
			int flags = 0;
			if(m_cursor > 16) {
				flags = CRenderBuf::POPUP_LEFT;
			}
			if(m_action == ACTION_DRAG_LOOP) {
				CRenderBuf::print_number2(1 + m_seq.m_loop_to -m_seq.m_loop_from,flags);
			}
			else {
				CRenderBuf::print_note_name(m_row + m_base_note,flags);
			}
		}


		CRenderBuf::unlock();
	}
	void run() {
		repaint();
		if(m_popup) {
			--m_popup;
		}
	}
};


#endif /* GRID_H_ */
