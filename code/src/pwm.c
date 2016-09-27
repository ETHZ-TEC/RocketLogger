#include "pwm.h"

// TODO: include other PWM functions

int mem_fd;

volatile uint16_t *pwm_regs;

// map pwm registers into user space
int pwm_setup() {
	
	// TODO
	
}

// setup range clock pwm module
int range_clock_setup(int sample_rate) {
	
	int period = PERIOD_SCALE / sample_rate;
	
	
    // open /dev/mem for memory mapping
    if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
            printf("can't open /dev/mem \n");
            return -1;
    }
	
	
	// map pwm1 registers into virtual memory
	pwm_regs = (volatile uint16_t *)mmap (NULL, PWM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, PWMSS1_BASE);
	if (pwm_regs == (volatile uint16_t *)MAP_FAILED) {
		perror ("mmap failed");
		close (mem_fd);
		return -1;
	}
	
	
	// setup ehrpwm
	pwm_regs[TBCTL] = TBCTL_DEFAULT | UP_DOWN_COUNT | PRESCALE2; // set clock mode
	
	pwm_regs[TBPRD] = period; // set clock period
	
	pwm_regs[CMPA] = (1 - PULSE_WIDTH/2) * period; // set compare registers for both output signals
	pwm_regs[CMPB] = PULSE_WIDTH/2 * period;
	
	pwm_regs[AQCTLA] = AQ_A; // set actions
	pwm_regs[AQCTLB] = AQ_B;
	
	
	// unmap memory
	munmap((void *)pwm_regs, PWM_SIZE);
	close (mem_fd);
	
	return 1;
	
}

int main(int argc, char **argv)
{
	int sample_rate = 2;
	
    range_clock_setup(sample_rate);

    return 0;
}