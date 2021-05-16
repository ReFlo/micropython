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
#include "py/obj.h"
#include "driverlib/timer.h"
#include "inc/hw_memmap.h"
#include "py/mphal.h"
#include "handlers.h"
#include "py/objstr.h"
#include "misc/mpirq.h"

typedef unsigned char byte;
typedef unsigned int uint;

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

/******************************************************************************
 DECLARE PRIVATE CONSTANTS
 ******************************************************************************/
#define PYBTIMER_NUM_TIMERS                         (6)
#define PYBTIMER_POLARITY_POS                       (0x01)
#define PYBTIMER_POLARITY_NEG                       (0x02)

#define PYBTIMER_TIMEOUT_TRIGGER                    (0x01)
#define PYBTIMER_MATCH_TRIGGER                      (0x02)

#define PYBTIMER_SRC_FREQ_HZ                        HAL_FCPU_HZ

/******************************************************************************
 DEFINE PRIVATE TYPES
 ******************************************************************************/
typedef struct _pyb_timer_obj_t {
    mp_obj_base_t base;
    uint32_t timer;
    uint32_t config;
    uint16_t irq_trigger;
    uint16_t irq_flags;
    uint8_t peripheral;
    uint8_t id;
} pyb_timer_obj_t;

typedef struct _pyb_timer_channel_obj_t {
    mp_obj_base_t base;
    struct _pyb_timer_obj_t *timer;
    uint32_t frequency;
    uint32_t period;
    uint16_t channel;
    uint16_t duty_cycle;
    uint8_t  polarity;
} pyb_timer_channel_obj_t;



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
    TimerIntRegister(self->timer_base, TIMER_A, TIMER0A_IRQHandler);    // Registering  isr       
    TimerEnable(self->timer_base, TIMER_A); 
    IntEnable(self->irqn); 
    // TimerIntEnable(self->timer_base, TIMER_TIMA_TIMEOUT);  
}

STATIC mp_obj_t machine_timer_callback(mp_obj_t self_in, mp_obj_t callback) {
machine_timer_obj_t *self = self_in;
    if (callback == mp_const_none) {
        // stop interrupt (but not timer)
        MAP_TimerIntDisable(TIMER0_BASE,TIMER_TIMA_TIMEOUT);
        self->callback = mp_const_none;
    } else if (mp_obj_is_callable(callback)) {
        MP_STATE_PORT(test_callback_obj)=callback;
        self->callback = MP_STATE_PORT(test_callback_obj);
        MAP_TimerIntEnable(TIMER0_BASE,TIMER_TIMA_TIMEOUT);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(timer_callback_obj, machine_timer_callback);


void timer_irq_handler(uint tim_id){
machine_timer_obj_t *self= MP_STATE_PORT(machine_timer_obj_all)[tim_id];
if(self->timer_id == TIMER_0){
        TimerIntClear(TIMER0_BASE,TIMER_TIMA_TIMEOUT);
     }
    else if(self->timer_id == TIMER_1){
        TimerIntClear(TIMER1_BASE,TIMER_TIMA_TIMEOUT);
     }
    else if(self->timer_id == TIMER_2){
        TimerIntClear(TIMER2_BASE,TIMER_TIMA_TIMEOUT);
     }
    else if(self->timer_id == TIMER_3){
        TimerIntClear(TIMER3_BASE,TIMER_TIMA_TIMEOUT);
     }
    else if(self->timer_id == TIMER_4){
        TimerIntClear(TIMER4_BASE,TIMER_TIMA_TIMEOUT);
     }
    else if(self->timer_id == TIMER_5){
        TimerIntClear(TIMER5_BASE,TIMER_TIMA_TIMEOUT);
     }


    if(self->timer_id){
        mp_hal_stdout_tx_str("Timer Works!\r\n"); 
        // mp_sched_lock();
        // gc_lock();
        // nlr_buf_t nlr;
        // if (nlr_push(&nlr) == 0) {

            

            if(self->callback != mp_const_none){
            // mp_obj_t callback = self->callback;
            gc_lock();
            nlr_buf_t nlr;
            if (nlr_push(&nlr) == 0) {
                mp_call_function_1(MP_STATE_PORT(test_callback_obj), self);
                nlr_pop();}
            }
        }
        // mp_call_function_1(self->callback, MP_OBJ_FROM_PTR(self));
        // self->callback;
    
    
}



// Init Helper for creation of new Timer
STATIC mp_obj_t machine_timer_init_helper(machine_timer_obj_t *tim, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args){
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode,         MP_ARG_KW_ONLY | MP_ARG_OBJ,  {.u_rom_obj = MP_ROM_QSTR(MP_QSTR_PERIODIC)}},
        { MP_QSTR_width,        MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 16} },
    };

    //parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    
    //--------------------------------test print of args---------------------------------------
    mp_hal_stdout_tx_str("Selected mode: ");
    mp_obj_print(MP_OBJ_FROM_PTR(args[0].u_obj),PRINT_STR); //string has to be printed in that way

    printf("\r\nSelected Timer Width: %d\n",args[1].u_int);
    //----------------------------------------------------------------------------------------- 

    //-------------------------------Check width and mode-------------------------------------
    // check mode
    if ((args[0].u_obj != MP_ROM_QSTR(MP_QSTR_PERIODIC)) && (args[0].u_obj != MP_ROM_QSTR(MP_QSTR_ONESHOT)) &&  (args[0].u_obj != MP_ROM_QSTR(MP_QSTR_PWM))) {
        goto error;
    }
    // check width (more accurate check will follow)
    if ((args[1].u_int != 16) && (args[1].u_int != 32)){
        goto error;
    }

    //----------------------------------------------------------------------------------------- 
    

    //call real timer init from here
    init_timer(tim);
    
    
    return mp_const_none;

error: 
    mp_raise_ValueError(MP_ERROR_TEXT("invalid argument(s) value"));
}

// function that is called to pass arguments to previously created timer e.g. t.init(mode="",freq="")
STATIC mp_obj_t machine_timer_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return machine_timer_init_helper(args[0], n_args - 1, args + 1, kw_args);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_timer_init_obj, 1, machine_timer_init);

// Create new Timer object
STATIC mp_obj_t machine_timer_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {

    // check arguments
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);
    
    //find timer id
    timer_id_t timer_id = Timer_find(all_args[0]);

    // create dynamically new Timer object
    machine_timer_obj_t *self;

    // get Timer object
    if (MP_STATE_PORT(machine_timer_obj_all)[timer_id] == NULL) {

        self =  m_new0(machine_timer_obj_t, 1);
        self->base.type = &machine_timer_type;
        MP_STATE_PORT(machine_timer_obj_all)[timer_id] = self;
    } else {
        // reference existing Timer object
        self = MP_STATE_PORT(machine_timer_obj_all)[timer_id];
    }

    self->timer_id = timer_id;  
    if(n_args > 1 || n_kw > 0)  {
        //dont know if needed
        mp_map_t kw_args;
        mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
        //init helper for checking the input args and setting attributes
        machine_timer_init_helper(self, n_args - 1, all_args + 1, &kw_args);
    }
   

    // //init_helper_timer(self,all_args) bla bla
    // self->callback = (mp_obj_t*)all_args[1];

    // init_timer(self);

    return MP_OBJ_FROM_PTR(self);

}


// #####################################################
// Try function from cc32000
// #####################################################

/******************************************************************************
 DEFINE PRIVATE DATA
 ******************************************************************************/
STATIC const mp_irq_methods_t pyb_timer_channel_irq_methods;
STATIC pyb_timer_obj_t pyb_timer_obj[PYBTIMER_NUM_TIMERS] = {{.timer = TIMER1_BASE},
                                                             {.timer = TIMER2_BASE},
                                                             {.timer = TIMER3_BASE},
                                                             {.timer = TIMER4_BASE}};
STATIC const mp_obj_type_t pyb_timer_channel_type;
/******************************************************************************
/******************************************************************************
 DEFINE PRIVATE FUNCTIONS
 ******************************************************************************/
STATIC void pyb_timer_channel_irq_enable (mp_obj_t self_in) {
    pyb_timer_channel_obj_t *self = self_in;
    MAP_TimerIntClear(self->timer->timer, self->timer->irq_trigger & self->channel);
    MAP_TimerIntEnable(self->timer->timer, self->timer->irq_trigger & self->channel);
}

STATIC void pyb_timer_channel_irq_disable (mp_obj_t self_in) {
    pyb_timer_channel_obj_t *self = self_in;
    MAP_TimerIntDisable(self->timer->timer, self->timer->irq_trigger & self->channel);
}

STATIC int pyb_timer_channel_irq_flags (mp_obj_t self_in) {
    pyb_timer_channel_obj_t *self = self_in;
    return self->timer->irq_flags;
}

STATIC pyb_timer_channel_obj_t *pyb_timer_channel_find (uint32_t timer, uint16_t channel_n) {
    for (mp_uint_t i = 0; i < MP_STATE_PORT(pyb_timer_channel_obj_list).len; i++) {
        pyb_timer_channel_obj_t *ch = ((pyb_timer_channel_obj_t *)(MP_STATE_PORT(pyb_timer_channel_obj_list).items[i]));
        // any 32-bit timer must be matched by any of its 16-bit versions
        if (ch->timer->timer == timer && ((ch->channel & TIMER_A) == channel_n || (ch->channel & TIMER_B) == channel_n)) {
            return ch;
        }
    }
    return MP_OBJ_NULL;
}

STATIC mp_obj_t pyb_timer_channel_irq(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    mp_arg_val_t args[mp_irq_INIT_NUM_ARGS];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, mp_irq_INIT_NUM_ARGS, mp_irq_init_args, args);
    pyb_timer_channel_obj_t *ch = pos_args[0];

    // convert the priority to the correct value
    uint priority = mp_irq_translate_priority (args[1].u_int);

    // // validate the power mode
    // int pwrmode = (args[3].u_obj == mp_const_none) ? PYB_PWR_MODE_ACTIVE : mp_obj_get_int(args[3].u_obj);
    // if (pwrmode != PYB_PWR_MODE_ACTIVE) {
    //     goto invalid_args;
    // }

    // get the trigger
    uint trigger = mp_obj_get_int(args[0].u_obj);

    // disable the callback first
    pyb_timer_channel_irq_disable(ch);

    uint8_t shift = (ch->channel == TIMER_B) ? 8 : 0;
    uint32_t _config = (ch->channel == TIMER_B) ? ((ch->timer->config & TIMER_B) >> 8) : (ch->timer->config & TIMER_A);
    switch (_config) {
    case TIMER_CFG_A_ONE_SHOT_UP:
    case TIMER_CFG_A_PERIODIC_UP:
        ch->timer->irq_trigger |= TIMER_TIMA_TIMEOUT << shift;
        if (trigger != PYBTIMER_TIMEOUT_TRIGGER) {
            goto invalid_args;
        }
        break;
    case TIMER_CFG_A_PWM:
        // special case for the PWM match interrupt
        ch->timer->irq_trigger |= ((ch->channel & TIMER_A) == TIMER_A) ? TIMER_TIMA_MATCH : TIMER_TIMB_MATCH;
        if (trigger != PYBTIMER_MATCH_TRIGGER) {
            goto invalid_args;
        }
        break;
    default:
        break;
    }
    // special case for a 32-bit timer
    if (ch->channel == (TIMER_A | TIMER_B)) {
       ch->timer->irq_trigger |= (ch->timer->irq_trigger << 8);
    }

    void (*pfnHandler)(void);
    uint32_t intregister;
    switch (ch->timer->timer) {
    case TIMER0_BASE:
        if (ch->channel == TIMER_B) {
            pfnHandler = &TIMER0BIntHandler;
            intregister = INT_TIMER0B;
        } else {
            pfnHandler = &TIMER0AIntHandler;
            intregister = INT_TIMER0A;
        }
        break;
    case TIMER1_BASE:
        if (ch->channel == TIMER_B) {
            pfnHandler = &TIMER1BIntHandler;
            intregister = INT_TIMER1B;
        } else {
            pfnHandler = &TIMER1AIntHandler;
            intregister = INT_TIMERA1A;
        }
        break;
    case TIMER2_BASE:
        if (ch->channel == TIMER_B) {
            pfnHandler = &TIMER2BIntHandler;
            intregister = INT_TIMER2B;
        } else {
            pfnHandler = &TIMER2AIntHandler;
            intregister = INT_TIMER2A;
        }
        break;
    case TIMER3_BASE:
    if (ch->channel == TIMER_B) {
        pfnHandler = &TIMER3BIntHandler;
        intregister = INT_TIMER3B;
    } else {
        pfnHandler = &TIMER3AIntHandler;
        intregister = INT_TIMER2A;
    }
    break;
    case TIMER4_BASE:
    if (ch->channel == TIMER_B) {
        pfnHandler = &TIMER4BIntHandler;
        intregister = INT_TIMER4B;
    } else {
        pfnHandler = &TIMER4AIntHandler;
        intregister = INT_TIMER4A;
    }
    break;
    default:
        if (ch->channel == TIMER_B) {
            pfnHandler = &TIMER5BIntHandler;
            intregister = INT_TIMER5B;
        } else {
            pfnHandler = &TIMER3AIntHandler;
            intregister = INT_TIMERA3A;
        }
        break;
    }

    // register the interrupt and configure the priority
    MAP_IntPrioritySet(intregister, priority);
    MAP_TimerIntRegister(ch->timer->timer, ch->channel, pfnHandler);

    // create the callback
    mp_obj_t _irq = mp_irq_new (ch, args[2].u_obj, &pyb_timer_channel_irq_methods);

    // enable the callback before returning
    pyb_timer_channel_irq_enable(ch);

    return _irq;

invalid_args:
    mp_raise_ValueError(MP_ERROR_TEXT("invalid argument(s) value"));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_timer_channel_irq_obj, 1, pyb_timer_channel_irq);

STATIC const mp_rom_map_elem_t machine_timer_locals_dict_table[] = {
      { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_timer_init_obj)},
      { MP_ROM_QSTR(MP_QSTR_info), MP_ROM_PTR(&subsystem_info_obj) },
      { MP_ROM_QSTR(MP_QSTR_print), MP_ROM_PTR(&machine_timer_print_obj) },
      { MP_ROM_QSTR(MP_QSTR_callback), MP_ROM_PTR(&timer_callback_obj) },
      { MP_ROM_QSTR(MP_QSTR_irq), MP_ROM_PTR(&pyb_timer_channel_irq_obj) },
      };

STATIC MP_DEFINE_CONST_DICT(machine_timer_locals_dict, machine_timer_locals_dict_table);

const mp_obj_type_t machine_timer_type = {
    { &mp_type_type },
    .name = MP_QSTR_Timer,
    // .print = machine_timer_print,
    .make_new = machine_timer_make_new,
    .locals_dict = (mp_obj_t)&machine_timer_locals_dict,
    };
