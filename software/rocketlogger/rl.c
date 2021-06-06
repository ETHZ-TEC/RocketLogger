/*
 * Copyright (c) 2016-2020, Swiss Federal Institute of Technology (ETH Zurich)
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
 * ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <zmq.h>

#include "log.h"
#include "sensor/sensor.h"

#include "rl.h"

#define RL_JSON_BUFFER_SIZE 10000

/**
 * RocketLogger reset configuration definition.
 */
const rl_config_t rl_config_default = {
    .config_version = RL_CONFIG_VERSION,
    .background_enable = false,
    .interactive_enable = false,
    .sample_limit = 0,
    .sample_rate = RL_SAMPLE_RATE_MIN,
    .update_rate = 1,
    .channel_enable = RL_CONFIG_CHANNEL_ENABLE_DEFAULT,
    .channel_force_range = RL_CONFIG_CHANNEL_FORCE_RANGE_DEFAULT,
    .aggregation_mode = RL_AGGREGATION_MODE_DOWNSAMPLE,
    .digital_enable = true,
    .web_enable = true,
    .calibration_ignore = false,
    .ambient_enable = false,
    .file_enable = true,
    .file_name = RL_CONFIG_FILE_DEFAULT,
    .file_format = RL_FILE_FORMAT_RLD,
    .file_size = RL_CONFIG_FILE_SIZE_DEFAULT,
    .file_comment = RL_CONFIG_COMMENT_DEFAULT,
};

/**
 * RocketLogger reset status definition.
 */
const rl_status_t rl_status_default = {
    .sampling = false,
    .error = false,
    .sample_count = 0,
    .buffer_count = 0,
    .calibration_time = 0,
    .calibration_file = RL_CALIBRATION_SYSTEM_FILE,
    .disk_free = 0,
    .disk_free_permille = 0,
    .disk_use_rate = 0,
    .sensor_count = 0,
    .sensor_available = {false},
    .config = NULL,
};

/// RocketLogger channel names
char const *const RL_CHANNEL_NAMES[RL_CHANNEL_COUNT] = {
    "V1", "V2", "V3", "V4", "I1L", "I1H", "I2L", "I2H", "DT"};

/// RocketLogger force range channel names
char const *const RL_CHANNEL_FORCE_NAMES[RL_CHANNEL_SWITCHED_COUNT] = {"I1H",
                                                                       "I2H"};

/// RocketLogger digital channel names
char const *const RL_CHANNEL_DIGITAL_NAMES[RL_CHANNEL_DIGITAL_COUNT] = {
    "DI1", "DI2", "DI3", "DI4", "DI5", "DI6"};

/// RocketLogger valid channel names
char const *const RL_CHANNEL_VALID_NAMES[RL_CHANNEL_SWITCHED_COUNT] = {
    "I1L_valid", "I2L_valid"};

/// Global RocketLogger status variable.
//rl_status_t rl_status = rl_status_default;
rl_status_t rl_status = {
    .sampling = false,
    .error = false,
    .sample_count = 0,
    .buffer_count = 0,
    .calibration_time = 0,
    .calibration_file = RL_CALIBRATION_SYSTEM_FILE,
    .disk_free = 0,
    .disk_free_permille = 0,
    .disk_use_rate = 0,
    .sensor_count = 0,
    .sensor_available = {false},
    .config = NULL,
};

/// The ZeroMQ context for status publishing
void *zmq_status_context = NULL;
/// The ZeroMQ status publisher
void *zmq_status_publisher = NULL;

/**
 * Print a configuration setting line with formated string value.
 *
 * @param description Configuration description
 * @param format Configuration formatting string passed to printf()
 * @param ... Variables used to format value string
 */
static void print_config_line(char const *const description, char const *format,
                              ...);

void rl_config_print(rl_config_t const *const config) {
    // sampling in background or interactively
    print_config_line("Run in background",
                      config->background_enable ? "yes" : "no");
    print_config_line("Interactive data display",
                      config->interactive_enable ? "yes" : "no");
    print_config_line("Sample limit", "%llu", config->sample_limit);

    // sample rate and aggregation
    print_config_line("Sampling rate", "%u Sps", config->sample_rate);
    switch (config->aggregation_mode) {
    case RL_AGGREGATION_MODE_DOWNSAMPLE:
        print_config_line("Data aggregation", "down sample");
        break;

    case RL_AGGREGATION_MODE_AVERAGE:
        print_config_line("Data aggregation", "average samples");
        break;
    default:
        print_config_line("Data aggregation", "undefined");
        break;
    }

    // channel and force configuration
    print_config_line(
        "Channels enabled", "V1:  %s  V2:  %s  V3:  %s  V4:  %s",
        config->channel_enable[RL_CONFIG_CHANNEL_V1] ? "on " : "off",
        config->channel_enable[RL_CONFIG_CHANNEL_V2] ? "on " : "off",
        config->channel_enable[RL_CONFIG_CHANNEL_V3] ? "on " : "off",
        config->channel_enable[RL_CONFIG_CHANNEL_V4] ? "on " : "off");
    print_config_line(
        "", "I1L: %s  I1H: %s  I2L: %s  I2H: %s  DT:  %s",
        config->channel_enable[RL_CONFIG_CHANNEL_I1L] ? "on " : "off",
        config->channel_enable[RL_CONFIG_CHANNEL_I1H] ? "on " : "off",
        config->channel_enable[RL_CONFIG_CHANNEL_I2L] ? "on " : "off",
        config->channel_enable[RL_CONFIG_CHANNEL_I2H] ? "on " : "off",
        config->channel_enable[RL_CONFIG_CHANNEL_DT] ? "on " : "off");
    print_config_line(
        "Force High Channels", "I1H: %s  I2H: %s",
        config->channel_force_range[RL_CONFIG_CHANNEL_I1] ? "on " : "off",
        config->channel_force_range[RL_CONFIG_CHANNEL_I2] ? "on " : "off");

    print_config_line("Digital inputs",
                      config->digital_enable ? "enabled" : "disabled");

    print_config_line("File storing",
                      config->file_enable ? "enabled" : "disabled");
    print_config_line("File name", config->file_name);
    switch (config->file_format) {
    case RL_FILE_FORMAT_RLD:
        print_config_line("File format", "RLD binary file");
        break;

    case RL_FILE_FORMAT_CSV:
        print_config_line("File format", "CSV text file");
        break;
    default:
        print_config_line("File format", "undefined");
        break;
    }
    print_config_line("Max. file size", "%llu Bytes", config->file_size);

    print_config_line("Update rate", "%u Hz", config->update_rate);
    print_config_line("Web server",
                      config->web_enable ? "enabled" : "disabled");
    print_config_line("Calibration measurement",
                      config->calibration_ignore ? "enabled" : "disabled");
}

void rl_config_print_cmd(rl_config_t const *const config) {
    // start appending command step by step
    printf("rocketlogger start");

    // sampling mode and limit
    if (config->background_enable) {
        printf(" --background");
    }
    if (config->interactive_enable) {
        printf(" --interactive");
    }

    printf(" --samples=%llu", config->sample_limit);

    // sample rate and aggregation
    printf(" --rate=%u", config->sample_rate);
    printf(" --update=%u", config->update_rate);

    // channels
    printf(" --channel=");
    for (int i = 0; i < RL_CHANNEL_COUNT; i++) {
        if (config->channel_enable[i]) {
            printf("%s,", RL_CHANNEL_NAMES[i]);
        }
    }
    printf(" --high-range=");
    for (int i = 0; i < RL_CHANNEL_SWITCHED_COUNT; i++) {
        if (config->channel_force_range[i]) {
            printf("%s,", RL_CHANNEL_FORCE_NAMES[i]);
        }
    }
    printf(" ");

    switch (config->aggregation_mode) {
    case RL_AGGREGATION_MODE_AVERAGE:
        printf(" --aggregate=average");
        break;
    case RL_AGGREGATION_MODE_DOWNSAMPLE:
    default:
        printf(" --aggregate=downsample");
        break;
    }

    // arguments
    printf(" --ambient=%s", config->ambient_enable ? "true" : "false");
    printf(" --digital=%s", config->digital_enable ? "true" : "false");
    printf(" --web=%s", config->web_enable ? "true" : "false");

    if (config->calibration_ignore) {
        printf(" --calibration");
    }

    // file
    if (config->file_enable) {
        printf(" --output=%s", config->file_name);
        printf(" --format=%s",
               (config->file_format == RL_FILE_FORMAT_RLD) ? "rld" : "csv");
        printf(" --size=%llu", config->file_size);
        printf(" --comment='%s'\n", config->file_comment);
    } else {
        printf(" --output=0\n");
    }
}

void rl_config_print_json(rl_config_t const *const config) {
    char const *const config_json = rl_config_get_json(config);
    printf("%s", config_json);
}

char *rl_config_get_json(rl_config_t const *const config) {
    int count;
    static char buffer[RL_JSON_BUFFER_SIZE];

    snprintf(buffer, RL_JSON_BUFFER_SIZE, "{ ");
    snprintfcat(buffer, RL_JSON_BUFFER_SIZE, "\"ambient_enable\": %s, ",
                config->ambient_enable ? "true" : "false");
    snprintfcat(buffer, RL_JSON_BUFFER_SIZE, "\"background_enable\": %s, ",
                config->background_enable ? "true" : "false");
    snprintfcat(buffer, RL_JSON_BUFFER_SIZE, "\"interactive_enable\": %s, ",
                config->interactive_enable ? "true " : "false");
    snprintfcat(buffer, RL_JSON_BUFFER_SIZE, "\"calibration_ignore\": %s, ",
                config->calibration_ignore ? "true" : "false");

    snprintfcat(buffer, RL_JSON_BUFFER_SIZE, "\"channel_enable\": [");
    count = 0;
    for (int i = 0; i < RL_CHANNEL_COUNT; i++) {
        if (!config->channel_enable[i]) {
            continue;
        }
        if (count > 0) {
            snprintfcat(buffer, RL_JSON_BUFFER_SIZE, ", ");
        }
        snprintfcat(buffer, RL_JSON_BUFFER_SIZE, "\"%s\"", RL_CHANNEL_NAMES[i]);
        count++;
    }
    snprintfcat(buffer, RL_JSON_BUFFER_SIZE, "], ");

    snprintfcat(buffer, RL_JSON_BUFFER_SIZE, "\"channel_force_range\": [");
    count = 0;
    for (int i = 0; i < RL_CHANNEL_SWITCHED_COUNT; i++) {
        if (!config->channel_force_range[i]) {
            continue;
        }
        if (count > 0) {
            snprintfcat(buffer, RL_JSON_BUFFER_SIZE, ", ");
        }
        snprintfcat(buffer, RL_JSON_BUFFER_SIZE, "\"%s\"",
                    RL_CHANNEL_FORCE_NAMES[i]);
        count++;
    }
    snprintfcat(buffer, RL_JSON_BUFFER_SIZE, "], ");
    snprintfcat(buffer, RL_JSON_BUFFER_SIZE, "\"digital_enable\": %s, ",
                config->digital_enable ? "true" : "false");
    if (config->file_enable == false) {
        snprintfcat(buffer, RL_JSON_BUFFER_SIZE, "\"file\": null, ");
    } else {
        snprintfcat(buffer, RL_JSON_BUFFER_SIZE, "\"file\": { ");
        snprintfcat(buffer, RL_JSON_BUFFER_SIZE, "\"comment\": \"%s\", ",
                    config->file_comment);
        snprintfcat(buffer, RL_JSON_BUFFER_SIZE, "\"filename\": \"%s\", ",
                    config->file_name);
        switch (config->file_format) {
        case RL_FILE_FORMAT_RLD:
            snprintfcat(buffer, RL_JSON_BUFFER_SIZE, "\"format\": \"rld\", ");
            break;
        case RL_FILE_FORMAT_CSV:
            snprintfcat(buffer, RL_JSON_BUFFER_SIZE, "\"format\": \"csv\", ");
            break;
        default:
            snprintfcat(buffer, RL_JSON_BUFFER_SIZE, "\"format\": null, ");
            break;
        }
        snprintfcat(buffer, RL_JSON_BUFFER_SIZE, "\"size\": %llu",
                    config->file_size);
        snprintfcat(buffer, RL_JSON_BUFFER_SIZE, " }, ");
    }
    snprintfcat(buffer, RL_JSON_BUFFER_SIZE, "\"sample_limit\": %llu, ",
                config->sample_limit);
    snprintfcat(buffer, RL_JSON_BUFFER_SIZE, "\"sample_rate\": %u, ",
                config->sample_rate);
    snprintfcat(buffer, RL_JSON_BUFFER_SIZE, "\"update_rate\": %u, ",
                config->update_rate);
    snprintfcat(buffer, RL_JSON_BUFFER_SIZE, "\"web_enable\": %s",
                config->web_enable ? "true" : "false");
    snprintfcat(buffer, RL_JSON_BUFFER_SIZE, " }");

    return buffer;
}

void rl_config_reset(rl_config_t *const config) {
    memcpy(config, &rl_config_default, sizeof(rl_config_t));
}

int rl_config_read_default(rl_config_t *const config) {
    char *config_file = NULL;
    int ret;

    // check if user/system config file existing
    ret = access(RL_CONFIG_USER_FILE, R_OK);
    if (ret == 0) {
        config_file = RL_CONFIG_USER_FILE;
    } else {
        ret = access(RL_CONFIG_SYSTEM_FILE, R_OK);
        if (ret == 0) {
            config_file = RL_CONFIG_SYSTEM_FILE;
        } else {
            // no config file found
            rl_config_reset(config);
            return SUCCESS;
        }
    }

    // open config file
    FILE *file = fopen(config_file, "r");
    if (file == NULL) {
        rl_log(RL_LOG_ERROR, "failed to open configuration file '%s'",
               config_file);
        return ERROR;
    }

    // read values
    fread(config, sizeof(rl_config_t), 1, file);

    // close file
    fclose(file);

    // reset file comment, as it is not stored yet
    /// @todo drop when comment is stored
    config->file_comment = RL_CONFIG_COMMENT_DEFAULT;

    // check version
    if (config->config_version != RL_CONFIG_VERSION) {
        rl_log(RL_LOG_WARNING,
               "Old or invalid configuration file. Using default "
               "config as fall back.");
        rl_config_reset(config);
        return SUCCESS;
    }

    return SUCCESS;
}

int rl_config_write_default(rl_config_t const *const config) {
    // open config file
    FILE *file = fopen(RL_CONFIG_USER_FILE, "w");
    if (file == NULL) {
        rl_log(RL_LOG_ERROR, "failed to create configuration file");
        return ERROR;
    }
    // write values
    fwrite(config, sizeof(rl_config_t), 1, file);

    // close file
    fclose(file);
    return SUCCESS;
}

int rl_config_validate(rl_config_t const *const config) {
    // check individual arguments
    if (config->config_version != RL_CONFIG_VERSION) {
        rl_log(RL_LOG_ERROR, "invalid config version (%x, should be %x).",
               config->config_version, RL_CONFIG_VERSION);
        return ERROR;
    }

    // check supported sample rate (non-zero, natively supported or divisor of
    // lowest rate for aggregation)
    bool sample_rate_native =
        (config->sample_rate == 1000 || config->sample_rate == 2000 ||
         config->sample_rate == 4000 || config->sample_rate == 8000 ||
         config->sample_rate == 16000 || config->sample_rate == 32000 ||
         config->sample_rate == 64000);
    if (config->sample_rate == 0 ||
        (!sample_rate_native &&
         ((RL_SAMPLE_RATE_MIN % config->sample_rate) > 0))) {
        rl_log(RL_LOG_ERROR,
               "invalid sample rate (%u). Needs to be natively supported "
               "value, or valid divisor of %u.",
               config->sample_rate, RL_SAMPLE_RATE_MIN);
        return ERROR;
    }

    // check supported update rate (non-zero, divisor of the sample rate)
    if (config->update_rate == 0 ||
        ((config->sample_rate % config->update_rate) > 0)) {
        rl_log(RL_LOG_ERROR,
               "invalid update rate (%u). Needs to be a valid divisor of the "
               "sample rate (%u).",
               config->update_rate, config->sample_rate);
        return ERROR;
    }

    // check supported file size (either zero or at least minimum value)
    if (config->file_size > 0 && config->file_size < RL_CONFIG_FILE_SIZE_MIN) {
        rl_log(RL_LOG_ERROR, "invalid update rate. Needs to be a valid divisor "
                             "of the sample rate.");
        return ERROR;
    }

    // file comment allows only for printable or space characters
    if (!is_printable_string(config->file_comment)) {
        rl_log(RL_LOG_ERROR, "invalid character in file comment, supports only "
                             "printable and white space characters.");
        return ERROR;
    }

    // sample limit: accept any positive integer, or zero -> no check needed
    // .sample_limit = 0UL,

    // checking boolean values not required:
    // .background_enable = false,
    // .interactive_enable = false,
    // .channel_enable = RL_CONFIG_CHANNEL_ENABLE_DEFAULT,
    // .channel_force_range = RL_CONFIG_CHANNEL_FORCE_RANGE_DEFAULT,
    // .digital_enable = true,
    // .web_enable = true,
    // .calibration_ignore = false,
    // .ambient_enable = false,
    // .file_enable = true,

    // checking enum values not required:
    // .aggregation_mode = RL_AGGREGATION_MODE_DOWNSAMPLE,
    // .file_format = RL_FILE_FORMAT_RLD,

    // check incompatible/invalid combinations
    if (config->background_enable && config->interactive_enable) {
        rl_log(RL_LOG_ERROR,
               "enabling both background and interactive is unsupported.");
        return ERROR;
    }

    return SUCCESS;
}

pid_t rl_pid_get(void) {
    pid_t pid;
    FILE *file = fopen(RL_PID_FILE, "r");
    if (file == NULL) {
        return 0;
    }

    fscanf(file, "%d", &pid);
    fclose(file);

    return pid;
}

int rl_pid_set(pid_t pid) {
    FILE *file = fopen(RL_PID_FILE, "w");
    if (file == NULL) {
        rl_log(RL_LOG_ERROR, "Failed to create pid file");
        return ERROR;
    }

    fprintf(file, "%d\n", pid);
    fclose(file);

    return SUCCESS;
}

void rl_status_reset(rl_status_t *const status) {
    memcpy(status, &rl_status_default, sizeof(rl_status_t));
}

int rl_status_pub_init(void) {
    // open and bind zmq status publisher
    zmq_status_context = zmq_ctx_new();
    zmq_status_publisher = zmq_socket(zmq_status_context, ZMQ_PUB);
    int zmq_res = zmq_bind(zmq_status_publisher, RL_ZMQ_STATUS_SOCKET);
    if (zmq_res < 0) {
        rl_log(RL_LOG_ERROR,
               "failed binding zeromq status publisher; %d message: %s", errno,
               strerror(errno));
        return ERROR;
    }

    return SUCCESS;
}

int rl_status_pub_deinit(void) {
    // close and destroy zmq status publisher
    zmq_close(zmq_status_publisher);
    zmq_ctx_destroy(zmq_status_context);

    zmq_status_publisher = NULL;
    zmq_status_context = NULL;

    return SUCCESS;
}

int rl_status_shm_init(void) {
    // create shared memory
    int shm_id = shmget(RL_SHMEM_STATUS_KEY, sizeof(rl_status_t),
                        IPC_CREAT | RL_SHMEM_PERMISSIONS);
    if (shm_id == -1) {
        rl_log(RL_LOG_ERROR,
               "failed creating shared status memory; %d message: %s", errno,
               strerror(errno));
        return ERROR;
    }

    return SUCCESS;
}

int rl_status_shm_deinit(void) {
    // get ID and attach shared memory
    int shm_id =
        shmget(RL_SHMEM_STATUS_KEY, sizeof(rl_status_t), RL_SHMEM_PERMISSIONS);
    if (shm_id == -1) {
        rl_log(RL_LOG_ERROR,
               "failed getting shared memory id for removal; message: %s",
               errno, strerror(errno));
        return ERROR;
    }

    // mark shared memory for deletion
    int res = shmctl(shm_id, IPC_RMID, NULL);
    if (res == -1) {
        rl_log(RL_LOG_ERROR,
               "failed removing shared status memory; %d message: %s", errno,
               strerror(errno));
        return ERROR;
    }

    return SUCCESS;
}

int rl_status_read(rl_status_t *const status) {
    // get ID and attach shared memory
    int shm_id =
        shmget(RL_SHMEM_STATUS_KEY, sizeof(rl_status_t), RL_SHMEM_PERMISSIONS);
    if (shm_id == -1) {
        rl_log(RL_LOG_ERROR,
               "failed getting shared memory id for reading the "
               "status; message: %s",
               errno, strerror(errno));
        return ERROR;
    }

    rl_status_t const *const shm_status =
        (rl_status_t const *const)shmat(shm_id, NULL, 0);
    if (shm_status == (void *)-1) {
        rl_log(RL_LOG_ERROR,
               "failed mapping shared memory for reading the "
               "status; %d message: %s",
               errno, strerror(errno));
        return ERROR;
    }

    // copy status read from shared memory
    memcpy(status, shm_status, sizeof(rl_status_t));
    status->config = NULL;

    // detach shared memory
    int res = shmdt(shm_status);
    if (res < 0) {
        rl_log(
            RL_LOG_ERROR,
            "failed detaching shared memory after status read; %d message: %s",
            errno, strerror(errno));
        return ERROR;
    }

    return SUCCESS;
}

int rl_status_write(rl_status_t *const status) {
    // get ID and attach shared memory
    int shm_id =
        shmget(RL_SHMEM_STATUS_KEY, sizeof(rl_status_t), RL_SHMEM_PERMISSIONS);
    if (shm_id == -1) {
        rl_log(RL_LOG_ERROR,
               "failed getting shared memory id for writing the status; "
               "message: %s",
               errno, strerror(errno));
        return ERROR;
    }

    rl_status_t *const shm_status = (rl_status_t *const)shmat(shm_id, NULL, 0);
    if (shm_status == (void *)-1) {
        rl_log(RL_LOG_ERROR,
               "failed mapping shared memory for writing the status; %d "
               "message: %s",
               errno, strerror(errno));
        return ERROR;
    }

    // write status
    memcpy(shm_status, status, sizeof(rl_status_t));

    // detach shared memory
    int res = shmdt(shm_status);
    if (res < 0) {
        rl_log(
            RL_LOG_ERROR,
            "failed detaching shared memory after status write; %d message: %s",
            errno, strerror(errno));
        return ERROR;
    }

    // write status json to zmq socket if enabled
    if (zmq_status_publisher != NULL) {
        // update file system state
        int64_t disk_free = 0;
        int64_t disk_total = 0;
        if (status->config != NULL) {
            disk_free = fs_space_free(status->config->file_name);
            disk_total = fs_space_total(status->config->file_name);
        } else {
            disk_free = fs_space_free(RL_CONFIG_FILE_DIR_DEFAULT);
            disk_total = fs_space_total(RL_CONFIG_FILE_DIR_DEFAULT);
        }

        status->disk_free = disk_free;
        if (disk_total > 0) {
            status->disk_free_permille = (1000 * disk_free) / disk_total;
        } else {
            status->disk_free_permille = 0;
        }

        // get status as json string and publish to zeromq
        char const *const status_json = rl_status_get_json(status);
        int zmq_res =
            zmq_send(zmq_status_publisher, status_json, strlen(status_json), 0);
        if (zmq_res < 0) {
            rl_log(RL_LOG_ERROR, "failed publishing status; %d message: %s",
                   errno, strerror(errno));
            return ERROR;
        }
    }

    return SUCCESS;
}

void rl_status_print(rl_status_t const *const status) {
    print_config_line("Sampling", status->sampling ? "yes" : "no");
    print_config_line("Error", status->error ? "yes" : "no");
    print_config_line("Sample count", "%llu", status->sample_count);
    print_config_line("Buffer count", "%llu", status->buffer_count);
    print_config_line("Calibration time", "%llu", status->calibration_time);
    if (status->calibration_time > 0) {
        print_config_line("Calibration file", status->calibration_file);
    } else {
        print_config_line("Calibration file", "calibration ignored!");
    }
    print_config_line("Disk free", "%llu Bytes", status->disk_free);
    print_config_line("Disk free", "%u/1000", status->disk_free_permille);
    print_config_line("Disk use rate", "%u Bytes/s", status->disk_use_rate);
    print_config_line("Sensors found", "%u total", status->sensor_count);
    for (uint16_t i = 0; i < SENSOR_REGISTRY_SIZE; i++) {
        if (status->sensor_available[i]) {
            print_config_line("", SENSOR_REGISTRY[i].name);
        }
    }
}

void rl_status_print_json(rl_status_t const *const status) {
    char const *const status_json = rl_status_get_json(status);
    printf("%s", status_json);
}

char *rl_status_get_json(rl_status_t const *const status) {
    static char buffer[RL_JSON_BUFFER_SIZE];

    snprintf(buffer, RL_JSON_BUFFER_SIZE, "{ ");
    snprintfcat(buffer, RL_JSON_BUFFER_SIZE, "\"sampling\": %s, ",
                status->sampling ? "true" : "false");
    snprintfcat(buffer, RL_JSON_BUFFER_SIZE, "\"error\": %s, ",
                status->error ? "true" : "false");
    snprintfcat(buffer, RL_JSON_BUFFER_SIZE, "\"sample_count\": %llu, ",
                status->sample_count);
    snprintfcat(buffer, RL_JSON_BUFFER_SIZE, "\"buffer_count\": %llu, ",
                status->buffer_count);
    snprintfcat(buffer, RL_JSON_BUFFER_SIZE, "\"calibration_time\": %llu, ",
                status->calibration_time);
    if (status->calibration_time > 0) {
        snprintfcat(buffer, RL_JSON_BUFFER_SIZE,
                    "\"calibration_file\": \"%s\", ", status->calibration_file);
    } else {
        snprintfcat(buffer, RL_JSON_BUFFER_SIZE,
                    "\"calibration_file\": null, ");
    }
    snprintfcat(buffer, RL_JSON_BUFFER_SIZE, "\"disk_free_bytes\": %llu, ",
                status->disk_free);
    snprintfcat(buffer, RL_JSON_BUFFER_SIZE, "\"disk_free_permille\": %u, ",
                status->disk_free_permille);
    snprintfcat(buffer, RL_JSON_BUFFER_SIZE, "\"disk_use_rate\": %u, ",
                status->disk_use_rate);
    snprintfcat(buffer, RL_JSON_BUFFER_SIZE, "\"sensor_count\": %u, ",
                status->sensor_count);
    snprintfcat(buffer, RL_JSON_BUFFER_SIZE, "\"sensor_names\": [");
    for (uint16_t i = 0; i < SENSOR_REGISTRY_SIZE; i++) {
        if (i > 0) {
            snprintfcat(buffer, RL_JSON_BUFFER_SIZE, ", ");
        }
        if (status->sensor_available[i]) {
            snprintfcat(buffer, RL_JSON_BUFFER_SIZE, "\"%s\"",
                        SENSOR_REGISTRY[i].name);
        } else {
            snprintfcat(buffer, RL_JSON_BUFFER_SIZE, "null");
        }
    }
    snprintfcat(buffer, RL_JSON_BUFFER_SIZE, "]");
    if (status->config != NULL) {
        char const *config_json = rl_config_get_json(status->config);
        snprintfcat(buffer, RL_JSON_BUFFER_SIZE, ", \"config\": %s",
                    config_json);
    }
    snprintfcat(buffer, RL_JSON_BUFFER_SIZE, " }");

    return buffer;
}

static void print_config_line(char const *description, char const *format,
                              ...) {
    va_list args;
    va_start(args, format);
    printf("  %24s - ", description);
    vprintf(format, args);
    printf("\n");
    va_end(args);
}
