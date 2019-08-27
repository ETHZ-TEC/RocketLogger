/*
 * Copyright (c) 2016-2019, Swiss Federal Institute of Technology (ETH Zurich)
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
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PWM_H_
#define PWM_H_

#include <stdint.h>

/// Linux sysfs paths device files
/// Path to the Linux sysfs PWM0 device files
#define PWM0_SYSFS_PATH "/sys/class/pwm/pwmchip1/"
/// Path to the Linux sysfs PWM1 device files
#define PWM1_SYSFS_PATH "/sys/class/pwm/pwmchip4/"

/// Index of the Linux sysfs ePWM0A module
#define EPWM0A_SYSFS_INDEX 0
/// Index of the Linux sysfs ePWM1A module
#define EPWM1A_SYSFS_INDEX 0
/// Index of the Linux sysfs ePWM1B module
#define EPWM1B_SYSFS_INDEX 1

/// Path to the Linux sysfs ePWM0A module
#define EPWM0A_SYSFS_PATH PWM0_SYSFS_PATH "pwm-1:0/"
/// Path to the Linux sysfs ePWM1A module
#define EPWM1A_SYSFS_PATH PWM1_SYSFS_PATH "pwm-4:0/"
/// Path to the Linux sysfs ePWM1B module
#define EPWM1B_SYSFS_PATH PWM1_SYSFS_PATH "pwm-4:1/"

/// Default PWM period
#define PWM_PERIOD_DEFAULT 100000000

// PWM module register base addresses, offsets and sizes
/// PWM0 register base address
#define PWM0_BASE 0x48300000
/// PWM1 register base address
#define PWM1_BASE 0x48302000
/// PWM2 module register offset
#define PWM2_BASE 0x48304000

/// PWMSS module register offset
#define PWMSS_OFFSET 0x0000
/// eCAP module register offset
#define ECAP_OFFSET 0x0100
/// eQEP module register offset
#define EQEP_OFFSET 0x0180
/// ePWM module register offset
#define EPWM_OFFSET 0x0200

/// PWM module register map size
#define PWM_SIZE 0x0260
/// PWMSS module register map size
#define PWMSS_SIZE 0x0100
/// eCAP module register map size
#define ECAP_SIZE 0x0080
/// eQEP module register map size
#define EQEP_SIZE 0x0080
/// ePWM module register map size
#define EPWM_SIZE 0x0060

// PWMSS sub-module register base addresses, offsets and sizes

/// PWMSS0 register base address
#define PWMSS0_BASE (PWM0_BASE + PWMSS_OFFSET)
/// PWMSS1 register base address
#define PWMSS1_BASE (PWM1_BASE + PWMSS_OFFSET)
/// PWMSS2 module register offset
#define PWMSS2_BASE (PWM2_BASE + PWMSS_OFFSET)

// ePWM sub-module register base addresses, offsets and sizes
/// ePWM0 register base address
#define EPWM0_BASE (PWM0_BASE + EPWM_OFFSET)
/// ePWM1 register base address
#define EPWM1_BASE (PWM1_BASE + EPWM_OFFSET)
/// ePWM2 register base address
#define EPWM2_BASE (PWM2_BASE + EPWM_OFFSET)

// PWMSS configuration register offsets
/// IP Revision Register
#define PWMSS_IDVER_OFFSET 0x00
/// System Configuration Register
#define PWMSS_SYSCONFIG_OFFSET 0x04
/// Clock Configuration Register
#define PWMSS_CLKCONFIG_OFFSET 0x08
/// Clock Status Register
#define PWMSS_CLKSTATUS_OFFSET 0x0C

// ePWM configuration register offsets
/// Time-Base Control Register
#define EPWM_TBCTL_OFFSET 0x00
/// Time-Base Status Register
#define EPWM_TBSTS_OFFSET 0x02
/// Time-Base Phase Register
#define EPWM_TBPHS_OFFSET 0x06
/// Time-Base Counter Register
#define EPWM_TBCNT_OFFSET 0x08
/// Time-Base Period Register
#define EPWM_TBPRD_OFFSET 0x0A
/// Counter-Compare Control Register
#define EPWM_CMPCTL_OFFSET 0x0E
/// Counter-Compare A Register
#define EPWM_CMPA_OFFSET 0x12
/// Counter-Compare B Register
#define EPWM_CMPB_OFFSET 0x14
/// Action-Qualifier Control Register for Output A (EPWMxA)
#define EPWM_AQCTLA_OFFSET 0x16
/// Action-Qualifier Control Register for Output B (EPWMxB)
#define EPWM_AQCTLB_OFFSET 0x18

// ePWM configuration register values (selection)
/// TBCTL default value after reset
#define TBCTL_FREERUN 0xC000
/// Up counting mode
#define TBCTL_COUNT_UP 0x0000
/// Up-down counting mode
#define TBCTL_COUNT_UP_DOWN 0x0002
/// Period register double buffer disable
#define TBCTL_PRDLD 0x0008
/// Use counter clock prescaler of 1
#define TBCTL_CLKDIV_1 0x0000
/// Use counter clock prescaler of 2
#define TBCTL_CLKDIV_2 0x0400

/// Action quilifier: clear on zero
#define AQ_ZROCLR 0x0001
/// Action quilifier: set on zero
#define AQ_ZROSET 0x0002
/// Action quilifier: clear on period
#define AQ_PRDCLR 0x0004
/// Action quilifier: clear set on period
#define AQ_PRDSET 0x0008
/// Action quilifier: clear on compare A when incrementing
#define AQ_A_INCCLR 0x0010
/// Action quilifier: set on compare A when incrementing
#define AQ_A_INCSET 0x0020
/// Action quilifier: clear on compare A when decrementing
#define AQ_A_DECCLR 0x0040
/// Action quilifier: set on compare A when decrementing
#define AQ_A_DECSET 0x0080
/// Action quilifier: clear on compare B when incrementing
#define AQ_B_INCCLR 0x0100
/// Action quilifier: set on compare B when incrementing
#define AQ_B_INCSET 0x0200
/// Action quilifier: clear on compare B when decrementing
#define AQ_B_DECCLR 0x0400
/// Action quilifier: set on compare B when decrementing
#define AQ_B_DECSET 0x0800

/// ADC master clock period (in units 10 ns)
#define ADC_CLOCK_PERIOD 49
/// Range latch reset pulse width (fraction of sampling period in [0, 1])
#define RANGE_RESET_PULSE_WIDTH 0.1
/// Range latch reset period margin (fraction of sampling period in [0, 1])
#define RANGE_RESET_PERIOD_MARGIN 0.1
/// Range latch reset period scaling factor (in units of 20 ns)
#define RANGE_RESET_PERIOD_SCALE                                               \
    (50000000 * (1 + RANGE_RESET_PULSE_WIDTH + RANGE_RESET_PERIOD_MARGIN))

/**
 * Initialize PWM modules.
 *
 * Map PWM registers into user space (on {@link pwm0_mem} and {@link
 * pwm1_mem} pointer)
 *
 * @return Returns 0 on success, negative on failure with errno set accordingly
 */
int pwm_init(void);

/**
 * Deinitialize PWM modules.
 *
 * Unmapping registers from user space
 */
void pwm_deinit(void);

/**
 * Setup PWMSS1 for range latch reset clock.
 *
 * @param sample_rate ADC sampling rate in Sps
 */
void pwm_setup_range_reset(uint32_t sample_rate);

/**
 * Setup PWMSS0 for ADC master clock.
 */
void pwm_setup_adc_clock(void);

#endif /* PWM_H_ */
