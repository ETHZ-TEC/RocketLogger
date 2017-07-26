/**
 * Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group
 */

#ifndef LOG_H_
#define LOG_H_

#include <stdio.h>

#include "types.h"

void rl_log(rl_log_type type, const char* format, ...);

#endif /* LOG_H_ */
