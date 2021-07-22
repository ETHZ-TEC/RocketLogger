/**
 * Copyright (c) 2016-2020, ETH Zurich, Computer Engineering Group
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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <string.h>

#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "log.h"
#include "rl.h"

#include "sem.h"

int sem_create(key_t key, int count) {
    int id = semget(key, count, IPC_CREAT | S_IRWXU);
    if (id < 0) {
        rl_log(RL_LOG_ERROR, "Failed to create semaphore; %d message: %s",
               errno, strerror(errno));
    }
    return id;
}

int sem_remove(int id) {
    int ret = semctl(id, 0, IPC_RMID);
    if (ret < 0) {
        rl_log(RL_LOG_ERROR, "Failed to remove semaphore; %d message: %s",
               errno, strerror(errno));
        return ret;
    }
    return SUCCESS;
}

int sem_open(key_t key, int count) {
    int id = semget(key, count, S_IRWXU);
    if (id < 0) {
        rl_log(RL_LOG_ERROR, "Failed to open semaphore; %d message: %s", errno,
               strerror(errno));
        return id;
    }
    return id;
}

int sem_wait(int id, int index, int timeout) {
    int num_ops = 1;
    int sem_op = -1;
    struct sembuf sem_ops = {index, sem_op, SEM_OPT_NO_FLAG};
    struct timespec timeout_struct = {timeout, 0};

    int ret = semtimedop(id, &sem_ops, num_ops, &timeout_struct);
    if (ret < 0) {
        if (errno == EAGAIN) {
            rl_log(RL_LOG_ERROR, "Timeout waiting on semaphore; %d message: %s",
                   errno, strerror(errno));
        } else if (errno == EIDRM) {
            rl_log(RL_LOG_ERROR,
                   "Failed waiting on semaphore, semaphore removed; %d "
                   "message: %s",
                   errno, strerror(errno));
        } else if (errno == EINVAL) {
            rl_log(RL_LOG_ERROR,
                   "Failed waiting on semaphore, semaphore inexistent; %d "
                   "message %s",
                   errno, strerror(errno));
        } else {
            rl_log(RL_LOG_ERROR, "Failed waiting on semaphore; %d message: %s",
                   errno, strerror(errno));
        }
        return ret;
    }
    return SUCCESS;
}

int sem_set(int id, int index, int value) {
    int num_ops = 1;
    struct sembuf sem_ops = {index, value, SEM_OPT_NO_FLAG};
    struct timespec timeout_struct = {SEM_TIMEOUT_WRITE, 0};

    int ret = semtimedop(id, &sem_ops, num_ops, &timeout_struct);
    if (ret < 0) {
        if (errno == EAGAIN) {
            rl_log(RL_LOG_ERROR,
                   "Timeout on setting semaphore count; %d message: %s", errno,
                   strerror(errno));
        } else {
            rl_log(RL_LOG_ERROR,
                   "Failed setting semaphore count; %d message: %s", errno,
                   strerror(errno));
        }
        return ret;
    }
    return SUCCESS;
}

int sem_get(int id, int index) {
    int count = semctl(id, index, GETNCNT);
    if (count < 0) {
        rl_log(RL_LOG_ERROR, "Failed getting semaphore count; %d message: %s",
               errno, strerror(errno));
        return count;
    }
    return count;
}
