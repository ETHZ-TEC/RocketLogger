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

#include <libgen.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/statvfs.h>

#include "rl_lib.h"
#include "rl_util.h"
#include "sem.h"
#include "web.h"

/// Number of input arguments required
#define ARG_COUNT 4
/// Maximum string line length
#define MAX_STRING_LENGTH 150
/// Maximum string value length
#define MAX_STRING_VALUE 20
/// Time margin for buffer number (in ms)
#define TIME_MARGIN 10

// Global variables
/// ID of semaphore set
int sem_id;
/// Pointer to shared memory data
struct web_shm* web_data;

/// Client request id
uint32_t id;
/// 1: data requested, 0: no data requested
uint8_t get_data;
/// Requested time scale
uint32_t t_scale;
/// Last client time stamp
int64_t last_time;
/// Time stamp of last buffer stored to shared memory
int64_t curr_time;
/// Number of channels sampled
int8_t num_channels;

/// Current status of RocketLogger
struct rl_status status;

/// Buffer sizes for different time scales
int buffer_sizes[WEB_RING_BUFFER_COUNT] = {BUFFER1_SIZE, BUFFER10_SIZE,
                                           BUFFER100_SIZE};

// functions
/**
 * Get free disk space in a directory
 * @param path Path to selected directory
 * @return Free disk space in bytes
 */
int64_t get_free_space(char* path) {

    struct statvfs stat;
    statvfs(path, &stat);

    return (uint64_t)stat.f_bavail * (uint64_t)stat.f_bsize;
}

/**
 * Print a 32-bit integer array in JSON format
 * @param data Data array to print
 * @param length Length of array
 */
void print_json_32(int32_t data[], int length) {
    char str[MAX_STRING_LENGTH];
    char val[MAX_STRING_VALUE];
    int i;
    sprintf(str, "[\"%d\"", data[0]);
    for (i = 1; i < length; i++) {
        sprintf(val, ",\"%d\"", data[i]);
        strcat(str, val);
    }
    strcat(str, "]\n");
    printf("%s", str);
}

/**
 * Print a 64-bit integer array in JSON format
 * @param data Data array to print
 * @param length Length of array
 */
void print_json_64(int64_t data[], int length) {
    char str[MAX_STRING_LENGTH];
    char val[MAX_STRING_VALUE];
    int i;
    sprintf(str, "[\"%lld\"", data[0]);
    for (i = 1; i < length; i++) {
        sprintf(val, ",\"%lld\"", data[i]);
        strcat(str, val);
    }
    strcat(str, "]\n");
    printf("%s", str);
}

/**
 * Print current status in JSON format
 */
void print_status(void) {
    // STATUS
    printf("%d\n", status.state);
    if (status.state != RL_RUNNING) {
        read_default_config(&status.conf);

        // read calibration time
        struct rl_calibration tmp_calibration;
        rl_read_calibration(&tmp_calibration, &status.conf);
        status.calibration_time = tmp_calibration.time;
    }
    // copy of filename (for dirname)
    char file_name_copy[MAX_PATH_LENGTH];
    strcpy(file_name_copy, status.conf.file_name);
    printf("%llu\n", get_free_space(dirname(file_name_copy)));
    printf("%llu\n", status.calibration_time);

    // CONFIG
    printf("%d\n", status.conf.sample_rate);
    printf("%d\n", status.conf.update_rate);
    printf("%d\n", status.conf.digital_inputs);
    printf("%d\n", status.conf.calibration);
    printf("%d\n", status.conf.file_format);
    printf("%s\n", status.conf.file_name);
    printf("%llu\n", status.conf.max_file_size);
    print_json_32(status.conf.channels, NUM_CHANNELS);
    print_json_32(status.conf.force_high_channels, NUM_I_CHANNELS);
    printf("%llu\n", status.samples_taken);
    printf("%d\n", status.conf.enable_web_server);
}

/**
 * Print requested data in JSON format
 */
void print_data(void) {

    // print data length
    int buffer_count = (curr_time - last_time + TIME_MARGIN) / 1000;

    // get available buffers
    if (wait_sem(sem_id, DATA_SEM, SEM_TIME_OUT) != SUCCESS) {
        return;
    }
    int buffer_available = web_data->buffer[t_scale].filled;
    set_sem(sem_id, DATA_SEM, 1);

    if (buffer_count > buffer_available) {
        buffer_count = buffer_available;
    }

    int buffer_size = buffer_sizes[t_scale];

    // print request id and status
    printf("%d\n", id);
    print_status();

    // data available
    printf("1\n");

    // print time scale
    printf("%d\n", t_scale);

    // print time
    printf("%lld\n", curr_time);

    // print buffer information
    printf("%d\n", buffer_count);
    printf("%d\n", buffer_size);

    // read data
    int64_t data[buffer_count][buffer_size][num_channels];
    int i;

    if (wait_sem(sem_id, DATA_SEM, SEM_TIME_OUT) != SUCCESS) {
        return;
    }
    for (i = 0; i < buffer_count; i++) {

        // read data buffer
        int64_t* shm_data = web_buffer_get(&web_data->buffer[t_scale], i);
        if (web_data->buffer[t_scale].element_size > sizeof(data)) {
            rl_log(ERROR,
                   "In print_data: memcpy is trying to copy to much data.");
            break;
        } else {
            memcpy(&data[i][0][0], shm_data,
                   web_data->buffer[t_scale].element_size);
        }
    }
    set_sem(sem_id, DATA_SEM, 1);

    // print data
    for (i = buffer_count - 1; i >= 0; i--) {
        int j;
        for (j = 0; j < buffer_size; j++) {
            print_json_64(data[i][j], num_channels);
        }
    }
}

/**
 * RocketLogger server program. Returns status and current sampling data (if
 * available) when running and default configuration otherwise
 *
 * @param argc Number of input arguments
 * @param argv Input argument string, consists of:
 *   - Request ID (can be used for client synchronisation)
 *   - Data requested (1 for yes, 0 for no)
 *   - Requested time scale (0: 100 samples/s, 1: 10 samples/s, 2: 1 sample/s)
 *   - Time stamp in UNIX time (UTC) of most recent data available at web client
 * @return standard Linux return codes
 */
int main(int argc, char* argv[]) {

    // parse arguments
    if (argc != ARG_COUNT + 1) {
        rl_log(ERROR, "in rl_server: not enough arguments");
        exit(FAILURE);
    }
    id = atoi(argv[1]);
    get_data = atoi(argv[2]);
    t_scale = atoi(argv[3]);
    last_time = atoll(argv[4]);

    // check time scale
    if (t_scale != S1 && t_scale != S10 && t_scale != S100) {
        rl_log(WARNING, "unknown time scale");
        t_scale = S1;
    }

    // get status
    rl_read_status(&status);

    // quit, if data not requested or not running or web disabled
    if (status.state != RL_RUNNING || status.sampling == SAMPLING_OFF ||
        status.conf.enable_web_server == 0 || get_data == 0) {
        // print request id and status
        printf("%d\n", id);
        print_status();
        printf("0\n"); // no data available
        exit(EXIT_SUCCESS);
    }

    // open semaphore
    sem_id = open_sem(SEM_KEY, NUM_SEMS);
    if (sem_id < 0) {
        // error already logged
        exit(EXIT_FAILURE);
    }

    // open shared memory
    web_data = web_open_shm();

    // fetch data
    uint8_t data_read = 0;
    while (data_read == 0) {

        // get current time
        if (wait_sem(sem_id, DATA_SEM, SEM_TIME_OUT) != SUCCESS) {
            exit(EXIT_FAILURE);
        }
        curr_time = web_data->time;
        num_channels = web_data->num_channels;
        set_sem(sem_id, DATA_SEM, 1);

        if (curr_time > last_time) {

            // re-read status
            rl_read_status(&status);

            // read and print data
            print_data();

            data_read = 1;
        } else {

            if (last_time > curr_time) {
                // assume outdated time stamp
                last_time = 0;
            }

            // wait on new data
            if (data_read == 0) {
                if (wait_sem(sem_id, WAIT_SEM, SEM_TIME_OUT) != SUCCESS) {
                    // time-out or error -> stop
                    break;
                }
            }
        }
    }

    // unmap shared memory
    shmdt(web_data);

    exit(EXIT_SUCCESS);
}
