#ifndef LIB_UTIL_H
#define LIB_UTIL_H

#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#include "types.h"


#define CONFIG_FILE "/var/run/rocketlogger.conf"
#define PID_FILE "/var/run/rocketlogger.pid"


// TODO: remove file parameter
int read_config(struct rl_conf_new* conf);

int write_config(struct rl_conf_new* conf);

pid_t get_pid();
void set_pid(pid_t pid);

#endif