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

#include <stdint.h>

#include <ncurses.h>

#include "calibration.h"
#include "pru.h"
#include "rl.h"
#include "util.h"

#include "meter.h"

/// Analog channel units
char const *const CHANNEL_UNITS[RL_CHANNEL_COUNT] = {"mA", "uA", "V", "V",
                                                     "mA", "uA", "V", "V"};

/// Analog channel scales
uint32_t const CHANNEL_SCALES[RL_CHANNEL_COUNT] = {
    1000000, 100000, 100000000, 100000000,
    1000000, 100000, 100000000, 100000000};

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

void meter_print_buffer(void const *const buffer, uint32_t samples_count,
                        rl_timestamp_t const *const timestamp_realtime,
                        rl_timestamp_t const *const timestamp_monotonic,
                        rl_config_t const *const config) {

    // suppress unused parameter warning
    (void)samples_count;
    (void)timestamp_realtime;
    (void)timestamp_monotonic;

    // clear screen
    erase();

    // counter variables
    uint32_t i = 0; // currents
    uint32_t v = 0; // voltages

    uint32_t num_channels = count_channels(config->channel_enable);

    // data
    int64_t value = 0;
    int32_t dig_data[2];
    int32_t channel_data[num_channels];

    // number of samples to average
    uint32_t avg_number = config->sample_rate / config->update_rate;

    // read digital channels
    dig_data[0] = (int32_t)(*((int8_t *)(buffer)));
    dig_data[1] = (int32_t)(*((int8_t *)(buffer + 1)));

    // read, average and scale values (if channel selected)
    int k = 0;
    for (int j = 0; j < RL_CHANNEL_COUNT; j++) {
        if (config->channel_enable[j]) {
            value = 0;
            for (uint32_t l = 0; l < avg_number; l++) {
                value += *((int32_t *)(buffer + PRU_DIGITAL_SIZE +
                                       j * PRU_SAMPLE_SIZE +
                                       l * (RL_CHANNEL_COUNT * PRU_SAMPLE_SIZE +
                                            PRU_DIGITAL_SIZE)));
            }
            value = value / (int64_t)avg_number;
            channel_data[k] =
                (int32_t)(((int32_t)value + rl_calibration.offsets[j]) *
                          rl_calibration.scales[j]);
            k++;
        }
    }

    // display values
    mvprintw(1, 28, "RocketLogger Interactive");

    for (int j = 0; j < RL_CHANNEL_COUNT; j++) {
        if (config->channel_enable[j]) {
            if (is_current(j)) {
                // current
                mvprintw(i * 2 + 5, 10, "%s:", RL_CHANNEL_NAMES[j]);
                mvprintw(i * 2 + 5, 15, "%12.6f%s",
                         ((float)channel_data[v + i]) / CHANNEL_SCALES[j],
                         CHANNEL_UNITS[j]);
                i++;
            } else {
                // voltage
                mvprintw(v * 2 + 5, 55, "%s:", RL_CHANNEL_NAMES[j]);
                mvprintw(v * 2 + 5, 60, "%9.6f%s",
                         ((float)channel_data[v + i]) / CHANNEL_SCALES[j],
                         CHANNEL_UNITS[j]);
                v++;
            }
        }
    }

    // display titles, range information
    if (i > 0) { // currents
        mvprintw(3, 10, "Currents:");

        // display range information
        mvprintw(3, 33, "Low range:");
        if ((dig_data[0] & PRU_VALID_MASK) > 0) {
            mvprintw(5, 33, "I1L invalid");
        } else {
            mvprintw(5, 33, "I1L valid");
        }
        if ((dig_data[1] & PRU_VALID_MASK) > 0) {
            mvprintw(11, 33, "I2L invalid");
        } else {
            mvprintw(11, 33, "I2L valid");
        }
    }

    if (v > 0) { // voltages
        mvprintw(3, 55, "Voltages:");
    }

    // digital inputs
    if (config->digital_enable) {
        mvprintw(20, 10, "Digital Inputs:");

        for (int j = 0; j < 3; j++) {
            mvprintw(20 + 2 * j, 30, "%s:", RL_CHANNEL_DIGITAL_NAMES[j]);
            mvprintw(20 + 2 * j, 38, "%d",
                     (dig_data[0] & DIGITAL_INPUT_BITS[j]) > 0);
        }
        for (int j = 3; j < 6; j++) {
            mvprintw(20 + 2 * (j - 3), 50, "%s:", RL_CHANNEL_DIGITAL_NAMES[j]);
            mvprintw(20 + 2 * (j - 3), 58, "%d",
                     (dig_data[1] & DIGITAL_INPUT_BITS[j]) > 0);
        }
    }

    refresh();
}
