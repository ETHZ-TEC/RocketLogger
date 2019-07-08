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
char const *const RL_CHANNEL_UNITS[RL_CHANNEL_COUNT] = {"V",  "V",  "V",  "V",
                                                        "uA", "mA", "uA", "mA"};

/// Analog channel scales
double const RL_CHANNEL_SCALES[RL_CHANNEL_COUNT] = {
    100000000, 100000000, 100000000, 100000000,
    100000,    1000000,   100000,    1000000};

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

void meter_print_buffer(pru_buffer_t const *const buffer, uint32_t buffer_size,
                        rl_timestamp_t const *const timestamp_realtime,
                        rl_timestamp_t const *const timestamp_monotonic,
                        rl_config_t const *const config) {

    // data
    uint8_t dig_data[2] = {0};
    double channel_data[RL_CHANNEL_COUNT] = {0};

    // read digital channels
    for (uint32_t i = 0; i < buffer_size; i++) {
        pru_data_t const pru_data = buffer->data[i];
        dig_data[0] |= (uint8_t)(pru_data.channel_digital & 0xff);
        dig_data[1] |= (uint8_t)((pru_data.channel_digital >> 8) & 0xff);
    }

    // read, average and scale values (if channel selected)
    int channel_index = 0;
    for (int i = 0; i < RL_CHANNEL_COUNT; i++) {
        if (config->channel_enable[i]) {
            int64_t value = 0;
            for (uint32_t j = 0; j < buffer_size; j++) {
                pru_data_t const pru_data = buffer->data[j];
                value += pru_data.channel_analog[i];
            }
            value = value / (int64_t)buffer_size;

            channel_data[i] =
                (double)(value + (int64_t)rl_calibration.offsets[i]) *
                rl_calibration.scales[i];
            channel_index++;
        }
    }

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
                     (channel_data[i] / RL_CHANNEL_SCALES[i]),
                     RL_CHANNEL_UNITS[i]);
            current_index++;
        } else {
            // voltage
            mvprintw(9 + 2 * voltage_index, 10, "%s:", RL_CHANNEL_NAMES[i]);
            mvprintw(9 + 2 * voltage_index, 20, "%9.6f %s",
                     (channel_data[i] / RL_CHANNEL_SCALES[i]),
                     RL_CHANNEL_UNITS[i]);
            voltage_index++;
        }
    }

    // display titles, range information
    mvprintw(7, 10, "Voltages:");
    mvprintw(7, 40, "Currents:");
    if ((dig_data[0] & PRU_VALID_MASK) > 0) {
        mvprintw(9, 70, "I1L invalid");
    } else {
        mvprintw(9, 70, "I1L valid");
    }
    if ((dig_data[1] & PRU_VALID_MASK) > 0) {
        mvprintw(13, 70, "I2L invalid");
    } else {
        mvprintw(13, 70, "I2L valid");
    }

    // digital inputs
    if (config->digital_enable) {
        mvprintw(20, 10, "Digital Inputs:");

        for (int i = 0; i < 3; i++) {
            mvprintw(20 + 2 * i, 30, "%s:", RL_CHANNEL_DIGITAL_NAMES[i]);
            mvprintw(20 + 2 * i, 38, "%d",
                     (dig_data[0] & DIGITAL_INPUT_BITS[i]) > 0);
        }
        for (int i = 3; i < 6; i++) {
            mvprintw(20 + 2 * (i - 3), 50, "%s:", RL_CHANNEL_DIGITAL_NAMES[i]);
            mvprintw(20 + 2 * (i - 3), 58, "%d",
                     (dig_data[1] & DIGITAL_INPUT_BITS[i]) > 0);
        }
    } else {
        mvprintw(20, 10, "Digital inputs disabled.");
    }

    refresh();
}
