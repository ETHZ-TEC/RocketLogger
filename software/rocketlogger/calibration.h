/**
 * Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group
 */

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include "log.h"
#include "types.h"

void reset_offsets(void);
void reset_scales(void);
int read_calibration(struct rl_conf* conf);
