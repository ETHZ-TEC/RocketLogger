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

#ifndef AMBIENT_H_
#define AMBIENT_H_

#include <stdio.h>

#include "rl.h"
#include "rl_file.h"
#include "util.h"

/// Ambient sensor data file name suffix
#define AMBIENT_FILE_NAME_SUFFIX "-ambient"

/// Ambient sensor read out rate in samples per second
#define AMBIENT_SAMPLING_RATE 1

/// Ambient sensor data file block size in measurements
#define AMBIENT_DATA_BLOCK_SIZE 1

/**
 * Derive the ambient file name from the data file name.
 *
 * @param data_file_name The data file name
 * @return Pointer to the buffer of the derived ambient file name
 */
char *ambient_get_file_name(char const *const data_file_name);

/**
 * @todo document
 */
void ambient_setup_lead_in(rl_file_lead_in_t *const lead_in);

/**
 * @todo document
 */
void ambient_setup_header(rl_file_header_t *const header,
                          rl_config_t const *const config);

/**
 * Handle a ambient data buffer, dependent on current configuration.
 *
 * @param ambient_file Ambient file to write to
 * @param buffer PRU data buffer to process
 * @param samples_count Number of samples in the buffer
 * @param timestamp_realtime Timestamp sampled from realtime clock
 * @param timestamp_monotonic Timestamp sampled from monotonic clock
 * @param config Current measurement configuration
 */
void ambient_append_data(FILE *ambient_file, void const *buffer,
                         uint32_t samples_count,
                         rl_timestamp_t const *const timestamp_realtime,
                         rl_timestamp_t const *const timestamp_monotonic,
                         rl_config_t const *const config);

#endif /* AMBIENT_H_ */
