/**
 * Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group
 */

#ifndef SEM_H_
#define SEM_H_

#define _GNU_SOURCE

#include <sys/types.h>

#include "log.h"
#include "types.h"

int create_sem(key_t key, int num_sems);
int remove_sem(int sem_id);
int open_sem(key_t key, int num_sems);
int wait_sem(int sem_id, int sem_num, int time_out);
int set_sem(int sem_id, int sem_num, int val);

#endif /* SEM_H_ */
