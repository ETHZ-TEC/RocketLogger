/*
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <linux/limits.h>
#include <time.h>

#include "log.h"
#include "pru.h"
#include "rl.h"
#include "rl_file.h"
#include "sensor/sensor.h"
#include "util.h"

#include "rl_file.h"

char *RL_UNIT_NAMES[] = {
    "",   //!< 0 -> Unitless
    "V",  //!< 1 -> Voltage (electric)
    "A",  //!< 2 -> Current (electric)
    "",   //!< 3 -> Binary signal
    "",   //!< 4 -> Range valid information
    "lx", //!< 5 -> Lux (illuminance)
    "C",  //!< 6 -> Degree celsius (temperature)
    "",   //!< 7 -> Integer channel (numeric)
    "%",  //!< 8 -> Percent (numeric, humidity)
    "P",  //!< 9 -> Pascal (pressure)
    "s",  //!< 10 -> Second (time delta)
};

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

/// Global variable to determine i1l valid channel index
int i1l_valid_channel = 0;
/// Global variable to determine i2l valid channel index
int i2l_valid_channel = 0;

char *rl_unit_to_string(rl_unit_t const unit) {
    if ((unsigned int)unit <
        (sizeof(RL_UNIT_NAMES) / sizeof(RL_UNIT_NAMES[0]))) {
        return RL_UNIT_NAMES[unit];
    } else {
        return NULL;
    }
}

char *rl_file_get_ambient_file_name(char const *const data_file_name) {
    static char ambient_file_name[PATH_MAX];

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
    char ambient_file_ending[PATH_MAX] = RL_FILE_AMBIENT_SUFFIX;
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
    lead_in->sample_rate =
        (config->update_rate < RL_SENSOR_SAMPLE_RATE ? config->update_rate
                                                     : RL_SENSOR_SAMPLE_RATE);
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
        case RL_UNIT_SECOND:
            fprintf(file_handle, "s]");
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

int rl_file_add_data_block(FILE *data_file, int32_t const *analog_buffer,
                           uint32_t const *digital_buffer, size_t buffer_size,
                           rl_timestamp_t const *const timestamp_realtime,
                           rl_timestamp_t const *const timestamp_monotonic,
                           rl_config_t const *const config) {
    // aggregation buffer and configuration
    int32_t aggregate_analog[RL_CHANNEL_COUNT] = {0};
    uint32_t aggregate_digital = ~(0);
    ;
    size_t aggregate_count = RL_SAMPLE_RATE_MIN / config->sample_rate;

    // skip if not storing to file, or invalid file structure
    if (!config->file_enable) {
        return 0;
    }
    if (data_file == NULL) {
        rl_log(RL_LOG_ERROR, "invalid data file provided, skip append data");
        return ERROR;
    }
    if (aggregate_count > buffer_size) {
        rl_log(RL_LOG_ERROR, "cannot aggregate more data than the buffer size, "
                             "skip append data");
        return ERROR;
    }

    // write buffer timestamp to file
    if (config->file_format == RL_FILE_FORMAT_RLD) {
        fwrite(timestamp_realtime, sizeof(rl_timestamp_t), 1, data_file);
        fwrite(timestamp_monotonic, sizeof(rl_timestamp_t), 1, data_file);
    } else if (config->file_format == RL_FILE_FORMAT_CSV) {
        fprintf(data_file, "%lli.%09lli", timestamp_realtime->sec,
                timestamp_realtime->nsec);
    }

    // process data buffers
    for (size_t i = 0; i < buffer_size; i++) {
        // point to sample buffer to store by default (updated for aggregates)
        int32_t const *analog_data = analog_buffer + i * RL_CHANNEL_COUNT;
        uint32_t const *digital_data = digital_buffer + i;

        // handle aggregation when applicable
        if (aggregate_count > 1) {
            bool aggregate_store = false;

            // reset aggregate buffer on window start
            if (i % aggregate_count == 0) {
                memset(aggregate_analog, 0, sizeof(aggregate_analog));
                aggregate_digital = ~(0);
            }

            switch (config->aggregation_mode) {
            case RL_AGGREGATION_MODE_DOWNSAMPLE:
                // store first sample of aggregate window, skip storing others
                if (i % aggregate_count == 0) {
                    // store data as in non-aggregate mode
                    aggregate_store = true;
                }
                break;

            case RL_AGGREGATION_MODE_AVERAGE:
                // accumulate data of the aggregate window, store at the end
                for (int j = 0; j < RL_CHANNEL_COUNT; j++) {
                    aggregate_analog[j] += *(analog_data + j);
                }
                aggregate_digital = aggregate_digital & *digital_data;

                // on last sample of the window: average analog data and store
                if ((i + 1) % aggregate_count == 0) {
                    for (int j = 0; j < RL_CHANNEL_COUNT; j++) {
                        aggregate_analog[j] =
                            aggregate_analog[j] / aggregate_count;
                    }

                    // store aggregated data
                    analog_data = (int32_t const *)aggregate_analog;
                    digital_data = (uint32_t const *)aggregate_digital;
                    aggregate_store = true;
                }
                break;

            default:
                rl_log(RL_LOG_WARNING,
                       "unknown data aggregation mode, storing all samples.");
                aggregate_store = true;
            }

            // skip storing data if no aggregates available
            if (!aggregate_store) {
                continue;
            }
        }

        // write digital channels
        if (config->file_format == RL_FILE_FORMAT_RLD) {
            size_t index = 0;
            uint32_t data = 0x00;

            // build binary bit field to store
            if (config->digital_enable) {
                data |= ((*digital_data & PRU_DIGITAL_INPUT_MASK) << index);
                index += RL_CHANNEL_DIGITAL_COUNT;
            }
            if (config->channel_enable[RL_CONFIG_CHANNEL_I1L]) {
                if (*digital_data & PRU_DIGITAL_I1L_VALID_MASK) {
                    data |= (1 << index);
                }
                index++;
            }
            if (config->channel_enable[RL_CONFIG_CHANNEL_I2L]) {
                if (*digital_data & PRU_DIGITAL_I2L_VALID_MASK) {
                    data |= (1 << index);
                }
                index++;
            }

            // write digital data to file
            fwrite(&data, sizeof(data), 1, data_file);
        } else if (config->file_format == RL_FILE_FORMAT_CSV) {
            if (config->digital_enable) {
                uint32_t binary_mask = PRU_DIGITAL_INPUT1_MASK;
                for (int j = 0; j < RL_CHANNEL_DIGITAL_COUNT; j++) {
                    fprintf(data_file, (RL_FILE_CSV_DELIMITER "%i"),
                            (*digital_data & binary_mask) > 0);
                    binary_mask = binary_mask << 1;
                }
            }
            if (config->channel_enable[RL_CONFIG_CHANNEL_I1L]) {
                fprintf(data_file, (RL_FILE_CSV_DELIMITER "%i"),
                        (*digital_data & PRU_DIGITAL_I1L_VALID_MASK) > 0);
            }
            if (config->channel_enable[RL_CONFIG_CHANNEL_I2L]) {
                fprintf(data_file, (RL_FILE_CSV_DELIMITER "%i"),
                        (*digital_data & PRU_DIGITAL_I2L_VALID_MASK) > 0);
            }
        }

        // write analog channels
        for (int j = 0; j < RL_CHANNEL_COUNT; j++) {
            // skip disabled channels
            if (!config->channel_enable[j]) {
                continue;
            }
            // write analog data to file
            if (config->file_format == RL_FILE_FORMAT_RLD) {
                fwrite(&analog_data[j], sizeof(int32_t), 1, data_file);
            } else if (config->file_format == RL_FILE_FORMAT_CSV) {
                fprintf(data_file, (RL_FILE_CSV_DELIMITER "%d"),
                        analog_data[j]);
            }
        }

        // end of data row
        if (config->file_format == RL_FILE_FORMAT_CSV) {
            fprintf(data_file, "\n");
        }
    }

    // flush processed data if data is stored
    if (config->file_enable && data_file != NULL) {
        fflush(data_file);
    }

    return 1;
}

int rl_file_add_ambient_block(FILE *ambient_file, int32_t const *ambient_buffer,
                              size_t buffer_size,
                              rl_timestamp_t const *const timestamp_realtime,
                              rl_timestamp_t const *const timestamp_monotonic,
                              rl_config_t const *const config) {
    // suppress unused parameter warning
    (void)config;

    // store timestamps
    fwrite(timestamp_realtime, sizeof(rl_timestamp_t), 1, ambient_file);
    fwrite(timestamp_monotonic, sizeof(rl_timestamp_t), 1, ambient_file);

    // store sensor data
    fwrite(ambient_buffer, sizeof(int32_t), buffer_size, ambient_file);

    return 1;
}

void rl_file_setup_data_channels(rl_file_header_t *const file_header,
                                 rl_config_t const *const config) {
    int total_channel_count = file_header->lead_in.channel_bin_count +
                              file_header->lead_in.channel_count;

    // reset channels
    memset(file_header->channel, 0,
           total_channel_count * sizeof(rl_file_channel_t));

    // overall channel index
    int ch = 0;

    // digital channels
    if (config->digital_enable) {
        for (int i = 0; i < RL_CHANNEL_DIGITAL_COUNT; i++) {
            file_header->channel[ch].unit = RL_UNIT_BINARY;
            file_header->channel[ch].channel_scale = RL_SCALE_UNIT;
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
        file_header->channel[ch].channel_scale = RL_SCALE_UNIT;
        file_header->channel[ch].data_size = 0;
        file_header->channel[ch].valid_data_channel = RL_FILE_CHANNEL_NO_LINK;
        strcpy(file_header->channel[ch].name, RL_CHANNEL_VALID_NAMES[0]);
        ch++;
    }
    if (config->channel_enable[RL_CONFIG_CHANNEL_I2L]) {
        file_header->channel[ch].unit = RL_UNIT_RANGE_VALID;
        file_header->channel[ch].channel_scale = RL_SCALE_UNIT;
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
            } else if (is_voltage(i)) {
                file_header->channel[ch].unit = RL_UNIT_VOLT;
                file_header->channel[ch].channel_scale = RL_SCALE_TEN_NANO;
                file_header->channel[ch].valid_data_channel =
                    RL_FILE_CHANNEL_NO_LINK;
            } else {
                file_header->channel[ch].unit = RL_UNIT_SECOND;
                file_header->channel[ch].channel_scale = RL_SCALE_NANO;
                file_header->channel[ch].valid_data_channel =
                    RL_FILE_CHANNEL_NO_LINK;
            }
            // if calibration measurement, set unit to undefined
            if (config->calibration_ignore) {
                file_header->channel[ch].unit = RL_UNIT_UNDEFINED;
                file_header->channel[ch].channel_scale = RL_SCALE_UNIT;
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
