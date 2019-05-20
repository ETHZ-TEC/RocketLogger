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

#ifndef RL_UTIL_H_
#define RL_UTIL_H_

#include "types.h"

/// Default configuration file
#define DEFAULT_CONFIG "/etc/rocketlogger/default.conf"

/**
 * Available command line options of the RocketLogger CLI
 */
enum rl_option {
    FILE_NAME,      //!< Name of data file to write
    SAMPLE_RATE,    //!< Sampling rate
    UPDATE_RATE,    //!< Data file update rate
    CHANNEL,        //!< Channels to sample
    FHR,            //!< Channels to force to high range
    WEB,            //!< En-/disable data averaging for web server
    DIGITAL_INPUTS, //!< Sample digital inputs
    AMBIENT,        //!< Ambient sensor logging
    AGGREGATION,    //!< Sample aggregation
    DEF_CONF,       //!< Set configuration as default
    CALIBRATION,    //!< Use/ignore existing calibration values
    FILE_FORMAT,    //!< File format
    FILE_SIZE,      //!< Maximum data file size
    COMMENT,        //!< File comment
    NO_OPTION       //!< No option
};

/**
 * Typedef for RocketLogger command line options
 */
typedef enum rl_option rl_option_t;

void rl_print_config(rl_config_t const *const conf);
void rl_print_status(rl_status_t const *const status);
void rl_print_version(void);

rl_mode_t get_mode(char const *const mode);
rl_option_t get_option(char const *const option);
int parse_args(int argc, char *argv[], rl_config_t *const config,
               bool *const set_as_default, char **const file_comment);

void print_usage(void);

void print_config(rl_config_t const *const conf);
void reset_config(rl_config_t *const conf);
int read_default_config(rl_config_t *const conf);
int write_default_config(rl_config_t const *const conf);

#endif /* RL_UTIL_H_ */
