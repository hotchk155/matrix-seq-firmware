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

#define MAIN_INCLUDE

#include <digital_out.h>
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MKE02Z4.h"
#include "fsl_clock.h"
#include "fsl_spi.h"
#include "fsl_pit.h"


#include "defs.h"
#include "chars.h"
#include "ui.h"
#include "clock.h"
#include "i2c_bus.h"
#include "storage.h"
#include "midi.h"
#include "cv_gate.h"
#include "scale.h"
#include "sequence_layer.h"
#include "digital_out.h"
#include "popup.h"
#include "sequencer.h"
#include "params.h"
#include "menu.h"
#include "selector.h"



/* TODO: insert other definitions and declarations here. */

 CIndicatorLED<kGPIO_PORTB, 5> LED1;
 CIndicatorLED<kGPIO_PORTC, 3> LED2;
 CIndicatorLED<kGPIO_PORTC, 2> LED3;
 CDigitalOut<kGPIO_PORTE, 2> PowerControl;
 CDigitalIn<kGPIO_PORTE, 1> OffSwitch;



typedef enum:byte {
	 VIEW_SEQUENCER,
	 VIEW_MENU,
	 VIEW_SELECTOR
 } VIEW_TYPE;

VIEW_TYPE g_view = VIEW_SEQUENCER;


void dispatch_event(int event, uint32_t param) {
	switch(g_view) {
	case VIEW_SEQUENCER:
		g_sequencer.event(event, param);
		break;
	case VIEW_MENU:
		g_menu.event(event, param);
		break;
	case VIEW_SELECTOR:
		g_selector.event(event, param);
		break;
	}
}

void select_layer(byte layer) {
	g_popup.layer(layer, g_sequencer.is_layer_enabled(layer));
	g_sequencer.set_active_layer(layer);
	force_full_repaint();
}

void fire_event(int event, uint32_t param) {

	switch(event) {
	///////////////////////////////////
	case EV_KEY_CLICK:
		switch(param) {
		case KEY_MENU:
			if(g_view != VIEW_MENU) {
				g_view = VIEW_MENU;
				g_menu.activate();
			}
			else {
				g_view = VIEW_SEQUENCER;
				g_menu.activate();
			}
			break;
		case KEY_MENU|KEY2_LAYER1:
			select_layer(0);
			break;
		case KEY_MENU|KEY2_LAYER2:
			select_layer(1);
			break;
		case KEY_MENU|KEY2_LAYER3:
			select_layer(2);
			break;
		case KEY_MENU|KEY2_LAYER4:
			select_layer(3);
			break;
		case KEY_MENU|KEY2_LAYER_MUTE:
			g_sequencer.enable_layer(g_sequencer.m_layer,!g_sequencer.is_layer_enabled(g_sequencer.m_layer));
			g_popup.layer(g_sequencer.m_layer, g_sequencer.is_layer_enabled(g_sequencer.m_layer));
			force_full_repaint();
			g_popup.align(CPopup::ALIGN_RIGHT);
			break;
		default:
			dispatch_event(event, param);
			break;
		}
		break;
	///////////////////////////////////
	case EV_KEY_PRESS:
		switch(param) {
		case KEY_RUN:
			fire_event(g_sequencer.is_running()? EV_SEQ_STOP : EV_SEQ_START, 0);
			break;
		default:
			dispatch_event(event, param);
			break;
		}
		break;
	///////////////////////////////////
	case EV_SEQ_STOP:
		g_sequencer.stop();
		g_popup.text("STOP", 4);
		g_popup.align(CPopup::ALIGN_RIGHT);
		break;
	///////////////////////////////////
	case EV_SEQ_RESTART:
		g_sequencer.reset();
		g_sequencer.start();
		g_popup.text("RST", 3);
		g_popup.align(CPopup::ALIGN_RIGHT);
		break;
	///////////////////////////////////
	case EV_SEQ_START:
		g_sequencer.start();
		g_popup.text("RUN", 3);
		g_popup.align(CPopup::ALIGN_RIGHT);
		break;
	default:
		dispatch_event(event, param);
		break;
	}
}

void fire_note(byte midi_note, byte midi_vel) {
	g_midi.send_note(0, midi_note, midi_vel);
}

void force_full_repaint() {
	g_popup.force_repaint();
	g_sequencer.force_repaint();
	g_menu.force_repaint();
	g_selector.force_repaint();
}


/*
 * @brief   Application entry point.
 */
int main(void) {
  	/* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();

    g_clock.init();
    g_clock.wait_ms(500);
    PowerControl.set(1);

    g_i2c_bus.init();
    g_i2c_bus.dac_init();
    g_midi.init();

    //g_storage.test();


    /* Enter an infinite loop, just incrementing a counter.
     * */
    g_ui.init();
    g_sequencer.m_layers[0].test();

    while(1) {

    	if(g_clock.m_ms_tick) {
    		g_clock.m_ms_tick = 0;

    		g_popup.run();
        	g_cv_gate.run();
        	g_midi.run();
       		g_sequencer.tick(g_clock.get_ticks(), g_clock.get_part_ticks());
        	g_ui.run();


    		g_ui.lock_for_update();
    		switch(g_view) {
    		case VIEW_SEQUENCER:
    			g_sequencer.repaint();
    			break;
    		case VIEW_MENU:
    			g_menu.repaint();
    			break;
    		case VIEW_SELECTOR:
    			g_selector.repaint();
    			break;
    		}
			g_popup.repaint();
			g_ui.unlock_for_update();

    		if(!OffSwitch.get()) {
    			PowerControl.set(0);
    		}

    	}
    }
    return 0 ;
}
