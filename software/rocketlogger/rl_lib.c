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

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>

#include "log.h"
#include "rl.h"
#include "rl_hw.h"
#include "util.h"

#include "rl_lib.h"

/**
 * Signal handler to catch stop signals.
 *
 * @todo allow for forced user interrupt (ctrl+C)
 *
 * @param signal_number The number of the signal to handle
 */
static void rl_signal_handler(int signal_number);

bool rl_is_sampling(void) {
    rl_status_t status;
    rl_read_status(&status);
    return status.sampling;
}

int rl_read_status(rl_status_t *const status) {
    pid_t pid = rl_pid_get();

    // if not running, return default status
    if (pid == 0 || kill(pid, 0) < 0) {
        rl_status_reset(status);
        return SUCCESS;
    }

    // map shared memory
    int shm_id =
        shmget(SHMEM_STATUS_KEY, sizeof(rl_status_t), SHMEM_PERMISSIONS);
    if (shm_id == -1) {
        rl_log(RL_LOG_ERROR,
               "In read_status: failed to get shared status memory id; "
               "%d message: %s",
               errno, strerror(errno));
        return ERROR;
    }
    rl_status_t const *const shm_status =
        (rl_status_t const *const)shmat(shm_id, NULL, 0);

    if (shm_status == (void *)-1) {
        rl_log(RL_LOG_ERROR,
               "In read_status: failed to map shared status memory; %d "
               "message: %s",
               errno, strerror(errno));
        return ERROR;
    }

    // copy status read from shared memory
    memcpy(status, shm_status, sizeof(rl_status_t));

    // unmap shared memory
    shmdt(shm_status);

    return SUCCESS;
}

int rl_run(rl_config_t *const config) {

    // // check mode
    // switch (config->mode) {
    // case LIMIT:
    //     break;
    // case CONTINUOUS:
    //     // create daemon to run in background
    //     if (daemon(1, 1) < 0) {
    //         rl_log(RL_LOG_ERROR, "failed to create background process");
    //         return SUCCESS;
    //     }
    //     break;
    // case METER:
    //     // set meter config
    //     config->update_rate = METER_UPDATE_RATE;
    //     config->sample_limit = 0;
    //     config->web_enable = false;
    //     config->file_enable = false;
    //     if (config->sample_rate < ADS131E0X_RATE_MIN) {
    //         rl_log(RL_LOG_WARNING,
    //                "too low sample rate. Setting rate to 1kSps");
    //         config->sample_rate = ADS131E0X_RATE_MIN;
    //     }
    //     break;
    // default:
    //     rl_log(RL_LOG_ERROR, "wrong mode");
    //     return FAILURE;
    // }

    // // check input
    // if (check_sample_rate(config->sample_rate) == FAILURE) {
    //     rl_log(RL_LOG_ERROR, "wrong sampling rate");
    //     return FAILURE;
    // }
    // if (check_update_rate(config->update_rate) == FAILURE) {
    //     rl_log(RL_LOG_ERROR, "wrong update rate");
    //     return FAILURE;
    // }
    // if (config->update_rate != 1 && config->web_enable) {
    //     rl_log(RL_LOG_WARNING,
    //            "webserver plot does not work with update rates >1. "
    //            "Disabling webserver ...");
    //     config->web_enable = false;
    // }

    // // check ambient configuration
    // if (config->ambient_enable && !(config->file_enable)) {
    //     rl_log(
    //         WARNING,
    //         "Ambient logging not possible without file. Disabling ambient
    //         ...");
    //     config->ambient_enable = false;
    // }

    // store PID to file
    pid_t pid = getpid();
    rl_pid_set(pid);

    // register signal handler for SIGQUIT and SIGINT (for stopping)
    struct sigaction signal_action;
    signal_action.sa_handler = rl_signal_handler;
    sigemptyset(&signal_action.sa_mask);
    signal_action.sa_flags = 0;

    int ret;
    ret = sigaction(SIGQUIT, &signal_action, NULL);
    if (ret < 0) {
        rl_log(RL_LOG_ERROR, "can't register signal handler for SIGQUIT.");
        return ERROR;
    }
    ret = sigaction(SIGINT, &signal_action, NULL);
    if (ret < 0) {
        rl_log(RL_LOG_ERROR, "can't register signal handler for SIGINT.");
        return ERROR;
    }

    // INITIATION
    hw_init(config);

    // check ambient sensor available
    if (config->ambient_enable && rl_status.sensor_count == 0) {
        config->ambient_enable = false;
        rl_log(RL_LOG_WARNING, "No ambient sensor found. Disabling ambient...");
    }

    // SAMPLING
    rl_log(RL_LOG_INFO, "sampling start");
    hw_sample(config);
    rl_log(RL_LOG_INFO, "sampling finished");

    // FINISH
    hw_deinit(config);

    return SUCCESS;
}

int rl_stop(void) {

    // check if running
    if (!rl_is_sampling()) {
        rl_log(RL_LOG_ERROR, "RocketLogger not running");
        return ERROR;
    }

    // ged pid
    pid_t pid = rl_pid_get();
    if (pid < 0) {
        rl_log(RL_LOG_ERROR, "RocketLogger PID not found");
        return ERROR;
    }

    // send stop signal
    kill(pid, SIGQUIT);

    return SUCCESS;
}

static void rl_signal_handler(int signal_number) {
    // signal generated by stop function
    if (signal_number == SIGQUIT) {
        // stop sampling
        rl_status.sampling = false;
    }

    // signal generated by user (ctrl+C)
    if (signal_number == SIGINT) {
        signal(signal_number, SIG_IGN);
        // printf("Stopping RocketLogger ...\n");
        rl_status.sampling = false;
    }
}
