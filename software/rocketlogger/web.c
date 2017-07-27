/**
 * Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group
 */

#include <stdint.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include "pru.h"
#include "sem.h"
#include "types.h"

#include "web.h"

/**
 * Create shared memory for data exchange with web server
 * @return pointer to shared memory, NULL in case of failure
 */
struct web_shm* web_create_shm(void) {

    int shm_id = shmget(SHMEM_DATA_KEY, sizeof(struct web_shm),
                        IPC_CREAT | SHMEM_PERMISSIONS);
    if (shm_id == -1) {
        rl_log(ERROR, "In create_web_shm: failed to get shared data memory id; "
                      "%d message: %s",
               errno, strerror(errno));
        return NULL;
    }
    struct web_shm* web_data = (struct web_shm*)shmat(shm_id, NULL, 0);

    if (web_data == (void*)-1) {
        rl_log(ERROR, "In create_web_shm: failed to map shared data memory; %d "
                      "message: %s",
               errno, strerror(errno));
        return NULL;
    }

    return web_data;
}

/**
 * Open existing shared memory for data exchange with web server
 * @return pointer to shared memory, NULL in case of failure
 */
struct web_shm* web_open_shm(void) {

    int shm_id =
        shmget(SHMEM_DATA_KEY, sizeof(struct web_shm), SHMEM_PERMISSIONS);
    if (shm_id == -1) {
        rl_log(ERROR, "In create_web_shm: failed to get shared data memory id; "
                      "%d message: %s",
               errno, strerror(errno));
        return NULL;
    }
    struct web_shm* web_data = (struct web_shm*)shmat(shm_id, NULL, 0);

    if (web_data == (void*)-1) {
        rl_log(ERROR, "In create_web_shm: failed to map shared data memory; %d "
                      "message: %s",
               errno, strerror(errno));
        return NULL;
    }

    return web_data;
}

/**
 * Reset web data ring buffer
 * @param buffer Pointer to ring buffer to reset
 * @param element_size Desired element size in bytes
 * @param length Buffer length in elements
 */
void web_buffer_reset(struct ringbuffer* buffer, int element_size, int length) {
    buffer->element_size = element_size;
    buffer->length = length;
    buffer->filled = 0;
    buffer->head = 0;
}

/**
 * Add element to ring buffer
 * @param buffer Pointer to ring buffer
 * @param data Pointer to data array to add
 */
void web_buffer_add(struct ringbuffer* buffer, int64_t* data) {
    memcpy((buffer->data) +
               buffer->head * buffer->element_size / sizeof(int64_t),
           data, buffer->element_size);
    if (buffer->filled < buffer->length) {
        buffer->filled++;
    }
    buffer->head = (buffer->head + 1) % buffer->length;
}

/**
 * Get pointer to a specific element of a ringbuffer
 * @param buffer Pointer to ring buffer
 * @param num Element number (0 corresponds to the newest element)
 * @return pointer to desired element
 */
int64_t* web_buffer_get(struct ringbuffer* buffer, int num) {
    int pos = ((int)buffer->head + (int)buffer->length - 1 - num) %
              (int)buffer->length;

    return buffer->data + pos * buffer->element_size / sizeof(int64_t);
}

/**
 * Merge high/low currents for web interface
 * @param valid Valid information of low range current channels
 * @param dest Pointer to destination array
 * @param src Pointer to source array
 * @param conf Pointer to current {@link rl_conf} configuration
 */
void web_merge_currents(uint8_t* valid, int64_t* dest, int64_t* src,
                        struct rl_conf* conf) {

    int ch_in = 0;
    int ch_out = 0;

    if (conf->channels[I1H_INDEX] == CHANNEL_ENABLED &&
        conf->channels[I1L_INDEX] == CHANNEL_ENABLED) {
        if (valid[0] == 1) {
            dest[ch_out++] = src[++ch_in];
        } else {
            dest[ch_out++] = src[ch_in++] * H_L_SCALE;
        }
        ch_in++;
    } else if (conf->channels[I1H_INDEX] == CHANNEL_ENABLED) {
        dest[ch_out++] = src[ch_in++] * H_L_SCALE;
    } else if (conf->channels[I1L_INDEX] == CHANNEL_ENABLED) {
        dest[ch_out++] = src[ch_in++];
    }
    if (conf->channels[V1_INDEX] == CHANNEL_ENABLED) {
        dest[ch_out++] = src[ch_in++];
    }
    if (conf->channels[V2_INDEX] == CHANNEL_ENABLED) {
        dest[ch_out++] = src[ch_in++];
    }

    if (conf->channels[I2H_INDEX] == CHANNEL_ENABLED &&
        conf->channels[I2L_INDEX] == CHANNEL_ENABLED) {
        if (valid[1] == 1) {
            dest[ch_out++] = src[++ch_in];
        } else {
            dest[ch_out++] = src[ch_in++] * H_L_SCALE;
        }
        ch_in++;
    } else if (conf->channels[I2H_INDEX] == CHANNEL_ENABLED) {
        dest[ch_out++] = src[ch_in++] * H_L_SCALE;
    } else if (conf->channels[I2L_INDEX] == CHANNEL_ENABLED) {
        dest[ch_out++] = src[ch_in++];
    }

    if (conf->channels[V3_INDEX] == CHANNEL_ENABLED) {
        dest[ch_out++] = src[ch_in++];
    }
    if (conf->channels[V4_INDEX] == CHANNEL_ENABLED) {
        dest[ch_out++] = src[ch_in++];
    }
}

/**
 * Process the data buffer for the web interface
 * @param web_data_ptr Pointer to shared web data
 * @param sem_id ID of semaphores for shared web data
 * @param buffer_addr Pointer to buffer to handle
 * @param sample_data_size Size of samples to read
 * @param samples_count Number of samples to read
 * @param timestamp_realtime {@link time_stamp} with realtime clock value
 * @param conf Current {@link rl_conf} configuration.
 */
void web_handle_data(struct web_shm* web_data_ptr, int sem_id,
                     void* buffer_addr, uint32_t sample_data_size,
                     uint32_t samples_count,
                     struct time_stamp* timestamp_realtime,
                     struct rl_conf* conf) {

    // count channels
    int num_bin_channels = 0;
    if (conf->digital_inputs == DIGITAL_INPUTS_ENABLED) {
        num_bin_channels = NUM_DIGITAL_INPUTS;
    }

    int num_channels = count_channels(conf->channels);

    // AVERAGE DATA //

    // averaged data (for web and low rates)
    uint32_t avg_length[WEB_RING_BUFFER_COUNT] = {
        samples_count / BUFFER1_SIZE, samples_count / BUFFER10_SIZE,
        samples_count / BUFFER100_SIZE};
    int64_t avg_data[WEB_RING_BUFFER_COUNT][NUM_CHANNELS];
    uint32_t bin_avg_data[WEB_RING_BUFFER_COUNT][NUM_DIGITAL_INPUTS];
    uint8_t avg_valid[WEB_RING_BUFFER_COUNT]
                     [NUM_I_CHANNELS] = {{1, 1}, {1, 1}, {1, 1}};

    memset(avg_data, 0, sizeof(int64_t) * num_channels * WEB_RING_BUFFER_COUNT);
    memset(bin_avg_data, 0,
           sizeof(uint32_t) * num_bin_channels * WEB_RING_BUFFER_COUNT);

    // WEB DATA //

    // data for webserver
    int64_t web_data[WEB_RING_BUFFER_COUNT][BUFFER1_SIZE]
                    [web_data_ptr->num_channels];

    // HANDLE BUFFER //
    for (uint32_t i = 0; i < samples_count; i++) {

        // channel data variables
        uint32_t bin_data;

        // read binary channels
        uint8_t bin_adc1 = (*((int8_t*)(buffer_addr)));
        uint8_t bin_adc2 = (*((int8_t*)(buffer_addr + 1)));

        buffer_addr += PRU_DIG_SIZE;

        // read and scale values (if channel selected)
        int ch = 0;
        for (int j = 0; j < NUM_CHANNELS; j++) {
            if (conf->channels[j] == CHANNEL_ENABLED) {
                int32_t adc_value;
                if (sample_data_size == 4) {
                    adc_value =
                        *((int32_t*)(buffer_addr + sample_data_size * j));
                } else {
                    adc_value =
                        *((int16_t*)(buffer_addr + sample_data_size * j));
                }

                int32_t channel_value =
                    (int32_t)((adc_value + calibration.offsets[j]) *
                              calibration.scales[j]);
                avg_data[BUF1_INDEX][ch] += channel_value;

                ch++;
            }
        }
        buffer_addr += NUM_CHANNELS * sample_data_size;

        // BINARY CHANNELS //

        // mask and combine digital inputs, if requestet
        int bin_channel_pos;
        if (conf->digital_inputs == DIGITAL_INPUTS_ENABLED) {
            bin_data = ((bin_adc1 & BINARY_MASK) >> 1) |
                       ((bin_adc2 & BINARY_MASK) << 2);
            bin_channel_pos = NUM_DIGITAL_INPUTS;
        } else {
            bin_channel_pos = 0;
        }

        // mask and combine valid info
        uint8_t valid1 = (~bin_adc1) & VALID_MASK;
        uint8_t valid2 = (~bin_adc2) & VALID_MASK;

        if (conf->channels[I1L_INDEX] == CHANNEL_ENABLED) {
            bin_data = bin_data | (valid1 << bin_channel_pos);
            bin_channel_pos++;
        }
        if (conf->channels[I2L_INDEX] == CHANNEL_ENABLED) {
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
        if ((i + 1) % avg_length[BUF1_INDEX] == 0) {

            // average channel data
            for (int j = 0; j < num_channels; j++) {
                avg_data[BUF1_INDEX][j] /= avg_length[BUF1_INDEX];
                avg_data[BUF10_INDEX][j] += avg_data[BUF1_INDEX][j];
            }

            // merge_currents (for web)
            web_merge_currents(avg_valid[BUF1_INDEX],
                               &web_data[BUF1_INDEX][i / avg_length[BUF1_INDEX]]
                                        [num_bin_channels],
                               avg_data[BUF1_INDEX], conf);

            // average bin channels
            for (int j = 0; j < num_bin_channels; j++) {

                bin_avg_data[BUF10_INDEX][j] += bin_avg_data[BUF1_INDEX][j];

                // store bin channels for web
                if (bin_avg_data[BUF1_INDEX][j] >=
                    (avg_length[BUF1_INDEX] / 2)) {
                    web_data[BUF1_INDEX][i / avg_length[BUF1_INDEX]][j] = 1;
                } else {
                    web_data[BUF1_INDEX][i / avg_length[BUF1_INDEX]][j] = 0;
                }
            }

            // reset values
            memset(avg_data[BUF1_INDEX], 0, sizeof(int64_t) * num_channels);
            memset(bin_avg_data[BUF1_INDEX], 0,
                   sizeof(uint32_t) * num_bin_channels);
            avg_valid[BUF1_INDEX][0] = 1;
            avg_valid[BUF1_INDEX][1] = 1;
        }

        // buffer 10
        if ((i + 1) % avg_length[BUF10_INDEX] == 0) {

            // average
            for (int j = 0; j < num_channels; j++) {
                avg_data[BUF10_INDEX][j] /=
                    (avg_length[BUF10_INDEX] / avg_length[BUF1_INDEX]);
                avg_data[BUF100_INDEX][j] += avg_data[BUF10_INDEX][j];
            }

            // merge_currents (for web)
            web_merge_currents(avg_valid[BUF10_INDEX],
                               &web_data[BUF10_INDEX]
                                        [i / avg_length[BUF10_INDEX]]
                                        [num_bin_channels],
                               avg_data[BUF10_INDEX], conf);

            // average bin channels
            for (int j = 0; j < num_bin_channels; j++) {

                bin_avg_data[BUF100_INDEX][j] += bin_avg_data[BUF10_INDEX][j];

                // store bin channels for web
                if (bin_avg_data[BUF10_INDEX][j] >=
                    (avg_length[BUF10_INDEX] / 2)) {
                    web_data[BUF10_INDEX][i / avg_length[BUF10_INDEX]][j] = 1;
                } else {
                    web_data[BUF10_INDEX][i / avg_length[BUF10_INDEX]][j] = 0;
                }
            }

            // reset values
            memset(avg_data[BUF10_INDEX], 0, sizeof(int64_t) * num_channels);
            memset(bin_avg_data[BUF10_INDEX], 0,
                   sizeof(uint32_t) * num_bin_channels);
            avg_valid[BUF10_INDEX][0] = 1;
            avg_valid[BUF10_INDEX][1] = 1;
        }

        // buffer 100
        if ((i + 1) % avg_length[BUF100_INDEX] == 0) {

            // average
            for (int j = 0; j < num_channels; j++) {
                avg_data[BUF100_INDEX][j] /=
                    (avg_length[BUF100_INDEX] / avg_length[BUF10_INDEX]);
            }

            // merge_currents (for web)
            web_merge_currents(avg_valid[BUF100_INDEX],
                               &web_data[BUF100_INDEX]
                                        [i / avg_length[BUF100_INDEX]]
                                        [num_bin_channels],
                               avg_data[BUF100_INDEX], conf);

            // store bin channels for web
            for (int j = 0; j < num_bin_channels; j++) {

                if (bin_avg_data[BUF100_INDEX][j] >=
                    (avg_length[BUF100_INDEX] / 2)) {
                    web_data[BUF100_INDEX][i / avg_length[BUF100_INDEX]][j] = 1;
                } else {
                    web_data[BUF100_INDEX][i / avg_length[BUF100_INDEX]][j] = 0;
                }
            }
        }
    }

    // WRITE WEB DATA //

    // get shared memory access
    if (wait_sem(sem_id, DATA_SEM, SEM_WRITE_TIME_OUT) == TIME_OUT) {
        // disable webserver and continue running
        conf->enable_web_server = 0;
        status.state = RL_RUNNING;
        rl_log(WARNING, "semaphore failure. Webserver disabled");
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
        set_sem(sem_id, DATA_SEM, 1);
    }
}
