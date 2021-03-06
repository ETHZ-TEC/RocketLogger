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

#ifndef TYPES_H_
#define TYPES_H_

// INCLUDES

// standard
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// ipc, memory mapping, multithreading
#include <pthread.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/shm.h>

// files
#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>

// time
#include <sys/time.h>
#include <time.h>

// pru
#include <pruss_intc_mapping.h>
#include <prussdrv.h>

// ncurses (text-based UI)
#include <ncurses.h>

// DEFINES

// return codes
/**
 * Return code definitions
 */
#define SUCCESS 1
#define UNDEFINED 0
#define TIME_OUT 0
#define FAILURE -1

// files
/// Process ID file for background process
#define PID_FILE "/var/run/rocketlogger.pid"
/// Log file name
#define LOG_FILE "/var/www/log/log.txt"
/// File to read MAC address
#define MAC_ADDRESS_FILE "/sys/class/net/eth0/address"
/// Calibration file name
#define CALIBRATION_FILE "/etc/rocketlogger/calibration.dat"

/// Log file size in bytes
#define MAX_LOG_FILE_SIZE 1000000

/// Key for status shared memory (used for creation)
#define SHMEM_STATUS_KEY 1111
/// Key for web shared memory (used for creation)
#define SHMEM_DATA_KEY 4443
/// Permissions for shared memory
#define SHMEM_PERMISSIONS 0666

// constants
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
#define PRU_DIG_SIZE 2
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

// ROCKETLOGGER
/**
 * RocketLogger state definition
 */
typedef enum state {
    RL_OFF = 0,     //!< Idle
    RL_RUNNING = 1, //!< Running
    RL_ERROR = -1,  //!< Error
} rl_state;

/**
 * RocketLogger sampling state definition
 */
typedef enum sampling {
    SAMPLING_OFF = 0, //!< Not sampling
    SAMPLING_ON = 1,  //!< Sampling
} rl_sampling;

/**
 * RocketLogger mode definition
 */
typedef enum mode {
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
} rl_mode;

/**
 * RocketLogger data aggregation mode definition
 */
typedef enum aggregation {
    AGGREGATE_NONE = 0,       //!< No aggregation
    AGGREGATE_DOWNSAMPLE = 1, //!< Aggregate using downsampling
    AGGREGATE_AVERAGE = 2     //!< Aggregate by averaging data
} rl_aggregation;

/**
 * RocketLogger file format definition
 */
typedef enum file_format {
    NO_FILE = 0, //!< No file
    CSV = 1,     //!< CSV format
    BIN = 2      //!< Binary format
} rl_file_format;

/**
 * RocketLogger calibration definition
 */
typedef enum use_cal {
    CAL_IGNORE = 0, //!< Ignore calibration
    CAL_USE = 1     //!< Use calibration (if existing)
} rl_use_cal;

/**
 * RocketLogger log file types definition
 */
typedef enum log_type {
    ERROR,   //!< Error
    WARNING, //!< Warning
    INFO     //!< Information
} rl_log_type;

/// RocketLogger configuration structure version number
#define RL_CONF_VERSION 0x01

// AMBIENT CONF //
#define AMBIENT_MAX_SENSOR_COUNT 128
#define AMBIENT_DISABLED 0
#define AMBIENT_ENABLED 1
struct rl_ambient {
    int enabled;
    int sensor_count;
    int available_sensors[AMBIENT_MAX_SENSOR_COUNT];
    char file_name[MAX_PATH_LENGTH];
} ambient;

// channel properties
/// Channel sampling disabled
#define CHANNEL_DISABLED 0
/// Channel sampling enabled
#define CHANNEL_ENABLED 1

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

// digital inputs
/// Digital input sampling disabled
#define DIGITAL_INPUTS_DISABLED 0
/// Digital input sampling ensabled
#define DIGITAL_INPUTS_ENABLED 1

/**
 * RocketLogger sampling configuration
 */
struct rl_conf {
    /// Configuration structure version
    int version;
    /// Sampling mode
    rl_mode mode;
    /// Sampling rate
    int sample_rate;
    /// Aggregation mode (for sampling rates below lowest native one)
    rl_aggregation aggregation;
    /// Data update rate
    int update_rate;
    /// Sample limit (0 for continuous)
    int sample_limit;
    /// Channels to sample
    int channels[NUM_CHANNELS];
    /// Current channels to force to high range
    int force_high_channels[NUM_I_CHANNELS];
    /// En-/disable digital inputs
    int digital_inputs;
    /// En-/disable plots on web interface
    int enable_web_server;
    /// Use/ignore existing calibration
    rl_use_cal calibration;
    /// File format
    rl_file_format file_format;
    /// Maximum data file size
    uint64_t max_file_size;
    /// Data file name
    char file_name[MAX_PATH_LENGTH];
    /// Ambient conf
    struct rl_ambient ambient;
};

/**
 * RocketLogger status
 */
struct rl_status {
    /// State
    rl_state state;
    /// Sampling state
    rl_sampling sampling;
    /// Number of samples taken
    uint64_t samples_taken;
    /// Number of buffers taken
    uint32_t buffer_number;
    /// Current configuration
    struct rl_conf conf;
    /// Time stamp of last calibration run
    uint64_t calibration_time;
};

/**
 * RocketLogger calibration data
 */
struct rl_calibration {
    /// Time stamp of calibration run
    uint64_t time;
    /// Channel offsets (in bit)
    int offsets[NUM_CHANNELS];
    /// Channel scalings
    double scales[NUM_CHANNELS];
};

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

// ----- GLOBAL VARIABLES ----- //
/// RocketLogger status
struct rl_status status;
/// Calibration data
struct rl_calibration calibration;
/// Channel names
extern const char* channel_names[NUM_CHANNELS];
/// Digital input names
extern const char* digital_input_names[NUM_DIGITAL_INPUTS];
/// Range valid information names
extern const char* valid_info_names[NUM_I_CHANNELS];

#endif /* TYPES_H_ */
