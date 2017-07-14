/**
 * Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group
 */

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "pru.h"
#include "util.h"

#include "file_handling.h"

/// Channel names
const char* channel_names[NUM_CHANNELS] = {"I1H", "I1L", "V1", "V2",
                                           "I2H", "I2L", "V3", "V4"};
/// Digital input names
const char* digital_input_names[NUM_DIGITAL_INPUTS] = {"DI1", "DI2", "DI3",
                                                       "DI4", "DI5", "DI6"};
/// Valid channel names
const char* valid_info_names[NUM_I_CHANNELS] = {"I1L_valid", "I2L_valid"};

/// Global variable to determine i1l valid channel
int i1l_valid_channel = 0;
/// Global variable to determine i2l valid channel
int i2l_valid_channel = 0;

/**
 * Set up file header lead-in with current configuration
 * @param lead_in Pointer to {@link rl_file_lead_in} struct to set up
 * @param conf Pointer to current {@link rl_conf} struct
 */
void file_setup_lead_in(struct rl_file_lead_in* lead_in, struct rl_conf* conf) {

    // number channels
    uint16_t channel_count = count_channels(conf->channels);
    // number binary channels
    uint16_t channel_bin_count = 0;
    if (conf->digital_inputs == DIGITAL_INPUTS_ENABLED) {
        channel_bin_count = NUM_DIGITAL_INPUTS;
    }
    if (conf->channels[I1L_INDEX] == CHANNEL_ENABLED) {
        i1l_valid_channel = ++channel_bin_count;
    }
    if (conf->channels[I2L_INDEX] == CHANNEL_ENABLED) {
        i2l_valid_channel = ++channel_bin_count;
    }
    // comment length
    uint32_t comment_length = strlen(RL_FILE_COMMENT) * sizeof(int8_t);
    // timestamps
    struct time_stamp timestamp_realtime;
    struct time_stamp timestamp_monotonic;
    create_time_stamp(&timestamp_realtime, &timestamp_monotonic);

    // lead_in setup
    lead_in->magic = RL_FILE_MAGIC;
    lead_in->file_version = RL_FILE_VERSION;
    lead_in->header_length =
        sizeof(struct rl_file_lead_in) + comment_length +
        (channel_count + channel_bin_count) * sizeof(struct rl_file_channel);
    lead_in->data_block_size = conf->sample_rate / conf->update_rate;
    lead_in->data_block_count = 0; // needs to be updated
    lead_in->sample_count = 0;     // needs to be updated
    lead_in->sample_rate = conf->sample_rate;
    get_mac_addr(lead_in->mac_address);
    lead_in->start_time = timestamp_realtime;
    lead_in->comment_length = comment_length;
    lead_in->channel_bin_count = channel_bin_count;
    lead_in->channel_count = channel_count;
}

/**
 * Set up channel information for file header with current configuration
 * @param file_header Pointer to {@link rl_file_header} struct to set up
 * @param conf Pointer to current {@link rl_conf} struct
 */
void file_setup_channels(struct rl_file_header* file_header,
                         struct rl_conf* conf) {
    int total_channel_count = file_header->lead_in.channel_bin_count +
                              file_header->lead_in.channel_count;

    // reset channels
    memset(file_header->channel, 0,
           total_channel_count * sizeof(struct rl_file_channel));

    // digital channels
    int ch = 0;
    if (conf->digital_inputs == DIGITAL_INPUTS_ENABLED) {
        for (int i = 0; i < NUM_DIGITAL_INPUTS; i++) {
            file_header->channel[ch].unit = RL_UNIT_BINARY;
            file_header->channel[ch].channel_scale = RL_SCALE_NONE;
            file_header->channel[ch].data_size = 0;
            file_header->channel[ch].valid_data_channel = NO_VALID_DATA;
            strcpy(file_header->channel[ch].name, digital_input_names[i]);
            ch++;
        }
    }

    // range valid channels
    if (conf->channels[I1L_INDEX] == CHANNEL_ENABLED) {
        file_header->channel[ch].unit = RL_UNIT_RANGE_VALID;
        file_header->channel[ch].channel_scale = RL_SCALE_NONE;
        file_header->channel[ch].data_size = 0;
        file_header->channel[ch].valid_data_channel = NO_VALID_DATA;
        strcpy(file_header->channel[ch].name, valid_info_names[0]);
        ch++;
    }
    if (conf->channels[I2L_INDEX] == CHANNEL_ENABLED) {
        file_header->channel[ch].unit = RL_UNIT_RANGE_VALID;
        file_header->channel[ch].channel_scale = RL_SCALE_NONE;
        file_header->channel[ch].data_size = 0;
        file_header->channel[ch].valid_data_channel = NO_VALID_DATA;
        strcpy(file_header->channel[ch].name, valid_info_names[1]);
        ch++;
    }

    // analog channels
    for (int i = 0; i < NUM_CHANNELS; i++) {
        if (conf->channels[i] == CHANNEL_ENABLED) {
            // current
            if (is_current(i)) {
                // low
                if (is_low_current(i)) {
                    file_header->channel[ch].channel_scale = RL_SCALE_TEN_PICO;
                    if (i == I1L_INDEX) {
                        file_header->channel[ch].valid_data_channel =
                            i1l_valid_channel;
                    } else {
                        file_header->channel[ch].valid_data_channel =
                            i2l_valid_channel;
                    }
                    // high
                } else {
                    file_header->channel[ch].channel_scale = RL_SCALE_NANO;
                    file_header->channel[ch].valid_data_channel = NO_VALID_DATA;
                }
                file_header->channel[ch].unit = RL_UNIT_AMPERE;
                // voltage
            } else {
                file_header->channel[ch].unit = RL_UNIT_VOLT;
                file_header->channel[ch].channel_scale = RL_SCALE_TEN_NANO;
                file_header->channel[ch].valid_data_channel = NO_VALID_DATA;
            }
            file_header->channel[ch].data_size = 4;
            strcpy(file_header->channel[ch].name, channel_names[i]);
            ch++;
        }
    }
}

/**
 * Set up file header with current configuration
 * @param file_header Pointer to {@link rl_file_header} to set up
 * @param conf Pointer to current {@link rl_conf} struct
 */
void file_setup_header(struct rl_file_header* file_header,
                       struct rl_conf* conf) {

    // comment
    char* comment = RL_FILE_COMMENT;
    file_header->comment = comment;

    // channels
    file_setup_channels(file_header, conf);
}

/**
 * Store file header to file (in binary format)
 * @param data File pointer to data file
 * @param file_header Pointer to {@link rl_file_header} struct
 */
void file_store_header_bin(FILE* data_file,
                           struct rl_file_header* file_header) {

    int total_channel_count = file_header->lead_in.channel_bin_count +
                              file_header->lead_in.channel_count;

    // check if alignment bytes are needed after header comment
    int comment_length = strlen(file_header->comment);
    int comment_unaligned_bytes =
        comment_length % RL_FILE_COMMENT_ALIGNMENT_BYTES;
    if (comment_unaligned_bytes > 0) {
        file_header->lead_in.comment_length = comment_length +
                                              RL_FILE_COMMENT_ALIGNMENT_BYTES -
                                              comment_unaligned_bytes;
        file_header->lead_in.header_length =
            sizeof(struct rl_file_lead_in) +
            file_header->lead_in.comment_length +
            total_channel_count * sizeof(struct rl_file_channel);
    }

    // write lead-in
    fwrite(&(file_header->lead_in), sizeof(struct rl_file_lead_in), 1,
           data_file);

    // write comment, add zero bytes for proper header alignment if necessary
    fwrite(file_header->comment, comment_length, 1, data_file);
    if (comment_unaligned_bytes > 0) {
        uint8_t zero_bytes[RL_FILE_COMMENT_ALIGNMENT_BYTES] = {0};
        fwrite(zero_bytes,
               RL_FILE_COMMENT_ALIGNMENT_BYTES - comment_unaligned_bytes, 1,
               data_file);
    }

    // write channel information
    fwrite(file_header->channel, sizeof(struct rl_file_channel),
           total_channel_count, data_file);
}

/**
 * Store file header to file (in CSV format)
 * @param data_file File pointer to data file
 * @param file_header Pointer to {@link rl_file_header} struct
 */
void file_store_header_csv(FILE* data_file,
                           struct rl_file_header* file_header) {
    // lead-in
    fprintf(data_file, "RocketLogger CSV File\n");
    fprintf(data_file, "File Version,%u\n",
            (uint32_t)file_header->lead_in.file_version);
    fprintf(data_file, "Block Size,%u\n",
            (uint32_t)file_header->lead_in.data_block_size);
    fprintf(data_file, "Block Count,%-20u\n",
            (uint32_t)file_header->lead_in.data_block_count);
    fprintf(data_file, "Sample Count,%-20llu\n",
            (uint64_t)file_header->lead_in.sample_count);
    fprintf(data_file, "Sample Rate,%u\n",
            (uint32_t)file_header->lead_in.sample_rate);
    fprintf(data_file, "MAC Address,%02x",
            (uint32_t)file_header->lead_in.mac_address[0]);

    for (int i = 0; i < MAC_ADDRESS_LENGTH; i++) {
        fprintf(data_file, ":%02x",
                (uint32_t)file_header->lead_in.mac_address[i]);
    }
    fprintf(data_file, "\n");

    time_t time = (time_t)file_header->lead_in.start_time.sec;
    fprintf(data_file, "Start Time,%s", ctime(&time));
    fprintf(data_file, "Comment,%s\n", file_header->comment);
    fprintf(data_file, "\n");

    // channels
    for (int i = 0; i < (file_header->lead_in.channel_count +
                         file_header->lead_in.channel_bin_count);
         i++) {
        fprintf(data_file, ",%s", file_header->channel[i].name);
        switch (file_header->channel[i].channel_scale) {
        case RL_SCALE_MILLI:
            fprintf(data_file, " [m");
            break;
        case RL_SCALE_MICRO:
            fprintf(data_file, " [u");
            break;
        case RL_SCALE_TEN_NANO:
            fprintf(data_file, " [10n");
            break;
        case RL_SCALE_NANO:
            fprintf(data_file, " [n");
            break;
        case RL_SCALE_TEN_PICO:
            fprintf(data_file, " [10p");
            break;
        default:
            break;
        }
        switch (file_header->channel[i].unit) {
        case RL_UNIT_VOLT:
            fprintf(data_file, "V]");
            break;
        case RL_UNIT_AMPERE:
            fprintf(data_file, "A]");
            break;
        default:
            break;
        }
    }
    fprintf(data_file, "\n");
}

/**
 * Update file with new header lead-in (to write current sample count) in binary
 * format
 * @param data_file File pointer to data file
 * @param file_header Pointer to {@link rl_file_header} struct
 */
void file_update_header_bin(FILE* data_file,
                            struct rl_file_header* file_header) {

    // seek to beginning and rewrite lead_in
    rewind(data_file);
    fwrite(&(file_header->lead_in), sizeof(struct rl_file_lead_in), 1,
           data_file);
    fseek(data_file, 0, SEEK_END);
}

/**
 * Update file with new header lead-in (to write current sample count) in CSV
 * format
 * @param data_file File pointer to data file
 * @param file_header Pointer to {@link rl_file_header} struct
 */
void file_update_header_csv(FILE* data_file,
                            struct rl_file_header* file_header) {
    rewind(data_file);
    fprintf(data_file, "RocketLogger CSV File\n");
    fprintf(data_file, "File Version,%u\n",
            (uint32_t)file_header->lead_in.file_version);
    fprintf(data_file, "Block Size,%u\n",
            (uint32_t)file_header->lead_in.data_block_size);
    fprintf(data_file, "Block Count,%-20u\n",
            (uint32_t)file_header->lead_in.data_block_count);
    fprintf(data_file, "Sample Count,%-20llu\n",
            (uint64_t)file_header->lead_in.sample_count);
    fseek(data_file, 0, SEEK_END);
}

/**
 * Handle a data buffer, dependent on current configuration
 * @param data_file File pointer to data file
 * @param buffer_addr Pointer to buffer to handle
 * @param sample_data_size Data size of the samples in bytes
 * @param samples_count Number of samples to read
 * @param conf Current {@link rl_conf} configuration.
 */
void file_handle_data(FILE* data_file, void* buffer_addr,
                      uint32_t sample_data_size, uint32_t samples_count,
                      struct time_stamp* timestamp_realtime,
                      struct time_stamp* timestamp_monotonic,
                      struct rl_conf* conf) {

    // write timestamp to file
    if (conf->file_format == BIN) {
        fwrite(&timestamp_realtime, sizeof(struct time_stamp), 1, data_file);
        fwrite(&timestamp_monotonic, sizeof(struct time_stamp), 1, data_file);
    } else if (conf->file_format == CSV) {
        fprintf(data_file, "%lli.%09lli", timestamp_realtime->sec,
                timestamp_realtime->nsec);
    }

    // count channels
    int num_bin_channels = 0;
    if (conf->digital_inputs == DIGITAL_INPUTS_ENABLED) {
        num_bin_channels = NUM_DIGITAL_INPUTS;
    }

    int num_channels = count_channels(conf->channels);

    // HANDLE BUFFER //
    for (uint32_t i = 0; i < samples_count; i++) {

        // channel data variables
        int32_t channel_data[NUM_CHANNELS];
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
                channel_data[ch] =
                    (int32_t)((adc_value + calibration.offsets[j]) *
                              calibration.scales[j]);
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

        // handle data aggregation for low sampling rates
        // @TODO implement data aggregation
        if (conf->sample_rate < MIN_ADC_RATE) {
            rl_log(INFO,
                   "Data aggregation for file storing not supported yet.");
        }

        // // HANDLE AVERAGE DATA //

        // if (conf->sample_rate < MIN_ADC_RATE) {

        //     // buffer 1
        //     if ((i + 1) % avg_length[BUF1_INDEX] == 0) {

        //         // average
        //         for (j = 0; j < num_channels; j++) {
        //             avg_data[BUF1_INDEX][j] /= avg_length[BUF1_INDEX];
        //             avg_data[BUF10_INDEX][j] += avg_data[BUF1_INDEX][j];
        //         }

        //         // average bin channels
        //         for (j = 0; j < num_bin_channels; j++) {
        //             bin_avg_data[BUF10_INDEX][j] +=
        //             bin_avg_data[BUF1_INDEX][j];
        //         }

        //         // write data to file
        //         if (conf->sample_rate == 100) {
        //             // binary data
        //             if (bin_channel_pos > 0) {
        //                 if (conf->file_format == BIN) {
        //                     uint32_t temp_bin = 0;
        //                     for (j = 0; j < num_bin_channels; j++) {
        //                         temp_bin =
        //                             temp_bin | ((bin_avg_data[BUF1_INDEX][j]
        //                             >=
        //                                          (avg_length[BUF1_INDEX] /
        //                                          2))
        //                                         << j);
        //                     }
        //                     if (conf->channels[I1L_INDEX] == CHANNEL_ENABLED)
        //                     {
        //                         temp_bin = temp_bin |
        //                                    (avg_valid[BUF1_INDEX][0] << j++);
        //                     }
        //                     if (conf->channels[I2L_INDEX] == CHANNEL_ENABLED)
        //                     {
        //                         temp_bin = temp_bin |
        //                                    (avg_valid[BUF1_INDEX][1] << j++);
        //                     }

        //                     fwrite(&temp_bin, sizeof(uint32_t), 1,
        //                     data_file);

        //                 } else if (conf->file_format == CSV) {
        //                     for (j = 0; j < num_bin_channels; j++) {
        //                         sprintf(value_char, ", %d",
        //                                 (bin_avg_data[BUF1_INDEX][j] >=
        //                                  (avg_length[BUF1_INDEX] / 2)));
        //                         strcat(channel_data_char, value_char);
        //                     }
        //                     if (conf->channels[I1L_INDEX] == CHANNEL_ENABLED)
        //                     {
        //                         sprintf(value_char, ", %d",
        //                                 avg_valid[BUF1_INDEX][0]);
        //                         strcat(channel_data_char, value_char);
        //                     }
        //                     if (conf->channels[I2L_INDEX] == CHANNEL_ENABLED)
        //                     {
        //                         sprintf(value_char, ", %d",
        //                                 avg_valid[BUF1_INDEX][1]);
        //                         strcat(channel_data_char, value_char);
        //                     }
        //                 }
        //             }

        //             // channel data
        //             if (conf->file_format == BIN) {
        //                 for (j = 0; j < num_channels; j++) {
        //                     int32_t tmp = (int32_t)avg_data[BUF1_INDEX][j];
        //                     fwrite(&tmp, sizeof(int32_t), 1, data_file);
        //                 }
        //             } else if (conf->file_format == CSV) {
        //                 for (j = 0; j < num_channels; j++) {
        //                     sprintf(value_char, ",%d",
        //                             (int32_t)avg_data[BUF1_INDEX][j]);
        //                     strcat(channel_data_char, value_char);
        //                 }
        //                 strcat(channel_data_char, "\n");
        //                 fprintf(data_file, "%s", channel_data_char);
        //             }
        //         }

        //         // reset values
        //         memset(avg_data[BUF1_INDEX], 0, sizeof(int64_t) *
        //         num_channels);
        //         memset(bin_avg_data[BUF1_INDEX], 0,
        //                sizeof(uint32_t) * num_bin_channels);
        //         avg_valid[BUF1_INDEX][0] = 1;
        //         avg_valid[BUF1_INDEX][1] = 1;
        //     }

        //     // buffer 10
        //     if ((i + 1) % avg_length[BUF10_INDEX] == 0) {

        //         // average
        //         for (j = 0; j < num_channels; j++) {
        //             avg_data[BUF10_INDEX][j] /=
        //                 (avg_length[BUF10_INDEX] / avg_length[BUF1_INDEX]);
        //             avg_data[BUF100_INDEX][j] += avg_data[BUF10_INDEX][j];
        //         }

        //         // average bin channels
        //         for (j = 0; j < num_bin_channels; j++) {

        //             bin_avg_data[BUF100_INDEX][j] +=
        //                 bin_avg_data[BUF10_INDEX][j];
        //         }

        //         // write data to file
        //         if (conf->sample_rate == 10) {
        //             // binary data
        //             if (bin_channel_pos > 0) {
        //                 if (conf->file_format == BIN) {
        //                     uint32_t temp_bin = 0;
        //                     for (j = 0; j < num_bin_channels; j++) {
        //                         temp_bin =
        //                             temp_bin | ((bin_avg_data[BUF10_INDEX][j]
        //                             >=
        //                                          (avg_length[BUF10_INDEX] /
        //                                          2))
        //                                         << j);
        //                     }
        //                     if (conf->channels[I1L_INDEX] == CHANNEL_ENABLED)
        //                     {
        //                         temp_bin = temp_bin |
        //                                    (avg_valid[BUF10_INDEX][0] <<
        //                                    j++);
        //                     }
        //                     if (conf->channels[I2L_INDEX] == CHANNEL_ENABLED)
        //                     {
        //                         temp_bin = temp_bin |
        //                                    (avg_valid[BUF10_INDEX][1] <<
        //                                    j++);
        //                     }

        //                     fwrite(&temp_bin, sizeof(uint32_t), 1,
        //                     data_file);

        //                 } else if (conf->file_format == CSV) {
        //                     for (j = 0; j < num_bin_channels; j++) {
        //                         sprintf(value_char, ", %d",
        //                                 (bin_avg_data[BUF10_INDEX][j] >=
        //                                  (avg_length[BUF10_INDEX] / 2)));
        //                         strcat(channel_data_char, value_char);
        //                     }
        //                     if (conf->channels[I1L_INDEX] == CHANNEL_ENABLED)
        //                     {
        //                         sprintf(value_char, ", %d",
        //                                 avg_valid[BUF10_INDEX][0]);
        //                         strcat(channel_data_char, value_char);
        //                     }
        //                     if (conf->channels[I2L_INDEX] == CHANNEL_ENABLED)
        //                     {
        //                         sprintf(value_char, ", %d",
        //                                 avg_valid[BUF10_INDEX][1]);
        //                         strcat(channel_data_char, value_char);
        //                     }
        //                 }
        //             }

        //             // channel data
        //             if (conf->file_format == BIN) {
        //                 for (j = 0; j < num_channels; j++) {
        //                     int32_t tmp = (int32_t)avg_data[BUF10_INDEX][j];
        //                     fwrite(&tmp, sizeof(int32_t), 1, data_file);
        //                 }
        //             } else if (conf->file_format == CSV) {
        //                 for (j = 0; j < num_channels; j++) {
        //                     sprintf(value_char, ",%d",
        //                             (int32_t)avg_data[BUF10_INDEX][j]);
        //                     strcat(channel_data_char, value_char);
        //                 }
        //                 strcat(channel_data_char, "\n");
        //                 fprintf(data_file, "%s", channel_data_char);
        //             }
        //         }

        //         // reset values
        //         memset(avg_data[BUF10_INDEX], 0,
        //                sizeof(int64_t) * num_channels);
        //         memset(bin_avg_data[BUF10_INDEX], 0,
        //                sizeof(uint32_t) * num_bin_channels);
        //         avg_valid[BUF10_INDEX][0] = 1;
        //         avg_valid[BUF10_INDEX][1] = 1;
        //     }

        //     // buffer 100
        //     if ((i + 1) % avg_length[BUF100_INDEX] == 0) {

        //         // average
        //         for (j = 0; j < num_channels; j++) {
        //             avg_data[BUF100_INDEX][j] /=
        //                 (avg_length[BUF100_INDEX] / avg_length[BUF10_INDEX]);
        //         }

        //         // write data to file
        //         if (conf->sample_rate == 1) {
        //             // binary data
        //             if (bin_channel_pos > 0) {
        //                 if (conf->file_format == BIN) {
        //                     uint32_t temp_bin = 0;
        //                     for (j = 0; j < num_bin_channels; j++) {
        //                         temp_bin = temp_bin |
        //                                    ((bin_avg_data[BUF100_INDEX][j] >=
        //                                      (avg_length[BUF100_INDEX] / 2))
        //                                     << j);
        //                     }
        //                     if (conf->channels[I1L_INDEX] == CHANNEL_ENABLED)
        //                     {
        //                         temp_bin = temp_bin |
        //                                    (avg_valid[BUF100_INDEX][0] <<
        //                                    j++);
        //                     }
        //                     if (conf->channels[I2L_INDEX] == CHANNEL_ENABLED)
        //                     {
        //                         temp_bin = temp_bin |
        //                                    (avg_valid[BUF100_INDEX][1] <<
        //                                    j++);
        //                     }

        //                     fwrite(&temp_bin, sizeof(uint32_t), 1,
        //                     data_file);

        //                 } else if (conf->file_format == CSV) {
        //                     for (j = 0; j < num_bin_channels; j++) {
        //                         sprintf(value_char, ", %d",
        //                                 (bin_avg_data[BUF100_INDEX][j] >=
        //                                  (avg_length[BUF100_INDEX] / 2)));
        //                         strcat(channel_data_char, value_char);
        //                     }
        //                     if (conf->channels[I1L_INDEX] == CHANNEL_ENABLED)
        //                     {
        //                         sprintf(value_char, ", %d",
        //                                 avg_valid[BUF100_INDEX][0]);
        //                         strcat(channel_data_char, value_char);
        //                     }
        //                     if (conf->channels[I2L_INDEX] == CHANNEL_ENABLED)
        //                     {
        //                         sprintf(value_char, ", %d",
        //                                 avg_valid[BUF100_INDEX][1]);
        //                         strcat(channel_data_char, value_char);
        //                     }
        //                 }
        //             }

        //             // channel data
        //             if (conf->file_format == BIN) {
        //                 for (j = 0; j < num_channels; j++) {
        //                     int32_t tmp = (int32_t)avg_data[BUF100_INDEX][j];
        //                     fwrite(&tmp, sizeof(int32_t), 1, data_file);
        //                 }
        //             } else if (conf->file_format == CSV) {
        //                 for (j = 0; j < num_channels; j++) {
        //                     sprintf(value_char, ",%d",
        //                             (int32_t)avg_data[BUF100_INDEX][j]);
        //                     strcat(channel_data_char, value_char);
        //                 }
        //                 strcat(channel_data_char, "\n");
        //                 fprintf(data_file, "%s", channel_data_char);
        //             }
        //         }
        //     }
        // }

        // WRITE FILE IF HIGH RATE //

        if (conf->sample_rate >= MIN_ADC_RATE) {
            // write binary channels
            if (bin_channel_pos > 0) {
                if (conf->file_format == BIN) {
                    fwrite(&bin_data, sizeof(uint32_t), 1, data_file);
                } else if (conf->file_format == CSV) {
                    uint32_t MASK = 0x01;
                    for (int j = 0; j < bin_channel_pos; j++) {
                        fprintf(data_file, (CSV_DELIMITER "%i"),
                                (bin_data & MASK) > 0);
                        MASK = MASK << 1;
                    }
                }
            }

            // write analog channels
            if (conf->file_format == BIN) {
                fwrite(channel_data, sizeof(int32_t), num_channels, data_file);
            } else if (conf->file_format == CSV) {
                for (int j = 0; j < num_channels; j++) {
                    fprintf(data_file, (CSV_DELIMITER "%d"), channel_data[j]);
                }
                fprintf(data_file, "\n");
            }
        }
    }

    // flush processed data
    fflush(data_file);
}
