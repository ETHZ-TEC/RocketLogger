/**
 * Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group
 */

#ifndef SENSOR_SENSOR_H_
#define SENSOR_SENSOR_H_

#include <stdint.h>

#include "../rl_file.h"

#define MAX_MESSAGE_LENGTH 1000

#ifndef I2C_BUS_FILENAME
#define I2C_BUS_FILENAME "/dev/i2c-1"
#endif

/// Number of sensor registred
#define SENSOR_REGISTRY_SIZE 5

#define SENSOR_NAME_LENGTH (RL_FILE_CHANNEL_NAME_LENGTH)

/**
 * Standardized RL sensor interface definition
 */
struct rl_sensor {
    char name[SENSOR_NAME_LENGTH];
    int identifier;
    int channel;
    rl_unit unit;
    int32_t scale;
    int (*init)(int);
    void (*close)(int);
    int (*read)(int);
    int32_t (*getValue)(int, int);
};

extern struct rl_sensor sensor_registry[SENSOR_REGISTRY_SIZE];

int Sensors_initSharedBus(void);
void Sensors_closeSharedBus(void);
int Sensors_getSharedBus(void);
int Sensors_initSharedComm(uint8_t);

int Sensors_openBus(void);
int Sensors_closeBus(int);

int Sensors_scan(int*);
void Sensors_close(int*);

#endif /* SENSOR_SENSOR_H_ */
