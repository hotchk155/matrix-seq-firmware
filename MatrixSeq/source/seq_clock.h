/*
 * clock.h
 *
 *  Created on: 22 Feb 2018
 *      Author: jason
 */

#ifndef SEQ_CLOCK_H_
#define SEQ_CLOCK_H_

#include "fsl_common.h"
#include "fsl_clock.h"
#include "fsl_pit.h"
#include "fsl_kbi.h"



/*
 24ppqn ticks interval is stored as
 */
//



class CSeqClock {
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
	enum {
		CHANNEL_BEAT_LED,
		CHANNEL_SEQ1,
		CHANNEL_SEQ2,
		CHANNEL_SEQ3,
		CHANNEL_SEQ4,
		CHANNEL_MAX
	};
	typedef struct {
		byte ticks_per_step;
		byte pending_steps;
		double tick_period;
		double next_tick;
	} CHANNEL;

	CHANNEL m_chan[CHANNEL_MAX] = {0};

	volatile uint32_t m_millis; 	// millisecond counter
	volatile uint32_t m_ticks;
	volatile byte m_ms_tick;
	float m_bpm;
	CSeqClock() {
		m_millis = 0;
//		m_ticks = 0;
		m_chan[0].ticks_per_step = RATE_4;
		for(int i=1; i<CHANNEL_MAX; ++i) {
			m_chan[i].ticks_per_step = RATE_16;
		}
		set_bpm(120);
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
//		m_tick_rate = 125; // 6 * (60 * 1000) / (bpm*24);
		for(int i=0; i<CHANNEL_MAX; ++i) {
			m_chan[i].tick_period = m_chan[i].ticks_per_step * ((double)60*1000) / ((double)bpm * 24);
		}
	}

	void run() {

		for(int i=0; i<CHANNEL_MAX; ++i) {
			while(m_millis >= m_chan[i].next_tick) {
				++m_chan[i].pending_steps;
				m_chan[i].next_tick = m_chan[i].next_tick + m_chan[i].tick_period;
			}
		}
	}
	byte is_pending(int chan) {
		if(m_chan[chan].pending_steps) {
			return m_chan[chan].pending_steps--;
		}
		return 0;
	}

	void wait_ms(int ms) {
		while(ms) {
			uint32_t m = m_millis;
			while(m_millis==m);
			--ms;
		}
	}


};

// declare global instance of the sequencer clock
extern CSeqClock g_clock;


#ifdef MAIN_INCLUDE

// define the clock instance
CSeqClock g_clock;

// define the GPIO pins used for clock (will initialise the port)
CDigitalIn<kGPIO_PORTA, 0> pClockIn;
CDigitalOut<kGPIO_PORTC, 5> pClockOut;

// ISR for the millisecond timer
extern "C" void PIT_CH0_IRQHandler(void) {
	PIT_ClearStatusFlags(PIT, kPIT_Chnl_0, kPIT_TimerFlag);
	++g_clock.m_millis;
	g_clock.m_ms_tick = 1;
}

// ISR for the KBI interrupt (SYNC IN)
extern "C" void KBI0_IRQHandler(void)
{
    if (KBI_IsInterruptRequestDetected(KBI0)) {
        KBI_ClearInterruptFlag(KBI0);
    }
}

#endif


#endif /* SEQ_CLOCK_H_ */
