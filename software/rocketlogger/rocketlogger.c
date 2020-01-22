/**
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
 * ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <argp.h>
#include <ctype.h>
#include <error.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "rl.h"
#include "rl_lib.h"
#include "version.h"

/**
 * Number of arguments to parse
 */
#define ARGP_ARGUMENTS_COUNT 1

#define OPT_FILE_SIZE 1

#define OPT_SAMPLES_COUNT 2

#define OPT_SET_DEFAULT 3

#define OPT_RESET_DEFAULT 4

#define OPT_CALIBRATION 5

#define OPT_JSON 6

#define OPT_CLI 7

/**
 * Program version output for GNU standard command line format compliance.
 */
const char *argp_program_version = "RocketLogger Characterization CLI";

/**
 * Bug report address output for GNU standard command line format compliance.
 */
const char *argp_program_bug_address = "<https://rocketlogger.ethz.ch/>";

/**
 * The generic program description, printed as head for the help command
 */
static char doc[] =
    "RocketLogger CLI -- manage your RocketLogger measurements.\n"
    "Control or configure measurements using the actions specified by ACTION. "
    "Supported values for ACTION are:\n"
    "\n"
    "  Measurement control:\n"
    "    start\tStart a new measurement with provided configuration\n"
    "    stop\tStop measurement running in the background\n"
    "\n"
    "  Measurement configuration and status management:\n"
    "    config\tDisplay configuration, not starting a new or affecting a "
    "running measurement\n"
    "    status\tDisplay the current sampling status\n";

/**
 * List of arguments the program accepts
 */
static char args_doc[] = "ACTION";

/**
 * Summary of program options
 */
static struct argp_option options[] = {
    {0, 0, 0, OPTION_DOC, "Basic measurement configuration options:", 1},
    {"samples", OPT_SAMPLES_COUNT, "COUNT", 0,
     "Number of samples to record before stopping measurement (k, M, G, T "
     "scaling suffixes can be used).",
     0},
    {"channel", 'c', "SELECTION", 0, "Channel selection to sample. A comma "
                                     "separated list of the channel names (V1, "
                                     "V2, V3, V4, I1L, I1H, I2L, and/or I2H) "
                                     "or 'all' to enable all channels.",
     0},
    {"rate", 'r', "RATE", 0, "Sampling rate in Hz. Supported values are: 1, "
                             "10, 100, 1k, 2k, 4k, 8k, 16k, 32k, 64k.",
     0},
    {"update", 'u', "RATE", 0,
     "Measurement data update rate in Hz. Supported values: 1, 2, 5, 10.", 0},
    {"output", 'o', "FILE", 0,
     "Store data to specified file. Use zero to disable file storage.", 0},
    {"interactive", 'i', 0, 0,
     "Display measurement data in the command line interface.", 0},
    {"background", 'b', 0, 0,
     "Start measurement in the background and exit after start.", 0},

    {0, 0, 0, OPTION_DOC,
     "Measurement configuration options for storing measurement files:", 3},
    {"format", 'f', "FORMAT", 0, "Select file format: 'csv', 'rld'.", 0},
    {"size", OPT_FILE_SIZE, "SIZE", 0,
     "Select max file size (k, M, G, T scaling suffixes can be used).", 0},
    {"comment", 'C', "COMMENT", 0, "Comment stored in file header. Comment is "
                                   "ignored if file saving is disabled.",
     0},

    {0, 0, 0, OPTION_DOC, "Setting and resetting the stored default:", 4},
    {"default", OPT_SET_DEFAULT, 0, 0, "Set current configuration as default. "
                                       "Supported for all measurement "
                                       "configurations.",
     0},
    {"reset", OPT_RESET_DEFAULT, 0, 0,
     "Reset default configuration to factory defaults and ignores any other "
     "provided configuration without notice. Only allowed in combination with "
     "the 'config' action.",
     0},

    {0, 0, 0, OPTION_DOC, "Optional arguments for extended sampling features:",
     5},
    {"digital", 'd', "BOOL", OPTION_ARG_OPTIONAL,
     "Enable logging of digital inputs. Enabled by default.", 0},
    {"ambient", 'a', "BOOL", OPTION_ARG_OPTIONAL,
     "Enable logging of ambient sensors, if available. Disabled by default.",
     0},
    {"aggregate", 'g', "MODE", 0, "Data aggregation mode for low sample rates. "
                                  "Existing modes: 'average', 'downsample'.",
     0},
    {"high-range", 'h', "SELECTION", 0,
     "Force high range measurements on selected channels. A comma separated "
     "list of the channel names (I1H and/or I2H) or 'all' to force high range "
     "measurements all channels. Inactive per default.",
     0},
    {"calibration", OPT_CALIBRATION, 0, 0, "Ignore existing calibration "
                                           "values. Use this option for device "
                                           "calibration measurements only.",
     0},
    {"web", 'w', "BOOL", OPTION_ARG_OPTIONAL,
     "Enable web server plotting. Enabled per default.", 0},

    {0, 0, 0, OPTION_DOC, "Optional arguments for status and config actions:",
     6},
    {"json", OPT_JSON, 0, 0,
     "Print configuration or status as JSON formatted string.", 0},
    {"cli", OPT_CLI, 0, 0, "Print configuration as full CLI command.", 0},

    {0, 0, 0, OPTION_DOC, "Generic program switches:", 7},
    {"verbose", 'v', 0, 0, "Produce verbose output", 0},
    {"quiet", 'q', 0, 0, "Do not produce any output", 0},
    {"silent", 's', 0, OPTION_ALIAS, 0, 0},

    {0, 0, 0, 0, 0, 0},
};

/**
 * The argument data structure to store parsed arguments
 */
struct arguments {
    char *args[ARGP_ARGUMENTS_COUNT]; /// program arguments
    rl_config_t *config;              /// pointer to sampling configuration
    bool config_reset;       /// whether to reset the stored default config
    bool config_set_default; /// whether to save provided config as default
    bool cli;                /// flag for CLI command formatted config output
    bool json;               /// flag for JSON formatted output
    bool silent;             /// flag for silent output
    bool verbose;            /// flag for verbose output
};

/* local function declarations */
static error_t parse_opt(int key, char *arg, struct argp_state *state);
static void parse_bool(char const *arg, struct argp_state *state,
                       bool *const value);
static void parse_bool_named_list(char const *arg, struct argp_state *state,
                                  char const *const *const names,
                                  bool *const values, int size);
static void parse_uint32(char const *arg, struct argp_state *state,
                         uint32_t *const value);
static void parse_uint64(char const *arg, struct argp_state *state,
                         uint64_t *const value);
static void print_config(rl_config_t const *const config);
static void print_version(FILE *stream, struct argp_state *state);

/**
 * The argp program version print function hook
 */
void (*argp_program_version_hook)(FILE *, struct argp_state *) = print_version;

/**
 * The full `argp` parser configuration
 */
static struct argp argp = {
    .options = options, .parser = parse_opt, .args_doc = args_doc, .doc = doc,
};

/**
 * RocketLogger main program log file.
 */
static char const *const log_filename = RL_MEASUREMENT_LOG_FILE;

/**
 * Main RocketLogger binary, controls the sampling
 *
 * @param argc Number of input arguments
 * @param argv Input arguments
 * @return standard Linux return codes
 */
int main(int argc, char *argv[]) {
    rl_config_t config;

    // load default configuration
    int res = rl_config_read_default(&config);
    if (res < 0) {
        error(EXIT_FAILURE, errno, "failed reading default configuration file");
    }

    // argument structure with default values
    struct arguments arguments = {
        .args = {NULL},
        .config = &config,
        .config_reset = false,
        .config_set_default = false,
        .cli = false,
        .json = false,
        .silent = false,
        .verbose = false,
    };

    // parse CLI arguments and options using argp
    int argp_status = argp_parse(&argp, argc, argv, 0, 0, &arguments);
    if (argp_status != 0) {
        error(0, argp_status, "argument parsing failed");
    }

    // init log module with appropriate verbosity level
    if (arguments.verbose) {
        rl_log_init(log_filename, RL_LOG_VERBOSE);
    } else if (arguments.silent) {
        rl_log_init(log_filename, RL_LOG_IGNORE);
    } else {
        rl_log_init(log_filename, RL_LOG_WARNING);
    }

    char const *const action = arguments.args[0];

    // validate arguments
    bool valid_action =
        (strcmp(action, "start") == 0 || strcmp(action, "stop") == 0 ||
         strcmp(action, "config") == 0 || strcmp(action, "status") == 0);
    if (!valid_action) {
        rl_log(RL_LOG_ERROR, "unknown action '%s'", action);
        exit(EXIT_FAILURE);
    }

    if (arguments.cli && arguments.json) {
        rl_log(RL_LOG_ERROR, "cannot format in output as JSON and and CLI "
                             "string at the same time.");
        exit(EXIT_FAILURE);
    }

    // validate sampling configuration
    int valid_config = rl_config_validate(&config);
    if (valid_config < 0) {
        rl_log(RL_LOG_ERROR, "invalid configuration, check message above");
        exit(EXIT_FAILURE);
    }

    /// @todo temporarily disabled web interface buffer handling
    // disable incompatible web interface
    if (config.web_enable) {
        rl_log(RL_LOG_WARNING, "no compatible web interface implemented, "
                               "disabling web interface.\n");
        config.web_enable = false;
    }

    // reset config if requested
    if (arguments.config_reset) {
        if (strcmp(action, "config") != 0) {
            rl_log(RL_LOG_ERROR,
                   "the --reset option is only allowed for config action.");
            exit(EXIT_FAILURE);
        }
        rl_config_reset(&config);
        rl_log(RL_LOG_INFO, "Configuration was reset to factory default.");
    }
    // store config as default
    if (arguments.config_set_default) {
        rl_config_write_default(&config);
        if (!arguments.silent) {
            printf("The following configuration was saved as new default:\n");
            rl_config_print(&config);
        }
    }

    // configure and run system in the requested MODE
    if (strcmp(action, "start") == 0) {
        // check if already sampling
        if (rl_is_sampling()) {
            rl_log(RL_LOG_ERROR, "RocketLogger is still running.\n"
                                 "Stop with `rocketlogger stop` first.\n");
            exit(EXIT_FAILURE);
        }

        if (arguments.verbose) {
            print_config(&config);
        }
        rl_log(RL_LOG_INFO, "Starting measurement...\n");
        rl_run(&config);
    }
    if (strcmp(action, "stop") == 0) {
        // exit with error if not sampling
        if (!rl_is_sampling()) {
            rl_log(RL_LOG_ERROR, "RocketLogger is not running.\n");
            exit(EXIT_FAILURE);
        }

        if (!arguments.silent) {
            printf("Wait for measurement to stop...\n");
        }
        rl_stop();
        /// @todo wait for measurement process to stop
    }
    if (strcmp(action, "config") == 0) {
        if (arguments.json) {
            rl_config_print_json(&config);
        } else if (arguments.cli) {
            rl_config_print_cmd(&config);
        } else {
            rl_config_print(&config);
        }
    }
    if (strcmp(action, "status") == 0) {
        rl_status_t status;
        int res = rl_get_status(&status);
        if (res < 0) {
            rl_log(RL_LOG_ERROR, "Failed getting RocketLogger status (%d).\n",
                   res);
            exit(EXIT_FAILURE);
        }
        if (arguments.json) {
            rl_status_print_json(&status);
        } else {
            rl_status_print(&status);
        }
        // @todo encode status in exit value
    }
    exit(EXIT_SUCCESS);
}

/**
 * The CLI option parser function.
 *
 * @param key Argument key
 * @param arg Argument string value
 * @param state Argument state structure
 * @return Error code or 0 on success
 */
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    // get pointer to where run configuration is stored
    struct arguments *arguments = state->input;
    rl_config_t *config = arguments->config;

    // parse actual argument
    switch (key) {
    /* options with shortcuts */
    case 'q':
    case 's':
        /* quiet/silent switch: no value */
        arguments->silent = true;
        arguments->verbose = false;
        break;
    case 'v':
        /* verbose switch: no value */
        arguments->verbose = true;
        arguments->silent = false;
        break;
    case 'b':
        /* run in background: no value */
        config->background_enable = true;
        break;
    case 'i':
        /* display measurements interactively: no value */
        config->interactive_enable = true;
        break;
    case 'c':
        /* channel selection: mandatory SELECTION value */
        parse_bool_named_list(arg, state, RL_CHANNEL_NAMES,
                              config->channel_enable, RL_CHANNEL_COUNT);
        break;
    case 'r':
        /* sampling rate: mandatory RATE value */
        parse_uint32(arg, state, &config->sample_rate);
        break;
    case 'u':
        /* measurement update rate: mandatory RATE value */
        parse_uint32(arg, state, &config->update_rate);
        break;
    case 'o':
        /* measurement output file: mandatory FILE value */
        if (arg != NULL) {
            if (strlen(arg) == 1 && arg[0] == '0') {
                config->file_enable = false;
            } else {
                config->file_enable = true;
                strncpy(config->file_name, arg, RL_PATH_LENGTH_MAX - 1);
            }
        } else {
            argp_usage(state);
        }
        break;
    case 'f':
        /* measurement file format: mandatory FORMAT value */
        if (strcmp(arg, "csv") == 0 || strcmp(arg, "CSV") == 0) {
            config->file_format = RL_FILE_FORMAT_CSV;
        } else if (strcmp(arg, "rld") == 0 || strcmp(arg, "RLD") == 0) {
            config->file_format = RL_FILE_FORMAT_RLD;
        } else {
            argp_usage(state);
        }
        break;
    case 'C':
        /* measurement file comment: mandatory COMMENT value */
        if (arg != NULL) {
            config->file_comment = arg;
        } else {
            argp_usage(state);
        }
        break;
    case 'd':
        /* digital channel: optional BOOL value */
        if (arg != NULL) {
            parse_bool(arg, state, &config->digital_enable);
        } else {
            config->digital_enable = true;
        }
        break;
    case 'a':
        /* ambient sensors: optional BOOL value */
        if (arg != NULL) {
            parse_bool(arg, state, &config->ambient_enable);
        } else {
            config->ambient_enable = true;
        }
        break;
    case 'g':
        /* data aggregation mode: mandatory MODE value */
        if (strcmp(arg, "downsample") == 0) {
            config->aggregation_mode = RL_AGGREGATION_MODE_DOWNSAMPLE;
        } else if (strcmp(arg, "average") == 0) {
            config->aggregation_mode = RL_AGGREGATION_MODE_AVERAGE;
        } else {
            argp_usage(state);
        }
        break;
    case 'h':
        /* force high-range current measurement: mandatory SELECTION value */
        parse_bool_named_list(arg, state, RL_CHANNEL_FORCE_NAMES,
                              config->channel_force_range,
                              RL_CHANNEL_SWITCHED_COUNT);
        break;
    case 'w':
        /* web interface enable: optional BOOL value */
        if (arg != NULL) {
            parse_bool(arg, state, &config->web_enable);
        } else {
            config->web_enable = true;
        }
        break;

    /* options without shortcuts */
    case OPT_SAMPLES_COUNT:
        /* sample count: mandatory COUNT value */
        parse_uint64(arg, state, &config->sample_limit);
        break;
    case OPT_FILE_SIZE:
        /* maximum file size: mandatory SIZE value */
        parse_uint64(arg, state, &config->file_size);
        break;
    case OPT_CLI:
        /* CLI format the config output: no value */
        arguments->cli = true;
        break;
    case OPT_JSON:
        /* JSON format the status and config output: no value */
        arguments->json = true;
        break;
    case OPT_SET_DEFAULT:
        /* set configuration as default: no value */
        arguments->config_set_default = true;
        break;
    case OPT_RESET_DEFAULT:
        /* reset configuration to system default: no value */
        arguments->config_reset = true;
        break;
    case OPT_CALIBRATION:
        /* perform calibration measurement: no value */
        config->calibration_ignore = true;
        break;

    /* unnamed argument options */
    case ARGP_KEY_ARG:
        // check for too many arguments
        if (state->arg_num >= ARGP_ARGUMENTS_COUNT) {
            argp_usage(state);
        }
        arguments->args[state->arg_num] = arg;
        break;
    case ARGP_KEY_END:
        // check for not enough arguments
        if (state->arg_num < ARGP_ARGUMENTS_COUNT) {
            argp_usage(state);
        }
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

/**
 * Parse a boolean argument value.
 *
 * Exits with error on parsing error.
 *
 * @param arg Argument string value
 * @param state Argument state structure
 * @param value Pointer to boolean to store result to
 */
static void parse_bool(char const *arg, struct argp_state *state,
                       bool *const value) {
    if (strlen(arg) == 1) {
        if (arg[0] == '0') {
            *value = false;
            return;
        } else if (arg[0] == '1') {
            *value = true;
            return;
        }
    }
    if (strcmp(arg, "true") == 0 || strcmp(arg, "True") == 0 ||
        strcmp(arg, "TRUE") == 0) {
        *value = true;
        return;
    }
    if (strcmp(arg, "false") == 0 || strcmp(arg, "False") == 0 ||
        strcmp(arg, "FALSE") == 0) {
        *value = false;
        return;
    }
    argp_usage(state);
}

/**
 * Parse comma separated list of names to set boolean list.
 *
 * When providing the argument 'all', all values are set to true. The names
 * are expected to be all upper case, to make the comparison case insensitive.
 * The function exits with an error on parsing error.
 *
 * @param arg Argument string value
 * @param state Argument state structure
 * @param names Pointer to upper case string array naming the values to parse
 * @param values Pointer to boolean array to store results to
 * @param size The size of the names and values arrays
 */
static void parse_bool_named_list(char const *arg, struct argp_state *state,
                                  char const *const *const names,
                                  bool *const values, int size) {
    // check for all enable argument
    if (strcmp(arg, "all") == 0) {
        memset(values, true, size * sizeof(bool));
        return;
    }
    // reset values
    memset(values, false, size * sizeof(bool));

    // split input by comma
    char const *split_pos = arg;
    while (*arg != '\0') {
        // find next argument name
        split_pos = strchr(arg, ',');
        char arg_name[16] = {0};
        if (split_pos == NULL) {
            strncpy(arg_name, arg, sizeof(arg_name) - 1);
            arg = arg + strlen(arg); // set to end to exit loop when done
        } else {
            strncpy(arg_name, arg, split_pos - arg);
            arg = split_pos + 1; // next argument starts right after comma
        }

        // convert parsed name
        char *ptr = arg_name;
        do {
            *ptr = toupper(*ptr);
        } while (*ptr++);

        // process channel name
        int i = 0;
        while (i < size) {
            if (strcmp(arg_name, names[i]) == 0) {
                values[i] = true;
                break;
            }
            i++;
        }
        // check valid channel was set
        if (i == size) {
            argp_usage(state);
            return;
        }
    }
}

/**
 * Parse a 32 bit unsigned integer argument value with optional scaling suffix.
 *
 * Exits with error on parsing error.
 *
 * @param arg Argument string value
 * @param state Argument state structure
 * @param value Pointer to the integer to store result to
 */
static void parse_uint32(char const *arg, struct argp_state *state,
                         uint32_t *const value) {
    uint64_t temp;
    parse_uint64(arg, state, &temp);
    if ((temp >> 32) == 0) {
        *value = (uint32_t)temp;
    } else {
        argp_usage(state);
    }
}

/**
 * Parse a 64 bit unsigned integer argument value with optional scaling suffix.
 *
 * Exits with error on parsing error.
 *
 * @param arg Argument string value
 * @param state Argument state structure
 * @param value Pointer to the integer to store result to
 */
static void parse_uint64(char const *arg, struct argp_state *state,
                         uint64_t *const value) {
    char *suffix = NULL;
    *value = strtoull(arg, &suffix, 10);

    // check for scaling suffix and apply it iteratively
    if (suffix != NULL) {
        switch (*suffix) {
        case 'T':
            *value = *value * 1000;
        /* FALL THROUGH */
        case 'G':
            *value = *value * 1000;
        /* FALL THROUGH */
        case 'M':
            *value = *value * 1000;
        /* FALL THROUGH */
        case 'k':
            *value = *value * 1000;
        /* FALL THROUGH */
        case '\0':
            break;
        default:
            argp_usage(state);
            return;
        }
    } else {
        argp_usage(state);
        return;
    }
}

/**
 * Print program version helper function.
 *
 * @param stream Stream to write to
 * @param state The argp state
 */
static void print_version(FILE *stream, struct argp_state *state) {
    (void)state; // suppress unused parameter warning
    fprintf(stream, "%s %s\n", argp_program_version, PROJECT_VERSION);
    fprintf(stream, "  git@%s (%s)\n", GIT_DESCRIPTION, GIT_DATE);
    fprintf(stream, "  compiled at %s\n", COMPILE_DATE);
}

/**
 * Print provided configuration to command line.
 *
 * @param config Pointer to {@link rl_conf} configuration
 */
static void print_config(rl_config_t const *const config) {
    printf("\nRocketLogger Configuration:\n");
    rl_config_print(config);
    printf("\n");
}
