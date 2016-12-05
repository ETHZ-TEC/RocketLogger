#ifndef LIB_UTIL_H
#define LIB_UTIL_H

#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#include "types.h"

int check_sample_rate(int sample_rate);
int check_update_rate(int update_rate);

pid_t get_pid();
int set_pid(pid_t pid);

#endif