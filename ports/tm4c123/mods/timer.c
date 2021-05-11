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

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "py/runtime.h"
#include "py/gc.h"
#include "timer.h"
#include "pin.h"
#include "driverlib/timer.h"
#include "inc/hw_memmap.h"
#include "py/mphal.h"
#include "handlers.h"


/// \moduleref pyb
/// \class Timer - periodically call a function
///
/// Timers can be used for a great variety of tasks.  At the moment, only
/// the simplest case is implemented: that of calling a function periodically.
///
/// Each timer consists of a counter that counts up at a certain rate.  The rate
/// at which it counts is the peripheral clock frequency (in Hz) divided by the
/// timer prescaler.  When the counter reaches the timer period it triggers an
/// event, and the counter resets back to zero.  By using the callback method,
/// the timer event can call a Python function.
///
/// Example usage to toggle an LED at a fixed frequency:
///
///     tim = pyb.Timer(4)              # create a timer object using timer 4
///     tim.init(freq=2)                # trigger at 2Hz
///     tim.callback(lambda t:pyb.LED(1).toggle())
///
/// Further examples:
///
///     tim = pyb.Timer(4, freq=100)    # freq in Hz
///     tim = pyb.Timer(4, prescaler=0, period=99)
///     tim.counter()                   # get counter (can also set)
///     tim.prescaler(2)                # set prescaler (can also get)
///     tim.period(199)                 # set period (can also get)
///     tim.callback(lambda t: ...)     # set callback for update interrupt (t=tim instance)
///     tim.callback(None)              # clear callback
///

// The timers can be used by multiple drivers, and need a common point for
// the interrupts to be dispatched, so they are all collected here.
//



// STATIC mp_obj_t timer_init(const mp_obj_t *self_in) {
//     machine_timer_obj_t *self = (machine_timer_obj_t*) self_in;
    
//     string a ="lafft";

//     }


void print_test(){
    mp_hal_stdout_tx_str("Callback Works!\r\n"); 
}

STATIC int Timer_find(mp_obj_t id) {
    if (MP_OBJ_IS_STR(id)) {
        // given a string id
        const char *port = mp_obj_str_get_str(id);
        if (0) {
        #ifdef MICROPY_HW_TIMER0_NAME
        } else if (strcmp(port, MICROPY_HW_TIMER0_NAME) == 0) {
            return TIMER_0;
        #endif
        #ifdef MICROPY_HW_TIMER1_NAME
        } else if (strcmp(port, MICROPY_HW_TIMER1_NAME) == 0) {
            return TIMER_1;
        #endif
        #ifdef MICROPY_HW_TIMER2_NAME
        } else if (strcmp(port, MICROPY_HW_TIMER2_NAME) == 0) {
            return TIMER_2;
        #endif
        #ifdef MICROPY_HW_TIMER3_NAME
        } else if (strcmp(port, MICROPY_HW_TIMER3_NAME) == 0) {
            return TIMER_3;
        #endif
        #ifdef MICROPY_HW_TIMER4_NAME
        } else if (strcmp(port, MICROPY_HW_TIMER4_NAME) == 0) {
            return TIMER_4;
        #endif
        #ifdef MICROPY_HW_TIMER5_NAME
        } else if (strcmp(port, MICROPY_HW_TIMER5_NAME) == 0) {
            return TIMER_5;
        #endif
        }
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("Timer(%s) doesn't exist"), port));
    } else {
        // given an integer id
        int timer_id = mp_obj_get_int(id);
        if (timer_id >= 0 && timer_id <= MP_ARRAY_SIZE(MP_STATE_PORT(machine_timer_obj_all))) {
            return timer_id;
        }
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("Timer(%d) doesn't exist"), timer_id));
    }
}

STATIC mp_obj_t machine_timer_print(mp_obj_t self_in) 
{
    mp_hal_stdout_tx_strn("lafft\n\r", 8);
    // return MP_OBJ_NEW_SMALL_INT(42);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(machine_timer_print_obj, machine_timer_print);


// Beispiel um Funktionen direkt fürs Paket zu hinterlegen
STATIC mp_obj_t py_subsystem_info(void) {
    return MP_OBJ_NEW_SMALL_INT(42);
}
MP_DEFINE_CONST_FUN_OBJ_0(subsystem_info_obj, py_subsystem_info);


STATIC void init_timer(mp_obj_t self_in){
     machine_timer_obj_t *self = (machine_timer_obj_t*) self_in;
     
     //structure for checking which timer should be initialized
     //bis jetzt nur TIMER A
     if(self->timer_id == TIMER_0){
         self->timer_base = TIMER0_BASE;
         self->periph = SYSCTL_PERIPH_TIMER0;
         self->regs = (periph_timer_t*)TIMER0_BASE;
         self->irqn = INT_TIMER0A;
     }
    else if(self->timer_id == TIMER_1){
         self->timer_base = TIMER1_BASE;
         self->periph = SYSCTL_PERIPH_TIMER1;
         self->regs = (periph_timer_t*)TIMER1_BASE;
         self->irqn = INT_TIMER1A;
     }
    else if(self->timer_id == TIMER_2){
         self->timer_base = TIMER2_BASE;
         self->periph = SYSCTL_PERIPH_TIMER2;
         self->regs = (periph_timer_t*)TIMER2_BASE;
         self->irqn = INT_TIMER2A;
     }
    else if(self->timer_id == TIMER_3){
         self->timer_base = TIMER3_BASE;
         self->periph = SYSCTL_PERIPH_TIMER3;
         self->regs = (periph_timer_t*)TIMER3_BASE;
         self->irqn = INT_TIMER3A;
     }
    else if(self->timer_id == TIMER_4){
         self->timer_base = TIMER4_BASE;
         self->periph = SYSCTL_PERIPH_TIMER4;
         self->regs = (periph_timer_t*)TIMER4_BASE;
         self->irqn = INT_TIMER4A;
     }
    else if(self->timer_id == TIMER_5){
         self->timer_base = TIMER5_BASE;
         self->periph = SYSCTL_PERIPH_TIMER5;
         self->regs = (periph_timer_t*)TIMER5_BASE;
         self->irqn = INT_TIMER5A;
     }

    SysCtlPeripheralEnable(self->periph);
    while(!SysCtlPeripheralReady(self->periph));
    TimerDisable(self->timer_base,TIMER_A);
    TimerConfigure(self->timer_base, TIMER_CFG_PERIODIC);   // 32 bits Timer
    TimerLoadSet(self->timer_base, TIMER_A, 4e+7);
    TimerIntRegister(self->timer_base, TIMER_A, TIM1_IRQHandler);    // Registering  isr       
    TimerEnable(self->timer_base, TIMER_A); 
    IntEnable(self->irqn); 
    TimerIntEnable(self->timer_base, TIMER_TIMA_TIMEOUT);  
    self->callback = &print_test;
}

void timer_irq_handler(uint tim_id){
     machine_timer_obj_t *self= MP_STATE_PORT(machine_timer_obj_all)[tim_id - 1];
// if(self->timer_id == TIMER_0){
//         TimerIntClear(TIMER0_BASE,TIMER_TIMA_TIMEOUT);
//      }
//     else if(self->timer_id == TIMER_1){
//         TimerIntClear(TIMER1_BASE,TIMER_TIMA_TIMEOUT);
//      }
//     else if(self->timer_id == TIMER_2){
//         TimerIntClear(TIMER2_BASE,TIMER_TIMA_TIMEOUT);
//      }
//     else if(self->timer_id == TIMER_3){
//         TimerIntClear(TIMER3_BASE,TIMER_TIMA_TIMEOUT);
//      }
//     else if(self->timer_id == TIMER_4){
//         TimerIntClear(TIMER4_BASE,TIMER_TIMA_TIMEOUT);
//      }
//     else if(self->timer_id == TIMER_5){
//         TimerIntClear(TIMER5_BASE,TIMER_TIMA_TIMEOUT);
//      }

    TimerIntClear(TIMER0_BASE,TIMER_TIMA_TIMEOUT);
    if(self->timer_id){
        mp_hal_stdout_tx_str("Timer Works!\r\n"); 
        // mp_call_function_1(self->callback, MP_OBJ_FROM_PTR(self));
        self->callback;
    }

}


// Create new Timer object
mp_obj_t machine_timer_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {

    // check arguments
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);
    // create dynamically new Timer object
    machine_timer_obj_t *self;

    timer_id_t timer_id = Timer_find(all_args[0]);
    // get Timer object
    if (MP_STATE_PORT(machine_timer_obj_all)[timer_id - 1] == NULL) {

        self =  m_new0(machine_timer_obj_t, 1);
        self->base.type = &machine_timer_type;
        MP_STATE_PORT(machine_timer_obj_all)[timer_id - 1] = self;
    } else {
        // reference existing Timer object
        self = MP_STATE_PORT(machine_timer_obj_all)[timer_id - 1];
    }

    // self->timer_id = timer_id;

    //printing test code
    mp_obj_print(MP_OBJ_FROM_PTR(all_args[1]), PRINT_STR);

    //init helper for checking the input args needed
    //init_helper_timer(self,all_args) bla bla

    init_timer(self);

    return MP_OBJ_FROM_PTR(self);

}

STATIC const mp_rom_map_elem_t machine_timer_locals_dict_table[] = {
      { MP_ROM_QSTR(MP_QSTR_info), MP_ROM_PTR(&subsystem_info_obj) },
      { MP_ROM_QSTR(MP_QSTR_print), MP_ROM_PTR(&machine_timer_print_obj) },
    };

STATIC MP_DEFINE_CONST_DICT(machine_timer_locals_dict, machine_timer_locals_dict_table);

const mp_obj_type_t machine_timer_type = {
    { &mp_type_type },
    .name = MP_QSTR_Timer,
    // .print = machine_timer_print,
    .make_new = machine_timer_make_new,
    .locals_dict = (mp_obj_t)&machine_timer_locals_dict,
    };
