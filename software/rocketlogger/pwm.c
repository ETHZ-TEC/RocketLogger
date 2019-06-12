/**
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

#include <stddef.h>
#include <stdint.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "log.h"
#include "rl.h"
#include "sysfs.h"

#include "pwm.h"

/// Physical memory file descriptor
int mem_fd = -1;
/// Pointer to PWM0 registers
volatile uint8_t *pwm0_mem = NULL;
/// Pointer to PWM1 registers
volatile uint8_t *pwm1_mem = NULL;

int pwm_init(void) {
    // export and enable PWM peripherals via sysfs interface
    sysfs_export_unexported((EPWM0A_SYSFS_PATH), (PWM0_SYSFS_PATH "export"),
                            EPWM0A_SYSFS_INDEX);
    sysfs_export_unexported((EPWM1A_SYSFS_PATH), (PWM1_SYSFS_PATH "export"),
                            EPWM1A_SYSFS_INDEX);
    sysfs_export_unexported((EPWM1B_SYSFS_PATH), (PWM1_SYSFS_PATH "export"),
                            EPWM1B_SYSFS_INDEX);

    sysfs_write_int((EPWM0A_SYSFS_PATH "period"), PWM_PERIOD_DEFAULT);
    sysfs_write_int((EPWM1A_SYSFS_PATH "period"), PWM_PERIOD_DEFAULT);
    sysfs_write_int((EPWM1B_SYSFS_PATH "period"), PWM_PERIOD_DEFAULT);

    sysfs_write_int((EPWM0A_SYSFS_PATH "enable"), 1);
    sysfs_write_int((EPWM1A_SYSFS_PATH "enable"), 1);
    sysfs_write_int((EPWM1B_SYSFS_PATH "enable"), 1);

    // open /dev/mem for memory mapping
    mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd < 0) {
        rl_log(RL_LOG_ERROR, "can't open /dev/mem (%d)", mem_fd);
        return ERROR;
    }

    // map PWM0 registers
    pwm0_mem = (volatile uint8_t *)mmap(0, PWM_SIZE, PROT_READ | PROT_WRITE,
                                        MAP_SHARED, mem_fd, PWM0_BASE);
    if ((void *)pwm0_mem == MAP_FAILED) {
        pwm_deinit();
        rl_log(RL_LOG_ERROR, "mmap for PWM0 failed");
        return ERROR;
    }

    // map PWM1 registers
    pwm1_mem = (volatile uint8_t *)mmap(0, PWM_SIZE, PROT_READ | PROT_WRITE,
                                        MAP_SHARED, mem_fd, PWM1_BASE);
    if ((void *)pwm1_mem == MAP_FAILED) {
        pwm_deinit();
        rl_log(RL_LOG_ERROR, "mmap for PWM1 failed");
        return ERROR;
    }

    return SUCCESS;
}

void pwm_deinit(void) {
    // unmap memory
    if (pwm0_mem != NULL) {
        munmap((void *)pwm0_mem, PWM_SIZE);
        pwm0_mem = NULL;
    }
    if (pwm1_mem != NULL) {
        munmap((void *)pwm1_mem, PWM_SIZE);
        pwm1_mem = NULL;
    }

    // close /dev/mem
    if (mem_fd >= 0) {
        close(mem_fd);
        mem_fd = -1;
    }

    // disable and unexport peripheral via sysfs interface
    sysfs_write_int((EPWM0A_SYSFS_PATH "enable"), 0);
    sysfs_write_int((EPWM1A_SYSFS_PATH "enable"), 0);
    sysfs_write_int((EPWM1B_SYSFS_PATH "enable"), 0);

    sysfs_unexport((PWM0_SYSFS_PATH "unexport"), EPWM0A_SYSFS_INDEX);
    sysfs_unexport((PWM1_SYSFS_PATH "unexport"), EPWM1A_SYSFS_INDEX);
    sysfs_unexport((PWM1_SYSFS_PATH "unexport"), EPWM1B_SYSFS_INDEX);
}

void pwm_setup_range_reset(uint32_t sample_rate) {
    // calculate period and compare register values
    uint32_t period = RANGE_RESET_PERIOD_SCALE / sample_rate;
    uint32_t compare_b = (uint32_t)(period * RANGE_RESET_PULSE_WIDTH) / 2;
    uint32_t compare_a = period - compare_b;

    // get pointers of PWMSS1 registers
    volatile uint16_t *epwm1_tbctl =
        (volatile uint16_t *)(pwm1_mem + EPWM_OFFSET + EPWM_TBCTL_OFFSET);
    volatile uint16_t *epwm1_tbprd =
        (volatile uint16_t *)(pwm1_mem + EPWM_OFFSET + EPWM_TBPRD_OFFSET);
    volatile uint16_t *epwm1_cmpa =
        (volatile uint16_t *)(pwm1_mem + EPWM_OFFSET + EPWM_CMPA_OFFSET);
    volatile uint16_t *epwm1_cmpb =
        (volatile uint16_t *)(pwm1_mem + EPWM_OFFSET + EPWM_CMPB_OFFSET);
    volatile uint16_t *epwm1_aqctla =
        (volatile uint16_t *)(pwm1_mem + EPWM_OFFSET + EPWM_AQCTLA_OFFSET);
    volatile uint16_t *epwm1_aqctlb =
        (volatile uint16_t *)(pwm1_mem + EPWM_OFFSET + EPWM_AQCTLB_OFFSET);

    // set clock prescaler 2 and up-down counting mode, no period double
    // buffering
    *epwm1_tbctl =
        TBCTL_FREERUN | TBCTL_CLKDIV_2 | TBCTL_PRDLD | TBCTL_COUNT_UP_DOWN;

    // set period and compare register values
    *epwm1_tbprd = (uint16_t)period;
    *epwm1_cmpa = (uint16_t)compare_a;
    *epwm1_cmpb = (uint16_t)compare_b;

    // set action qualifiers for both channels
    *epwm1_aqctla = AQ_A_INCSET | AQ_A_DECCLR;
    *epwm1_aqctlb = AQ_B_INCCLR | AQ_B_DECSET;
}

void pwm_setup_adc_clock(void) {
    // get pointers of PWMSS0 registers
    volatile uint16_t *epwm0_tbctl =
        (volatile uint16_t *)(pwm0_mem + EPWM_OFFSET + EPWM_TBCTL_OFFSET);
    volatile uint16_t *epwm0_tbprd =
        (volatile uint16_t *)(pwm0_mem + EPWM_OFFSET + EPWM_TBPRD_OFFSET);
    volatile uint16_t *epwm0_cmpa =
        (volatile uint16_t *)(pwm0_mem + EPWM_OFFSET + EPWM_CMPA_OFFSET);
    volatile uint16_t *epwm0_aqctla =
        (volatile uint16_t *)(pwm0_mem + EPWM_OFFSET + EPWM_AQCTLA_OFFSET);

    // set clock prescaler 1 and up counting mode, no period double buffering
    *epwm0_tbctl =
        TBCTL_FREERUN | TBCTL_CLKDIV_1 | TBCTL_PRDLD | TBCTL_COUNT_UP;

    // set period and compare register values
    *epwm0_tbprd = ADC_CLOCK_PERIOD;
    *epwm0_cmpa = ADC_CLOCK_PERIOD / 2;

    // set action qualifiers for both channels
    *epwm0_aqctla = AQ_A_INCSET | AQ_PRDCLR | AQ_ZROCLR;
}
