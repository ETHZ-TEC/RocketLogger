/**
 * Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group
 */

#ifndef SENSOR_TSL4531_H_
#define SENSOR_TSL4531_H_

#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../log.h"
#include "../types.h"

#define TSL4531_I2C_ADDRESS_LEFT 0x29
#define TSL4531_I2C_ADDRESS_RIGHT 0x28

#define TSL4531_I2C_ADDRESSES                                                  \
    { (TSL4531_I2C_ADDRESS_LEFT), (TSL4531_I2C_ADDRESS_RIGHT) }

#define TSL4531_CHANNEL_DEFAULT 0

// register definitions
#define TSL4531_ID 162

#define TSL4531_COMMAND 0x80

#define TSL4531_REG_CONTROL 0x00
#define TSL4531_REG_CONFIG 0x01
#define TSL4531_REG_DATALOW 0x04
#define TSL4531_REG_DATAHIGH 0x05
#define TSL4531_REG_ID 0x0A

#define TSL4531_SAMPLE_OFF 0x00
#define TSL4531_SAMPLE_SINGLE 0x2
#define TSL4531_SAMPLE_CONTINUOUS 0x03

#define TSL4531_HIGH_POWER 0x08
#define TSL4531_LOW_POWER 0x00

#define TSL4531_INT_TIME_100 0x02
#define TSL4531_INT_TIME_200 0x01
#define TSL4531_INT_TIME_400 0x00

#define TSL4531_MULT_100 4
#define TSL4531_MULT_200 2
#define TSL4531_MULT_400 1

/**
 * Ranges
 */
enum TSL4531_range {
    TSL4531_RANGE_LOW,
    TSL4531_RANGE_MEDIUM,
    TSL4531_RANGE_HIGH,
    TSL4531_RANGE_AUTO,
};
#define TSL4531_RANGE_LOW_MAX 65000
#define TSL4531_RANGE_MEDIUM_MAX 130000
#define TSL4531_RANGE_HYSTERESIS 5000

/*
 * API FUNCTIONS
 */
int TSL4531_init(uint8_t);
void TSL4531_close(uint8_t);
int TSL4531_read(uint8_t);
int32_t TSL4531_getValue(uint8_t, uint8_t);

int TSL4531_setRange(uint8_t, int);
int TSL4531_getRange(uint8_t);

/*
 * Helper FUNCTIONS
 */
uint8_t TSL4531_getID(void);
int TSL4531_setParameters(uint8_t);
int TSL4531_sendRange(uint8_t, int);
int TSL4531_getIndex(uint8_t);

#endif /* SENSOR_TSL4531_H_ */
