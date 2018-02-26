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

#include "rl_util.h"

/// Number of possible sampling rates
#define NUMBER_SAMPLE_RATES 10
/// Possible sampling rates
int possible_sample_rates[NUMBER_SAMPLE_RATES] = {
    1, 10, 100, 1000, 2000, 4000, 8000, 16000, 32000, 64000};

/// Number of possible update rates
#define NUMBER_UPDATE_RATES 4
/// Possible update rates
int possible_update_rates[NUMBER_UPDATE_RATES] = {1, 2, 5, 10};

/**
 * Check if provided sampling rate is possible
 * @param sample_rate Sampling rate
 * @return {@link SUCCESS} if possible, {@link FAILURE} otherwise
 */
int check_sample_rate(int sample_rate) {
    for (int i = 0; i < NUMBER_SAMPLE_RATES; i++) {
        if (possible_sample_rates[i] == sample_rate) {
            return SUCCESS;
        }
    }
    return FAILURE;
}

/**
 * Check if provided update rate is possible
 * @param update_rate Update rate
 * @return {@link SUCCESS} if possible, {@link FAILURE} otherwise
 */
int check_update_rate(int update_rate) {
    for (int i = 0; i < NUMBER_UPDATE_RATES; i++) {
        if (possible_update_rates[i] == update_rate) {
            return SUCCESS;
        }
    }
    return FAILURE;
}

/**
 * Get process ID (PID) of background sampling process
 * @return PID of background process
 */
pid_t get_pid(void) {

    // open file
    pid_t pid;
    FILE* file = fopen(PID_FILE, "r");
    if (file == NULL) { // no pid found -> no process running
        return FAILURE;
    }

    // read pid
    fread(&pid, sizeof(pid_t), 1, file); // get PID of background process

    // close file
    fclose(file);

    return pid;
}

/**
 * Store process ID (PID)
 * @param pid Own PID
 * @return {@link SUCCESS} in case of success, {@link FAILURE} otherwise.
 */
int set_pid(pid_t pid) {

    // open file
    FILE* file = fopen(PID_FILE, "w");
    if (file == NULL) {
        rl_log(ERROR, "failed to create pid file");
        return FAILURE;
    }

    // write pid
    fwrite(&pid, sizeof(pid_t), 1, file);

    // close file
    fclose(file);

    return SUCCESS;
}
