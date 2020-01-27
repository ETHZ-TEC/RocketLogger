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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <linux/limits.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "log.h"
#include "rl.h"
#include "sysfs.h"

#include "gpio.h"

int gpio_init(int gpio_number, gpio_mode_t mode) {
    int ret = 0;
    char sysfs_file[PATH_MAX];

    // export unexported GPIO
    sprintf(sysfs_file, (GPIO_SYSFS_PATH "gpio%d"), gpio_number);
    ret = sysfs_export_unexported(sysfs_file, (GPIO_SYSFS_PATH "export"),
                                  gpio_number);
    if (ret < 0) {
        rl_log(RL_LOG_ERROR, "could not export GPIO pin #%d", gpio_number);
        return ret;
    }

    // configure GPIO direction
    sprintf(sysfs_file, (GPIO_SYSFS_PATH "gpio%d/direction"), gpio_number);
    if (mode == GPIO_MODE_IN) {
        ret = sysfs_write_string(sysfs_file, "in");
    } else if (mode == GPIO_MODE_OUT) {
        ret = sysfs_write_string(sysfs_file, "out");
    }
    if (ret < 0) {
        rl_log(RL_LOG_ERROR, "could not configure mode of GPIO pin #%d",
               gpio_number);
        return ret;
    }

    return SUCCESS;
}

int gpio_deinit(int gpio_number) {
    // unexport sysfs GPIO module
    int ret = sysfs_unexport((GPIO_SYSFS_PATH "unexport"), gpio_number);
    if (ret < 0) {
        rl_log(RL_LOG_ERROR, "could not unexport GPIO pin #%d", gpio_number);
        return ret;
    }

    return SUCCESS;
}

int gpio_reset(int gpio_number) {
    char sysfs_file[PATH_MAX];
    sprintf(sysfs_file, (GPIO_SYSFS_PATH "gpio%d"), gpio_number);
    int ret = sysfs_unexport_exported(sysfs_file, (GPIO_SYSFS_PATH "unexport"),
                                      gpio_number);
    if (ret < 0) {
        rl_log(RL_LOG_ERROR, "could not reset GPIO pin #%d", gpio_number);
        return ret;
    }

    return SUCCESS;
}

int gpio_interrupt(int gpio_number, gpio_interrupt_t interrupt_mode) {
    // construct sysfs path
    char sysfs_file[PATH_MAX];
    sprintf(sysfs_file, (GPIO_SYSFS_PATH "gpio%d/edge"), gpio_number);

    // set mode
    int ret;
    switch (interrupt_mode) {
    case GPIO_INT_NONE:
        ret = sysfs_write_string(sysfs_file, "none");
        break;
    case GPIO_INT_RISING:
        ret = sysfs_write_string(sysfs_file, "rising");
        break;
    case GPIO_INT_FALLING:
        ret = sysfs_write_string(sysfs_file, "falling");
        break;
    case GPIO_INT_BOTH:
        ret = sysfs_write_string(sysfs_file, "both");
        break;
    default:
        rl_log(RL_LOG_ERROR, "invalid GPIO interrupt mode");
        return ERROR;
    }
    if (ret < 0) {
        rl_log(RL_LOG_ERROR, "could not configure interrupt of GPIO pin #%d",
               gpio_number);
        return ret;
    }

    return SUCCESS;
}

int gpio_set_value(int gpio_number, int value) {
    // check value
    if (value < 0 || value > 1) {
        return ERROR;
    }

    // construct sysfs path
    char sysfs_file[PATH_MAX];
    sprintf(sysfs_file, (GPIO_SYSFS_PATH "gpio%d/value"), gpio_number);

    // set the output value
    int ret = sysfs_write_int(sysfs_file, value);
    if (ret < 0) {
        rl_log(RL_LOG_ERROR, "could not set value of GPIO pin #%d",
               gpio_number);
        return ret;
    }

    return SUCCESS;
}

int gpio_get_value(int gpio_number) {
    // construct sysfs path
    char sysfs_file[PATH_MAX];
    sprintf(sysfs_file, (GPIO_SYSFS_PATH "gpio%d/value"), gpio_number);

    // set the output value
    int value = -1;
    int ret = sysfs_read_int(sysfs_file, &value);
    if (ret < 0) {
        rl_log(RL_LOG_ERROR, "could not get value of GPIO pin #%d",
               gpio_number);
        return ret;
    }

    return value;
}

int gpio_wait_interrupt(int gpio_number, int timeout) {
    // construct sysfs path
    char sysfs_file[PATH_MAX];
    sprintf(sysfs_file, (GPIO_SYSFS_PATH "gpio%d/value"), gpio_number);

    int fd = open(sysfs_file, O_RDONLY);
    if (fd < 0) {
        rl_log(RL_LOG_ERROR, "could not open value file for GPIO pin #%d",
               gpio_number);
        return fd;
    }

    // initialize polling structure
    struct pollfd fds = {
        .fd = fd, .events = POLLPRI, .revents = 0,
    };
    int nfds = 1;

    // dummy read to enable blocking polling
    char value_buffer[2] = "";
    int ret;
    ret = read(fds.fd, &value_buffer, 1);
    if (ret <= 0) {
        rl_log(RL_LOG_ERROR, "GPIO poll read failed for GPIO pin #%d",
               gpio_number);
        close(fd);
        return ERROR;
    }

    // wait on gpio change
    ret = poll(&fds, nfds, timeout);
    if (ret < 0) {
        rl_log(RL_LOG_ERROR, "GPIO poll failed for GPIO pin #%d", gpio_number);
        close(fd);
        return ret;
    }

    // debounce GPIO input and read back GPIO pin value
    usleep(GPIO_DEBOUNCE_DELAY);
    lseek(fds.fd, 0, SEEK_SET);
    ret = read(fds.fd, &value_buffer, 1);
    close(fd);
    if (ret <= 0) {
        rl_log(RL_LOG_ERROR,
               "GPIO read back after poll event failed for GPIO pin #%d",
               gpio_number);
        return ERROR;
    }
    return atoi(value_buffer);
}
