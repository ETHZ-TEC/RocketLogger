/**
 * Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group
 */

#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>

#include "types.h"

/// Permissions for shared memory
#define SHMEM_PERMISSIONS 0666

/// MAC address length in bytes
#define MAC_ADDRESS_LENGTH 6

/**
 * Time stamp definition (UNIX time, UTC)
 */
struct time_stamp {
    /// Seconds in UNIX time (UTC)
    int64_t sec;
    /// Nanoseconds
    int64_t nsec;
};

int is_current(int index);
int is_low_current(int index);
int count_channels(int channels[NUM_CHANNELS]);

int read_status(struct rl_status* status);
int write_status(struct rl_status* status);

int ceil_div(int n, int d);

void sig_handler(int signo);

int read_file_value(char filename[]);

void create_time_stamp(struct time_stamp* time_real,
                       struct time_stamp* time_monotonic);
void get_mac_addr(uint8_t mac_address[MAC_ADDRESS_LENGTH]);

#endif
