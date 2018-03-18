///////////////////////////////////////////////////////////////////////////////
// MATRIX SEQUENCER
// Sixty four pixels Ltd	March 2018
//
// MIDI HANDLING
#ifndef MIDI_H_
#define MIDI_H_

#include "fsl_uart.h"

class CMidi {

public:

	// MIDI receive buffer
	enum {
		RXBUF_SIZE = 32,
		RXBUF_SIZE_MASK = 0x1F,
		MAX_PLAYING_NOTES = 16
	};
	enum {
		MIDI_TICK		= 0xF8,
		MIDI_START		= 0xFA,
		MIDI_CONTINUE	= 0xFB,
		MIDI_STOP		= 0xFC
	};
	volatile byte m_rxbuf[RXBUF_SIZE];
	volatile byte m_rx_head;
	volatile byte m_rx_tail;

	typedef struct {
		uint16_t key;
		uint16_t count;
	} PLAYING_NOTE;

	PLAYING_NOTE m_notes[MAX_PLAYING_NOTES];

	CMidi() {
		m_rx_head = 0;
		m_rx_tail = 0;
		memset(m_notes, 0, sizeof(m_notes));
	}

	/////////////////////////////////////////////////////////////////////////////////
	// INITIALISE MIDI
	void init() {
	    uart_config_t config;
	    UART_GetDefaultConfig(&config);
	    config.baudRate_Bps = 31250;
	    config.enableTx = true;
	    config.enableRx = true;
	    UART_Init(UART0, &config, CLOCK_GetFreq(kCLOCK_BusClk));
	    UART_EnableInterrupts(UART0, kUART_RxDataRegFullInterruptEnable | kUART_RxOverrunInterruptEnable);
	    EnableIRQ(UART0_IRQn);
	}



	/////////////////////////////////////////////////////////////////////////////////
	// TRANSMIT A CHARACTER (BLOCKING)
	void send_byte(byte ch) {
		while(!(kUART_TxDataRegEmptyFlag & UART_GetStatusFlags(UART0)));
		UART_WriteByte(UART0, ch);
	}
	void send_note(byte chan, byte note, byte velocity) {
		send_byte(0x90 | chan);
		send_byte(note & 0x7F);
		send_byte(velocity & 0x7F);
	}

	inline void irq_handler() {
	    // character received at UART
	    if ((kUART_RxDataRegFullFlag | kUART_RxOverrunFlag) & UART_GetStatusFlags(UART0))
	    {
	        byte data = UART_ReadByte(UART0); // clears the status flags
	        byte next = (m_rx_head+1)%RXBUF_SIZE_MASK;
			if(next != m_rx_tail) {
				m_rxbuf[m_rx_head] = data;
				m_rx_head = next;
				// TODO: flag overflow!
			}
	    }
	}

	/////////////////////////////////////////////////////////////////////////////////
	// once per ms
	void run() {
		for(int i=0; i<MAX_PLAYING_NOTES; ++i) {
			if(m_notes[i].key) {
				if(!m_notes[i].count) {
					send_note((m_notes[i].key >> 8) & 0xF, (byte)m_notes[i].key, 0);
					m_notes[i].key = 0;
				}
				else {
					--m_notes[i].count;
				}
			}
		}
	}

	/////////////////////////////////////////////////////////////////////////////////
	void play_note(byte chan, byte note, byte velocity, int duration, byte replace) {

		// get the "key" value for the new note
		uint16_t new_key = 0x8000U | ((uint16_t)chan) << 8 | note;

		// if we are replacing a specific old note for legato playing
		// then form key value otherwise there is no special key to search for
		uint16_t old_key;
		if(replace) {
			old_key = 0x8000U | ((uint16_t)chan) << 8 | replace;
		}
		else {
			old_key = 0;
		}

		int slot;
		int new_slot = -1;
		int steal_slot = -1;
		byte legato = 0;
		uint16_t steal_count = 0xFFFF;
		for(slot=0; slot < MAX_PLAYING_NOTES; ++slot) {

			// this slot unused?
			if(!m_notes[slot].key) {
				if(!old_key) {
					// no specific slot to go for so
					// this one will do
					break;
				}
				// otherwise remember this slot in case
				// it is needed
				if(new_slot < 0) {
					new_slot = slot;
				}
			}
			else if(m_notes[slot].key == new_key) {
				// the new note is already playing - we will
				// just re-use the same slot
				break;
			}
			else if(m_notes[slot].key == old_key) {
				// we found the old note to replace
				// so we will be playing legato
				legato = 1;
				break;
			}
			else if(m_notes[slot].count < steal_count){
				// track the note that has the least amount
				// of time to play, so we can steal its slot
				// if there are no spare ones
				steal_count = m_notes[slot].count;
				steal_slot = slot;
			}
		}

		// if we completed the search without finding
		// the replacement note / free slot
		if(slot >= MAX_PLAYING_NOTES) {
			if(new_slot >= 0) {
				// grab free slot
				slot = new_slot;
			}
			else {
				// steal slot
				slot = steal_slot;
			}
		}

		if(slot >= 0) {
			if(legato) {
				// start playing the new note before stopping old
				send_note(chan, note, velocity);
			}

			if(m_notes[slot].key) {
				// stop the old note
				send_note((m_notes[slot].key >> 8) & 0xF, (byte)m_notes[slot].key, 0);
			}
			m_notes[slot].key = new_key;
			m_notes[slot].count = duration;

			if(!legato) {
				// start playing the new note after stopping old
				send_note(chan, note, velocity);
			}

		}
	}

};

extern CMidi g_midi;
#ifdef MAIN_INCLUDE
CMidi g_midi;
extern "C" void UART0_IRQHandler(void)
{
	g_midi.irq_handler();
}
#endif

#endif /* MIDI_H_ */
