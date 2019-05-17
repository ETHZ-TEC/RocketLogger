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

#ifndef TYPES_H_
#define TYPES_H_

#include <stdbool.h>
#include <stdint.h>

/**
 * Return code definitions
 */
#define SUCCESS 1
#define UNDEFINED 0
#define FAILURE -1
#define TIMEOUT 0

/// Process ID file for background process
#define PID_FILE "/var/run/rocketlogger.pid"

/// Log file name
#ifndef LOG_FILE
#define LOG_FILE "/var/www/rocketlogger/log/rocketlogger.log"
#endif

/// Log file size in bytes
#define MAX_LOG_FILE_SIZE 1000000

/// Key for status shared memory (used for creation)
#define SHMEM_STATUS_KEY 1111
/// Key for web shared memory (used for creation)
#define SHMEM_DATA_KEY 4443
/// Permissions for shared memory
#define SHMEM_PERMISSIONS 0666

/// Maximum path length in characters
#define MAX_PATH_LENGTH 256
/// Maximum number of RocketLogger channels
#define NUM_CHANNELS 8
/// Maximum number of RocketLogger current channels
#define NUM_I_CHANNELS 2
/// Maximum number of RocketLogger voltage channels
#define NUM_V_CHANNELS 4
/// Number of RocketLogger digital channels
#define NUM_DIGITAL_INPUTS 6
/// Data update rate in {@link METER} mode
#define METER_UPDATE_RATE 5
/// Size of PRU digital information in bytes
#define PRU_DIG_SIZE 4
/// Size of PRU buffer status in bytes
#define PRU_BUFFER_STATUS_SIZE 4

/// KSPS <-> SPS conversion factor
#define KSPS 1000

/**
 * Digital channel bit position in PRU digital information
 */
#define I1L_VALID_BIT 1
#define I2L_VALID_BIT 1
#define DIGIN1_BIT 2
#define DIGIN2_BIT 4
#define DIGIN3_BIT 8
#define DIGIN4_BIT 2
#define DIGIN5_BIT 4
#define DIGIN6_BIT 8

/// Minimal ADC sampling rate
#define MIN_ADC_RATE 1000

/// RocketLogger configuration structure version number
#define RL_CONF_VERSION 0x02

// AMBIENT CONF //
#define AMBIENT_MAX_SENSOR_COUNT 128

// channel indices in channels array
/**
 * Channel indices in channel array
 */
#define I1H_INDEX 0
#define I1L_INDEX 1
#define V1_INDEX 2
#define V2_INDEX 3
#define I2H_INDEX 4
#define I2L_INDEX 5
#define V3_INDEX 6
#define V4_INDEX 7

// SEMAPHORES
/// Semaphore key (used for set creation)
#define SEM_KEY 2222
/// Number of semaphores in set
#define NUM_SEMS 2
/// Time out time in seconds, waiting on semaphore read
#define SEM_TIME_OUT 3
/// Time out time in seconds, waiting on semaphore write
#define SEM_WRITE_TIME_OUT 1
/// Time out time in seconds, waiting on semaphore value set
#define SEM_SET_TIME_OUT 1

/// Number of data semaphore in set (manages access to shared memory data)
#define DATA_SEM 0
/// Number of wait semaphore in set (blocks all server processes, until new data
/// is available)
#define WAIT_SEM 1

/// No flag
#define NO_FLAG 0


/**
 * RocketLogger execution mode definition
 */
enum rl_mode {
    LIMIT, //!< Limited sampling mode (limited by number of samples to take)
    CONTINUOUS,    //!< Continuous sampling mode (in background)
    METER,         //!< Meter mode (display current values in terminal)
    STATUS,        //!< Get current status of RocketLogger
    STOPPED,       //!< Stop continuous sampling
    SET_DEFAULT,   //!< Set default configuration
    PRINT_DEFAULT, //!< Print default configuration
    PRINT_VERSION, //!< Print the RocketLogger Software Stack version
    HELP,          //!< Show help
    NO_MODE        //!< No mode
};

/**
 * Typedef for RocketLogger execution modes
 */
typedef enum rl_mode rl_mode_t;

/**
 * RocketLogger state definition
 */
enum rl_state {
    RL_OFF = 0,     //!< Idle
    RL_RUNNING = 1, //!< Running
    RL_ERROR = -1,  //!< Error
};

/**
 * Typedef for RocketLogger states
 */
typedef enum rl_state rl_state_t;

/**
 * RocketLogger sampling state definition
 */
enum rl_sampling {
    RL_SAMPLING_OFF = 0, //!< Not sampling
    RL_SAMPLING_ON = 1,  //!< Sampling
};

/**
 * Typedef for RocketLogger sampling states
 */
typedef enum rl_sampling rl_sampling_t;

/**
 * RocketLogger data aggregation mode definition
 */
enum rl_aggregation {
    RL_AGGREGATE_NONE = 0,       //!< No aggregation
    RL_AGGREGATE_DOWNSAMPLE = 1, //!< Aggregate using downsampling
    RL_AGGREGATE_AVERAGE = 2     //!< Aggregate by averaging data
};

/**
 * Typedef for RocketLogger data aggregation modes
 */
typedef enum rl_aggregation rl_aggregation_t;

/**
 * RocketLogger file format definition
 */
enum rl_file_format {
    RL_FILE_NONE = 0, //!< No file
    RL_FILE_CSV = 1,  //!< CSV format
    RL_FILE_BIN = 2   //!< Binary format
};

/**
 * Typedef for RocketLogger file formats
 */
typedef enum rl_file_format rl_file_format_t;

/**
 * RocketLogger ambient sensor configuration.
 */
struct rl_ambient {
    /// Enable ambient sensors
    bool enabled;
    /// Number of sensors found on the bus
    int sensor_count;
    /// Identifiers of sensors found
    int available_sensors[AMBIENT_MAX_SENSOR_COUNT];
    /// Filename of the ambient measurement file
    char file_name[MAX_PATH_LENGTH];
};

/**
 * Typedef of RocketLogger ambient sensor configuration
 */
typedef struct rl_ambient rl_ambient_t;

/**
 * RocketLogger sampling configuration structure.
 */
struct rl_config {
    /// Configuration structure version
    int version;
    /// Sampling mode
    rl_mode_t mode;
    /// Sampling rate
    int sample_rate;
    /// Aggregation mode (for sampling rates below lowest native one)
    rl_aggregation_t aggregation;
    /// Data update rate
    int update_rate;
    /// Sample limit (0 for continuous)
    int sample_limit;
    /// Channels to sample
    bool channels[NUM_CHANNELS];
    /// Current channels to force to high range
    bool force_high_channels[NUM_I_CHANNELS];
    /// En-/disable digital inputs
    bool digital_input_enable;
    /// En-/disable plots on web interface
    bool web_interface_enable;
    /// Use/ignore existing calibration
    bool calibration_ignore;
    /// File format
    rl_file_format_t file_format;
    /// Maximum data file size
    uint64_t max_file_size;
    /// Data file name
    char file_name[MAX_PATH_LENGTH];
    /// Ambient conf
    struct rl_ambient ambient;
};

/**
 * Typedef for RocketLogger sampling configuration.
 */
typedef struct rl_config rl_config_t;

/**
 * RocketLogger status structure definition.
 */
struct rl_status {
    /// State
    rl_state_t state;
    /// Sampling state
    rl_sampling_t sampling;
    /// Number of samples taken
    uint64_t samples_taken;
    /// Number of buffers taken
    uint32_t buffer_number;
    /// Current configuration
    rl_config_t config;
    /// Time stamp of last calibration run
    uint64_t calibration_time;
};

/**
 * Typedef for RocketLogger sampling configuration.
 */
typedef struct rl_status rl_status_t;

/// RocketLogger status
rl_status_t status;

/// Channel names
extern char const *const channel_names[NUM_CHANNELS];
/// Digital input names
extern char const *const digital_input_names[NUM_DIGITAL_INPUTS];
/// Range valid information names
extern char const *const valid_info_names[NUM_I_CHANNELS];

#endif /* TYPES_H_ */
