#include "gpio.h"

#define GPIO_BUTTON 26
#define MIN_INTERVAl 1 // minimal interval between 2 interrupts (in s)

int gpio_setup() {
	
	gpio_export(GPIO_BUTTON);
	gpio_dir(GPIO_BUTTON, IN);
	gpio_interrupt(GPIO_BUTTON, FALLING);
	
	return SUCCESS; // TODO: add possibility to fail
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
	gpio_setup();
	
	while(1) {
		int val = gpio_wait_interrupt(GPIO_BUTTON, timeout);
		interrupt_handler(val);
	}
	
	exit(EXIT_SUCCESS);
}

