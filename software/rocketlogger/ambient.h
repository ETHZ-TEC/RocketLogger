/**
 * Copyright (c) 2016-2017, Swiss Federal Institute of Technology (ETH Zurich)
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
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef AMBIENT_H_
#define AMBIENT_H_

#include <stdint.h>

#include "log.h"
#include "rl_file.h"
#include "types.h"
#include "util.h"

#define AMBIENT_SAMPLING_RATE 1 // Sps
#define AMBIENT_DATA_BLOCK_SIZE 1

void ambient_store_data(FILE* ambient_file,
                        struct time_stamp* timestamp_realtime,
                        struct time_stamp* timestamp_monotonic,
                        struct rl_conf* conf);
void ambient_set_file_name(struct rl_conf* conf);
void ambient_setup_lead_in(struct rl_file_lead_in* lead_in,
                           struct rl_conf* conf);
void ambient_setup_channels(struct rl_file_header* file_header,
                            struct rl_conf* conf);
void ambient_setup_header(struct rl_file_header* file_header,
                          struct rl_conf* conf, char* comment);

#endif /* AMBIENT_H_ */
