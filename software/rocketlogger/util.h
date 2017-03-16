/**
 * Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group
 */

#ifndef UTIL_H
#define UTIL_H

#include <sys/shm.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <syslog.h>
#include <errno.h>
#include <stdarg.h>

#include "types.h"
#include "log.h"

/// Permissions for shared memory
#define SHMEM_PERMISSIONS 0666

int is_current(int index);
int is_low_current(int index);
int count_channels(int channels[NUM_CHANNELS]);


int read_status(struct rl_status* status);
int write_status(struct rl_status* status);

int ceil_div(int n, int d);

void sig_handler(int signo);

int read_file_value(char filename[]);

#endif
