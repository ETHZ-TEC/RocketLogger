/**
 * Copyright (c) 2016-2019, ETH Zurich, Computer Engineering Group
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

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <sys/statvfs.h>
#include <time.h>

#include "log.h"
#include "rl.h"

#include "util.h"

bool is_current(int index) {
    if (index == RL_CONFIG_CHANNEL_I1H || index == RL_CONFIG_CHANNEL_I1L ||
        index == RL_CONFIG_CHANNEL_I2H || index == RL_CONFIG_CHANNEL_I2L) {
        return true;
    }
    return false;
}

bool is_low_current(int index) {
    if (index == RL_CONFIG_CHANNEL_I1L || index == RL_CONFIG_CHANNEL_I2L) {
        return true;
    }
    return false;
}

bool is_voltage(int index) {
    if (index == RL_CONFIG_CHANNEL_V1 || index == RL_CONFIG_CHANNEL_V2 ||
        index == RL_CONFIG_CHANNEL_V3 || index == RL_CONFIG_CHANNEL_V4) {
        return true;
    }
    return false;
}


int count_channels(bool const channels[RL_CHANNEL_COUNT]) {
    int count = 0;
    for (int i = 0; i < RL_CHANNEL_COUNT; i++) {
        if (channels[i]) {
            count++;
        }
    }
    return count;
}

int div_ceil(int n, int d) {
    if (n % d == d || n % d == 0) {
        return n / d;
    } else {
        return n / d + 1;
    }
}

void create_time_stamp(rl_timestamp_t *const timestamp_realtime,
                       rl_timestamp_t *const timestamp_monotonic) {

    struct timespec spec_real;
    struct timespec spec_monotonic;

    // get time stamp of real-time and monotonic clock
    int ret1 = clock_gettime(CLOCK_REALTIME, &spec_real);
    int ret2 = clock_gettime(CLOCK_MONOTONIC_RAW, &spec_monotonic);

    if (ret1 < 0 || ret2 < 0) {
        rl_log(RL_LOG_ERROR, "failed to get time");
    }

    // convert to own time stamp
    timestamp_realtime->sec = (int64_t)spec_real.tv_sec;
    timestamp_realtime->nsec = (int64_t)spec_real.tv_nsec;
    timestamp_monotonic->sec = (int64_t)spec_monotonic.tv_sec;
    timestamp_monotonic->nsec = (int64_t)spec_monotonic.tv_nsec;
}

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

int64_t fs_space_free(char const *const path) {
    struct statvfs stat;
    int ret = statvfs(path, &stat);
    if (ret < 0) {
        rl_log(RL_LOG_WARNING,
               "failed getting free file system size;%d message: %s", errno,
               strerror(errno));
    }

    return (uint64_t)stat.f_bavail * (uint64_t)stat.f_bsize;
}

int64_t fs_space_total(char const *const path) {
    struct statvfs stat;
    int ret = statvfs(path, &stat);
    if (ret < 0) {
        rl_log(RL_LOG_WARNING,
               "failed getting total file system size;%d message: %s", errno,
               strerror(errno));
    }

    return ((uint64_t)stat.f_blocks * (uint64_t)stat.f_frsize);
}

bool is_empty_string(char const *str) {
    while (*str) {
        if (isgraph(*str)) {
            return false;
        }
    }
    return true;
}

bool is_printable_string(char const *str) {
    while (*str) {
        if (isprint(*str) || isspace(*str)) {
            str++;
            continue;
        }
        return false;
    }
    return true;
}

void print_json_bool(bool const *const data, const int length) {
    printf("[");
    for (int i = 0; i < length; i++) {
        if (i > 0) {
            printf(", ");
        }
        printf("%d", data[i] ? 1 : 0);
    }
    printf("]");
}

void print_json_int64(int64_t const *const data, const int length) {
    printf("[");
    for (int i = 0; i < length; i++) {
        if (i > 0) {
            printf(", ");
        }
        printf("%lld", data[i]);
    }
    printf("]");
}
