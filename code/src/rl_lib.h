#ifndef RL_LIB_H
#define RL_LIB_H

#include "lib_util.h"
#include "types.h"
#include "rl_hw.h"

void rl_print_config(struct rl_conf* conf, int web);

void rl_reset_calibration();

int rl_get_data();

int rl_get_status(int print, int web);

int rl_sample(struct rl_conf* conf);

int rl_stop();

#endif