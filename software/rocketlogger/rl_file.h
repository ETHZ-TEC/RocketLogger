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
 * ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef RL_FILE_H_
#define RL_FILE_H_

#include <stdint.h>

#include "types.h"
#include "util.h"

// Defines

/**
 * Channel scaling definitions
 */
#define RL_SCALE_PICO -12
#define RL_SCALE_TEN_PICO -11
#define RL_SCALE_NANO -9
#define RL_SCALE_TEN_NANO -8
#define RL_SCALE_MICRO -6
#define RL_SCALE_MILLI -3
#define RL_SCALE_NONE 0
#define RL_SCALE_KILO 3
#define RL_SCALE_MEGA 6
#define RL_SCALE_GIGA 9
#define RL_SCALE_TERA 12

// Constants

/// File header magic number (ascii %RLD)
#define RL_FILE_MAGIC 0x444C5225 // const uint32_t RL_FILE_MAGIC = 0x25524C42;

/// File format version of current implementation
#define RL_FILE_VERSION 0x03 // const uint8_t RL_FILE_VERSION = 0x01;

/// Maximum channel description length
#define RL_FILE_CHANNEL_NAME_LENGTH                                            \
    16 // const uint8_t RL_FILE_CHANNEL_NAME_LENGTH = 16;

/// No additional range valid information available
#define NO_VALID_DATA 0xFFFF // const uint16_t NO_VALID_DATA = 0xFFFF;

/// Comment for file header
#define RL_FILE_COMMENT "This is a comment"

/// Comment alignment in bytes
#define RL_FILE_COMMENT_ALIGNMENT_BYTES sizeof(uint32_t)

// Types

/**
 * Data unit definition
 */
typedef enum unit {
    RL_UNIT_UNITLESS = 0,           //!< Unitless
    RL_UNIT_VOLT = 1,               //!< Voltage (electric)
    RL_UNIT_AMPERE = 2,             //!< Current (electric)
    RL_UNIT_BINARY = 3,             //!< Binary signal
    RL_UNIT_RANGE_VALID = 4,        //!< Range valid information
    RL_UNIT_LUX = 5,                //!< Lux (illuminance)
    RL_UNIT_DEG_C = 6,              //!< Degree celcius (temperature)
    RL_UNIT_INTEGER = 7,            //!< Integer channel (numeric)
    RL_UNIT_PERCENT = 8,            //!< Percent (numeric, humidity)
    RL_UNIT_PASCAL = 9,             //!< Pascal (preasure)
    RL_UNIT_UNDEFINED = 0xffffffff, //!< Undefined unit
} rl_unit;

/**
 * File header lead in (constant size) definition for the binary file.
 */
struct rl_file_lead_in {
    /// File magic constant
    uint32_t magic; // = RL_FILE_MAGIC;

    /// File version number
    uint16_t file_version; // = RL_FILE_VERSION;

    /// Total size of the header in bytes
    uint16_t header_length; // = 0;

    /// Size of the data blocks in the file in rows
    uint32_t data_block_size; // = 0;

    /// Number of data blocks stored in the file
    uint32_t data_block_count; // = 0;

    /// Total sample count
    uint64_t sample_count; // = 0;

    /// Sampling rate of the measurement
    uint16_t sample_rate; // = 0;

    /// Instrument ID (mac address)
    uint8_t mac_address[MAC_ADDRESS_LENGTH];

    /// Start time of the measurement in UNIX time, UTC
    struct time_stamp start_time; // = 0;

    /// Comment length
    uint32_t comment_length; // = 0;

    /// Binary channel count
    uint16_t channel_bin_count; // = 0;

    /// Analog channel count
    uint16_t channel_count; // = 0;
};

/**
 * Channel definition for the binary file header.
 */
struct rl_file_channel {

    /// Channel unit
    rl_unit unit; // = RL_UNIT_UNDEFINED;

    /// Channel scale (in power of ten, for voltage and current)
    int32_t channel_scale; // = RL_SCALE_NONE;

    /// Datum size in bytes (for voltage and current)
    uint16_t data_size; // = 0;

    /// Link to channel valid data (for low-range current channels)
    uint16_t valid_data_channel; // = NO_VALID_DATA;

    /// Channel name/description
    char name[RL_FILE_CHANNEL_NAME_LENGTH];
};

/**
 * File header definition for the binary file.
 */
struct rl_file_header {

    /// File header lead in (constant size)
    struct rl_file_lead_in lead_in;

    /// Comment field
    char* comment; // = NULL;

    /// Channels definitions (binary and normal)
    struct rl_file_channel* channel; // = NULL;
};

#endif /* RL_FILE_H_ */
