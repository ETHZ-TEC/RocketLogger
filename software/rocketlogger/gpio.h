/**
 * Copyright (c) 2016-2019, ETH Zurich, Computer Engineering Group
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

/// Path to the Linux sysfs GPIO device files
#define GPIO_SYSFS_PATH "/sys/class/gpio/"
/// Minimal time a button needs to be pressed (in microseconds)
#define GPIO_DEBOUNCE_DELAY 100000
/// Infinite GPIO interrupt wait timeout
#define GPIO_INT_TIMEOUT_INF -1

/// Linux sysfs GPIO number for forcing I1 high
#define GPIO_FHR1 30
/// Linux sysfs GPIO number for forcing I2 high
#define GPIO_FHR2 60
/// Linux sysfs GPIO number of status LED
#define GPIO_LED_STATUS 45
/// Linux sysfs GPIO number of error LED
#define GPIO_LED_ERROR 44
/// Linux sysfs GPIO number of start/stop button
#define GPIO_BUTTON 26
/// Linux sysfs GPIO number of RocketLogger cape power enable
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
    GPIO_INT_NONE,    //!< No interrupt
    GPIO_INT_RISING,  //!< Interrupt on rising edge
    GPIO_INT_FALLING, //!< Interrupt on falling edge
    GPIO_INT_BOTH,    //!< Interrupt on both edges
} gpio_interrupt_t;

/**
 * Initialize a GPIO.
 *
 * Export sysfs GPIO resource and configure GPIO mode.
 *
 * @param gpio_number Linux sysfs GPIO resource number
 * @param mode GPIO mode (input or output) to configure
 * @return {@link SUCCESS} in case of success, {@link FAILURE} otherwise
 */
int gpio_init(int gpio_number, gpio_mode_t mode);

/**
 * Denitialize a GPIO.
 *
 * Unexport an exported sysfs GPIO resource.
 *
 * @param gpio_number Linux sysfs GPIO resource number
 * @return {@link SUCCESS} in case of success, {@link FAILURE} otherwise
 */
int gpio_deinit(int gpio_number);

/**
 * Reset a GPIO.
 *
 * Unexport sysfs GPIO resource if not exported.
 *
 * @param gpio_number Linux sysfs GPIO resource number
 * @return {@link SUCCESS} in case of success, {@link FAILURE} otherwise
 */
int gpio_reset(int gpio_number);

/**
 * Set the GPIO value.
 *
 * @param gpio_number Linux sysfs GPIO resource number
 * @param value GPIO state to set (0 or 1)
 * @return {@link SUCCESS} in case of success, {@link FAILURE} otherwise.
 */
int gpio_set_value(int gpio_number, int value);

/**
 * Get the GPIO value.
 *
 * @param gpio_number Linux sysfs GPIO resource number
 * @return The read GPIO value, {@link FAILURE} on failure
 */
int gpio_get_value(int gpio_number);

/**
 * Configure the GPIO interrupt mode.
 *
 * @param gpio_number Linux sysfs GPIO resource number
 * @param interrupt_mode GPIO interrupt mode to configure
 * @return {@link SUCCESS} in case of success, {@link FAILURE} otherwise.
 */
int gpio_interrupt(int gpio_number, gpio_interrupt_t interrupt_mode);

/**
 * Wait for interrupt on GPIO pin.
 *
 * @param gpio_number Linux sysfs GPIO resource number
 * @param timeout Maximum waiting time (in milliseconds), negative for infinite
 * @return {@link SUCCESS} in case of interrupt, {@link FAILURE} otherwise.
 */
int gpio_wait_interrupt(int gpio_number, int timeout);

#endif /* GPIO_H_ */
