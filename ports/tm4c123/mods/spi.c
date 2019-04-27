/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
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

#include <stdio.h>
#include <string.h>

#include "py/runtime.h"
#include "py/mphal.h"
#include "extmod/machine_spi.h"
#include "irq.h"
#include "pin.h"
#include "bufhelper.h"
#include "spi.h"

#include "driverlib/ssi.h"
#include "inc/hw_ssi.h"
#include "inc/hw_memmap.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "inc/hw_udma.h"

/// \moduleref pyb
/// \class SPI - a master-driven serial protocol
///
/// SPI is a serial protocol that is driven by a master.  At the physical level
/// there are 3 lines: SCK, MOSI, MISO.
///
/// See usage model of I2C; SPI is very similar.  Main difference is
/// parameters to init the SPI bus:
///
///     from pyb import SPI
///     spi = SPI(1, SPI.MASTER, baudrate=600000, polarity=1, phase=0, crc=0x7)
///
/// Only required parameter is mode, SPI.MASTER or SPI.SLAVE.  Polarity can be
/// 0 or 1, and is the level the idle clock line sits at.  Phase can be 0 or 1
/// to sample data on the first or second clock edge respectively.  Crc can be
/// None for no CRC, or a polynomial specifier.
///
/// Additional method for SPI:
///
///     data = spi.send_recv(b'1234')        # send 4 bytes and receive 4 bytes
///     buf = bytearray(4)
///     spi.send_recv(b'1234', buf)          # send 4 bytes and receive 4 into buf
///     spi.send_recv(buf, buf)              # send/recv 4 bytes from/to buf

// Possible DMA configurations for SPI busses:
// SPI1_TX: DMA2_Stream3.CHANNEL_3 or DMA2_Stream5.CHANNEL_3
// SPI1_RX: DMA2_Stream0.CHANNEL_3 or DMA2_Stream2.CHANNEL_3
// SPI2_TX: DMA1_Stream4.CHANNEL_0
// SPI2_RX: DMA1_Stream3.CHANNEL_0
// SPI3_TX: DMA1_Stream5.CHANNEL_0 or DMA1_Stream7.CHANNEL_0
// SPI3_RX: DMA1_Stream0.CHANNEL_0 or DMA1_Stream2.CHANNEL_0
// SPI4_TX: DMA2_Stream4.CHANNEL_5 or DMA2_Stream1.CHANNEL_4
// SPI4_RX: DMA2_Stream3.CHANNEL_5 or DMA2_Stream0.CHANNEL_4
// SPI5_TX: DMA2_Stream4.CHANNEL_2 or DMA2_Stream6.CHANNEL_7
// SPI5_RX: DMA2_Stream3.CHANNEL_2 or DMA2_Stream5.CHANNEL_7
// SPI6_TX: DMA2_Stream5.CHANNEL_1
// SPI6_RX: DMA2_Stream6.CHANNEL_1

// #if defined(MICROPY_HW_SPI1_SCK)
// SPI_HandleTypeDef SPIHandle1 = {.Instance = NULL};
// #endif
// #if defined(MICROPY_HW_SPI2_SCK)
// SPI_HandleTypeDef SPIHandle2 = {.Instance = NULL};
// #endif
// #if defined(MICROPY_HW_SPI3_SCK)
// SPI_HandleTypeDef SPIHandle3 = {.Instance = NULL};
// #endif
// #if defined(MICROPY_HW_SPI4_SCK)
// SPI_HandleTypeDef SPIHandle4 = {.Instance = NULL};
// #endif
// #if defined(MICROPY_HW_SPI5_SCK)
// SPI_HandleTypeDef SPIHandle5 = {.Instance = NULL};
// #endif
// #if defined(MICROPY_HW_SPI6_SCK)
// SPI_HandleTypeDef SPIHandle6 = {.Instance = NULL};
// #endif

void spi_init0(void) {
    // Initialise the SPI handles.
    // The structs live on the BSS so all other fields will be zero after a reset.
    // #if defined(MICROPY_HW_SPI1_SCK)
    // SPIHandle1.Instance = SPI1;
    // #endif
    // #if defined(MICROPY_HW_SPI2_SCK)
    // SPIHandle2.Instance = SPI2;
    // #endif
    // #if defined(MICROPY_HW_SPI3_SCK)
    // SPIHandle3.Instance = SPI3;
    // #endif
    // #if defined(MICROPY_HW_SPI4_SCK)
    // SPIHandle4.Instance = SPI4;
    // #endif
    // #if defined(MICROPY_HW_SPI5_SCK)
    // SPIHandle5.Instance = SPI5;
    // #endif
    // #if defined(MICROPY_HW_SPI6_SCK)
    // SPIHandle6.Instance = SPI6;
    // #endif
}

STATIC int spi_find(mp_obj_t id) {
    if (MP_OBJ_IS_STR(id)) {
        // given a string id
        const char *port = mp_obj_str_get_str(id);
        if (0) {
        #ifdef MICROPY_HW_SPI1_NAME
        } else if (strcmp(port, MICROPY_HW_SPI0_NAME) == 0) {
            return 0;
        #endif
        #ifdef MICROPY_HW_SPI1_NAME
        } else if (strcmp(port, MICROPY_HW_SPI1_NAME) == 0) {
            return 1;
        #endif
        #ifdef MICROPY_HW_SPI2_NAME
        } else if (strcmp(port, MICROPY_HW_SPI2_NAME) == 0) {
            return 2;
        #endif
        #ifdef MICROPY_HW_SPI3_NAME
        } else if (strcmp(port, MICROPY_HW_SPI3_NAME) == 0) {
            return 3;
        #endif
        #ifdef MICROPY_HW_SPI4_NAME
        } else if (strcmp(port, MICROPY_HW_SPI4_NAME) == 0) {
            return 4;
        #endif
        #ifdef MICROPY_HW_SPI5_NAME
        } else if (strcmp(port, MICROPY_HW_SPI5_NAME) == 0) {
            return 5;
        #endif
        #ifdef MICROPY_HW_SPI6_NAME
        } else if (strcmp(port, MICROPY_HW_SPI6_NAME) == 0) {
            return 6;
        #endif
        }
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError,
            "SPI(%s) doesn't exist", port));
    } else {
        // given an integer id
        int spi_id = mp_obj_get_int(id);
        if (spi_id >= 1 && spi_id <= MP_ARRAY_SIZE(spi_obj)
            && spi_obj[spi_id - 1].spi != NULL) {
            return spi_id;
        }
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError,
            "SPI(%d) doesn't exist", spi_id));
    }
}

// sets the parameters in the SPI_InitTypeDef struct
// if an argument is -1 then the corresponding parameter is not changed
STATIC void spi_set_params(mp_obj_t *spi_obj, uint8_t prescale, int32_t baudrate,
    int32_t polarity, int32_t phase, int32_t bits, int32_t firstbit) {
    machine_hard_spi_obj_t *self = spi_obj;

    if (prescale != 0xff || baudrate != -1) {
        if (prescale == 0xff) {
            // prescaler not given, so select one that yields at most the requested baudrate
            mp_uint_t spi_clock = MAP_SysCtlClockGet();
            prescale = (spi_clock / baudrate);
        }
        prescale &= 0xFE;
    }

    if (polarity != -1) {
        init->CLKPolarity = polarity == 0 ? SPI_POLARITY_LOW : SPI_POLARITY_HIGH;
    }

    if (phase != -1) {
        init->CLKPhase = phase == 0 ? SPI_PHASE_1EDGE : SPI_PHASE_2EDGE;
    }

    if (bits != -1) {
        init->DataSize = (bits == 16) ? SPI_DATASIZE_16BIT : SPI_DATASIZE_8BIT;
    }

    if (firstbit != -1) {
        init->FirstBit = firstbit;
    }
}

// TODO allow to take a list of pins to use
void spi_init(const machine_hard_spi_obj_t *self_in) {
    machine_hard_spi_obj_t *self = self_in;
    const pin_obj_t *pins[4] = { NULL, NULL, NULL, NULL };

    if (0) {
    #if defined(MICROPY_HW_SPI0_SCK)
    } else if (self->spi_id == PYB_SPI_0) {
        self->spi_base = SSI0_BASE;
        self->periph = SYSCTL_PERIPH_SSI0;
        self->regs = (periph_spi_t*)SSI0_BASE;
        self->irqn = INT_SSI0;
        pins[0] = MICROPY_HW_SPI0_SCK;
        #if defined(MICROPY_HW_SPI0_MISO)
        pins[1] = MICROPY_HW_SPI0_MISO;
        #endif
        #if defined(MICROPY_HW_SPI0_MOSI)
        pins[2] = MICROPY_HW_SPI0_MOSI;
        #endif
        #if defined(MICROPY_HW_SPI0_FSS)
        if(!self->soft_fss) {
            pins[3] = MICROPY_HW_SPI0_FSS;
        }
        #endif
    #endif
    #if defined(MICROPY_HW_SPI1_SCK)
    } else if (self->spi_id == PYB_SPI_1) {
        self->spi_base = SSI1_BASE;
        self->periph = SYSCTL_PERIPH_SSI1;
        self->regs = (periph_spi_t*)SSI1_BASE;
        self->irqn = INT_SSI1;
        pins[0] = MICROPY_HW_SPI1_SCK;
        #if defined(MICROPY_HW_SPI1_MISO)
        pins[1] = MICROPY_HW_SPI1_MISO;
        #endif
        #if defined(MICROPY_HW_SPI1_MOSI)
        pins[2] = MICROPY_HW_SPI1_MOSI;
        #endif
        #if defined(MICROPY_HW_SPI1_FSS)
        if(!self->soft_fss) {
            pins[3] = MICROPY_HW_SPI1_FSS;
        }
        #endif
    #endif
    #if defined(MICROPY_HW_SPI2_SCK)
    } else if (self->spi_id == PYB_SPI_2) {
        self->spi_base = SSI2_BASE;
        self->periph = SYSCTL_PERIPH_SSI2;
        self->regs = (periph_spi_t*)SSI2_BASE;
        self->irqn = INT_SSI2;
        pins[0] = MICROPY_HW_SPI2_SCK;
        #if defined(MICROPY_HW_SPI2_MISO)
        pins[1] = MICROPY_HW_SPI2_MISO;
        #endif
        #if defined(MICROPY_HW_SPI2_MOSI)
        pins[2] = MICROPY_HW_SPI2_MOSI;
        #endif
        #if defined(MICROPY_HW_SPI2_FSS)
        if(!self->soft_fss) {
            pins[3] = MICROPY_HW_SPI2_FSS;
        }
        #endif
    #endif
    #if defined(MICROPY_HW_SPI3_SCK)
    } else if (self->spi_id == PYB_SPI_3) {
        self->spi_base = SSI3_BASE;
        self->periph = SYSCTL_PERIPH_SSI3;
        self->regs = (periph_spi_t*)SSI3_BASE;
        self->irqn = INT_SSI3;
        pins[0] = MICROPY_HW_SPI3_SCK;
        #if defined(MICROPY_HW_SPI3_MISO)
        pins[1] = MICROPY_HW_SPI3_MISO;
        #endif
        #if defined(MICROPY_HW_SPI3_MOSI)
        pins[2] = MICROPY_HW_SPI3_MOSI;
        #endif
        #if defined(MICROPY_HW_SPI3_FSS)
        if(!self->soft_fss) {
            pins[3] = MICROPY_HW_SPI3_FSS;
        }
        #endif
    #endif
    } else {
        // SPI does not exist for this board (shouldn't get here, should be checked by caller)
        return;
    }

    // init the GPIO lines
    for (uint i = 0; i < (self->soft_fss ? 3 : 4); i++) {
        if (pins[i] == NULL) {
            continue;
        }
        mp_hal_pin_config_alt(pins[i], PIN_FN_SSI, self->spi_id);
        // idle_high_polarity
        if(i==0 && (self->protocol & 0x1) ) MAP_GPIOPadConfigSet(pins[i]->gpio, pins[i]->pin_mask, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
    }

    // init the SPI device

    SysCtlPeripheralEnable(self->periph);
    while(!SysCtlPeripheralReady(self->periph));
    IntDisable(self->irqn);
    SSIDisable(self->spi_base);
    
    SSIClockSourceSet(self->spi_base, SSI_CLOCK_SYSTEM);
    SSIConfigSetExpClk(self->spi_base, MAP_SysCtlClockGet(), self->protocol, self->mode, self->baudrate, self->bits);
    SSIDMADisable(self->spi_base, SSI_DMA_TX | SSI_DMA_RX);
    SSIDMAEnable(self->spi_base, self->dma_enabled);

    SSIEnable(self->spi_base);
}

void spi_deinit(const spi_t *spi_obj) {
    SPI_HandleTypeDef *spi = spi_obj->spi;
    HAL_SPI_DeInit(spi);
    if (0) {
    #if defined(MICROPY_HW_SPI1_SCK)
    } else if (spi->Instance == SPI1) {
        __HAL_RCC_SPI1_FORCE_RESET();
        __HAL_RCC_SPI1_RELEASE_RESET();
        __HAL_RCC_SPI1_CLK_DISABLE();
    #endif
    #if defined(MICROPY_HW_SPI2_SCK)
    } else if (spi->Instance == SPI2) {
        __HAL_RCC_SPI2_FORCE_RESET();
        __HAL_RCC_SPI2_RELEASE_RESET();
        __HAL_RCC_SPI2_CLK_DISABLE();
    #endif
    #if defined(MICROPY_HW_SPI3_SCK)
    } else if (spi->Instance == SPI3) {
        __HAL_RCC_SPI3_FORCE_RESET();
        __HAL_RCC_SPI3_RELEASE_RESET();
        __HAL_RCC_SPI3_CLK_DISABLE();
    #endif
    #if defined(MICROPY_HW_SPI4_SCK)
    } else if (spi->Instance == SPI4) {
        __HAL_RCC_SPI4_FORCE_RESET();
        __HAL_RCC_SPI4_RELEASE_RESET();
        __HAL_RCC_SPI4_CLK_DISABLE();
    #endif
    #if defined(MICROPY_HW_SPI5_SCK)
    } else if (spi->Instance == SPI5) {
        __HAL_RCC_SPI5_FORCE_RESET();
        __HAL_RCC_SPI5_RELEASE_RESET();
        __HAL_RCC_SPI5_CLK_DISABLE();
    #endif
    #if defined(MICROPY_HW_SPI6_SCK)
    } else if (spi->Instance == SPI6) {
        __HAL_RCC_SPI6_FORCE_RESET();
        __HAL_RCC_SPI6_RELEASE_RESET();
        __HAL_RCC_SPI6_CLK_DISABLE();
    #endif
    }
}

STATIC HAL_StatusTypeDef spi_wait_dma_finished(const spi_t *spi, uint32_t timeout) {
    uint32_t start = HAL_GetTick();
    volatile HAL_SPI_StateTypeDef *state = &spi->spi->State;
    for (;;) {
        // Do an atomic check of the state; WFI will exit even if IRQs are disabled
        uint32_t irq_state = disable_irq();
        if (*state == HAL_SPI_STATE_READY) {
            enable_irq(irq_state);
            return HAL_OK;
        }
        __WFI();
        enable_irq(irq_state);
        if (HAL_GetTick() - start >= timeout) {
            return HAL_TIMEOUT;
        }
    }
    return HAL_OK;
}

// A transfer of "len" bytes should take len*8*1000/baudrate milliseconds.
// To simplify the calculation we assume the baudrate is never less than 8kHz
// and use that value for the baudrate in the formula, plus a small constant.
#define SPI_TRANSFER_TIMEOUT(len) ((len) + 100)

STATIC void spi_transfer(const spi_t *self, size_t len, const uint8_t *src, uint8_t *dest, uint32_t timeout) {
    // Note: there seems to be a problem sending 1 byte using DMA the first
    // time directly after the SPI/DMA is initialised.  The cause of this is
    // unknown but we sidestep the issue by using polling for 1 byte transfer.

    HAL_StatusTypeDef status;

    if (dest == NULL) {
        // send only
        if (len == 1 || query_irq() == IRQ_STATE_DISABLED) {
            status = HAL_SPI_Transmit(self->spi, (uint8_t*)src, len, timeout);
        } else {
            DMA_HandleTypeDef tx_dma;
            dma_init(&tx_dma, self->tx_dma_descr, self->spi);
            self->spi->hdmatx = &tx_dma;
            self->spi->hdmarx = NULL;
            MP_HAL_CLEAN_DCACHE(src, len);
            status = HAL_SPI_Transmit_DMA(self->spi, (uint8_t*)src, len);
            if (status == HAL_OK) {
                status = spi_wait_dma_finished(self, timeout);
            }
            dma_deinit(self->tx_dma_descr);
        }
    } else if (src == NULL) {
        // receive only
        if (len == 1 || query_irq() == IRQ_STATE_DISABLED) {
            status = HAL_SPI_Receive(self->spi, dest, len, timeout);
        } else {
            DMA_HandleTypeDef tx_dma, rx_dma;
            if (self->spi->Init.Mode == SPI_MODE_MASTER) {
                // in master mode the HAL actually does a TransmitReceive call
                dma_init(&tx_dma, self->tx_dma_descr, self->spi);
                self->spi->hdmatx = &tx_dma;
            } else {
                self->spi->hdmatx = NULL;
            }
            dma_init(&rx_dma, self->rx_dma_descr, self->spi);
            self->spi->hdmarx = &rx_dma;
            MP_HAL_CLEANINVALIDATE_DCACHE(dest, len);
            status = HAL_SPI_Receive_DMA(self->spi, dest, len);
            if (status == HAL_OK) {
                status = spi_wait_dma_finished(self, timeout);
            }
            if (self->spi->hdmatx != NULL) {
                dma_deinit(self->tx_dma_descr);
            }
            dma_deinit(self->rx_dma_descr);
        }
    } else {
        // send and receive
        if (len == 1 || query_irq() == IRQ_STATE_DISABLED) {
            status = HAL_SPI_TransmitReceive(self->spi, (uint8_t*)src, dest, len, timeout);
        } else {
            DMA_HandleTypeDef tx_dma, rx_dma;
            dma_init(&tx_dma, self->tx_dma_descr, self->spi);
            self->spi->hdmatx = &tx_dma;
            dma_init(&rx_dma, self->rx_dma_descr, self->spi);
            self->spi->hdmarx = &rx_dma;
            MP_HAL_CLEAN_DCACHE(src, len);
            MP_HAL_CLEANINVALIDATE_DCACHE(dest, len);
            status = HAL_SPI_TransmitReceive_DMA(self->spi, (uint8_t*)src, dest, len);
            if (status == HAL_OK) {
                status = spi_wait_dma_finished(self, timeout);
            }
            dma_deinit(self->tx_dma_descr);
            dma_deinit(self->rx_dma_descr);
        }
    }

    if (status != HAL_OK) {
        mp_hal_raise(status);
    }
}

STATIC void spi_print(const mp_print_t *print, const spi_t *spi_obj, bool legacy) {
    SPI_HandleTypeDef *spi = spi_obj->spi;

    uint spi_num = 1; // default to SPI1
    if (spi->Instance == SPI2) { spi_num = 2; }
    else if (spi->Instance == SPI3) { spi_num = 3; }
    #if defined(SPI4)
    else if (spi->Instance == SPI4) { spi_num = 4; }
    #endif
    #if defined(SPI5)
    else if (spi->Instance == SPI5) { spi_num = 5; }
    #endif
    #if defined(SPI6)
    else if (spi->Instance == SPI6) { spi_num = 6; }
    #endif

    mp_printf(print, "SPI(%u", spi_num);
    if (spi->State != HAL_SPI_STATE_RESET) {
        if (spi->Init.Mode == SPI_MODE_MASTER) {
            // compute baudrate
            uint spi_clock;
            if (spi->Instance == SPI2 || spi->Instance == SPI3) {
                // SPI2 and SPI3 are on APB1
                spi_clock = HAL_RCC_GetPCLK1Freq();
            } else {
                // SPI1, SPI4, SPI5 and SPI6 are on APB2
                spi_clock = HAL_RCC_GetPCLK2Freq();
            }
            uint log_prescaler = (spi->Init.BaudRatePrescaler >> 3) + 1;
            uint baudrate = spi_clock >> log_prescaler;
            if (legacy) {
                mp_printf(print, ", SPI.MASTER");
            }
            mp_printf(print, ", baudrate=%u", baudrate);
            if (legacy) {
                mp_printf(print, ", prescaler=%u", 1 << log_prescaler);
            }
        } else {
            mp_printf(print, ", SPI.SLAVE");
        }
        mp_printf(print, ", polarity=%u, phase=%u, bits=%u", spi->Init.CLKPolarity == SPI_POLARITY_LOW ? 0 : 1, spi->Init.CLKPhase == SPI_PHASE_1EDGE ? 0 : 1, spi->Init.DataSize == SPI_DATASIZE_8BIT ? 8 : 16);
        if (spi->Init.CRCCalculation == SPI_CRCCALCULATION_ENABLE) {
            mp_printf(print, ", crc=0x%x", spi->Init.CRCPolynomial);
        }
    }
    mp_print_str(print, ")");
}

/******************************************************************************/
/* MicroPython bindings for legacy pyb API                                    */

typedef struct _pyb_spi_obj_t {
    mp_obj_base_t base;
    spi_obj_t *spi;
} pyb_spi_obj_t;

STATIC const pyb_spi_obj_t pyb_spi_obj[] = {
    {{&pyb_spi_type}, &spi_obj[0]},
    {{&pyb_spi_type}, &spi_obj[1]},
    {{&pyb_spi_type}, &spi_obj[2]},
    {{&pyb_spi_type}, &spi_obj[3]},
    {{&pyb_spi_type}, &spi_obj[4]},
    {{&pyb_spi_type}, &spi_obj[5]},
};

STATIC void pyb_spi_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pyb_spi_obj_t *self = self_in;
    spi_print(print, self->spi, true);
}

/// \method init(mode, baudrate=328125, *, polarity=1, phase=0, bits=8, firstbit=SPI.MSB, ti=False, crc=None)
///
/// Initialise the SPI bus with the given parameters:
///
///   - `mode` must be either `SPI.MASTER` or `SPI.SLAVE`.
///   - `baudrate` is the SCK clock rate (only sensible for a master).
STATIC mp_obj_t pyb_spi_init_helper(const pyb_spi_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode,     MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_baudrate, MP_ARG_INT, {.u_int = 328125} },
        { MP_QSTR_prescaler, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0xffffffff} },
        { MP_QSTR_polarity, MP_ARG_KW_ONLY | MP_ARG_INT,  {.u_int = 1} },
        { MP_QSTR_phase,    MP_ARG_KW_ONLY | MP_ARG_INT,  {.u_int = 0} },
        { MP_QSTR_dir,      MP_ARG_KW_ONLY | MP_ARG_INT,  {.u_int = SPI_DIRECTION_2LINES} },
        { MP_QSTR_bits,     MP_ARG_KW_ONLY | MP_ARG_INT,  {.u_int = 8} },
        { MP_QSTR_nss,      MP_ARG_KW_ONLY | MP_ARG_INT,  {.u_int = SPI_NSS_SOFT} },
        { MP_QSTR_firstbit, MP_ARG_KW_ONLY | MP_ARG_INT,  {.u_int = SPI_FIRSTBIT_MSB} },
        { MP_QSTR_ti,       MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_crc,      MP_ARG_KW_ONLY | MP_ARG_OBJ,  {.u_obj = mp_const_none} },
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // set the SPI configuration values
    SPI_InitTypeDef *init = &self->spi->spi->Init;
    init->Mode = args[0].u_int;

    spi_set_params(self->spi, args[2].u_int, args[1].u_int, args[3].u_int, args[4].u_int,
        args[6].u_int, args[8].u_int);

    init->Direction = args[5].u_int;
    init->NSS = args[7].u_int;
    init->TIMode = args[9].u_bool ? SPI_TIMODE_ENABLE : SPI_TIMODE_DISABLE;
    if (args[10].u_obj == mp_const_none) {
        init->CRCCalculation = SPI_CRCCALCULATION_DISABLE;
        init->CRCPolynomial = 0;
    } else {
        init->CRCCalculation = SPI_CRCCALCULATION_ENABLE;
        init->CRCPolynomial = mp_obj_get_int(args[10].u_obj);
    }

    // init the SPI bus
    spi_init(self->spi, init->NSS != SPI_NSS_SOFT);

    return mp_const_none;
}

/// \classmethod \constructor(bus, ...)
///
/// Construct an SPI object on the given bus.  `bus` can be 1 or 2.
/// With no additional parameters, the SPI object is created but not
/// initialised (it has the settings from the last initialisation of
/// the bus, if any).  If extra arguments are given, the bus is initialised.
/// See `init` for parameters of initialisation.
///
/// The physical pins of the SPI busses are:
///
///   - `SPI(1)` is on the X position: `(NSS, SCK, MISO, MOSI) = (X5, X6, X7, X8) = (PA4, PA5, PA6, PA7)`
///   - `SPI(2)` is on the Y position: `(NSS, SCK, MISO, MOSI) = (Y5, Y6, Y7, Y8) = (PB12, PB13, PB14, PB15)`
///
/// At the moment, the NSS pin is not used by the SPI driver and is free
/// for other use.
STATIC mp_obj_t pyb_spi_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // check arguments
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

    // work out SPI bus
    int spi_id = spi_find(args[0]);

    // get SPI object
    const pyb_spi_obj_t *spi_obj = &pyb_spi_obj[spi_id - 1];

    if (n_args > 1 || n_kw > 0) {
        // start the peripheral
        mp_map_t kw_args;
        mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
        pyb_spi_init_helper(spi_obj, n_args - 1, args + 1, &kw_args);
    }

    return (mp_obj_t)spi_obj;
}

STATIC mp_obj_t pyb_spi_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return pyb_spi_init_helper(args[0], n_args - 1, args + 1, kw_args);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_spi_init_obj, 1, pyb_spi_init);

/// \method deinit()
/// Turn off the SPI bus.
STATIC mp_obj_t pyb_spi_deinit(mp_obj_t self_in) {
    pyb_spi_obj_t *self = self_in;
    spi_deinit(self->spi);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_spi_deinit_obj, pyb_spi_deinit);

/// \method send(send, *, timeout=5000)
/// Send data on the bus:
///
///   - `send` is the data to send (an integer to send, or a buffer object).
///   - `timeout` is the timeout in milliseconds to wait for the send.
///
/// Return value: `None`.
STATIC mp_obj_t pyb_spi_send(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // TODO assumes transmission size is 8-bits wide

    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_send,    MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 5000} },
    };

    // parse args
    pyb_spi_obj_t *self = pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // get the buffer to send from
    mp_buffer_info_t bufinfo;
    uint8_t data[1];
    pyb_buf_get_for_send(args[0].u_obj, &bufinfo, data);

    // send the data
    spi_transfer(self->spi, bufinfo.len, bufinfo.buf, NULL, args[1].u_int);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_spi_send_obj, 1, pyb_spi_send);

/// \method recv(recv, *, timeout=5000)
///
/// Receive data on the bus:
///
///   - `recv` can be an integer, which is the number of bytes to receive,
///     or a mutable buffer, which will be filled with received bytes.
///   - `timeout` is the timeout in milliseconds to wait for the receive.
///
/// Return value: if `recv` is an integer then a new buffer of the bytes received,
/// otherwise the same buffer that was passed in to `recv`.
STATIC mp_obj_t pyb_spi_recv(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // TODO assumes transmission size is 8-bits wide

    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_recv,    MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 5000} },
    };

    // parse args
    pyb_spi_obj_t *self = pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // get the buffer to receive into
    vstr_t vstr;
    mp_obj_t o_ret = pyb_buf_get_for_recv(args[0].u_obj, &vstr);

    // receive the data
    spi_transfer(self->spi, vstr.len, NULL, (uint8_t*)vstr.buf, args[1].u_int);

    // return the received data
    if (o_ret != MP_OBJ_NULL) {
        return o_ret;
    } else {
        return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_spi_recv_obj, 1, pyb_spi_recv);

/// \method send_recv(send, recv=None, *, timeout=5000)
///
/// Send and receive data on the bus at the same time:
///
///   - `send` is the data to send (an integer to send, or a buffer object).
///   - `recv` is a mutable buffer which will be filled with received bytes.
///   It can be the same as `send`, or omitted.  If omitted, a new buffer will
///   be created.
///   - `timeout` is the timeout in milliseconds to wait for the receive.
///
/// Return value: the buffer with the received bytes.
STATIC mp_obj_t pyb_spi_send_recv(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // TODO assumes transmission size is 8-bits wide

    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_send,    MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_recv,    MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 5000} },
    };

    // parse args
    pyb_spi_obj_t *self = pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // get buffers to send from/receive to
    mp_buffer_info_t bufinfo_send;
    uint8_t data_send[1];
    mp_buffer_info_t bufinfo_recv;
    vstr_t vstr_recv;
    mp_obj_t o_ret;

    if (args[0].u_obj == args[1].u_obj) {
        // same object for send and receive, it must be a r/w buffer
        mp_get_buffer_raise(args[0].u_obj, &bufinfo_send, MP_BUFFER_RW);
        bufinfo_recv = bufinfo_send;
        o_ret = args[0].u_obj;
    } else {
        // get the buffer to send from
        pyb_buf_get_for_send(args[0].u_obj, &bufinfo_send, data_send);

        // get the buffer to receive into
        if (args[1].u_obj == MP_OBJ_NULL) {
            // only send argument given, so create a fresh buffer of the send length
            vstr_init_len(&vstr_recv, bufinfo_send.len);
            bufinfo_recv.len = vstr_recv.len;
            bufinfo_recv.buf = vstr_recv.buf;
            o_ret = MP_OBJ_NULL;
        } else {
            // recv argument given
            mp_get_buffer_raise(args[1].u_obj, &bufinfo_recv, MP_BUFFER_WRITE);
            if (bufinfo_recv.len != bufinfo_send.len) {
                mp_raise_ValueError("recv must be same length as send");
            }
            o_ret = args[1].u_obj;
        }
    }

    // do the transfer
    spi_transfer(self->spi, bufinfo_send.len, bufinfo_send.buf, bufinfo_recv.buf, args[2].u_int);

    // return the received data
    if (o_ret != MP_OBJ_NULL) {
        return o_ret;
    } else {
        return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr_recv);
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_spi_send_recv_obj, 1, pyb_spi_send_recv);

STATIC const mp_rom_map_elem_t pyb_spi_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&pyb_spi_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&pyb_spi_deinit_obj) },

    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_machine_spi_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_machine_spi_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_machine_spi_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_write_readinto), MP_ROM_PTR(&mp_machine_spi_write_readinto_obj) },

    // legacy methods
    { MP_ROM_QSTR(MP_QSTR_send), MP_ROM_PTR(&pyb_spi_send_obj) },
    { MP_ROM_QSTR(MP_QSTR_recv), MP_ROM_PTR(&pyb_spi_recv_obj) },
    { MP_ROM_QSTR(MP_QSTR_send_recv), MP_ROM_PTR(&pyb_spi_send_recv_obj) },

    // class constants
    /// \constant MASTER - for initialising the bus to master mode
    /// \constant SLAVE - for initialising the bus to slave mode
    /// \constant MSB - set the first bit to MSB
    /// \constant LSB - set the first bit to LSB
    { MP_ROM_QSTR(MP_QSTR_MASTER), MP_ROM_INT(SPI_MODE_MASTER) },
    { MP_ROM_QSTR(MP_QSTR_SLAVE),  MP_ROM_INT(SPI_MODE_SLAVE) },
    { MP_ROM_QSTR(MP_QSTR_MSB),    MP_ROM_INT(SPI_FIRSTBIT_MSB) },
    { MP_ROM_QSTR(MP_QSTR_LSB),    MP_ROM_INT(SPI_FIRSTBIT_LSB) },
    /* TODO
    { MP_ROM_QSTR(MP_QSTR_DIRECTION_2LINES             ((uint32_t)0x00000000)
    { MP_ROM_QSTR(MP_QSTR_DIRECTION_2LINES_RXONLY      SPI_CR1_RXONLY
    { MP_ROM_QSTR(MP_QSTR_DIRECTION_1LINE              SPI_CR1_BIDIMODE
    { MP_ROM_QSTR(MP_QSTR_NSS_SOFT                    SPI_CR1_SSM
    { MP_ROM_QSTR(MP_QSTR_NSS_HARD_INPUT              ((uint32_t)0x00000000)
    { MP_ROM_QSTR(MP_QSTR_NSS_HARD_OUTPUT             ((uint32_t)0x00040000)
    */
};

STATIC MP_DEFINE_CONST_DICT(pyb_spi_locals_dict, pyb_spi_locals_dict_table);

STATIC void spi_transfer_machine(mp_obj_base_t *self_in, size_t len, const uint8_t *src, uint8_t *dest) {
    pyb_spi_obj_t *self = (pyb_spi_obj_t*)self_in;
    spi_transfer(self->spi, len, src, dest, SPI_TRANSFER_TIMEOUT(len));
}

STATIC const mp_machine_spi_p_t pyb_spi_p = {
    .transfer = spi_transfer_machine,
};

const mp_obj_type_t pyb_spi_type = {
    { &mp_type_type },
    .name = MP_QSTR_SPI,
    .print = pyb_spi_print,
    .make_new = pyb_spi_make_new,
    .protocol = &pyb_spi_p,
    .locals_dict = (mp_obj_dict_t*)&pyb_spi_locals_dict,
};

/******************************************************************************/
// Implementation of hard SPI for machine module

typedef struct _machine_hard_spi_obj_t {
    mp_obj_base_t base;
    uint32_t spi_base;
    uint32_t periph;
    periph_spi_t* regs;
    uint32_t irqn;
    uint8_t mode : 1;
    uint32_t baudrate;
    uint8_t bits;
    // uint8_t polarity : 1;
    // uint8_t phase : 1;
    uint32_t protocol;
    spi_id_t spi_id : 3;
    bool is_enabled : 1;
    bool soft_fss : 1; // Frame signal generated by hardware, or manually
    uint8_t dma_enabled : 3;
} machine_hard_spi_obj_t;

STATIC void machine_hard_spi_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_hard_spi_obj_t *self = (machine_hard_spi_obj_t*)self_in;
    spi_print(print, self->spi, false);
}

mp_obj_t machine_hard_spi_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_mode, ARG_id, ARG_baudrate,/* ARG_prescaler, ARG_polarity, ARG_phase,*/ ARG_bits, ARG_fss, ARG_protocol, ARG_dma, ARG_firstbit, ARG_sck, ARG_mosi, ARG_miso };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode,     MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = SSI_MODE_MASTER} }, // Default as master
        { MP_QSTR_id,       MP_ARG_OBJ, {.u_obj = MP_OBJ_NEW_SMALL_INT(-1)} },
        { MP_QSTR_baudrate, MP_ARG_INT, {.u_int = 500000} },
        //{ MP_QSTR_prescaler, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0xFF} },       // max prescaler
        // { MP_QSTR_polarity, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} }, // idle low          
        // { MP_QSTR_phase,    MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} }, // first clock edge
        //{ MP_QSTR_dir,      MP_ARG_KW_ONLY | MP_ARG_INT,  {.u_int = SSI_CR1_DIR} },
        { MP_QSTR_bits,     MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 8} }, 
        { MP_QSTR_fss,   MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false}}, // chip select mode (software or hardware)
        { MP_QSTR_protocol,      MP_ARG_KW_ONLY | MP_ARG_INT,  {.u_int = SSI_FRF_MOTO_MODE_0} }, // SSI in SPI mode
        { MP_QSTR_dma,      MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_int = 0}}, // no dma
        //{ MP_QSTR_firstbit, MP_ARG_KW_ONLY | MP_ARG_INT,  {.u_int = SPI_FIRSTBIT_MSB} },
        //{ MP_QSTR_ti,       MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
        //{ MP_QSTR_crc,      MP_ARG_KW_ONLY | MP_ARG_OBJ,  {.u_obj = mp_const_none} },
        //{ MP_QSTR_sck,      MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        //{ MP_QSTR_mosi,     MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        //{ MP_QSTR_miso,     MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // create dynamic peripheral object
    spi_id_t spi_id = spi_find(args[ARG_id].u_obj);
    machine_hard_spi_obj_t *self;
    if (MP_STATE_PORT(machine_spi_obj_all)[spi_id - 1] == NULL) {
        // create new SSI object
        self = m_new0(machine_hard_spi_obj_t, 1);
        self->base.type = &machine_hard_spi_type;
        self->spi_id = spi_id;
        MP_STATE_PORT(machine_spi_obj_all)[spi_id - 1] = self;
    } else {
        // reference existing SSI object
        self = MP_STATE_PORT(machine_spi_obj_all)[spi_id - 1];
    }

    // here we would check the sck/mosi/miso pins and configure them, but it's not implemented
    if (args[ARG_sck].u_obj != MP_OBJ_NULL
        || args[ARG_mosi].u_obj != MP_OBJ_NULL
        || args[ARG_miso].u_obj != MP_OBJ_NULL) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_raise_ValueError,"explicit choice of sck/mosi/miso is not implemented"));
    }

    // set the SPI configuration values
    if(args[ARG_mode].u_int > 2) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_raise_ValueError, "Mode accepts only MASTER or SLAVE"));
    }
    self->mode = args[ARG_mode].u_int;

    if(args[ARG_bits].u_int >= 16 || args[ARG_bits].u_int <= 4) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_raise_ValueError, "invalid word length, only values 4..16 are available"));
    }
    self->bits = args[ARG_bits].u_int;

    // if(args[ARG_prescaler].u_int & 0xFFFFFF00) {
    //     nlr_raise(mp_obj_new_exception_msg_varg(&mp_raise_ValueError, "prescaler only 2-254!"));
    // }
    // uint8_t prescale = args[ARG_prescaler].u_int;

    if(self->mode) {
        if(args[ARG_baudrate].u_int >= 25000000) {
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_raise_ValueError, "baudrate too high, max 25 Mbaud as master"));
        }
    } else {
        if(args[ARG_baudrate].u_int >= 6666666) {
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_raise_ValueError, "baudrate too high, max 6.66 mBaud as slave"));
        }
    }
    self->baudrate = args[ARG_baudrate].u_int;
    // BR = Clk / (prescale * (1+ clockrate))
    // assume cr as 0x7F, prescaler only even values
    // if (prescale != 0xff || baudrate != -1) {
    //     if (prescale == 0xff) {
    //         // prescaler not given, so select one that yields at most the requested baudrate
    //         prescale = MAP_SysCtlClockGet() / (baudrate * 0x80);
    //         prescale &= 0xFE;
    //         if(!prescale) prescale = 2;
    //     }
    //     self->prescaler = prescale;
    // }
//! Polarity Phase       Mode
//!   0       0   SSI_FRF_MOTO_MODE_0
//!   0       1   SSI_FRF_MOTO_MODE_1
//!   1       0   SSI_FRF_MOTO_MODE_2
//!   1       1   SSI_FRF_MOTO_MODE_3

    // if(args[ARG_polarity].u_int != 0 || args[ARG_polarity].u_int != 1) {
    //     nlr_raise(mp_obj_new_exception_msg_varg(&mp_raise_ValueError, "polarity accepts only IDLE_LOW(0) or IDLE_HIGH(1)"));
    // }
    // self->polarity = args[ARG_polarity].u_int;
    // if(args[ARG_phase].u_int != 0 || args[ARG_phase].u_int != 1) {
    //     nlr_raise(mp_obj_new_exception_msg_varg(&mp_raise_ValueError, "phase accepts only FIRST_EDGE(0) or SECOND_EDGE(1)"));
    // }
    // self->phase = args[ARG_phase].u_int;

    if(!(args[ARG_protocol].u_int == SSI_FRF_MOTO_MODE_0 || args[ARG_protocol].u_int == SSI_FRF_MOTO_MODE_1  ||
         args[ARG_protocol].u_int == SSI_FRF_MOTO_MODE_2 || args[ARG_protocol].u_int == SSI_FRF_MOTO_MODE_3  ||
         args[ARG_protocol].u_int == SSI_FRF_TI || args[ARG_protocol].u_int == SSI_FRF_NMW)) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_raise_ValueError, "protocol not supported, please use SPI0..3, TI or MICROWIRE!"));
    }
    self->protocol = args[ARG_fss].u_int;

    if(args[ARG_fss].u_int != 0 || args[ARG_fss].u_int != 1) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_raise_ValueError, "fss only accepts CS_HARD(0) or CS_SOFT(1)"));
    }
    // Automatic or Manually assert chipselect? Only applicable in SPI mode
    if(self->protocol == SSI_CR0_FRF_MOTO) {
        self->soft_fss = args[ARG_fss].u_bool;
    } else {
        self->soft_fss = false;
    }

    if(!(args[ARG_dma].u_int & 0x03)) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_raise_ValueError, "dma accepts only DMA_NONE(0), DMA_RX(1), DMA_TX(2) or DMA_BOTH(3)"));
    }
    self->dma_enabled = args[ARG_dma].u_int;
    
    // init the SPI bus
    spi_init(self);

    return MP_OBJ_FROM_PTR(self);
}

STATIC void machine_hard_spi_init(mp_obj_base_t *self_in, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    machine_hard_spi_obj_t *self = (machine_hard_spi_obj_t*)self_in;

    enum { ARG_baudrate, ARG_polarity, ARG_phase, ARG_bits, ARG_firstbit };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_baudrate, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_polarity, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_phase,    MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_bits,     MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_firstbit, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // set the SPI configuration values
    spi_set_params(self->spi, 0xffffffff, args[ARG_baudrate].u_int,
        args[ARG_polarity].u_int, args[ARG_phase].u_int, args[ARG_bits].u_int,
        args[ARG_firstbit].u_int);

    // re-init the SPI bus
    spi_init(self->spi, false);
}

STATIC void machine_hard_spi_deinit(mp_obj_base_t *self_in) {
    machine_hard_spi_obj_t *self = (machine_hard_spi_obj_t*)self_in;
    spi_deinit(self->spi);
}

STATIC void machine_hard_spi_transfer(mp_obj_base_t *self_in, size_t len, const uint8_t *src, uint8_t *dest) {
    machine_hard_spi_obj_t *self = (machine_hard_spi_obj_t*)self_in;
    spi_transfer(self->spi, len, src, dest, SPI_TRANSFER_TIMEOUT(len));
}

STATIC const mp_rom_map_elem_t pyb_spi_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&pyb_spi_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&pyb_spi_deinit_obj) },

    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_machine_spi_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_machine_spi_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_machine_spi_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_write_readinto), MP_ROM_PTR(&mp_machine_spi_write_readinto_obj) },

    // legacy methods
    { MP_ROM_QSTR(MP_QSTR_send), MP_ROM_PTR(&pyb_spi_send_obj) },
    { MP_ROM_QSTR(MP_QSTR_recv), MP_ROM_PTR(&pyb_spi_recv_obj) },
    { MP_ROM_QSTR(MP_QSTR_send_recv), MP_ROM_PTR(&pyb_spi_send_recv_obj) },

    // class constants
    /// \constant MASTER - for initialising the bus to master mode
    /// \constant SLAVE - for initialising the bus to slave mode
    /// \constant MSB - set the first bit to MSB
    /// \constant LSB - set the first bit to LSB
    { MP_ROM_QSTR(MP_QSTR_MASTER), MP_ROM_INT(SSI_MODE_MASTER) },
    { MP_ROM_QSTR(MP_QSTR_SLAVE),  MP_ROM_INT(SSI_MODE_SLAVE) },
    { MP_ROM_QSTR(MP_QSTR_SLAVE_OD),  MP_ROM_INT(SSI_MODE_SLAVE_OD) },
    // { MP_ROM_QSTR(MP_QSTR_MSB),    MP_ROM_INT(SPI_FIRSTBIT_MSB) },
    // { MP_ROM_QSTR(MP_QSTR_LSB),    MP_ROM_INT(SPI_FIRSTBIT_LSB) },
    { MP_ROM_QSTR(MP_QSTR_SPI0),    MP_ROM_INT(SSI_FRF_MOTO_MODE_0)},
    { MP_ROM_QSTR(MP_QSTR_SPI1),    MP_ROM_INT(SSI_FRF_MOTO_MODE_1)},
    { MP_ROM_QSTR(MP_QSTR_SPI2),    MP_ROM_INT(SSI_FRF_MOTO_MODE_2)},
    { MP_ROM_QSTR(MP_QSTR_SPI3),    MP_ROM_INT(SSI_FRF_MOTO_MODE_3)},
    { MP_ROM_QSTR(MP_QSTR_TI),      MP_ROM_INT(SSI_FRF_TI)},
    { MP_ROM_QSTR(MP_QSTR_MICROWIRE),    MP_ROM_INT(SSI_FRF_NMW)},
    { MP_ROM_QSTR(MP_QSTR_FIRST_EDGE),    MP_ROM_INT(0)},
    { MP_ROM_QSTR(MP_QSTR_SECOND_EDGE),    MP_ROM_INT(1)},
    { MP_ROM_QSTR(MP_QSTR_IDLE_LOW),    MP_ROM_INT(0)},
    { MP_ROM_QSTR(MP_QSTR_IDLE_HIGH),    MP_ROM_INT(1)},
    { MP_ROM_QSTR(MP_QSTR_FSS_SOFT),    MP_ROM_INT(true)},
    { MP_ROM_QSTR(MP_QSTR_FSS_HARD),    MP_ROM_INT(false)},
    { MP_ROM_QSTR(MP_QSTR_DMA_NONE),    MP_ROM_INT(0)},
    { MP_ROM_QSTR(MP_QSTR_DMA_RX),    MP_ROM_INT(SSI_DMA_RX)},
    { MP_ROM_QSTR(MP_QSTR_DMA_TX),    MP_ROM_INT(SSI_DMA_TX)},
    { MP_ROM_QSTR(MP_QSTR_DMA_BOTH),    MP_ROM_INT(SSI_DMA_TX | SSI_DMA_RX)},

    /* TODO
    { MP_ROM_QSTR(MP_QSTR_DIRECTION_2LINES             ((uint32_t)0x00000000)
    { MP_ROM_QSTR(MP_QSTR_DIRECTION_2LINES_RXONLY      SPI_CR1_RXONLY
    { MP_ROM_QSTR(MP_QSTR_DIRECTION_1LINE              SPI_CR1_BIDIMODE
    { MP_ROM_QSTR(MP_QSTR_NSS_SOFT                    SPI_CR1_SSM
    { MP_ROM_QSTR(MP_QSTR_NSS_HARD_INPUT              ((uint32_t)0x00000000)
    { MP_ROM_QSTR(MP_QSTR_NSS_HARD_OUTPUT             ((uint32_t)0x00040000)
    */
};

STATIC MP_DEFINE_CONST_DICT(pyb_spi_locals_dict, pyb_spi_locals_dict_table);

STATIC const mp_machine_spi_p_t machine_hard_spi_p = {
    .init = machine_hard_spi_init,
    .deinit = machine_hard_spi_deinit,
    .transfer = machine_hard_spi_transfer,
};

const mp_obj_type_t machine_hard_spi_type = {
    { &mp_type_type },
    .name = MP_QSTR_SPI,
    .print = machine_hard_spi_print,
    .make_new = mp_machine_spi_make_new, // delegate to master constructor
    .protocol = &machine_hard_spi_p,
    .locals_dict = (mp_obj_t)&mp_machine_spi_locals_dict,
};

const spi_t *spi_from_mp_obj(mp_obj_t o) {
    if (MP_OBJ_IS_TYPE(o, &pyb_spi_type)) {
        pyb_spi_obj_t *self = o;
        return self->spi;
    } else if (MP_OBJ_IS_TYPE(o, &machine_hard_spi_type)) {
        machine_hard_spi_obj_t *self = o;;
        return self->spi;
    } else {
        mp_raise_TypeError("expecting an SPI object");
    }
}
