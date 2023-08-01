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

#ifndef CALIBRATION_H_
#define CALIBRATION_H_

#include <stdint.h>

#include "rl.h"

/// Calibration file header magic
#define RL_CALIBRATION_FILE_MAGIC 0x434C5225
/// Calibration file header version
#define RL_CALIBRATION_FILE_VERSION 0x02
/// Calibration file header length
#define RL_CALIBRATION_FILE_HEADER_LENGTH 0x10

/**
 * RocketLogger calibration data structure.
 */
struct rl_calibration {
    /// Channel offsets (in bit)
    int offsets[RL_CHANNEL_COUNT];
    /// Channel scales
    double scales[RL_CHANNEL_COUNT];
} __attribute__((packed));

/**
 * Typedef for RocketLogger calibration data.
 */
typedef struct rl_calibration rl_calibration_t;

/**
 * RocketLogger calibration file data structure.
 */
struct rl_calibration_file {
    /// File magic constant
    uint32_t file_magic;
    /// File version number
    uint16_t file_version;
    /// Total size of the header in bytes
    uint16_t header_length;
    /// Timestamp of the measurements used for calibration generation
    uint64_t calibration_time;
    /// The actual calibration data
    rl_calibration_t data;
} __attribute__((packed));

/**
 * Typedef for RocketLogger calibration file structure.
 */
typedef struct rl_calibration_file rl_calibration_file_t;

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
 * @note Updates the RocketLogger status. Manually update shared status after
 * loading the calibration as needed.
 *
 * @return Returns 0 on success, negative on failure with errno set accordingly
 */
int calibration_load(void);

/**
 * Global calibration data structure.
 */
extern rl_calibration_t rl_calibration;

#endif /* CALIBRATION_H_ */
