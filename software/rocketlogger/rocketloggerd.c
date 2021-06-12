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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <signal.h>
#include <sys/reboot.h>
#include <unistd.h>

#include "gpio.h"
#include "log.h"
#include "pwm.h"
#include "rl.h"

/// Minimal time interval between two interrupts (in seconds)
#define RL_DAEMON_MIN_INTERVAL 1

/// Delay on cape power up (in microseconds)
#define RL_POWERUP_DELAY_US 5000

/// Min duration of long button press (in seconds)
#define RL_BUTTON_LONG_PRESS 3

/// Min duration of very long button press (in seconds)
#define RL_BUTTON_VERY_LONG_PRESS 10

/// RocketLogger daemon log file.
static char const *const log_filename = RL_DAEMON_LOG_FILE;

/// Flag to terminate the infinite daemon loop
volatile bool daemon_shutdown = false;

/// Flag to shutdown the system when exiting the daemon
volatile bool system_reboot = false;

/// GPIO handle for power switch
gpio_t *gpio_power = NULL;

/// GPIO handle for user button
gpio_t *gpio_button = NULL;

/**
 * GPIO interrupt handler
 *
 * @param value GPIO value after interrupt
 */
void button_interrupt_handler(int value) {
    static time_t timestamp_down = -1;
    time_t timestamp = time(NULL);

    // capture timestamp on button press
    if (value == 0) {
        timestamp_down = timestamp;
    }
    // process button action on button release
    if (value == 1) {
        // get duration and reset timestamp down
        time_t duration = timestamp - timestamp_down;
        timestamp_down = -1;

        // skip further processing on invalid timestamp
        if (duration > timestamp) {
            return;
        }

        // get RocketLogger status
        rl_status_t status;
        int ret = rl_status_read(&status);
        if (ret < 0) {
            rl_log(RL_LOG_ERROR, "Failed reading status.");
        }

        if (duration >= RL_BUTTON_VERY_LONG_PRESS) {
            rl_log(RL_LOG_INFO,
                   "Registered very long press, requesting system shutdown.");
            daemon_shutdown = true;
            system_reboot = true;
            if (!status.sampling) {
                return;
            }
        } else if (duration >= RL_BUTTON_LONG_PRESS) {
            rl_log(RL_LOG_INFO,
                   "Registered long press, requesting daemon shutdown.");
            daemon_shutdown = true;
            if (!status.sampling) {
                return;
            }
        }

        char *cmd[3] = {"rocketlogger", NULL, NULL};
        if (status.sampling) {
            cmd[1] = "stop";
        } else {
            cmd[1] = "start";
        }

        // create child process to start RocketLogger
        pid_t pid = fork();
        if (pid < 0) {
            rl_log(RL_LOG_ERROR, "Failed forking process. %d message: %s",
                   errno, strerror(errno));
        }
        if (pid == 0) {
            // in child process, execute RocketLogger
            execvp(cmd[0], cmd);
            rl_log(RL_LOG_ERROR,
                   "Failed executing `rocketlogger %s`. %d message: %s", cmd,
                   errno, strerror(errno));
        } else {
            // in parent process log pid
            rl_log(RL_LOG_INFO, "Started RocketLogger with pid=%d.", pid);
        }
    }
    // interrupt rate control
    sleep(RL_DAEMON_MIN_INTERVAL);
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
        rl_log(RL_LOG_INFO, "Received SIGTERM, shutting down daemon.");
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
    rl_log_init(log_filename, RL_LOG_VERBOSE);

    // set effective user ID of the process
    ret = setuid(0);
    if (ret < 0) {
        rl_log(RL_LOG_ERROR, "Failed setting effective user ID. %d message: %s",
               errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    rl_log(RL_LOG_VERBOSE, "running with real user ID: %d", getuid());
    rl_log(RL_LOG_VERBOSE, "running with effective user ID: %d", geteuid());
    rl_log(RL_LOG_VERBOSE, "running with real group ID: %d", getgid());
    rl_log(RL_LOG_VERBOSE, "running with effective group ID: %d", getegid());

    // initialize GPIO module
    gpio_init();

    // hardware initialization
    gpio_t *gpio_power = gpio_setup(GPIO_POWER, GPIO_MODE_OUT, "rocketloggerd");
    if (gpio_power == NULL) {
        rl_log(RL_LOG_ERROR, "Failed configuring power switch.");
        exit(EXIT_FAILURE);
    }
    ret = gpio_set_value(gpio_power, 1);
    if (ret < 0) {
        rl_log(RL_LOG_ERROR, "Failed powering up cape.");
        exit(EXIT_FAILURE);
    }

    // wait for converter soft start (> 1 ms)
    usleep(RL_POWERUP_DELAY_US);

    gpio_t *gpio_button =
        gpio_setup_interrupt(GPIO_BUTTON, GPIO_INTERRUPT_BOTH, "rocketloggerd");
    if (gpio_button == NULL) {
        rl_log(RL_LOG_ERROR, "Failed configuring button.");
        exit(EXIT_FAILURE);
    }

    ret = pwm_init();
    if (ret < 0) {
        rl_log(RL_LOG_ERROR, "Failed initializing PWM modules.");
        exit(EXIT_FAILURE);
    }

    // create shared memory for state
    ret = rl_status_shm_init();
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
        int value = gpio_wait_interrupt(gpio_button, NULL);
        button_interrupt_handler(value);
    }

    rl_log(RL_LOG_INFO, "RocketLogger daemon stopped.");

    // remove shared memory for state
    rl_status_shm_deinit();

    // deinitialize and shutdown hardware
    pwm_deinit();
    gpio_set_value(gpio_power, 0);
    gpio_release(gpio_power);
    gpio_release(gpio_button);
    gpio_deinit();

    // reboot system if requested
    if (system_reboot) {
        rl_log(RL_LOG_INFO, "Rebooting system.");
        sync();
        reboot(RB_AUTOBOOT);
    }

    exit(EXIT_SUCCESS);
}
