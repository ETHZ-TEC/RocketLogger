/**
 * Copyright (c) 2016-2019, ETH Zurich, Computer Engineering Group
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
#include "sem.h"
#include "types.h"
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
    int shm_id = shmget(SHMEM_DATA_KEY, sizeof(web_shm_t),
                        IPC_CREAT | SHMEM_PERMISSIONS);
    if (shm_id == -1) {
        rl_log(ERROR, "Failed creating shared web data memory; %d message: %s",
               errno, strerror(errno));
        return NULL;
    }
    web_shm_t *web_data = (web_shm_t *)shmat(shm_id, NULL, 0);

    if (web_data == (void *)-1) {
        rl_log(ERROR, "Failed mapping shared web data memory; %d message: %s",
               errno, strerror(errno));
        return NULL;
    }

    return web_data;
}

web_shm_t *web_open_shm(void) {
    int shm_id = shmget(SHMEM_DATA_KEY, sizeof(web_shm_t), SHMEM_PERMISSIONS);
    if (shm_id == -1) {
        rl_log(ERROR, "Failed getting shared web data memory; %d message: %s",
               errno, strerror(errno));
        return NULL;
    }
    web_shm_t *web_data = (web_shm_t *)shmat(shm_id, NULL, 0);

    if (web_data == (void *)-1) {
        rl_log(ERROR, "Failed mapping shared web data memory; %d message: %s",
               errno, strerror(errno));
        return NULL;
    }

    return web_data;
}

void web_close_shm(web_shm_t const *web_shm) {
    int res = shmdt(web_shm);
    if (res != 0) {
        rl_log(ERROR, "Failed detaching shared web data memory; %d message: %s",
               errno, strerror(errno));
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
                    void const *buffer_addr, uint32_t samples_count,
                    rl_timestamp_t const *const timestamp_realtime,
                    rl_config_t const *const config) {

    // count channels
    int num_bin_channels = 0;
    if (config->digital_input_enable) {
        num_bin_channels = NUM_DIGITAL_INPUTS;
    }

    int num_channels = count_channels(config->channels);

    // AVERAGE DATA //

    // averaged data for web
    uint32_t avg_window[WEB_RING_BUFFER_COUNT] = {
        samples_count / BUFFER1_SIZE, samples_count / BUFFER10_SIZE,
        samples_count / BUFFER100_SIZE};
    int64_t avg_data[WEB_RING_BUFFER_COUNT][NUM_CHANNELS] = {{0}};
    uint32_t bin_avg_data[WEB_RING_BUFFER_COUNT][NUM_DIGITAL_INPUTS] = {{0}};
    uint8_t avg_valid[WEB_RING_BUFFER_COUNT]
                     [NUM_I_CHANNELS] = {{1, 1}, {1, 1}, {1, 1}};

    // WEB DATA //

    // data for web server
    int64_t web_data[WEB_RING_BUFFER_COUNT][BUFFER1_SIZE]
                    [web_data_ptr->num_channels];

    // HANDLE BUFFER //
    for (uint32_t i = 0; i < samples_count; i++) {

        // channel data variables
        uint32_t bin_data;

        // read binary channels
        uint8_t bin_adc1 = (*((int8_t *)(buffer_addr)));
        uint8_t bin_adc2 = (*((int8_t *)(buffer_addr + 1)));

        buffer_addr += PRU_DIG_SIZE;

        // read and scale values (if channel selected)
        int ch = 0;
        for (int j = 0; j < NUM_CHANNELS; j++) {
            if (config->channels[j]) {
                int32_t adc_value =
                    *((int32_t *)(buffer_addr + j * PRU_SAMPLE_SIZE));
                int32_t channel_value =
                    (int32_t)((adc_value + calibration_data.offsets[j]) *
                              calibration_data.scales[j]);
                avg_data[BUF1_INDEX][ch] += channel_value;

                ch++;
            }
        }
        buffer_addr += NUM_CHANNELS * PRU_SAMPLE_SIZE;

        // BINARY CHANNELS //

        // mask and combine digital inputs, if requested
        int bin_channel_pos;
        if (config->digital_input_enable) {
            bin_data = ((bin_adc1 & PRU_BINARY_MASK) >> 1) |
                       ((bin_adc2 & PRU_BINARY_MASK) << 2);
            bin_channel_pos = NUM_DIGITAL_INPUTS;
            // average digital inputs
            int32_t MASK = 1;
            for (int j = 0; j < num_bin_channels; j++) {
                if ((bin_data & MASK) > 0) {
                    bin_avg_data[BUF1_INDEX][j] += 1;
                }
                MASK = MASK << 1;
            }
        } else {
            bin_channel_pos = 0;
        }

        // mask and combine valid info
        uint8_t valid1 = (~bin_adc1) & PRU_VALID_MASK;
        uint8_t valid2 = (~bin_adc2) & PRU_VALID_MASK;

        if (config->channels[I1L_INDEX]) {
            bin_data = bin_data | (valid1 << bin_channel_pos);
            bin_channel_pos++;
        }
        if (config->channels[I2L_INDEX]) {
            bin_data = bin_data | (valid2 << bin_channel_pos);
            bin_channel_pos++;
        }

        // average valid info
        for (int j = 0; j <= BUF100_INDEX; j++) {
            avg_valid[j][0] = avg_valid[j][0] & valid1;
            avg_valid[j][1] = avg_valid[j][1] & valid2;
        }

        // HANDLE AVERAGE DATA //

        // buffer 1s/div
        if ((i + 1) % avg_window[BUF1_INDEX] == 0) {

            // average channel data
            for (int j = 0; j < num_channels; j++) {
                avg_data[BUF1_INDEX][j] /= avg_window[BUF1_INDEX];
                avg_data[BUF10_INDEX][j] += avg_data[BUF1_INDEX][j];
            }

            // merge_currents (for web)
            web_merge_currents(avg_valid[BUF1_INDEX],
                               &web_data[BUF1_INDEX][i / avg_window[BUF1_INDEX]]
                                        [num_bin_channels],
                               avg_data[BUF1_INDEX], config);

            // average bin channels
            for (int j = 0; j < num_bin_channels; j++) {
                bin_avg_data[BUF10_INDEX][j] += bin_avg_data[BUF1_INDEX][j];

                // store bin channels for web
                if (bin_avg_data[BUF1_INDEX][j] >=
                    (avg_window[BUF1_INDEX] / 2)) {
                    web_data[BUF1_INDEX][i / avg_window[BUF1_INDEX]][j] = 1;
                } else {
                    web_data[BUF1_INDEX][i / avg_window[BUF1_INDEX]][j] = 0;
                }
            }

            // reset values
            memset(avg_data[BUF1_INDEX], 0, sizeof(int64_t) * num_channels);
            memset(bin_avg_data[BUF1_INDEX], 0,
                   sizeof(uint32_t) * num_bin_channels);
            avg_valid[BUF1_INDEX][0] = 1;
            avg_valid[BUF1_INDEX][1] = 1;
        }

        // buffer 10s/div
        if ((i + 1) % avg_window[BUF10_INDEX] == 0) {

            // average
            for (int j = 0; j < num_channels; j++) {
                avg_data[BUF10_INDEX][j] /=
                    (avg_window[BUF10_INDEX] / avg_window[BUF1_INDEX]);
                avg_data[BUF100_INDEX][j] += avg_data[BUF10_INDEX][j];
            }

            // merge_currents (for web)
            web_merge_currents(avg_valid[BUF10_INDEX],
                               &web_data[BUF10_INDEX]
                                        [i / avg_window[BUF10_INDEX]]
                                        [num_bin_channels],
                               avg_data[BUF10_INDEX], config);

            // average bin channels
            for (int j = 0; j < num_bin_channels; j++) {

                bin_avg_data[BUF100_INDEX][j] += bin_avg_data[BUF10_INDEX][j];

                // store bin channels for web
                if (bin_avg_data[BUF10_INDEX][j] >=
                    (avg_window[BUF10_INDEX] / 2)) {
                    web_data[BUF10_INDEX][i / avg_window[BUF10_INDEX]][j] = 1;
                } else {
                    web_data[BUF10_INDEX][i / avg_window[BUF10_INDEX]][j] = 0;
                }
            }

            // reset values
            memset(avg_data[BUF10_INDEX], 0, sizeof(int64_t) * num_channels);
            memset(bin_avg_data[BUF10_INDEX], 0,
                   sizeof(uint32_t) * num_bin_channels);
            avg_valid[BUF10_INDEX][0] = 1;
            avg_valid[BUF10_INDEX][1] = 1;
        }

        // buffer 100s/div
        if ((i + 1) % avg_window[BUF100_INDEX] == 0) {

            // average
            for (int j = 0; j < num_channels; j++) {
                avg_data[BUF100_INDEX][j] /=
                    (avg_window[BUF100_INDEX] / avg_window[BUF10_INDEX]);
            }

            // merge_currents (for web)
            web_merge_currents(avg_valid[BUF100_INDEX],
                               &web_data[BUF100_INDEX]
                                        [i / avg_window[BUF100_INDEX]]
                                        [num_bin_channels],
                               avg_data[BUF100_INDEX], config);

            // store bin channels for web
            for (int j = 0; j < num_bin_channels; j++) {

                if (bin_avg_data[BUF100_INDEX][j] >=
                    (avg_window[BUF100_INDEX] / 2)) {
                    web_data[BUF100_INDEX][i / avg_window[BUF100_INDEX]][j] = 1;
                } else {
                    web_data[BUF100_INDEX][i / avg_window[BUF100_INDEX]][j] = 0;
                }
            }

            // reset values
            memset(avg_data[BUF100_INDEX], 0, sizeof(int64_t) * num_channels);
            memset(bin_avg_data[BUF100_INDEX], 0,
                   sizeof(uint32_t) * num_bin_channels);
            avg_valid[BUF100_INDEX][0] = 1;
            avg_valid[BUF100_INDEX][1] = 1;
        }
    }

    // WRITE WEB DATA //

    // get shared memory access
    int res = sem_wait(sem_id, DATA_SEM, SEM_WRITE_TIME_OUT);
    if (res == TIMEOUT) {
        return FAILURE;
    } else {

        // write time
        web_data_ptr->time =
            timestamp_realtime->sec * 1000 + timestamp_realtime->nsec / 1000000;

        // write data to ring buffer
        web_buffer_add(&web_data_ptr->buffer[BUF1_INDEX],
                       &web_data[BUF1_INDEX][0][0]);
        web_buffer_add(&web_data_ptr->buffer[BUF10_INDEX],
                       &web_data[BUF10_INDEX][0][0]);
        web_buffer_add(&web_data_ptr->buffer[BUF100_INDEX],
                       &web_data[BUF100_INDEX][0][0]);

        // release shared memory
        sem_set(sem_id, DATA_SEM, 1);
    }

    return SUCCESS;
}

static void web_merge_currents(uint8_t const *const valid, int64_t *dest,
                               int64_t const *const src,
                               rl_config_t const *const config) {

    int ch_in = 0;
    int ch_out = 0;

    if (config->channels[I1H_INDEX] && config->channels[I1L_INDEX]) {
        if (valid[0] == 1) {
            dest[ch_out++] = src[++ch_in];
        } else {
            dest[ch_out++] = src[ch_in++] * H_L_SCALE;
        }
        ch_in++;
    } else if (config->channels[I1H_INDEX]) {
        dest[ch_out++] = src[ch_in++] * H_L_SCALE;
    } else if (config->channels[I1L_INDEX]) {
        dest[ch_out++] = src[ch_in++];
    }
    if (config->channels[V1_INDEX]) {
        dest[ch_out++] = src[ch_in++];
    }
    if (config->channels[V2_INDEX]) {
        dest[ch_out++] = src[ch_in++];
    }

    if (config->channels[I2H_INDEX] && config->channels[I2L_INDEX]) {
        if (valid[1] == 1) {
            dest[ch_out++] = src[++ch_in];
        } else {
            dest[ch_out++] = src[ch_in++] * H_L_SCALE;
        }
        ch_in++;
    } else if (config->channels[I2H_INDEX]) {
        dest[ch_out++] = src[ch_in++] * H_L_SCALE;
    } else if (config->channels[I2L_INDEX]) {
        dest[ch_out++] = src[ch_in++];
    }

    if (config->channels[V3_INDEX]) {
        dest[ch_out++] = src[ch_in++];
    }
    if (config->channels[V4_INDEX]) {
        dest[ch_out++] = src[ch_in++];
    }
}
