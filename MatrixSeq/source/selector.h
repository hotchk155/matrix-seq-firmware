/*
 * selector.h
 *
 *  Created on: 2 Apr 2018
 *      Author: jason
 */

#ifndef SELECTOR_H_
#define SELECTOR_H_



/////////////////////////////////////////////////////////////////////////////////////////////////
class CSelector {
public:
	byte m_value;
	byte m_repaint;
	const char *m_prompt;
	PARAM_ID m_param;
	PARAM_TYPE m_type;
	const char *m_values_text;

	/////////////////////////////////////////////////////////////////////////////////////////////////
	CSelector() {
		m_value = 0;
		m_repaint = 0;
		m_param = P_NONE;
		m_type = PT_NONE;
		m_values_text = NULL;
		m_prompt = NULL;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////
	void activate(const char *prompt, PARAM_ID param, PARAM_TYPE type, const char *values_text) {
		m_prompt = prompt;
		m_param = param;
		m_type = type;
		m_values_text = values_text;
		m_value = get_param(param);
		m_repaint = 1;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////
	void event(int evt, uint32_t param) {
		int i;
		switch(evt) {
		case EV_ENCODER:
			i = m_value + (int)param;
			if(i >= param_min_value(m_type) && i <= param_max_value(m_type, m_values_text)) {
				m_value = i;
				m_repaint = 1;
			}
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
			CRenderBuf::print_text(m_prompt, 0, 3, CRenderBuf::RASTER);
			CRenderBuf::print_text(param_value_string(m_type, m_value, m_values_text), 0, 9, CRenderBuf::RASTER|CRenderBuf::HILITE);
			m_repaint = 0;
		}
	}
};

extern CSelector g_selector;
#ifdef MAIN_INCLUDE
CSelector g_selector;
#endif


#endif /* SELECTOR_H_ */
