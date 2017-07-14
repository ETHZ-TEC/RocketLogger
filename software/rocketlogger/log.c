/**
 * Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group
 */

#include "log.h"

/**
 * Log a message.
 * @param type Type of message.
 * @param format Message format.
 */
void rl_log(rl_log_type type, const char* format, ...) {

    // open/init file
    FILE* log_fp;
    if (access(LOG_FILE, F_OK) == -1) {
        // create file
        log_fp = fopen(LOG_FILE, "w");
        if (log_fp == NULL) {
            printf("Error: failed to open log file\n");
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

    // reset, if file too large
    int file_size = ftell(log_fp);
    if (file_size > MAX_LOG_FILE_SIZE) {
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
    if (type == ERROR) {

        // file
        fprintf(log_fp, "     Error: ");
        vfprintf(log_fp, format, args);
        fprintf(log_fp, "\n");
        // terminal
        printf("Error: ");
        vprintf(format, args);
        printf("\n\n");

        // set state to error
        status.state = RL_ERROR;

    } else if (type == WARNING) {

        // file
        fprintf(log_fp, "     Warning: ");
        vfprintf(log_fp, format, args);
        fprintf(log_fp, "\n");
        // terminal
        printf("Warning: ");
        vprintf(format, args);
        printf("\n\n");

    } else if (type == INFO) {

        fprintf(log_fp, "     Info: ");
        vfprintf(log_fp, format, args);
        fprintf(log_fp, "\n");

    } else {
        // for debugging purposes
        printf("Error: wrong error-code\n");
    }

    // facilitate return
    va_end(args);

    // close file
    fflush(log_fp);
    fclose(log_fp);
}
