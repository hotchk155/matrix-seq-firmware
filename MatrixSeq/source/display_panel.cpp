/*
 * display_panel.cpp
 *
 *  Created on: 4 Feb 2018
 *      Author: jason
 */



#include "defs.h"
#include "display_panel.h"
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

#define DEBOUNCE_MS_PRESS 		50
#define DEBOUNCE_MS_RELEASE		50


// The render buffer, contains two "layers". Elements 0-15 are layer 1
// and elements 16-31 are layer 2
// Refresh is done in 2 phases
// Layer 1 time --- Layer 2 time

static volatile uint16_t l_acc_key1 = 0;
static volatile uint16_t l_acc_key2 = 0;
static volatile uint16_t l_acc_key3 = 0;
static volatile uint32_t l_key_state = 0;
static uint32_t l_prev_key_state = ~0;


static volatile byte l_keys_pending = 0;
static volatile uint32_t l_disp_buf[32] = {0};
volatile byte g_disp_update = 0;

static volatile byte l_enc_state[3] = {0};

static volatile int l_enc_pos = 0;
static int l_prev_enc_pos = 0;
static int l_debounce = 0;

static volatile uint32_t l_next_pit_period = 0;


uint32_t g_render_buf[DISPLAY_BUF_SIZE] = {
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

		// First layer
	switch(l_phase) {

	////////////////////////////////////////////////////////////////////////////
	// NORMAL BRIGHTNESS PHASE
	case PHASE_NORMAL:
		SET_GPIOA(BIT_GATE2);

		if(!l_cathode) { // starting  a new refresh cycle
			// copy over updated render buffer if available
			if(g_disp_update) {
				memcpy((void*)l_disp_buf,(void*)g_render_buf,32*sizeof(uint32_t));
				g_disp_update = 0;
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
					++l_enc_pos;
				}
				else if( (l_enc_state[0] == 0b01) &&
					(l_enc_state[1] == 0b00) &&
					(l_enc_state[2] == 0b10)) {
					--l_enc_pos;
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


// called once per ms
void panelRun()
{
	int pos = l_enc_pos;
	if(pos != l_prev_enc_pos) {
		fire_event(EV_ENCODER, (uint32_t)(pos - l_prev_enc_pos));
		l_prev_enc_pos = pos;
	}

	if(l_debounce) {
		--l_debounce;
	}
	else {
		uint32_t keys = l_key_state;
		if(l_key_state != l_prev_key_state) {
			uint32_t bit = KEY_MAXBIT;
			uint32_t released = ~keys & l_prev_key_state;
			uint32_t pressed = keys & ~l_prev_key_state;
			while(bit) {
				if(pressed & bit) {
					fire_event(EV_KEY_PRESS, bit);
					l_debounce = DEBOUNCE_MS_PRESS;
				}
				if(released & bit) {
					fire_event(EV_KEY_RELEASE, bit);
					l_debounce = DEBOUNCE_MS_RELEASE;
				}
				bit>>=1;
			}
			l_prev_key_state = keys;
		}
	}
}



