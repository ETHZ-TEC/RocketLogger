#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <unistd.h>
#include <errno.h>
#include <stdint.h>

#include "types.h"
#include "log.h"

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
#define TBCTL 	(EPWM_OFFSET+0x0) / sizeof (uint16_t)	// counter control
/// Period register offset
#define TBPRD	(EPWM_OFFSET+0xA) / sizeof (uint16_t)	// period
/// Compare register A offset
#define CMPA	(EPWM_OFFSET+0x12) / sizeof (uint16_t)	// compare
/// Compare register B offset
#define CMPB	(EPWM_OFFSET+0x14) / sizeof (uint16_t)	
/// Action qualifier register offset
#define AQCTLA	(EPWM_OFFSET+0x16) / sizeof (uint16_t)	// action qualifier
/// Action qualifier register B offset
#define AQCTLB	(EPWM_OFFSET+0x18) / sizeof (uint16_t)	

// register values
/// Default counter value (see AM335x_TR)
#define TBCTL_DEFAULT	0xC000
/// Up-down counting
#define UP_DOWN_COUNT	0x0002
/// Counter prescale 2
#define PRESCALE2		0x0400


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
#define PWM_PERIOD_SCALE 50000000 * (1 + PULSE_WIDTH + MARGIN) // period scaling factor (period is set in 5ns, (/2 clock prescaling))


// ADC clock settings
/// ADC master clock period in ns
#define ADC_CLOCK_PERIOD	48		// in 10ns
/// Action qualifier value for ADC clock (see AM335x_TR)
#define ADC_AQ				0x0025	// clear on zero and period, set at 50%

int pwm_setup(void);
void pwm_close(void);

void range_clock_setup(int sample_rate); // sampling rate [Hz]
void adc_clock_setup(void);
