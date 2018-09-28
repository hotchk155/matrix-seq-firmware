//////////////////////////////////////////////////////////////////////////////
//
// MATRIX SEQUENCER
// 2018/Sixty Four Pixels
//
// Beat clock generator
//
//////////////////////////////////////////////////////////////////////////////
#ifndef CLOCK_H_
#define CLOCK_H_


// define the GPIO pins used for clock (will initialise the port)
CDigitalIn<kGPIO_PORTA, 0> g_clock_in;
CPulseOut<kGPIO_PORTC, 5> g_clock_out;

/////////////////////////////////////////////////////////////////
// This class maintains a count of 24ppqn ticks based on the
// current BPM or the external MIDI clock. Internal clock is
// generated based on a once-per-millisecond interrupt
class CClock {


public:

	// Define different musical beat intervals based on
	// number of 24ppqn ticks
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
	volatile double m_ticks_per_ms;
	volatile double m_part_tick;
	volatile byte m_ms_tick;
	volatile uint32_t m_ticks;
	//uint32_t m_midi_ticks;
	volatile byte m_beat_count;
	volatile byte m_pulse_clock_count;
	byte m_pulse_clock_div;

	typedef struct {
		V_CLOCK_SRC m_source;
		byte m_pulse_clock_div;
	} CONFIG;
	CONFIG m_cfg;

	///////////////////////////////////////////////////////////////////////////////
	CClock() {
		init_config();
		init_state();
	}

	///////////////////////////////////////////////////////////////////////////////
	void set_bpm(float bpm) {
		m_bpm = bpm;
		m_ticks_per_ms = ((double)bpm * RATE_4) / (60.0 * 1000.0);
	}

	int get_ms_for_measure(int measure) {
		return measure/m_ticks_per_ms;
	}

	///////////////////////////////////////////////////////////////////////////////
	void init_state() {
		m_ms_tick = 0;
		set_bpm(120);
		on_restart();
	}

	///////////////////////////////////////////////////////////////////////////////
	void init_config() {
		m_cfg.m_source = V_CLOCK_SRC_INTERNAL;
		m_cfg.m_pulse_clock_div = RATE_16;
	}

	///////////////////////////////////////////////////////////////////////////////
	void init() {
		// configure a timer to cause an interrupt once per millisecond
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

	inline void private_on_tick() {
		if(!m_beat_count) {
			g_tempo_led.blink(g_tempo_led.MEDIUM_BLINK);
		}
		if(++m_beat_count >= 24) {
			m_beat_count = 0;
		}

		if(!m_pulse_clock_count) {
			g_clock_out.blink(15);
		}
		if(++m_pulse_clock_count >= m_cfg.m_pulse_clock_div) {
			m_pulse_clock_count = 0;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	void set(PARAM_ID param, int value) {
		switch(param) {
			case P_CLOCK_BPM: set_bpm(value); break;
			case P_CLOCK_SRC: m_cfg.m_source = (V_CLOCK_SRC)value; break;
		default:
			break;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	int get(PARAM_ID param) {
		switch(param) {
		case P_CLOCK_BPM: return m_bpm;
		case P_CLOCK_SRC: return m_cfg.m_source;
		default: return 0;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	int is_valid_param(PARAM_ID param) {
		switch(param) {
		case P_CLOCK_BPM: return !!(m_cfg.m_source == V_CLOCK_SRC_INTERNAL);
		default: return 1;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	void wait_ms(int ms) {
		while(ms) {
			m_ms_tick = 0;
			while(!m_ms_tick);
			--ms;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	inline uint32_t get_ticks() {
		return m_ticks;
	}

	///////////////////////////////////////////////////////////////////////////////
	inline byte get_part_ticks() {
		if(m_cfg.m_source == V_CLOCK_SRC_INTERNAL) {
			return (byte)(256*m_part_tick);
		}
		else {
			//TODO..?
			return 0;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	void on_midi_tick() {
		if(m_cfg.m_source == V_CLOCK_SRC_MIDI) {
			++m_ticks;
			private_on_tick();
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	void on_restart() {
		m_ticks = 0;
		m_part_tick = 0.0;
		m_beat_count = 0;
		m_pulse_clock_count = 0;
	}

	///////////////////////////////////////////////////////////////////////////////
	// Interrupt service routine called once per millisecond
	inline void tick_isr() {

		// set flag to indicate that a milliecond has elapsed
		// this is used for general timing purposes
		m_ms_tick = 1;

		if(m_cfg.m_source == V_CLOCK_SRC_INTERNAL) {
			// add the fractional number of ticks per millisecond to
			// the tick counter and see whether we now have at least
			// one complete tick
			m_part_tick += m_ticks_per_ms;
			int whole_tick = (int)m_part_tick;
			if(whole_tick) {
				m_ticks ++;
				m_part_tick -= whole_tick;
				private_on_tick();
			}
		}
	}
};



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// define the clock instance
CClock g_clock;


// ISR for the millisecond timer
extern "C" void PIT_CH0_IRQHandler(void) {
	PIT_ClearStatusFlags(PIT, kPIT_Chnl_0, kPIT_TimerFlag);
	g_clock.tick_isr();
}

// ISR for the KBI interrupt (SYNC IN)
extern "C" void KBI0_IRQHandler(void)
{
    if (KBI_IsInterruptRequestDetected(KBI0)) {
//TODO - clock based on interpolated pulse in
        KBI_ClearInterruptFlag(KBI0);
    }
}

#endif // CLOCK_H_
