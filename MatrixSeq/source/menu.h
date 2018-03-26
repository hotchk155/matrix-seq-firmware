/*
 * menu.h
 *
 *  Created on: 12 Mar 2018
 *      Author: jason
 */

#ifndef MENU_H_
#define MENU_H_


#define NUM_MENU_OPTS (int)(sizeof(CMenu::m_opts)/sizeof(CMenu::OPTION))

/////////////////////////////////////////////////////////////////////////////////////////////////
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
		BPM,
		DURATION,

		ACTION_NONE = 0,
		ACTION_VALUE_SELECTED,
		ACTION_VALUE_CHANGED
	};
	const OPTION m_opts[14] = {
			{"TYP", P_SQL_SEQ_MODE, CMenu::ENUMERATED, "CHR|FSC|KEY|MOD|MODF"},
			{"RTE", P_SQL_STEP_RATE, CMenu::ENUMERATED, "1|2D|2|4D|2T|4|8D|4T|8|16D|8T|16|16T|32"},
			{"DUR", P_SQL_STEP_DUR, CMenu::ENUMERATED, "STEP|FULL|NONE|32|16T|16|8T|16D|8|4T|8D|4|2T|4D|2|2D|1"},
			{"CHN", P_SQL_MIDI_CHAN, CMenu::ENUMERATED, "NONE|1|2|3|4|5|6|7|8|9|10|11|12|13|14|15|16"},
			{"CC", P_SQL_MIDI_CC, CMenu::NUMBER_7BIT},
			{"CV", P_CVGATE_VRANGE, CMenu::VOLT_RANGE},
			{"TUN", P_CVGATE_VSCALE, CMenu::ENUMERATED, "1V|1.2V|HZV"},
			{"SCL", P_SQL_SCALE_TYPE, CMenu::ENUMERATED, "IONI|DORI|PHRY|LYDI|MIXO|AEOL|LOCR"},
			{"ROOT", P_SQL_SCALE_ROOT, CMenu::ENUMERATED, "C|C#|D|D#|E|F|F#|G|G#|A|A#|B"},
			{"BPM", P_CLOCK_BPM, CMenu::BPM},
			{"CLK", P_CLOCK_SRC, CMenu::ENUMERATED, "INT|MIDI"},
			{"VELH", P_SQL_MIDI_VEL_HI, CMenu::NUMBER_7BIT},
			{"VELM", P_SQL_MIDI_VEL_MED, CMenu::NUMBER_7BIT},
			{"VELL", P_SQL_MIDI_VEL_LO, CMenu::NUMBER_7BIT}
	};


	byte m_item;
	byte m_value;
	byte m_repaint;
	byte m_action;

	/////////////////////////////////////////////////////////////////////////////////////////////////
	CMenu() {
		m_item = 0;
		m_repaint = 1;
		m_action = ACTION_NONE;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////
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

	/////////////////////////////////////////////////////////////////////////////////////////////////
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
		case DURATION:
			format_number(value, buf, 2);
			return buf;
		case NUMBER_7BIT:
		case BPM:
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

	/////////////////////////////////////////////////////////////////////////////////////////////////
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
		case BPM:
			return 300;
		default:
			return 0;
		}
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////
	int get_min_value(const CMenu::OPTION &opt) {
		switch(opt.mode) {
		case BPM:
			return 30;
		default:
			return 0;
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////
	void activate() {
		m_action = ACTION_NONE;
		m_repaint = 1;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////
	void event(int evt, uint32_t param) {
		int i;
		switch(evt) {
		case EV_ENCODER:
			if(m_action == ACTION_VALUE_SELECTED || m_action == ACTION_VALUE_CHANGED) {
				i = m_value + (int)param;
				if(i >= get_min_value(m_opts[m_item]) && i <= get_max_value(m_opts[m_item])) {
					m_value = i;
					m_action = ACTION_VALUE_CHANGED;
					m_repaint = 1;
				}
			}
			else {
				i = m_item + (int)param;
				if(i>=0 && i<NUM_MENU_OPTS) {
					m_item = i;
					m_repaint = 1;
				}
			}
			break;
		case EV_KEY_PRESS:
			if(param == KEY_EDIT) {
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

	/////////////////////////////////////////////////////////////////////////////////////////////////
	void force_repaint() {
		m_repaint = 1;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////
	void repaint() {
		if(m_repaint) {
			CRenderBuf::clear();
			int row = -2;
			for(int opt = m_item - 1; opt <= m_item + 1; ++opt) {

				uint32_t buf[5] = {0};
				int state = 0;
				if(opt >= 0 && opt < NUM_MENU_OPTS) {
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
CMenu g_menu;
#endif

#endif
