/**
 * Copyright (c) 2016-2019, ETH Zurich, Computer Engineering Group
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
#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


// Clock Management module register base addresses, offsets and sizes
/// CM_PER register base address
#define CM_PER_BASE 0x44E00000
#define CM_PER_SIZE 0x0400

#define L4LS_CLKSTCTRL_OFFSET       0x00
#define L4LS_CLKCTRL_OFFSET         0x60
#define EPWMSS1_CLKCTRL_OFFSET      0xCC
#define EPWMSS0_CLKCTRL_OFFSET      0xD4
#define EPWMSS2_CLKCTRL_OFFSET      0xD8
#define PRU_ICSS_CLKCTRL_OFFSET     0xE8


// CONTROL module register base addresses, offsets and sizes
/// CONTROL register base address
#define CONTROL_BASE        0x44E10000
#define CONTROL_SIZE        0x20000

#define PWMSS_CTRL_OFFSET   0x664

// PWM module register base addresses, offsets and sizes
/// PWM0 register base address
#define PWM0_BASE 0x48300000
/// PWM1 register base address
#define PWM1_BASE 0x48302000
/// PWM2 module base address
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

/// Program main fuction
int main() {
    /// Physical memory file descriptor
    int mem_fd = -1;
    /// Pointer to CM_PER registers
    volatile uint8_t const *cm_per_mem = NULL;
    /// Pointer to CONTROL registers
    volatile uint8_t const *control_mem = NULL;
    /// Pointer to PWM0 registers
    volatile uint8_t const *pwm0_mem = NULL;
    /// Pointer to PWM1 registers
    volatile uint8_t const *pwm1_mem = NULL;

    // open /dev/mem for memory mapping
    mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd < 0) {
        return EXIT_FAILURE;
    }

    // map CM_PER registers
    cm_per_mem = (volatile uint8_t const *)mmap(0, CM_PER_SIZE, PROT_READ | PROT_WRITE,
                                        MAP_SHARED, mem_fd, CM_PER_BASE);
    if ((void *)cm_per_mem == MAP_FAILED) {
        return EXIT_FAILURE;
    }

    // map CONTROL registers
    control_mem = (volatile uint8_t const *)mmap(0, CONTROL_SIZE, PROT_READ | PROT_WRITE,
                                        MAP_SHARED, mem_fd, CONTROL_BASE);
    if ((void *)cm_per_mem == MAP_FAILED) {
        return EXIT_FAILURE;
    }

    // map PWM0 registers
    pwm0_mem = (volatile uint8_t const *)mmap(0, PWM_SIZE, PROT_READ | PROT_WRITE,
                                        MAP_SHARED, mem_fd, PWM0_BASE);
    if ((void *)pwm0_mem == MAP_FAILED) {
        return EXIT_FAILURE;
    }

    // map PWM1 registers
    pwm1_mem = (volatile uint8_t const *)mmap(0, PWM_SIZE, PROT_READ | PROT_WRITE,
                                        MAP_SHARED, mem_fd, PWM1_BASE);
    if ((void *)pwm1_mem == MAP_FAILED) {
        return EXIT_FAILURE;
    }

    // read memory mapped registers

    // get pointers of CM_PER registers
    volatile uint32_t const *cm_per_l4ls_clkstctrl =
        (volatile uint32_t const *)(cm_per_mem + L4LS_CLKSTCTRL_OFFSET);
    volatile uint32_t const *cm_per_l4ls_clkctrl =
        (volatile uint32_t const *)(cm_per_mem + L4LS_CLKCTRL_OFFSET);
    volatile uint32_t const *cm_per_epwmss0_clkctrl =
        (volatile uint32_t const *)(cm_per_mem + EPWMSS0_CLKCTRL_OFFSET);
    volatile uint32_t const *cm_per_epwmss1_clkctrl =
        (volatile uint32_t const *)(cm_per_mem + EPWMSS1_CLKCTRL_OFFSET);
    volatile uint32_t const *cm_per_epwmss2_clkctrl =
        (volatile uint32_t const *)(cm_per_mem + EPWMSS2_CLKCTRL_OFFSET);
    volatile uint32_t const *cm_per_pru_icss_clkctrl =
        (volatile uint32_t const *)(cm_per_mem + PRU_ICSS_CLKCTRL_OFFSET);

    printf("CM_PER registers:\n");
    printf("  L4LS_CLKSTCTRL:  \t0x%08x\n", *cm_per_l4ls_clkstctrl);
    printf("  L4LS_CLKCTRL:    \t0x%08x\n", *cm_per_l4ls_clkctrl);
    printf("  EPWMSS0_CLKCTRL: \t0x%08x\n", *cm_per_epwmss0_clkctrl);
    printf("  EPWMSS1_CLKCTRL: \t0x%08x\n", *cm_per_epwmss1_clkctrl);
    printf("  EPWMSS2_CLKCTRL: \t0x%08x\n", *cm_per_epwmss2_clkctrl);
    printf("  PRU_ICSS_CLKCTRL:\t0x%08x\n", *cm_per_pru_icss_clkctrl);

    // get pointers of CONTROL registers
    volatile uint32_t const *control_pwmss_ctrl =
        (volatile uint32_t const *)(control_mem + PWMSS_CTRL_OFFSET);

    printf("CONTROL registers:\n");
    printf("  PWMSS_CTRL:  \t0x%08x\n", *control_pwmss_ctrl);

    // get pointers of PWMSS0 registers
    volatile uint32_t const *pwmss0_sysconfig =
        (volatile uint32_t const *)(pwm0_mem + PWMSS_OFFSET + PWMSS_SYSCONFIG_OFFSET);
    volatile uint32_t const *pwmss0_clkconfig =
        (volatile uint32_t const *)(pwm0_mem + PWMSS_OFFSET + PWMSS_CLKCONFIG_OFFSET);
    volatile uint32_t const *pwmss0_clkstatus =
        (volatile uint32_t const *)(pwm0_mem + PWMSS_OFFSET + PWMSS_CLKSTATUS_OFFSET);

    volatile uint16_t const *epwm0_tbctl =
        (volatile uint16_t const *)(pwm0_mem + EPWM_OFFSET + EPWM_TBCTL_OFFSET);
    volatile uint16_t const *epwm0_tbcnt =
        (volatile uint16_t const *)(pwm0_mem + EPWM_OFFSET + EPWM_TBCNT_OFFSET);
    volatile uint16_t const *epwm0_tbprd =
        (volatile uint16_t const *)(pwm0_mem + EPWM_OFFSET + EPWM_TBPRD_OFFSET);
    volatile uint16_t const *epwm0_cmpa =
        (volatile uint16_t const *)(pwm0_mem + EPWM_OFFSET + EPWM_CMPA_OFFSET);
    volatile uint16_t const *epwm0_cmpb =
        (volatile uint16_t const *)(pwm0_mem + EPWM_OFFSET + EPWM_CMPB_OFFSET);
    volatile uint16_t const *epwm0_aqctla =
        (volatile uint16_t const *)(pwm0_mem + EPWM_OFFSET + EPWM_AQCTLA_OFFSET);
    volatile uint16_t const *epwm0_aqctlb =
        (volatile uint16_t const *)(pwm0_mem + EPWM_OFFSET + EPWM_AQCTLB_OFFSET);

    printf("PWMSS0 registers:\n");
    printf("  SYSCONFIG: \t0x%08x\n", *pwmss0_sysconfig);
    printf("  CLKCONFIG: \t0x%08x\n", *pwmss0_clkconfig);
    printf("  CLKSTATUS: \t0x%08x\n", *pwmss0_clkstatus);
    printf(" ePWM0 registers:\n");
    printf("  TBCTL: \t0x%04x\n", *epwm0_tbctl);
    printf("  TBCNT: \t0x%04x\n", *epwm0_tbcnt);
    printf("  TBPRD: \t0x%04x\n", *epwm0_tbprd);
    printf("  CMPA:  \t0x%04x\n", *epwm0_cmpa);
    printf("  CMPB:  \t0x%04x\n", *epwm0_cmpb);
    printf("  AQCTLA:\t0x%04x\n", *epwm0_aqctla);
    printf("  AQCTLB:\t0x%04x\n", *epwm0_aqctlb);

    // get pointers of PWMSS1 registers
    volatile uint32_t const *pwmss1_sysconfig =
        (volatile uint32_t const *)(pwm1_mem + PWMSS_OFFSET + PWMSS_SYSCONFIG_OFFSET);
    volatile uint32_t const *pwmss1_clkconfig =
        (volatile uint32_t const *)(pwm1_mem + PWMSS_OFFSET + PWMSS_CLKCONFIG_OFFSET);
    volatile uint32_t const *pwmss1_clkstatus =
        (volatile uint32_t const *)(pwm1_mem + PWMSS_OFFSET + PWMSS_CLKSTATUS_OFFSET);

    volatile uint16_t const *epwm1_tbctl =
        (volatile uint16_t const *)(pwm1_mem + EPWM_OFFSET + EPWM_TBCTL_OFFSET);
    volatile uint16_t const *epwm1_tbcnt =
        (volatile uint16_t const *)(pwm1_mem + EPWM_OFFSET + EPWM_TBCNT_OFFSET);
    volatile uint16_t const *epwm1_tbprd =
        (volatile uint16_t const *)(pwm1_mem + EPWM_OFFSET + EPWM_TBPRD_OFFSET);
    volatile uint16_t const *epwm1_cmpa =
        (volatile uint16_t const *)(pwm1_mem + EPWM_OFFSET + EPWM_CMPA_OFFSET);
    volatile uint16_t const *epwm1_cmpb =
        (volatile uint16_t const *)(pwm1_mem + EPWM_OFFSET + EPWM_CMPB_OFFSET);
    volatile uint16_t const *epwm1_aqctla =
        (volatile uint16_t const *)(pwm1_mem + EPWM_OFFSET + EPWM_AQCTLA_OFFSET);
    volatile uint16_t const *epwm1_aqctlb =
        (volatile uint16_t const *)(pwm1_mem + EPWM_OFFSET + EPWM_AQCTLB_OFFSET);

    printf("PWMSS1 registers:\n");
    printf("  SYSCONFIG: \t0x%08x\n", *pwmss1_sysconfig);
    printf("  CLKCONFIG: \t0x%08x\n", *pwmss1_clkconfig);
    printf("  CLKSTATUS: \t0x%08x\n", *pwmss1_clkstatus);
    printf(" ePWM1 registers:\n");
    printf("  TBCTL: \t0x%04x\n", *epwm1_tbctl);
    printf("  TBCNT: \t0x%04x\n", *epwm1_tbcnt);
    printf("  TBPRD: \t0x%04x\n", *epwm1_tbprd);
    printf("  CMPA:  \t0x%04x\n", *epwm1_cmpa);
    printf("  CMPB:  \t0x%04x\n", *epwm1_cmpb);
    printf("  AQCTLA:\t0x%04x\n", *epwm1_aqctla);
    printf("  AQCTLB:\t0x%04x\n", *epwm1_aqctlb);

    // unmap memory
    if (cm_per_mem != NULL) {
        munmap((void *)cm_per_mem, CM_PER_SIZE);
        cm_per_mem = NULL;
    }
    if (control_mem != NULL) {
        munmap((void *)control_mem, CONTROL_SIZE);
        control_mem = NULL;
    }
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
}
