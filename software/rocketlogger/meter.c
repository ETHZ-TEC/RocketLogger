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

#include <stdint.h>

#include <ncurses.h>

#include "pru.h"
#include "rl.h"
#include "util.h"

#include "meter.h"

/// Analog channel units
char const *const RL_CHANNEL_UNITS[RL_CHANNEL_COUNT] = {
    "V", "V", "V", "V", "uA", "mA", "uA", "mA", "ms"};

/// Analog channel scales
double const RL_CHANNEL_SCALES[RL_CHANNEL_COUNT] = {
    100000000, 100000000, 100000000, 100000000, 100000,
    1000000,   100000,    1000000,   1000000};

/// Digital input bit location in binary data
uint32_t const DIGITAL_INPUT_BITS[RL_CHANNEL_DIGITAL_COUNT] = {
    PRU_DIGITAL_INPUT1_MASK, PRU_DIGITAL_INPUT2_MASK, PRU_DIGITAL_INPUT3_MASK,
    PRU_DIGITAL_INPUT4_MASK, PRU_DIGITAL_INPUT5_MASK, PRU_DIGITAL_INPUT6_MASK};

void meter_init(void) {
    // init ncurses mode
    initscr();
    // hide cursor
    curs_set(0);

    mvprintw(1, 1, "Starting RocketLogger Meter ...");
    refresh();
}

void meter_deinit(void) { endwin(); }

void meter_print_buffer(int32_t const *analog_buffer,
                        uint32_t const *digital_buffer, size_t buffer_size,
                        rl_timestamp_t const *const timestamp_realtime,
                        rl_timestamp_t const *const timestamp_monotonic,
                        rl_config_t const *const config) {
    // aggregation buffer and configuration
    uint32_t aggregate_digital = ~(0);
    double aggregate_analog[RL_CHANNEL_COUNT] = {0};

    // process data buffers
    for (size_t i = 0; i < buffer_size; i++) {
        // point to sample buffer
        int32_t const *const analog_data = analog_buffer + i * RL_CHANNEL_COUNT;
        uint32_t const *const digital_data = digital_buffer + i;

        // aggregate data
        switch (config->aggregation_mode) {
        case RL_AGGREGATION_MODE_DOWNSAMPLE:
            // use first sample of buffer only, skip storing others
            if (i == 0) {
                for (int j = 0; j < RL_CHANNEL_COUNT; j++) {
                    aggregate_analog[j] = (double)analog_data[j];
                }
                aggregate_digital = *digital_data;
            }
            break;

        default:
        case RL_AGGREGATION_MODE_AVERAGE:
            // accumulate data of the aggregate window, store at the end
            for (int j = 0; j < RL_CHANNEL_COUNT; j++) {
                aggregate_analog[j] += (double)*(analog_data + j);
            }
            aggregate_digital = aggregate_digital & *digital_data;
            break;
        }
    }

    // on last sample of the window: average analog data and store
    if (config->aggregation_mode == RL_AGGREGATION_MODE_AVERAGE) {
        for (int j = 0; j < RL_CHANNEL_COUNT; j++) {
            aggregate_analog[j] = aggregate_analog[j] / buffer_size;
        }
    }

    // display data
    // clear screen
    erase();

    // display values
    mvprintw(1, 28, "RocketLogger CLI Monitor");

    mvprintw(3, 10, "Time:");
    mvprintw(3, 20, "% 12lld.%09lld (monotonic)", timestamp_monotonic->sec,
             timestamp_monotonic->nsec);
    mvprintw(4, 20, "% 12lld.%09lld (realtime)", timestamp_realtime->sec,
             timestamp_realtime->nsec);

    int current_index = 0;
    int voltage_index = 0;
    for (int i = 0; i < RL_CHANNEL_COUNT; i++) {
        if (is_current(i)) {
            // current
            mvprintw(9 + 2 * current_index, 40, "%s:", RL_CHANNEL_NAMES[i]);
            mvprintw(9 + 2 * current_index, 50, "%12.6f %s",
                     (aggregate_analog[i] / RL_CHANNEL_SCALES[i]),
                     RL_CHANNEL_UNITS[i]);
            current_index++;
        } else {
            // voltage
            mvprintw(9 + 2 * voltage_index, 10, "%s:", RL_CHANNEL_NAMES[i]);
            mvprintw(9 + 2 * voltage_index, 20, "%9.6f %s",
                     (aggregate_analog[i] / RL_CHANNEL_SCALES[i]),
                     RL_CHANNEL_UNITS[i]);
            voltage_index++;
        }
    }

    // display titles, range information
    mvprintw(7, 10, "Voltages:");
    mvprintw(7, 40, "Currents:");
    if (aggregate_digital & PRU_DIGITAL_I1L_VALID_MASK) {
        mvprintw(9, 70, "I1L valid");
    } else {
        mvprintw(9, 70, "I1L invalid");
    }
    if (aggregate_digital & PRU_DIGITAL_I2L_VALID_MASK) {
        mvprintw(13, 70, "I2L valid");
    } else {
        mvprintw(13, 70, "I2L invalid");
    }

    // digital inputs
    if (config->digital_enable) {
        mvprintw(20, 10, "Digital Inputs:");

        for (int i = 0; i < 3; i++) {
            mvprintw(20 + 2 * i, 30, "%s:", RL_CHANNEL_DIGITAL_NAMES[i]);
            mvprintw(20 + 2 * i, 38, "%d",
                     (aggregate_digital & DIGITAL_INPUT_BITS[i]) > 0);
        }
        for (int i = 3; i < 6; i++) {
            mvprintw(20 + 2 * (i - 3), 50, "%s:", RL_CHANNEL_DIGITAL_NAMES[i]);
            mvprintw(20 + 2 * (i - 3), 58, "%d",
                     (aggregate_digital & DIGITAL_INPUT_BITS[i]) > 0);
        }
    } else {
        mvprintw(20, 10, "Digital inputs disabled.");
    }

    // move cursor for warning outputs on new line
    mvprintw(27, 2, "");

    refresh();
}
