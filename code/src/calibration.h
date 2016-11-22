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


void reset_offsets();
void reset_scales();
int read_calibration(struct rl_conf* conf);
