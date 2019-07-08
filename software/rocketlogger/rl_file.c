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
#include <stdlib.h>
#include <string.h>

#include <time.h>

#include "ads131e0x.h"
#include "calibration.h"
#include "log.h"
#include "pru.h"
#include "rl.h"
#include "rl_file.h"
#include "sensor/sensor.h"
#include "util.h"

#include "rl_file.h"

/**
 * Set up channel information of the data file header.
 *
 * @param header The file header structure to configure
 * @param config Current measurement configuration
 */
void rl_file_setup_data_channels(rl_file_header_t *const header,
                                 rl_config_t const *const config);

/**
 * Set up channel information of the ambient file header.
 *
 * @param header The ambient file header structure to configure
 * @param config Current measurement configuration
 */
void rl_file_setup_ambient_channels(rl_file_header_t *const header);

// counter to control ambient sample rate
static uint32_t ambient_rate_counter = 0;

/// Global variable to determine i1l valid channel index
int i1l_valid_channel = 0;
/// Global variable to determine i2l valid channel index
int i2l_valid_channel = 0;

char *rl_file_get_ambient_file_name(char const *const data_file_name) {
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
    char ambient_file_ending[RL_PATH_LENGTH_MAX] = RL_FILE_AMBIENT_SUFFIX;
    strcat(ambient_file_ending, file_ending);
    strcpy(file_ending, ambient_file_ending);

    return ambient_file_name;
}

void rl_file_setup_data_lead_in(rl_file_lead_in_t *const lead_in,
                                rl_config_t const *const config) {

    // number channels
    uint16_t channel_count = count_channels(config->channel_enable);
    // number binary channels
    uint16_t channel_bin_count = 0;
    if (config->digital_enable) {
        channel_bin_count = RL_CHANNEL_DIGITAL_COUNT;
    }
    if (config->channel_enable[RL_CONFIG_CHANNEL_I1L]) {
        i1l_valid_channel = channel_bin_count++;
    }
    if (config->channel_enable[RL_CONFIG_CHANNEL_I2L]) {
        i2l_valid_channel = channel_bin_count++;
    }

    // comment length
    uint32_t comment_length = RL_FILE_COMMENT_ALIGNMENT_BYTES;

    // timestamps
    rl_timestamp_t timestamp_realtime;
    rl_timestamp_t timestamp_monotonic;
    create_time_stamp(&timestamp_realtime, &timestamp_monotonic);

    // lead_in setup
    lead_in->file_magic = RL_FILE_MAGIC;
    lead_in->file_version = RL_FILE_VERSION;
    lead_in->header_length =
        sizeof(rl_file_lead_in_t) + comment_length +
        (channel_count + channel_bin_count) * sizeof(rl_file_channel_t);
    lead_in->data_block_size = config->sample_rate / config->update_rate;
    lead_in->data_block_count = 0; // needs to be updated
    lead_in->sample_count = 0;     // needs to be updated
    lead_in->sample_rate = config->sample_rate;
    get_mac_addr(lead_in->mac_address);
    lead_in->start_time = timestamp_realtime;
    lead_in->comment_length = comment_length;
    lead_in->channel_bin_count = channel_bin_count;
    lead_in->channel_count = channel_count;
}

void rl_file_setup_ambient_lead_in(rl_file_lead_in_t *const lead_in,
                                   rl_config_t const *const config) {

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
    lead_in->file_magic = RL_FILE_MAGIC;
    lead_in->file_version = RL_FILE_VERSION;
    lead_in->header_length =
        sizeof(rl_file_lead_in_t) + comment_length +
        (channel_count + channel_bin_count) * sizeof(rl_file_channel_t);
    lead_in->data_block_size = RL_FILE_AMBIENT_DATA_BLOCK_SIZE;
    lead_in->data_block_count = 0; // needs to be updated
    lead_in->sample_count = 0;     // needs to be updated
    lead_in->sample_rate = (config->update_rate < RL_FILE_AMBIENT_SAMPLING_RATE
                                ? config->update_rate
                                : RL_FILE_AMBIENT_SAMPLING_RATE);
    get_mac_addr(lead_in->mac_address);
    lead_in->start_time = time_real;
    lead_in->comment_length = comment_length;
    lead_in->channel_bin_count = channel_bin_count;
    lead_in->channel_count = channel_count;
}

void rl_file_setup_data_header(rl_file_header_t *const header,
                               rl_config_t const *const config) {
    if (config->file_comment == NULL) {
        header->comment = "";
    } else {
        header->comment = config->file_comment;
    }

    rl_file_setup_data_channels(header, config);
}

void rl_file_setup_ambient_header(rl_file_header_t *const header,
                                  rl_config_t const *const config) {
    if (config->file_comment == NULL) {
        header->comment = "";
    } else {
        header->comment = config->file_comment;
    }

    rl_file_setup_ambient_channels(header);
}

void rl_file_store_header_bin(FILE *file_handle,
                              rl_file_header_t *const file_header) {

    int total_channel_count = file_header->lead_in.channel_bin_count +
                              file_header->lead_in.channel_count;

    // check if alignment bytes are needed after header comment
    int comment_length = strlen(file_header->comment) + 1;
    int comment_align_bytes = 0;
    if (comment_length % RL_FILE_COMMENT_ALIGNMENT_BYTES > 0) {
        comment_align_bytes =
            RL_FILE_COMMENT_ALIGNMENT_BYTES -
            (comment_length % RL_FILE_COMMENT_ALIGNMENT_BYTES);
    }

    file_header->lead_in.comment_length = comment_length + comment_align_bytes;

    file_header->lead_in.header_length =
        sizeof(rl_file_lead_in_t) + file_header->lead_in.comment_length +
        total_channel_count * sizeof(rl_file_channel_t);

    // write lead-in
    fwrite(&(file_header->lead_in), sizeof(rl_file_lead_in_t), 1, file_handle);

    // write comment, add zero bytes for proper header alignment if necessary
    fwrite(file_header->comment, comment_length, 1, file_handle);
    if (comment_align_bytes > 0) {
        uint8_t zero_bytes[RL_FILE_COMMENT_ALIGNMENT_BYTES] = {0};
        fwrite(zero_bytes, comment_align_bytes, 1, file_handle);
    }

    // write channel information
    fwrite(file_header->channel, sizeof(rl_file_channel_t), total_channel_count,
           file_handle);
    fflush(file_handle);
}

void rl_file_store_header_csv(FILE *file_handle,
                              rl_file_header_t const *const file_header) {
    // lead-in
    fprintf(file_handle, "RocketLogger CSV File\n");
    fprintf(file_handle, "File Version,%u\n",
            (uint32_t)file_header->lead_in.file_version);
    fprintf(file_handle, "Block Size,%u\n",
            (uint32_t)file_header->lead_in.data_block_size);
    fprintf(file_handle, "Block Count,%-20u\n",
            (uint32_t)file_header->lead_in.data_block_count);
    fprintf(file_handle, "Sample Count,%-20llu\n",
            (uint64_t)file_header->lead_in.sample_count);
    fprintf(file_handle, "Sample Rate,%u\n",
            (uint32_t)file_header->lead_in.sample_rate);
    fprintf(file_handle, "MAC Address,%02x",
            (uint32_t)file_header->lead_in.mac_address[0]);

    for (int i = 1; i < MAC_ADDRESS_LENGTH; i++) {
        fprintf(file_handle, ":%02x",
                (uint32_t)file_header->lead_in.mac_address[i]);
    }
    fprintf(file_handle, "\n");

    time_t time = (time_t)file_header->lead_in.start_time.sec;
    fprintf(file_handle, "Start Time,%s", ctime(&time));
    fprintf(file_handle, "Comment,%s\n", file_header->comment);
    fprintf(file_handle, "\n");

    // channels
    for (int i = 0; i < (file_header->lead_in.channel_count +
                         file_header->lead_in.channel_bin_count);
         i++) {
        fprintf(file_handle, ",%s", file_header->channel[i].name);
        switch (file_header->channel[i].channel_scale) {
        case RL_SCALE_MILLI:
            fprintf(file_handle, " [m");
            break;
        case RL_SCALE_MICRO:
            fprintf(file_handle, " [u");
            break;
        case RL_SCALE_TEN_NANO:
            fprintf(file_handle, " [10n");
            break;
        case RL_SCALE_NANO:
            fprintf(file_handle, " [n");
            break;
        case RL_SCALE_TEN_PICO:
            fprintf(file_handle, " [10p");
            break;
        default:
            break;
        }
        switch (file_header->channel[i].unit) {
        case RL_UNIT_VOLT:
            fprintf(file_handle, "V]");
            break;
        case RL_UNIT_AMPERE:
            fprintf(file_handle, "A]");
            break;
        default:
            break;
        }
    }
    fprintf(file_handle, "\n");
    fflush(file_handle);
}

void rl_file_update_header_bin(FILE *file_handle,
                               rl_file_header_t const *const file_header) {

    // seek to beginning and rewrite lead_in
    rewind(file_handle);
    fwrite(&(file_header->lead_in), sizeof(rl_file_lead_in_t), 1, file_handle);
    fflush(file_handle);
    fseek(file_handle, 0, SEEK_END);
}

void rl_file_update_header_csv(FILE *file_handle,
                               rl_file_header_t const *const file_header) {
    rewind(file_handle);
    fprintf(file_handle, "RocketLogger CSV File\n");
    fprintf(file_handle, "File Version,%u\n",
            (uint32_t)file_header->lead_in.file_version);
    fprintf(file_handle, "Block Size,%u\n",
            (uint32_t)file_header->lead_in.data_block_size);
    fprintf(file_handle, "Block Count,%-20u\n",
            (uint32_t)file_header->lead_in.data_block_count);
    fprintf(file_handle, "Sample Count,%-20llu\n",
            (uint64_t)file_header->lead_in.sample_count);
    fflush(file_handle);
    fseek(file_handle, 0, SEEK_END);
}

void rl_file_add_data_block(FILE *data_file, pru_buffer_t const *const buffer,
                            uint32_t buffer_size,
                            rl_timestamp_t const *const timestamp_realtime,
                            rl_timestamp_t const *const timestamp_monotonic,
                            rl_config_t const *const config) {
    // skip if not storing to file, or invalid file structure
    if (!config->file_enable) {
        return;
    }
    if (data_file == NULL) {
        rl_log(RL_LOG_ERROR, "invalid data file provided, skip append data");
        return;
    }

    // write timestamp to file
    if (config->file_format == RL_FILE_FORMAT_RLD) {
        fwrite(timestamp_realtime, sizeof(rl_timestamp_t), 1, data_file);
        fwrite(timestamp_monotonic, sizeof(rl_timestamp_t), 1, data_file);
    } else if (config->file_format == RL_FILE_FORMAT_CSV) {
        fprintf(data_file, "%lli.%09lli", timestamp_realtime->sec,
                timestamp_realtime->nsec);
    }

    // count channels
    int channel_count = count_channels(config->channel_enable);

    // aggregation
    int32_t aggregate_count = ADS131E0X_RATE_MIN / config->sample_rate;
    int32_t aggregate_channel_data[RL_CHANNEL_COUNT] = {0};
    uint32_t aggregate_bin_data = 0xffffffff;

    // HANDLE BUFFER //
    for (uint32_t i = 0; i < buffer_size; i++) {

        // channel data variables
        int32_t channel_data[RL_CHANNEL_COUNT] = {0};
        uint32_t bin_data = 0x00000000;

        pru_data_t const pru_data = buffer->data[i];

        // read digital channels
        uint8_t bin_adc1 = (uint8_t)(pru_data.channel_digital & 0xff);
        uint8_t bin_adc2 = (uint8_t)((pru_data.channel_digital >> 8) & 0xff);
        // uint8_t bin_adc1 = (*((int8_t *)(buffer)));
        // uint8_t bin_adc2 = (*((int8_t *)(buffer + 1)));
        // buffer += PRU_DIGITAL_SIZE;

        // read and scale values (if channel selected)
        int channel_index = 0;
        for (int j = 0; j < RL_CHANNEL_COUNT; j++) {
            if (config->channel_enable[j]) {
                channel_data[channel_index] = (int32_t)(
                    (pru_data.channel_analog[j] + rl_calibration.offsets[j]) *
                    rl_calibration.scales[j]);
                channel_index++;
            }
        }

        // BINARY CHANNELS //

        // mask and combine digital inputs, if requested
        int bin_channel_pos = 0;
        if (config->digital_enable) {
            bin_data = ((bin_adc1 & PRU_BINARY_MASK) >> 1) |
                       ((bin_adc2 & PRU_BINARY_MASK) << 2);
            bin_channel_pos = RL_CHANNEL_DIGITAL_COUNT;
        }

        // mask and combine valid info
        uint8_t valid1 = (~bin_adc1) & PRU_VALID_MASK;
        uint8_t valid2 = (~bin_adc2) & PRU_VALID_MASK;

        if (config->channel_enable[RL_CONFIG_CHANNEL_I1L]) {
            bin_data = bin_data | (valid1 << bin_channel_pos);
            bin_channel_pos++;
        }
        if (config->channel_enable[RL_CONFIG_CHANNEL_I2L]) {
            bin_data = bin_data | (valid2 << bin_channel_pos);
            bin_channel_pos++;
        }

        // handle data aggregation for low sampling rates
        if (config->sample_rate < ADS131E0X_RATE_MIN) {
            switch (config->aggregation_mode) {
            case RL_AGGREGATION_MODE_DOWNSAMPLE:
                // drop intermediate samples (skip writing to file)
                if (i % aggregate_count > 0) {
                    // aggregate this sample only, skip file writing for now
                    continue;
                }
                break;

            case RL_AGGREGATION_MODE_AVERAGE:
                // accumulate intermediate samples only (skip writing)
                if ((i + 1) % aggregate_count > 0) {
                    for (int i = 0; i < channel_count; i++) {
                        aggregate_channel_data[i] += channel_data[i];
                    }
                    aggregate_bin_data = aggregate_bin_data & bin_data;

                    // aggregate this sample only, skip file writing for now
                    continue;
                }

                // calculate average for writing to file
                for (int i = 0; i < channel_count; i++) {
                    channel_data[i] =
                        aggregate_channel_data[i] / aggregate_count;
                    aggregate_channel_data[i] = 0;
                }

                bin_data = aggregate_bin_data;
                aggregate_bin_data = 0xffffffff;
                break;
            }
        }

        // write data to file

        // write binary channels if enabled
        if (bin_channel_pos > 0) {
            if (config->file_format == RL_FILE_FORMAT_RLD) {
                fwrite(&bin_data, sizeof(uint32_t), 1, data_file);
            } else if (config->file_format == RL_FILE_FORMAT_CSV) {
                uint32_t bin_mask = 0x01;
                for (int j = 0; j < bin_channel_pos; j++) {
                    fprintf(data_file, (RL_FILE_CSV_DELIMITER "%i"),
                            (bin_data & bin_mask) > 0);
                    bin_mask = bin_mask << 1;
                }
            }
        }

        // write analog channels
        if (config->file_format == RL_FILE_FORMAT_RLD) {
            fwrite(channel_data, sizeof(int32_t), channel_count, data_file);
        } else if (config->file_format == RL_FILE_FORMAT_CSV) {
            for (int j = 0; j < channel_count; j++) {
                fprintf(data_file, (RL_FILE_CSV_DELIMITER "%d"),
                        channel_data[j]);
            }
            fprintf(data_file, "\n");
        }
    }

    // flush processed data if data is stored
    if (config->file_enable && data_file != NULL) {
        fflush(data_file);
    }
}

void rl_file_add_ambient_block(FILE *ambient_file,
                               pru_buffer_t const *const buffer,
                               uint32_t buffer_size,
                               rl_timestamp_t const *const timestamp_realtime,
                               rl_timestamp_t const *const timestamp_monotonic,
                               rl_config_t const *const config) {
    // suppress unused parameter warning
    (void)buffer;
    (void)buffer_size;

    // rate limit sampling of the ambient sensors
    if (ambient_rate_counter < config->update_rate) {
        ambient_rate_counter += RL_FILE_AMBIENT_SAMPLING_RATE;
        return;
    }

    // reset rate control counter
    ambient_rate_counter = 0;

    // store timestamps
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

void rl_file_setup_data_channels(rl_file_header_t *const file_header,
                                 rl_config_t const *const config) {
    int total_channel_count = file_header->lead_in.channel_bin_count +
                              file_header->lead_in.channel_count;

    // reset channels
    memset(file_header->channel, 0,
           total_channel_count * sizeof(rl_file_channel_t));

    // digital channels
    int ch = 0;
    if (config->digital_enable) {
        for (int i = 0; i < RL_CHANNEL_DIGITAL_COUNT; i++) {
            file_header->channel[ch].unit = RL_UNIT_BINARY;
            file_header->channel[ch].channel_scale = RL_SCALE_NONE;
            file_header->channel[ch].data_size = 0;
            file_header->channel[ch].valid_data_channel =
                RL_FILE_CHANNEL_NO_LINK;
            strcpy(file_header->channel[ch].name, RL_CHANNEL_DIGITAL_NAMES[i]);
            ch++;
        }
    }

    // range valid channels
    if (config->channel_enable[RL_CONFIG_CHANNEL_I1L]) {
        file_header->channel[ch].unit = RL_UNIT_RANGE_VALID;
        file_header->channel[ch].channel_scale = RL_SCALE_NONE;
        file_header->channel[ch].data_size = 0;
        file_header->channel[ch].valid_data_channel = RL_FILE_CHANNEL_NO_LINK;
        strcpy(file_header->channel[ch].name, RL_CHANNEL_VALID_NAMES[0]);
        ch++;
    }
    if (config->channel_enable[RL_CONFIG_CHANNEL_I2L]) {
        file_header->channel[ch].unit = RL_UNIT_RANGE_VALID;
        file_header->channel[ch].channel_scale = RL_SCALE_NONE;
        file_header->channel[ch].data_size = 0;
        file_header->channel[ch].valid_data_channel = RL_FILE_CHANNEL_NO_LINK;
        strcpy(file_header->channel[ch].name, RL_CHANNEL_VALID_NAMES[1]);
        ch++;
    }

    // analog channels
    for (int i = 0; i < RL_CHANNEL_COUNT; i++) {
        if (config->channel_enable[i]) {
            if (is_current(i)) {
                if (is_low_current(i)) {
                    file_header->channel[ch].channel_scale = RL_SCALE_TEN_PICO;
                    if (i == RL_CONFIG_CHANNEL_I1L) {
                        file_header->channel[ch].valid_data_channel =
                            i1l_valid_channel;
                    } else {
                        file_header->channel[ch].valid_data_channel =
                            i2l_valid_channel;
                    }
                } else {
                    file_header->channel[ch].channel_scale = RL_SCALE_NANO;
                    file_header->channel[ch].valid_data_channel =
                        RL_FILE_CHANNEL_NO_LINK;
                }
                file_header->channel[ch].unit = RL_UNIT_AMPERE;
            } else {
                file_header->channel[ch].unit = RL_UNIT_VOLT;
                file_header->channel[ch].channel_scale = RL_SCALE_TEN_NANO;
                file_header->channel[ch].valid_data_channel =
                    RL_FILE_CHANNEL_NO_LINK;
            }
            file_header->channel[ch].data_size = 4;
            strncpy(file_header->channel[ch].name, RL_CHANNEL_NAMES[i],
                    RL_FILE_CHANNEL_NAME_LENGTH - 1);
            ch++;
        }
    }
}

void rl_file_setup_ambient_channels(rl_file_header_t *const header) {
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
            header->channel[ch].valid_data_channel = RL_FILE_CHANNEL_NO_LINK;
            header->channel[ch].data_size = 4;
            strcpy(header->channel[ch].name, SENSOR_REGISTRY[i].name);
            ch++;
        }
    }
}
