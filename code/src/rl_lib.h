#ifndef RL_LIB_H
#define RL_LIB_H

#include "lib_util.h"
#include "types.h"
#include "rl_hw.h"


void rl_reset_calibration();

int rl_get_data();

void rl_print_config(struct rl_conf* conf, int web);

void rl_print_status(struct rl_status* status, int web);

enum rl_state rl_get_status(int print, int web);

int rl_sample(struct rl_conf* conf);

int rl_stop();

#endif