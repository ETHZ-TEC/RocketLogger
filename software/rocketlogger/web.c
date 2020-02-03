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

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include "calibration.h"
#include "log.h"
#include "pru.h"
#include "rl.h"
#include "sem.h"
#include "util.h"

#include "web.h"

/**
 * Merge high/low currents for web interface.
 *
 * @param valid Valid information of low range current channels
 * @param dest Pointer to destination array
 * @param src Pointer to source array
 * @param config Pointer to current {@link rl_config_t} configuration
 */
static void web_merge_currents(uint8_t const *const valid, int64_t *dest,
                               int64_t const *const src,
                               rl_config_t const *const config);

web_shm_t *web_create_shm(void) {
    int shm_id = shmget(RL_SHMEM_DATA_KEY, sizeof(web_shm_t),
                        IPC_CREAT | RL_SHMEM_PERMISSIONS);
    if (shm_id == -1) {
        rl_log(RL_LOG_ERROR,
               "Failed creating shared web data memory; %d message: %s", errno,
               strerror(errno));
        return NULL;
    }
    web_shm_t *web_data = (web_shm_t *)shmat(shm_id, NULL, 0);

    if (web_data == (void *)-1) {
        rl_log(RL_LOG_ERROR,
               "Failed mapping shared web data memory; %d message: %s", errno,
               strerror(errno));
        return NULL;
    }

    return web_data;
}

web_shm_t *web_open_shm(void) {
    int shm_id =
        shmget(RL_SHMEM_DATA_KEY, sizeof(web_shm_t), RL_SHMEM_PERMISSIONS);
    if (shm_id == -1) {
        rl_log(RL_LOG_ERROR,
               "Failed getting shared web data memory; %d message: %s", errno,
               strerror(errno));
        return NULL;
    }
    web_shm_t *web_data = (web_shm_t *)shmat(shm_id, NULL, 0);

    if (web_data == (void *)-1) {
        rl_log(RL_LOG_ERROR,
               "Failed mapping shared web data memory; %d message: %s", errno,
               strerror(errno));
        return NULL;
    }

    return web_data;
}

void web_close_shm(web_shm_t const *web_shm) {
    int res = shmdt(web_shm);
    if (res != 0) {
        rl_log(RL_LOG_ERROR,
               "Failed detaching shared web data memory; %d message: %s", errno,
               strerror(errno));
    }
}

void web_buffer_reset(web_buffer_t *const buffer, int element_size,
                      int length) {
    buffer->element_size = element_size;
    buffer->length = length;
    buffer->filled = 0;
    buffer->head = 0;
}

void web_buffer_add(web_buffer_t *const buffer, int64_t const *const data) {
    memcpy((buffer->data) +
               buffer->head * buffer->element_size / sizeof(int64_t),
           data, buffer->element_size);
    if (buffer->filled < buffer->length) {
        buffer->filled++;
    }
    buffer->head = (buffer->head + 1) % buffer->length;
}

int64_t *web_buffer_get(web_buffer_t *const buffer, int num) {
    int pos = ((int)buffer->head + (int)buffer->length - 1 - num) %
              (int)buffer->length;

    return buffer->data + pos * buffer->element_size / sizeof(int64_t);
}

int web_handle_data(web_shm_t *const web_data_ptr, int sem_id,
                    int32_t const *analog_buffer,
                    uint32_t const *digital_buffer, size_t buffer_size,
                    rl_timestamp_t const *const timestamp_realtime,
                    rl_config_t const *const config) {

    // count channels
    int num_bin_channels = 0;
    if (config->digital_enable) {
        num_bin_channels = RL_CHANNEL_DIGITAL_COUNT;
    }

    int num_channels = count_channels(config->channel_enable);

    // AVERAGE DATA //

    // averaged data for web
    uint32_t avg_window[WEB_BUFFER_COUNT] = {buffer_size / BUFFER1_SIZE,
                                             buffer_size / BUFFER10_SIZE,
                                             buffer_size / BUFFER100_SIZE};
    int64_t avg_data[WEB_BUFFER_COUNT][RL_CHANNEL_COUNT] = {{0}};
    uint32_t bin_avg_data[WEB_BUFFER_COUNT][RL_CHANNEL_DIGITAL_COUNT] = {{0}};
    uint8_t avg_valid[WEB_BUFFER_COUNT]
                     [RL_CHANNEL_SWITCHED_COUNT] = {{1, 1}, {1, 1}, {1, 1}};

    // WEB DATA //

    // data for web server
    int64_t web_data[WEB_BUFFER_COUNT][BUFFER1_SIZE]
                    [web_data_ptr->num_channels];

    // process data buffers
    for (size_t i = 0; i < buffer_size; i++) {
        // point to sample buffer to store by default (updated for aggregates)
        int32_t const *const analog_data = analog_buffer + i * RL_CHANNEL_COUNT;
        uint32_t const *const digital_data = digital_buffer + i;

        // overall channel index
        size_t index = 0;

        // process analog data of selected channels
        for (int j = 0; j < RL_CHANNEL_COUNT; j++) {
            if (config->channel_enable[j]) {
                avg_data[WEB_BUFFER1_INDEX][index] += analog_data[j];
                index++;
            }
        }

        // process binary channels
        int binary_index;
        if (config->digital_enable) {
            uint32_t binary_mask = PRU_DIGITAL_INPUT1_MASK;
            for (int j = 0; j < RL_CHANNEL_DIGITAL_COUNT; j++) {
                if (*digital_data & binary_mask) {
                    bin_avg_data[WEB_BUFFER1_INDEX][j] += 1;
                }
                binary_mask = binary_mask << 1;
            }
            binary_index += RL_CHANNEL_DIGITAL_COUNT;
        }

        // average valid information
        for (int j = 0; j <= WEB_BUFFER100_INDEX; j++) {
            avg_valid[j][0] =
                avg_valid[j][0] & (*digital_data & PRU_DIGITAL_I1L_VALID_MASK);
            avg_valid[j][1] =
                avg_valid[j][1] & (*digital_data & PRU_DIGITAL_I2L_VALID_MASK);
        }

        // HANDLE AVERAGE DATA //

        // buffer 1s/div
        if ((i + 1) % avg_window[WEB_BUFFER1_INDEX] == 0) {
            // average channel data
            for (int j = 0; j < num_channels; j++) {
                avg_data[WEB_BUFFER1_INDEX][j] /= avg_window[WEB_BUFFER1_INDEX];
                avg_data[WEB_BUFFER10_INDEX][j] +=
                    avg_data[WEB_BUFFER1_INDEX][j];
            }

            // merge_currents (for web)
            web_merge_currents(avg_valid[WEB_BUFFER1_INDEX],
                               &web_data[WEB_BUFFER1_INDEX]
                                        [i / avg_window[WEB_BUFFER1_INDEX]]
                                        [num_bin_channels],
                               avg_data[WEB_BUFFER1_INDEX], config);

            // average bin channels
            for (int j = 0; j < num_bin_channels; j++) {
                bin_avg_data[WEB_BUFFER10_INDEX][j] +=
                    bin_avg_data[WEB_BUFFER1_INDEX][j];

                // store bin channels for web
                if (bin_avg_data[WEB_BUFFER1_INDEX][j] >=
                    (avg_window[WEB_BUFFER1_INDEX] / 2)) {
                    web_data[WEB_BUFFER1_INDEX]
                            [i / avg_window[WEB_BUFFER1_INDEX]][j] = 1;
                } else {
                    web_data[WEB_BUFFER1_INDEX]
                            [i / avg_window[WEB_BUFFER1_INDEX]][j] = 0;
                }
            }

            // reset values
            memset(avg_data[WEB_BUFFER1_INDEX], 0,
                   sizeof(int64_t) * num_channels);
            memset(bin_avg_data[WEB_BUFFER1_INDEX], 0,
                   sizeof(uint32_t) * num_bin_channels);
            avg_valid[WEB_BUFFER1_INDEX][0] = 1;
            avg_valid[WEB_BUFFER1_INDEX][1] = 1;
        }

        // buffer 10s/div
        if ((i + 1) % avg_window[WEB_BUFFER10_INDEX] == 0) {

            // average
            for (int j = 0; j < num_channels; j++) {
                avg_data[WEB_BUFFER10_INDEX][j] /=
                    (avg_window[WEB_BUFFER10_INDEX] /
                     avg_window[WEB_BUFFER1_INDEX]);
                avg_data[WEB_BUFFER100_INDEX][j] +=
                    avg_data[WEB_BUFFER10_INDEX][j];
            }

            // merge_currents (for web)
            web_merge_currents(avg_valid[WEB_BUFFER10_INDEX],
                               &web_data[WEB_BUFFER10_INDEX]
                                        [i / avg_window[WEB_BUFFER10_INDEX]]
                                        [num_bin_channels],
                               avg_data[WEB_BUFFER10_INDEX], config);

            // average bin channels
            for (int j = 0; j < num_bin_channels; j++) {

                bin_avg_data[WEB_BUFFER100_INDEX][j] +=
                    bin_avg_data[WEB_BUFFER10_INDEX][j];

                // store bin channels for web
                if (bin_avg_data[WEB_BUFFER10_INDEX][j] >=
                    (avg_window[WEB_BUFFER10_INDEX] / 2)) {
                    web_data[WEB_BUFFER10_INDEX]
                            [i / avg_window[WEB_BUFFER10_INDEX]][j] = 1;
                } else {
                    web_data[WEB_BUFFER10_INDEX]
                            [i / avg_window[WEB_BUFFER10_INDEX]][j] = 0;
                }
            }

            // reset values
            memset(avg_data[WEB_BUFFER10_INDEX], 0,
                   sizeof(int64_t) * num_channels);
            memset(bin_avg_data[WEB_BUFFER10_INDEX], 0,
                   sizeof(uint32_t) * num_bin_channels);
            avg_valid[WEB_BUFFER10_INDEX][0] = 1;
            avg_valid[WEB_BUFFER10_INDEX][1] = 1;
        }

        // buffer 100s/div
        if ((i + 1) % avg_window[WEB_BUFFER100_INDEX] == 0) {

            // average
            for (int j = 0; j < num_channels; j++) {
                avg_data[WEB_BUFFER100_INDEX][j] /=
                    (avg_window[WEB_BUFFER100_INDEX] /
                     avg_window[WEB_BUFFER10_INDEX]);
            }

            // merge_currents (for web)
            web_merge_currents(avg_valid[WEB_BUFFER100_INDEX],
                               &web_data[WEB_BUFFER100_INDEX]
                                        [i / avg_window[WEB_BUFFER100_INDEX]]
                                        [num_bin_channels],
                               avg_data[WEB_BUFFER100_INDEX], config);

            // store bin channels for web
            for (int j = 0; j < num_bin_channels; j++) {

                if (bin_avg_data[WEB_BUFFER100_INDEX][j] >=
                    (avg_window[WEB_BUFFER100_INDEX] / 2)) {
                    web_data[WEB_BUFFER100_INDEX]
                            [i / avg_window[WEB_BUFFER100_INDEX]][j] = 1;
                } else {
                    web_data[WEB_BUFFER100_INDEX]
                            [i / avg_window[WEB_BUFFER100_INDEX]][j] = 0;
                }
            }

            // reset values
            memset(avg_data[WEB_BUFFER100_INDEX], 0,
                   sizeof(int64_t) * num_channels);
            memset(bin_avg_data[WEB_BUFFER100_INDEX], 0,
                   sizeof(uint32_t) * num_bin_channels);
            avg_valid[WEB_BUFFER100_INDEX][0] = 1;
            avg_valid[WEB_BUFFER100_INDEX][1] = 1;
        }
    }

    // WRITE WEB DATA //

    // get shared memory access
    int res = sem_wait(sem_id, SEM_INDEX_DATA, SEM_TIMEOUT_WRITE);
    if (res != SUCCESS) {
        return ERROR;
    } else {

        // write time
        web_data_ptr->time =
            timestamp_realtime->sec * 1000 + timestamp_realtime->nsec / 1000000;

        // write data to ring buffer
        web_buffer_add(&web_data_ptr->buffer[WEB_BUFFER1_INDEX],
                       &web_data[WEB_BUFFER1_INDEX][0][0]);
        web_buffer_add(&web_data_ptr->buffer[WEB_BUFFER10_INDEX],
                       &web_data[WEB_BUFFER10_INDEX][0][0]);
        web_buffer_add(&web_data_ptr->buffer[WEB_BUFFER100_INDEX],
                       &web_data[WEB_BUFFER100_INDEX][0][0]);

        // release shared memory
        sem_set(sem_id, SEM_INDEX_DATA, 1);
    }

    return SUCCESS;
}

static void web_merge_currents(uint8_t const *const valid, int64_t *dest,
                               int64_t const *const src,
                               rl_config_t const *const config) {

    int ch_in = 0;
    int ch_out = 0;

    if (config->channel_enable[RL_CONFIG_CHANNEL_I1H] &&
        config->channel_enable[RL_CONFIG_CHANNEL_I1L]) {
        if (valid[0] == 1) {
            dest[ch_out++] = src[++ch_in];
        } else {
            dest[ch_out++] = src[ch_in++] * H_L_SCALE;
        }
        ch_in++;
    } else if (config->channel_enable[RL_CONFIG_CHANNEL_I1H]) {
        dest[ch_out++] = src[ch_in++] * H_L_SCALE;
    } else if (config->channel_enable[RL_CONFIG_CHANNEL_I1L]) {
        dest[ch_out++] = src[ch_in++];
    }
    if (config->channel_enable[RL_CONFIG_CHANNEL_V1]) {
        dest[ch_out++] = src[ch_in++];
    }
    if (config->channel_enable[RL_CONFIG_CHANNEL_V2]) {
        dest[ch_out++] = src[ch_in++];
    }

    if (config->channel_enable[RL_CONFIG_CHANNEL_I2H] &&
        config->channel_enable[RL_CONFIG_CHANNEL_I2L]) {
        if (valid[1] == 1) {
            dest[ch_out++] = src[++ch_in];
        } else {
            dest[ch_out++] = src[ch_in++] * H_L_SCALE;
        }
        ch_in++;
    } else if (config->channel_enable[RL_CONFIG_CHANNEL_I2H]) {
        dest[ch_out++] = src[ch_in++] * H_L_SCALE;
    } else if (config->channel_enable[RL_CONFIG_CHANNEL_I2L]) {
        dest[ch_out++] = src[ch_in++];
    }

    if (config->channel_enable[RL_CONFIG_CHANNEL_V3]) {
        dest[ch_out++] = src[ch_in++];
    }
    if (config->channel_enable[RL_CONFIG_CHANNEL_V4]) {
        dest[ch_out++] = src[ch_in++];
    }
}
