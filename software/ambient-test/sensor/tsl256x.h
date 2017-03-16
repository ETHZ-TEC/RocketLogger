/**
 * Header for TSL256x light sensor interfacing.
 *
 * Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group
 *
 */

#ifndef SENSOR_TSL256X_H_
#define SENSOR_TSL256X_H_

#include <stdint.h>

// TSL256x I2C address (0x29, 0x39, or 0x49, depending of hardware config)
#define TSL256X_I2C_ADDRESS 0x29

// TSL256x command bit definitions
#define TSL256X_CMD_CMD 0x80
#define TSL256X_CMD_CLEAR 0x40
#define TSL256X_CMD_WORD 0x20
#define TSL256X_CMD_BLOCK 0x10
#define TSL256X_CMD_ADDR 0x0f

// TSL256x register address definitions
#define TSL256X_REG_CONTROL 0x00
#define TSL256X_REG_TIMING 0x01
#define TSL256X_REG_THRESHLOWLOW 0x02
#define TSL256X_REG_THRESHLOWHIGH 0x03
#define TSL256X_REG_THRESHHIGHLOW 0x04
#define TSL256X_REG_THRESHHIGHHIGH 0x05
#define TSL256X_REG_INTERRUPT 0x06
#define TSL256X_REG_CRC 0x08
#define TSL256X_REG_ID 0x0a
#define TSL256X_REG_DATA0LOW 0x0c
#define TSL256X_REG_DATA0HIGH 0x0d
#define TSL256X_REG_DATA1LOW 0x0e
#define TSL256X_REG_DATA1HIGH 0x0f

// TSL256x register bit definitions
#define TSL256X_CONTROL_POWER 0x03

#define TSL256X_TIMING_INTEG_MASK 0x03
#define TSL256X_TIMING_INTEG_13_7MS 0x00
#define TSL256X_TIMING_INTEG_101MS 0x01
#define TSL256X_TIMING_INTEG_402MS 0x02
#define TSL256X_TIMING_MANUAL 0x08
#define TSL256X_TIMING_GAIN_1 0x10
#define TSL256X_TIMING_GAIN_16 0x00

#define TSL256X_INTERRUPT_PERSIST_MASK 0x0f
#define TSL256X_INTERRUPT_DISABLED 0x00
#define TSL256X_INTERRUPT_LEVEL 0x10
#define TSL256X_INTERRUPT_SMBUS 0x20
#define TSL256X_INTERRUPT_TEST 0x30

#define TSL256X_ID_TSL2561 0x10
#define TSL256X_ID_PACKAGE 0x40

int TSL256x_init(int);
int TSL256x_initComm(int);
int TSL256x_getID(int, uint8_t*);
int TSL256x_getLux(int, double*);
int TSL256x_getTiming(int, uint8_t*);
int TSL256x_setTiming(int, uint8_t, uint8_t);
int TSL256x_readValues(int, uint16_t*);
double TSL256x_calculateLux(uint16_t, uint16_t);

#endif /* SENSOR_TSL256X_H_ */
