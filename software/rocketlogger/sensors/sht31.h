/**
 * Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group
 */

#ifndef SENSOR_SHT31_H_
#define SENSOR_SHT31_H_

#include <stdint.h>


#define SHT31_I2C_ADDRESS 0x44

#define SHT31_CHANNEL_TEMPERATURE 0
#define SHT31_CHANNEL_HUMIDITY 1


// register definitions
#define SHT31_ID 162

#define SHT31_COMMAND 0x80

#define SHT31_REG_CONTROL 0x00
#define SHT31_REG_CONFIG 0x01
#define SHT31_REG_DATALOW 0x04
#define SHT31_REG_DATAHIGH 0x05
#define SHT31_REG_ID 0x0A

#define SHT31_SAMPLE_OFF 0x00
#define SHT31_SAMPLE_SINGLE 0x2
#define SHT31_SAMPLE_CONTINUOUS 0x03

#define SHT31_HIGH_POWER 0x08
#define SHT31_LOW_POWER 0x00

#define SHT31_INT_TIME_100 0x02
#define SHT31_INT_TIME_200 0x01
#define SHT31_INT_TIME_400 0x00

#define SHT31_MULT_100 4
#define SHT31_MULT_200 2
#define SHT31_MULT_400 1

/**
 * Ranges
 */
enum SHT31_range {
	SHT31_RANGE_LOW, SHT31_RANGE_MEDIUM, SHT31_RANGE_HIGH, SHT31_RANGE_AUTO,
};
#define SHT31_RANGE_LOW_MAX 65000
#define SHT31_RANGE_MEDIUM_MAX 130000
#define SHT31_RANGE_HYSTERESIS 5000


/*
 * API FUNCTIONS
 */
int SHT31_init(int sensor);
void SHT31_close(int sensor);
int32_t SHT31_readValue(int sensor);
void SHT31_setRange(int range, int sensor);
int SHT31_getRange(int sensor);


/*
 * Helper FUNCTIONS
 */
int SHT31_initComm(int i2c_bus, int sensor);
uint8_t SHT31_getID(int sensor);
void SHT31_setParams(int sensor);
void SHT31_sendRange(int new_range, int sensor);


#endif /* SENSOR_SHT31_H_ */
