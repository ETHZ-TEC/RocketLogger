#include "gpio.h"

#define GPIO_BUTTON 26
#define MIN_INTERVAl 1 // minimal interval between 2 interrupts (in s)

int gpio_setup() {
	
	int ret1 = gpio_export(GPIO_BUTTON);
	int ret2 = gpio_dir(GPIO_BUTTON, IN);
	int ret3 = gpio_interrupt(GPIO_BUTTON, FALLING);
	
	if(ret1 == FAILURE || ret2 == FAILURE || ret3 == FAILURE) {
		return FAILURE;
	}

	return SUCCESS;
}

void interrupt_handler(int value) {
	
	if (value == 0) { // only react if button pressed enough long
		
		// get RL status
		int status = system("rocketlogger status > /dev/null");
		
		if (status > 0) {
			system("rocketlogger stop > /dev/null");
		} else {
			system("rocketlogger cont > /dev/null");
		}
		
		// debouncing
		sleep(MIN_INTERVAl);
		
	}
	
}


int main() {
	
	int timeout = -1; //infinite timeout
	if(gpio_setup() == FAILURE) {
		exit(EXIT_FAILURE);
	}
	
	while(1) {
		int val = gpio_wait_interrupt(GPIO_BUTTON, timeout);
		interrupt_handler(val);
	}
	
	exit(EXIT_SUCCESS);
}

