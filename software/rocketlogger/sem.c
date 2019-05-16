/**
 * Copyright (c) 2016-2019, Swiss Federal Institute of Technology (ETH Zurich)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#define _GNU_SOURCE

#include <errno.h>
#include <string.h>

#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "log.h"
#include "types.h"

#include "sem.h"

int sem_create(key_t key, int num_sems) {
    int sem_id = semget(key, num_sems, IPC_CREAT | S_IRWXU);
    if (sem_id < 0) {
        rl_log(ERROR, "Failed to create semaphore; %d message: %s", errno,
               strerror(errno));
    }
    return sem_id;
}

int sem_remove(int sem_id) {
    // remove semaphores
    if (semctl(sem_id, 0, IPC_RMID) < 0) {
        rl_log(ERROR, "Failed to remove semaphore; %d message: %s", errno,
               strerror(errno));
        return FAILURE;
    }
    return SUCCESS;
}

int sem_open(key_t key, int num_sems) {
    int sem_id = semget(key, num_sems, S_IRWXU);
    if (sem_id < 0) {
        rl_log(ERROR, "Failed to open semaphore; %d message: %s", errno,
               strerror(errno));
    }
    return sem_id;
}

int sem_wait(int sem_id, int sem_num, int time_out) {
    // operation on semaphore
    int num_ops = 1;
    int sem_op = -1;
    struct sembuf sem_ops = {sem_num, sem_op, NO_FLAG};
    struct timespec t_out = {time_out, 0};

    if (semtimedop(sem_id, &sem_ops, num_ops, &t_out) < 0) {
        if (errno == EAGAIN) {
            rl_log(ERROR, "Timeout waiting on semaphore; %d message: %s", errno,
                   strerror(errno));
            rl_log(WARNING, "time-out waiting on semaphore");
            return TIME_OUT;
        } else if (errno == EIDRM) {
            rl_log(ERROR, "Failed waiting on semaphore, semaphore removed; %d "
                          "message: %s",
                   errno, strerror(errno));
            return TIME_OUT;
        } else if (errno == EINVAL) {
            rl_log(ERROR, "Failed waiting on semaphore, semaphore inexistent; "
                          "%d message: %s",
                   errno, strerror(errno));
            return FAILURE;
        } else {
            rl_log(ERROR, "Failed waiting on semaphore; %d message: %s", errno,
                   strerror(errno));
            return FAILURE;
        }
    }
    return SUCCESS;
}

int sem_set(int sem_id, int sem_num, int val) {
    // operation on semaphore
    int num_ops = 1;
    int sem_op = val;
    struct sembuf sem_ops = {sem_num, sem_op, NO_FLAG};
    struct timespec time_out = {SEM_SET_TIME_OUT, 0};

    if (semtimedop(sem_id, &sem_ops, num_ops, &time_out) < 0) {
        if (errno == EAGAIN) {
            rl_log(ERROR, "Timeout on setting semaphore count; %d message: %s",
                   errno, strerror(errno));
            return TIME_OUT;
        } else {
            rl_log(ERROR, "Failed setting semaphore count; %d message: %s",
                   errno, strerror(errno));
            return FAILURE;
        }
    }
    return SUCCESS;
}

int sem_get(int sem_id, int sem_num) {
    int count = semctl(sem_id, sem_num, GETNCNT);
    if (count < 0) {
        rl_log(ERROR, "Failed getting semaphore count; %d message: %s", errno,
               strerror(errno));
    }
    return count;
}
