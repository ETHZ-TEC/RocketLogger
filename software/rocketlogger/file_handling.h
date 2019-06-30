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

#ifndef FILE_HANDLING_H_
#define FILE_HANDLING_H_

#include <stdio.h>

#include "pru.h"
#include "rl.h"
#include "rl_file.h"
#include "util.h"

/// CSV value delimiter character
#define FILE_CSV_DELIMITER ","

/// Ambient sensor data file name suffix
#define FILE_AMBIENT_SUFFIX "-ambient"

/// Ambient sensor read out rate in samples per second
#define FILE_AMBIENT_SAMPLING_RATE 1

/// Ambient sensor data file block size in measurements
#define FILE_AMBIENT_DATA_BLOCK_SIZE 1

/**
 * Derive the ambient file name from the data file name.
 *
 * @param data_file_name The data file name
 * @return Pointer to the buffer of the derived ambient file name
 */
char *file_get_ambient_file_name(char const *const data_file_name);

/**
 * Set up data file header lead-in with current configuration.
 *
 * @param lead_in The file lead-in data structure to set up
 * @param config Current measurement configuration
 */
void file_setup_data_lead_in(rl_file_lead_in_t *const lead_in,
                             rl_config_t const *const config);

/**
 * Set up ambient file header lead-in with current configuration.
 *
 * @param lead_in The file lead-in data structure to set up
 * @param config Current measurement configuration
 */
void file_setup_ambient_lead_in(rl_file_lead_in_t *const lead_in,
                                rl_config_t const *const config);

/**
 * Set up data file header with current configuration.
 *
 * @param file_header The file header data structure to set up
 * @param config Current measurement configuration
 */
void file_setup_data_header(rl_file_header_t *const file_header,
                            rl_config_t const *const config);

/**
 * Set up ambient file header with current configuration.
 *
 * @param file_header The file header data structure to set up
 * @param config Current measurement configuration
 */
void file_setup_ambient_header(rl_file_header_t *const file_header,
                               rl_config_t const *const config);

/**
 * Store file header to file (in binary format).
 *
 * @param file_handle Data file to write to
 * @param file_header The file header data structure to store to the file
 */
void file_store_header_bin(FILE *file_handle,
                           rl_file_header_t *const file_header);

/**
 * Store file header to file (in CSV format).
 *
 * @param file_handle Data file to write to
 * @param file_header The file header data structure to store to the file
 */
void file_store_header_csv(FILE *file_handle,
                           rl_file_header_t const *const file_header);

/**
 * Update file with new header lead-in (to write current sample count) in binary
 * format.
 *
 * @param file_handle Data file to write to
 * @param file_header The file header data structure to store to the file
 */
void file_update_header_bin(FILE *file_handle,
                            rl_file_header_t const *const file_header);

/**
 * Update file with new header lead-in (to write current sample count) in CSV
 * format.
 *
 * @param file_handle Data file to write to
 * @param file_header The file header data structure to store to the file
 */
void file_update_header_csv(FILE *file_handle,
                            rl_file_header_t const *const file_header);

/**
 * Handle the sampling data buffer to add a new block to the data file.
 *
 * @param data_file Data file to write to
 * @param buffer PRU data buffer to process
 * @param buffer_size Number of samples in the buffer
 * @param timestamp_realtime Timestamp sampled from realtime clock
 * @param timestamp_monotonic Timestamp sampled from monotonic clock
 * @param config Current measurement configuration
 */
void file_add_data_block(FILE *data_file, pru_buffer_t const *const buffer,
                         uint32_t buffer_size,
                         rl_timestamp_t const *const timestamp_realtime,
                         rl_timestamp_t const *const timestamp_monotonic,
                         rl_config_t const *const config);

/**
 * Handle the sampling data buffer to add a new block to the ambient file.
 *
 * @param ambient_file Ambient file to write to
 * @param buffer PRU data buffer to process
 * @param buffer_size Number of samples in the buffer
 * @param timestamp_realtime Timestamp sampled from realtime clock
 * @param timestamp_monotonic Timestamp sampled from monotonic clock
 * @param config Current measurement configuration
 */
void file_add_ambient_block(FILE *ambient_file,
                            pru_buffer_t const *const buffer,
                            uint32_t buffer_size,
                            rl_timestamp_t const *const timestamp_realtime,
                            rl_timestamp_t const *const timestamp_monotonic,
                            rl_config_t const *const config);

#endif /* FILE_HANDLING_H_ */
