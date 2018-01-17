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
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "pwm.h"

/// Physical memory file descriptor
int mem_fd;
/// Pointer to PWMSS0 (PWM-Sub-System) registers
volatile uint16_t* pwmss0_regs;
/// Pointer to PWMSS1 (PWM-Sub-System) registers
volatile uint16_t* pwmss1_regs;

/**
 * Map PWM registers into user space (on {@link pwmss0_regs} and {@link
 * pwmss1_regs} pointer)
 * @return {@link SUCCESS} in case of success, {@link FAILURE} otherwise
 */
int pwm_setup(void) {

    // open /dev/mem for memory mapping
    if ((mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
        rl_log(ERROR, "can't open /dev/mem");
        return FAILURE;
    }

    // map pwm0 registers into virtual memory
    pwmss0_regs =
        (volatile uint16_t*)mmap(NULL, PWM_SIZE, PROT_READ | PROT_WRITE,
                                 MAP_SHARED, mem_fd, PWMSS0_BASE);
    if (pwmss1_regs == (volatile uint16_t*)MAP_FAILED) {
        rl_log(ERROR, "mmap failed");
        close(mem_fd);
        return FAILURE;
    }

    // map pwm1 registers into virtual memory
    pwmss1_regs =
        (volatile uint16_t*)mmap(NULL, PWM_SIZE, PROT_READ | PROT_WRITE,
                                 MAP_SHARED, mem_fd, PWMSS1_BASE);
    if (pwmss1_regs == (volatile uint16_t*)MAP_FAILED) {
        rl_log(ERROR, "mmap failed");
        close(mem_fd);
        return FAILURE;
    }

    return SUCCESS;
}

/**
 * Unmap PWM registers from user space
 */
void pwm_close(void) {

    // unmap memory
    munmap((void*)pwmss0_regs, PWM_SIZE);
    munmap((void*)pwmss1_regs, PWM_SIZE);

    // close /dev/mem
    close(mem_fd);
}

/**
 * Setup PWMSS1 for range latch reset clock
 * @param sample_rate ADC sampling rate in Sps
 */
void pwm_setup_range_clock(int sample_rate) {

    int period = PWM_PERIOD_SCALE / sample_rate;

    // setup ehrpwm
    pwmss1_regs[TBCTL] =
        TBCTL_DEFAULT | UP_DOWN_COUNT | PRESCALE2; // set clock mode

    pwmss1_regs[TBPRD] = period; // set clock period

    pwmss1_regs[CMPA] = (1 - PULSE_WIDTH / 2) *
                        period; // set compare registers for both output signals
    pwmss1_regs[CMPB] = PULSE_WIDTH / 2 * period;

    pwmss1_regs[AQCTLA] = RWC_AQ_A; // set actions
    pwmss1_regs[AQCTLB] = RWC_AQ_B;
}

/**
 * Setup PWMSS0 for ADC master clock
 */
void pwm_setup_adc_clock(void) {

    // setup ehrpwm
    pwmss0_regs[TBCTL] = TBCTL_DEFAULT;
    pwmss0_regs[TBPRD] = ADC_CLOCK_PERIOD;
    pwmss0_regs[CMPA] = ADC_CLOCK_PERIOD / 2; // 50% duty
    pwmss0_regs[AQCTLA] = ADC_AQ;
}
