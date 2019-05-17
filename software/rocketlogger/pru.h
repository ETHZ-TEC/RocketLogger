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

#ifndef PRU_H_
#define PRU_H_

#include <stdint.h>
#include <stdio.h>

#include "types.h"

/// PRU binary file location
#define PRU_BINARY_FILE "/lib/firmware/rocketlogger.bin"
/// Memory map file
#define PRU_MMAP_SYSFS_PATH "/sys/class/uio/uio0/maps/map1/"

/// Sample size definition
#define PRU_SAMPLE_SIZE 4

/// Mask for valid bit read from PRU
#define PRU_VALID_MASK 0x1
/// Mask for binary inputs read from PRU
#define PRU_BINARY_MASK 0xE

/// PRU time out in seconds
#define PRU_TIMEOUT 3

/// Number of ADC commands
#define PRU_ADC_COMMAND_COUNT 12

/**
 * PRU state definition
 */
typedef enum pru_state {
    PRU_OFF = 0x00,       //!< PRU off
    PRU_FINITE = 0x01,    //!< Finite sampling mode
    PRU_CONTINUOUS = 0x03 //!< Continuous sampling mode
} pru_state_t;

/**
 * Struct for data exchange with PRU
 */
struct pru_data {
    /// Current PRU state
    pru_state_t state;
    /// Pointer to shared buffer 0
    uint32_t buffer0_ptr;
    /// Pointer to shared buffer 1
    uint32_t buffer1_ptr;
    /// Shared buffer length in number of data elements
    uint32_t buffer_length;
    /// Samples to take (0 for continuous)
    uint32_t sample_limit;
    /// ADC precision (in bit)
    uint32_t adc_precision;
    /// Number of ADC commands to send
    uint32_t adc_command_count;
    /// ADC commands to send: command starts in MSB, optional bytes
    /// (e.g. register address and values) aligned in degreasing byte order
    uint32_t adc_command[PRU_ADC_COMMAND_COUNT];
};

/**
 * Typedef for PRU data exchange structure
 */
typedef struct pru_data pru_data_t;

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
 * Halts the PRU, unmaps PRU shared memory and disables PRU interrupts.
 */
void pru_deinit(void);

/**
 * PRU data structure initialization.
 *
 * @param pru_data {@link pru_data_t} data structure to initialize
 * @param config Pointer to current {@link rl_config_t} configuration
 * @param aggregates Number of samples to aggregate for sampling rates smaller
 * than the minimal ADC rate (set 1 for no aggregates)
 * @return {@link SUCCESS} on success, {@link FAILURE} otherwise
 */
int pru_data_init(pru_data_t *const pru_data, rl_config_t const *const config,
                  uint32_t aggregates);

/**
 * Write a new state to the PRU shared memory.
 *
 * @param state The PRU state to write
 * @return Number of bytes written, negative value on error.
 */
int pru_set_state(pru_state_t state);

/**
 * Wait for a PRU event with timeout
 *
 * @param event PRU event to wait for
 * @param timeout Time out in seconds
 * @return Zero on success, error code otherwise, see also
 * pthread_cond_timedwait() documentation
 */
int pru_wait_event_timeout(unsigned int event, unsigned int timeout);

/**
 * Main PRU sampling routine.
 *
 * Configures and runs the actual RocketLogger measurements
 *
 * @param data_file File pointer to data file
 * @param ambient_file File pointer to ambient file
 * @param config Pointer to current {@link rl_config_tig_t} configuration
 * @param file_comment Comment to store in the file header
 * @return {@link SUCCESS} on success, {@link FAILURE} otherwise
 */
int pru_sample(FILE *data, FILE *ambient_file, rl_config_t const *const config,
               char const *const file_comment);

/**
 * Stop running PRU measurements.
 *
 * @note When sampling in continuous mode, this has to be called before {@link
 * pru_deinit}.
 */
void pru_stop(void);

#endif /* PRU_H_ */
