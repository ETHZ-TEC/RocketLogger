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

#ifndef SENSOR_TSL4531_H_
#define SENSOR_TSL4531_H_

#include <stdint.h>

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
int TSL4531_init(int);
void TSL4531_close(int);
int TSL4531_read(int);
int32_t TSL4531_getValue(int, int);

int TSL4531_setRange(int, int);
int TSL4531_getRange(int);

/*
 * Helper FUNCTIONS
 */
int TSL4531_getID(void);
int TSL4531_setParameters(int);
int TSL4531_sendRange(int, int);
int TSL4531_getIndex(int);

#endif /* SENSOR_TSL4531_H_ */
