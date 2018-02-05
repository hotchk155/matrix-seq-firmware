/*
 * display_panel.cpp
 *
 *  Created on: 4 Feb 2018
 *      Author: jason
 */



#include "defs.h"
#include "digital_out.h"
#include "fsl_clock.h"
#include "fsl_spi.h"
#include "fsl_pit.h"

CDigitalOut<kGPIO_PORTD, 5> pKDAT;
CDigitalOut<kGPIO_PORTC, 1> pKCLK;
CDigitalOut<kGPIO_PORTC, 0> pARCK;
CDigitalOut<kGPIO_PORTB, 3> pADAT;
CDigitalOut<kGPIO_PORTB, 2> pASCK;
CDigitalOut<kGPIO_PORTA, 1> pENABLE;

CDigitalIn<kGPIO_PORTC, 6> pKeyScan1;
CDigitalIn<kGPIO_PORTD, 7> pKeyScan2;
CDigitalIn<kGPIO_PORTD, 6> pKeyScan3;

// The render buffer, contains two "layers". Elements 0-15 are layer 1
// and elements 16-31 are layer 2
// Refresh is done in 2 phases
// Layer 1 time --- Layer 2 time

static volatile uint16_t l_key1 = 0;
static volatile uint16_t l_key2 = 0;
static volatile uint16_t l_key3 = 0;

static volatile uint16_t l_acc_key1 = 0;
static volatile uint16_t l_acc_key2 = 0;
static volatile uint16_t l_acc_key3 = 0;

static volatile byte l_keys_pending = 0;
static volatile uint32_t l_buf1[32] = {0};
static volatile uint32_t l_buf2[32] = {0};
static volatile uint32_t *l_disp_buf = l_buf1;
static volatile uint32_t *l_load_buf = l_buf2;
static volatile byte l_switch_buffers = 0;

static uint32_t l_render_buf[32] = {
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
		0b01000011010000000100001001000000,
		0b01000011010000100100001001000000,
		0b01111100001111000111110001111110,
		0b00011000000000000000000000000000,

		0b11111111000000001111111100000000,
		0b11111111000000001111111100000000,
		0b11111111000000001111111100000000,
		0b11111111000000001111111100000000,
		0b11111111000000001111111100000000,
		0b11111111000000001111111100000000,
		0b11111111000000001111111100000000,
		0b11111111000000001111111100000000,
		0b00000111111111110000000011111111,
		0b00000001111111110000000011111111,
		0b00000001111111110000000011111111,
		0b00000000111111110000000011111111,
		0b00000000111111110000000011111111,
		0b00000000111111110000000011111111,
		0b00000000111111110000000011111111,
		0b00000000111111110000000011111111

};


const byte charSet[] = {

1,1,1,1,1,0,1,0, //!
5,5,0,0,0,0,0,0, //"
10,10,31,10,31,10,10,0, //#
2,7,4,7,1,7,2,0, //$
0,0,25,26,4,11,19,0, //%
0,4,10,4,11,10,5,0, //&
1,1,0,0,0,0,0,0, //'
1,2,2,2,2,2,2,1, //(
2,1,1,1,1,1,1,2, //)
0,5,2,7,2,5,0,0, //*
0,2,2,7,2,2,0,0, //+
0,0,0,0,0,1,1,2, //,
0,0,0,7,0,0,0,0, //-
0,0,0,0,0,0,1,0, //.
0,1,1,2,2,4,4,0, ///

6,9,11,13,9,9,6,0, //0
2,6,2,2,2,2,7,0, //1
6,9,1,6,8,8,15,0, //2
6,9,1,6,1,9,6,0, //3
1,3,5,9,15,1,1,0, //4
15,8,8,6,1,9,6,0, //5
6,9,8,14,9,9,6,0, //6
15,1,1,2,4,4,4,0, //7
6,9,9,6,9,9,6,0, //8
6,9,9,7,1,9,6,0, //9

0,0,0,1,0,1,0,0,//:
0,0,0,1,0,1,1,2, //;
0,1,2,4,2,1,0,0, //<
0,0,7,0,7,0,0,0, //=
0,4,2,1,2,4,0,0, //>
2,5,1,2,2,0,2,0, //?
14,17,17,23,21,23,16,15, //@

6,9,9,15,9,9,9,0, //A
14,9,9,14,9,9,14,0, //B
6,9,8,8,8,9,6,0, //C
14,9,9,9,9,9,14,0, //D
15,8,8,14,8,8,15,0, //E
15,8,8,14,8,8,8,0, //F
6,9,8,11,9,9,7,0, //G
9,9,9,15,9,9,9,0, //H
7,2,2,2,2,2,7,0, //I
15,1,1,1,1,9,6,0, //J
9,10,12,8,12,10,9,0, //K
8,8,8,8,8,8,15,0, //L
17,27,21,17,17,17,17,0, //M
9,9,13,11,9,9,9,0, //N
6,9,9,9,9,9,6,0, //O
14,9,9,14,8,8,8,0, //P
6,9,9,9,11,9,7,0, //Q
14,9,9,14,9,9,9,0, //R
6,9,8,6,1,9,6,0, //S
31,4,4,4,4,4,4,0, //T
9,9,9,9,9,9,6,0, //U
9,9,9,9,9,10,12,0, //V
17,17,17,17,21,27,17,0, //W
17,17,10,4,10,17,17,0, //X
17,17,10,4,4,4,4,0, //Y
31,1,2,4,8,16,31,0, //Z

3,2,2,2,2,2,2,3, //[
0,4,4,2,2,1,1,0, // backslash
3,1,1,1,1,1,1,3, //]
2,5,0,0,0,0,0,0, //^
0,0,0,0,0,0,0,7, //_

0b01111110,
0b10000001,
0b10100101,
0b10000001,
0b10100101,
0b10111101,
0b10000001,
0b01111110,

0,0,14,1,7,9,7,0, //a
8,8,14,9,9,9,14,0, //b
0,0,6,9,8,9,6,0, //c
1,1,7,9,9,9,7,0, //d
0,0,6,9,15,8,6,0, //e
3,4,4,6,4,4,4,0, //f
0,0,7,9,9,7,1,14, //g
8,8,14,9,9,9,9,0, //h
1,0,1,1,1,1,1,0, //i
1,0,1,1,1,1,1,2, //j
8,8,9,10,12,10,9,0, //k
2,2,2,2,2,2,1,0, //l
0,0,30,21,21,17,17,0, //m
0,0,14,9,9,9,9,0, //n
0,0,6,9,9,9,6,0, //o
0,0,14,9,9,14,8,8, //p
0,0,7,9,9,7,1,1, //q
0,0,14,9,8,8,8,0, //r
0,0,7,8,6,1,14,0, //s
2,2,3,2,2,2,1,0, //t
0,0,9,9,9,9,7,0, //u
0,0,9,9,9,10,12,0, //v
0,0,17,17,21,21,27,0, //w
0,0,17,10,4,10,17,0, //x
0,0,9,9,9,7,1,14, //y
0,0,31,2,4,8,31,0, //z


1,2,2,4,2,2,2,1, //{
1,1,1,1,1,1,1,1, //|
4,2,2,1,2,2,2,4, //}
5,10,0,0,0,0,0,0 //~

};



/////////////////////////////////////////////////////////////////////////
// Lookup the 8 raster rows for an ASCII character. Unknown characters
// are blanks. The function return the width of the character
int lookupChar(char ch, byte *buf)
{
  if(ch<33 || ch>126) {
    memset(buf, 0, 8);
    return 3;
  }
  else {
    byte mask = 0;
    const byte *p = charSet + ((ch-33) * 8);
    for(int i=0; i<8; ++i) {
      buf[i] = *(p + i);
      mask |= buf[i];
    }
    if(mask & 0x80) {  return 8; }
    if(mask & 0x40) {  return 7; }
    if(mask & 0x20) {  return 6; }
    if(mask & 0x10) {  return 5; }
    if(mask & 0x08) {  return 4; }
    if(mask & 0x04) {  return 3; }
    if(mask & 0x02) {  return 2; }
    if(mask & 0x01) {  return 1; }
    return 3;
  }
}

void print_char(char ch, int row, int col) {
	byte buf[8];
	lookupChar(ch, buf);
	int shift = 31 - 7 - col;
	for(int i=0; i<8; ++i) {
		if(row >= 0 && row < 16) {
			if(shift < 0) {
				l_render_buf[row] |= ((uint32_t)buf[i]>>shift);
			}
			else {
				l_render_buf[row] |= ((uint32_t)buf[i]<<shift);
			}
		}
		++row;
	}
}

/*
 * Copy the contents of the "write buffer" to the buffer that is actually used
 * for the display refreshes
 */
void load() {

	while(l_switch_buffers);

	for(int i=0; i<32; ++i) {
		l_load_buf[i] = 0;
	}
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
			int k;
			uint32_t dat;

			dat = l_render_buf[j];
			k = i;
			if(dat & mask1) l_load_buf[k] |= bit1;
			if(dat & mask2) l_load_buf[k] |= bit2;
			if(dat & mask3) l_load_buf[k] |= bit3;
			if(dat & mask4) l_load_buf[k] |= bit4;

			dat = l_render_buf[j+8];
			k = 8+i;
			if(dat & mask1) l_load_buf[k] |= bit1;
			if(dat & mask2) l_load_buf[k] |= bit2;
			if(dat & mask3) l_load_buf[k] |= bit3;
			if(dat & mask4) l_load_buf[k] |= bit4;

			dat = l_render_buf[j+16];
			k = 16+i;
			if(dat & mask1) l_load_buf[k] |= bit1;
			if(dat & mask2) l_load_buf[k] |= bit2;
			if(dat & mask3) l_load_buf[k] |= bit3;
			if(dat & mask4) l_load_buf[k] |= bit4;


			dat = l_render_buf[j+24];
			k = 24+i;
			if(dat & mask1) l_load_buf[k] |= bit1;
			if(dat & mask2) l_load_buf[k] |= bit2;
			if(dat & mask3) l_load_buf[k] |= bit3;
			if(dat & mask4) l_load_buf[k] |= bit4;

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
	l_switch_buffers = 1;
}

void cls() {
	for(int i=0; i<32; ++i) {
		l_render_buf[i] = 0;
	}
}

static uint32_t s_DisplayPhaseDuration1;
static uint32_t s_DisplayPhaseDuration2;
void panelInit() {

	//spi_master_config_t config;
	//SPI_MasterGetDefaultConfig(&config);
	//config.baudRate_Bps = 50000;
	//config.outputMode = kSPI_SlaveSelectAsGpio;
	//config.phase = kSPI_ClockPhaseSecondEdge;
	//config.polarity=kSPI_ClockPolarityActiveLow;
	//config.direction = kSPI_LsbFirst;
	//SPI_MasterInit(SPI0, &config, CLOCK_GetFreq(kCLOCK_BusClk));
	//SPI_Enable(SPI0, true);

	// turn the panel off
	s_DisplayPhaseDuration1 = USEC_TO_COUNT(1000, CLOCK_GetBusClkFreq());
	s_DisplayPhaseDuration2 = USEC_TO_COUNT(200, CLOCK_GetBusClkFreq());



	pENABLE.set(1);
	//cls();
	//load();

	  EnableIRQ(PIT_CH1_IRQn);
	  PIT_EnableInterrupts(PIT, kPIT_Chnl_1, kPIT_TimerInterruptEnable);
	  PIT_SetTimerPeriod(PIT, kPIT_Chnl_1, (uint32_t) USEC_TO_COUNT(200, CLOCK_GetBusClkFreq()));
	  PIT_StartTimer(PIT, kPIT_Chnl_1);
}


#define PORTA_BASE 0
#define PORTB_BASE 8
#define PORTC_BASE 16
#define PORTD_BASE 24

#define MK_GPIOA_BIT(port, bit) (((uint32_t)1) << ((port) + (bit)))
#define BIT_KDAT	MK_GPIOA_BIT(PORTD_BASE, 5)
#define BIT_KCLK	MK_GPIOA_BIT(PORTC_BASE, 1)
#define BIT_ARCK	MK_GPIOA_BIT(PORTC_BASE, 0)
#define BIT_ADAT	MK_GPIOA_BIT(PORTB_BASE, 3)
#define BIT_ASCK	MK_GPIOA_BIT(PORTB_BASE, 2)
#define BIT_ENABLE	MK_GPIOA_BIT(PORTA_BASE, 1)
#define BIT_KEYSCAN1	MK_GPIOA_BIT(PORTC_BASE, 6)
#define BIT_KEYSCAN2	MK_GPIOA_BIT(PORTD_BASE, 7)
#define BIT_KEYSCAN3	MK_GPIOA_BIT(PORTD_BASE, 6)



/*
CDigitalOut<kGPIO_PORTD, 5> pKDAT;
CDigitalOut<kGPIO_PORTC, 1> pKCLK;
CDigitalOut<kGPIO_PORTC, 0> pARCK;
CDigitalOut<kGPIO_PORTB, 3> pADAT;
CDigitalOut<kGPIO_PORTB, 2> pASCK;
CDigitalOut<kGPIO_PORTA, 1> pENABLE;
*/

#define SET_GPIOA(mask) ((GPIO_Type *)GPIOA_BASE)->PSOR = (mask)
#define CLR_GPIOA(mask) ((GPIO_Type *)GPIOA_BASE)->PCOR = (mask)
#define READ_GPIOA(mask) (((GPIO_Type *)GPIOA_BASE)->PDIR & (mask))

static int l_cathode = 0;
static byte l_phase = 0;
extern "C" {
void PIT_CH1_IRQHandler(void) {
	PIT_ClearStatusFlags(PIT, kPIT_Chnl_1, kPIT_TimerFlag);
	//PIT_StopTimer(PIT, kPIT_Chnl_1);

	uint32_t data;


	if(!l_phase) {
		if(!l_cathode) {
			// start of cycle - clock a bit into cathode register
			SET_GPIOA(BIT_KDAT);
			CLR_GPIOA(BIT_KCLK);
			SET_GPIOA(BIT_KCLK);
			CLR_GPIOA(BIT_KDAT);
		}
		// fetch data from layer 1
		data = l_disp_buf[l_cathode];
	}
	else if(l_phase < 3) {
		++l_phase;
		return;
	}
	else {
		// fetch data from layer 2
		data = l_disp_buf[l_cathode+16];
	}

	// fill the 32 bits of anode shift register data
	uint32_t mask = 0x80000000;
	while(mask) {
		if(data&mask) {
			SET_GPIOA(BIT_ADAT);
		}
		else {
			CLR_GPIOA(BIT_ADAT);
		}
		CLR_GPIOA(BIT_ASCK);
		SET_GPIOA(BIT_ASCK);
		mask >>= 1;
	}

	if(!l_phase) { // layer 1

		SET_GPIOA(BIT_ENABLE); 	// turn off the display
		CLR_GPIOA(BIT_KCLK); 	// clock cathode bit along one place..
		SET_GPIOA(BIT_KCLK);	// ..so we are addressing next anode row
		CLR_GPIOA(BIT_ARCK);	// anode shift register store clock pulse..
		SET_GPIOA(BIT_ARCK); 	// ..loads new data on to anode lines
		CLR_GPIOA(BIT_ENABLE);	// turn the display back on

		l_phase = 1;
	}
	else { // layer 2

		CLR_GPIOA(BIT_ARCK);	// anode shift register store clock pulse..
		SET_GPIOA(BIT_ARCK);	// ..loads new data on to anode lines

		// read status of keys tied to this cathode bit
		if(!READ_GPIOA(BIT_KEYSCAN1)) {
			l_acc_key1 |= (1U<<l_cathode);
		}
		if(!READ_GPIOA(BIT_KEYSCAN2)) {
			l_acc_key2 |= (1U<<l_cathode);
		}
		if(!READ_GPIOA(BIT_KEYSCAN3)) {
			l_acc_key3 |= (1U<<l_cathode);
		}

		// move along to next cathode bit
		if(++l_cathode >= 16) {
			l_cathode = 0; // scan is complete, go back to beginning

			l_key1 = l_acc_key1;
			l_key2 = l_acc_key2;
			l_key3 = l_acc_key3;

			l_acc_key1 = 0;
			l_acc_key2 = 0;
			l_acc_key3 = 0;

			if(l_switch_buffers) {
				volatile uint32_t *p = l_disp_buf;
				l_disp_buf = l_load_buf;
				l_load_buf = p;
				l_switch_buffers = 0;
			}
		}

		l_phase = 0;
	}

	// swap to other layer
	//PIT_StartTimer(PIT, kPIT_Chnl_1);

}

} // extern "C"

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

		uint32_t data = l_disp_buf[i];
		/*byte buf[4] = {
			(byte)(data>>8),
			(byte)(data),
			(byte)(data>>24),
			(byte)(data>>16)
		};
		SPI_WriteBlocking(SPI0, buf, 4);*/
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




void panelRun()
{



	cls();

	uint32_t mask = 1;
	char row = 0;
	int i;
	for(i=0; i<16; ++i) {
		if(l_key1 & mask) {
			row = 'A';
			break;
		}
		if(l_key2 & mask) {
			row = 'B';
			break;
		}
		if(l_key3 & mask) {
			row = 'C';
			break;
		}
		mask<<=1;
	}
	if(row > 0) {
		print_char(row,5,5);
		print_char('0' + i/10,5,11);
		print_char('0' + i%10,5,17);
	}


	uint32_t key_state = ((uint32_t)l_key2);
	key_state |= ((uint32_t)l_key1);
	key_state |= (((uint32_t)l_key3)<<8);

	l_render_buf[15] = key_state;


	Left side top to bottom
	0	B00		0
	1 	B01		1
	2	B02		2
	3	B03		3
	4	C11		19
	5	C10		18
	6	C09		17
	7	C08		16

	Bottom left to right
	0	A08		8
	1	A09		9
	2	A10		10
	3	A11		11
	4	A12		12
	5	A13		13
	6	A14		14
	7	A15		15

	Left side top to bottom
	0	B07		7
	1	B06		6
	2	B05		5
	3	B04		4
	4	C12		20
	5	C13		21
	6	C14		22
	7	C15		23

	/*
Left side top to bottom
0	B00		0
1 	B01		1
2	B02		2
3	B03		3
4	C11		19
5	C10		18
6	C09		17
7	C08		16

Bottom left to right
0	A08		8
1	A09		9
2	A10		10
3	A11		11
4	A12		12
5	A13		13
6	A14		14
7	A15		15

Left side top to bottom
0	B07		7
1	B06		6
2	B05		5
3	B04		4
4	C12		20
5	C13		21
6	C14		22
7	C15		23


	 */
//	l_render_buf[0] = l_key1;
//	l_render_buf[1] = l_key2;
//	l_render_buf[2] = l_key3;
	load();
}
