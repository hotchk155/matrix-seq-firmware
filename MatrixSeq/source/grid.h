#ifndef GRID_H_
#define GRID_H_

class CGrid {
	CSequence &m_seq;
	int m_cursor;
	int m_row;
	int m_base_note;
	int m_action;
	int m_popup;
	byte m_flags;
public:
	enum {
		GRID_WIDTH = 32,
		MAX_CURSOR = 31,
		POPUP_MS = 2000
	};
	enum {
		ACTION_NONE,
		ACTION_NOTE_DRAG,
		ACTION_ERASE,
		ACTION_CLONE
	};
	CGrid(CSequence& seq) : m_seq(seq) {
		init();

	}
	void init() {
		m_cursor = 0;
		m_row = 0;
		m_flags = 0;
		m_base_note = 0;
		m_action = ACTION_NONE;
		m_popup = 0;
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
			else {
				if(m_action == ACTION_ERASE) {
					m_seq.notes[m_cursor].note = CSequence::NO_NOTE;
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
				if(m_action == ACTION_CLONE) {
					m_seq.notes[m_cursor].note = m_base_note + m_row;
					m_popup = POPUP_MS;
				}
			}
			break;
		case EV_KEY_PRESS:
			switch(param) {
			case KEY_B1:
				if(m_action == ACTION_NONE) {
					if(m_seq.notes[m_cursor].note == CSequence::NO_NOTE) {
						m_seq.notes[m_cursor].note = m_base_note + m_row;
					}
					else {
						m_row = m_seq.notes[m_cursor].note - m_base_note;
					}
					m_popup = POPUP_MS;
					m_action = ACTION_NOTE_DRAG;
				}
				break;
			case KEY_B2:
				if(m_action == ACTION_NONE) {
					if(m_seq.notes[m_cursor].note != CSequence::NO_NOTE) {
						m_row = m_seq.notes[m_cursor].note - m_base_note;
						m_action = ACTION_CLONE;
						m_popup = POPUP_MS;
					}
				}
				break;
			case KEY_B3:
				if(m_action == ACTION_NONE) {
					m_seq.notes[m_cursor].note = CSequence::NO_NOTE;
					m_action = ACTION_ERASE;
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
		int i;
		uint32_t mask;

		CRenderBuf::lock();
		CRenderBuf::clear();

		mask = CRenderBuf::bit(m_cursor);
		for(i=0; i<13; ++i) {
			CRenderBuf::raster(i) &= ~mask;
			CRenderBuf::hilite(i) |= mask;
		}

		CRenderBuf::raster(15) |= CRenderBuf::make_mask(0, m_seq.m_play_length);
		mask = CRenderBuf::bit(m_seq.m_play_pos);
		CRenderBuf::hilite(15) |= mask;


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

				if(m_seq.notes[i].vel) {
					CRenderBuf::set_raster(14, mask); // trig
				}
				else {
					CRenderBuf::set_hilite(14, mask); // legato
				}
			}
			mask>>=1;
		}

		if(m_popup) {
			if(m_cursor < 16) {
				CRenderBuf::print_note_name(m_row + m_base_note,0);
			}
			else {
				CRenderBuf::print_note_name(m_row + m_base_note,CRenderBuf::NOTE_NAME_LEFT);
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
