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

#ifndef WEB_H_
#define WEB_H_

#include <stdint.h>

#include "log.h"
#include "types.h"
#include "util.h"

/// Number of ring buffers in shared memory
#define WEB_RING_BUFFER_COUNT 3
/// Index of 1s/div buffer
#define BUF1_INDEX 0
/// Index of 10s/div buffer
#define BUF10_INDEX 1
/// Index of 100s/div buffer
#define BUF100_INDEX 2

/**
 * RocketLogger time scale definition
 */
enum time_scale {
    S1 = 0,  //!< 100 sample/s
    S10 = 1, //!< 10 samples/s
    S100 = 2 //!< 1 samples/s
};

/// Size of 1s/div buffer
#define BUFFER1_SIZE 100
/// Size of 10s/div buffer
#define BUFFER10_SIZE 10
/// Size of 100s/div buffer
#define BUFFER100_SIZE 1

/// Maximum number of channels in web interface (6 analog + 6 digital)
#define NUM_WEB_CHANNELS 12 // 6 analog + 6 digital
/// Number of data points in web plot
#define NUM_WEB_POINTS 1000
/// Number of time divisions in web plot
#define NUM_WEB_DIVS 10

/// Current high-low scale difference
#define H_L_SCALE 100

/**
 * Ring buffer for data exchange to web server
 */
struct ringbuffer {
    /// Size of buffer element
    uint32_t element_size;
    ///  Size of buffer in elements
    uint32_t length;
    /// Number of elements in buffer
    uint32_t filled;
    /// Current position (in elements)
    uint32_t head;
    /// Data array
    int64_t data[NUM_WEB_CHANNELS * NUM_WEB_POINTS];
};

/**
 * Shared memory struct for data exchange to web server
 */
struct web_shm {
    /// Time stamp of most recent datum (in UNIX time, UTC)
    int64_t time;
    /// Number of channels sampled
    uint32_t num_channels;
    /// Array of ring buffers for different time scales
    struct ringbuffer buffer[WEB_RING_BUFFER_COUNT];
};

struct web_shm* web_create_shm(void);
struct web_shm* web_open_shm(void);

void web_buffer_reset(struct ringbuffer* buffer, int element_size, int length);

void web_buffer_add(struct ringbuffer* buffer, int64_t* data);

int64_t* web_buffer_get(struct ringbuffer* buffer, int num);

void web_handle_data(struct web_shm* web_data_ptr, int sem_id,
                     void* buffer_addr, uint32_t sample_data_size,
                     uint32_t samples_count,
                     struct time_stamp* timestamp_realtime,
                     struct rl_conf* conf);

#endif /* WEB_H_ */
