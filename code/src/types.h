#ifndef RL_TYPES_H
#define RL_TYPES_H


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

#define SHMEM_KEY 1111 // TODO: usefull key


// constants
#define MAX_PATH_LENGTH 100
#define NUM_CHANNELS 10
#define NUM_I_CHANNELS 2
#define NUM_TOT_I_CHANNELS 6
#define NUM_V_CHANNELS 4

#define METER_UPDATE_RATE 5

#define STATUS_SIZE 2 // status size in bytes

#define RATE_SCALING 1000 // rates are in ksps

#define I1L_VALID_BIT 1
#define I2L_VALID_BIT 1
#define DIGIN1_BIT 2
#define DIGIN2_BIT 4
#define DIGIN3_BIT 8
#define DIGIN4_BIT 2
#define DIGIN5_BIT 4
#define DIGIN6_BIT 8

// enumerations TODO: typedefs

// PRU 

// PRU states
enum pru_states {
	PRU_OFF = 0,
	PRU_LIMIT = 1,
	PRU_CONTINUOUS = 3
};

#define NUMBER_PRU_COMMANDS 10
struct pru_data_struct {
	enum pru_states state;
	unsigned int precision;
	unsigned int sample_size;
	unsigned int buffer0_location;
	unsigned int buffer1_location;
	unsigned int buffer_size;
	unsigned int sample_limit;
	unsigned int add_currents;
	unsigned int number_commands;
	unsigned int commands[NUMBER_PRU_COMMANDS];
};

// ROCKETLOGGER

enum rl_state {
	RL_OFF = 0,
	RL_RUNNING = 1,
	RL_ERROR = -1
};
enum rl_mode {LIMIT, CONTINUOUS, METER, STATUS, STOPPED, DATA, CALIBRATE, SET_DEFAULT, PRINT_DEFAULT, HELP, NO_MODE};
enum rl_file_format {NO_FILE, CSV, BIN};

typedef enum log_type {ERROR, WARNING, INFO} rl_log_type;

// configuration struct

// channel indices in channels array
#define I1H_INDEX	0
#define I1M_INDEX	1
#define I1L_INDEX	2
#define V1_INDEX	3
#define V2_INDEX	4
#define I2H_INDEX	5
#define I2M_INDEX	6
#define I2L_INDEX	7
#define V3_INDEX	8
#define V4_INDEX	9

struct rl_conf {
	enum rl_mode mode;
	int sample_rate;
	int update_rate;
	int sample_limit;
	int channels[NUM_CHANNELS];
	int force_high_channels[NUM_I_CHANNELS];
	int enable_web_server;
	enum rl_file_format file_format;
	char file_name[MAX_PATH_LENGTH];
};

// status struct
struct rl_status {
	enum rl_state state;
	int samples_taken;
	int buffer_number;
};


// FILE HEADER
#define HEADERLENGTH 6
struct header {
	int header_length;
	int number_samples;
	int buffer_size;
	int rate;
	int channels;
	int precision;
};

#define HEADER_VERSION 0
struct file_header_new {
	int header_version;
	int number_samples;
	int buffer_size;
	int sample_rate;
	int precision;
	int channels[NUM_CHANNELS];
};


// ----- GLOBAL VARIABLES ----- //

struct rl_status status;

int offsets[NUM_CHANNELS];
double scales[NUM_CHANNELS];


#endif