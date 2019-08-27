/*
 * Copyright (c) 2016-2018, Swiss Federal Institute of Technology (ETH Zurich)
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

#include "ads131e0x.h"
#include "calibration.h"
#include "log.h"
#include "sensor/sensor.h"

#include "rl.h"

/**
 * RocketLogger reset configuration definition.
 */
const rl_config_t rl_config_default = {
    .config_version = RL_CONFIG_VERSION,
    .background_enable = false,
    .interactive_enable = false,
    .sample_limit = 0,
    .sample_rate = ADS131E0X_RATE_MIN,
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
};

/// RocketLogger channel names
const char *RL_CHANNEL_NAMES[RL_CHANNEL_COUNT] = {"V1",  "V2",  "V3",  "V4",
                                                  "I1L", "I1H", "I2L", "I2H"};

/// RocketLogger force range channel names
const char *RL_CHANNEL_FORCE_NAMES[RL_CHANNEL_SWITCHED_COUNT] = {"I1H", "I2H"};

/// RocketLogger digital channel names
const char *RL_CHANNEL_DIGITAL_NAMES[RL_CHANNEL_DIGITAL_COUNT] = {
    "DI1", "DI2", "DI3", "DI4", "DI5", "DI6"};

/// RocketLogger valid channel names
const char *RL_CHANNEL_VALID_NAMES[RL_CHANNEL_SWITCHED_COUNT] = {"I1L_valid",
                                                                 "I2L_valid"};

/// Global RocketLogger status variable.
rl_status_t rl_status = rl_status_default;

/**
 * Print a configuration setting line with formated string value.
 *
 * @param description Configuration description
 * @param value Configuration value formatting string passed to fprintf()
 * @param ... Variables used to format value string
 */
static void print_config_line(char const *description, char const *value, ...);

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
        "", "I1L: %s  I1H: %s  I2L: %s  I2H: %s",
        config->channel_enable[RL_CONFIG_CHANNEL_I1L] ? "on " : "off",
        config->channel_enable[RL_CONFIG_CHANNEL_I1H] ? "on " : "off",
        config->channel_enable[RL_CONFIG_CHANNEL_I2L] ? "on " : "off",
        config->channel_enable[RL_CONFIG_CHANNEL_I2H] ? "on " : "off");
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
    printf("{");
    printf("ambient_enable: %s, ", config->ambient_enable ? "true" : "false");
    printf("background_enable: %s, ",
           config->background_enable ? "true" : "false");
    printf("interactive_enable: %s, ",
           config->interactive_enable ? "true " : "false");

    printf("calibration_ignore: %s, ",
           config->calibration_ignore ? "true" : "false");
    printf("channel_enable: [");
    for (int i = 0; i < RL_CHANNEL_COUNT; i++) {
        if (config->channel_enable[i]) {
            printf("\"%s\", ", RL_CHANNEL_NAMES[i]);
        }
    }
    printf("], ");
    printf("channel_force_range: [");
    for (int i = 0; i < RL_CHANNEL_SWITCHED_COUNT; i++) {
        if (config->channel_force_range[i]) {
            printf("\"%s\", ", RL_CHANNEL_FORCE_NAMES[i]);
        }
    }
    printf("], ");
    printf("digital_enable: %s, ", config->digital_enable ? "true" : "false");
    if (config->file_enable == false) {
        printf("file: null, ");
    } else {
        printf("file: { ");
        printf("comment: \"%s\", ", config->file_comment);
        printf("filename: \"%s\", ", config->file_name);
        switch (config->file_format) {
        case RL_FILE_FORMAT_RLD:
            printf("format: \"rld\", ");
            break;
        case RL_FILE_FORMAT_CSV:
            printf("format: \"csv\", ");
            break;
        default:
            printf("format: null, ");
            break;
        }
        printf("size: %llu, ", config->file_size);
        printf("}, ");
    }
    printf("sample_limit: %llu, ", config->sample_limit);
    printf("sample_rate: %u, ", config->sample_rate);
    printf("update_rate: %u, ", config->update_rate);
    printf("web_enable: %s, ", config->web_enable ? "true" : "false");
    printf("}");
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
         ((ADS131E0X_RATE_MIN % config->sample_rate) > 0))) {
        rl_log(RL_LOG_ERROR, "invalid sample rate (%u). Needs to be natively "
                             "supported value, or valid divisor of %u.",
               config->sample_rate, ADS131E0X_RATE_MIN);
        return ERROR;
    }

    // check supported update rate (non-zero, divisor of the sample rate)
    if (config->update_rate == 0 ||
        ((config->sample_rate % config->update_rate) > 0)) {
        rl_log(RL_LOG_ERROR, "invalid update rate (%u). Needs to be a valid "
                             "divisor of the sample rate (%u).",
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

    fprintf(file, "%d", pid);
    fclose(file);

    return SUCCESS;
}

void rl_status_reset(rl_status_t *const status) {
    memcpy(status, &rl_status_default, sizeof(rl_status_t));
}

int rl_status_read(rl_status_t *const status) {
    // map shared memory
    int shm_id =
        shmget(SHMEM_STATUS_KEY, sizeof(rl_status_t), SHMEM_PERMISSIONS);
    if (shm_id == -1) {
        return ERROR;
    }

    rl_status_t const *const shm_status =
        (rl_status_t const *const)shmat(shm_id, NULL, 0);
    if (shm_status == (void *)-1) {
        rl_log(RL_LOG_ERROR,
               "failed to map shared status memory; %d message: %s", errno,
               strerror(errno));
        return ERROR;
    }

    // copy status read from shared memory
    memcpy(status, shm_status, sizeof(rl_status_t));

    // unmap shared memory
    shmdt(shm_status);

    return SUCCESS;
}

int rl_status_write(rl_status_t const *const status) {
    // map shared memory
    int shm_id = shmget(SHMEM_STATUS_KEY, sizeof(rl_status_t),
                        IPC_CREAT | SHMEM_PERMISSIONS);
    if (shm_id == -1) {
        rl_log(RL_LOG_ERROR,
               "In write_status: failed to get shared status memory id; "
               "%d message: %s",
               errno, strerror(errno));
        return ERROR;
    }

    rl_status_t *const shm_status = (rl_status_t * const)shmat(shm_id, NULL, 0);
    if (shm_status == (void *)-1) {
        rl_log(RL_LOG_ERROR,
               "In write_status: failed to map shared status memory; %d "
               "message: %s",
               errno, strerror(errno));
        return ERROR;
    }

    // write status
    memcpy(shm_status, status, sizeof(rl_status_t));

    // unmap shared memory
    shmdt(shm_status);

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
    print_config_line("Disk use rate", "%u Bytes/min", status->disk_use_rate);
    print_config_line("Sensors found", "%u total", status->sensor_count);
    for (uint16_t i = 0; i < status->sensor_count; i++) {
        if (status->sensor_available[i]) {
            print_config_line("", SENSOR_REGISTRY[i].name);
        } else {
            print_config_line("", "unknown");
        }
    }
}

void rl_status_print_json(rl_status_t const *const status) {
    printf("{");
    printf("sampling: %s, ", status->sampling ? "true" : "false");
    printf("error: %s, ", status->error ? "true" : "false");
    printf("sample_count: %llu, ", status->sample_count);
    printf("buffer_count: %llu, ", status->buffer_count);
    printf("calibration_time: %llu, ", status->calibration_time);
    if (status->calibration_time > 0) {
        printf("calibration_file: \"%s\", ", status->calibration_file);
    } else {
        printf("calibration_file: null, ");
    }
    printf("disk_free: %llu, ", status->disk_free);
    printf("disk_free_permille: %u, ", status->disk_free_permille);
    printf("disk_use_rate: %u, ", status->disk_use_rate);
    printf("sensor_count: %u, ", status->sensor_count);
    printf("sensor_names: [");
    for (uint16_t i = 0; i < status->sensor_count; i++) {
        if (status->sensor_available[i]) {
            printf("\"%s\", ", SENSOR_REGISTRY[i].name);
        } else {
            printf("null, ");
        }
    }
    printf("], ");
    printf("}");
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
