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

#ifndef SYSFS_H_
#define SYSFS_H_

/**
 * Export (activate) a sysfs device.
 *
 * If the ressource is busy an error is returned. You might want to use
 * {@link sysfs_unexport} first to make sure a device is deactivated.
 *
 * @param sysfs_file The sysfs file used to export
 * @param value The resource number of the device to export
 * @return 0 in case of success, -1 otherwise.
 */
int sysfs_export(char const *const sysfs_file, int value);

/**
 * Unexport (deactivate) a sysfs device.
 *
 * If the ressource is deactivated already, it is not considered an error.
 *
 * @param sysfs_file The sysfs file used to unexport
 * @param value The resource number of the device to unexport
 * @return 0 in case of success, -1 otherwise.
 */
int sysfs_unexport(char const *const sysfs_file, int value);

/**
 * Write a string to a sysfs device file.
 *
 * @param sysfs_file The sysfs file to write to
 * @param value The value to write to write
 * @return 0 in case of success, -1 otherwise.
 */
int sysfs_write_string(char const *const sysfs_file, char const *const value);

/**
 * Read a string from a sysfs device file.
 *
 * @param sysfs_file The sysfs file to read from
 * @param value Pointer to the character array to write to.
 * @param length Max length to the string to read.
 * @return Read string length in case of success, -1 on failure.
 */
int sysfs_read_string(char const *const sysfs_file, char *const value,
                      int length);

/**
 * Write an integer to a sysfs device file.
 *
 * @param sysfs_file The sysfs file to write to
 * @param value The value to write to write
 * @return 0 in case of success, -1 otherwise.
 */
int sysfs_write_int(char const *const sysfs_file, int value);

/**
 * Write an integer to a sysfs device file.
 *
 * @param sysfs_file The sysfs file to read from
 * @param value Pointer to the integer variable to write to.
 * @return 0 in case of success, -1 otherwise.
 */
int sysfs_read_int(char const *const sysfs_file, int *const value);

#endif /* SYSFS_H_ */
