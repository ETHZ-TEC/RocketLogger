#include "gpio.h"

#define GPIO_BUTTON 26

int gpio_setup() {
	
	gpio_export(GPIO_BUTTON);
	gpio_dir(GPIO_BUTTON, IN);
	gpio_interrupt(GPIO_BUTTON, RISING);
	
	return 1;
}

int interrupt_handler(int value) {
	
	if (value == 1) {
		printf("Interrupt! Value = %d\n",value); // TODO: remove
		
		int rl_status = 1; // TODO: replace with function
		
		if (rl_status == 1) {
			system("rocketlogger stop");
		} else {
			system("rocketlogger sample 10");
		}
	} else {
		printf("Value %d received!\n",value);
	}
	
	return 1;
	
}


int main(int argc, char **argv) {
	
	gpio_setup();
	
	while(1) {
		int val = gpio_wait_interrupt(GPIO_BUTTON);
		interrupt_handler(val);
	}
	
	return 1;
}

