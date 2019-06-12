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

#ifndef CALIBRATION_H_
#define CALIBRATION_H_

#include <stdint.h>

#include "rl.h"

/// Default system wide calibration file path
#define RL_CALIBRATION_SYSTEM_FILE "/etc/rocketlogger/calibration.dat"

/// User folder calibration file path
#define RL_CALIBRATION_USER_FILE                                               \
    "/home/rocketlogger/.config/rocketlogger/calibration.dat"

/**
 * RocketLogger calibration data structure.
 */
struct rl_calibration {
    /// Time stamp of calibration run
    uint64_t time;
    /// Calibration filename of the file loaded
    char const *file_name;
    /// Channel offsets (in bit)
    int offsets[RL_CHANNEL_COUNT];
    /// Channel scalings
    double scales[RL_CHANNEL_COUNT];
};

/**
 * Typedef for RocketLogger calibration data.
 */
typedef struct rl_calibration rl_calibration_t;

/**
 * Reset all calibration offsets to default state (0).
 */
void calibration_reset_offsets(void);

/**
 * Reset all calibration scales to default state (1).
 */
void calibration_reset_scales(void);

/**
 * Load the calibration values from calibration file.
 *
 * @param config Pointer to {@link rl_config_t} struct.
 * @return Returns 0 on success, negative on failure with errno set accordingly
 */
int calibration_load(void);

/**
 * Global calibration data structure.
 */
extern rl_calibration_t rl_calibration;

#endif /* CALIBRATION_H_ */
