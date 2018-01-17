/**
 * Copyright (c) 2016-2018, Swiss Federal Institute of Technology (ETH Zurich)
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * 
 * * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
