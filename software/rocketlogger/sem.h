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

/// Semaphore key (used for set creation)
#define SEM_KEY 2222
/// Time out time in seconds, waiting on semaphore write
#define SEM_OPT_NO_FLAG 0
/// Time out time in seconds, waiting on semaphore read
#define SEM_TIMEOUT_READ 3
/// Time out time in seconds, waiting on semaphore write
#define SEM_TIMEOUT_WRITE 1

/// Total number of semaphores in set
#define SEM_SEM_COUNT 2
/// Index of the data semaphore (protects shared memory accesses)
#define SEM_INDEX_DATA 0
/// Index of the wait semaphore (blocks until new data is available)
#define SEM_INDEX_WAIT 1

/**
 * Create RocketLogger semaphore set.
 *
 * @param key The key identifying the semaphore set to create
 * @param count The number of semaphores in the set to create
 * @return ID of created set on success, negative on failure with errno set
 * accordingly
 */
int sem_create(key_t key, int count);

/**
 * Remove semaphore set.
 *
 * @param id ID of set to remove
 * @return Returns 0 on success, negative on failure with errno set accordingly
 */
int sem_remove(int id);

/**
 * Open existing RocketLogger semaphore set.
 *
 * @param key The key identifying the semaphore set
 * @param count The number of semaphores in the set
 * @return ID of opened set, negative on failure with errno set accordingly
 */
int sem_open(key_t key, int count);

/**
 * Wait on a semaphore until access granted.
 *
 * @param id ID of semaphore set
 * @param index Index of semaphore in set
 * @param timeout Maximum waiting time before failing in seconds
 * @return Returns 0 on success, negative on failure with errno set accordingly
 */
int sem_wait(int id, int index, int timeout);

/**
 * Set value to semaphore.
 *
 * @param id ID of semaphore set
 * @param index Index of semaphore in set
 * @param value Value to be set to semaphore
 * @return Returns 0 on success, negative on failure with errno set accordingly
 */
int sem_set(int id, int index, int value);

/**
 * Get value of a semaphore.
 *
 * @param id ID of semaphore set
 * @param index Index of semaphore in set
 * @return The semaphore count, or negative on failure with errno set
 * accordingly
 */
int sem_get(int id, int index);

#endif /* SEM_H_ */
