/**
 * Copyright (c) 2016-2018, Swiss Federal Institute of Technology (ETH Zurich)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PWM_H_
#define PWM_H_

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "log.h"
#include "types.h"

// base addresses
/// PWMSS0 register base address
#define PWMSS0_BASE 0x48300000
/// PWMSS1 register base address
#define PWMSS1_BASE 0x48302000
/// EPWM module register offset
#define EPWM_OFFSET 0x0200

// pwm size
/// Size of PWM register memory
#define PWM_SIZE 0x00000FFF

// configuration registers
/// Counter control register offset
#define TBCTL (EPWM_OFFSET + 0x0) / sizeof(uint16_t) // counter control
/// Period register offset
#define TBPRD (EPWM_OFFSET + 0xA) / sizeof(uint16_t) // period
/// Compare register A offset
#define CMPA (EPWM_OFFSET + 0x12) / sizeof(uint16_t) // compare
/// Compare register B offset
#define CMPB (EPWM_OFFSET + 0x14) / sizeof(uint16_t)
/// Action qualifier register offset
#define AQCTLA (EPWM_OFFSET + 0x16) / sizeof(uint16_t) // action qualifier
/// Action qualifier register B offset
#define AQCTLB (EPWM_OFFSET + 0x18) / sizeof(uint16_t)

// register values
/// Default counter value (see AM335x_TR)
#define TBCTL_DEFAULT 0xC000
/// Up-down counting
#define UP_DOWN_COUNT 0x0002
/// Counter prescale 2
#define PRESCALE2 0x0400

// range switch clock configuration (action qualifier)
/// Action qualifier A value for latch reset (see AM335x_TR)
#define RWC_AQ_A 0x0060 // set when incrementing, clear when decrementing
/// Action qualifier B value for latch reset (see AM335x_TR)
#define RWC_AQ_B 0x0900 // set when decrementing, clear when incrementing

// pulse configuration
/// Latch reset pulse width (part of sampling period)
#define PULSE_WIDTH 0.1 // 10% of sampling period
/// Latch reset period margin
#define MARGIN 0.1
/// Latch reset period scaling factor
#define PWM_PERIOD_SCALE                                                       \
    50000000 *                                                                 \
        (1 + PULSE_WIDTH + MARGIN) // period scaling factor (period is set in
                                   // 5ns, (/2 clock prescaling))

// ADC clock settings
/// ADC master clock period in ns
#define ADC_CLOCK_PERIOD 48 // in 10ns
/// Action qualifier value for ADC clock (see AM335x_TR)
#define ADC_AQ 0x0025 // clear on zero and period, set at 50%

int pwm_setup(void);
void pwm_close(void);

void pwm_setup_range_clock(int sample_rate);
void pwm_setup_adc_clock(void);

#endif /* PWM_H_ */
