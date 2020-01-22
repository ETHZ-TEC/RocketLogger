/**
 * Copyright (c) 2016-2020, ETH Zurich, Computer Engineering Group
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
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#define LOG_FILE "/var/www/rocketlogger/log/daemon.log"

#include <stdlib.h>

#include <signal.h>
#include <unistd.h>

#include "gpio.h"
#include "log.h"
#include "pwm.h"
#include "rl.h"

/// Minimal time interval between two interrupts (in seconds)
#define RL_DAEMON_MIN_INTERVAL 1

/// RocketLogger daemon log file.
static char const *const log_filename = "/var/www/rocketlogger/log/daemon.log";

/// Flag to terminate the infinite daemon loop
volatile bool daemon_shutdown = false;

/**
 * Setup power GPIO and power of RocketLogger cape.
 *
 * @return Returns 0 on success, negative on failure with errno set accordingly
 */
int power_init(void) {
    int ret = SUCCESS;
    ret = gpio_init(GPIO_POWER, GPIO_MODE_OUT);
    if (ret < 0) {
        return ret;
    }

    ret = gpio_set_value(GPIO_POWER, 1);
    if (ret < 0) {
        return ret;
    }

    return ret;
}

/**
 * Deinitialize power GPIO.
 *
 * @return Returns 0 on success, negative on failure with errno set accordingly
 */
int power_deinit(void) {
    int ret = SUCCESS;
    ret = gpio_set_value(GPIO_POWER, 0);
    if (ret < 0) {
        return ret;
    }

    ret = gpio_deinit(GPIO_POWER);
    if (ret < 0) {
        return ret;
    }

    return ret;
}

/**
 * Setup button GPIO and enable interrupt.
 *
 * @return Returns 0 on success, negative on failure with errno set accordingly
 */
int button_init(void) {
    int ret = SUCCESS;
    ret = gpio_init(GPIO_BUTTON, GPIO_MODE_IN);
    if (ret < 0) {
        return ret;
    }

    ret = gpio_interrupt(GPIO_BUTTON, GPIO_INT_FALLING);
    if (ret < 0) {
        return ret;
    }

    return ret;
}

/**
 * Deinitialize button GPIO.
 *
 * @return Returns 0 on success, negative on failure with errno set accordingly
 */
int button_deinit(void) {
    int ret = SUCCESS;
    ret = gpio_interrupt(GPIO_BUTTON, GPIO_INT_NONE);
    if (ret < 0) {
        return ret;
    }

    ret = gpio_deinit(GPIO_BUTTON);
    if (ret < 0) {
        return ret;
    }

    return ret;
}

/**
 * Setup LED GPIOs.
 *
 * @return Returns 0 on success, negative on failure with errno set accordingly
 */
int leds_init(void) {
    int ret = SUCCESS;
    ret = gpio_init(GPIO_LED_STATUS, GPIO_MODE_OUT);
    if (ret < 0) {
        return ret;
    }

    ret = gpio_init(GPIO_LED_ERROR, GPIO_MODE_OUT);
    if (ret < 0) {
        return ret;
    }

    ret = gpio_set_value(GPIO_LED_STATUS, 0);
    if (ret < 0) {
        return ret;
    }

    ret = gpio_set_value(GPIO_LED_ERROR, 0);
    if (ret < 0) {
        return ret;
    }

    return ret;
}

/**
 * Deinitialize LED GPIOs.
 *
 * @return Returns 0 on success, negative on failure with errno set accordingly
 */
int leds_deinit(void) {
    int ret = SUCCESS;
    ret = gpio_set_value(GPIO_LED_STATUS, 0);
    if (ret < 0) {
        return ret;
    }

    ret = gpio_set_value(GPIO_LED_ERROR, 0);
    if (ret < 0) {
        return ret;
    }

    ret = gpio_deinit(GPIO_LED_STATUS);
    if (ret < 0) {
        return ret;
    }

    ret = gpio_deinit(GPIO_LED_ERROR);
    if (ret < 0) {
        return ret;
    }

    return ret;
}

/**
 * Setup range forcing GPIOs.
 *
 * @return Returns 0 on success, negative on failure with errno set accordingly
 */
int fhr_init(void) {
    int ret = SUCCESS;
    ret = gpio_init(GPIO_FHR1, GPIO_MODE_OUT);
    if (ret < 0) {
        return ret;
    }

    ret = gpio_init(GPIO_FHR2, GPIO_MODE_OUT);
    if (ret < 0) {
        return ret;
    }

    ret = gpio_set_value(GPIO_FHR1, 0);
    if (ret < 0) {
        return ret;
    }

    ret = gpio_set_value(GPIO_FHR2, 0);
    if (ret < 0) {
        return ret;
    }

    return ret;
}

/**
 * Deinitialize range forcing GPIOs.
 *
 * @return Returns 0 on success, negative on failure with errno set accordingly
 */
int fhr_deinit(void) {
    int ret = SUCCESS;
    ret = gpio_set_value(GPIO_FHR1, 0);
    if (ret < 0) {
        return ret;
    }

    ret = gpio_set_value(GPIO_FHR2, 0);
    if (ret < 0) {
        return ret;
    }

    ret = gpio_deinit(GPIO_FHR1);
    if (ret < 0) {
        return ret;
    }

    ret = gpio_deinit(GPIO_FHR2);
    if (ret < 0) {
        return ret;
    }

    return ret;
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

        // interrupt rate control
        sleep(RL_DAEMON_MIN_INTERVAL);
    }
}

/**
 * Signal handler to catch interrupt signals.
 *
 * @param signal_number The number of the signal to handle
 */
static void signal_handler(int signal_number) {
    // signal to terminate the daemon (systemd stop)
    if (signal_number == SIGTERM) {
        signal(signal_number, SIG_IGN);
        daemon_shutdown = true;
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
    int ret = SUCCESS;

    // init log module
    rl_log_init(log_filename, RL_LOG_INFO);

    // reset all GPIOs to known reset state
    gpio_reset(GPIO_POWER);
    gpio_reset(GPIO_BUTTON);
    gpio_reset(GPIO_FHR1);
    gpio_reset(GPIO_FHR2);
    gpio_reset(GPIO_LED_STATUS);
    gpio_reset(GPIO_LED_ERROR);

    sleep(1);

    // hardware initialization
    ret = power_init();
    if (ret < 0) {
        rl_log(RL_LOG_ERROR, "Failed powering up cape.");
        exit(EXIT_FAILURE);
    }

    ret = button_init();
    if (ret < 0) {
        rl_log(RL_LOG_ERROR, "Failed configuring button.");
        exit(EXIT_FAILURE);
    }

    ret = leds_init();
    if (ret < 0) {
        rl_log(RL_LOG_ERROR, "Failed configuring LEDs.");
        exit(EXIT_FAILURE);
    }

    ret = fhr_init();
    if (ret < 0) {
        rl_log(RL_LOG_ERROR, "Failed configuring range forcing.");
        exit(EXIT_FAILURE);
    }

    ret = pwm_init();
    if (ret < 0) {
        rl_log(RL_LOG_ERROR, "Failed initializing PWM modules.");
        exit(EXIT_FAILURE);
    }

    // create shared memory for state
    ret = rl_status_init();
    if (ret < 0) {
        rl_log(RL_LOG_ERROR, "Failed initializing status shared memory.");
        exit(EXIT_FAILURE);
    }

    // register signal handler for SIGTERM (for stopping daemon)
    struct sigaction signal_action;
    signal_action.sa_handler = signal_handler;
    sigemptyset(&signal_action.sa_mask);
    signal_action.sa_flags = 0;

    ret = sigaction(SIGTERM, &signal_action, NULL);
    if (ret < 0) {
        rl_log(RL_LOG_ERROR, "can't register signal handler for SIGTERM.");
        exit(EXIT_FAILURE);
    }

    rl_log(RL_LOG_INFO, "RocketLogger daemon started.");

    daemon_shutdown = false;
    while (!daemon_shutdown) {
        // wait for interrupt with infinite timeout
        int value = gpio_wait_interrupt(GPIO_BUTTON, GPIO_INT_TIMEOUT_INF);
        button_interrupt_handler(value);
    }

    rl_log(RL_LOG_INFO, "RocketLogger daemon stopped.");

    // remove shared memory for state
    rl_status_deinit();

    // deinitialize and shutdown hardware
    button_deinit();
    power_deinit();
    leds_deinit();
    fhr_deinit();
    pwm_deinit();

    exit(EXIT_SUCCESS);
}
