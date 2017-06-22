/**
 * Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group
 */

#ifndef AMBIENT_H_
#define AMBIENT_H_

#include <stdint.h>

#include "log.h"
#include "rl_file.h"
#include "types.h"
#include "util.h"


#define AMBIENT_SAMPLING_RATE 1 // Sps
#define AMBIENT_DATA_BLOCK_SIZE 1


void store_ambient_data(FILE* ambient_file, struct rl_conf* conf);

void set_ambient_file_name(struct rl_conf* conf);
void setup_ambient_lead_in(struct rl_file_lead_in* lead_in, struct rl_conf* conf);
void setup_ambient_channels(struct rl_file_header* file_header, struct rl_conf* conf);
void setup_ambient_header(struct rl_file_header* file_header, struct rl_conf* conf);

#endif /* AMBIENT_H_ */
