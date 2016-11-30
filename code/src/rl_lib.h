#ifndef RL_LIB_H
#define RL_LIB_H

#include "lib_util.h"
#include "types.h"
#include "rl_hw.h"
#include "util.h"


rl_state rl_get_status();

int rl_read_status(struct rl_status* status);

int rl_start(struct rl_conf* conf);

int rl_stop();

#endif
