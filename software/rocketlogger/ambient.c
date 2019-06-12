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

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "rl.h"
#include "rl_file.h"
#include "sensor/sensor.h"
#include "util.h"

#include "ambient.h"

/**
 * @todo document
 */
void ambient_setup_channels(rl_file_header_t *const header);

void ambient_append_data(FILE *ambient_file,
                         rl_timestamp_t const *const timestamp_realtime,
                         rl_timestamp_t const *const timestamp_monotonic) {

    // store timestamp
    fwrite(timestamp_realtime, sizeof(rl_timestamp_t), 1, ambient_file);
    fwrite(timestamp_monotonic, sizeof(rl_timestamp_t), 1, ambient_file);

    // FETCH VALUES //
    int32_t sensor_data[SENSOR_REGISTRY_SIZE];

    int ch = 0;
    int mutli_channel_read = -1;
    for (int i = 0; i < SENSOR_REGISTRY_SIZE; i++) {
        // only read registered sensors
        if (rl_status.sensor_available[i]) {
            // read multi-channel sensor data only once
            if (SENSOR_REGISTRY[i].identifier != mutli_channel_read) {
                SENSOR_REGISTRY[i].read(SENSOR_REGISTRY[i].identifier);
                mutli_channel_read = SENSOR_REGISTRY[i].identifier;
            }
            sensor_data[ch] = SENSOR_REGISTRY[i].get_value(
                SENSOR_REGISTRY[i].identifier, SENSOR_REGISTRY[i].channel);
            ch++;
        }
    }

    // WRITE VALUES //
    fwrite(sensor_data, sizeof(int32_t), rl_status.sensor_count, ambient_file);
}

// FILE HEADER //

char *ambient_get_file_name(char const *const data_file_name) {
    static char ambient_file_name[RL_PATH_LENGTH_MAX];

    // determine new file name
    strcpy(ambient_file_name, data_file_name);

    // search for last . character
    char target = '.';
    char *file_ending = ambient_file_name;
    while (strchr(file_ending, target) != NULL) {
        file_ending = strchr(file_ending, target);
        file_ending++; // Increment file_ending, otherwise we'll find target at
                       // the same location
    }
    file_ending--;

    // add file ending
    char ambient_file_ending[RL_PATH_LENGTH_MAX] = AMBIENT_FILE_NAME_SUFFIX;
    strcat(ambient_file_ending, file_ending);
    strcpy(file_ending, ambient_file_ending);

    return ambient_file_name;
}

void ambient_setup_lead_in(struct rl_file_lead_in *const lead_in) {

    // number channels
    uint16_t channel_count = rl_status.sensor_count;

    // number binary channels
    uint16_t channel_bin_count = 0;

    // comment length
    uint32_t comment_length = RL_FILE_COMMENT_ALIGNMENT_BYTES;

    // timestamps
    rl_timestamp_t time_real;
    rl_timestamp_t time_monotonic;
    create_time_stamp(&time_real, &time_monotonic);

    // lead_in setup
    lead_in->magic = RL_FILE_MAGIC;
    lead_in->file_version = RL_FILE_VERSION;
    lead_in->header_length =
        sizeof(struct rl_file_lead_in) + comment_length +
        (channel_count + channel_bin_count) * sizeof(rl_file_channel_t);
    lead_in->data_block_size = AMBIENT_DATA_BLOCK_SIZE;
    lead_in->data_block_count = 0; // needs to be updated
    lead_in->sample_count = 0;     // needs to be updated
    lead_in->sample_rate = AMBIENT_SAMPLING_RATE;
    get_mac_addr(lead_in->mac_address);
    lead_in->start_time = time_real;
    lead_in->comment_length = comment_length;
    lead_in->channel_bin_count = channel_bin_count;
    lead_in->channel_count = channel_count;
}

void ambient_setup_header(struct rl_file_header *const header,
                          rl_config_t const *const config) {
    if (config->file_comment == NULL) {
        header->comment = "";
    } else {
        header->comment = config->file_comment;
    }

    ambient_setup_channels(header);
}

void ambient_setup_channels(struct rl_file_header *const header) {
    int total_channel_count =
        header->lead_in.channel_bin_count + header->lead_in.channel_count;

    // reset channels
    memset(header->channel, 0, total_channel_count * sizeof(rl_file_channel_t));

    // write channels
    int ch = 0;
    for (int i = 0; i < SENSOR_REGISTRY_SIZE; i++) {
        if (rl_status.sensor_available[i]) {
            header->channel[ch].unit = SENSOR_REGISTRY[i].unit;
            header->channel[ch].channel_scale = SENSOR_REGISTRY[i].scale;
            header->channel[ch].valid_data_channel = NO_VALID_DATA;
            header->channel[ch].data_size = 4;
            strcpy(header->channel[ch].name, SENSOR_REGISTRY[i].name);
            ch++;
        }
    }
}
