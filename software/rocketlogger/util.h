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

#include "rl.h"

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

/**
 * Checks whether a channel is a current channel.
 *
 * @param index Index of channel in array
 * @return Returns true if channel is a current, false otherwise
 */
bool is_current(int index);

/**
 * Checks whether a channel is a low range current channel.
 *
 * @param index Index of channel in array
 * @return Returns true if channel is a low range current, false otherwise
 */
bool is_low_current(int index);

/**
 * Check whether a string is empty, i.e. does not contain visible characters.
 *
 * Validates to true if none of the characters belongs to either isgraph()
 *
 * @param str The string to validate
 * @return True if string is empty, false otherwise
 */
bool is_empty_string(char const *str);

/**
 * Check whether a string is printable.
 *
 * Validates to true if all characters belong to either isspace() or isprint()
 *
 * @param str The string to validate
 * @return True if string is printable, false otherwise
 */
bool is_printable_string(char const *str);

/**
 * Counts the number of channels enabled.
 *
 * @param channels Channel enable array
 * @return The number of enabled channels
 */
int count_channels(bool const channels[RL_CHANNEL_COUNT]);

/**
 * Integer division with ceiling.
 *
 * @param n Numerator
 * @param d Denominator
 * @return Division result rounded up to next integer
 */
int div_ceil(int n, int d);

/**
 * Create time stamps (real and monotonic).
 *
 * @param timestamp_realtime Pointer to {@link rl_timestamp_t} struct
 * @param timestamp_monotonic Pointer to {@link rl_timestamp_t} struct
 */
void create_time_stamp(rl_timestamp_t *const time_real,
                       rl_timestamp_t *const time_monotonic);

/**
 * Get MAC address of network device.
 *
 * @param mac_address Array to write the MAC address to
 */
void get_mac_addr(uint8_t mac_address[MAC_ADDRESS_LENGTH]);

/**
 * Get total disk space in a directory in bytes.
 *
 * @param path Path to selected directory
 * @return Total disk space in bytes
 */
int64_t fs_space_total(char *path);

/**
 * Get free disk space in a directory in bytes.
 *
 * @param path Path to selected directory
 * @return Free disk space in bytes
 */
int64_t fs_space_free(char *path);

/**
 * Print a boolean array in JSON format.
 *
 * @param data Data array to print
 * @param length Length of array
 */
void print_json_bool(bool const *const data, const int length);

/**
 * Print a 64-bit integer array in JSON format.
 *
 * @param data Data array to print
 * @param length Length of array
 */
void print_json_int64(int64_t const *const data, const int length);

/**
 * Reads the status of the running measurement from shared memory.
 *
 * @param status Pointer to struct array to write the status to.
 * @return Returns 0 on success, negative on failure with errno set accordingly
 */
int read_status(rl_status_t *const status);

/**
 * Writes the status to the shared memory.
 *
 * @param status Pointer to struct array.
 * @return Returns 0 on success, negative on failure with errno set accordingly
 */
int write_status(rl_status_t const *const status);

#endif /* UTIL_H_ */
