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

#ifndef RL_SOCKET_H_
#define RL_SOCKET_H_

#include <stdint.h>

#include "rl.h"
#include "util.h"

/**
 * Initialize socket for data streaming.
 *
 * @return Returns 0 on success, negative on failure with errno set accordingly
 */
int rl_socket_init(void);

/**
 * Deinitialize the data streaming socket.
 *
 * @return Returns 0 on success, negative on failure with errno set accordingly
 */
int rl_socket_deinit(void);

/**
 * Initialize metadata for data socket.
 *
 * @param config Current measurement configuration
 * @return Returns 0 on success, negative on failure with errno set accordingly
 */
int rl_socket_metadata(rl_config_t const *const config);

/**
 * Process the data buffer for publishing to data socket.
 *
 * @param analog_buffer Analog data buffer to process
 * @param digital_buffer Digital data buffer to process
 * @param ambient_buffer Ambient sensor data buffer to process
 * @param buffer_size Number of data samples in the buffer
 * @param ambient_buffer_size Number of sensor samples in the buffer
 * @param timestamp_realtime Timestamp sampled from realtime clock
 * @param timestamp_monotonic Timestamp sampled from monotonic clock
 * @param config Current measurement configuration
 * @return Returns 0 on success, negative on failure with errno set accordingly
 */
int rl_socket_handle_data(int32_t const *analog_buffer,
                          uint32_t const *digital_buffer,
                          int32_t const *ambient_buffer, size_t buffer_size,
                          size_t ambient_buffer_size,
                          rl_timestamp_t const *const timestamp_realtime,
                          rl_timestamp_t const *const timestamp_monotonic,
                          rl_config_t const *const config);

#endif /* RL_SOCKET_H_ */