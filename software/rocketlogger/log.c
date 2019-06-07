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

#include <stdarg.h>
#include <stdio.h>

#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "log.h"

/**
 * Log a message.
 * @param type Type of message.
 * @param format Message format.
 */
void rl_log(rl_log_t type, char const *const format, ...) {
    // open/init file
    FILE *log_fp;
    if (access(LOG_FILE, F_OK) == -1) {
        // create file
        log_fp = fopen(LOG_FILE, "w");
        if (log_fp == NULL) {
            printf("Error: failed to create log file\n");
            return;
        }
        fprintf(log_fp, "--- RocketLogger Log File ---\n\n");
    } else {
        log_fp = fopen(LOG_FILE, "a");
        if (log_fp == NULL) {
            printf("Error: failed to open log file\n");
            return;
        }
    }

    // reset file if file is getting too large
    int file_size = ftell(log_fp);
    if (file_size > LOG_FILE_MAX_SIZE) {
        fclose(log_fp);
        fopen(LOG_FILE, "w");
        fprintf(log_fp, "--- RocketLogger Log File ---\n\n");
    }

    // print date/time
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    time_t nowtime = current_time.tv_sec;
    fprintf(log_fp, "  %s", ctime(&nowtime));

    // get arguments
    va_list args;
    va_start(args, format);

    // print error message
    switch (type) {
    case RL_LOG_ERROR:
        // file
        fprintf(log_fp, "     Error: ");
        vfprintf(log_fp, format, args);
        fprintf(log_fp, "\n");
        // terminal
        printf("Error: ");
        vprintf(format, args);
        printf("\n\n");
        break;
    case RL_LOG_WARNING:
        // file
        fprintf(log_fp, "     Warning: ");
        vfprintf(log_fp, format, args);
        fprintf(log_fp, "\n");
        // terminal
        printf("Warning: ");
        vprintf(format, args);
        printf("\n\n");
        break;
    case RL_LOG_INFO:
        fprintf(log_fp, "     Info: ");
        vfprintf(log_fp, format, args);
        fprintf(log_fp, "\n");
        break;
    default:
        // for debugging purposes
        printf("Error: wrong error-code\n");
        break;
    }

    // facilitate return
    va_end(args);

    // close file
    fflush(log_fp);
    fclose(log_fp);
}
