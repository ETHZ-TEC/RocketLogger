#include "pwm.h"

int mem_fd;

volatile uint16_t *pwmss0_regs;
volatile uint16_t *pwmss1_regs;

// map pwm registers into user space
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

// unmap pwm registers
void pwm_close() {
	
	// unmap memory
	munmap((void *)pwmss0_regs, PWM_SIZE);
	munmap((void *)pwmss1_regs, PWM_SIZE);
	
	// close /dev/mem
	close (mem_fd);
	
}


// setup range clock pwm module
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

// setup adc clock
void adc_clock_setup() {
	
	// setup ehrpwm
	pwmss0_regs[TBCTL] = TBCTL_DEFAULT;
	pwmss0_regs[TBPRD] = ADC_CLOCK_PERIOD;
	pwmss0_regs[CMPA] =  ADC_CLOCK_PERIOD/2; // 50% duty
	pwmss0_regs[AQCTLA] = ADC_AQ;
	
}
