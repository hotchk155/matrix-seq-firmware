/*
 * delays.h
 *
 *  Created on: 4 Feb 2018
 *      Author: jason
 */

#ifndef DELAYS_H_
#define DELAYS_H_
#include "fsl_common.h"
#include "fsl_clock.h"
#include "fsl_pit.h"

 class CDelay {
 public:
	 volatile static uint32_t g_ticks;
	 volatile static bool g_ticked;

	 static void init() {
		g_ticks = 0;
		g_ticked = 0;
	   CLOCK_EnableClock(kCLOCK_Pit0);
	   pit_config_t timerConfig = {
	     .enableRunInDebug = true,
	   };
	   PIT_Init(PIT, &timerConfig);
	   EnableIRQ(PIT_CH0_IRQn);
	   PIT_EnableInterrupts(PIT, kPIT_Chnl_0, kPIT_TimerInterruptEnable);
	   PIT_SetTimerPeriod(PIT, kPIT_Chnl_0, (uint32_t) MSEC_TO_COUNT(1, CLOCK_GetBusClkFreq()));
	   PIT_StartTimer(PIT, kPIT_Chnl_0);
	 }
	 static void wait_ms(uint32_t ms) {
		while(ms) {
			g_ticked = 0;
			while(!g_ticked);
			--ms;
		}
	 }
	 static void wait_s(uint32_t s) {
		wait_ms(s * 1000);
	 }
	 static uint32_t millis() {
	 	return g_ticks;
	 }

 };

#ifdef MAIN_INCLUDE
 volatile uint32_t CDelay::g_ticks = 0;
 volatile bool CDelay::g_ticked = 0;
 extern "C" {
	 void PIT_CH0_IRQHandler(void) {
		  PIT_ClearStatusFlags(PIT, kPIT_Chnl_0, kPIT_TimerFlag);
		  CDelay::g_ticked = 1;
		 ++CDelay::g_ticks;
	 }
 }
#endif

#endif /* DELAYS_H_ */
