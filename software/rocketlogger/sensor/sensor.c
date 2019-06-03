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
#include <string.h>

#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../log.h"

#include "bme280.h"
#include "tsl4531.h"

#include "sensor.h"

/// I2C sensor bus identifier for communication
int sensor_bus = -1;

/**
 * The sensor registry structure.
 *
 * Register your sensor (channels) here. Multiple channels from the same
 * sensor should be added as consecutive entries.
 *
 * @note The SENSOR_REGISTRY_SIZE needs to be adjusted accordingly.
 */
const rl_sensor_t sensor_registry[SENSOR_REGISTRY_SIZE] = {
    {
        "TSL4531_left", TSL4531_I2C_ADDRESS_LEFT, TSL4531_CHANNEL_DEFAULT,
        RL_UNIT_LUX, RL_SCALE_NONE, &tsl4531_init, &tsl4531_deinit,
        &tsl4531_read, &tsl4531_get_value,
    },
    {
        "TSL4531_right", TSL4531_I2C_ADDRESS_RIGHT, TSL4531_CHANNEL_DEFAULT,
        RL_UNIT_LUX, RL_SCALE_NONE, &tsl4531_init, &tsl4531_deinit,
        &tsl4531_read, &tsl4531_get_value,
    },
    {
        "BME280_temp", BME280_I2C_ADDRESS_LEFT, BME280_CHANNEL_TEMPERATURE,
        RL_UNIT_DEG_C, RL_SCALE_MILLI, &bme280_init, &bme280_deinit,
        &bme280_read, &bme280_get_value,
    },
    {
        "BME280_rh", BME280_I2C_ADDRESS_LEFT, BME280_CHANNEL_HUMIDITY,
        RL_UNIT_INTEGER, RL_SCALE_MICRO, &bme280_init, &bme280_deinit,
        &bme280_read, &bme280_get_value,
    },
    {
        "BME280_press", BME280_I2C_ADDRESS_LEFT, BME280_CHANNEL_PRESSURE,
        RL_UNIT_PASCAL, RL_SCALE_MILLI, &bme280_init, &bme280_deinit,
        &bme280_read, &bme280_get_value,
    },
};

/**
 * Initialize the shared I2C sensor bus.
 * @return Status (error) code
 */
int sensors_init(void) {
    sensor_bus = sensors_open_bus();
    return sensor_bus;
}

/**
 * Deinitialize the shared I2C sensor bus.
 * @return Status (error) code
 */
void sensors_deinit(void) {
    sensors_close_bus(sensor_bus);
    int sensor_bus = -1;
    (void)sensor_bus; // suppress unused warning
}

/**
 * Open a new handle of the I2C bus.
 * @return The bus handle
 */
int sensors_open_bus(void) {
    int bus = open(I2C_BUS_FILENAME, O_RDWR);
    if (bus < 0) {
        rl_log(ERROR, "failed to open the I2C bus", bus);
    }
    return bus;
}

/**
 * Close a I2C sensor bus.
 * @param bus The I2C bus to close
 * @return Status (error) code
 */
int sensors_close_bus(int bus) {
    int result = close(bus);
    if (result < 0) {
        rl_log(ERROR, "failed to close the I2C bus", bus);
    }
    return result;
}

/**
 * Get the shared I2C bus handle.
 * @return The I2C bus handle
 */
int sensors_get_bus(void) { return sensor_bus; }

/**
 * Initiate an I2C communication with a device.
 * @param sensor_address The I2C address of the device
 * @return Status (error) code
 */
int sensors_init_comm(uint8_t device_address) {
    return ioctl(sensor_bus, I2C_SLAVE, device_address);
}

/**
 * Scan the I2C sensor for sensor in the registry and initialize them.
 * @param sensors_available List of sensors of the registry available
 * @return Number of sensors from the registry found on the bus
 */
int sensors_scan(int sensors_available[SENSOR_REGISTRY_SIZE]) {

    // log message
    char message[MAX_MESSAGE_LENGTH] =
        "List of available ambient sensors:\n\t- ";

    // Scan for available sensors //
    int sensor_count = 0;

    // scan
    int mutli_channel_initialized = -1;
    for (int i = 0; i < SENSOR_REGISTRY_SIZE; i++) {
        // do not initialize multi channel sensors more than once
        int result = 0;
        if (sensor_registry[i].identifier != mutli_channel_initialized) {
            result = sensor_registry[i].init(sensor_registry[i].identifier);
        } else {
            result = SUCCESS;
        }

        if (result == SUCCESS) {
            // sensor available
            sensors_available[i] = sensor_registry[i].identifier;
            mutli_channel_initialized = sensor_registry[i].identifier;
            sensor_count++;

            // message
            strcat(message, sensor_registry[i].name);
            strcat(message, "\n\t- ");
        } else {
            // sensor not available
            sensors_available[i] = -1;
            mutli_channel_initialized = -1;
        }
    }

    // message & return
    if (sensor_count == 0) {
        rl_log(WARNING, "no ambient sensor found...");
    } else {
        message[strlen(message) - 3] = 0;
        rl_log(INFO, "%s", message);
        printf("\n\n%s\n", message);
    }
    return sensor_count;
}

/**
 * Close all sensors used on the I2C bus.
 * @param sensors_available List of available (previously initialized) sensors
 */
void sensors_close(int const sensors_available[SENSOR_REGISTRY_SIZE]) {
    for (int i = 0; i < SENSOR_REGISTRY_SIZE; i++) {
        if (sensors_available[i] >= 0) {
            sensor_registry[i].deinit(sensor_registry[i].identifier);
        }
    }
}
