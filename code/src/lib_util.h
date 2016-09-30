#ifndef LIB_UTIL_H
#define LIB_UTIL_H

#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#include "types.h"


#define CONFIG_FILE "/etc/rocketlogger/rl.conf"
#define PID_FILE "/etc/rocketlogger/rl.pid"


// TODO: remove file parameter
int read_config(struct rl_conf_new* conf, char file_name[MAX_PATH_LENGTH]);

int write_config(struct rl_conf_new* conf, char file_name[MAX_PATH_LENGTH]);


#endif