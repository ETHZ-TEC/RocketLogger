/**
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

#include <unistd.h>

#include "log.h"

#include "rl.h"

/**
 * RocketLogger reset configuration definition.
 */
static const rl_config_t rl_config_default = {
    .config_version = RL_CONFIG_VERSION,
    .sampling_mode = RL_SAMPLING_MODE_CONTINUOUS,
    .sample_limit = 0UL,
    .sample_rate = 1000UL,
    .update_rate = 1UL,
    .channel_enable = RL_CONFIG_DEFAULT_CHANNEL_ENABLE,
    .channel_force_range = RL_CONFIG_DEFAULT_CHANNEL_FORCE_RANGE,
    .aggregation_mode = RL_AGGREGATION_MODE_DOWNSAMPLE,
    .digital_enable = true,
    .web_enable = true,
    .calibration_ignore = false,
    .ambient_enable = false,
    .file_enable = true,
    .file_name = RL_CONFIG_DEFAULT_FILE,
    .file_format = RL_FILE_FORMAT_RLD,
    .file_size = (1000UL * 1000UL * 1000UL),
    .file_comment = RL_CONFIG_DEFAULT_COMMENT,
};

/**
 * RocketLogger reset status definition.
 */
static const rl_status_t rl_status_default = {
    .sampling = false,
    .error = false,
    .sample_count = 0,
    .buffer_count = 0,
    .calibration_time = 0,
    .disk_free = 0,
    .disk_free_permille = 0,
    .disk_use_rate = 0,
    .sensor_count = 0,
    .sensor_index = {-1},
    .config = (rl_config_t *)NULL,
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
rl_status_t status = rl_status_default;

/**
 * Print a configuration setting line with formated string value.
 *
 * @param description Configuration description
 * @param value Configuration value formatting string passed to fprintf()
 * @param ... Variables used to format value string
 */
static void print_config_line(char const *description, char const *value, ...);

void rl_config_print(rl_config_t const *const config) {
    // sampling mode and limit
    switch (config->sampling_mode) {
    case RL_SAMPLING_MODE_FINITE:
        print_config_line("Sampling mode", "finite sampling mode");
        break;
    case RL_SAMPLING_MODE_CONTINUOUS:
        print_config_line("Sampling mode", "continuous mode");
        break;
    case RL_SAMPLING_MODE_METER:
        print_config_line("Sampling mode", "console meter mode");
        break;
    default:
        print_config_line("Sampling mode", "undefined");
        break;
    }
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
    printf("rocketlogger ");

    // sampling mode and limit
    switch (config->sampling_mode) {
    case RL_SAMPLING_MODE_FINITE:
        printf("sample --samples=%llu ", config->sample_limit);
        break;
    default:
    case RL_SAMPLING_MODE_CONTINUOUS:
        printf("cont ");
        break;
    case RL_SAMPLING_MODE_METER:
        printf("meter ");
        break;
    }

    // sample rate and aggregation
    printf("--rate=%u ", config->sample_rate);
    printf("--update=%u ", config->update_rate);

    // channels
    printf("--channel=");
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
        printf("--aggregate=average ");
        break;
    case RL_AGGREGATION_MODE_DOWNSAMPLE:
    default:
        printf("--aggregate=downsample ");
        break;
    }

    // arguments
    printf("--ambient=%s ", config->ambient_enable ? "true" : "false");
    printf("--digital=%s ", config->digital_enable ? "true" : "false");
    printf("--web=%s ", config->web_enable ? "true" : "false");

    if (config->calibration_ignore) {
        printf("--calibration");
    }

    // file
    if (config->file_enable) {
        printf("--output=%s ", config->file_name);
        printf("--format=%s ",
               (config->file_format == RL_FILE_FORMAT_RLD) ? "rld" : "csv");
        printf("--size=%llu ", config->file_size);
        printf("--comment='%s'\n", config->file_comment);
    } else {
        printf("--output=0\n");
    }
}

void rl_config_print_json(rl_config_t const *const config) {
    printf("{");
    printf("ambient_enable: %s, ", config->ambient_enable ? "true" : "false");
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
    config->file_comment = RL_CONFIG_DEFAULT_COMMENT;

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
    /// @todo parameters individually

    // check mandatory arguments
    switch (config->sampling_mode) {
    case RL_SAMPLING_MODE_CONTINUOUS:
        /// @todo check mandatory parameters
        break;
    case RL_SAMPLING_MODE_FINITE:
        /// @todo check mandatory parameters
        break;
    case RL_SAMPLING_MODE_METER:
        /// @todo check mandatory parameters
        break;
    default:
        errno = EINVAL;
        return ERROR;
    }

    return SUCCESS;
}

void rl_status_reset(rl_status_t *const status) {
    memcpy(status, &rl_status_default, sizeof(rl_status_t));
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