/*
 * display_panel.h
 *
 *  Created on: 4 Feb 2018
 *      Author: jason
 */

#ifndef DISPLAY_PANEL_H_
#define DISPLAY_PANEL_H_

void panelInit();
void panelRefresh();
void panelRun();

#define DISPLAY_BUF_SIZE 32
extern uint32_t g_render_buf[DISPLAY_BUF_SIZE];
extern volatile byte g_disp_update;


#define KEY_L1	(1U<<0)
#define KEY_L2	(1U<<1)
#define KEY_L3	(1U<<2)
#define KEY_L4	(1U<<3)
#define KEY_L5	(1U<<19)
#define KEY_L6	(1U<<18)
#define KEY_L7	(1U<<17)
#define KEY_L8	(1U<<16)

#define KEY_B1	(1U<<8)
#define KEY_B2	(1U<<9)
#define KEY_B3	(1U<<10)
#define KEY_B4	(1U<<11)
#define KEY_B5	(1U<<12)
#define KEY_B6	(1U<<13)
#define KEY_B7	(1U<<14)
#define KEY_B8	(1U<<15)

#define KEY_R1	(1U<<7)
#define KEY_R2	(1U<<6)
#define KEY_R3	(1U<<5)
#define KEY_R4	(1U<<4)
#define KEY_R5	(1U<<20)
#define KEY_R6	(1U<<21)
#define KEY_R7	(1U<<22)
#define KEY_R8	(1U<<23)

#define KEY_MAXBIT	(1U<<23)

extern const uint32_t char4x5[];
#define CHAR4X5_ALPHA		0
#define CHAR4X5_HASH		26
#define CHAR4X5_NUMERIC		32
#define CHAR4X5_MINUS		42
#define CHAR4X5_DOT			43
#define CHAR4X5_BLOCK		47


class CRenderBuf {
public:
	enum {
		CHAR_RASTER = 1,
		CHAR_HILITE = 2,

		POPUP_LEFT = 1

	};
	static inline void lock() {
		g_disp_update = 0;
	}
	static inline void unlock() {
		g_disp_update = 1;
	}
	static inline void clear() {
		memset((byte*)g_render_buf, 0, sizeof g_render_buf);
	}
	static inline uint32_t& raster(int index) {
		return g_render_buf[index];
	}
	static inline uint32_t& hilite(int index) {
		return g_render_buf[16+index];
	}
	static inline void set_raster(int index, uint32_t mask) {
		g_render_buf[index] |= mask;
	}
	static inline void clear_raster(int index, uint32_t mask) {
		g_render_buf[index] &= ~mask;
	}
	static inline void set_hilite(int index, uint32_t mask) {
		g_render_buf[16+index] |= mask;
	}
	static inline void clear_hilite(int index, uint32_t mask) {
		g_render_buf[16+index] &= ~mask;
	}
	static inline void dim(int index, uint32_t mask) {
		g_render_buf[index] &= ~mask;
		g_render_buf[16+index] |= mask;
	}
	static inline void normal(int index, uint32_t mask) {
		g_render_buf[index] |= mask;
		g_render_buf[16+index] &= ~mask;
	}
	static inline void bright(int index, uint32_t mask) {
		g_render_buf[index] |= mask;
		g_render_buf[16+index] |= mask;
	}

	static inline uint32_t bit(int index) {
		return 0x80000000U >> index;
	}
	static uint32_t make_mask(int from, int to) {
		uint32_t mask = 0x80000000U >> from;
		uint32_t result = 0;
		while(from++ < to) {
			result |= mask;
			mask >>= 1;
		}
		return result;
	}
	static void print_char(int ch, int col, int row, uint32_t *buf, int size) {

		switch(ch) {
		case '#': ch = CHAR4X5_HASH; break;
		case '-': ch = CHAR4X5_MINUS; break;
		case '~': ch = CHAR4X5_BLOCK; break;
		case '.': ch = CHAR4X5_DOT; break;
		default:
			if(ch >= '0' && ch <= '9') {
				ch = CHAR4X5_NUMERIC + ch - '0';
			}
			else if(ch >= 'A' && ch <= 'Z') {
				ch = CHAR4X5_ALPHA + ch - 'A';
			}
			else if(ch >= 'a' && ch <= 'z') {
				ch = CHAR4X5_ALPHA + ch - 'a';
			}
			else {
				return;
			}
		}

		// 8 characters are stored in 5 x 32-bit words so
		// first word containing data for character ch is
		// at index 5 * (ch/8)
		const uint32_t *p = char4x5 + 5 * (ch/8);

		// to extract the data for the character from each of
		// these 5 words, we need to first right-shift the data
		// intobit positions 0-3 to be masked from the other chars
		// in these words
		int shift1 = (28 - 4*(ch%8));

		// then a second left or right shift is needed to get the
		// data into the required column to be masked into the
		// display buffer
		int shift2 = col - 28;

		for(int i=0; i<5 && row >= 0 && row < size; ++i) {

			// fetch data word
			uint32_t d = p[i];

			// extract the row of data for requested char
			d>>=shift1;
			d &= 0x0F;

			// shift to the display position
			if(shift2<0) {
				d<<=-shift2;
			}
			else {
				d>>=shift2;
			}

			buf[row] |= d;
			++row;
		}
	}

	/*
	//
	// 0111011101110
	static void print_note_name(byte note, int flags) {


		char first;
		char second = 0;
		switch(note%12) {
		case 0: first = 'C'; break;
		case 1: first = 'C'; second = '#'; break;
		case 2: first = 'D'; break;
		case 3: first = 'D'; second = '#'; break;
		case 4: first = 'E'; break;
		case 5: first = 'F'; break;
		case 6: first = 'F'; second = '#'; break;
		case 7: first = 'G'; break;
		case 8: first = 'G'; second = '#'; break;
		case 9: first = 'A'; break;
		case 10: first = 'A'; second = '#'; break;
		case 11: first = 'B'; break;
		}

		int octave = note/12 - 1;
		int width = 7;
		if(second) {
			width += 4;
		}
		if(octave < 0) {
			width += 4;
		}


		int col;
		uint32_t mask;
		if(flags & POPUP_LEFT) {
			col = 0;
			mask = make_mask(0,width);
		}
		else {
			col = 32-width;
			mask = make_mask(32-width,32);
		}

		for(int i=1; i<6; ++i) {
			g_render_buf[i] &= ~mask;
			g_render_buf[i+16] |= mask;
		}

		print_char(first, col, 1, CHAR_RASTER);
		col += 4;
		if(second) {
			print_char(second, col, 1, CHAR_RASTER);
			col += 4;
		}
		if(octave<0) {
			print_char('-', col, 1, CHAR_RASTER);
			col += 4;
			octave = -octave;
		}
		print_char('0' + octave, col, 1, CHAR_RASTER);
	}

	static void print_number2(int value, int flags) {
		int col;
		uint32_t mask;
		if(flags & POPUP_LEFT) {
			col = 0;
			mask = make_mask(0,7);
		}
		else {
			col = 25;
			mask = make_mask(25,32);
		}

		for(int i=1; i<6; ++i) {
			g_render_buf[i] &= ~mask;
			g_render_buf[i+16] |= mask;
		}

		value %= 100;
		print_char('0' + (value/10), col, 1, CHAR_RASTER);
		col += 4;
		print_char('0' + (value%10), col, 1, CHAR_RASTER);
	}


	static void print_text(char *sz, int flags) {
		int col;
		uint32_t mask;
		char *q;
		for(q=sz; *q; ++q);
		if(flags & POPUP_LEFT) {
			col = 0;
			mask = make_mask(0,4*(q-sz));
		}
		else {
			col = 33-4*(q-sz);
			mask = make_mask(25,32);
		}

		for(int i=1; i<6; ++i) {
			g_render_buf[i] &= ~mask;
			g_render_buf[i+16] |= mask;
		}

		while(*sz) {
			print_char(*sz, col, 1, CHAR_RASTER);
			col += 4;
			++sz;
		}
	}*/

};


#endif /* DISPLAY_PANEL_H_ */
