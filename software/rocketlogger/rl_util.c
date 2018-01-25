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
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "rl_version.h"

#include "rl_util.h"

/**
 * Print RocketLogger configuration on command line
 * @param conf Pointer to {@link rl_conf} configuration
 */
void rl_print_config(struct rl_conf* conf) {

    char file_format_names[3][8] = {"no file", "csv", "binary"};
    char data_aggregation_names[3][10] = {"none", "downsample", "average"};

    if (conf->sample_rate >= KSPS) {
        printf("  Sampling rate:    %dkSps\n", conf->sample_rate / KSPS);
    } else {
        printf("  Sampling rate:    %dSps\n", conf->sample_rate);
    }
    printf("  Data aggregation: %s\n",
           data_aggregation_names[conf->aggregation]);

    printf("  Update rate:      %dHz\n", conf->update_rate);
    if (conf->enable_web_server == 1) {
        printf("  Webserver:        enabled\n");
    } else {
        printf("  Webserver:        disabled\n");
    }
    if (conf->digital_inputs == 1) {
        printf("  Digital inputs:   enabled\n");
    } else {
        printf("  Digital inputs:   disabled\n");
    }
    printf("  File format:      %s\n", file_format_names[conf->file_format]);
    if (conf->file_format != NO_FILE) {
        printf("  File name:        %s\n", conf->file_name);
    }
    if (conf->max_file_size != 0) {
        printf("  Max file size:    %lluMB\n",
               conf->max_file_size / (uint64_t)1e6);
    }
    if (conf->calibration == CAL_IGNORE) {
        printf("  Calibration:      ignored\n");
    }
    printf("  Channels:         ");
    for (int i = 0; i < NUM_CHANNELS; i++) {
        if (conf->channels[i] == CHANNEL_ENABLED) {
            printf("%d,", i);
        }
    }
    printf("\n");
    if (conf->force_high_channels[0] == CHANNEL_ENABLED ||
        conf->force_high_channels[1] == CHANNEL_ENABLED) {
        printf("  Forced channels:  ");
        for (int i = 0; i < NUM_I_CHANNELS; i++) {
            if (conf->force_high_channels[i] == CHANNEL_ENABLED) {
                printf("%d,", i + 1);
            }
        }
        printf("\n");
    }
    if (conf->sample_limit == 0) {
        printf("  Sample limit:     no limit\n");
    } else {
        printf("  Sample limit:     %d\n", conf->sample_limit);
    }
}

/**
 * Print RocketLogger status on command line
 * @param status Pointer to {@link rl_status} status
 */
void rl_print_status(struct rl_status* status) {

    if (status->state == RL_OFF) {
        printf("\nRocketLogger IDLE\n\n");
    } else {
        printf("\nRocketLogger Status: RUNNING\n");
        rl_print_config(&(status->conf));
        printf("  Samples taken:    %llu\n", status->samples_taken);
        time_t time = (time_t)status->calibration_time;
        if (time > 0) {
            printf("  Calibration time: %s\n", ctime(&time));
        } else {
            printf("  Calibration time: No calibration file found\n");
        }
        printf("\n");
    }
}

/**
 * Print the RocketLogger software version on the command line.
 */
void rl_print_version(void) {
    printf("RocketLogger Software Stack version " RL_VERSION "\n");
}

/**
 * Get the the RocketLogger software version number string.
 * @return The RocketLogger Sofware Stack's version number string.
 */
char* rl_get_version(void) { return RL_VERSION; }

// argument parsing
/**
 * Get RocketLogger mode of provided command line argument
 * @param mode Pointer to argument string to parse
 * @return provided mode
 */
rl_mode get_mode(char* mode) {
    if (strcmp(mode, "sample") == 0) {
        return LIMIT;
    } else if (strcmp(mode, "cont") == 0) {
        return CONTINUOUS;
    } else if (strcmp(mode, "meter") == 0) {
        return METER;
    } else if (strcmp(mode, "status") == 0) {
        return STATUS;
    } else if (strcmp(mode, "stop") == 0) {
        return STOPPED;
    } else if (strcmp(mode, "set") == 0) {
        return SET_DEFAULT;
    } else if (strcmp(mode, "conf") == 0) {
        return PRINT_DEFAULT;
    } else if (strcmp(mode, "version") == 0 || strcmp(mode, "--version") == 0) {
        return PRINT_VERSION;
    } else if (strcmp(mode, "help") == 0 || strcmp(mode, "h") == 0 ||
               strcmp(mode, "-h") == 0 || strcmp(mode, "--help") == 0) {
        return HELP;
    }

    return NO_MODE;
}

/**
 * Get RocketLogger option of provided command line argument
 * @param option Pointer to argument string to parse
 * @return provided option
 */
rl_option get_option(char* option) {
    if (strcmp(option, "f") == 0) {
        return FILE_NAME;
    } else if (strcmp(option, "r") == 0) {
        return SAMPLE_RATE;
    } else if (strcmp(option, "u") == 0) {
        return UPDATE_RATE;
    } else if (strcmp(option, "ch") == 0) {
        return CHANNEL;
    } else if (strcmp(option, "fhr") == 0) {
        return FHR;
    } else if (strcmp(option, "w") == 0) {
        return WEB;
    } else if (strcmp(option, "d") == 0) {
        return DIGITAL_INPUTS;
    } else if (strcmp(option, "a") == 0) {
        return AMBIENT;
    } else if (strcmp(option, "g") == 0) {
        return AGGREGATION;
    } else if (strcmp(option, "s") == 0) {
        return DEF_CONF;
    } else if (strcmp(option, "c") == 0) {
        return CALIBRATION;
    } else if (strcmp(option, "C") == 0) {
        return COMMENT;
    } else if (strcmp(option, "format") == 0) {
        return FILE_FORMAT;
    } else if (strcmp(option, "size") == 0) {
        return FILE_SIZE;
    }

    return NO_OPTION;
}

/**
 * Parse command line argument to selected channels
 * @param channels Channel array to write
 * @param value Pointer to argument string to parse
 * @return {@link SUCCESS} on success, {@link FAILURE} otherwise
 */
int parse_channels(int channels[], char* value) {

    // check first channel number
    if (isdigit(value[0]) && atoi(value) >= 0 && atoi(value) <= 9) {

        // reset default channel selection
        memset(channels, 0, sizeof(int) * NUM_CHANNELS);
        channels[atoi(value)] = 1;

    } else if (strcmp(value, "all") == 0) {
        // all channels
        int i;
        for (i = 0; i < NUM_CHANNELS; i++) {
            channels[i] = CHANNEL_ENABLED;
        }
    } else {
        rl_log(ERROR, "wrong channel number");
        return FAILURE;
    }

    // loop
    int j;
    for (j = 1; j < 2 * (NUM_CHANNELS - 1) && value[j] == ','; j = j + 2) {

        // check channel number
        char* c = &value[j + 1];
        if (isdigit(c[0]) && atoi(c) >= 0 && atoi(c) < NUM_CHANNELS) {
            channels[atoi(c)] = 1;
        } else {
            rl_log(ERROR, "wrong channel number");
            return FAILURE;
        }
    }

    return SUCCESS;
}

/**
 * Parse input arguments of RocketLogger CLI
 * @param argc Number of input arguments
 * @param argv Input argument string
 * @param conf Pointer to {@link rl_conf} configuration to write
 * @param set_as_default Is set to 1, if configuration should be set as default
 * @param file_comment Comment to write into the file header
 * @return {@link SUCCESS} on success, {@link FAILURE} otherwise
 */
int parse_args(int argc, char* argv[], struct rl_conf* conf,
               int* set_as_default, char** file_comment) {

    int i; // argument count variable
    int no_file = 0;
    *set_as_default = 0;
    *file_comment = NULL;

    // need at least 2 arguments
    if (argc < 2) {
        rl_log(ERROR, "no mode");
        return FAILURE;
    }

    // MODE
    conf->mode = get_mode(argv[1]);
    if (conf->mode == NO_MODE) {
        rl_log(ERROR, "wrong mode");
        return FAILURE;
    }

    if (conf->mode == LIMIT) {
        // parse sample limit
        if (argc > 2 && isdigit(argv[2][0]) && atoi(argv[2]) > 0) {
            conf->sample_limit = atoi(argv[2]);
            i = 3;
        } else {
            rl_log(ERROR, "no possible sample limit");
            return FAILURE;
        }
    } else {
        i = 2;
    }

    // disable webserver as default for non-continuous mode
    if (conf->mode == STATUS || conf->mode == LIMIT) {
        conf->enable_web_server = 0;
    }

    // stop parsing for some modes
    if (conf->mode == STOPPED || conf->mode == PRINT_DEFAULT ||
        conf->mode == HELP) {
        return SUCCESS;
    }

    // reset default configuration
    if (conf->mode == SET_DEFAULT && isdigit(argv[i][0]) &&
        atoi(argv[i]) == 0) {
        reset_config(conf);
        conf->mode = SET_DEFAULT;
        return SUCCESS;
    }

    // OPTIONS
    for (; i < argc; i++) {
        if (argv[i][0] == '-') {
            switch (get_option(&argv[i][1])) {

            case FILE_NAME:
                if (argc > ++i) {
                    if (isdigit(argv[i][0]) &&
                        atoi(argv[i]) == 0) { // no file write
                        no_file = 1;
                        conf->file_format = NO_FILE;
                    } else if (strlen(argv[i]) >= MAX_PATH_LENGTH) {
                        rl_log(ERROR, "file name too long");
                        return FAILURE;
                    } else {
                        strcpy(conf->file_name, argv[i]);
                    }
                } else {
                    rl_log(ERROR, "no file name given");
                    return FAILURE;
                }
                break;

            case SAMPLE_RATE:
                if (argc > ++i && isdigit(argv[i][0])) {
                    if (argv[i][strlen(argv[i]) - 1] == 'k') {
                        conf->sample_rate = atoi(argv[i]) * KSPS;
                    } else {
                        conf->sample_rate = atoi(argv[i]);
                    }
                    if (check_sample_rate(conf->sample_rate) ==
                        FAILURE) { // check if rate allowed
                        rl_log(ERROR, "wrong sampling rate");
                        return FAILURE;
                    }
                } else {
                    rl_log(ERROR, "no sampling rate");
                    return FAILURE;
                }
                break;

            case UPDATE_RATE:
                if (argc > ++i && isdigit(argv[i][0])) {
                    conf->update_rate = atoi(argv[i]);
                    if (check_update_rate(conf->update_rate) ==
                        FAILURE) { // check if rate allowed
                        rl_log(ERROR, "wrong update rate");
                        return FAILURE;
                    }
                } else {
                    rl_log(ERROR, "no update rate");
                    return FAILURE;
                }
                break;

            case CHANNEL:
                if (argc > ++i) {
                    if (parse_channels(conf->channels, argv[i]) == FAILURE) {
                        return FAILURE;
                    }
                } else {
                    rl_log(ERROR, "no channel number");
                    return FAILURE;
                }
                break;

            case FHR:
                if (argc > ++i) {

                    // check first number
                    char* c = argv[i];
                    if (isdigit(c[0]) && atoi(c) < 3 && atoi(c) >= 0) {

                        // reset default forced channel selection
                        memset(conf->force_high_channels, 0,
                               sizeof(conf->force_high_channels));
                        if (atoi(c) > 0) {
                            conf->force_high_channels[atoi(c) - 1] =
                                CHANNEL_ENABLED;
                        }
                    } else {
                        rl_log(ERROR, "wrong force-channel number");
                        return FAILURE;
                    }
                    // check second number
                    if (argv[i][1] == ',') {

                        char* c = &argv[i][2];
                        if (atoi(c) < 3 && atoi(c) > 0) {
                            conf->force_high_channels[atoi(c) - 1] =
                                CHANNEL_ENABLED;
                        } else {
                            rl_log(ERROR, "wrong force-channel number");
                            return FAILURE;
                        }
                    }
                } else {
                    rl_log(ERROR, "no force channel number");
                    return FAILURE;
                }
                break;

            case WEB:
                if (argc > i + 1 && isdigit(argv[i + 1][0]) &&
                    atoi(argv[i + 1]) == 0) {
                    i++;
                    conf->enable_web_server = 0;
                } else {
                    conf->enable_web_server = 1;
                }
                break;

            case DIGITAL_INPUTS:
                if (argc > i + 1 && isdigit(argv[i + 1][0]) &&
                    atoi(argv[i + 1]) == 0) {
                    i++;
                    conf->digital_inputs = DIGITAL_INPUTS_DISABLED;
                } else {
                    conf->digital_inputs = DIGITAL_INPUTS_ENABLED;
                }
                break;

            case AMBIENT:
                if (argc > i + 1 && isdigit(argv[i + 1][0]) &&
                    atoi(argv[i + 1]) == 0) {
                    i++;
                    conf->ambient.enabled = AMBIENT_DISABLED;
                } else {
                    conf->ambient.enabled = AMBIENT_ENABLED;
                }
                break;

            case AGGREGATION:
                if (argc > ++i) {
                    if (isdigit(argv[i][0]) && atoi(argv[i]) == 0) {
                        conf->aggregation = AGGREGATE_NONE;
                    } else if (no_file == 0) {
                        // ignore format, when no file is written
                        if (strcmp(argv[i], "none") == 0) {
                            conf->aggregation = AGGREGATE_NONE;
                        } else if (strcmp(argv[i], "average") == 0) {
                            conf->aggregation = AGGREGATE_AVERAGE;
                        } else if (strcmp(argv[i], "downsample") == 0) {
                            conf->aggregation = AGGREGATE_DOWNSAMPLE;
                        } else {
                            rl_log(ERROR, "wrong file format");
                            return FAILURE;
                        }
                    } else {
                        rl_log(INFO, "aggregation type ignored");
                    }
                } else {
                    rl_log(ERROR, "no aggregation type");
                    return FAILURE;
                }
                break;

            case DEF_CONF:
                *set_as_default = 1;
                break;

            case CALIBRATION:
                if (argc > i + 1 && isdigit(argv[i + 1][0]) &&
                    atoi(argv[i + 1]) == 0) {
                    i++;
                    conf->calibration = CAL_IGNORE;
                } else {
                    conf->calibration = CAL_USE;
                }
                break;

            case COMMENT:
                if (argc > ++i) {
                    if (no_file == 0) {
                        *file_comment = argv[i];
                    } else {
                        rl_log(INFO, "comment ignored");
                    }
                } else {
                    rl_log(ERROR, "no comment given");
                    return FAILURE;
                }
                break;

            case FILE_FORMAT:
                if (argc > ++i) {
                    // ignore format, when no file is written
                    if (no_file == 0) {
                        if (strcmp(argv[i], "csv") == 0) {
                            conf->file_format = CSV;
                        } else if (strcmp(argv[i], "bin") == 0) {
                            conf->file_format = BIN;
                        } else {
                            rl_log(ERROR, "wrong file format");
                            return FAILURE;
                        }
                    } else {
                        rl_log(INFO, "file format ignored");
                    }
                } else {
                    rl_log(ERROR, "no file format");
                    return FAILURE;
                }
                break;

            case FILE_SIZE:
                if (argc > ++i && isdigit(argv[i][0])) {
                    conf->max_file_size = atoll(argv[i]);
                    switch (argv[i][strlen(argv[i]) - 1]) {
                    case 'k':
                    case 'K':
                        conf->max_file_size *= 1000;
                        break;
                    case 'm':
                    case 'M':
                        conf->max_file_size *= 1000000;
                        break;
                    case 'g':
                    case 'G':
                        conf->max_file_size *= 1000000000;
                        break;
                    default:
                        break;
                    }
                    // check file size
                    if (conf->max_file_size != 0 &&
                        conf->max_file_size < 5000000) {
                        rl_log(ERROR, "too small file size (min: 5m)");
                        return FAILURE;
                    }
                } else {
                    rl_log(ERROR, "no file size");
                    return FAILURE;
                }
                break;

            case NO_OPTION:
                rl_log(ERROR, "wrong option");
                return FAILURE;

            default:
                rl_log(ERROR, "wrong option");
                return FAILURE;
            }
        } else {
            rl_log(ERROR, "use -[option] [value]");
            return FAILURE;
        }
    }

    // ambient file name
    if (conf->file_format != NO_FILE &&
        conf->ambient.enabled == AMBIENT_ENABLED) {
        ambient_set_file_name(conf);
    }

    return SUCCESS;
}

// help
/**
 * Print help for RocketLogger CLI on command line
 */
void print_usage(void) {
    printf("\n");
    printf("Usage:\n");
    printf("  rocketlogger mode -[option value]\n");
    printf("\n");
    printf("  Modes:\n");
    printf("    sample number      Acquires number of samples.\n");
    printf("    cont               Continuously acquires samples.\n");
    printf("    meter              Starts RocketLogger Meter.\n");
    printf("    status             Get status of RocketLogger.\n");
    printf("    stop               Stops RocketLogger.\n");
    printf("    set                Set default configuration of RocketLogger "
           "(use normal options).\n");
    printf("                         Use 'set 0' to reset the default "
           "configuration.\n");
    printf("    conf               Print default configuration of "
           "RocketLogger.\n");
    printf("\n");
    printf("  Options:\n");
    printf("    -r rate            Acquisition rate selection. Supported "
           "rates:\n");
    printf(
        "                         1, 10, 100, 1k, 2k, 4k, 8k, 16k, 32k, 64k\n");
    printf("    -u update_rate     Data update rate selection. Supported "
           "rates:\n");
    printf("                         1, 2, 5, 10 (in Hz)\n");
    printf("    -ch ch1,ch2,...    Channel selection.\n");
    printf("                       Possible channels ('-ch all' to enable "
           "all):\n");
    printf("                         0: I1H\t\t4: I2H\n");
    printf("                         1: I1L\t\t5: I2L\n");
    printf("                         2: V1 \t\t6: V3\n");
    printf("                         3: V2 \t\t7: V4\n");
    printf("    -fhr ch1,ch2       Force high-range.\n");
    printf("                         0: no channel, 1: I1, 2: I2\n");
    printf("    -c                 Use calibration, if existing.\n");
    printf("                         '-c 0' to ignore calibration.\n");
    printf("    -f file            Stores data to specified file.\n");
    printf("                         '-f 0' will disable file storing.\n");
    printf("    -d                 Log digital inputs.\n");
    printf("                         '-d 0' to disable digital input "
           "logging.\n");
    printf("    -a                 Log ambient sensors, if available.\n");
    printf("                         '-a 0' to disable ambient sensor "
           "logging.\n");
    printf("    -g                 Data aggregation mode for low sample "
           "rates.\n");
    printf("                         Existing modes: 'average', "
           "'downsample'.\n");
    printf("                         '-g 0' to disable aggregation/low sample "
           "rates.\n");
    printf("    -format format     Select file format: csv, bin.\n");
    printf("    -size   file_size  Select max file size (k, m, g can be "
           "used).\n");
    printf("    -C comment         Comment stored in file header. Comment is "
           "ignored\n");
    printf("                         if file saving is disabled.\n");
    printf("    -w                 Enable webserver plotting.\n");
    printf("                         Use '-w 0' to disable webserver "
           "plotting.\n");
    printf("    -s                 Set configuration as default (all except "
           "comment).\n");
    printf("\n");
    printf("  Help/Info:\n");
    printf("    help, --help       Display this help message.\n");
    printf("    version, --version Display the RocketLogger software "
           "version.\n");
    printf("\n");
}

// configuration handling
/**
 * Print provided configuration to command line
 * @param conf Pointer to {@link rl_conf} configuration
 */
void print_config(struct rl_conf* conf) {
    printf("\nRocketLogger Configuration:\n");
    rl_print_config(conf);
    printf("\n");
}

/**
 * Reset configuration to standard values
 * @param conf Pointer to {@link rl_conf} configuration
 */
void reset_config(struct rl_conf* conf) {
    conf->version = RL_CONF_VERSION;
    conf->mode = CONTINUOUS;
    conf->sample_rate = 1000;
    conf->aggregation = AGGREGATE_DOWNSAMPLE;
    conf->update_rate = 1;
    conf->sample_limit = 0;
    conf->digital_inputs = DIGITAL_INPUTS_ENABLED;
    conf->enable_web_server = 1;
    conf->calibration = CAL_USE;
    conf->file_format = BIN;
    conf->max_file_size = 0;

    strcpy(conf->file_name, "/var/www/data/data.rld");

    int i;
    for (i = 0; i < NUM_CHANNELS; i++) {
        conf->channels[i] = CHANNEL_ENABLED;
    }
    memset(conf->force_high_channels, 0, sizeof(conf->force_high_channels));

    conf->ambient.enabled = AMBIENT_DISABLED;
    strcpy(conf->ambient.file_name, "/var/www/data/data-ambient.rld");
}

/**
 * Read default configuration from file
 * @param conf Pointer to {@link rl_conf} configuration
 * @return {@link SUCCESS} on success, {@link FAILURE} otherwise
 */
int read_default_config(struct rl_conf* conf) {

    // check if config file existing
    if (open(DEFAULT_CONFIG, O_RDWR) <= 0) {
        reset_config(conf);
        return UNDEFINED;
    }

    // open config file
    FILE* file = fopen(DEFAULT_CONFIG, "r");
    if (file == NULL) {
        rl_log(ERROR, "failed to open configuration file");
        return FAILURE;
    }
    // read values
    fread(conf, sizeof(struct rl_conf), 1, file);

    // close file
    fclose(file);

    // check version
    if (conf->version != RL_CONF_VERSION) {
        rl_log(WARNING, "Old or invalid configration file. Using default "
                        "config as fallback.");
        reset_config(conf);
        return UNDEFINED;
    }

    // reset mode
    conf->mode = CONTINUOUS;

    return SUCCESS;
}

/**
 * Write provided configuration as default to file
 * @param conf Pointer to {@link rl_conf} configuration to write
 * @return {@link SUCCESS} on success, {@link FAILURE} otherwise
 */
int write_default_config(struct rl_conf* conf) {

    // open config file
    FILE* file = fopen(DEFAULT_CONFIG, "w");
    if (file == NULL) {
        rl_log(ERROR, "failed to create configuration file");
        return FAILURE;
    }
    // write values
    fwrite(conf, sizeof(struct rl_conf), 1, file);

    // close file
    fclose(file);
    return SUCCESS;
}
