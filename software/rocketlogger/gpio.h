/**
 * Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group
 */

#ifndef GPIO_H_
#define GPIO_H_

#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "log.h"
#include "types.h"

/// Path to linux GPIO device files
#define GPIO_PATH "/sys/class/gpio/"
/// Minimal time a button needs to be pressed (in µs)
#define MIN_BUTTON_TIME 100000

/**
 * GPIO direction definition
 */
typedef enum direction {
    IN, //!< GPIO read mode
    OUT //!< GPIO write mode
} rl_direction;

/**
 * GPIO interrupt edge definition
 */
typedef enum edge {
    NONE,    //!< No interrupt
    RISING,  //!< Interrupt on rising edge
    FALLING, //!< Interrupt on falling edge
    BOTH     //!< Interrupt on both edges
} rl_edge;

// gpio unexport
int gpio_unexport(int num);

// gpio export
int gpio_export(int num);

// set direction
int gpio_dir(int num, rl_direction dir);

// gpio value
int gpio_set_value(int num, int val);
int gpio_get_value(int num);

// interrupt
int gpio_interrupt(int num, rl_edge e);
int gpio_wait_interrupt(int num, int timeout); // timout<0 -> infinite

#endif /* GPIO_H_ */
