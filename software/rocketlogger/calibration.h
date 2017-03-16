/**
 * Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group
 */

#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>

#include "types.h"
#include "log.h"


void reset_offsets(void);
void reset_scales(void);
int read_calibration(struct rl_conf* conf);
