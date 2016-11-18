#define _FILE_OFFSET_BITS 64

#include "rl_hw.h"

// init all hardware modules
void hw_init(struct rl_conf* conf) {
	
	// PWM
	pwm_setup();
	range_clock_setup(conf->sample_rate);
	adc_clock_setup();
	
	// GPIO
	// force high range
	gpio_export(FHR1_GPIO);
	gpio_export(FHR2_GPIO);
	gpio_dir(FHR1_GPIO, OUT);
	gpio_dir(FHR2_GPIO, OUT);
	gpio_set_value(FHR1_GPIO, (conf->force_high_channels[0] == 0));
	gpio_set_value(FHR2_GPIO, (conf->force_high_channels[1] == 0));
	// leds
	gpio_export(LED_STATUS_GPIO);
	gpio_export(LED_ERROR_GPIO);
	gpio_dir(LED_STATUS_GPIO, OUT);
	gpio_dir(LED_ERROR_GPIO, OUT);
	gpio_set_value(LED_STATUS_GPIO, 1);
	gpio_set_value(LED_ERROR_GPIO, 0);
	
	// PRU
	pru_init();
}

void hw_close(struct rl_conf* conf) {
	
	// PWM
	pwm_close();
	
	// GPIO
	// force high range
	gpio_unexport(FHR1_GPIO);
	gpio_unexport(FHR2_GPIO);
	// leds (not unexport!)
	gpio_set_value(LED_STATUS_GPIO, 0);
	
	// PRU
	if(conf->mode != LIMIT) {
		pru_stop();
	}
	pru_close();
	
	// RESET SHARED MEM
	status.samples_taken = 0;
	status.buffer_number = 0;
	write_status(&status);
}

int hw_sample(struct rl_conf* conf) {
	
	// open data file
	FILE* data = (FILE*) -1;
	if (conf->file_format != NO_FILE) { // open file only if storing requested
		data = fopen(conf->file_name, "w+");
		if(data == NULL) {
			rl_log(ERROR, "failed to open data-file");
			return FAILURE;
		}
	}
	
	// read calibration
	if(conf->calibration == CAL_USE) {
		read_calibration(conf);
	} else {
		reset_offsets();
		reset_scales();
	}
	
	// SAMPLE
	if(pru_sample(data, conf) == FAILURE) {
		// error ocurred
		gpio_set_value(LED_ERROR_GPIO, 1);
	}
	
	// close data file
	if (conf->file_format != NO_FILE) {
		fclose(data);
	}
	
	return SUCCESS;
	
}
