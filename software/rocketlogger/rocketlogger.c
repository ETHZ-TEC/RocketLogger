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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "log.h"
#include "rl_lib.h"
#include "rl_util.h"

/**
 * Main RocketLogger binary, controls the sampling
 *
 * @param argc Number of input arguments
 * @param argv Input argument string, consists of:
 *   - Mode {@link rl_mode}
 *   - Options {@link rl_option} (+ value)
 * @return standard Linux return codes
 */
int main(int argc, char *argv[]) {

    rl_config_t config;
    bool set_as_default;
    char *file_comment;

    // get default config
    read_default_config(&config);

    // parse arguments
    if (parse_args(argc, argv, &config, &set_as_default, &file_comment) ==
        FAILURE) {
        print_usage();
        exit(EXIT_FAILURE);
    }

    // store config as default
    if (set_as_default == 1) {
        write_default_config(&config);
    }

    switch (config.mode) {
    case LIMIT:
        if (rl_get_status() == RL_RUNNING) {
            rl_log(ERROR,
                   "RocketLogger already running\n Run:  rocketlogger stop\n");
            exit(EXIT_FAILURE);
        }
        print_config(&config);
        printf("Start sampling ...\n");
        break;

    case CONTINUOUS:
        if (rl_get_status() == RL_RUNNING) {
            rl_log(ERROR,
                   "RocketLogger already running\n Run:  rocketlogger stop\n");
            exit(EXIT_FAILURE);
        }
        print_config(&config);
        printf("Data acquisition running in background ...\n  Stop with:   "
               "rocketlogger stop\n\n");
        break;

    case METER:
        if (rl_get_status() == RL_RUNNING) {
            rl_log(ERROR,
                   "RocketLogger already running\n Run:  rocketlogger stop\n");
            exit(EXIT_FAILURE);
        }
        break;

    case STATUS: {
        rl_status_t status;
        rl_read_status(&status);
        rl_print_status(&status);
        return status.state;
    }

    case STOPPED:
        if (rl_get_status() != RL_RUNNING) {
            rl_log(ERROR, "RocketLogger not running");
            exit(EXIT_FAILURE);
        }
        printf("Stopping RocketLogger ...\n");
        rl_stop();
        exit(EXIT_SUCCESS);

    case SET_DEFAULT:
        write_default_config(&config);
        if (rl_get_status() == RL_RUNNING) {
            printf("\n");
            rl_log(WARNING, "change will not affect current measurement");
        }
        print_config(&config);
        exit(EXIT_SUCCESS);

    case PRINT_DEFAULT:
        print_config(&config);
        exit(EXIT_SUCCESS);

    case PRINT_VERSION:
        rl_print_version();
        exit(EXIT_SUCCESS);

    case HELP:
        print_usage();
        exit(EXIT_SUCCESS);

    default:
        print_usage();
        exit(EXIT_FAILURE);
    }

    // start the sampling
    rl_start(&config, file_comment);

    exit(EXIT_SUCCESS);
}
