///////////////////////////////////////////////////////////////////////////////////
//
//                                  ~~  ~~             ~~
//  ~~~~~~    ~~~~~    ~~~~~    ~~~~~~  ~~     ~~~~~   ~~~~~~    ~~~~~   ~~    ~~
//  ~~   ~~  ~~   ~~  ~~   ~~  ~~   ~~  ~~    ~~   ~~  ~~   ~~  ~~   ~~   ~~  ~~
//  ~~   ~~  ~~   ~~  ~~   ~~  ~~   ~~  ~~    ~~~~~~~  ~~   ~~  ~~   ~~     ~~
//  ~~   ~~  ~~   ~~  ~~   ~~  ~~   ~~  ~~    ~~       ~~   ~~  ~~   ~~   ~~  ~~
//  ~~   ~~   ~~~~~    ~~~~~    ~~~~~~   ~~~   ~~~~~   ~~~~~~    ~~~~~   ~~    ~~
//
//  Serendipity Sequencer                                   CC-NC-BY-SA
//  hotchk155/2018                                          Sixty-four pixels ltd
//
//  CV / GATE DRIVER
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef CV_GATE_H_
#define CV_GATE_H_

//
// MACRO DEFS
//
#define PORTD_BIT_GATE4 4
#define PORTD_BIT_GATE2 3
#define PORTD_BIT_GATE3 2
#define PORTA_BIT_GATE1 6

#define BIT_GATE4		MK_GPIOA_BIT(PORTD_BASE, PORTD_BIT_GATE4)
#define BIT_GATE2		MK_GPIOA_BIT(PORTD_BASE, PORTD_BIT_GATE2)
#define BIT_GATE3		MK_GPIOA_BIT(PORTD_BASE, PORTD_BIT_GATE3)
#define BIT_GATE1		MK_GPIOA_BIT(PORTA_BASE, PORTA_BIT_GATE1)

//
// GLOBAL DATA
//

// These definitions are used to initialise the port
CDigitalOut<kGPIO_PORTA, PORTA_BIT_GATE1> g_gate_1;
CDigitalOut<kGPIO_PORTD, PORTD_BIT_GATE2> g_gate_2;
CDigitalOut<kGPIO_PORTD, PORTD_BIT_GATE3> g_gate_3;
CDigitalOut<kGPIO_PORTD, PORTD_BIT_GATE4> g_gate_4;

/////////////////////////////////////////////////////////////////////////////////
//
// CLASS WRAPS UP CV AND GATE FUNCTIONS
//
/////////////////////////////////////////////////////////////////////////////////
class CCVGate {
public:
	typedef enum:byte  {
		GATE_CLOSED,
		GATE_OPEN,
		GATE_RETRIG,
	} GATE_STATE;
	enum {
		MAX_CV = 4,
		MAX_GATE = 4,
		I2C_BUF_SIZE = 100
	};

	typedef struct {
		int pitch;			// 32-bit current pitch value (dac << 16)
		int target;  		// 32-bit current target value (dac << 16)
		int glide_rate;  	// glide rate applied per ms to the pitch
		uint16_t dac;		// raw 12-bit DAC value
	} CV_STATE;


	enum {
		DAC_INIT_PENDING_1,
		DAC_INIT_PENDING_2,
		DAC_DATA_PENDING,
		DAC_IDLE
	};

	CV_STATE m_cv[MAX_CV];
	GATE_STATE m_gate[MAX_GATE];
	byte m_dac_state;
	byte m_gate_pending;
	byte m_i2c_buf[I2C_BUF_SIZE];
	volatile CI2CBus::TRANSACTION m_txn;

	/////////////////////////////////////////////////////////////////////////////////
	void gate_on(byte which) {
		switch(which) {
		case 0: SET_GPIOA(BIT_GATE1); break;
		case 1: SET_GPIOA(BIT_GATE2); break;
		case 2: SET_GPIOA(BIT_GATE3); break;
		case 3: SET_GPIOA(BIT_GATE4); break;
		}
	}

	/////////////////////////////////////////////////////////////////////////////////
	void gate_off(byte which) {
		switch(which) {
		case 0: CLR_GPIOA(BIT_GATE1); break;
		case 1: CLR_GPIOA(BIT_GATE2); break;
		case 2: CLR_GPIOA(BIT_GATE3); break;
		case 3: CLR_GPIOA(BIT_GATE4); break;
		}
	}

	void impl_set_cv(int which, uint16_t dac)
	{
		// get the appropriate offset into the DAC structure
		// for the target CV output
/*		int ofs;
		switch(which) {
		case 1: ofs = 1; break;
		case 2: ofs = 2; break;
		case 3: ofs = 0; break;
		default: ofs = 3; break;
		}*/

		if(m_dac_state >= DAC_DATA_PENDING) {
			// if the output has changed then load the new one
			if(m_cv[which].dac != dac) {
				m_cv[which].dac = dac;
				m_dac_state = DAC_DATA_PENDING;
			}
		}
	}

	// pitch is defined a 256 * midi note number + fractional part
	typedef uint16_t PITCH_TYPE;

	// gate type is defined as milliseconds. zero is infinite
	typedef uint16_t GATE_TYPE;

	// cv type is a 16 bit value to be mapped to the full voltage range
	typedef uint16_t CV_TYPE;

	typedef uint16_t DAC_TYPE;


public:


	/////////////////////////////////////////////////////////////////////////////////
	CCVGate()
	{
		memset((byte*)m_cv,0,sizeof m_cv);
		memset((byte*)m_gate,0,sizeof m_gate);
		m_gate_pending = 0;
		m_dac_state = DAC_INIT_PENDING_1;

		m_txn.addr = I2C_ADDR_DAC;
		m_txn.location = 0;
		m_txn.location_size = 0;
		m_txn.data = m_i2c_buf;
		m_txn.data_len = I2C_BUF_SIZE;
		m_txn.status  = kStatus_Success;
		m_txn.pending = 0;
	}


	/////////////////////////////////////////////////////////////////////////////////
	void gate(byte which, GATE_STATE gate) {
		if(m_gate[which] != gate) {
			switch(gate) {
				case GATE_CLOSED:
					gate_off(which);
					m_gate[which] = GATE_CLOSED;
					break;
				case GATE_RETRIG:
					gate_off(which);
					m_gate[which] = GATE_OPEN;
					m_gate_pending = 1;
					g_gate_led.blink(g_gate_led.MEDIUM_BLINK);
					break;
				case GATE_OPEN:
					gate_on(which);
//TODO - gate sync with CV change on pitch channel
					m_gate[which] = GATE_OPEN;
					m_gate_pending = 1;
					g_gate_led.blink(g_gate_led.MEDIUM_BLINK);
					break;
			}
		}
	}

	/////////////////////////////////////////////////////////////////////////////////
	void close_all_gates() {
		for(int i=0; i<MAX_GATE; ++i) {
			gate_off(i);
			m_gate[i] = GATE_CLOSED;
		}
	}

	/////////////////////////////////////////////////////////////////////////////////
	void pitch_cv(int which, int note, V_SQL_CVSCALE scaling, int glide_time) {
		// convert the note to DAC value
		// TODO support other note/cv mappings
		// Ensure enough headroom from psu!

		while(note > 96) {
			note -= 12;
		}
		while(note < 0) {
			note += 12;
		}

		uint16_t dac = (500 * (int)note)/12;

		if(glide_time) {
			m_cv[which].target = dac<<16;
			m_cv[which].glide_rate = (m_cv[which].target - m_cv[which].pitch)/glide_time;
		}
		else {
			m_cv[which].pitch = dac<<16;
			m_cv[which].glide_rate = 0;
			impl_set_cv(which, dac);
		}
	}


	/////////////////////////////////////////////////////////////////////////////////
	void mod_cv(int which, int value, int volt_range, int value2, int sweep_time) {
		m_cv[which].pitch = (500*volt_range*value)<<9; // divide by 128 then left shift by 16
		if(sweep_time) {
			m_cv[which].target = (500*volt_range*value2)<<9;
			m_cv[which].glide_rate = (m_cv[which].target - m_cv[which].pitch)/sweep_time;
		}
		else {
			m_cv[which].glide_rate = 0;
		}

		uint16_t dac = m_cv[which].pitch>>16;
		impl_set_cv(which, dac);
	}


	/////////////////////////////////////////////////////////////////////////////////
	// get block of data to send to i2c
	void run_i2c() {
		if(g_i2c_bus.busy()) {
			return;
		}
		switch(m_dac_state) {
		case DAC_INIT_PENDING_1:
			m_i2c_buf[0] = 0b10001111; // set each channel to use internal vref
			m_txn.data_len = 1;
			g_i2c_bus.transmit(&m_txn);
			m_dac_state = DAC_INIT_PENDING_2;
			break;
		case DAC_INIT_PENDING_2:
			m_i2c_buf[0] = 0b11001111; // set x2 gain on each channel
			m_txn.data_len = 1;
			g_i2c_bus.transmit(&m_txn);
			m_dac_state = DAC_IDLE;
			break;
		case DAC_DATA_PENDING:
			m_i2c_buf[0] = ((m_cv[3].dac>>8) & 0xF);
			m_i2c_buf[1] = (byte)m_cv[3].dac;
			m_i2c_buf[2] = ((m_cv[2].dac>>8) & 0xF);
			m_i2c_buf[3] = (byte)m_cv[2].dac;
			m_i2c_buf[4] = ((m_cv[1].dac>>8) & 0xF);
			m_i2c_buf[5] = (byte)m_cv[1].dac;
			m_i2c_buf[6] = ((m_cv[0].dac>>8) & 0xF);
			m_i2c_buf[7] = (byte)m_cv[0].dac;
			m_txn.data_len = 8;
			g_i2c_bus.transmit(&m_txn);
			g_cv_led.blink(g_cv_led.MEDIUM_BLINK);
			m_dac_state = DAC_IDLE;
			break;
		case DAC_IDLE:
		default:
			break;
		}
	}

	/////////////////////////////////////////////////////////////////////////////////
	// called once per ms
	void run() {
		for(int i=0; i<MAX_CV; ++i) {
			if(m_cv[i].glide_rate) {
				m_cv[i].pitch += m_cv[i].glide_rate;
				if(m_cv[i].glide_rate < 0) {
					if(m_cv[i].pitch <= m_cv[i].target) {
						m_cv[i].pitch = m_cv[i].target;
						m_cv[i].glide_rate = 0;
					}
				}
				else {
					if(m_cv[i].pitch >= m_cv[i].target) {
						m_cv[i].pitch = m_cv[i].target;
						m_cv[i].glide_rate = 0;
					}
				}
				int dac = m_cv[i].pitch>>16;
				impl_set_cv(i, dac);
			}
		}
	}

};

// define global instance of the CV/Gate controller
CCVGate g_cv_gate;

#endif /* CV_GATE_H_ */
