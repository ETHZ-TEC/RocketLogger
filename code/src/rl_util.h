#ifndef RL_UTIL_H
#define RL_UTIL_H

#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#include "types.h"
#include "rl_lib.h"

#define DEFAULT_CONFIG "/etc/rocketlogger/default.conf"

void print_usage(struct rl_conf* conf);
void print_config(struct rl_conf* conf);

void reset_config(struct rl_conf* conf);
int read_default_config(struct rl_conf* conf);
int write_default_config(struct rl_conf* conf);

#endif