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

// map specific key functions
#define KEY_EDIT	KEY_B1

#define KEY_STORE	KEY_B6

#define KEY_MENU	KEY_B8

#define KEY_STARTSTOP	KEY_B7
#define KEY2_LAYER1	KEY_B1
#define KEY2_LAYER2	KEY_B2
#define KEY2_LAYER3	KEY_B3
#define KEY2_LAYER4	KEY_B4





class CRenderBuf {
public:
	enum {
		RASTER = 1,
		HILITE = 2,

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
	static void print_char(int ch, int col, int row, uint32_t *raster, uint32_t *hilite, int size) {

		switch(ch) {
		case '#': ch = CHAR4X5_HASH; break;
		case '-': ch = CHAR4X5_MINUS; break;
		case '+': ch = CHAR4X5_PLUS; break;
		case '~': ch = CHAR4X5_BLOCK; break;
		case '.': ch = CHAR4X5_DOT; break;
		case '>': ch = CHAR4X5_GT; break;
		case '$': ch = CHAR4X5_CROSS; break;
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

			if(raster) {
				raster[row] |= d;
			}
			if(hilite) {
				hilite[row] |= d;
			}
			++row;
		}
	}
	static void print_text(const char *sz, int col, int row, byte layer) {
		uint32_t *raster = (layer & RASTER)? g_render_buf : 0;
		uint32_t *hilite = (layer & HILITE)? &g_render_buf[16] : 0;
		while(*sz) {
			print_char(*sz, col, row, raster, hilite, 16);
			++sz;
			col+=4;
		}
	}


};


#endif /* DISPLAY_PANEL_H_ */
