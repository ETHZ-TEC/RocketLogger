/**
 * Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group
 */

#define _GNU_SOURCE

#include <fcntl.h>

#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>

#include "sem.h"

/**
 * Create RocketLogger semaphore set
 * @return ID of created set
 */
int create_sem(key_t key, int num_sems) {
    int sem_id = semget(key, num_sems, IPC_CREAT | S_IRWXU);
    if (sem_id < 0) {
        rl_log(ERROR, "failed to create semaphores. Errno = %d", errno);
    }
    return sem_id;
}

/**
 * Remove semaphore set
 * @param sem_id ID of set to remove
 * @return {@link SUCCESS} on success, {@link FAILURE} otherwise
 */
int remove_sem(int sem_id) {
    // remove semaphores
    if (semctl(sem_id, 0, IPC_RMID) < 0) {
        rl_log(ERROR, "failed to remove semaphores. Errno = %d", errno);
        return FAILURE;
    }
    return SUCCESS;
}

/**
 * Open existing RocketLogger semaphore set
 * @return ID of opened set
 */
int open_sem(key_t key, int num_sems) {
    int sem_id = semget(key, num_sems, S_IRWXU);
    if (sem_id < 0) {
        rl_log(ERROR, "failed to open semaphores. Errno = %d", errno);
    }
    return sem_id;
}

/**
 * Wait on a semaphore until access granted
 * @param sem_id ID of semaphore set
 * @param sem_num Number of semaphore in set
 * @param time_out Maximum waiting time
 * @return {@link SUCCESS} on success (access granted), {@link TIME_OUT} on time
 * out, {@link FAILURE} otherwise
 */
int wait_sem(int sem_id, int sem_num, int time_out) {
    // operation on semaphore
    int num_ops = 1;
    int sem_op = -1;
    struct sembuf sem_ops = {sem_num, sem_op, NO_FLAG};
    struct timespec t_out = {time_out, 0};

    if (semtimedop(sem_id, &sem_ops, num_ops, &t_out) < 0) {
        if (errno == EAGAIN) {
            rl_log(WARNING, "time-out waiting on semaphore");
            return TIME_OUT;
        } else if (errno == EIDRM) {
            rl_log(INFO, "waiting on semaphore failed: semaphore removed");
            return TIME_OUT;
        } else if (errno == EINVAL) {
            rl_log(WARNING,
                   "waiting on semaphore failed: semaphore not existing");
            return FAILURE;
        } else {
            rl_log(ERROR, "failed doing operation on semaphore. Errno = %d",
                   errno);
            return FAILURE;
        }
    }

    return SUCCESS;
}

/**
 * Set value to semaphore
 * @param sem_id ID of semaphore set
 * @param sem_num Number of semaphore in set
 * @param val Value to be set to semaphore
 * @return {@link SUCCESS} on success (access granted), {@link TIME_OUT} on time
 * out, {@link FAILURE} otherwise
 */
int set_sem(int sem_id, int sem_num, int val) {
    // operation on semaphore
    int num_ops = 1;
    int sem_op = val;
    struct sembuf sem_ops = {sem_num, sem_op, NO_FLAG};
    struct timespec time_out = {SEM_SET_TIME_OUT, 0};

    if (semtimedop(sem_id, &sem_ops, num_ops, &time_out) < 0) {
        if (errno == EAGAIN) {
            rl_log(ERROR, "time-out waiting on semaphore");
            return TIME_OUT;
        } else {
            rl_log(ERROR, "failed doing operation on semaphore. Errno = %d",
                   errno);
            return FAILURE;
        }
    }

    return SUCCESS;
}
