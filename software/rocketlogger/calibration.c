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

#include <stdio.h>

#include "calibration.h"

void calibration_reset_offsets(void) {
    int i;
    for (i = 0; i < NUM_CHANNELS; i++) {
        calibration.offsets[i] = 0;
    }
}

void calibration_reset_scales(void) {
    int i;
    for (i = 0; i < NUM_CHANNELS; i++) {
        calibration.scales[i] = 1;
    }
}

int calibration_load(struct rl_conf const *const conf) {

    // open calibration file
    FILE *file = fopen(CALIBRATION_FILE, "r");
    if (file == NULL) {
        // no calibration file available
        calibration_reset_offsets();
        calibration_reset_scales();
        status.calibration_time = 0;
        return FAILURE;
    }
    // read calibration
    fread(&calibration, sizeof(struct rl_calibration), 1, file);

    // reset calibration, if ignored
    if (conf->calibration == CAL_IGNORE) {
        calibration_reset_offsets();
        calibration_reset_scales();
    }

    // store timestamp to conf and status
    status.calibration_time = calibration.time;

    // close file
    fclose(file);

    return SUCCESS;
}
