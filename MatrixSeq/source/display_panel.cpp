/*
 * display_panel.cpp
 *
 *  Created on: 4 Feb 2018
 *      Author: jason
 */



#include "defs.h"
#include "digital_out.h"

CDigitalOut<kGPIO_PORTD, 5> pKDAT;
CDigitalOut<kGPIO_PORTC, 1> pKCLK;
CDigitalOut<kGPIO_PORTC, 0> pARCK;
CDigitalOut<kGPIO_PORTB, 3> pADAT;
CDigitalOut<kGPIO_PORTB, 2> pASCK;
CDigitalOut<kGPIO_PORTA, 1> pENABLE;

static uint32_t l_buf[16];


static uint32_t l_write_buf[16] = {
		/*
		0xFF000000,
		0xFF000000,
		0xFF000000,
		0xFF000000,
		0xFF000000,
		0xFF000000,
		0xFF000000,
		0xFF000000
		*/

		0b00000000000000000000000000000000,
		0b00011000011111000011110001111100,
		0b00100100010000100100001001000010,
		0b01111110011111000100000001000010,
		0b01000010010000100100000001000010,
		0b01000010010000100100001001000010,
		0b01000010011111000011110001111100,
		0b00000000000000000000000000000000,
		0b00000000000000000000000000000000,
		0b01111100001111000111110001111110,
		0b01000010010000100100001001000000,
		0b01111100010000000100001001111000,
		0b01000010010000000100001001000000,
		0b01000010010000100100001001000000,
		0b01111100001111000111110001111110,
		0b00000000000000000000000000000000

};

/*
 * Copy the contents of the "write buffer" to the buffer that is actually used
 * for the display refreshes
 */
void load() {


	uint32_t mask1 = (uint32_t)0x80000000;
	uint32_t mask2 = (uint32_t)0x00800000;
	uint32_t mask3 = (uint32_t)0x00008000;
	uint32_t mask4 = (uint32_t)0x00000080;

	// loop over the cathode index range for top row 0-7
	for(int i=0; i<8; ++i) {

		uint32_t bit1 = (uint32_t)0x00000001;
		uint32_t bit2 = (uint32_t)0x00000100;
		uint32_t bit3 = (uint32_t)0x00010000;
		uint32_t bit4 = (uint32_t)0x01000000;
		for(int j=0; j<8; ++j) {
			uint32_t dat = l_write_buf[j];
			if(dat & mask1) l_buf[i] |= bit1;
			if(dat & mask2) l_buf[i] |= bit2;
			if(dat & mask3) l_buf[i] |= bit3;
			if(dat & mask4) l_buf[i] |= bit4;


			int k = 8+i;
			dat = l_write_buf[j+8];
			if(dat & mask1) l_buf[k] |= bit1;
			if(dat & mask2) l_buf[k] |= bit2;
			if(dat & mask3) l_buf[k] |= bit3;
			if(dat & mask4) l_buf[k] |= bit4;

			bit1<<=1;
			bit2<<=1;
			bit3<<=1;
			bit4<<=1;
		}
		mask1 >>= 1;
		mask2 >>= 1;
		mask3 >>= 1;
		mask4 >>= 1;
	}
}

void cls() {
	for(int i=0; i<16; ++i) {
		l_buf[i] = 0;
	}
}
void plot(int x, int y) {

	// determine the cathode index (0-15)
	int k_idx = (x%8) + 8*(y/8);

	// get the anode index
	int a_idx = (y%8) + 8*(x/8);

	//a_idx =0;
	l_buf[k_idx] |= (uint32_t)1<<a_idx;
}

void panelInit() {
	// turn the panel off
	pENABLE.set(1);
	cls();
	load();
}

void panelRefresh() {
	// clock a HIGH bit into cathode register
	pKDAT.set(1);
	pKCLK.set(0);
	pKCLK.set(1);
	pKDAT.set(0);

//	pKCLK.set(0);
//	pKCLK.set(1);


	// cycle through the 16 cathode lines
	for(int i=0; i<16; ++i) {

		pKCLK.set(0);
		pKCLK.set(1);

		uint32_t data = l_buf[i];
		uint32_t mask = 0x80000000;
		while(mask) {
			pADAT.set(!!(data&mask));
			pASCK.set(0);
			pASCK.set(1);
			mask >>= 1;
		}
		pARCK.set(0);
		pARCK.set(1);
		pENABLE.set(0); // display on

		volatile int q;
		for(q=0; q<1000; ++q);

		pENABLE.set(1);	// display off

	}
}
