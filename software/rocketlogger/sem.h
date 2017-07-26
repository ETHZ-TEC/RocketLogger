/**
 * Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group
 */

#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>

#include "log.h"
#include "types.h"

int create_sem(key_t key, int num_sems);

int remove_sem(int sem_id);

int open_sem(key_t key, int num_sems);

int wait_sem(int sem_id, int sem_num, int time_out);

int set_sem(int sem_id, int sem_num, int val);
