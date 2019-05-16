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

#ifndef WEB_H_
#define WEB_H_

#include <stdint.h>

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

/**
 * Typedef for shared web server data structure.
 */
typedef struct web_shm web_shm_t;

/**
 * Create shared memory for data exchange with web server.
 *
 * @return pointer to shared memory, NULL in case of failure
 */
web_shm_t *web_create_shm(void);

/**
 * Open existing shared memory for data exchange with web server.
 *
 * @return pointer to shared memory, NULL in case of failure
 */
web_shm_t *web_open_shm(void);

/**
 * Reset web data ring buffer.
 *
 * @param buffer Pointer to ring buffer to reset
 * @param element_size Desired element size in bytes
 * @param length Buffer length in elements
 */
void web_buffer_reset(struct ringbuffer *const buffer, int element_size,
                      int length);

/**
 * Add element to ring buffer.
 *
 * @param buffer Pointer to ring buffer
 * @param data Pointer to data array to add
 */
void web_buffer_add(struct ringbuffer *const buffer, int64_t const *const data);

/**
 * Get pointer to a specific element of a ringbuffer.
 *
 * @param buffer Pointer to ring buffer
 * @param num Element number (0 corresponds to the newest element)
 * @return pointer to desired element
 */
int64_t *web_buffer_get(struct ringbuffer *const buffer, int num);

/**
 * Process the data buffer for the web interface.
 *
 * @param web_data_ptr Pointer to shared web data
 * @param sem_id ID of semaphores for shared web data
 * @param buffer_addr Pointer to buffer to handle
 * @param samples_count Number of samples to read
 * @param timestamp_realtime {@link time_stamp} with realtime clock value
 * @param conf Current {@link rl_conf} configuration.
 * @return {@link SUCCESS} on successful processing, {@link FAILURE} otherwise

 */
int web_handle_data(web_shm_t *const web_data_ptr, int sem_id,
                    void const *buffer_addr, uint32_t samples_count,
                    struct time_stamp const *const timestamp_realtime,
                    struct rl_conf const *const conf);

#endif /* WEB_H_ */
