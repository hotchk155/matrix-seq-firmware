/*
 * Copyright (c) 2017, NXP Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of NXP Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 
/**
 * @file    MKE02Z64xxx4_Project.cpp
 * @brief   Application entry point.
 */
#include <digital_out.h>
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MKE02Z4.h"
/* TODO: insert other include files here. */
#define MAIN_INCLUDE


#include "defs.h"
#include "params.h"
#include "clock.h"
#include "i2c_bus.h"
#include "cv_gate.h"
#include "sequence_layer.h"
#include "midi.h"
#include "digital_out.h"
#include "display_panel.h"
#include "popup.h"
#include "sequencer.h"
#include "menu.h"



/* TODO: insert other definitions and declarations here. */

 CIndicatorLED<kGPIO_PORTB, 5> LED1;
 CIndicatorLED<kGPIO_PORTC, 3> LED2;
 CIndicatorLED<kGPIO_PORTC, 2> LED3;
 CDigitalOut<kGPIO_PORTE, 2> PowerControl;
 CDigitalIn<kGPIO_PORTE, 1> OffSwitch;



 enum {
	 VIEW_SEQUENCER,
	 VIEW_MENU
 };

byte g_view = VIEW_SEQUENCER;

void set_param(PARAM_ID param, int value) {
	if(param < P_SQL_MAX) {
		g_sequencer.set(param,value);
	}
	else if(param < P_CVGATE_MAX) {
		g_cv_gate.set(g_sequencer.get_active_layer(),param,value);
	}
}
int get_param(PARAM_ID param) {
	if(param < P_SQL_MAX) {
		return g_sequencer.get(param);
	}
	else if(param < P_CVGATE_MAX) {
		return g_cv_gate.get(g_sequencer.get_active_layer(),param);
	}
	return 0;
}



void fire_event(int event, uint32_t param) {

	if(event == EV_KEY_PRESS) {
		switch(param) {
		case KEY_L1:
			if(g_view == VIEW_SEQUENCER) {
				g_view = VIEW_MENU;
				g_menu.update();
			}
			else {
				g_view = VIEW_SEQUENCER;
			}
			return;
		case KEY_L5: g_sequencer.set_active_layer(3); g_menu.update(); return;
		case KEY_L6: g_sequencer.set_active_layer(2); g_menu.update(); return;
		case KEY_L7: g_sequencer.set_active_layer(1); g_menu.update(); return;
		case KEY_L8: g_sequencer.set_active_layer(0); g_menu.update(); return;
		}
	}

	switch(g_view) {
	case VIEW_SEQUENCER:
		g_sequencer.event(event, param);
		break;
	case VIEW_MENU:
		g_menu.event(event, param);
		break;
	}
}

void fire_note(byte midi_note, byte midi_vel) {
	g_midi.send_note(0, midi_note, midi_vel);
	//LED1.set(!!midi_vel);
}


/*
 * @brief   Application entry point.
 */
int main(void) {
  	/* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();


    //printf("Hello World\n");
    /* Force the counter to be placed into memory. */



    g_i2c_bus.init();
    g_i2c_bus.dac_init();
    g_midi.init();
    g_clock.init();


    g_clock.wait_ms(500);
    PowerControl.set(1);

    /* Enter an infinite loop, just incrementing a counter.
     * */
    panelInit();
    g_sequencer.m_layers[0].test();

    /*for(;;) {
    LED1.set(0);
    CDelay::wait_ms(500);
    LED1.set(1);
    CDelay::wait_ms(500);
    }*/

    while(1) {

    	if(g_clock.m_ms_tick) {
    		g_popup.run();
    		g_clock.m_ms_tick = 0;
        	g_cv_gate.run();
        	g_sequencer.tick(g_clock.m_ticks, (byte)(256*g_clock.m_part_tick));
    		//g_sequencer.run();

    		if(!OffSwitch.get()) {
    			PowerControl.set(0);
    		}

/*       		if(j<500) {
       			LED3.set(1);
       		}
       		else {
       			LED3.set(0);
       		}
       		if(++j>=1000) {
       			j = 0;
       		}*/
        	panelRun();

    		CRenderBuf::lock();
    		switch(g_view) {
    		case VIEW_SEQUENCER:
    			CRenderBuf::clear();
    			g_sequencer.repaint();
    			g_popup.repaint();
    			break;
    		case VIEW_MENU:
    			g_menu.repaint();
    			break;
    		}
    		CRenderBuf::unlock();


    	}
    	//g_cv_gate.write(3, j);
        //g_cv.write(1, j);
        //g_cv.write(2, j);
        //g_cv.write(3, j);
        //if(++j>4095) j = 0;

   		//sequence.run();
    	/*
    	if(g_clock.is_pending(CSeqClock::CHANNEL_SEQ1)) {
    		g_seq[0].step();
    	}
    	if(g_clock.is_pending(CSeqClock::CHANNEL_SEQ2)) {
    		g_seq[1].step();
    	}
    	if(g_clock.is_pending(CSeqClock::CHANNEL_SEQ3)) {
    		g_seq[2].step();
    	}
    	if(g_clock.is_pending(CSeqClock::CHANNEL_SEQ4)) {
    		g_seq[3].step();
    	}
*/

    }
    return 0 ;
}
