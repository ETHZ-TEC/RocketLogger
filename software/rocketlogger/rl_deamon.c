/**
 * Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group
 */

#include "gpio.h"

/// Linux GPIO number of start/stop button
#define GPIO_BUTTON 26
/// Minimal time interval between two interrupts (in seconds)
#define MIN_INTERVAL 1

/**
 * Setup button GPIO
 * @return {@link SUCCESS} on success, {@link FAILURE} otherwise
 */
int gpio_setup(void) {

    int ret1 = gpio_export(GPIO_BUTTON);
    int ret2 = gpio_dir(GPIO_BUTTON, IN);
    int ret3 = gpio_interrupt(GPIO_BUTTON, FALLING);

    if (ret1 == FAILURE || ret2 == FAILURE || ret3 == FAILURE) {
        return FAILURE;
    }

    return SUCCESS;
}

/**
 * GPIO interrupt handler
 * @param value GPIO value after interrupt
 */
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
        sleep(MIN_INTERVAL);
    }
}

/**
 * RocketLogger deamon program. Continuously waits on interrupt on button GPIO
 * and starts/stops RocketLogger
 *
 * Arguments: none
 * @return standard Linux return codes
 */
int main(void) {

    int timeout = -1; // infinite timeout
    if (gpio_setup() == FAILURE) {
        exit(EXIT_FAILURE);
    }

    while (1) {
        int val = gpio_wait_interrupt(GPIO_BUTTON, timeout);
        interrupt_handler(val);
    }

    exit(EXIT_SUCCESS);
}
