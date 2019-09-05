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

#ifndef RL_LIB_H_
#define RL_LIB_H_

#include "rl.h"

/**
 * Check whether RocketLogger is sampling.
 *
 * @return The current sampling status
 */
bool rl_is_sampling(void);

/**
 * Get the current status of the RocketLogger.
 *
 * @param status The status data structure to write to
 * @return Returns 0 on success, negative on failure with errno set accordingly
 */
int rl_get_status(rl_status_t *const status);

/**
 * Run a new RocketLogger measurement.
 *
 * Returns when done unless configured to run in background in which case it
 * returns after successful start.
 *
 * @param config Configuration of the measurement to run
 * @return Returns 0 on success, negative on failure with errno set accordingly
 */
int rl_run(rl_config_t *const config);

/**
 * RocketLogger stop function (to stop a measurement run in background).
 *
 * Sends stop signal to running RocketLogger process to terminate.
 *
 * @return Returns 0 on success, negative on failure with errno set accordingly
 */
int rl_stop(void);

#endif /* RL_LIB_H_ */
