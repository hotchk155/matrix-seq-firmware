/*
 * clock.h
 *
 *  Created on: 22 Feb 2018
 *      Author: jason
 */

#ifndef SEQ_CLOCK_H_
#define SEQ_CLOCK_H_

#include "fsl_kbi.h"



//

class CSeqClock {
public:


	// COUNTER_TYPE is a 64 bit unsigned integer
	// the top 16 bits are "ticks"
	// there are 96 ticks per sixteenth note
	// the remaining are fractions of a tick
	// The counter is updated once per millisecond, so the increment is defined as the
	// number of ticks that elapse per millisecond
	// the counter runs for 32 sixteenth notes before resetting
	//
	typedef uint64_t COUNTER_TYPE;
	typedef uint16_t TICKS_TYPE;

	typedef struct {
		int id;
		int step_rate;
		int step_count;
		int swing;
		int slide;
	} CHANNEL;



	//static const int TICKS_PER_QTR_NOTE = 96;
	static const int TICKS_PER_16TH_NOTE = 24;
	static const int TICKS_PER_MIDI_TICK = 24; // MIDI 24PPQN
	static const int TICKS_LEFT_SHIFT = 48;


	static const TICKS_TYPE NUM_16THS_PER_CYCLE = 32;
//	static const TICKS_TYPE NUM_TICKS_PER_CYCLE = ((TICKS_TYPE)NUM_16THS_PER_CYCLE * TICKS_PER_16TH);
//	static const COUNTER_TYPE TICK_COUNTER_ROLLOVER = (((COUNTER_TYPE)NUM_TICKS_PER_CYCLE)<<TICKS_LEFT_SHIFT);

	static const int MAX_EVENTS = 4;


	COUNTER_TYPE m_counter;		// the time counter
	COUNTER_TYPE m_increment;	// the counter increment per millisecond

	enum
	{
	  RATE_1    = 2304,
	  RATE_2D   = 1728,
	  RATE_2    = 1152,
	  RATE_4D   = 864,
	  RATE_2T   = 768,
	  RATE_4    = 576,
	  RATE_8D   = 432,
	  RATE_4T   = 384,
	  RATE_8    = 288,
	  RATE_16D  = 216,
	  RATE_8T   = 192,
	  RATE_16   = 144,
	  RATE_16T  = 96,
	  RATE_32   = 72,
	  RATE_64   = 36,
	  RATE_128  = 18
	};

	byte m_ttt;
	volatile int m_ticks;
	CSeqClock() {
		m_ppn = 8;
		m_ticks = 0;
		m_zzz = 0;
		m_ttt = 0;
	}
	int m_ppn;
	CSeqClock::TICKS_TYPE m_zzz;


	void set_bpm(uint32_t value) {
		// calculate a 32 bit increment that will roll over after X milliseconds
		// where X is the number of miliseconds in a single tick

		// 120 BPM is 120 * 4 = 480 sixteenths per min
		// calculate total number of ticks per minute
		m_increment = (value * TICKS_PER_16TH_NOTE * 4);

		m_increment = (bpm * TICKS_PER_QTR_NOTE) << TICKS_LEFT_SHIFT; // 10 * ticks per minute at current BPM
		m_increment /= (60U * 1000U * 10U); // ticks per millisecond at current BPM
	}
	float get_bpm(void) {
		// TODO
		return 0;
	}
	// called once per millisecond
	void tick() {

#define QQQ (((COUNTER_TYPE)TICKS_PER_QTR_NOTE)<<48)
		m_counter = (m_counter + m_increment);
		if(m_counter >= QQQ) {
			m_ttt = 1;
			m_counter -= QQQ;
		}
		/*
		//TICKS_TYPE t = get_ticks();
		if(m_counter >= m_zzz) {
			m_zzz = m_zzz + (((COUNTER_TYPE)RATE_16)<<48);
			if(m_zzz >= TICK_COUNTER_ROLLOVER) {
				m_zzz -= TICK_COUNTER_ROLLOVER;
				m_ttt = 1;
			}
		}*/
		//if(++i >= 500) {
		//	i=0;
		//	m_ttt = 1;
		//}
	}
	byte ticked() {
		if(m_ttt) {
			m_ttt = 0;
			return 1;
		}
		return 0;
	}
	void reset() {
		m_counter = 0;
	}

	inline TICKS_TYPE get_ticks() {
		return (m_counter >> TICKS_LEFT_SHIFT);
	}
	static inline TICKS_TYPE add_ticks(TICKS_TYPE lhs, TICKS_TYPE rhs) {
		return 0;//(lhs + rhs)%NUM_TICKS_PER_CYCLE;
	}

	void init() {
	    kbi_config_t kbiConfig;
	    kbiConfig.mode = kKBI_EdgesDetect;
	    kbiConfig.pinsEnabled = 0x01; // KBI0 pin 0
	    kbiConfig.pinsEdge = 0; // Falling Edge
	    KBI_Init(KBI0, &kbiConfig);
	}
};
extern CSeqClock g_clock;
#ifdef MAIN_INCLUDE
CSeqClock g_clock;
CDigitalIn<kGPIO_PORTA, 0> pClockIn;
CDigitalOut<kGPIO_PORTC, 5> pClockOut;
extern "C" void KBI0_IRQHandler(void)
{
    if (KBI_IsInterruptRequestDetected(KBI0)) {
        KBI_ClearInterruptFlag(KBI0);
        g_clock.m_ticks++;
    }
}
#endif


#endif /* SEQ_CLOCK_H_ */
