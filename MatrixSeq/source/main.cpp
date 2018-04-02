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
#include "chars.h"
#include "params.h"
#include "clock.h"
#include "i2c_bus.h"
#include "storage.h"
#include "midi.h"
#include "cv_gate.h"
#include "scale.h"
#include "sequence_layer.h"
#include "digital_out.h"
#include "display_panel.h"
#include "popup.h"
#include "sequencer.h"
#include "menu.h"
#include "selector.h"



/* TODO: insert other definitions and declarations here. */

 CIndicatorLED<kGPIO_PORTB, 5> LED1;
 CIndicatorLED<kGPIO_PORTC, 3> LED2;
 CIndicatorLED<kGPIO_PORTC, 2> LED3;
 CDigitalOut<kGPIO_PORTE, 2> PowerControl;
 CDigitalIn<kGPIO_PORTE, 1> OffSwitch;



 enum {
	 VIEW_SEQUENCER,
	 VIEW_MENU,
	 VIEW_SELECTOR
 };

byte g_view = VIEW_SEQUENCER;
byte g_current_layer = 0;

void set_param(PARAM_ID param, int value) {
	if(param < P_SQL_MAX) {
		g_sequencer.set(param,value);
	}
	else if(param < P_CLOCK_MAX) {
		g_clock.set(param,value);
	}
}
int get_param(PARAM_ID param) {
	if(param < P_SQL_MAX) {
		return g_sequencer.get(param);
	}
	else if(param < P_CLOCK_MAX) {
		return g_clock.get(param);
	}
	return 0;
}
int is_valid_param(PARAM_ID param) {
	if(param < P_SQL_MAX) {
		return g_sequencer.is_valid_param(param);
	}
	else if(param < P_CLOCK_MAX) {
		return g_clock.is_valid_param(param);
	}
	return 0;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
void format_number(int value, char *buf, int digits) {
	if(digits > 2) {
		*buf++ = '0' + value/100;
	}
	value %= 100;
	if(digits > 1) {
		*buf++ = '0' + value/10;
	}
	value %= 10;
	*buf++ = '0' + value;
	*buf = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
const char *param_value_string(PARAM_TYPE type, int value, const char *values_text) {
	static char buf[9];
	int pos;
	switch(type) {
	case PT_ENUMERATED:
		while(values_text && *values_text && value) {
			if(*values_text == '|') {
				--value;
			}
			++values_text;
		}
		pos = 0;
		while(*values_text && *values_text != '|') {
			buf[pos++] = *values_text++;
		}
		buf[pos] = 0;
		break;
	case PT_MIDI_CHANNEL:
		format_number(value + 1, buf, 2);
		break;
	case PT_DURATION:
		format_number(value, buf, 2);
		break;
	case PT_NUMBER_7BIT:
	case PT_BPM:
		format_number(value, buf, 3);
		break;
	case PT_VOLT_RANGE:
		format_number(value, buf, 1);
		buf[1] = 'V';
		buf[2] = 0;
		break;
	case PT_PATTERN:
		buf[0] = 'A' + value/8;
		buf[1] = '1' + value%8;
		buf[2] = 0;
		break;
	default:
		buf[0] = 0;
		break;
	}
	return buf;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
int param_max_value(PARAM_TYPE type, const char *values_text) {
	int count;
	switch(type) {
	case PT_ENUMERATED:
		count = 0;
		while(values_text && *values_text) {
			if(*values_text == '|') {
				++count;
			}
			++values_text;
		}
		return count;
	case PT_MIDI_CHANNEL:
		return 15;
	case PT_NUMBER_7BIT:
		return 127;
	case PT_VOLT_RANGE:
		return 8;
	case PT_BPM:
		return 300;
	case PT_PATTERN:
		return 39;
	default:
		return 0;
	}
}
/////////////////////////////////////////////////////////////////////////////////////////////////
int param_min_value(PARAM_TYPE type) {
	switch(type) {
	case PT_BPM:
		return 30;
	default:
		return 0;
	}
}


/*
enum {
	ACTION_NONE,
	ACTION_MENU_PRESS,
	ACTION_MENU_DRAG
};
int g_action = ACTION_NONE;*/
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

enum {
	MENU_PRESS_DOWN = 1, // menu key has been pressed but no action specified yet
	MENU_PRESS_SHIFT = 2,
	MENU_PRESS_LAYER_COPY = 3 // a shifted layer button has been pressed
};
byte g_menu_press= 0;
char g_selected_layer = -1;

//////////////////////////////////////////////////////////////
// one of the layer buttons has been pressed while the menu button
// is held
void layer_button_event(int event, uint32_t param) {
	byte layer = 0;
	switch(param) {
	case KEY2_LAYER2: layer = 1; break;
	case KEY2_LAYER3: layer = 2; break;
	case KEY2_LAYER4: layer = 3; break;
	}
	if(event == EV_KEY_PRESS) {
		//
		if(g_menu_press == MENU_PRESS_LAYER_COPY) {
			char text[4] = {(char)('1'+g_current_layer), '-', '>', (char)('1'+layer)};
			g_sequencer.copy_layer(g_current_layer, layer);
			g_popup.text(text,4);
			g_popup.align(CPopup::ALIGN_RIGHT);
		}
		else {
			// the current layer button is not currently
			// held so we are selecting a new layer
			g_current_layer = layer;
			g_popup.layer(g_current_layer, g_sequencer.is_layer_enabled(g_current_layer));
			g_sequencer.set_active_layer(g_current_layer);
			force_full_repaint();
			g_menu_press = MENU_PRESS_LAYER_COPY;
		}
	}
	else {
		if(layer == g_current_layer) {
			// the button for the current layer has been
			// released, so copying is no longer active
			g_menu_press = MENU_PRESS_SHIFT;
		}
	}
}
void fire_event(int event, uint32_t param) {

	switch(event) {
	//////////////////////////////////////////////////////////////
	case EV_KEY_PRESS:
		// when menu key initially pressed, remember but do not take action yet
		if(param == KEY_MENU && !g_menu_press) {
			g_menu_press = MENU_PRESS_DOWN;
		}
		if(g_menu_press && (param == KEY2_LAYER1 || param == KEY2_LAYER2 || param == KEY2_LAYER3 ||  param == KEY2_LAYER4)) {
			layer_button_event(event,param);
		}
		else if(param == KEY_STARTSTOP) {
			if(g_menu_press) {
				g_sequencer.enable_layer(g_current_layer,!g_sequencer.is_layer_enabled(g_current_layer));
				g_popup.layer(g_current_layer, g_sequencer.is_layer_enabled(g_current_layer));
				force_full_repaint();
				g_menu_press = MENU_PRESS_SHIFT;
				g_popup.align(CPopup::ALIGN_RIGHT);
			}
			else {
				fire_event(g_sequencer.is_running()? EV_SEQ_STOP : EV_SEQ_START, 0);
			}
		}
		else if(param == KEY_STORE) {
			g_view = VIEW_SELECTOR;
			g_selector.activate("LOAD FR", P_SQL_LOAD_PATTERN, PT_PATTERN, NULL);
		}
		else {
			// pass event to active view
			dispatch_event(event, param);
		}
		break;

	//////////////////////////////////////////////////////////////
	case EV_KEY_RELEASE:
		// is menu button pressed and released without any action?
		// if so we use that to toggle between menu and sequencer
		// views
		if(param == KEY_MENU && g_menu_press == MENU_PRESS_DOWN) {
			if(g_view == VIEW_MENU) {
				g_view = VIEW_SEQUENCER;
				g_sequencer.activate();
			}
			else {
				g_view = VIEW_MENU;
				g_menu.activate();
			}
		}
		if(g_menu_press && (param == KEY2_LAYER1 || param == KEY2_LAYER2 || param == KEY2_LAYER3 || param == KEY2_LAYER4)) {
			layer_button_event(event,param);
		}
		else {
			// pass event to active view
			dispatch_event(event,param);
		}
		// in any case remember when menu button is released
		if(param == KEY_MENU) {
			g_menu_press = 0;
			g_selected_layer = -1;
		}
		break;

	//////////////////////////////////////////////////////////////
	case EV_ENCODER:
		if(g_menu_press && g_view == VIEW_SEQUENCER) {
			g_sequencer.scroll((int)param);
		}
		else {
			dispatch_event(event,param);
		}
		if(g_menu_press) {
			g_menu_press = MENU_PRESS_SHIFT;
		}
		break;
	case EV_SEQ_STOP:
		g_sequencer.stop();
		g_popup.text("STOP", 4);
		g_popup.align(CPopup::ALIGN_RIGHT);
		break;
	case EV_SEQ_RESTART:
		g_sequencer.reset();
		g_sequencer.start();
		g_popup.text("RST", 3);
		g_popup.align(CPopup::ALIGN_RIGHT);
		break;
	case EV_SEQ_START:
		g_sequencer.start();
		g_popup.text("RUN", 3);
		g_popup.align(CPopup::ALIGN_RIGHT);
		break;
	}
}

void fire_note(byte midi_note, byte midi_vel) {
	g_midi.send_note(0, midi_note, midi_vel);
	//LED1.set(!!midi_vel);
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


    //printf("Hello World\n");
    /* Force the counter to be placed into memory. */



    g_clock.init();
    g_clock.wait_ms(500);
    PowerControl.set(1);

    g_i2c_bus.init();
    g_i2c_bus.dac_init();
    g_midi.init();

    //g_storage.test();


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
        	g_midi.run();

       		g_sequencer.tick(g_clock.get_ticks(), g_clock.get_part_ticks());

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
