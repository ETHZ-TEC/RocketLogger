/**
 * Copyright (c) 2016-2020, ETH Zurich, Computer Engineering Group
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

#ifndef LOG_H_
#define LOG_H_

/// Default log file name
#define RL_LOG_DEFAULT_FILE "/var/log/rocketlogger.log"

/// Maximum log file name path length
#define RL_LOG_PATH_LENGTH_MAX 256

/// Maximum log file size in bytes
#define RL_LOG_FILE_SIZE_MAX (1000 * 1000)

/**
 * RocketLogger log level definition
 */
enum rl_log_level {
    RL_LOG_IGNORE,  //!< Ignore log (only for verbosity configuration)
    RL_LOG_ERROR,   //!< Error
    RL_LOG_WARNING, //!< Warning
    RL_LOG_INFO,    //!< Information
    RL_LOG_VERBOSE, //!< Verbose
};

/**
 * Typedef for RocketLogger level types
 */
typedef enum rl_log_level rl_log_level_t;

/**
 * Initialize the log module.
 *
 * @note This function should be called before the first log message is stored.
 *
 * @param log_file The filename of the file the log messages are written to
 * @param verbosity Messages with a log level more severe or equal to this level
 *                  are also printed to the terminal
 * @return Returns 0 on success, negative on failure with errno set accordingly
 */
int rl_log_init(char const *const log_file, rl_log_level_t verbosity);

/**
 * Change the log module's verbosity.
 *
 * @param verbosity Messages with log levels more severe or equal to the
 *                        verbosity level are also printed to the terminal
 */
void rl_log_verbosity(rl_log_level_t verbosity);

/**
 * Write a new log message.
 *
 * @param log_level Log level of the message
 * @param format The message format string passed to fprintf()
 * @param ... Variables used to format value string
 * @return Returns 0 on success, negative on failure with errno set accordingly
 */
int rl_log(rl_log_level_t log_level, char const *const format, ...);

#endif /* LOG_H_ */
