#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#include "util.h"
#include "types.h"


int create_sem();

int remove_sem(int sem_id);

int open_sem();

int wait_sem(int sem_id, int sem_num, int time_out);

int set_sem(int sem_id, int sem_num, int val);