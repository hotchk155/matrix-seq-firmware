/*
 * midi.h
 *
 *  Created on: 14 Feb 2018
 *      Author: jason
 */

#ifndef MIDI_H_
#define MIDI_H_

#include "fsl_uart.h"

class CMidi {

public:

	// MIDI receive buffer
	enum {
		RXBUF_SIZE = 32,
		RXBUF_SIZE_MASK = 0x1F
	};
	volatile byte m_rxbuf[RXBUF_SIZE];
	volatile byte m_rx_head;
	volatile byte m_rx_tail;


public:
	enum {
		MIDI_TICK		= 0xF8,
		MIDI_START		= 0xFA,
		MIDI_CONTINUE	= 0xFB,
		MIDI_STOP		= 0xFC
	};
	CMidi() {
		m_rx_head = 0;
		m_rx_tail = 0;
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

	// once per ms
	void run() {

	}
};

#ifdef MAIN_INCLUDE
CMidi g_midi;

extern "C" void UART0_IRQHandler(void)
{
	g_midi.irq_handler();
}
#endif

#endif /* MIDI_H_ */
