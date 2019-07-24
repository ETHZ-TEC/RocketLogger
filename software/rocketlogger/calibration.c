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

#include "calibration.h"

rl_calibration_t calibration_data;

void calibration_reset_offsets(void) {
    for (int i = 0; i < NUM_CHANNELS; i++) {
        calibration_data.offsets[i] = 0;
    }
}

void calibration_reset_scales(void) {
    for (int i = 0; i < NUM_CHANNELS; i++) {
        calibration_data.scales[i] = 1;
    }
}

int calibration_load(rl_config_t const *const config) {
    FILE *file = fopen(CALIBRATION_FILE, "r");
    if (file == NULL) {
        // no calibration file available
        calibration_reset_offsets();
        calibration_reset_scales();
        status.calibration_time = 0;
        return FAILURE;
    }

    // read calibration
    fread(&calibration_data, sizeof(rl_calibration_t), 1, file);

    // reset calibration, if ignored
    if (config->calibration_ignore) {
        calibration_reset_offsets();
        calibration_reset_scales();
    }

    // store timestamp to config and status
    status.calibration_time = calibration_data.time;

    // close file
    fclose(file);

    return SUCCESS;
}
