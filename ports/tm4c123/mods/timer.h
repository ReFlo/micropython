/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2019 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_TM4C_TIMER_H
#define MICROPY_INCLUDED_TM4C_TIMER_H

#include <stdint.h>
#include "driverlib/timer.h"
#include "inc/hw_timer.h"


/*
Timer ID Enum
*/
// typedef enum {
//     TIMER_0,
//     TIMER_1,
//     TIMER_2,
//     TIMER_3,
//     TIMER_4,
//     TIMER_5,
//     W_TIMER_0,
//     W_TIMER_1,
//     W_TIMER_2,
//     W_TIMER_3,
//     W_TIMER_4,
//     W_TIMER_5,
//     TIMER_NONE
// } timer_id_t;


/*
Timer Mode Enum
*/

// typedef enum{
//     ONE_SHOT,
//     PERIODIC,
//     RTC,
//     IECM,           // Input Edge Count Mode
//     IETM,           // Input Edge Time Mode
//     PWM,            // Pulse With Modulation
// }timer_mode_t;


/**
 *  Timer register struct for easy access 
 *  for register description, see datasheet
*/

// typedef struct  {
//     volatile uint32_t CFG;
//     volatile uint32_t TAMR;
//     volatile uint32_t TBMR;
//     volatile uint32_t CTL;
//     volatile uint32_t SYNC;
//     uint32_t _1[1];
//     volatile uint32_t IMR;
//     volatile uint32_t RIS;
//     volatile uint32_t MIS;
//     volatile uint32_t ICR;
//     volatile uint32_t TAILR;
//     volatile uint32_t TBILR;
//     volatile uint32_t TAMATCHR;
//     volatile uint32_t TBMATCHR;
//     volatile uint32_t TAPR;
//     volatile uint32_t TBPR;
//     volatile uint32_t TAPMR;
//     volatile uint32_t TBPMR;
//     volatile uint32_t TAR;
//     volatile uint32_t TBR;
//     volatile uint32_t TAV;
//     volatile uint32_t TBV;
//     volatile uint32_t RTCPD;
//     volatile uint32_t TAPS;
//     volatile uint32_t TBPS;
//     volatile uint32_t TAPV;
//     volatile uint32_t TBPV;
//     uint32_t _2[981]; 
//     volatile uint32_t MPP;
// } periph_timer_t;


// typedef struct _machine_timer_obj_t {
//     mp_obj_base_t base;
//     uint32_t timer_base;        // base address of timer module
//     uint32_t periph;            // address needed for tivaware sysctl functions
//     periph_timer_t* regs;       // register access struct pointer (usage: timer_obj.regs->DR)
//     timer_id_t timer_id;        // Timer Id 0..5, for identification purposes
//     uint32_t irqn;              // Interrupt request number
//     uint8_t width;              // Timer with 16/32 or 32/64 Bit Mode
//     uint32_t prescale;          // Timer Prescaler value
//     uint32_t frequency;         // Frequency of Timer
//     mp_obj_t callback;          // callback function
// } machine_timer_obj_t;




//extern TIM_HandleTypeDef TIM5_Handle;

extern const mp_obj_type_t machine_timer_type;
// void timer_irq_handler(uint tim_id);
void TIMERGenericIntHandler(uint32_t timer, uint16_t channel);
/******************************************************************************
 DECLARE EXPORTED DATA
 ******************************************************************************/


/******************************************************************************
 DECLARE PUBLIC FUNCTIONS
 ******************************************************************************/
void timer_init0 (void);

#endif // MICROPY_INCLUDED_TM4C_TIMER_H
