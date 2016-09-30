#ifndef LIB_UTIL_H
#define LIB_UTIL_H

#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#include "types.h"


#define CONFIG_FILE "/var/run/rocketlogger.conf"
#define PID_FILE "/var/run/rocketlogger.pid"

int check_sample_rate(int sample_rate);
int check_update_rate(int update_rate);

int read_config(struct rl_conf* conf);
int write_config(struct rl_conf* conf);

void print_json(float data[], int length);
void print_channels_new(int channels[NUM_CHANNELS]);
void print_status(struct rl_conf* conf, int web);

pid_t get_pid();
void set_pid(pid_t pid);

#endif