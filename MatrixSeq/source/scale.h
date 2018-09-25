/*
 * scale.h
 *
 *  Created on: 23 Mar 2018
 *      Author: jason
 */

#ifndef SCALE_H_
#define SCALE_H_

/////////////////////////////////////////////////////////////////
// Scale class represents a musical scale that covers all the
// MIDI note range 0-127
class CScale {


public:
	typedef struct {
		V_SQL_SCALE_TYPE scale_type;
		V_SQL_SCALE_ROOT scale_root;
	} CONFIG;
	CONFIG m_cfg;


	void init_config() {
		m_cfg.scale_type = V_SQL_SCALE_TYPE_IONIAN;
		m_cfg.scale_root = V_SQL_SCALE_ROOT_C;
	}

	CScale() {
		init_config();
		build();
	}
	byte m_index_to_note[8];
	byte m_note_to_index[12];
	byte m_max_index;
	///////////////////////////////////////////////////////////////////////////////
	void build() {
		byte interval[7] = {
			1, 1, 0, 1, 1, 1, 0
		};
		byte ofs = (int)m_cfg.scale_type; // 0 = ionian
		byte i2n_value = m_cfg.scale_root;
		byte n2i_index = 0;
		for(int i=0; i<8; ++i) {
			m_index_to_note[i] = i2n_value++;
			m_note_to_index[n2i_index++] = i;
			if(interval[ofs]) {
				i2n_value++;
				m_note_to_index[n2i_index++] = i;
			}
			if(++ofs >= 7) {
				ofs = 0;
			}
		}

		m_max_index = note_to_index(127);
	}
	/////////////////////////////////////////////////////////////////
	inline byte max_index() {
		return m_max_index;
	}

	///////////////////////////////////////////////////////////////////////////////
	void set(PARAM_ID param, int value) {
		switch(param) {
		case P_SQL_SCALE_TYPE: m_cfg.scale_type = (V_SQL_SCALE_TYPE)value; build(); break;
		case P_SQL_SCALE_ROOT: m_cfg.scale_root = (V_SQL_SCALE_ROOT)value; build(); break;
		default: break;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	int get(PARAM_ID param) {
		switch(param) {
		case P_SQL_SCALE_TYPE: return m_cfg.scale_type;
		case P_SQL_SCALE_ROOT: return m_cfg.scale_root;
		default: return 0;
		}
	}

	/////////////////////////////////////////////////////////////////
	// convert scale index to MIDI note
	byte index_to_note(int index) {
		int octave = index/7;
		int note_in_scale = m_index_to_note[index%7];
		int note = 12*octave + note_in_scale;
		if (note < 0 || note > 127) {
			return 0;
		}
		return note;
	}

	/////////////////////////////////////////////////////////////////
	// convert MIDI note to scale index
	byte note_to_index(int note) {
		int octave = note/12;
		int note_in_scale = m_note_to_index[note%12];
		int index = 7*octave + note_in_scale;
		if (index < 0 || index > 127) {
			return 0;
		}
		return index;
	}

	/////////////////////////////////////////////////////////////////
	// force a MIDI note to valid MIDI note in scale, sharpening if needed
	byte force_to_scale(byte note) {
		int octave = note/12;
		int note_in_scale = m_note_to_index[note%12];
		if(note_in_scale > 11) {
			note_in_scale -= 12;
			++octave;
		}
		int result = 12 * octave + m_index_to_note[note_in_scale];
		if (result < 0 || result > 127) {
			return 0;
		}
		return result;
	}

};

CScale g_scale;
#endif /* SCALE_H_ */
