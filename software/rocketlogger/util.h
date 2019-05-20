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

#ifndef UTIL_H_
#define UTIL_H_

#include <stdbool.h>
#include <stdint.h>

#include "types.h"

/// MAC address length in bytes
#define MAC_ADDRESS_LENGTH 6

/// File to read MAC address
#define MAC_ADDRESS_FILE "/sys/class/net/eth0/address"

/**
 * Timestamp data structure definition (UNIX time, UTC)
 */
struct rl_timestamp {
    /// Seconds in UNIX time (UTC)
    int64_t sec;
    /// Nanoseconds
    int64_t nsec;
};

/**
 * Typedef for timestamp structure
 */
typedef struct rl_timestamp rl_timestamp_t;

int is_current(int index);
int is_low_current(int index);
int count_channels(bool const channels[NUM_CHANNELS]);

int read_status(rl_status_t *const status);
int write_status(rl_status_t const *const status);

int ceil_div(int n, int d);

void sig_handler(int signo);

int read_file_value(char filename[]);

void create_time_stamp(rl_timestamp_t *const time_real,
                       rl_timestamp_t *const time_monotonic);

void get_mac_addr(uint8_t mac_address[MAC_ADDRESS_LENGTH]);

#endif /* UTIL_H_ */
