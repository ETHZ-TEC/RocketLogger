#include "rl_lib.h"

#define GPIO_BUTTON 26
#define MIN_INTERVAl 1 // minimal interval between 2 interrupts (in s)

int gpio_setup() {
	
	gpio_export(GPIO_BUTTON);
	gpio_dir(GPIO_BUTTON, IN);
	gpio_interrupt(GPIO_BUTTON, RISING);
	
	return 1;
}

void interrupt_handler(int value) {
	
	if (value == 1) { // only react if button pressed enough long
		
		// get RL status (without print)
		int status = rl_get_status(0,0);
		
		if (status == 1) {
			system("sudo rocketlogger stop > /dev/null");
		} else {
			system("sudo rocketlogger continuous > /dev/null");
		}
		
		// debouncing
		sleep(MIN_INTERVAl);
		
	}
	
}


int main(int argc, char **argv) {
	
	int timeout = -1; //infinite timeout
	gpio_setup();
	
	while(1) {
		int val = gpio_wait_interrupt(GPIO_BUTTON, timeout);
		interrupt_handler(val);
	}
	
	return 1;
}

