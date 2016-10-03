#ifndef RL_UTIL_H
#define RL_UTIL_H

#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#include "types.h"
#include "rl_lib.h"

#define DEFAULT_CONFIG "/etc/rocketlogger/default.conf"

enum rl_option { FILE_NAME, SAMPLE_RATE, UPDATE_RATE, CHANNEL, FHR, WEB, BINARY_FILE, DEF_CONF, FILE_FORMAT, NO_OPTION };


enum rl_mode get_mode(char* mode);
enum rl_option get_option(char* option);
int parse_args(int argc, char* argv[], struct rl_conf* conf, int* set_as_default);

void print_usage(struct rl_conf* conf);

void print_config(struct rl_conf* conf);
void reset_config(struct rl_conf* conf);
int read_default_config(struct rl_conf* conf);
int write_default_config(struct rl_conf* conf);

#endif