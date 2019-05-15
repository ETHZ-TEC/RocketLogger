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

extern const struct rl_sensor sensor_registry[SENSOR_REGISTRY_SIZE];

int Sensors_initSharedBus(void);
void Sensors_closeSharedBus(void);
int Sensors_getSharedBus(void);
int Sensors_initSharedComm(uint8_t);

int Sensors_openBus(void);
int Sensors_closeBus(int);

int Sensors_scan(int *);
void Sensors_close(int *);

#endif /* SENSOR_SENSOR_H_ */
