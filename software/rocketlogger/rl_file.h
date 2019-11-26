/**
 * Copyright (c) 2016-2019, ETH Zurich, Computer Engineering Group
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

#ifndef RL_FILE_H_
#define RL_FILE_H_

#include <stdint.h>
#include <stdio.h>

#include "pru.h"
#include "rl.h"
#include "rl_file.h"
#include "util.h"

/// File header magic number (ascii %RLD)
#define RL_FILE_MAGIC 0x444C5225

/// File format version of current implementation
#define RL_FILE_VERSION 0x04

/// Maximum channel description length
#define RL_FILE_CHANNEL_NAME_LENGTH 16

/// No additional range valid information available
#define RL_FILE_CHANNEL_NO_LINK (UINT16_MAX)

/// Comment alignment in bytes
#define RL_FILE_COMMENT_ALIGNMENT_BYTES sizeof(uint32_t)

/// CSV value delimiter character
#define RL_FILE_CSV_DELIMITER ","

/// Ambient sensor data file name suffix
#define RL_FILE_AMBIENT_SUFFIX "-ambient"

/// Ambient sensor read out rate in samples per second
#define RL_FILE_AMBIENT_SAMPLING_RATE 1

/// Ambient sensor data file block size in measurements
#define RL_FILE_AMBIENT_DATA_BLOCK_SIZE 1

/**
 * Channel scaling definitions
 */
#define RL_SCALE_PICO -12
#define RL_SCALE_TEN_PICO -11
#define RL_SCALE_NANO -9
#define RL_SCALE_TEN_NANO -8
#define RL_SCALE_MICRO -6
#define RL_SCALE_MILLI -3
#define RL_SCALE_UNIT 0
#define RL_SCALE_KILO 3
#define RL_SCALE_MEGA 6
#define RL_SCALE_GIGA 9
#define RL_SCALE_TERA 12

/**
 * Data unit definition
 */
enum rl_unit {
    RL_UNIT_UNITLESS = 0,           //!< Unitless
    RL_UNIT_VOLT = 1,               //!< Voltage (electric)
    RL_UNIT_AMPERE = 2,             //!< Current (electric)
    RL_UNIT_BINARY = 3,             //!< Binary signal
    RL_UNIT_RANGE_VALID = 4,        //!< Range valid information
    RL_UNIT_LUX = 5,                //!< Lux (illuminance)
    RL_UNIT_DEG_C = 6,              //!< Degree celsius (temperature)
    RL_UNIT_INTEGER = 7,            //!< Integer channel (numeric)
    RL_UNIT_PERCENT = 8,            //!< Percent (numeric, humidity)
    RL_UNIT_PASCAL = 9,             //!< Pascal (pressure)
    RL_UNIT_SECOND = 10,            //!< Second (time delta)
    RL_UNIT_UNDEFINED = 0xffffffff, //!< Undefined unit
};

/**
 * Type definition for RocketLogger data unit.
 */
typedef enum rl_unit rl_unit_t;

/**
 * File header lead in (constant size) definition for the binary file.
 */
struct rl_file_lead_in {
    /// File magic constant
    uint32_t file_magic;
    /// File version number
    uint16_t file_version;
    /// Total size of the header in bytes
    uint16_t header_length;
    /// Size of the data blocks in the file in rows
    uint32_t data_block_size;
    /// Number of data blocks stored in the file
    uint32_t data_block_count;
    /// Total sample count
    uint64_t sample_count;
    /// Sampling rate of the measurement
    uint16_t sample_rate;
    /// Instrument ID (mac address)
    uint8_t mac_address[MAC_ADDRESS_LENGTH];
    /// Start time of the measurement in UNIX time, UTC
    rl_timestamp_t start_time;
    /// Comment length
    uint32_t comment_length;
    /// Binary channel count
    uint16_t channel_bin_count;
    /// Analog channel count
    uint16_t channel_count;
};

/**
 * Typedef for RocketLogger file head lead in
 */
typedef struct rl_file_lead_in rl_file_lead_in_t;

/**
 * Channel definition for the binary file header.
 */
struct rl_file_channel {
    /// Channel unit
    rl_unit_t unit;
    /// Channel scale (in power of ten, for voltage and current)
    int32_t channel_scale;
    /// Datum size in bytes (for voltage and current)
    uint16_t data_size;
    /// Link to channel valid data (for low-range current channels)
    uint16_t valid_data_channel;
    /// Channel name/description
    char name[RL_FILE_CHANNEL_NAME_LENGTH];
};

/**
 * Typedef for RocketLogger file header channel description.
 */
typedef struct rl_file_channel rl_file_channel_t;

/**
 * File header definition for the binary file.
 */
struct rl_file_header {
    /// File header lead in (constant size)
    rl_file_lead_in_t lead_in;
    /// Comment field
    char const *comment;
    /// Channels definitions (binary and normal)
    rl_file_channel_t *channel;
};

/**
 * Typedef for RocketLogger file header.
 */
typedef struct rl_file_header rl_file_header_t;

/**
 * Derive the ambient file name from the data file name.
 *
 * @param data_file_name The data file name
 * @return Pointer to the buffer of the derived ambient file name
 */
char *rl_file_get_ambient_file_name(char const *const data_file_name);

/**
 * Set up data file header lead-in with current configuration.
 *
 * @param lead_in The file lead-in data structure to set up
 * @param config Current measurement configuration
 */
void rl_file_setup_data_lead_in(rl_file_lead_in_t *const lead_in,
                                rl_config_t const *const config);

/**
 * Set up ambient file header lead-in with current configuration.
 *
 * @param lead_in The file lead-in data structure to set up
 * @param config Current measurement configuration
 */
void rl_file_setup_ambient_lead_in(rl_file_lead_in_t *const lead_in,
                                   rl_config_t const *const config);

/**
 * Set up data file header with current configuration.
 *
 * @param file_header The file header data structure to set up
 * @param config Current measurement configuration
 */
void rl_file_setup_data_header(rl_file_header_t *const file_header,
                               rl_config_t const *const config);

/**
 * Set up ambient file header with current configuration.
 *
 * @param file_header The file header data structure to set up
 * @param config Current measurement configuration
 */
void rl_file_setup_ambient_header(rl_file_header_t *const file_header,
                                  rl_config_t const *const config);

/**
 * Store file header to file (in binary format).
 *
 * @param file_handle Data file to write to
 * @param file_header The file header data structure to store to the file
 */
void rl_file_store_header_bin(FILE *file_handle,
                              rl_file_header_t *const file_header);

/**
 * Store file header to file (in CSV format).
 *
 * @param file_handle Data file to write to
 * @param file_header The file header data structure to store to the file
 */
void rl_file_store_header_csv(FILE *file_handle,
                              rl_file_header_t const *const file_header);

/**
 * Update file with new header lead-in (to write current sample count) in binary
 * format.
 *
 * @param file_handle Data file to write to
 * @param file_header The file header data structure to store to the file
 */
void rl_file_update_header_bin(FILE *file_handle,
                               rl_file_header_t const *const file_header);

/**
 * Update file with new header lead-in (to write current sample count) in CSV
 * format.
 *
 * @param file_handle Data file to write to
 * @param file_header The file header data structure to store to the file
 */
void rl_file_update_header_csv(FILE *file_handle,
                               rl_file_header_t const *const file_header);

/**
 * Handle the sampling data buffer to add a new block to the data file.
 *
 * @param data_file Data file to write to
 * @param buffer PRU data buffer to process
 * @param buffer_size Number of samples in the buffer
 * @param timestamp_realtime Timestamp sampled from realtime clock
 * @param timestamp_monotonic Timestamp sampled from monotonic clock
 * @param config Current measurement configuration
 * @return Returns the number of data blocks written to the file, negative on
 * failure with errno set accordingly
 */
int rl_file_add_data_block(FILE *data_file, pru_buffer_t const *const buffer,
                           uint32_t buffer_size,
                           rl_timestamp_t const *const timestamp_realtime,
                           rl_timestamp_t const *const timestamp_monotonic,
                           rl_config_t const *const config);

/**
 * Handle the sampling data buffer to add a new block to the ambient file.
 *
 * @param ambient_file Ambient file to write to
 * @param buffer PRU data buffer to process
 * @param buffer_size Number of samples in the buffer
 * @param timestamp_realtime Timestamp sampled from realtime clock
 * @param timestamp_monotonic Timestamp sampled from monotonic clock
 * @param config Current measurement configuration
 * @return Returns the number of data blocks written to the file, negative on
 * failure with errno set accordingly
 */
int rl_file_add_ambient_block(FILE *ambient_file,
                              pru_buffer_t const *const buffer,
                              uint32_t buffer_size,
                              rl_timestamp_t const *const timestamp_realtime,
                              rl_timestamp_t const *const timestamp_monotonic,
                              rl_config_t const *const config);

#endif /* RL_FILE_H_ */
