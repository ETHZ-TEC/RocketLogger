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

// ------  ADC DEFINITIONS  ------ //

/**
 * ADS131E08S command (extended to 32 bits for PRU use) definitions
 */
#define WAKEUP 0x02000000
#define STANDBY 0x04000000
#define RESET 0x06000000
#define START 0x08000000
#define STOP 0x0A000000
#define OFFSETCAL 0x1A000000
#define RDATAC 0x10000000
#define SDATAC 0x11000000
#define RDATA 0x12000000
#define RREG 0x20000000
#define WREG 0x40000000

/**
 * ADS131E08S register definitions
 */
#define ID 0x00000000
#define CONFIG1 0x01000000
#define CONFIG2 0x02000000
#define CONFIG3 0x03000000
#define CH1SET 0x05000000
#define CH2SET 0x06000000
#define CH3SET 0x07000000
#define CH4SET 0x08000000
#define CH5SET 0x09000000
#define CH6SET 0x0A000000
#define CH7SET 0x0B000000
#define CH8SET 0x0C000000

/**
 * ADS131E08S gain settings
 */
#define GAIN1 0x1000
#define GAIN2 0x2000
#define GAIN12 0x6000

/**
 * ADS131E08S sampling rates
 */
#define K1 0x0600
#define K2 0x0500
#define K4 0x0400
#define K8 0x0300
#define K16 0x0200
#define K32 0x0100
#define K64 0x0000

/**
 * ADS131E08S configuration default value defines
 */
#define CONFIG1DEFAULT 0x9000
#define CONFIG2DEFAULT 0xE000
#define CONFIG3DEFAULT 0xE800

/// PRU binary file location
#define PRU_BINARY_FILE "/lib/firmware/rocketlogger.bin"
/// Memory map file
#define PRU_MMAP_SYSFS_PATH "/sys/class/uio/uio0/maps/map1/"

/**
 * ADS131E08S precision defines({@link PRU_PRECISION_HIGH} for low sampling
 * rates, {@link PRU_PRECISION_LOW} for high ones)
 */
#define PRU_PRECISION_HIGH 24
#define PRU_PRECISION_LOW 16

/**
 * Sample size definition
 */
#define PRU_SAMPLE_SIZE 4

/// Mask for valid bit read from PRU
#define PRU_VALID_MASK 0x1
/// Mask for binary inputs read from PRU
#define PRU_BINARY_MASK 0xE

/// PRU time out in seconds
#define PRU_TIMEOUT 3

/**
 * PRU state definition
 */
typedef enum pru_state {
    PRU_OFF = 0x00,       //!< PRU off
    PRU_FINITE = 0x01,    //!< Finite sampling mode
    PRU_CONTINUOUS = 0x03 //!< Continuous sampling mode
} pru_state_t;

/// Number of ADC commands
#define PRU_ADC_COMMAND_COUNT 12

/**
 * Struct for data exchange with PRU
 */
typedef struct pru_data {
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
    /// ADC commands to send
    uint32_t adc_command[PRU_ADC_COMMAND_COUNT];
} pru_data_t;

// ------  FUNCTIONS  ------ //

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
 * @param pru {@link pru_data_t} data structure to initialize
 * @param conf Pointer to current {@link rl_conf} configuration
 * @param aggregates Number of samples to aggregate for sampling rates smaller
 * than the minimal ADC rate (set 1 for no aggregates)
 * @return {@link SUCCESS} on success, {@link FAILURE} otherwise
 */
int pru_init_data(pru_data_t *pru, struct rl_conf *conf, uint32_t aggregates);

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
 * @param conf Pointer to current {@link rl_conf} configuration
 * @param file_comment Comment to store in the file header
 * @return {@link SUCCESS} on success, {@link FAILURE} otherwise
 */
int pru_sample(FILE *data, FILE *ambient_file, struct rl_conf *conf,
               char *file_comment);

/**
 * Stop running PRU measurements.
 *
 * @note When sampling in continuous mode, this has to be called before {@link
 * pru_close}.
 */
void pru_stop(void);

#endif /* PRU_H_ */
