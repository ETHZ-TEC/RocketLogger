/**
 * Copyright (c) 2016-2019, Swiss Federal Institute of Technology (ETH Zurich)
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
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>

#define LOG_FILE "/var/www/log/deamon.log"

#include "gpio.h"
#include "log.h"

/// Linux GPIO number of start/stop button
#define GPIO_BUTTON 26

/// Linux GPIO number of start/stop button
#define GPIO_POWER 31

/// Minimal time interval between two interrupts (in seconds)
#define MIN_INTERVAL 1

/**
 * Setup power GPIO and power of RocketLogger cape
 * 
 * @return {@link SUCCESS} on success, {@link FAILURE} otherwise
 */
int power_init(void) {
    int ret1 = gpio_export(GPIO_POWER);
    int ret2 = gpio_dir(GPIO_POWER, OUT);
    int ret3 = gpio_set_value(GPIO_POWER, 1);

    if (ret1 == FAILURE || ret2 == FAILURE || ret3 == FAILURE) {
        return FAILURE;
    }
    return SUCCESS;
}

/**
 * Deinitialize power GPIO
 * 
 * @return {@link SUCCESS} on success, {@link FAILURE} otherwise
 */
int power_deinit(void) {
    int ret1 = gpio_set_value(GPIO_POWER, 0);
    int ret2 = gpio_unexport(GPIO_POWER);

    if (ret1 == FAILURE || ret2 == FAILURE) {
        return FAILURE;
    }
    return SUCCESS;
}

/**
 * Setup button GPIO and enable interrup
 * 
 * @return {@link SUCCESS} on success, {@link FAILURE} otherwise
 */
int button_init(void) {
    int ret1 = gpio_export(GPIO_BUTTON);
    int ret2 = gpio_dir(GPIO_BUTTON, IN);
    int ret3 = gpio_interrupt(GPIO_BUTTON, FALLING);

    if (ret1 == FAILURE || ret2 == FAILURE || ret3 == FAILURE) {
        return FAILURE;
    }
    return SUCCESS;
}

/**
 * Deinitialize button GPIO
 * 
 * @return {@link SUCCESS} on success, {@link FAILURE} otherwise
 */
int button_deinit(void) {
    int ret1 = gpio_interrupt(GPIO_BUTTON, NONE);
    int ret2 = gpio_unexport(GPIO_BUTTON);

    if (ret1 == FAILURE || ret2 == FAILURE) {
        return FAILURE;
    }
    return SUCCESS;
}

/**
 * GPIO interrupt handler
 * 
 * @param value GPIO value after interrupt
 */
void button_interrupt_handler(int value) {
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
    // reset GPIO configuration
    gpio_unexport(GPIO_POWER);
    gpio_unexport(GPIO_BUTTON);
    sleep(1);

    if (power_init() == FAILURE) {
        rl_log(ERROR, "Failed powering up cape.");
        exit(EXIT_FAILURE);
    }
    if (button_init() == FAILURE) {
        rl_log(ERROR, "Failed configuring button.");
        exit(EXIT_FAILURE);
    }

    rl_log(INFO, "RocketLogger daemon started.");

    while (1) {
        // wait for interrupt with indefinite timeout (-1)
        int val = gpio_wait_interrupt(GPIO_BUTTON, -1);
        button_interrupt_handler(val);
    }

    rl_log(INFO, "RocketLogger daemon stopping...");

    button_deinit();
    power_deinit();

    exit(EXIT_SUCCESS);
}
