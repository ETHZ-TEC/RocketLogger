#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <unistd.h>
#include <errno.h>
#include <stdint.h>

// base addresses
#define PWMSS2_BASE 0x48304000
#define PWMSS1_BASE 0x48302000
#define PWMSS0_BASE 0x48300000
#define EPWM_OFFSET 0x0200

// pwm size
#define PWM_SIZE 0x00000FFF

// configuration registers
#define TBCTL 0x0	// counter control
#define TBPRD 0xA	// period
#define CMPA 0x12	// compare
#define CMPB 0x14	
#define AQCTLA 0x16	// action qualifier
#define AQCTLB 0x18	

// register values
#define TBCTL_DEFAULT	0xC000
#define UP_DOWN_COUNT	0x0002
#define PRESCALE2		0x0400

#define AQ_A 0x0060
#define AQ_B 0x0900


// pulse configuration
#define PULSE_WIDTH 0.1 // 10% of sampling period
#define MARGIN 0.1
#define PERIOD_SCALE 50000 * (1 + PULSE_WIDTH + MARGIN) // period scaling factor (period is set in 5ns, /2 prescaling)


int mem_fd;

volatile uint16_t *pwm_regs;

int range_clock_setup(int sample_rate) { // sampling rate [kHz]
	
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
	
	// set clock mode
	pwm_regs[(EPWM_OFFSET+TBCTL) / sizeof (uint16_t)] = TBCTL_DEFAULT | UP_DOWN_COUNT | PRESCALE2;
	//printf("TBCTL: %04x\n",pwm_regs[(EPWM_OFFSET+TBCTL) / sizeof (uint16_t)]);
	
	// set clock period
	pwm_regs[(EPWM_OFFSET+TBPRD) / sizeof (uint16_t)] = period;
	//printf("TBPRD: %d\n",pwm_regs[(EPWM_OFFSET+TBPRD) / sizeof (uint16_t)]);

	// set compare registers for both output signals
	pwm_regs[(EPWM_OFFSET+CMPA) / sizeof (uint16_t)] = (1 - PULSE_WIDTH/2) * period;
	//printf("CMPA: %d\n",pwm_regs[(EPWM_OFFSET+CMPA) / sizeof (uint16_t)]);
	
	pwm_regs[(EPWM_OFFSET+CMPB) / sizeof (uint16_t)] = PULSE_WIDTH/2 * period;
	//printf("CMPB: %d\n",pwm_regs[(EPWM_OFFSET+CMPB) / sizeof (uint16_t)]);
	
	// set actions
	pwm_regs[(EPWM_OFFSET+AQCTLA) / sizeof (uint16_t)] = AQ_A;
	//printf("AQCTLA: %04x\n",pwm_regs[(EPWM_OFFSET+AQCTLA) / sizeof (uint16_t)]);
	
	pwm_regs[(EPWM_OFFSET+AQCTLB) / sizeof (uint16_t)] = AQ_B;
	//printf("AQCTLB: %04x\n",pwm_regs[(EPWM_OFFSET+AQCTLB) / sizeof (uint16_t)]);
	
	
	munmap ((void *)pwm_regs, PWM_SIZE);
	close (mem_fd);
	
	return 1;
	
}

int main(int argc, char **argv)
{
	int sample_rate = 1;
	
    range_clock_setup(sample_rate);

    return 0;
}