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

#ifndef GPIO_H_
#define GPIO_H_

#include <gpiod.h>
#include <time.h>

/// GPIO number for forcing I1 high
#define GPIO_FHR1 30
/// GPIO number for forcing I2 high
#define GPIO_FHR2 60
/// GPIO number of status LED
#define GPIO_LED_STATUS 45
/// GPIO number of error LED
#define GPIO_LED_ERROR 44
/// GPIO number of start/stop button
#define GPIO_BUTTON 26
/// GPIO number of RocketLogger cape power enable
#define GPIO_POWER 31

/**
 * GPIO direction definition
 */
typedef enum gpio_mode {
    GPIO_MODE_IN, //!< GPIO read mode
    GPIO_MODE_OUT //!< GPIO write mode
} gpio_mode_t;

/**
 * GPIO interrupt edge definition
 */
typedef enum gpio_interrupt {
    GPIO_INTERRUPT_NONE,    //!< No interrupt
    GPIO_INTERRUPT_RISING,  //!< Interrupt on rising edge
    GPIO_INTERRUPT_FALLING, //!< Interrupt on falling edge
    GPIO_INTERRUPT_BOTH,    //!< Interrupt on both edges
} gpio_interrupt_t;

/**
 * GPIO type wrapper
 */
typedef struct gpiod_line gpio_t;

/**
 * Initialize GPIO module.
 *
 * @return Returns 0 on success, negative on failure with errno set accordingly
 */
int gpio_init();

/**
 * Denitialize GPIO module.
 *
 * @note release any used GPIO pin first.
 */
void gpio_deinit();

/**
 * Set up a specific GPIO pin for input/output.
 *
 * @note need to initialize GPIO module first.
 *
 * @param gpio_number Resource number of the GPIO to set up
 * @param mode GPIO mode (input or output) to configure
 * @param name Name to label the set up GPIO
 * @return Returns GPIO resource on success, NULL on failure with errno set
 * accordingly
 */
gpio_t *gpio_setup(int gpio_number, gpio_mode_t mode, const char *name);

/**
 * Set up a specific GPIO pin for interrupt event.
 *
 * @param gpio_number Resource number of the GPIO to set up
 * @param edge GPIO interrupt edge to configure
 * @param name Name to label the set up GPIO
 * @return Returns GPIO resource on success, NULL on failure with errno set
 * accordingly
 */
gpio_t *gpio_setup_interrupt(int gpio_number, gpio_interrupt_t edge,
                             const char *name);

/**
 * Release an aquired GPIO resource.
 *
 * @param gpio The GPIO resource to release
 */
void gpio_release(gpio_t *gpio);

/**
 * Set the GPIO value.
 *
 * @param gpio GPIO resource to set the output value
 * @param value GPIO state to set (0 or 1)
 * @return Returns 0 on success, negative on failure with errno set accordingly
 */
int gpio_set_value(gpio_t *gpio, int value);

/**
 * Get the GPIO value.
 *
 * @param gpio GPIO resource to get the value
 * @return The read GPIO value, negative on failure with errno set accordingly
 */
int gpio_get_value(gpio_t *gpio);

/**
 * Wait for interrupt on GPIO pin.
 *
 * @param gpio_number Linux sysfs GPIO resource number
 * @param timeout Pointer to timeout timespec, NULL for infinite timeout
 * @return Returns the GPIO pin value (0 or 1) on success, negative on failure
 * with errno set accordingly
 */
int gpio_wait_interrupt(gpio_t *gpio, const struct timespec *timeout);

#endif /* GPIO_H_ */
