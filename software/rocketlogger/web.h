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

#ifndef WEB_H_
#define WEB_H_

#include <stdint.h>

#include "rl.h"
#include "util.h"

/// Number of ring buffers in shared memory
#define WEB_BUFFER_COUNT 3
/// Index of 1s/div buffer
#define WEB_BUFFER1_INDEX 0
/// Index of 10s/div buffer
#define WEB_BUFFER10_INDEX 1
/// Index of 100s/div buffer
#define WEB_BUFFER100_INDEX 2

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
 * RocketLogger web interface time scale definition
 */
enum web_time_scale {
    WEB_TIME_SCALE_1 = 0,   //!< 100 sample/s
    WEB_TIME_SCALE_10 = 1,  //!< 10 samples/s
    WEB_TIME_SCALE_100 = 2, //!< 1 samples/s
};

/**
 * Typedef for web interface time scale.
 */
typedef enum web_time_scale web_time_scale_t;

/**
 * Ring buffer data structure for data exchange with web server
 */
struct web_buffer {
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
 * Typedef for web server data buffer
 */
typedef struct web_buffer web_buffer_t;

/**
 * Shared memory struct for data exchange to web server
 */
struct web_shm {
    /// Time stamp of most recent datum (in UNIX time, UTC)
    int64_t time;
    /// Number of channels sampled
    uint32_t num_channels;
    /// Array of ring buffers for different time scales
    web_buffer_t buffer[WEB_BUFFER_COUNT];
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
 * @return Pointer to shared memory, NULL in case of failure
 */
web_shm_t *web_open_shm(void);

/**
 * Close shared memory mapped for data exchange with web server.
 *
 * @param web_shm Pointer to shared memory used for web server data exchange
 */
void web_close_shm(web_shm_t const *web_shm);

/**
 * Reset web data ring buffer.
 *
 * @param buffer The ring buffer data structure to reset
 * @param element_size Desired element size in bytes
 * @param length Buffer length in elements
 */
void web_buffer_reset(web_buffer_t *const buffer, int element_size, int length);

/**
 * Add element to ring buffer.
 *
 * @param buffer The ring buffer data structure to add an element to
 * @param data Pointer to data array to add
 */
void web_buffer_add(web_buffer_t *const buffer, int64_t const *const data);

/**
 * Get pointer to a specific element of a ringbuffer.
 *
 * @param buffer The ring buffer data structure to read an element from
 * @param num Element number (0 corresponds to the newest element)
 * @return Pointer to desired element in the buffer
 */
int64_t *web_buffer_get(web_buffer_t *const buffer, int num);

/**
 * Process the data buffer for the web interface.
 *
 * @param web_data The shared web data buffer
 * @param sem_id ID of semaphore set for shared web data
 * @param analog_buffer Analog data buffer to process
 * @param digital_buffer Digital data buffer to process
 * @param buffer_size Number of samples in the buffer
 * @param timestamp_realtime Timestamp sampled from realtime clock
 * @param config Current measurement configuration
 * @return Returns 0 on success, negative on failure with errno set accordingly
 */
int web_handle_data(web_shm_t *const web_data, int sem_id,
                    int32_t const *analog_buffer,
                    uint32_t const *digital_buffer, size_t buffer_size,
                    rl_timestamp_t const *const timestamp_realtime,
                    rl_config_t const *const config);

#endif /* WEB_H_ */
