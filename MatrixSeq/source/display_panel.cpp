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

CDigitalOut<kGPIO_PORTA, 6> pGate4;
CDigitalOut<kGPIO_PORTD, 4> pGate3;
CDigitalOut<kGPIO_PORTD, 3> pGate2;
CDigitalOut<kGPIO_PORTD, 2> pGate1;


CDigitalOut<kGPIO_PORTD, 5> pKDAT;
CDigitalOut<kGPIO_PORTC, 1> pKCLK;
CDigitalOut<kGPIO_PORTC, 0> pARCK;
CDigitalOut<kGPIO_PORTB, 3> pADAT;
CDigitalOut<kGPIO_PORTB, 2> pASCK;
CDigitalOut<kGPIO_PORTA, 1> pENABLE;

CDigitalIn<kGPIO_PORTC, 6> pKeyScan1;
CDigitalIn<kGPIO_PORTD, 7> pKeyScan2;
CDigitalIn<kGPIO_PORTD, 6> pKeyScan3;

CDigitalIn<kGPIO_PORTD, 0> pEncoder1;
CDigitalIn<kGPIO_PORTD, 1> pEncoder2;

#define BIT_KDAT		MK_GPIOA_BIT(PORTD_BASE, 5)
#define BIT_KCLK		MK_GPIOA_BIT(PORTC_BASE, 1)
#define BIT_ARCK		MK_GPIOA_BIT(PORTC_BASE, 0)
#define BIT_ADAT		MK_GPIOA_BIT(PORTB_BASE, 3)
#define BIT_ASCK		MK_GPIOA_BIT(PORTB_BASE, 2)
#define BIT_ENABLE		MK_GPIOA_BIT(PORTA_BASE, 1)
#define BIT_KEYSCAN1	MK_GPIOA_BIT(PORTC_BASE, 6)
#define BIT_KEYSCAN2	MK_GPIOA_BIT(PORTD_BASE, 7)
#define BIT_KEYSCAN3	MK_GPIOA_BIT(PORTD_BASE, 6)
#define BIT_ENCODER1	MK_GPIOA_BIT(PORTD_BASE, 0)
#define BIT_ENCODER2	MK_GPIOA_BIT(PORTD_BASE, 1)

#define BIT_GATE1		MK_GPIOA_BIT(PORTD_BASE, 2)
#define BIT_GATE2		MK_GPIOA_BIT(PORTD_BASE, 3)
#define BIT_GATE3		MK_GPIOA_BIT(PORTD_BASE, 4)
#define BIT_GATE4		MK_GPIOA_BIT(PORTA_BASE, 6)
/*
CDigitalOut<kGPIO_PORTA, 6> pGate4;
CDigitalOut<kGPIO_PORTD, 4> pGate3;
CDigitalOut<kGPIO_PORTD, 3> pGate2;
CDigitalOut<kGPIO_PORTD, 2> pGate1;
*/
// The render buffer, contains two "layers". Elements 0-15 are layer 1
// and elements 16-31 are layer 2
// Refresh is done in 2 phases
// Layer 1 time --- Layer 2 time

static volatile uint32_t l_key_state = 0;
static volatile uint16_t l_acc_key1 = 0;
static volatile uint16_t l_acc_key2 = 0;
static volatile uint16_t l_acc_key3 = 0;

static volatile byte l_keys_pending = 0;
static volatile uint32_t l_disp_buf[32] = {0};
static volatile byte l_disp_update = 0;

static volatile byte l_enc_state[3] = {0};

volatile int g_enc_pos = 0;

static volatile uint32_t l_next_pit_period = 0;

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
		0b01000010010000000100001001000000,
		0b01000010010000100100001001000000,
		0b01111100001111000111110001111110,
		0b00000000000000000000000000000000,

		0b11111111000000001111111100000000,
		0b11111111000000001111111100000000,
		0b11111111000000001111111100000000,
		0b11111111000000001111111100000000,
		0b11111111000000001111111100000000,
		0b11111111000000001111111100000000,
		0b11111111000000001111111100000000,
		0b11111111000000001111111100000000,
		0b00000000111111110000000011111111,
		0b00000000111111110000000011111111,
		0b00000000111111110000000011111111,
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

void print_char(char ch, int row, int col, int bright=0) {
	byte buf[8];
	lookupChar(ch, buf);
	int shift = 31 - 7 - col;
	int base = bright? 16:0;
	for(int i=0; i<8; ++i) {
		if(row >= 0 && row < 16) {
			if(shift < 0) {
				l_render_buf[base+row] |= ((uint32_t)buf[i]>>shift);
			}
			else {
				l_render_buf[base+row] |= ((uint32_t)buf[i]<<shift);
			}
		}
		++row;
	}
}


void cls() {
	for(int i=0; i<32; ++i) {
		l_render_buf[i] = 0;
	}
}

static uint32_t periodShort;
static uint32_t periodMedium;
static uint32_t periodLong;


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


	periodShort = (uint32_t) USEC_TO_COUNT(50, CLOCK_GetBusClkFreq());
	periodMedium = (uint32_t) USEC_TO_COUNT(300, CLOCK_GetBusClkFreq());
	periodLong = (uint32_t) USEC_TO_COUNT(400, CLOCK_GetBusClkFreq());

	pENABLE.set(1);
	//cls();
	//load();

	  l_next_pit_period = periodShort;
	  EnableIRQ(PIT_CH1_IRQn);
	  PIT_EnableInterrupts(PIT, kPIT_Chnl_1, kPIT_TimerInterruptEnable);
	  PIT_SetTimerPeriod(PIT, kPIT_Chnl_1, l_next_pit_period);
	  PIT_StartTimer(PIT, kPIT_Chnl_1);
}






/*
CDigitalOut<kGPIO_PORTD, 5> pKDAT;
CDigitalOut<kGPIO_PORTC, 1> pKCLK;
CDigitalOut<kGPIO_PORTC, 0> pARCK;
CDigitalOut<kGPIO_PORTB, 3> pADAT;
CDigitalOut<kGPIO_PORTB, 2> pASCK;
CDigitalOut<kGPIO_PORTA, 1> pENABLE;
*/

enum {
	PHASE_NORMAL,		// layer 1 only
	PHASE_DIM,			// layer 2 only
	PHASE_BRIGHT		// layer 1 and layer 2 together
};
static int l_cathode = 0;
static byte l_phase = PHASE_NORMAL;

////////////////////////////////////////////////////////////////////////////
// DISPLAY UPDATE INTERRUPT SERVICE ROUTINE
// The ISR is called sequentially 3 times for each of the cathode rows (each
// of which has 32 bits of image data and 32 bits of brightness data)
//
// Call 1 - Load image (layer 1) data for this cathode row
// ("medium" period before next PIT call to ISR)
// Call 2 - Load brightness (layer 2) data for this cathode row
// ("short" period before next PIT call to ISR)
// Call 3 - Load logical AND of layer 1 and layer 2 data for this cathode row
//          and prepare to move to the next cathode row
// ("long" period before next PIT call to ISR)
//
extern "C" void PIT_CH1_IRQHandler(void) {
	PIT_ClearStatusFlags(PIT, kPIT_Chnl_1, kPIT_TimerFlag);
	PIT_StopTimer(PIT, kPIT_Chnl_1);
	SET_GPIOA(BIT_GATE1);

	volatile uint32_t *src;

	// First layer
	switch(l_phase) {

	////////////////////////////////////////////////////////////////////////////
	// NORMAL BRIGHTNESS PHASE
	case PHASE_NORMAL:
		SET_GPIOA(BIT_GATE2);

		if(!l_cathode) { // starting  a new refresh cycle
			// copy over updated render buffer if available
			if(l_disp_update) {
				memcpy((void*)l_disp_buf,(void*)l_render_buf,32*sizeof(uint32_t));
				l_disp_update = 0;
			}
			// clock bit into cathode shift reg
			SET_GPIOA(BIT_KDAT);
			CLR_GPIOA(BIT_KCLK);
			SET_GPIOA(BIT_KCLK);
			CLR_GPIOA(BIT_KDAT);
		}

		// populate the anode shift register with data from display layer 1
		for(int anode = 31; anode >=0; --anode) {
			int src_index= (l_cathode & 0xF8) + (anode & 0x07);
			int src_mask = ((l_cathode & 0x07) + (anode & 0xF8));
			if(l_disp_buf[src_index] & (0x80000000U >> src_mask)) {
				SET_GPIOA(BIT_ADAT);
			}
			else {
				CLR_GPIOA(BIT_ADAT);
			}
			CLR_GPIOA(BIT_ASCK);
			SET_GPIOA(BIT_ASCK);
		}

		SET_GPIOA(BIT_ENABLE);  // turn off the display
		CLR_GPIOA(BIT_KCLK); 	// clock cathode bit along one place..
		SET_GPIOA(BIT_KCLK);	// ..so we are addressing next anode row
		CLR_GPIOA(BIT_ARCK);	// anode shift register store clock pulse..
		SET_GPIOA(BIT_ARCK); 	// ..loads new data on to anode lines
		CLR_GPIOA(BIT_ENABLE);	// turn the display back on

		l_phase = PHASE_DIM;
		PIT_SetTimerPeriod(PIT, kPIT_Chnl_1, periodMedium);
		break;

	////////////////////////////////////////////////////////////////////////////
	// LOW BRIGHTNESS PHASE
	case PHASE_DIM:
		SET_GPIOA(BIT_ENABLE); // turn off the display
		for(int anode = 31; anode >=0; --anode) {
			int src_index= (l_cathode & 0xF8) + (anode & 0x07);
			int src_mask = ((l_cathode & 0x07) + (anode & 0xF8));
			if(l_disp_buf[16+src_index] & (0x80000000U >> src_mask)) {
				SET_GPIOA(BIT_ADAT);
			}
			else {
				CLR_GPIOA(BIT_ADAT);
			}
			CLR_GPIOA(BIT_ASCK);
			SET_GPIOA(BIT_ASCK);
		}
		CLR_GPIOA(BIT_ARCK);
		SET_GPIOA(BIT_ARCK);
		CLR_GPIOA(BIT_ENABLE); // turn on the display
		l_phase = PHASE_BRIGHT;
		PIT_SetTimerPeriod(PIT, kPIT_Chnl_1, periodShort);
		break;

	////////////////////////////////////////////////////////////////////////////
	// HIGH BRIGHTNESS PHASE
	case PHASE_BRIGHT:
		SET_GPIOA(BIT_ENABLE); // turn off the display
		for(int anode = 31; anode >=0; --anode) {
			int src_index= (l_cathode & 0xF8) + (anode & 0x07);
			int src_mask = ((l_cathode & 0x07) + (anode & 0xF8));
			if(l_disp_buf[src_index] & l_disp_buf[16+src_index] & (0x80000000U >> src_mask)) {
				SET_GPIOA(BIT_ADAT);
			}
			else {
				CLR_GPIOA(BIT_ADAT);
			}
			CLR_GPIOA(BIT_ASCK);
			SET_GPIOA(BIT_ASCK);
		}
		CLR_GPIOA(BIT_ARCK);
		SET_GPIOA(BIT_ARCK);
		CLR_GPIOA(BIT_ENABLE); // turn display back on again

		////////////////////////////////////////////////
		// READ ENCODER
		// get the state of the two encoder inputs into a 2 bit value
		byte new_state = 0;
		if(!(READ_GPIOA(BIT_ENCODER1))) {
			new_state |= 0b10;
		}
		if(!(READ_GPIOA(BIT_ENCODER2))) {
			new_state |= 0b01;
		}

		// make sure the state has changed and does not match
		// the previous state (which may indicate a bounce)
		if(new_state != l_enc_state[0] && new_state != l_enc_state[1]) {

			if(new_state == 0b11) {
				if( (l_enc_state[0] == 0b10) &&
					(l_enc_state[1] == 0b00) &&
					(l_enc_state[2] == 0b01)) {
					++g_enc_pos;
				}
				else if( (l_enc_state[0] == 0b01) &&
					(l_enc_state[1] == 0b00) &&
					(l_enc_state[2] == 0b10)) {
					--g_enc_pos;
				}
			}

			l_enc_state[2] = l_enc_state[1];
			l_enc_state[1] = l_enc_state[0];
			l_enc_state[0] = new_state;
		}

		////////////////////////////////////////////////
		// SCAN KEYBOARD ROW
		if(!READ_GPIOA(BIT_KEYSCAN1)) {
			l_acc_key1 |= (1U<<l_cathode);
		}
		if(!READ_GPIOA(BIT_KEYSCAN2)) {
			l_acc_key2 |= (1U<<l_cathode);
		}
		if(!READ_GPIOA(BIT_KEYSCAN3)) {
			l_acc_key3 |= (1U<<l_cathode);
		}

		// move along to next cathode bit - have we finished a scan?
		if(++l_cathode >= 16) {
			l_cathode = 0;

			// form the final 32 bit key state value
			l_key_state = ((uint32_t)l_acc_key2);
			l_key_state |= ((uint32_t)l_acc_key1);
			l_key_state |= (((uint32_t)l_acc_key3)<<8);

			// zero the key state accumulators
			l_acc_key1 = 0;
			l_acc_key2 = 0;
			l_acc_key3 = 0;
		}

		l_phase = PHASE_NORMAL;
		PIT_SetTimerPeriod(PIT, kPIT_Chnl_1, periodLong);
		break;
	}

	PIT_StartTimer(PIT, kPIT_Chnl_1);
	CLR_GPIOA(BIT_GATE1);
}


int pos = 0;
void panelRun()
{



//	cls();
/*
	uint32_t k = 1;
	int i;
	for(i=0; i<24; ++i) {
		if(l_key_state & k) {
			break;
		}
		k<<=1;
	}
	char c1=0;
	char c2=0;
	switch(k) {
	case KEY_L1: c1='L'; c2='1'; break;
	case KEY_L2: c1='L'; c2='2'; break;
	case KEY_L3: c1='L'; c2='3'; break;
	case KEY_L4: c1='L'; c2='4'; break;
	case KEY_L5: c1='L'; c2='5'; break;
	case KEY_L6: c1='L'; c2='6'; break;
	case KEY_L7: c1='L'; c2='7'; break;
	case KEY_L8: c1='L'; c2='8'; break;
	case KEY_B1: c1='B'; c2='1'; break;
	case KEY_B2: c1='B'; c2='2'; break;
	case KEY_B3: c1='B'; c2='3'; break;
	case KEY_B4: c1='B'; c2='4'; break;
	case KEY_B5: c1='B'; c2='5'; break;
	case KEY_B6: c1='B'; c2='6'; break;
	case KEY_B7: c1='B'; c2='7'; break;
	case KEY_B8: c1='B'; c2='8'; break;
	case KEY_R1: c1='R'; c2='1'; break;
	case KEY_R2: c1='R'; c2='2'; break;
	case KEY_R3: c1='R'; c2='3'; break;
	case KEY_R4: c1='R'; c2='4'; break;
	case KEY_R5: c1='R'; c2='5'; break;
	case KEY_R6: c1='R'; c2='6'; break;
	case KEY_R7: c1='R'; c2='7'; break;
	case KEY_R8: c1='R'; c2='8'; break;
	}

	print_char(c1,5,5);
	print_char(c1,5,5,1);
	print_char(c2,5,11);
*/
	int pos = 31 - g_enc_pos;
	if(pos < 0) pos = 0;
	if(pos > 31) pos = 31;

	l_render_buf[0] = 1U<<pos;
	l_render_buf[16] = 0;
	l_disp_update = 1;
}



