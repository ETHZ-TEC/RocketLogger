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

#include <linux/i2c-dev.h>

#include "../log.h"
#include "sensor.h"

#include "tsl4531.h"

/**
 * List of potentially connected TSL4531 sensors.
 */
const int tsl4531_sensors[] = TSL4531_I2C_ADDRESSES;

/**
 * Set mesurement range of light sensor.
 *
 * @param sensor_identifier The I2C address of the sensor
 * @param range The range {@link tsl4531_range} to set
 * @return Returns 0 on success, negative on failure with errno set accordingly
 */
int tsl4531_set_range(int sensor_identifier, int range);

/**
 * Get current measurement range.
 *
 * @param sensor_identifier The I2C address of the sensor
 * @return current range {@link tsl4531_range}
 */
int tsl4531_get_range(int sensor_identifier);

/**
 * Get the device ID.
 *
 * @param sensor_identifier The I2C address of the sensor
 * @return Returns device ID on success, negative on failure with errno set
 * accordingly
 */
int tsl4531_get_id(void);

/**
 * Set the sensor parameter to default for continuous sensing.
 *
 * @param sensor_identifier The I2C address of the sensor
 * @return Returns 0 on success, negative on failure with errno set accordingly
 */
int tsl4531_set_parameters(int sensor_identifier);

/**
 * Configure the range of the sensor.
 *
 * @param sensor_identifier The I2C address of the sensor
 * @param range The range {@link tsl4531_range} to set
 * @return Returns 0 on success, negative on failure with errno set accordingly
 */
int tsl4531_send_range(int sensor_identifier, int range);

/**
 * Get the index of the sensor with specified address.
 *
 * @param sensor_identifier The sensor address used to look up the index
 * @return Returns 0 on success, or -1 if sensor with given index was not found
 */
int tsl4531_get_index(int sensor_identifier);

/**
 * The currently configured measurement range.
 */
tsl4531_range_t tsl4531_range[sizeof(tsl4531_sensors)] = {TSL4531_RANGE_AUTO};

/**
 * Dynamic sensor range currently set if auto ranging is used.
 */
tsl4531_range_t tsl4531_auto_range[sizeof(tsl4531_sensors)] = {
    TSL4531_RANGE_MEDIUM};

/**
 * The measurement range dependent sensor value multiplier to use.
 */
uint8_t tsl4531_multiplier[sizeof(tsl4531_sensors)] = {TSL4531_MULT_200};

/**
 * The sensor value data buffer.
 */
int32_t tsl4531_values[sizeof(tsl4531_sensors)] = {0};

int tsl4531_init(int sensor_identifier) {
    int sensor_bus = sensors_get_bus();

    if (sensor_bus < 0) {
        rl_log(RL_LOG_ERROR, "TSL4531 I2C bus not initialized properly");
        return sensor_bus;
    }

    int result = 0;

    uint8_t device_address = (uint8_t)sensor_identifier;
    result = sensors_init_comm(device_address);
    if (result < 0) {
        rl_log(RL_LOG_ERROR, "TSL4531 I2C initialization failed");
        return result;
    }

    int sensor_id = tsl4531_get_id();
    if (sensor_id != TSL4531_ID) {
        rl_log(RL_LOG_ERROR, "TSL4531 with wrong sensor ID: %d", sensor_id);
        return sensor_id;
    }

    result = tsl4531_set_parameters(sensor_identifier);
    if (result < 0) {
        rl_log(RL_LOG_ERROR, "TSL4531 setting configuration failed");
        return result;
    }

    return 0;
}

void tsl4531_deinit(int sensor_identifier) {
    (void)sensor_identifier; // suppress unused parameter warning
}

int tsl4531_read(int sensor_identifier) {
    int sensor_index = tsl4531_get_index(sensor_identifier);
    int sensor_bus = sensors_get_bus();

    int result;

    uint8_t device_address = (uint8_t)sensor_identifier;
    result = sensors_init_comm(device_address);
    if (result < 0) {
        rl_log(RL_LOG_ERROR, "TSL4531 I2C communication failed");
        return result;
    }

    // read sensor data word
    int32_t data = i2c_smbus_read_word_data(
        sensor_bus, TSL4531_COMMAND | TSL4531_REG_DATALOW);
    if (data < 0) {
        rl_log(RL_LOG_ERROR, "TSL4531 reading data word failed");
        return data;
    }

    tsl4531_values[sensor_index] =
        (data & 0xffff) * tsl4531_multiplier[sensor_index];

    if (tsl4531_range[sensor_index] == TSL4531_RANGE_AUTO) {
        // Auto-Range
        tsl4531_range_t range_set = tsl4531_auto_range[sensor_index];
        switch (range_set) {
        case TSL4531_RANGE_LOW:
            if (tsl4531_values[sensor_index] >= TSL4531_RANGE_LOW_MAX) {
                tsl4531_auto_range[sensor_index] = TSL4531_RANGE_MEDIUM;
            }
            break;
        case TSL4531_RANGE_MEDIUM:
            if (tsl4531_values[sensor_index] >= TSL4531_RANGE_MEDIUM_MAX) {
                tsl4531_auto_range[sensor_index] = TSL4531_RANGE_HIGH;
            } else if (tsl4531_values[sensor_index] <
                       TSL4531_RANGE_LOW_MAX - TSL4531_RANGE_HYSTERESIS) {
                tsl4531_auto_range[sensor_index] = TSL4531_RANGE_LOW;
            }
            break;
        case TSL4531_RANGE_HIGH:
            if (tsl4531_values[sensor_index] <
                TSL4531_RANGE_MEDIUM_MAX - TSL4531_RANGE_HYSTERESIS) {
                tsl4531_auto_range[sensor_index] = TSL4531_RANGE_MEDIUM;
            }
            break;
        default:
            rl_log(RL_LOG_ERROR, "TSL4531 auto range logic failure");
            return ERROR;
        }

        int result = tsl4531_send_range(sensor_identifier,
                                        tsl4531_auto_range[sensor_index]);
        if (result < 0) {
            tsl4531_auto_range[sensor_index] = range_set;
            rl_log(RL_LOG_ERROR, "TSL4531 auto range update failed");
            return result;
        }
    }

    return 0;
}

int32_t tsl4531_get_value(int sensor_identifier, int channel) {
    if (channel > 0) {
        return 0;
    }

    int sensor_index = tsl4531_get_index(sensor_identifier);

    return tsl4531_values[sensor_index];
}

int tsl4531_set_range(int sensor_identifier, int range) {
    int sensor_index = tsl4531_get_index(sensor_identifier);

    int result = tsl4531_send_range(sensor_identifier, range);
    if (result < 0) {
        rl_log(RL_LOG_ERROR, "TSL4531 auto range update failed");
        return result;
    }

    tsl4531_range[sensor_index] = range;

    return 0;
}

int tsl4531_get_range(int sensor_identifier) {
    int sensor_index = tsl4531_get_index(sensor_identifier);

    if (tsl4531_range[sensor_index] == TSL4531_RANGE_AUTO) {
        return tsl4531_auto_range[sensor_index];
    } else {
        return tsl4531_range[sensor_index];
    }
}

int tsl4531_get_id(void) {
    int sensor_bus = sensors_get_bus();

    int read_result =
        i2c_smbus_read_byte_data(sensor_bus, TSL4531_COMMAND | TSL4531_REG_ID);

    if (read_result < 0) {
        rl_log(RL_LOG_ERROR, "TSL4531 I2C error reading ID of sensor");
    }
    return read_result;
}

int tsl4531_set_parameters(int sensor_identifier) {
    int sensor_bus = sensors_get_bus();

    int result;

    result = i2c_smbus_write_byte_data(sensor_bus,
                                       TSL4531_COMMAND | TSL4531_REG_CONTROL,
                                       TSL4531_SAMPLE_CONTINUOUS);
    if (result < 0) {
        rl_log(RL_LOG_ERROR, "TSL4531 writing control register failed");
        return result;
    }

    result = tsl4531_set_range(sensor_identifier, TSL4531_RANGE_AUTO);
    if (result < 0) {
        rl_log(RL_LOG_ERROR, "TSL4531 setting range failed");
        return result;
    }

    return 0;
}

int tsl4531_send_range(int sensor_identifier, int range) {
    int sensor_index = tsl4531_get_index(sensor_identifier);
    int sensor_bus = sensors_get_bus();

    int result = -1;

    switch (range) {
    case TSL4531_RANGE_LOW:
        result = i2c_smbus_write_byte_data(
            sensor_bus, TSL4531_COMMAND | TSL4531_REG_CONFIG,
            TSL4531_INT_TIME_400 | TSL4531_LOW_POWER);
        if (result < 0) {
            rl_log(RL_LOG_ERROR,
                   "TSL4531 writing new range configuration failed");
            return result;
        }
        tsl4531_multiplier[sensor_index] = TSL4531_MULT_400;
        break;
    case TSL4531_RANGE_MEDIUM:
        result = i2c_smbus_write_byte_data(
            sensor_bus, TSL4531_COMMAND | TSL4531_REG_CONFIG,
            TSL4531_INT_TIME_200 | TSL4531_LOW_POWER);
        if (result < 0) {
            rl_log(RL_LOG_ERROR,
                   "TSL4531 writing new range configuration failed");
            return result;
        }
        tsl4531_multiplier[sensor_index] = TSL4531_MULT_200;
        break;
    case TSL4531_RANGE_HIGH:
        result = i2c_smbus_write_byte_data(
            sensor_bus, TSL4531_COMMAND | TSL4531_REG_CONFIG,
            TSL4531_INT_TIME_100 | TSL4531_LOW_POWER);
        if (result < 0) {
            rl_log(RL_LOG_ERROR,
                   "TSL4531 writing new range configuration failed");
            return result;
        }
        tsl4531_multiplier[sensor_index] = TSL4531_MULT_100;
        break;
    case TSL4531_RANGE_AUTO:
        result = i2c_smbus_write_byte_data(
            sensor_bus, TSL4531_COMMAND | TSL4531_REG_CONFIG,
            TSL4531_INT_TIME_200 | TSL4531_LOW_POWER);
        if (result < 0) {
            rl_log(RL_LOG_ERROR,
                   "TSL4531 writing new range configuration failed");
            return result;
        }
        tsl4531_multiplier[sensor_index] = TSL4531_MULT_200;
        tsl4531_auto_range[sensor_index] = TSL4531_RANGE_MEDIUM;
        break;
    default:
        rl_log(RL_LOG_ERROR, "TSL4531 invalid range");
        return result;
    }

    return 0;
}

int tsl4531_get_index(int sensor_identifier) {
    unsigned int index = 0;
    while (index < sizeof(tsl4531_sensors)) {
        if (sensor_identifier == tsl4531_sensors[index]) {
            return (int)index;
        }
        index++;
    }
    return -1;
}
