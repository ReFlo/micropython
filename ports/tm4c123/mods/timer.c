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

#include "py/mphal.h"
#include "mperrno.h"
#include "bufhelper.h"
#include "inc/hw_memmap.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/udma.h"
#include "inc/hw_udma.h"

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

/**
 * constructor for TIMER!!!!!!! object
 */
STATIC void machine_timer_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    
    mp_printf(print, "Laft");
}




mp_obj_t machine_timer_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {

    // check arguments
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);


    // create dynamic peripheral object
    machine_timer_obj_t *self;
    self=  m_new0(machine_timer_obj_t, 1);
    // mp_print_str("done");

    return MP_OBJ_FROM_PTR(self);
}

STATIC const mp_rom_map_elem_t machine_timer_locals_dict_table[] = {
    // instance methods
    // { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_Timer) }
    };

STATIC MP_DEFINE_CONST_DICT(machine_timer_locals_dict, machine_timer_locals_dict_table);

const mp_obj_type_t machine_timer_type = {
    { &mp_type_type },
    .name = MP_QSTR_TIMER,
    .print = machine_timer_print,
    .make_new = machine_timer_make_new,
    // .protocol = &machine_hard_spi_p,
    .locals_dict = (mp_obj_t)&machine_timer_locals_dict,
    };