/**
 * Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group
 */

#ifndef RL_LIB_H_
#define RL_LIB_H_


#include "lib_util.h"
#include "rl_hw.h"
#include "types.h"
#include "util.h"

rl_state rl_get_status(void);

int rl_read_status(struct rl_status* status);

void rl_read_calibration(struct rl_calibration* calibration_ptr,
                         struct rl_conf* conf);

int rl_start(struct rl_conf* conf);

int rl_stop(void);

#endif /* RL_LIB_H_ */
