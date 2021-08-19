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
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "log.h"

/**
 * The filename of the log file to write to.
 */
static char log_filename[RL_LOG_PATH_LENGTH_MAX] = RL_LOG_DEFAULT_FILE;

/**
 * The verbosity level for printing log messages to the standard output.
 */
static rl_log_level_t log_verbosity = RL_LOG_VERBOSE;

int rl_log_init(char const *const log_file, rl_log_level_t verbosity) {
    // update log configuration
    rl_log_verbosity(verbosity);
    strncpy(log_filename, log_file, sizeof(log_filename) - 1);

    // open log file (create inexistent) to check for max file size
    FILE *log_fp = fopen(log_filename, "a");
    if (log_fp == NULL) {
        printf("Error: failed to open log file\n");
        return -1;
    }

    // reset the log file if it is getting large
    if (ftell(log_fp) > RL_LOG_FILE_SIZE_MAX) {
        log_fp = freopen(log_filename, "w", log_fp);
        if (log_fp == NULL) {
            printf("Error: failed to reset log file size\n");
            return -1;
        }
    }

    // write header for new (empty) file, or empty line for existing file
    if (ftell(log_fp) == 0) {
        fprintf(log_fp, "# RocketLogger Log File\n");
    }

    // close log file
    fclose(log_fp);
    return 0;
}

void rl_log_verbosity(rl_log_level_t verbosity) { log_verbosity = verbosity; }

int rl_log(rl_log_level_t log_level, char const *const format, ...) {
    // do not handle ignore log level
    if (log_level == RL_LOG_IGNORE) {
        return 0;
    }

    // log only if log level is equal or higher than configured verbosity
    if (log_level > log_verbosity) {
        return 0;
    }

    // get arguments
    va_list args;
    va_start(args, format);

    // get current time
    time_t time_raw;
    struct tm *time_info;
    char time_str[24];

    time(&time_raw);
    time_info = gmtime(&time_raw);
    strftime(time_str, sizeof(time_str), "%F %T\t", time_info);

    // open log file
    FILE *log_fp = fopen(log_filename, "a");
    if (log_fp == NULL) {
        printf("Error: failed to open log file\n");
        return -1;
    }

    // write time, log level, and message to log file
    fprintf(log_fp, "%s", time_str);
    switch (log_level) {
    case RL_LOG_ERROR:
        fprintf(log_fp, "ERROR\t");
        break;
    case RL_LOG_WARNING:
        fprintf(log_fp, "WARN\t");
        break;
    case RL_LOG_INFO:
        fprintf(log_fp, "INFO\t");
        break;
    case RL_LOG_VERBOSE:
        fprintf(log_fp, "VERB\t");
        break;
    default:
        // for debugging purposes
        fprintf(log_fp, "N/A\t");
        printf("Error: unsupported log level with message:\n  ");
        break;
    }
    vfprintf(log_fp, format, args);
    fprintf(log_fp, "\n");

    // close file
    fclose(log_fp);

    // output to terminal
    switch (log_level) {
    case RL_LOG_ERROR:
        printf("Error: ");
        break;
    case RL_LOG_WARNING:
        printf("Warning: ");
        break;
    case RL_LOG_INFO:
        printf("Info: ");
        break;
    case RL_LOG_VERBOSE:
        printf("Verbose: ");
        break;
    default:
        // skip handled above
        break;
    }
    vprintf(format, args);
    printf("\n");

    // facilitate return
    va_end(args);

    return 0;
}
