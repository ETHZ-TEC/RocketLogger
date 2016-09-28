#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <unistd.h>
#include <errno.h>
#include <stdint.h>

// base addresses
#define PWMSS0_BASE 0x48300000
#define PWMSS1_BASE 0x48302000
#define PWMSS2_BASE 0x48304000
#define EPWM_OFFSET 0x0200

// pwm size
#define PWM_SIZE 0x00000FFF

// configuration registers
#define TBCTL 	(EPWM_OFFSET+0x0) / sizeof (uint16_t)	// counter control
#define TBPRD	(EPWM_OFFSET+0xA) / sizeof (uint16_t)	// period
#define CMPA	(EPWM_OFFSET+0x12) / sizeof (uint16_t)	// compare
#define CMPB	(EPWM_OFFSET+0x14) / sizeof (uint16_t)	
#define AQCTLA	(EPWM_OFFSET+0x16) / sizeof (uint16_t)	// action qualifier
#define AQCTLB	(EPWM_OFFSET+0x18) / sizeof (uint16_t)	

// register values (see AM335x_TR)
#define TBCTL_DEFAULT	0xC000
#define UP_DOWN_COUNT	0x0002
#define PRESCALE2		0x0400


// range switch clock configuration (action qualifier)
#define RWC_AQ_A 0x0060 // set when incrementing, clear when decrementing
#define RWC_AQ_B 0x0900 // set when decrementing, clear when incrementing

// pulse configuration
#define PULSE_WIDTH 0.1 // 10% of sampling period
#define MARGIN 0.1
#define PERIOD_SCALE 50000 * (1 + PULSE_WIDTH + MARGIN) // period scaling factor (period is set in 5ns, (/2 clock prescaling))


// ADC clock settings
#define ADC_CLOCK_PERIOD	48		// in 10ns
#define ADC_AQ				0x0025	// clear on zero and period, set at 50%

int pwm_setup();
int pwm_close();

int range_clock_setup(int sample_rate); // sampling rate [kHz]
int adc_clock_setup();