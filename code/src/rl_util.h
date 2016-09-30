#ifndef RL_UTIL_H
#define RL_UTIL_H

#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#include "types.h"


#define DEFAULT_CONFIG "/etc/rocketlogger/default.conf"


void reset_config(struct rl_conf_new* conf);

// TODO: remove file parameter
int read_default_config(struct rl_conf_new* conf, char file_name[MAX_PATH_LENGTH]);

int write_default_config(struct rl_conf_new* conf, char file_name[MAX_PATH_LENGTH]);

#endif