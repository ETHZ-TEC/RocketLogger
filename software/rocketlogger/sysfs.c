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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>

#include "sysfs.h"

/// String buffer size for dynamic string operations
#define SYSFS_STRING_BUFFER_LENGTH 16

/// Buffer for dynamic string operations
static char string_buffer[SYSFS_STRING_BUFFER_LENGTH];

int sysfs_export(char *sysfs_file, int value) {
    return sysfs_write_int(sysfs_file, value);
}

int sysfs_unexport(char *sysfs_file, int value) {
    int ret = sysfs_write_int(sysfs_file, value);

    // do not treat EBUSY as error
    if (ret < 0) {
        if (errno == EBUSY) {
            return 0;
        }
        return -1;
    }
    return -0;
}

int sysfs_write_string(char *sysfs_file, char *value) {
    int fd = open(sysfs_file, O_WRONLY);
    if (fd < 0) {
        return -1;
    }

    int len = strlen(value);
    int ret = write(fd, value, len);
    close(fd);
    if (ret != len) {
        return -1;
    }
    return 0;
}

int sysfs_read_string(char *sysfs_file, char *value, int length) {
    int fd = open(sysfs_file, O_RDONLY);
    if (fd < 0) {
        return -1;
    }

    int len = read(fd, value, length);
    close(fd);
    if (len < 0) {
        return -1;
    }
    return len;
}

int sysfs_write_int(char *sysfs_file, int value) {
    string_buffer[0] = 0;
    int ret = snprintf(string_buffer, SYSFS_STRING_BUFFER_LENGTH, "%d", value);
    if (ret <= 0 || ret >= SYSFS_STRING_BUFFER_LENGTH) {
        return -1;
    }
    return sysfs_write_string(sysfs_file, string_buffer);
}

int sysfs_read_int(char *sysfs_file, int *value) {
    int ret = sysfs_read_string(sysfs_file, string_buffer,
                                SYSFS_STRING_BUFFER_LENGTH);
    if (ret <= 0) {
        return -1;
    }
    if (strlen(string_buffer) <= 0) {
        return -1;
    }
    *value = atoi(string_buffer);
    return 0;
}
