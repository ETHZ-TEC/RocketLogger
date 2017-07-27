
#include "sensor/sensor.h"

#include "ambient.h"

/**
 * Handle a ambient data buffer, dependent on current configuration
 * @param ambient_file File pointer to ambient file
 * @param timestamp_realtime {@link time_stamp} with realtime clock value
 * @param timestamp_monotonic {@link time_stamp} with monotonic clock value
 * @param conf Current {@link rl_conf} configuration.
 */
void ambient_store_data(FILE* ambient_file,
                        struct time_stamp* timestamp_realtime,
                        struct time_stamp* timestamp_monotonic,
                        struct rl_conf* conf) {

    // store timestamp
    fwrite(&timestamp_realtime, sizeof(struct time_stamp), 1, ambient_file);
    fwrite(&timestamp_monotonic, sizeof(struct time_stamp), 1, ambient_file);

    // FETCH VALUES //
    int32_t sensor_data[conf->ambient.sensor_count];

    int ch = 0;
    uint8_t mutli_channel_read = 0xff;
    for (int i = 0; i < SENSOR_REGISTRY_SIZE; i++) {
        // only read registered sensors
        if (conf->ambient.available_sensors[i] > 0) {
            // read multi-channel sensor data only once
            if (sensor_registry[i].address != mutli_channel_read) {
                sensor_registry[i].read(sensor_registry[i].address);
                mutli_channel_read = sensor_registry[i].address;
            }
            sensor_data[ch] = sensor_registry[i].getValue(
                sensor_registry[i].address, sensor_registry[i].channel);
            ch++;
        }
    }

    // WRITE VALUES //
    fwrite(sensor_data, sizeof(int32_t), conf->ambient.sensor_count,
           ambient_file);
}

// FILE HEADER //

void ambient_set_file_name(struct rl_conf* conf) {

    // determine new file name
    char ambient_file_name[MAX_PATH_LENGTH];
    strcpy(ambient_file_name, conf->file_name);

    // search for last .
    char target = '.';
    char* file_ending = ambient_file_name;
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
    strcpy(conf->ambient.file_name, ambient_file_name);
}

void ambient_setup_lead_in(struct rl_file_lead_in* lead_in,
                           struct rl_conf* conf) {

    // number channels
    uint16_t channel_count = conf->ambient.sensor_count;
    // number binary channels
    uint16_t channel_bin_count = 0;
    // comment length
    uint32_t comment_length = strlen(RL_FILE_COMMENT) * sizeof(int8_t);
    // timestamps
    struct time_stamp time_real;
    struct time_stamp time_monotonic;
    create_time_stamp(&time_real, &time_monotonic);

    // lead_in setup
    lead_in->magic = RL_FILE_MAGIC;
    lead_in->file_version = RL_FILE_VERSION;
    lead_in->header_length =
        sizeof(struct rl_file_lead_in) + comment_length +
        (channel_count + channel_bin_count) * sizeof(struct rl_file_channel);
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

void ambient_setup_channels(struct rl_file_header* file_header,
                            struct rl_conf* conf) {

    int total_channel_count = file_header->lead_in.channel_bin_count +
                              file_header->lead_in.channel_count;

    // reset channels
    memset(file_header->channel, 0,
           total_channel_count * sizeof(struct rl_file_channel));

    // write channels
    int ch = 0;
    for (int i = 0; i < SENSOR_REGISTRY_SIZE; i++) {
        if (conf->ambient.available_sensors[i] > 0) {

            file_header->channel[ch].unit = sensor_registry[i].unit;
            file_header->channel[ch].channel_scale = sensor_registry[i].scale;
            file_header->channel[ch].valid_data_channel = NO_VALID_DATA;
            file_header->channel[ch].data_size = 4;
            strcpy(file_header->channel[ch].name, sensor_registry[i].name);
            ch++;
        }
    }
}

void ambient_setup_header(struct rl_file_header* file_header,
                          struct rl_conf* conf) {

    // comment
    char* comment = RL_FILE_COMMENT;
    file_header->comment = comment;

    // channels
    ambient_setup_channels(file_header, conf);
}
