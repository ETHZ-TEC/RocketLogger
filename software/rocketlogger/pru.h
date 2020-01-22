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

#ifndef PRU_H_
#define PRU_H_

#include <stdint.h>
#include <stdio.h>

#include "rl.h"

/// PRU binary file location
#define PRU_BINARY_FILE "/lib/firmware/rocketlogger.bin"
/// Memory map file
#define PRU_MMAP_SYSFS_PATH "/sys/class/uio/uio0/maps/map1/"

/// Overall size of FRU digital channels in bytes
#define PRU_DIGITAL_SIZE 4
/// Size of PRU channel data in bytes
#define PRU_SAMPLE_SIZE 4
/// Size of PRU buffer status in bytes
#define PRU_BUFFER_STATUS_SIZE 4

/// Mask for valid bit read from PRU
#define PRU_VALID_MASK 0x1
/// Mask for binary inputs read from PRU
#define PRU_BINARY_MASK 0xE

// /**
//  * Digital channel bit position in PRU digital information
//  */
#define PRU_DIGITAL_I1L_VALID_MASK 0x01
#define PRU_DIGITAL_I2L_VALID_MASK 0x01
#define PRU_DIGITAL_INPUT1_MASK 0x02
#define PRU_DIGITAL_INPUT2_MASK 0x04
#define PRU_DIGITAL_INPUT3_MASK 0x08
#define PRU_DIGITAL_INPUT4_MASK 0x02
#define PRU_DIGITAL_INPUT5_MASK 0x04
#define PRU_DIGITAL_INPUT6_MASK 0x08

/// PRU time out in micro seconds
#define PRU_TIMEOUT_US 2000000

/// Number of ADC commands
#define PRU_ADC_COMMAND_COUNT 12

// #define PRU_STATE_OFF 0x00
// #define PRU_STATE_SAMPLING 0x01

/**
 * PRU state definition
 */
enum pru_state {
    PRU_STATE_OFF = 0x00,               /// PRU off
    PRU_STATE_SAMPLE_FINITE = 0x01,     /// PRU sampling in finite mode
    PRU_STATE_SAMPLE_CONTINUOUS = 0x03, /// PRU sampling in continuous mode
};

/**
 * Typedef for PRU state
 */
typedef enum pru_state pru_state_t;

/**
 * Struct for controlling the PRU
 */
struct pru_control {
    /// Current PRU state
    pru_state_t state;
    /// Memory address of the shared buffer 0
    uint32_t buffer0_addr;
    /// Memory address of the shared buffer 1
    uint32_t buffer1_addr;
    /// Shared buffer length in number of data elements
    uint32_t buffer_length;
    /// Samples to take (0 for continuous)
    uint32_t sample_limit;
    /// ADC sample rate (in kSPS)
    uint32_t adc_sample_rate;
    /// Number of ADC commands to send
    uint32_t adc_command_count;
    /// ADC commands to send: command starts in MSB, optional bytes
    /// (e.g. register address and values) aligned in degreasing byte order
    uint32_t adc_command[PRU_ADC_COMMAND_COUNT];
};

/**
 * Typedef for PRU control data structure
 */
typedef struct pru_control pru_control_t;

/**
 * PRU channel data block structure
 */
struct pru_data {
    /// bit map of digital channels
    uint32_t channel_digital;
    /// analog channel data
    int32_t channel_analog[RL_CHANNEL_COUNT];
};

/**
 * Typedef for PRU channel data block structure
 */
typedef struct pru_data pru_data_t;

/**
 * PRU data buffer structure
 */
struct pru_buffer {
    /// buffer index
    uint32_t index;
    /// the data blocks
    pru_data_t const data[];
};

/**
 * Typedef for PRU data buffer structure
 */
typedef struct pru_buffer pru_buffer_t;

/**
 * Initialize PRU driver.
 *
 * Map PRU shared memory and enable PRU interrupts.
 *
 * @return {@link SUCCESS} on success, {@link FAILURE} otherwise
 */
int pru_init(void);

/**
 * Shutdown PRU and deinitialize PRU driver.
 *
 * Halt the PRU, unmap PRU shared memory and disable PRU interrupts.
 */
void pru_deinit(void);

/**
 * PRU data structure initialization.
 *
 * @param pru_control PRU data structure to initialize
 * @param config Current measurement configuration
 * @param aggregates Number of samples to aggregate for sampling rates smaller
 * than the minimal ADC rate (set 1 for no aggregates)
 * @return Returns 0 on success, negative on failure with errno set accordingly
 */
int pru_control_init(pru_control_t *const pru_control,
                     rl_config_t const *const config, uint32_t aggregates);

/**
 * Write a new state to the PRU shared memory.
 *
 * @param state The PRU state to write
 * @return Returns number of bytes written, negative on failure with errno set
 * accordingly
 */
int pru_set_state(pru_state_t state);

/**
 * Main PRU sampling routine.
 *
 * Configures and runs the actual RocketLogger measurements.
 *
 * @param data_file Data file to write to
 * @param ambient_file Ambient file to write to
 * @param config Current measurement configuration
 * @return Returns 0 on success, negative on failure with errno set accordingly
 */
int pru_sample(FILE *data_file, FILE *ambient_file,
               rl_config_t const *const config);

/**
 * Stop running PRU measurements.
 *
 * @note When sampling in continuous mode, this has to be called before {@link
 * pru_deinit}.
 */
void pru_stop(void);

#endif /* PRU_H_ */
