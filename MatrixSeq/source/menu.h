/*
 * menu.h
 *
 *  Created on: 12 Mar 2018
 *      Author: jason
 */

#ifndef MENU_H_
#define MENU_H_



class CMenu {
public:
	typedef struct {
		const char *const prompt;
		const PARAM_ID param;
		const uint8_t mode;
		const char *const values;
	} OPTION;
	enum {
		ENUMERATED = 1,
		MIDI_CHANNEL,
		NUMBER_7BIT,
		VOLT_RANGE,
		NUM_OPTS = 6,

		ACTION_NONE = 0,
		ACTION_VALUE_SELECTED = 1,
		ACTION_VALUE_CHANGED = 2
	};
	static const CMenu::OPTION m_opts[NUM_OPTS];

	byte m_item;
	byte m_value;
	byte m_repaint;
	byte m_action;
	CMenu() {
		m_item = 0;
		m_repaint = 1;
		m_action = ACTION_NONE;
	}
	/*

	 00     Y
	 01     YY
	 02     Y
	 03     YYYY
	 04
	 05		XXXXX
	 06     X
	 07     XXX
	 08     X
	 09     XXXX
	 10
	 11     ZZZZ
	 12     Z
	 13     ZZZ
	 14     Z


	 */
	void format_number(int value, char *buf, int digits) {
		if(digits > 2) {
			*buf++ = '0' + value/100;
		}
		value %= 100;
		if(digits > 1) {
			*buf++ = '0' + value/10;
		}
		value %= 10;
		*buf++ = '0' + value;
		*buf = 0;
	}
	const char *get_value_string(const CMenu::OPTION &opt, int value) {
		static char buf[5];
		switch(opt.mode) {
		case ENUMERATED: {
			const char *values = opt.values;
			while(*values && value) {
				if(*values == '|') {
					--value;
				}
				++values;
			}
			return values;
		}
		case MIDI_CHANNEL:
			format_number(value + 1, buf, 2);
			return buf;
		case NUMBER_7BIT:
			format_number(value, buf, 3);
			return buf;
		case VOLT_RANGE:
			format_number(value, buf, 1);
			buf[1] = 'V';
			buf[2] = 0;
			return buf;
		default:
			return buf;
		}
	}
	int get_max_value(const CMenu::OPTION &opt) {
		switch(opt.mode) {
		case ENUMERATED: {
			const char *values = opt.values;
			int count = 0;
			while(*values) {
				if(*values == '|') {
					++count;
				}
				++values;
			}
			return count;
		}
		case MIDI_CHANNEL:
			return 15;
		case NUMBER_7BIT:
			return 127;
		case VOLT_RANGE:
			return 8;
		default:
			return 0;
		}
	}
	void event(int evt, uint32_t param) {
		int i;
		switch(evt) {
		case EV_ENCODER:
			if(m_action == ACTION_VALUE_SELECTED || m_action == ACTION_VALUE_CHANGED) {
				i = m_value + (int)param;
				if(i >= 0 && i <= get_max_value(m_opts[m_item])) {
					m_value = i;
					m_action = ACTION_VALUE_CHANGED;
					m_repaint = 1;
				}
			}
			else {
				i = m_item + (int)param;
				if(i>=0 && i<NUM_OPTS) {
					m_item = i;
					m_repaint = 1;
				}
			}
			break;
		case EV_KEY_PRESS:
			if(param == KEY_B1) {
				m_value = get_param(m_opts[m_item].param);
				m_action = ACTION_VALUE_SELECTED;
			}
			break;
		case EV_KEY_RELEASE:
			if(m_action == ACTION_VALUE_CHANGED) {
				set_param(m_opts[m_item].param, m_value);
				m_repaint = 1;
			}
			m_action = ACTION_NONE;
			break;

		}
	}

	void update() {
		m_repaint = 1;
	}
	void repaint() {
		if(m_repaint) {
			CRenderBuf::clear();
			int row = -2;
			for(int opt = m_item - 1; opt <= m_item + 1; ++opt) {

				uint32_t buf[5] = {0};
				int state = 0;
				if(opt >= 0 && opt < NUM_OPTS) {
					const CMenu::OPTION &this_opt = m_opts[opt];

					int value;
					if(opt == m_item) {
						if(m_action == ACTION_VALUE_CHANGED) {
							state = 2;
							value = m_value;
						}
						else {
							state = 1;
						}
					}
					if(state != 2) {
						value = get_param(this_opt.param);
					}

					const char *sz = this_opt.prompt;
					int col = 0;
					while(*sz) {
						CRenderBuf::print_char(*sz, col, 0, buf, 5);
						++sz;
						col += 4;
					}
					sz = get_value_string(this_opt, value);
					col = 16;
					while(*sz && *sz != '|') {
						CRenderBuf::print_char(*sz, col, 0, buf, 5);
						++sz;
						col += 4;
					}
				}
				for(int i=0; i<5; ++i) {
					if(row>=0 && row<16) {
						switch(state) {
						case 0:
							CRenderBuf::hilite(row) = buf[i];
							CRenderBuf::raster(row) = 0;
							break;
						case 1:
							CRenderBuf::hilite(row) = buf[i] & 0xFFFF0000U;
							CRenderBuf::raster(row) = buf[i];
							break;
						case 2:
							CRenderBuf::hilite(row) = buf[i];
							CRenderBuf::raster(row) = buf[i];
							break;
						}
					}
					++row;
				}
				++row;
			}
			m_repaint = 0;
		}
	}


};

extern CMenu g_menu;
#ifdef MAIN_INCLUDE
const CMenu::OPTION CMenu::m_opts[NUM_OPTS] = {
		{"TYP", P_SQL_SEQ_MODE, CMenu::ENUMERATED, "NOTE|MOD|VEL|TRAN"},
		{"RTE", P_SQL_STEP_RATE, CMenu::ENUMERATED, "1|2D|2|4D|2T|4|8D|4T|8|16D|8T|16|16T|32"},
		{"CHN", P_SQL_MIDI_CHAN, CMenu::MIDI_CHANNEL},
		{"CC", P_SQL_MIDI_CC, CMenu::NUMBER_7BIT},
		{"SCAL", P_CVGATE_VSCALE, CMenu::ENUMERATED, "1V|1.2V|HZV"},
		{"RNGE", P_CVGATE_VRANGE, CMenu::VOLT_RANGE}
};
CMenu g_menu;
#endif
#endif /* MENU_H_ */
