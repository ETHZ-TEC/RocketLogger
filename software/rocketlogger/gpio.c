/**
 * Copyright (c) 2016-2020, ETH Zurich, Computer Engineering Group
 * Copyright (c) 2021, Lukas Sigrist <lsigrist@mailbox.org>
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
#include <stdint.h>
#include <string.h>

#include <gpiod.h>
#include <time.h>
#include <unistd.h>

#include "log.h"
#include "rl.h"

#include "gpio.h"

/// Number of GPIO chips
#define GPIO_CHIP_COUNT 4

/// Number of GPIO lines per GPIO chip
#define GPIO_LINES_PER_CHIP 32

/// Minimal time a button needs to be pressed (in microseconds)
#define GPIO_DEBOUNCE_DELAY_US 50

struct gpiod_chip *gpio_chip[GPIO_CHIP_COUNT] = {NULL};

/**
 * Get GPIO line for GPIO resource number.
 *
 * @param gpio_number GPIO resource number
 * @return GPIO resource on success, NULL on failure with errno set accordingly
 */
static gpio_t *gpio_get_line(int gpio_number) {
    int gpio_chip_number = gpio_number / GPIO_LINES_PER_CHIP;
    int gpio_line_index = gpio_number % GPIO_LINES_PER_CHIP;

    // check gpio chip initialized
    if (gpio_chip[gpio_chip_number] == NULL) {
        rl_log(RL_LOG_ERROR, "GPIO chip for GPIO%d uninitialized.\n",
               gpio_number);
        return NULL;
    }

    // get and return gpio line
    return gpiod_chip_get_line(gpio_chip[gpio_chip_number], gpio_line_index);
}

int gpio_init() {
    // open gpio chips
    for (int i = 0; i < GPIO_CHIP_COUNT; i++) {
        gpio_chip[i] = gpiod_chip_open_by_number(i);
        if (gpio_chip[i] == NULL) {
            rl_log(RL_LOG_ERROR,
                   "Failed opening GPIO chip %d. %d message: %s\n", i, errno,
                   strerror(errno));
            return ERROR;
        }
    }

    return SUCCESS;
}

void gpio_deinit() {
    // open gpio chips
    for (int i = 0; i < GPIO_CHIP_COUNT; i++) {
        if (gpio_chip[i] == NULL) {
            continue;
        }
        gpiod_chip_close(gpio_chip[i]);
        gpio_chip[i] = NULL;
    }
}

gpio_t *gpio_setup(int gpio_number, gpio_mode_t mode, const char *name) {
    // get gpio line
    gpio_t *gpio_line = gpio_get_line(gpio_number);
    if (gpio_line == NULL) {
        rl_log(RL_LOG_ERROR,
               "Failed to get GPIO line for GPIO %s. %d message: %s\n", name,
               errno, strerror(errno));
        return NULL;
    }

    // configure GPIO direction
    int ret = -1;
    if (mode == GPIO_MODE_OUT) {
        ret = gpiod_line_request_output(gpio_line, name, 0);
    } else if (mode == GPIO_MODE_IN) {
        ret = gpiod_line_request_input(gpio_line, name);
    }
    if (ret < 0) {
        rl_log(RL_LOG_ERROR,
               "Failed to configure for GPIO %s. %d message: %s\n", name, errno,
               strerror(errno));
        return NULL;
    }

    return gpio_line;
}

void gpio_release(gpio_t *gpio) {
    gpiod_line_release(gpio);
    gpio = NULL;
}

int gpio_set_value(gpio_t *gpio, int value) {
    // validate output value to set
    if (value < 0 || value > 1) {
        errno = EINVAL;
        return ERROR;
    }

    // set gpio value and return result
    return gpiod_line_set_value(gpio, value);
}

int gpio_get_value(gpio_t *gpio) { return gpiod_line_get_value(gpio); }

gpio_t *gpio_setup_interrupt(int gpio_number, gpio_interrupt_t edge,
                             const char *name) {
    // get gpio line
    gpio_t *gpio_line = gpio_get_line(gpio_number);
    if (gpio_line == NULL) {
        rl_log(RL_LOG_ERROR,
               "Failed to get GPIO line for GPIO %s. %d message: %s\n", name,
               errno, strerror(errno));
        return NULL;
    }

    // request interrupt events from gpio line
    int ret = 0;
    switch (edge) {
    case GPIO_INTERRUPT_NONE:
        break;
    case GPIO_INTERRUPT_RISING:
        ret = gpiod_line_request_rising_edge_events(gpio_line, name);
        break;
    case GPIO_INTERRUPT_FALLING:
        ret = gpiod_line_request_falling_edge_events(gpio_line, name);
        break;
    case GPIO_INTERRUPT_BOTH:
        ret = gpiod_line_request_both_edges_events(gpio_line, name);
        break;
    default:
        rl_log(RL_LOG_ERROR, "Invalid GPIO interrupt edge configuration");
        return NULL;
    }
    if (ret < 0) {
        rl_log(RL_LOG_ERROR,
               "Failed to configure interrupt for GPIO %s. %d message: %s\n",
               name, errno, strerror(errno));
        return NULL;
    }

    return gpio_line;
}

int gpio_wait_interrupt(gpio_t *gpio, const struct timespec *timeout) {
    int ret = -1;
    struct gpiod_line_event event;

    // wait for registerd events
    ret = gpiod_line_event_wait(gpio, timeout);
    if (ret < 0) {
        rl_log(RL_LOG_ERROR,
               "Failed waiting for interrupt of GPIO %s. %d message: %s\n",
               gpiod_line_consumer(gpio), errno, strerror(errno));
        return ret;
    }

    // small debounce delay
    usleep(GPIO_DEBOUNCE_DELAY_US);

    // get event data
    ret = gpiod_line_event_read(gpio, &event);
    if (ret < 0) {
        rl_log(RL_LOG_ERROR,
               "Failed reading event for GPIO %s. %d message: %s\n",
               gpiod_line_consumer(gpio), errno, strerror(errno));
        return ret;
    }

    // decode and return resulting gpio input value
    if (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE) {
        return 1;
    }
    if (event.event_type == GPIOD_LINE_EVENT_FALLING_EDGE) {
        return 0;
    }

    // treat neither falling nor rising edge as error
    return ERROR;
}
