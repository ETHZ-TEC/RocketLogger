/**
 * Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group
 */

#ifndef SENSOR_BME280_H_
#define SENSOR_BME280_H_

#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>

#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../types.h"
#include "../log.h"


#define BME280_I2C_ADDRESS_LEFT 0x76

#define BME280_I2C_ADDRESSES { (BME280_I2C_ADDRESS_LEFT) }

#define BME280_CHANNEL_TEMPERATURE 0
#define BME280_CHANNEL_HUMIDITY 1
#define BME280_CHANNEL_PREASURE 2


// register definitions
#define BME280_ID 0x60

#define BME280_REG_CALIBRATION_BLOCK1 0x88
#define BME280_REG_ID 0xD0
#define BME280_REG_RESET 0xE0
#define BME280_REG_CALIBRATION_BLOCK2 0xE1
#define BME280_REG_CONTROL_HUMIDITY 0xF2
#define BME280_REG_STATUS 0xF3
#define BME280_REG_CONTROL_MEASURE 0xF4
#define BME280_REG_CONFIG 0xF5
#define BME280_REG_PREASURE_MSB 0xF7
#define BME280_REG_PREASURE_LSB 0xF8
#define BME280_REG_PREASURE_XLSB 0xF9
#define BME280_REG_TEMPERATURE_MSB 0xFA
#define BME280_REG_TEMPERATURE_LSB 0xFB
#define BME280_REG_TEMPERATURE_XLSB 0xFC
#define BME280_REG_HUMIDITY_MSB 0xFD
#define BME280_REG_HUMIDITY_LSB 0xFE

#define BME280_CALIBRATION_BLOCK1_SIZE 26
#define BME280_CALIBRATION_BLOCK2_SIZE 7

#define BME280_DATA_BLOCK_SIZE 8

#define BME280_RESET_VALUE 0xB6

#define BME280_OVERSAMPLE_HUMIDITY_OFF 0x00
#define BME280_OVERSAMPLE_HUMIDITY_1 0x01
#define BME280_OVERSAMPLE_HUMIDITY_2 0x02
#define BME280_OVERSAMPLE_HUMIDITY_4 0x03
#define BME280_OVERSAMPLE_HUMIDITY_8 0x04
#define BME280_OVERSAMPLE_HUMIDITY_16 0x0f

#define BME280_MEASURING 0x03
#define BME280_UPDATE 0x1

#define BME280_OVERSAMPLE_TEMPERATURE_OFF 0x00
#define BME280_OVERSAMPLE_TEMPERATURE_1 0x20
#define BME280_OVERSAMPLE_TEMPERATURE_2 0x40
#define BME280_OVERSAMPLE_TEMPERATURE_4 0x60
#define BME280_OVERSAMPLE_TEMPERATURE_8 0x80
#define BME280_OVERSAMPLE_TEMPERATURE_16 0xA0
#define BME280_OVERSAMPLE_PREASURE_OFF 0x00
#define BME280_OVERSAMPLE_PREASURE_1 0x04
#define BME280_OVERSAMPLE_PREASURE_2 0x08
#define BME280_OVERSAMPLE_PREASURE_4 0x0C
#define BME280_OVERSAMPLE_PREASURE_8 0x10
#define BME280_OVERSAMPLE_PREASURE_16 0x14
#define BME280_MODE_SLEEP 0x00
#define BME280_MODE_FORCE 0x01
#define BME280_MODE_NORMAL 0x03

#define BME280_STANDBY_DURATION_0_5 0x00
#define BME280_STANDBY_DURATION_62 0x20
#define BME280_STANDBY_DURATION_125 0x40
#define BME280_STANDBY_DURATION_250 0x60
#define BME280_STANDBY_DURATION_500 0x80
#define BME280_STANDBY_DURATION_1000 0xA0
#define BME280_STANDBY_DURATION_10 0xC0
#define BME280_STANDBY_DURATION_20 0xE0
#define BME280_FILTER_OFF 0x00
#define BME280_FILTER_2 0x04
#define BME280_FILTER_4 0x08
#define BME280_FILTER_8 0x0C
#define BME280_FILTER_16 0x10
#define BME280_SPI_EN 0x01


/*
 * Data Structures
 */
struct BME280_calibration_t {
    // temperature
    uint16_t T1;
    int16_t T2;
    int16_t T3;

    // preasure
    uint16_t P1;
    int16_t P2;
    int16_t P3;
    int16_t P4;
    int16_t P5;
    int16_t P6;
    int16_t P7;
    int16_t P8;
    int16_t P9;

    // humidity
    uint8_t H1;
    int16_t H2;
    uint8_t H3;
    int16_t H4;
    int16_t H5;
    int8_t H6;
};


/*
 * API FUNCTIONS
 */
int BME280_init(uint8_t);
void BME280_close(uint8_t);
int BME280_read(uint8_t);
int32_t BME280_getValue(uint8_t, uint8_t);


/*
 * Helper FUNCTIONS
 */
uint8_t BME280_getID(void);
int BME280_readCalibration(uint8_t);
int BME280_setParameters(uint8_t);
int BME280_getIndex(uint8_t);

int32_t BME280_compensate_temperature_fine(uint8_t, int32_t);
int32_t BME280_compensate_temperature(uint8_t, int32_t);
uint32_t BME280_compensate_preasure(uint8_t, int32_t, int32_t);
uint32_t BME280_compensate_humidity(uint8_t, int32_t, int32_t);


#endif /* SENSOR_BME280_H_ */
