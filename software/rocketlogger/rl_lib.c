/**
 * Copyright (c) 2016-2018, Swiss Federal Institute of Technology (ETH Zurich)
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
 * ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "rl_lib.h"

/**
 * Get status of RocketLogger
 * @return current status {@link rl_state}
 */
rl_state rl_get_status(void) {

    struct rl_status status;

    // get pid
    pid_t pid = get_pid();
    if (pid == FAILURE || kill(pid, 0) < 0) {
        // process not running
        status.state = RL_OFF;
    } else {
        // read status
        read_status(&status);
    }

    return status.state;
}

/**
 * Read status of RocketLogger
 * @param status Pointer to {@link rl_status} struct to write to
 * @return current status {@link rl_state}
 */
int rl_read_status(struct rl_status* status) {

    // get pid
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
 * @param calibration_ptr Pointer to {@link rl_calibration} to write to
 * @param conf Current {@link rl_conf} configuration
 */
void rl_read_calibration(struct rl_calibration* calibration_ptr,
                         struct rl_conf* conf) {
    read_calibration(conf);
    memcpy(calibration_ptr, &calibration, sizeof(struct rl_calibration));
}

/**
 * RocketLogger start function: start sampling
 * @param conf Pointer to desired {@link rl_conf} configuration
 * @param file_comment Comment to store in the file header
 * @return {@link SUCCESS} in case of success, {@link FAILURE} otherwise
 */
int rl_start(struct rl_conf* conf, char* file_comment) {

    // check mode
    switch (conf->mode) {
    case LIMIT:
        break;
    case CONTINUOUS:
        // create deamon to run in background
        if (daemon(1, 1) < 0) {
            rl_log(ERROR, "failed to create background process");
            return SUCCESS;
        }
        break;
    case METER:
        // set meter config
        conf->update_rate = METER_UPDATE_RATE;
        conf->sample_limit = 0;
        conf->enable_web_server = 0;
        conf->file_format = NO_FILE;
        if (conf->sample_rate < MIN_ADC_RATE) {
            rl_log(WARNING, "too low sample rate. Setting rate to 1kSps");
            conf->sample_rate = MIN_ADC_RATE;
        }
        break;
    default:
        rl_log(ERROR, "wrong mode");
        return FAILURE;
    }

    // check input
    if (check_sample_rate(conf->sample_rate) == FAILURE) {
        rl_log(ERROR, "wrong sampling rate");
        return FAILURE;
    }
    if (check_update_rate(conf->update_rate) == FAILURE) {
        rl_log(ERROR, "wrong update rate");
        return FAILURE;
    }
    if (conf->update_rate != 1 && conf->enable_web_server == 1) {
        rl_log(WARNING, "webserver plot does not work with update rates >1. "
                        "Disabling webserver ...");
        conf->enable_web_server = 0;
    }

    // check ambient configuration
    if (conf->ambient.enabled == AMBIENT_ENABLED &&
        conf->file_format == NO_FILE) {
        rl_log(
            WARNING,
            "Ambient logging not possible without file. Disabling ambient ...");
        conf->ambient.enabled = AMBIENT_DISABLED;
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
    hw_init(conf);

    // check ambient sensor available
    if (conf->ambient.enabled == AMBIENT_ENABLED &&
        conf->ambient.sensor_count == 0) {
        conf->ambient.enabled = AMBIENT_DISABLED;
        rl_log(WARNING, "No ambient sensor found. Disabling ambient ...");
    }

    // SAMPLING
    rl_log(INFO, "sampling start");
    hw_sample(conf, file_comment);
    rl_log(INFO, "sampling finished");

    // FINISH
    hw_close(conf);

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
