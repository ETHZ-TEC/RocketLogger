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

#include "rl_file.h"
#include "sensor/sensor.h"
#include "types.h"
#include "util.h"

#include "ambient.h"

/**
 * Handle a ambient data buffer, dependent on current configuration
 * @param ambient_file File pointer to ambient file
 * @param timestamp_realtime {@link time_stamp} with realtime clock value
 * @param timestamp_monotonic {@link time_stamp} with monotonic clock value
 * @param config Current {@link rl_config_t} configuration.
 */
void ambient_store_data(FILE *ambient_file,
                        rl_timestamp_t const *const timestamp_realtime,
                        rl_timestamp_t const *const timestamp_monotonic,
                        rl_config_t const *const config) {

    // store timestamp
    fwrite(timestamp_realtime, sizeof(rl_timestamp_t), 1, ambient_file);
    fwrite(timestamp_monotonic, sizeof(rl_timestamp_t), 1, ambient_file);

    // FETCH VALUES //
    int32_t sensor_data[config->ambient.sensor_count];

    int ch = 0;
    int mutli_channel_read = -1;
    for (int i = 0; i < SENSOR_REGISTRY_SIZE; i++) {
        // only read registered sensors
        if (config->ambient.available_sensors[i] > 0) {
            // read multi-channel sensor data only once
            if (sensor_registry[i].identifier != mutli_channel_read) {
                sensor_registry[i].read(sensor_registry[i].identifier);
                mutli_channel_read = sensor_registry[i].identifier;
            }
            sensor_data[ch] = sensor_registry[i].get_value(
                sensor_registry[i].identifier, sensor_registry[i].channel);
            ch++;
        }
    }

    // WRITE VALUES //
    fwrite(sensor_data, sizeof(int32_t), config->ambient.sensor_count,
           ambient_file);
}

// FILE HEADER //

void ambient_set_file_name(rl_config_t *const config) {

    // determine new file name
    char ambient_file_name[MAX_PATH_LENGTH];
    strcpy(ambient_file_name, config->file_name);

    // search for last .
    char target = '.';
    char *file_ending = ambient_file_name;
    while (strchr(file_ending, target) != NULL) {
        file_ending = strchr(file_ending, target);
        file_ending++; // Increment file_ending, otherwise we'll find target at
                       // the same location
    }
    file_ending--;

    // add file ending
    char ambient_file_ending[9] = "-ambient";
    strcat(ambient_file_ending, file_ending);
    strcpy(file_ending, ambient_file_ending);
    strcpy(config->ambient.file_name, ambient_file_name);
}

void ambient_setup_lead_in(struct rl_file_lead_in *const lead_in,
                           rl_config_t const *const config) {

    // number channels
    uint16_t channel_count = config->ambient.sensor_count;

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

void ambient_setup_channels(struct rl_file_header *const file_header,
                            rl_config_t const *const config) {

    int total_channel_count = file_header->lead_in.channel_bin_count +
                              file_header->lead_in.channel_count;

    // reset channels
    memset(file_header->channel, 0,
           total_channel_count * sizeof(rl_file_channel_t));

    // write channels
    int ch = 0;
    for (int i = 0; i < SENSOR_REGISTRY_SIZE; i++) {
        if (config->ambient.available_sensors[i] > 0) {

            file_header->channel[ch].unit = sensor_registry[i].unit;
            file_header->channel[ch].channel_scale = sensor_registry[i].scale;
            file_header->channel[ch].valid_data_channel = NO_VALID_DATA;
            file_header->channel[ch].data_size = 4;
            strcpy(file_header->channel[ch].name, sensor_registry[i].name);
            ch++;
        }
    }
}

void ambient_setup_header(struct rl_file_header *const file_header,
                          rl_config_t const *const config,
                          char const *const comment) {

    // comment
    if (comment == NULL) {
        file_header->comment = "";
    } else {
        file_header->comment = comment;
    }

    // channels
    ambient_setup_channels(file_header, config);
}
