/**
 * Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group
 */

#ifndef METER_H_
#define METER_H_

#include <ncurses.h>
#include <stdint.h>

#include "log.h"
#include "types.h"
#include "util.h"

void meter_init(void);

void meter_stop(void);

void meter_print_buffer(struct rl_conf* conf, void* virt_addr,
                        uint32_t sample_size);

#endif /* METER_H_ */
