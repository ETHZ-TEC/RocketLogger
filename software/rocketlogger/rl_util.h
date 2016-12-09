#ifndef RL_UTIL_H
#define RL_UTIL_H

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "types.h"
#include "rl_lib.h"

/// Default configuration file
#define DEFAULT_CONFIG "/etc/rocketlogger/default.conf"

/**
 * Available options for RocketLogger CLI
 */
typedef enum option {
	FILE_NAME,     //!< Name of data file to write
	SAMPLE_RATE,   //!< Sampling rate
	UPDATE_RATE,   //!< Data file update rate
	CHANNEL,       //!< Channels to sample
	FHR,           //!< Channels to force to high range
	WEB,           //!< En-/disable data averaging for web server
	DIGITAL_INPUTS,//!< Sample digital inputs
	DEF_CONF,      //!< Set configuration as default
	CALIBRATION,   //!< Use/ignore existing calibration values
	FILE_FORMAT,   //!< File format
	FILE_SIZE,     //!< Maximum data file size
	NO_OPTION      //!< No option
} rl_option;

void rl_print_config(struct rl_conf* conf);
void rl_print_status(struct rl_status* status);

rl_mode get_mode(char* mode);
rl_option get_option(char* option);
int parse_args(int argc, char* argv[], struct rl_conf* conf, int* set_as_default);

void print_usage(void);

void print_config(struct rl_conf* conf);
void reset_config(struct rl_conf* conf);
int read_default_config(struct rl_conf* conf);
int write_default_config(struct rl_conf* conf);

#endif
