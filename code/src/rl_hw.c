#include "rl_hw.h"

// init all hardware modules
void hw_init(struct rl_conf_new* conf) {
	
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

int hw_sample(struct rl_conf_new* confn) {
	
	/*int meter = 0;
	if (conf->state == RL_METER) { // remove??
		meter = 1;
	}*/
	
	/*int rate;
	// set rate
	switch (conf->rate) {
		case 1:
			rate = K1;
			break;
		case 2:
			rate = K2;
			break;
		case 4:
			rate = K4;
			break;
		case 8:
			rate = K8;
			break;
		case 16:
			rate = K16;
			break;
		case 32:
			rate = K32;
			break;
		case 64:
			rate = K64;
			break;
		default:
			printf("Wrong sample rate.\n");
			return -1;
	}*/
	
	// set update rate (remove?!)
	if ((confn->update_rate != 1) && (confn->update_rate != 2) && (confn->update_rate != 5) && (confn->update_rate != 10)) { // TODO: replace with confn
		printf("Wrong update rate.\n");
			return -1;
	}
	/*switch (update_rate) {
		case 1:
			update_rate = HZ1;
			break;
		case 2:
			update_rate = HZ2;
			break;
		case 5:
			update_rate = HZ5;
			break;
		case 10:
			update_rate = HZ10;
			break;
		default:
			printf("Wrong update rate.\n");
			return -1;
	}*/
	
	// open data file
	FILE* data = (FILE*) -1;
	if (confn->file_format != NO_FILE) { // open file only if storing requested
		data = fopen(confn->file_name, "w+"); // TODO: replace with confn
		if(data == NULL) {
			printf("Error: Error opening data-file");
			return -1;
		}
		// change access rights to data file
		char cmd[50];
		sprintf(cmd, "chmod 777 %s", confn->file_name); // TODO: replace with confn
		system(cmd);
	}
	
	// read calibration
	if(read_calibration() > 0) {
		// TODO
	}
	
	// TODO temporary solution
	int i;
	int MASK = 1;
	int channels = 0;
	int binary;
	int store;
	for(i=0; i<NUM_CHANNELS; i++) {
		if(confn->channels[i] > 0) {
			channels = channels | MASK;
		}
		MASK = MASK << 1;
	}
	if(confn->file_format == BIN) {
		binary = 1;
	} else {
		binary = 0;
	}
	if(confn->file_format == NO_FILE || confn->mode == METER) {
		store = 0;
	} else {
		store = 1;
	}
	//printf("Old: %d, new: %d\n",conf->binary_file,binary);
	
	// SAMPLE
	pru_sample(data, channels, store, binary, confn);
	
	// close data file
	if (confn->file_format != NO_FILE) {// TODO: replace with confn
		fclose(data);
	}
	
	return 1;
	
}