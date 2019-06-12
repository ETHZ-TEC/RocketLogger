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

#ifndef RL_H_
#define RL_H_

#include <stdbool.h>
#include <stdint.h>

#include "version.h"

/// Function return value for successful completion
#define SUCCESS (0)

/// Function return value for errors (use errno to indicate the error)
#define ERROR (-1)

/// Default system configuration file path
#define RL_CONFIG_VERSION 0x03

/// Default system configuration file path
#define RL_CONFIG_SYSTEM_FILE "/etc/rocketlogger/settings.dat"

/// User configuration file path
#define RL_CONFIG_USER_FILE                                                    \
    "/home/rocketlogger/.config/rocketlogger/settings.dat"

/// Configuration channel enable default
#define RL_CONFIG_DEFAULT_CHANNEL_ENABLE                                       \
    { true, true, true, true, true, true, true, true }
/// Configuration channel force range default
#define RL_CONFIG_DEFAULT_CHANNEL_FORCE_RANGE                                  \
    { false, false }
/// Configuration file name default
#define RL_CONFIG_DEFAULT_FILE "/var/www/rocketlogger/data/data.rld"
/// Configuration file commment default
#define RL_CONFIG_DEFAULT_COMMENT "Sampled using the RocketLogger"

/// Maximum path length in characters
#define RL_PATH_LENGTH_MAX 256
/// Number of RocketLogger analog channels
#define RL_CHANNEL_COUNT 8
/// Number of RocketLogger switched channels (allowing to force range)
#define RL_CHANNEL_SWITCHED_COUNT 2
/// Number of RocketLogger digtial channels
#define RL_CHANNEL_DIGITAL_COUNT 6
/// Maximum number of sensors that can be connected to the system
#define RL_SENSOR_COUNT_MAX 128

/// Configuration channel indexes
#define RL_CONFIG_CHANNEL_V1 0
#define RL_CONFIG_CHANNEL_V2 1
#define RL_CONFIG_CHANNEL_V3 2
#define RL_CONFIG_CHANNEL_V4 3
#define RL_CONFIG_CHANNEL_I1L 4
#define RL_CONFIG_CHANNEL_I1H 5
#define RL_CONFIG_CHANNEL_I2L 6
#define RL_CONFIG_CHANNEL_I2H 7

/// Configuration merged/forced channel indexes
#define RL_CONFIG_CHANNEL_I1 0
#define RL_CONFIG_CHANNEL_I2 1

/**
 * RocketLogger data aggregation modes.
 */
enum rl_aggregation_mode {
    RL_AGGREGATION_MODE_DOWNSAMPLE = 1, /// Aggregate using down sampling
    RL_AGGREGATION_MODE_AVERAGE = 2,    /// Aggregate by averaging samples
};

/**
 * Type definition for RocketLogger data aggregation mode.
 */
typedef enum rl_aggregation_mode rl_aggregation_mode_t;

/**
 * RocketLogger file formats.
 */
enum rl_file_format {
    RL_FILE_FORMAT_CSV, /// CSV format
    RL_FILE_FORMAT_RLD, /// RLD binary format
};

/**
 * Type definition for RocketLogger file format.
 */
typedef enum rl_file_format rl_file_format_t;

/**
 * RocketLogger sampling modes.
 */
enum rl_sampling_mode {
    RL_SAMPLING_MODE_FINITE,     /// Finite sampling mode for sampling a fixed
                                 /// number of sample
    RL_SAMPLING_MODE_CONTINUOUS, /// Continuous sampling mode
    RL_SAMPLING_MODE_METER,      /// Meter mode in console window
};

/**
 * Type definition for RocketLogger sampling mode.
 */
typedef enum rl_sampling_mode rl_sampling_mode_t;

/**
 * RocketLogger sampling configuration.
 */
struct rl_config {
    /// Configuration structure version
    uint8_t config_version;
    /// Sampling mode
    rl_sampling_mode_t sampling_mode;
    /// Sample limit (0 for continuous)
    uint64_t sample_limit;
    /// Sampling rate
    uint32_t sample_rate;
    /// Data update rate
    uint32_t update_rate;
    /// Channels to sample
    bool channel_enable[RL_CHANNEL_COUNT];
    /// Current channels to force to high range
    bool channel_force_range[RL_CHANNEL_SWITCHED_COUNT];
    /// Sample aggregation mode (for sampling rates below lowest native one)
    rl_aggregation_mode_t aggregation_mode;
    /// Enable digital inputs
    bool digital_enable;
    /// Enable web interface connection
    bool web_enable;
    /// Perform calibration measurement (ignore existing calibration)
    bool calibration_ignore;
    /// Enable logging of ambient sensor
    bool ambient_enable;
    /// Enable storing measurements to file
    bool file_enable;
    /// Data file name
    char file_name[RL_PATH_LENGTH_MAX];
    /// File format
    rl_file_format_t file_format;
    /// Maximum data file size
    uint64_t file_size;
    /// File comment
    char const *file_comment;
};

/**
 * Type definition for RocketLogger configuration.
 */
typedef struct rl_config rl_config_t;

/**
 * RocketLogger status structure definition.
 */
struct rl_status {
    /// Sampling state, true: sampling, false: idle
    bool sampling;
    /// Whether the logger is in an error state
    bool error;
    /// Number of samples taken
    uint64_t sample_count;
    /// Number of buffers taken
    uint32_t buffer_count;
    /// Time stamp of last calibration run
    uint64_t calibration_time;
    /// Time stamp of last calibration run
    char *const calibration_file;
    /// Time stamp of last calibration run
    uint64_t disk_free;
    /// Time stamp of last calibration run
    uint16_t disk_free_permille;
    /// Disk space in bytes required per minute when sampling
    uint32_t disk_use_rate;
    /// Number of sensors found connected to the system
    uint16_t sensor_count;
    /// Identifiers of sensors found
    int32_t sensor_index[RL_SENSOR_COUNT_MAX];
    /// Current configuration
    rl_config_t const *config;
};

/**
 * Type definition for RocketLogger configuration.
 */
typedef struct rl_status rl_status_t;

/// RocketLogger channel names sorted by name
extern const char *RL_CHANNEL_NAMES[RL_CHANNEL_COUNT];

/// RocketLogger channel names sorted by internally used index
// extern const char *RL_CHANNEL_INDEX_NAMES[RL_CHANNEL_COUNT];

/// RocketLogger force range channel names
extern const char *RL_CHANNEL_FORCE_NAMES[RL_CHANNEL_SWITCHED_COUNT];

/// RocketLogger digital channel names
extern const char *RL_CHANNEL_DIGITAL_NAMES[RL_CHANNEL_DIGITAL_COUNT];

/// RocketLogger valid channel names
extern const char *RL_CHANNEL_VALID_NAMES[RL_CHANNEL_SWITCHED_COUNT];

/**
 * Print RocketLogger configuration as text output.
 *
 * @param config Pointer to {@link rl_config_t} configuration
 */
void rl_config_print(rl_config_t const *const config);

/**
 * Print RocketLogger command line string correpsinding to the current
 * configuration.
 *
 * @param config Pointer to {@link rl_config_t} configuration
 */
void rl_config_print_cmd(rl_config_t const *const config);

/**
 * Print RocketLogger configuration as JSON data structure.
 *
 * @param config Pointer to {@link rl_config_t} configuration
 */
void rl_config_print_json(rl_config_t const *const config);

/**
 * Reset configuration to standard values.
 *
 * @param config Pointer to {@link rl_config_t} configuration
 */
void rl_config_reset(rl_config_t *const config);

/**
 * Read default configuration from file.
 *
 * Try to read user configuration otherwis fallback to system configuration.
 *
 * @param config Pointer to {@link rl_config_t} configuration to write to
 * @return Returns 0 on success, negative on failure with errno set accordingly
 */
int rl_config_read_default(rl_config_t *const config);

/**
 * Write provided configuration as default to file.
 *
 * @param config Pointer to {@link rl_config_t} configuration to write
 * @return Returns 0 on success, negative on failure with errno set accordingly
 */
int rl_config_write_default(rl_config_t const *const config);

/**
 * Validate RocketLogger configuration.
 *
 * @param config Pointer to {@link rl_config_t} configuration to write
 * @return Returns 0 on success, negative on failure with errno set accordingly
 */
int rl_config_validate(rl_config_t const *const config);

/**
 * Reset RocketLogger status to standard values.
 *
 * @param status Pointer to {@link rl_status_t} configuration
 */
void rl_status_reset(rl_status_t *const status);

/// Global RocketLogger status variable.
extern rl_status_t status;

#endif /* RL_H_ */