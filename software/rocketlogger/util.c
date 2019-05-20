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

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>

#include "log.h"
#include "types.h"

#include "util.h"

// -----  CHANNEL FUNCTIONS  ----- //

/**
 * Checks if channel is a current channel.
 * @param index Index of channel in array.
 * @return 1, if channel is a current, 0 otherwise.
 */
int is_current(int index) {
    if (index == I1H_INDEX || index == I1L_INDEX || index == I2H_INDEX ||
        index == I2L_INDEX) {
        return 1;
    } else {
        return 0;
    }
}

/**
 * Checks if channel is a low range current channel.
 * @param index Index of channel in array.
 * @return 1, if channel is a low range current, 0 otherwise.
 */
int is_low_current(int index) {
    if (index == I1L_INDEX || index == I2L_INDEX) {
        return 1;
    } else {
        return 0;
    }
}

/**
 * Counts the number of channels sampled.
 * @param channels Channel array.
 * @return the number of sampled channels.
 */
int count_channels(bool const channels[NUM_CHANNELS]) {
    int c = 0;
    for (int i = 0; i < NUM_CHANNELS; i++) {
        if (channels[i]) {
            c++;
        }
    }
    return c;
}

/**
 * Reads the status of the running measurement from shared memory.
 * @param status Pointer to struct array to write the status to.
 * @return {@link SUCCESS} in case of a success, {@link FAILURE} otherwise.
 */
int read_status(rl_status_t *const status) {

    // map shared memory
    int shm_id =
        shmget(SHMEM_STATUS_KEY, sizeof(rl_status_t), SHMEM_PERMISSIONS);
    if (shm_id == -1) {
        rl_log(ERROR, "In read_status: failed to get shared status memory id; "
                      "%d message: %s",
               errno, strerror(errno));
        return FAILURE;
    }
    rl_status_t *shm_status = (rl_status_t *)shmat(shm_id, NULL, 0);

    if (shm_status == (void *)-1) {
        rl_log(ERROR, "In read_status: failed to map shared status memory; %d "
                      "message: %s",
               errno, strerror(errno));
        return FAILURE;
    }

    // read status
    *status = *shm_status;

    // unmap shared memory
    shmdt(shm_status);

    return SUCCESS;
}

/**
 * Writes the status to the shared memory.
 * @param status Pointer to struct array.
 * @return {@link SUCCESS} in case of a success, {@link FAILURE} otherwise.
 */
int write_status(rl_status_t const *const status) {

    // map shared memory
    int shm_id = shmget(SHMEM_STATUS_KEY, sizeof(rl_status_t),
                        IPC_CREAT | SHMEM_PERMISSIONS);
    if (shm_id == -1) {
        rl_log(ERROR, "In write_status: failed to get shared status memory id; "
                      "%d message: %s",
               errno, strerror(errno));
        return FAILURE;
    }

    rl_status_t *shm_status = (rl_status_t *)shmat(shm_id, NULL, 0);
    if (shm_status == (void *)-1) {
        rl_log(ERROR, "In write_status: failed to map shared status memory; %d "
                      "message: %s",
               errno, strerror(errno));
        return FAILURE;
    }

    // write status
    *shm_status = *status;

    // unmap shared memory
    shmdt(shm_status);

    return SUCCESS;
}

/**
 * Integer division with ceiling.
 * @param n Numerator
 * @param d Denominator
 * @return Result
 */
int ceil_div(int n, int d) {
    if (n % d == d || n % d == 0) {
        return n / d;
    } else {
        return n / d + 1;
    }
}

// -----  SIGNAL HANDLER  ----- //

/**
 * Signal handler to catch stop signals.
 * @param signo Signal type.
 */
void sig_handler(int signo) {

    // signal generated by stop function
    if (signo == SIGQUIT) {
        // stop sampling
        status.sampling = RL_SAMPLING_OFF;
    }

    // Ctrl+C handling
    if (signo == SIGINT) {
        signal(signo, SIG_IGN);
        printf("Stopping RocketLogger ...\n");
        status.sampling = RL_SAMPLING_OFF;
    }
}
// TODO: allow forced Ctrl+C

// -----  FILE READING/WRITING  ----- //

/**
 * Read a single integer from file.
 * @param filename Path to file.
 * @return integer value in the file, {@link FAILURE} when failed.
 */
int read_file_value(char filename[]) {
    FILE *fp;
    unsigned int value = 0;
    fp = fopen(filename, "rt");
    if (fp == NULL) {
        rl_log(ERROR, "failed to open file");
        return FAILURE;
    }
    if (fscanf(fp, "%x", &value) <= 0) {
        rl_log(ERROR, "failed to read from file");
        return FAILURE;
    }
    fclose(fp);
    return value;
}

/**
 * Create time stamps (real and monotonic)
 * @param timestamp_realtime Pointer to {@link rl_timestamp_t} struct
 * @param timestamp_monotonic Pointer to {@link rl_timestamp_t} struct
 */
void create_time_stamp(rl_timestamp_t *const timestamp_realtime,
                       rl_timestamp_t *const timestamp_monotonic) {

    struct timespec spec_real;
    struct timespec spec_monotonic;

    // get time stamp of real-time and monotonic clock
    int ret1 = clock_gettime(CLOCK_REALTIME, &spec_real);
    int ret2 = clock_gettime(CLOCK_MONOTONIC_RAW, &spec_monotonic);

    if (ret1 < 0 || ret2 < 0) {
        rl_log(ERROR, "failed to get time");
    }

    // convert to own time stamp
    timestamp_realtime->sec = (int64_t)spec_real.tv_sec;
    timestamp_realtime->nsec = (int64_t)spec_real.tv_nsec;
    timestamp_monotonic->sec = (int64_t)spec_monotonic.tv_sec;
    timestamp_monotonic->nsec = (int64_t)spec_monotonic.tv_nsec;
}

/**
 * Get MAC address of device
 * @param mac_address Empty array with size {@link MAC_ADDRESS_LENGTH}
 */
void get_mac_addr(uint8_t mac_address[MAC_ADDRESS_LENGTH]) {
    FILE *fp = fopen(MAC_ADDRESS_FILE, "r");

    unsigned int temp;
    fscanf(fp, "%x", &temp);
    mac_address[0] = (uint8_t)temp;
    for (int i = 1; i < MAC_ADDRESS_LENGTH; i++) {
        fscanf(fp, ":%x", &temp);
        mac_address[i] = (uint8_t)temp;
    }
    fclose(fp);
}
