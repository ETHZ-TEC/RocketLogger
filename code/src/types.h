#ifndef RL_TYPES_H
#define RL_TYPES_H

/// test mode: for usage withouth RL cape
#define TEST_MODE 0

// INCLUDES

// standard
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>

// ipc, memory mapping, multithreading
#include <signal.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <pthread.h>

// files
#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>

// time
#include <time.h>
#include <sys/time.h>

// pru
#include <prussdrv.h>
#include <pruss_intc_mapping.h>

// ncurses (text-based UI)
#include <ncurses.h>



// DEFINES

// add currents (1: add, 0: not add)
#define ADD_CURRENTS 0

// return codes
#define SUCCESS 1
#define UNDEFINED 0
#define FAILURE -1

// files
#define CONFIG_FILE		"/var/run/rocketlogger.conf"
#define PID_FILE		"/var/run/rocketlogger.pid"
#define LOG_FILE		"/var/www/log/log.txt"

/// log file size in bytes
#define MAX_LOG_FILE_SIZE 1000000

#define SHMEM_STATUS_KEY 1111
#define SHMEM_DATA_KEY 4443


// constants
#define MAX_PATH_LENGTH 100
#define NUM_CHANNELS 8
#define NUM_PRU_CHANNELS 10 // medium range! -> TODO: remove
#define NUM_I_CHANNELS 2
#define NUM_V_CHANNELS 4
#define NUM_DIGITAL_INPUTS 6

#define METER_UPDATE_RATE 5

#define PRU_DIG_SIZE 2 // status size in bytes
#define PRU_BUFFER_STATUS_SIZE 4 //buffer status size in bytes

#define RATE_SCALING 1000 // rates are in ksps

#define I1L_VALID_BIT 1
#define I2L_VALID_BIT 1
#define DIGIN1_BIT 2
#define DIGIN2_BIT 4
#define DIGIN3_BIT 8
#define DIGIN4_BIT 2
#define DIGIN5_BIT 4
#define DIGIN6_BIT 8



// ROCKETLOGGER

typedef enum state {
	RL_OFF = 0,
	RL_RUNNING = 1,
	RL_ERROR = -1
} rl_state;

typedef enum sampling {
	SAMPLING_OFF = 0,
	SAMPLING_ON = 1
} rl_sampling;

typedef enum mode {
	LIMIT,
	CONTINUOUS,
	METER,
	STATUS,
	STOPPED,
	CALIBRATE,
	SET_DEFAULT,
	PRINT_DEFAULT,
	HELP,
	NO_MODE
} rl_mode;

typedef enum file_format {
	NO_FILE = 0,
	CSV = 1,
	BIN = 2
} rl_file_format;

typedef enum use_cal {CAL_USE, CAL_IGNORE} rl_use_cal;

typedef enum log_type {ERROR, WARNING, INFO} rl_log_type;

// configuration struct

// TODO: use
#define CHANNEL_DISABLED 0
#define CHANNEL_ENABLED 1

// channel indices in channels array
#define I1H_INDEX	0
#define I1L_INDEX	1
#define V1_INDEX	2
#define V2_INDEX	3
#define I2H_INDEX	4
#define I2L_INDEX	5
#define V3_INDEX	6
#define V4_INDEX	7

// digital inputs
#define DIGITAL_INPUTS_DISABLED 0
#define DIGITAL_INPUTS_ENABLED 1

struct rl_conf {
	rl_mode mode;
	int sample_rate;
	int update_rate;
	int sample_limit;
	int channels[NUM_CHANNELS];
	int force_high_channels[NUM_I_CHANNELS];
	int digital_inputs;
	int enable_web_server;
	rl_use_cal calibration;
	rl_file_format file_format;
	char file_name[MAX_PATH_LENGTH];
};

// status struct
struct rl_status {
	rl_state state;
	rl_sampling sampling;
	int samples_taken;
	int buffer_number;
	struct rl_conf conf;
};


// SEMAPHORES
#define SEM_KEY 2222
#define NUM_SEMS 2
#define SEM_TIME_OUT 3
#define SEM_WRITE_TIME_OUT 1
#define SEM_SET_TIME_OUT 1

#define DATA_SEM 0
#define WAIT_SEM 1

#define NO_FLAG 0

#define TIME_OUT 0

// ----- GLOBAL VARIABLES ----- //

struct rl_status status;

int offsets[NUM_CHANNELS];
double scales[NUM_CHANNELS];

extern const char* channel_names[NUM_CHANNELS];
extern const char* digital_input_names[NUM_DIGITAL_INPUTS];
extern const char* valid_info_names[NUM_I_CHANNELS];

#endif
