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

#include <stdio.h>
#include <string.h>

#include <unistd.h>

#include "log.h"
#include "rl.h"

#include "calibration.h"

/// Global calibration data structure.
rl_calibration_t rl_calibration;

void calibration_reset_offsets(void) {
    for (int i = 0; i < RL_CHANNEL_COUNT; i++) {
        rl_calibration.offsets[i] = 0;
    }
}

void calibration_reset_scales(void) {
    for (int i = 0; i < RL_CHANNEL_COUNT; i++) {
        rl_calibration.scales[i] = 1;
    }
}

int calibration_load(void) {
    int ret;
    char const *calibration_file_name = NULL;

    // check if user/system calibration file existing
    ret = access(RL_CALIBRATION_USER_FILE, R_OK);
    if (ret == 0) {
        calibration_file_name = RL_CALIBRATION_USER_FILE;
    } else {
        ret = access(RL_CALIBRATION_SYSTEM_FILE, R_OK);
        if (ret == 0) {
            calibration_file_name = RL_CALIBRATION_SYSTEM_FILE;
        } else {
            // no calibration file available
            calibration_reset_offsets();
            calibration_reset_scales();
            rl_status.calibration_time = 0;
            rl_status.calibration_file[0] = 0; // empty string
            return ERROR;
        }
    }

    FILE *file = fopen(calibration_file_name, "r");
    if (file == NULL) {
        // no calibration file available
        calibration_reset_offsets();
        calibration_reset_scales();
        rl_status.calibration_time = 0;
        return ERROR;
    }

    // read calibration
    rl_calibration_file_t calibration_file;
    fread(&calibration_file, sizeof(rl_calibration_file_t), 1, file);

    // check data
    if (calibration_file.file_magic != RL_CALIBRATION_FILE_MAGIC) {
        rl_log(RL_LOG_ERROR, "invalid calibration file magic %x",
               calibration_file.file_magic);
        return ERROR;
    }
    if (calibration_file.file_version != RL_CALIBRATION_FILE_VERSION) {
        rl_log(RL_LOG_ERROR, "unsupported calibration file version %d",
               calibration_file.file_version);
        return ERROR;
    }
    if (calibration_file.header_length != RL_CALIBRATION_FILE_HEADER_LENGTH) {
        rl_log(RL_LOG_ERROR, "invalid calibration file header length %d",
               calibration_file.header_length);
        return ERROR;
    }

    memcpy(&rl_calibration, &(calibration_file.data), sizeof(rl_calibration_t));

    // store calibration info information to status
    rl_status.calibration_time = calibration_file.calibration_time;
    strncpy(rl_status.calibration_file, calibration_file_name,
            sizeof(rl_status.calibration_file) - 1);

    // close file
    fclose(file);

    return SUCCESS;
}
