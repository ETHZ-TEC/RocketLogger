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
#include <stdlib.h>
#include <string.h>

#include <zmq.h>

#include "log.h"
#include "rl.h"
#include "rl_file.h"
#include "sensor/sensor.h"
#include "util.h"

#include "rl_socket.h"

#define RL_SOCKET_METADATA_SIZE 1000

/// data socket metadata in JSON format
char metadata_json[RL_SOCKET_METADATA_SIZE];
/// the ZeroMQ context for data publishing
void *zmq_data_context = NULL;
/// the ZeroMQ data socket
void *zmq_data_socket = NULL;

int rl_socket_init(void) {
    // open and bind zmq data socket
    zmq_data_context = zmq_ctx_new();
    zmq_data_socket = zmq_socket(zmq_data_context, ZMQ_PUB);
    int zmq_res = zmq_bind(zmq_data_socket, RL_ZMQ_DATA_SOCKET);
    if (zmq_res < 0) {
        rl_log(RL_LOG_ERROR,
               "failed binding zeromq data socket; %d message: %s", errno,
               strerror(errno));
        return ERROR;
    }

    return SUCCESS;
}

int rl_socket_deinit(void) {
    // close and destroy zmq data socket
    zmq_close(zmq_data_socket);
    zmq_ctx_destroy(zmq_data_context);

    zmq_data_socket = NULL;
    zmq_data_context = NULL;

    return SUCCESS;
}

int rl_socket_metadata(rl_config_t const *const config) {
    // data rate and channel info init
    snprintf(metadata_json, RL_SOCKET_METADATA_SIZE,
             "{\"data_rate\":%d,\"channels\":[", config->sample_rate);

    // analog channel metadata
    for (int ch = 0; ch < RL_CHANNEL_COUNT; ch++) {
        if (!config->channel_enable[ch]) {
            continue;
        }

        snprintfcat(metadata_json, RL_SOCKET_METADATA_SIZE, "{\"name\":\"%s\",",
                    RL_CHANNEL_NAMES[ch]);
        if (is_current(ch)) {
            if (is_low_current(ch)) {
                snprintfcat(metadata_json, RL_SOCKET_METADATA_SIZE,
                            "\"unit\":\"A\",\"scale\":1e-11},");
            } else {
                snprintfcat(metadata_json, RL_SOCKET_METADATA_SIZE,
                            "\"unit\":\"A\",\"scale\":1e-9},");
            }
        } else if (is_voltage(ch)) {
            snprintfcat(metadata_json, RL_SOCKET_METADATA_SIZE,
                        "\"unit\":\"V\",\"scale\":1e-8},");
        } else {
            snprintfcat(metadata_json, RL_SOCKET_METADATA_SIZE,
                        "\"unit\":null},");
        }
    }

    // ambient channel metadata
    if (config->ambient_enable) {
        for (int ch = 0; ch < SENSOR_REGISTRY_SIZE; ch++) {
            if (rl_status.sensor_available[ch]) {
                snprintfcat(metadata_json, RL_SOCKET_METADATA_SIZE,
                            "{\"name\":\"%s\",\"unit\":\"%s\",\"scale\":1e%d},",
                            SENSOR_REGISTRY[ch].name,
                            rl_unit_to_string(SENSOR_REGISTRY[ch].unit),
                            SENSOR_REGISTRY[ch].scale);
            }
        }
    }

    // digital channel metadata
    if (config->digital_enable) {
        for (int i = 0; i < RL_CHANNEL_DIGITAL_COUNT; i++) {
            snprintfcat(metadata_json, RL_SOCKET_METADATA_SIZE,
                        "{\"name\":\"DI%d\",\"unit\":\"binary\",\"bit\":%d},",
                        i + 1, i);
        }
    }

    // current range valid metadata and JSON end
    if (config->channel_enable[RL_CONFIG_CHANNEL_I1L]) {
        snprintfcat(metadata_json, RL_SOCKET_METADATA_SIZE,
                    "{\"name\":\"I1L_valid\",\"unit\":\"binary\",\"bit\":6,"
                    "\"hidden\":true},");
    }
    if (config->channel_enable[RL_CONFIG_CHANNEL_I2L]) {
        snprintfcat(metadata_json, RL_SOCKET_METADATA_SIZE,
                    "{\"name\":\"I2L_valid\",\"unit\":\"binary\",\"bit\":7,"
                    "\"hidden\":true},");
    }

    // trim trailing comma of last array entry
    if (metadata_json[strlen(metadata_json) - 1] == ',') {
        metadata_json[strlen(metadata_json) - 1] = 0;
    }

    // JSON end
    snprintfcat(metadata_json, RL_SOCKET_METADATA_SIZE, "]}");

    return SUCCESS;
}

int rl_socket_handle_data(int32_t const *analog_buffer,
                          uint32_t const *digital_buffer,
                          int32_t const *ambient_buffer, size_t buffer_size,
                          size_t ambient_buffer_size,
                          rl_timestamp_t const *const timestamp_realtime,
                          rl_timestamp_t const *const timestamp_monotonic,
                          rl_config_t const *const config) {

    // publish metadata to socket
    int zmq_res = zmq_send(zmq_data_socket, metadata_json,
                           strlen(metadata_json), ZMQ_SNDMORE);
    if (zmq_res < 0) {
        rl_log(RL_LOG_ERROR, "failed publishing analog data; %d message: %s",
               errno, strerror(errno));
        return ERROR;
    }

    // publish raw timestamp values
    rl_timestamp_t timestamps[2] = {
        *timestamp_realtime,
        *timestamp_monotonic,
    };

    // publish timestamp data to socket
    zmq_res =
        zmq_send(zmq_data_socket, timestamps, sizeof(timestamps), ZMQ_SNDMORE);
    if (zmq_res < 0) {
        rl_log(RL_LOG_ERROR, "failed publishing timestamps; %d message: %s",
               errno, strerror(errno));
        return ERROR;
    }

    // process analog channels
    int32_t *const data_buffer = malloc(buffer_size * sizeof(int32_t));
    for (int ch = 0; ch < RL_CHANNEL_COUNT; ch++) {
        if (!config->channel_enable[ch]) {
            continue;
        }

        // copy standalone channel buffer
        for (size_t i = 0; i < buffer_size; i++) {
            data_buffer[i] = *(analog_buffer + i * RL_CHANNEL_COUNT + ch);
        }

        // publish channel data to socket
        int zmq_res = zmq_send(zmq_data_socket, data_buffer,
                               buffer_size * sizeof(int32_t), ZMQ_SNDMORE);
        if (zmq_res < 0) {
            rl_log(RL_LOG_ERROR,
                   "failed publishing analog data; %d message: %s", errno,
                   strerror(errno));
            // free data send buffer and exit
            free(data_buffer);
            return ERROR;
        }
    }
    free(data_buffer);

    // publish ambient data to socket
    if (config->ambient_enable) {
        for (int ch = 0; ch < rl_status.sensor_count; ch++) {
            // publish channel data to socket
            int zmq_res = zmq_send(zmq_data_socket, &ambient_buffer[ch],
                                   sizeof(int32_t) * (ambient_buffer_size > 0),
                                   ZMQ_SNDMORE);
            if (zmq_res < 0) {
                rl_log(RL_LOG_ERROR,
                       "failed publishing sensor data; %d message: %s", errno,
                       strerror(errno));
                return ERROR;
            }
        }
    }

    // publish digital data (or empty if none available) to socket
    if (config->digital_enable ||
        config->channel_enable[RL_CONFIG_CHANNEL_I1L] ||
        config->channel_enable[RL_CONFIG_CHANNEL_I2L]) {
        zmq_res = zmq_send(zmq_data_socket, digital_buffer,
                           buffer_size * sizeof(uint32_t), 0);
    } else {
        zmq_res = zmq_send(zmq_data_socket, "", 0, 0);
    }
    if (zmq_res < 0) {
        rl_log(RL_LOG_ERROR, "failed publishing digital data; %d message: %s",
               errno, strerror(errno));
        return ERROR;
    }

    return SUCCESS;
}
