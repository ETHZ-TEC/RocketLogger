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

#include <stdint.h>

#include <i2c/smbus.h>

#include "../log.h"
#include "sensor.h"

#include "bme280.h"

const int bme280_sensors[] = BME280_I2C_ADDRESSES;

int32_t bme280_temperature[sizeof(bme280_sensors)] = {0};
int32_t bme280_humidity[sizeof(bme280_sensors)] = {0};
int32_t bme280_preasure[sizeof(bme280_sensors)] = {0};

bme280_calibration_t BME280_calibration[sizeof(bme280_sensors)];

/**
 * Initialize the light sensor
 * @param sensor_identifier The I2C address of the sensor
 * @return return code
 */
int bme280_init(int sensor_identifier) {
    int sensor_bus = sensors_get_bus();
    if (sensor_bus < 0) {
        rl_log(ERROR, "BME280 I2C bus not initialized properly");
        return FAILURE;
    }

    int result;

    uint8_t device_address = (uint8_t)sensor_identifier;
    result = sensors_init_comm(device_address);
    if (result < 0) {
        rl_log(ERROR, "BME280 I2C initialization failed");
        return FAILURE;
    }

    int sensor_id = bme280_get_id();
    if (sensor_id != BME280_ID) {
        rl_log(ERROR, "BME280 with wrong sensor ID: %d", sensor_id);
        return FAILURE;
    }

    result = bme280_read_calibration(sensor_identifier);
    if (result < 0) {
        rl_log(ERROR, "BME280 reading calibration failed");
        return FAILURE;
    }

    result = bme280_set_parameters(sensor_identifier);
    if (result < 0) {
        rl_log(ERROR, "BME280 setting configuraiton failed");
        return FAILURE;
    }

    return SUCCESS;
}

/**
 * Deinitialize BME280 sensor.
 * @param sensor_identifier The I2C address of the sensor
 */
void bme280_deinit(int sensor_identifier) {
    (void)sensor_identifier; // suppress unsued variable warning
}

/**
 * Read the sensor values.
 * @param sensor_identifier The I2C address of the sensor
 * @return Value in lux
 */
int bme280_read(int sensor_identifier) {
    int sensor_bus = sensors_get_bus();
    int sensor_index = bme280_get_index(sensor_identifier);
    uint8_t data[BME280_DATA_BLOCK_SIZE];

    // select sensor
    uint8_t device_address = (uint8_t)sensor_identifier;
    int result = sensors_init_comm(device_address);
    if (result < 0) {
        rl_log(ERROR, "BME280 I2C communication failed");
        return FAILURE;
    }

    // burst read required for data consistency
    // 	i2c_smbus_read_i2c_block_data(int file, __u8 command, __u8 length, __u8
    // *values)
    int data_size = i2c_smbus_read_i2c_block_data(
        sensor_bus, BME280_REG_PREASURE_MSB, BME280_DATA_BLOCK_SIZE, data);
    if (data_size != BME280_DATA_BLOCK_SIZE) {
        rl_log(ERROR, "BME280 reading data block failed");
        return FAILURE;
    }

    // reconstruct and compensate data
    int32_t preasure_raw = (((int32_t)data[0]) << 12) |
                           (((int32_t)data[1]) << 4) |
                           (((int32_t)data[2]) & 0x0f);
    int32_t temperature_raw = (((int32_t)data[3]) << 12) |
                              (((int32_t)data[4]) << 4) |
                              (((int32_t)data[5]) & 0x0f);
    int32_t humidity_raw = (((int32_t)data[6]) << 8) | ((int32_t)data[7]);

    bme280_temperature[sensor_index] =
        bme280_compensate_temperature(sensor_identifier, temperature_raw);
    bme280_humidity[sensor_index] = (int32_t)bme280_compensate_humidity(
        sensor_identifier, humidity_raw, temperature_raw);
    bme280_preasure[sensor_index] = (int32_t)bme280_compensate_preasure(
        sensor_identifier, preasure_raw, temperature_raw);

    return SUCCESS;
}

/**
 * Get the values read from the sensor
 * @param sensor_identifier The I2C address of the sensor
 * @param channel The channel of the sensor to get
 * @return Channel value
 */
int32_t bme280_get_value(int sensor_identifier, int channel) {
    int sensor_index = bme280_get_index(sensor_identifier);

    switch (channel) {
    case BME280_CHANNEL_TEMPERATURE:
        return bme280_temperature[sensor_index];

    case BME280_CHANNEL_HUMIDITY:
        return bme280_humidity[sensor_index];

    case BME280_CHANNEL_PRESSURE:
        return bme280_preasure[sensor_index];

    default:
        return 0;
    }
}

/**
 * Get the device ID
 * @param sensor_identifier The I2C address of the sensor
 * @return Devie ID or negative status code
 */
int bme280_get_id(void) {
    int sensor_bus = sensors_get_bus();

    int read_result = i2c_smbus_read_byte_data(sensor_bus, BME280_REG_ID);

    if (read_result < 0) {
        rl_log(ERROR, "BME280 I2C error reading ID of sensor");
    }
    return read_result;
}

/**
 * Read the sensor specifc calibration values.
 * @param sensor_identifier The I2C address of the sensor
 * @return Status code
 */
int bme280_read_calibration(int sensor_identifier) {
    int sensor_bus = sensors_get_bus();
    int sensor_index = bme280_get_index(sensor_identifier);
    uint8_t data[26];

    // first calibration data block (0x88...0xA1, 26 values)
    int data_size1 =
        i2c_smbus_read_i2c_block_data(sensor_bus, BME280_REG_CALIBRATION_BLOCK1,
                                      BME280_CALIBRATION_BLOCK1_SIZE, data);
    if (data_size1 != BME280_CALIBRATION_BLOCK1_SIZE) {
        rl_log(ERROR, "BME280 reading calibration block 1 failed");
        return FAILURE;
    }

    BME280_calibration[sensor_index].T1 =
        ((uint16_t)data[1] << 8) | ((uint16_t)data[0]);
    BME280_calibration[sensor_index].T2 =
        ((int16_t)data[3] << 8) | ((int16_t)data[2]);
    BME280_calibration[sensor_index].T3 =
        ((int16_t)data[5] << 8) | ((int16_t)data[4]);

    BME280_calibration[sensor_index].P1 =
        ((uint16_t)data[7] << 8) | ((uint16_t)data[6]);
    BME280_calibration[sensor_index].P2 =
        ((int16_t)data[9] << 8) | ((int16_t)data[8]);
    BME280_calibration[sensor_index].P3 =
        ((int16_t)data[11] << 8) | ((int16_t)data[10]);
    BME280_calibration[sensor_index].P4 =
        ((int16_t)data[13] << 8) | ((int16_t)data[12]);
    BME280_calibration[sensor_index].P5 =
        ((int16_t)data[15] << 8) | ((int16_t)data[14]);
    BME280_calibration[sensor_index].P6 =
        ((int16_t)data[17] << 8) | ((int16_t)data[16]);
    BME280_calibration[sensor_index].P7 =
        ((int16_t)data[19] << 8) | ((int16_t)data[18]);
    BME280_calibration[sensor_index].P8 =
        ((int16_t)data[21] << 8) | ((int16_t)data[20]);
    BME280_calibration[sensor_index].P9 =
        ((int16_t)data[23] << 8) | ((int16_t)data[22]);

    BME280_calibration[sensor_index].H1 = (uint8_t)data[25];

    // second calibration data block (0xE1...0xE7 [0xF0], 7 [16] values)
    int data_size2 =
        i2c_smbus_read_i2c_block_data(sensor_bus, BME280_REG_CALIBRATION_BLOCK2,
                                      BME280_CALIBRATION_BLOCK2_SIZE, data);
    if (data_size2 != BME280_CALIBRATION_BLOCK2_SIZE) {
        rl_log(ERROR, "BME280 reading calibration block 2 failed");
        return FAILURE;
    }

    BME280_calibration[sensor_index].H2 =
        ((int16_t)data[1] << 8) | ((int16_t)data[0]);
    BME280_calibration[sensor_index].H3 = (uint8_t)data[2];
    BME280_calibration[sensor_index].H4 =
        ((int16_t)data[3] << 4) | ((int16_t)data[4] & 0x0f);
    BME280_calibration[sensor_index].H5 =
        ((int16_t)data[5] << 4) | ((int16_t)data[4] >> 4);
    BME280_calibration[sensor_index].H6 = (int8_t)data[6];

    return SUCCESS;
}

/**
 * Set the sensor parameter to default for continuous sensing.
 * @return Status code
 */
int bme280_set_parameters(int sensor_identifier) {
    (void)sensor_identifier; // suppress unsued variable warning
    int sensor_bus = sensors_get_bus();

    int result;

    // config: standby 250ms, filter off, no SPI, continuous measurement
    result = i2c_smbus_write_byte_data(sensor_bus, BME280_REG_CONFIG,
                                       BME280_STANDBY_DURATION_250 |
                                           BME280_FILTER_OFF);
    if (result < 0) {
        rl_log(ERROR, "BME280 setting config register failed");
        return FAILURE;
    }

    // humidity control: oversampling x1
    result = i2c_smbus_write_byte_data(sensor_bus, BME280_REG_CONTROL_HUMIDITY,
                                       BME280_OVERSAMPLE_HUMIDITY_1);
    if (result < 0) {
        rl_log(ERROR, "BME280 setting humidiy control register failed");
        return FAILURE;
    }

    // measure control: oversampling x1, continuous measurement
    result = i2c_smbus_write_byte_data(sensor_bus, BME280_REG_CONTROL_MEASURE,
                                       BME280_OVERSAMPLE_PREASURE_1 |
                                           BME280_OVERSAMPLE_TEMPERATURE_1 |
                                           BME280_MODE_NORMAL);
    if (result < 0) {
        rl_log(ERROR, "BME280 setting measure control register failed");
        return FAILURE;
    }

    return SUCCESS;
}

/**
 * Get the index of the sensor with specified address.
 * @param sensor_identifier The sensor address used to look up the index
 * @return The index of the sensor, or if not found -1
 */
int bme280_get_index(int sensor_identifier) {
    unsigned int index = 0;
    while (index < sizeof(bme280_sensors)) {
        if (sensor_identifier == bme280_sensors[index]) {
            return (int)index;
        }
        index++;
    }
    return -1;
}

/**
 * Get fine grained temperature compensation value for further processing.
 * @param sensor_identifier The I2C address of the sensor
 * @param temperature_raw The raw temperature reading
 * @return Fine grained, compensated temperature (arbritrary unit)
 */
int32_t bme280_compensate_temperature_fine(int sensor_identifier,
                                           int32_t temperature_raw) {
    int sensor_index = bme280_get_index(sensor_identifier);

    int32_t var1, var2, temperature_fine;
    var1 = ((((temperature_raw >> 3) -
              ((int32_t)BME280_calibration[sensor_index].T1 << 1))) *
            ((int32_t)((int16_t)BME280_calibration[sensor_index].T2))) >>
           11;
    var2 = (((((temperature_raw >> 4) -
               ((int32_t)BME280_calibration[sensor_index].T1)) *
              ((temperature_raw >> 4) -
               ((int32_t)BME280_calibration[sensor_index].T1))) >>
             12) *
            ((int32_t)((int16_t)BME280_calibration[sensor_index].T3))) >>
           14;
    temperature_fine = var1 + var2;

    return temperature_fine;
}

/**
 * Get the compensated temperature in milli degree centigrade.
 * @param sensor_identifier The I2C address of the sensor
 * @param temperature_raw The raw temperature reading
 * @return The compensated temperature in 0.001 degree centigrade
 */
int32_t bme280_compensate_temperature(int sensor_identifier,
                                      int32_t temperature_raw) {
    int32_t temperature_fine =
        bme280_compensate_temperature_fine(sensor_identifier, temperature_raw);
    int32_t temperature = (temperature_fine * 50 + 1280) >> 8;
    return temperature;
}

/**
 * Get the compensated preasure in milli Pascal.
 * @param sensor_identifier The I2C address of the sensor
 * @param preasure_raw The raw preasure reading
 * @param temperature_raw The raw temperature reading
 * @return The compensated preasure in 0.001 Pascal
 */
uint32_t bme280_compensate_preasure(int sensor_identifier, int32_t preasure_raw,
                                    int32_t temperature_raw) {
    int sensor_index = bme280_get_index(sensor_identifier);
    int64_t var1, var2, preasure;

    int64_t temperature_fine =
        bme280_compensate_temperature_fine(sensor_identifier, temperature_raw);
    var1 = temperature_fine - 128000;
    var2 = var1 * var1 * (int64_t)BME280_calibration[sensor_index].P6 +
           ((var1 * (int64_t)BME280_calibration[sensor_index].P5) << 17) +
           (((int64_t)BME280_calibration[sensor_index].P4) << 35);
    var1 = ((var1 * var1 * (int64_t)BME280_calibration[sensor_index].P3) >> 8) +
           ((var1 * (int64_t)BME280_calibration[sensor_index].P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1) *
            ((int64_t)BME280_calibration[sensor_index].P1)) >>
           33;
    if (var1 == 0) {
        return 0; // avoid exception caused by division by zero
    }
    preasure = 1048576 - preasure_raw;
    preasure = (((preasure << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)BME280_calibration[sensor_index].P9) * (preasure >> 13) *
            (preasure >> 13)) >>
           25;
    var2 = (((int64_t)BME280_calibration[sensor_index].P8) * preasure) >> 19;
    preasure = ((preasure + var1 + var2) >> 8) +
               (((int64_t)BME280_calibration[sensor_index].P7) << 4);

    // Returns pressure in Pa as unsigned 32 bit integer in Q24.8 format (24
    // integer bits and 8 fractional bits).
    // Output value of '24674867' represents 24674867/256 = 96386.2 Pa = 963.862
    // hPa
    // return (uint32_t)preasure;

    int64_t preasure_rebased = (1000 * preasure) >> 8;
    return (uint32_t)preasure_rebased;
}

/**
 * Get the compensated relative humidity in micros (0.0001 percents).
 * @param sensor_identifier The I2C address of the sensor
 * @param humidity_raw The raw humidity reading
 * @param temperature_raw The raw temperature reading
 * @return The compensated humidity in micros (0.0001 percents)
 */
uint32_t bme280_compensate_humidity(int sensor_identifier, int32_t humidity_raw,
                                    int32_t temperature_raw) {
    int sensor_index = bme280_get_index(sensor_identifier);

    int32_t temperature_fine =
        bme280_compensate_temperature_fine(sensor_identifier, temperature_raw);

    int32_t humidity = (temperature_fine - ((int32_t)76800));
    humidity =
        ((((humidity_raw << 14) -
           (((int32_t)BME280_calibration[sensor_index].H4) << 20) -
           (((int32_t)BME280_calibration[sensor_index].H5) * humidity)) +
          ((int32_t)16384)) >>
         15) *
        (((((((humidity * ((int32_t)BME280_calibration[sensor_index].H6)) >>
              10) *
             (((humidity * ((int32_t)BME280_calibration[sensor_index].H3)) >>
               11) +
              ((int32_t)32768))) >>
            10) +
           ((int32_t)2097152)) *
              ((int32_t)BME280_calibration[sensor_index].H2) +
          8192) >>
         14);
    humidity = (humidity - (((((humidity >> 15) * (humidity >> 15)) >> 7) *
                             ((int32_t)BME280_calibration[sensor_index].H1)) >>
                            4));
    if (humidity < 0) {
        humidity = 0;
    } else if (humidity > 419430400) {
        humidity = 419430400;
    }

    // Returns humidity in %RH as unsigned 32 bit integer in Q22.10 format (22
    // integer and 10 fractional bits).
    // Output value of '47445' represents 47445/1024 = 46.333 %RH
    // return (uint32_t)(humidity >> 12);

    uint32_t humidity_rebased = (((uint64_t)10000 * humidity) >> 22);
    return humidity_rebased;
}
