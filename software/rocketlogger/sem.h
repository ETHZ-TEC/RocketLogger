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

#ifndef SEM_H_
#define SEM_H_

#include <sys/types.h>

/**
 * Create RocketLogger semaphore set.
 *
 * @return ID of created set
 */
int sem_create(key_t key, int num_sems);

/**
 * Remove semaphore set.
 *
 * @param sem_id ID of set to remove
 * @return {@link SUCCESS} on success, {@link FAILURE} otherwise
 */
int sem_remove(int sem_id);

/**
 * Open existing RocketLogger semaphore set.
 *
 * @return ID of opened set
 */
int sem_open(key_t key, int num_sems);

/**
 * Wait on a semaphore until access granted.
 *
 * @param sem_id ID of semaphore set
 * @param sem_num Number of semaphore in set
 * @param time_out Maximum waiting time
 * @return {@link SUCCESS} on success (access granted), {@link TIME_OUT} on time
 * out, {@link FAILURE} otherwise
 */
int sem_wait(int sem_id, int sem_num, int time_out);

/**
 * Set value to semaphore.
 *
 * @param sem_id ID of semaphore set
 * @param sem_num Number of semaphore in set
 * @param val Value to be set to semaphore
 * @return {@link SUCCESS} on success (access granted), {@link TIME_OUT} on time
 * out, {@link FAILURE} otherwise
 */
int sem_set(int sem_id, int sem_num, int val);

/**
 * Get value of a semaphore.
 *
 * @param sem_id ID of semaphore set
 * @param sem_num Number of semaphore in set
 * @return The semaphore count or -1 on error.
 */
int sem_get(int sem_id, int sem_num);

#endif /* SEM_H_ */
