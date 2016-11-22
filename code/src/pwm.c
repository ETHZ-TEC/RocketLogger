#include "pwm.h"

/// Physical memory file descriptor
int mem_fd;
/// Pointer to PWMSS0 (PWM-Sub-System) registers
volatile uint16_t *pwmss0_regs;
/// Pointer to PWMSS1 (PWM-Sub-System) registers
volatile uint16_t *pwmss1_regs;

/**
 * Map PWM registers into user space (on {@link pwmss0_regs} and {@link pwmss1_regs} pointer).
 * @return {@link SUCCESS} in case of success, {@link FAILURE} otherwise.
 */
int pwm_setup() {
	
	// open /dev/mem for memory mapping
    if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
		rl_log(ERROR, "can't open /dev/mem");
		return FAILURE;
    }
	
	// map pwm0 registers into virtual memory
	pwmss0_regs = (volatile uint16_t *)mmap (NULL, PWM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, PWMSS0_BASE);
	if (pwmss1_regs == (volatile uint16_t *)MAP_FAILED) {
		rl_log(ERROR, "mmap failed");
		close (mem_fd);
		return FAILURE;
	}
	
	// map pwm1 registers into virtual memory
	pwmss1_regs = (volatile uint16_t *)mmap (NULL, PWM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, PWMSS1_BASE);
	if (pwmss1_regs == (volatile uint16_t *)MAP_FAILED) {
		rl_log(ERROR, "mmap failed");
		close (mem_fd);
		return FAILURE;
	}
	
	return SUCCESS;
}

/**
 * Unmap PWM registers from user space.
 */
void pwm_close() {
	
	// unmap memory
	munmap((void *)pwmss0_regs, PWM_SIZE);
	munmap((void *)pwmss1_regs, PWM_SIZE);
	
	// close /dev/mem
	close (mem_fd);
	
}


/**
 * Setup PWMSS1 for range latch reset clock.
 * @param sample_rate ADC sampling rate in kSps.
 */
void range_clock_setup(int sample_rate) {
	
	if(sample_rate < 1000) {
		sample_rate = 1000;
	}

	int period = 1000*PERIOD_SCALE / sample_rate;
	
	// setup ehrpwm
	pwmss1_regs[TBCTL] = TBCTL_DEFAULT | UP_DOWN_COUNT | PRESCALE2; // set clock mode
	
	pwmss1_regs[TBPRD] = period; // set clock period
	
	pwmss1_regs[CMPA] = (1 - PULSE_WIDTH/2) * period; // set compare registers for both output signals
	pwmss1_regs[CMPB] = PULSE_WIDTH/2 * period;
	
	pwmss1_regs[AQCTLA] = RWC_AQ_A; // set actions
	pwmss1_regs[AQCTLB] = RWC_AQ_B;
	
}

/**
 * Setup PWMSS0 for ADC master clock.
 */
void adc_clock_setup() {
	
	// setup ehrpwm
	pwmss0_regs[TBCTL] = TBCTL_DEFAULT;
	pwmss0_regs[TBPRD] = ADC_CLOCK_PERIOD;
	pwmss0_regs[CMPA] =  ADC_CLOCK_PERIOD/2; // 50% duty
	pwmss0_regs[AQCTLA] = ADC_AQ;
	
}
