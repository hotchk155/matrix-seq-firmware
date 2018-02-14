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
#include "i2c_bus.h"
#include <cv_gate.h>
#include "digital_out.h"
#include "delays.h"
#include "display_panel.h"
#include "sequence.h"
#include "grid.h"


/* TODO: insert other definitions and declarations here. */

 CDigitalOut<kGPIO_PORTB, 5> LED1;
 CDigitalOut<kGPIO_PORTC, 3> LED2;
 CDigitalOut<kGPIO_PORTC, 2> LED3;
 CDigitalOut<kGPIO_PORTE, 2> PowerControl;
 CDigitalIn<kGPIO_PORTE, 1> OffSwitch;

CI2CBus g_i2c_bus;
CCVGate g_cv_gate;

CGrid *g_grid = nullptr;
void fire_event(int event, uint32_t param) {
	g_grid->event(event, param);
}


/*
 * @brief   Application entry point.
 */
int main(void) {
  	/* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();

    CDelay::init();
    CDelay::wait_ms(500);
    PowerControl.set(1);

    //printf("Hello World\n");
    /* Force the counter to be placed into memory. */

    CSequence sequence;
    CGrid grid(sequence);

    g_grid = &grid;

    g_i2c_bus.init();
    g_i2c_bus.dac_init();

    /* Enter an infinite loop, just incrementing a counter.
     * */
    panelInit();
    sequence.test();

    int j=0;
    while(1) {
    	if(CDelay::g_ticked) {
    		sequence.tick();
    		grid.run();
    		CDelay::g_ticked = 0;
    	}
    	panelRun();
    	g_cv_gate.write(3, j);
        //g_cv.write(1, j);
        //g_cv.write(2, j);
        //g_cv.write(3, j);
    	g_cv_gate.run();
        if(++j>4095) j = 0;
    }
    return 0 ;
}
