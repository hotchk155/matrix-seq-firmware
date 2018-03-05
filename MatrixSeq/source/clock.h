/*
 * clock.h
 *
 *  Created on: 22 Feb 2018
 *      Author: jason
 */

#ifndef CLOCK_H_
#define CLOCK_H_

#include "fsl_common.h"
#include "fsl_clock.h"
#include "fsl_pit.h"
#include "fsl_kbi.h"



/*

 65536 for 32 beats
 2048 ticks per beat


 */
//



class CClock {
public:

	enum
	{
	  RATE_1    = 96,
	  RATE_2D   = 72,
	  RATE_2    = 48,
	  RATE_4D   = 36,
	  RATE_2T   = 32,
	  RATE_4    = 24,
	  RATE_8D   = 18,
	  RATE_4T   = 16,
	  RATE_8    = 12,
	  RATE_16D  = 9,
	  RATE_8T   = 8,
	  RATE_16   = 6,
	  RATE_16T  = 4,
	  RATE_32   = 3
	};


	float m_bpm;


	volatile double m_part_tick;
	volatile double m_ticks_per_ms;
	volatile byte m_ms_tick;
	volatile uint32_t m_ticks;

	CClock() {
		reset();
		set_bpm(120);
	}

	void reset() {
		m_part_tick = 0.0;
		m_ms_tick = 0;
		m_ticks = 0;
	}

	void init() {

		// configure a timer to cause an interrupt once per second
		CLOCK_EnableClock(kCLOCK_Pit0);
		pit_config_t timerConfig = {
		 .enableRunInDebug = true,
		};
		PIT_Init(PIT, &timerConfig);
		EnableIRQ(PIT_CH0_IRQn);
		PIT_EnableInterrupts(PIT, kPIT_Chnl_0, kPIT_TimerInterruptEnable);
		PIT_SetTimerPeriod(PIT, kPIT_Chnl_0, (uint32_t) MSEC_TO_COUNT(1, CLOCK_GetBusClkFreq()));
		PIT_StartTimer(PIT, kPIT_Chnl_0);

		// configure the KBI peripheral to cause an interrupt when sync pulse in is triggered
		kbi_config_t kbiConfig;
		kbiConfig.mode = kKBI_EdgesDetect;
		kbiConfig.pinsEnabled = 0x01; // KBI0 pin 0
		kbiConfig.pinsEdge = 0; // Falling Edge
		KBI_Init(KBI0, &kbiConfig);
	}

	void set_bpm(float bpm) {
		m_bpm = bpm;
		m_ticks_per_ms = ((double)bpm * RATE_4) / (60.0 * 1000.0);
	}

	void wait_ms(int ms) {
		while(ms) {
			m_ms_tick = 0;
			while(!m_ms_tick);
			--ms;
		}
	}


};

// declare global instance of the sequencer clock
extern CClock g_clock;


#ifdef MAIN_INCLUDE

// define the clock instance
CClock g_clock;

// define the GPIO pins used for clock (will initialise the port)
CDigitalIn<kGPIO_PORTA, 0> pClockIn;
CDigitalOut<kGPIO_PORTC, 5> pClockOut;

// ISR for the millisecond timer
extern "C" void PIT_CH0_IRQHandler(void) {
	PIT_ClearStatusFlags(PIT, kPIT_Chnl_0, kPIT_TimerFlag);
	g_clock.m_ms_tick = 1;

	g_clock.m_part_tick += g_clock.m_ticks_per_ms;
	int int_part = (int)g_clock.m_part_tick;
	if(int_part) {
		++g_clock.m_ticks;
		g_clock.m_part_tick -= (int)g_clock.m_part_tick;
	}
}

// ISR for the KBI interrupt (SYNC IN)
extern "C" void KBI0_IRQHandler(void)
{
    if (KBI_IsInterruptRequestDetected(KBI0)) {
        KBI_ClearInterruptFlag(KBI0);
    }
}

#endif


#endif /* CLOCK_H_ */
