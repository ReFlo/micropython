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

/**
 *  Timer register struct for easy access 
 *  for register description, see datasheet
*/
typedef struct  {
    volatile uint32_t CFG;
    volatile uint32_t TAMR;
    volatile uint32_t TBMR;
    volatile uint32_t CTL;
    volatile uint32_t SYNC;
    uint32_t _1[1];
    volatile uint32_t IMR;
    volatile uint32_t RIS;
    volatile uint32_t MIS;
    volatile uint32_t ICR;
    volatile uint32_t TAILR;
    volatile uint32_t TBILR;
    volatile uint32_t TAMATCHR;
    volatile uint32_t TBMATCHR;
    volatile uint32_t TAPR;
    volatile uint32_t TBPR;
    volatile uint32_t TAPMR;
    volatile uint32_t TBPMR;
    volatile uint32_t TAR;
    volatile uint32_t TBR;
    volatile uint32_t TAV;
    volatile uint32_t TBV;
    volatile uint32_t RTCPD;
    volatile uint32_t TAPS;
    volatile uint32_t TBPS;
    volatile uint32_t TAPV;
    volatile uint32_t TBPV;
    uint32_t _2[981]; 
    volatile uint32_t MPP;
} periph_timer_t;

//extern TIM_HandleTypeDef TIM5_Handle;

extern const mp_obj_type_t pyb_timer_type;

//void timer_init0(void);
//void timer_tim5_init(void);
//TIM_HandleTypeDef *timer_tim6_init(uint freq);
//void timer_deinit(void);
//uint32_t timer_get_source_freq(uint32_t tim_id);
//void timer_irq_handler(uint tim_id);

//TIM_HandleTypeDef *pyb_timer_get_handle(mp_obj_t timer);

#endif // MICROPY_INCLUDED_TM4C_TIMER_H
