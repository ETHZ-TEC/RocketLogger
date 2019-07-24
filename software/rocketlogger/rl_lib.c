/**
 * Copyright (c) 2016-2019, ETH Zurich, Computer Engineering Group
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

#include <stdbool.h>
#include <string.h>

#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

#include "calibration.h"
#include "lib_util.h"
#include "log.h"
#include "rl_hw.h"
#include "util.h"

#include "rl_lib.h"

/**
 * Get status of RocketLogger
 * @return current status {@link rl_state_t}
 */
rl_state_t rl_get_status(void) {
    rl_status_t status;
    return rl_read_status(&status);
}

/**
 * Read status of RocketLogger
 * @param status Pointer to {@link rl_status_t} struct to write to
 * @return current status {@link rl_state_t}
 */
rl_state_t rl_read_status(rl_status_t *const status) {
    pid_t pid = get_pid();
    if (pid == FAILURE || kill(pid, 0) < 0) {
        // process not running
        status->state = RL_OFF;
    } else {
        // read status
        read_status(status);
    }
    return status->state;
}

/**
 * Read calibration file
 * @param config Current {@link rl_config_t} configuration
 * @param calibration Pointer to {@link rl_calibration_t} to write to
 */
void rl_read_calibration(rl_config_t const *const config,
                         rl_calibration_t *const calibration) {
    calibration_load(config);
    memcpy(calibration, &calibration_data, sizeof(rl_calibration_t));
}

/**
 * RocketLogger start function: start sampling
 * @param config Pointer to desired {@link rl_config_t} configuration
 * @param file_comment Comment to store in the file header
 * @return {@link SUCCESS} in case of success, {@link FAILURE} otherwise
 */
int rl_start(rl_config_t *const config, char const *const file_comment) {

    // check mode
    switch (config->mode) {
    case LIMIT:
        break;
    case CONTINUOUS:
        // create daemon to run in background
        if (daemon(1, 1) < 0) {
            rl_log(ERROR, "failed to create background process");
            return SUCCESS;
        }
        break;
    case METER:
        // set meter config
        config->update_rate = METER_UPDATE_RATE;
        config->sample_limit = 0;
        config->web_interface_enable = false;
        config->file_format = RL_FILE_NONE;
        if (config->sample_rate < MIN_ADC_RATE) {
            rl_log(WARNING, "too low sample rate. Setting rate to 1kSps");
            config->sample_rate = MIN_ADC_RATE;
        }
        break;
    default:
        rl_log(ERROR, "wrong mode");
        return FAILURE;
    }

    // check input
    if (check_sample_rate(config->sample_rate) == FAILURE) {
        rl_log(ERROR, "wrong sampling rate");
        return FAILURE;
    }
    if (check_update_rate(config->update_rate) == FAILURE) {
        rl_log(ERROR, "wrong update rate");
        return FAILURE;
    }
    if (config->update_rate != 1 && config->web_interface_enable) {
        rl_log(WARNING, "webserver plot does not work with update rates >1. "
                        "Disabling webserver ...");
        config->web_interface_enable = false;
    }

    // check ambient configuration
    if (config->ambient.enabled && config->file_format == RL_FILE_NONE) {
        rl_log(
            WARNING,
            "Ambient logging not possible without file. Disabling ambient ...");
        config->ambient.enabled = false;
    }

    // store PID to file
    pid_t pid = getpid();
    set_pid(pid);

    // register signal handler (for stopping)
    if (signal(SIGQUIT, sig_handler) == SIG_ERR ||
        signal(SIGINT, sig_handler) == SIG_ERR) {
        rl_log(ERROR, "can't register signal handler");
        return FAILURE;
    }

    // INITIATION
    hw_init(config);

    // check ambient sensor available
    if (config->ambient.enabled && config->ambient.sensor_count == 0) {
        config->ambient.enabled = false;
        rl_log(WARNING, "No ambient sensor found. Disabling ambient ...");
    }

    // SAMPLING
    rl_log(INFO, "sampling start");
    hw_sample(config, file_comment);
    rl_log(INFO, "sampling finished");

    // FINISH
    hw_deinit(config);

    return SUCCESS;
}

/**
 * RocketLogger stop function (to stop continuous mode)
 * @return {@link SUCCESS} in case of success, {@link FAILURE} otherwise
 */
int rl_stop(void) {

    // check if running
    if (rl_get_status() != RL_RUNNING) {
        rl_log(ERROR, "RocketLogger not running");
        return FAILURE;
    }

    // ged pid
    pid_t pid = get_pid();

    // send stop signal
    kill(pid, SIGQUIT);

    return SUCCESS;
}
