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
 * ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef FILE_HANDLING_H_
#define FILE_HANDLING_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "log.h"
#include "rl_file.h"
#include "sem.h"
#include "types.h"
#include "util.h"

/// CSV value delimiter character
#define CSV_DELIMITER ","

// FUNCTIONS
void file_setup_lead_in(struct rl_file_lead_in* lead_in, struct rl_conf* conf);
void file_setup_header(struct rl_file_header* file_header, struct rl_conf* conf,
                       char* comment);
void file_store_header_bin(FILE* data, struct rl_file_header* file_header);
void file_store_header_csv(FILE* data, struct rl_file_header* file_header);
void file_update_header_bin(FILE* data, struct rl_file_header* file_header);
void file_update_header_csv(FILE* data, struct rl_file_header* file_header);
void file_handle_data(FILE* data_file, void* buffer_addr,
                      uint32_t sample_data_size, uint32_t samples_count,
                      struct time_stamp* timestamp_realtime,
                      struct time_stamp* timestamp_monotonic,
                      struct rl_conf* conf);

#endif /* FILE_HANDLING_H_ */
