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
	gpio_set_value(FHR1_GPIO, conf->force_high_channels[0]);
	gpio_set_value(FHR2_GPIO, conf->force_high_channels[1]);
	// leds
	gpio_export(LED_STATUS_GPIO);
	gpio_export(LED_ERROR_GPIO);
	gpio_dir(LED_STATUS_GPIO, OUT);
	gpio_dir(LED_ERROR_GPIO, OUT);
	gpio_set_value(LED_STATUS_GPIO, 1);
	gpio_set_value(LED_ERROR_GPIO, 0);
	
	// PRU TODO
}

void hw_close() {
	
	// PWM
	pwm_close();
	
	// GPIO
	// force high range
	gpio_unexport(FHR1_GPIO);
	gpio_unexport(FHR2_GPIO);
	// leds (not unexport!)
	gpio_set_value(LED_STATUS_GPIO, 0);
	
	// PRU TODO
}

int hw_sample(struct rl_conf* conf) {
	
	// set update rate (remove?!)
	if ((conf->update_rate != 1) && (conf->update_rate != 2) && (conf->update_rate != 5) && (conf->update_rate != 10)) {
		printf("Wrong update rate.\n");
			return -1;
	}
	
	// open data file
	FILE* data = (FILE*) -1;
	if (conf->file_format != NO_FILE) { // open file only if storing requested
		data = fopen(conf->file_name, "w+");
		if(data == NULL) {
			printf("Error: Error opening data-file");
			return -1;
		}
		// change access rights to data file
		char cmd[50];
		sprintf(cmd, "chmod 777 %s", conf->file_name);
		system(cmd);
	}
	
	// read calibration
	if(read_calibration() > 0) {
		// TODO
	}
	
	// SAMPLE
	if(pru_sample(data, conf) < 0) {
		// error ocurred
		gpio_set_value(LED_ERROR_GPIO, 1);
	}
	
	// close data file
	if (conf->file_format != NO_FILE) {
		fclose(data);
	}
	
	return 1;
	
}