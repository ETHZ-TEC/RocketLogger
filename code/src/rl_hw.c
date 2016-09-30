#include "rl_hw.h"

// init all hardware modules
int hw_init(struct rl_conf_new* conf) {
	
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

int hw_close() {
	
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