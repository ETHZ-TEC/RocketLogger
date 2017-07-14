/**
 * Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group
 */

#include "meter.h"

/// Analog channel units
char* channel_units[NUM_CHANNELS] = {"mA", "uA", "V", "V",
                                     "mA", "uA", "V", "V"};
/// Analog channel scales
uint32_t channel_scales[NUM_CHANNELS] = {1000000, 100000, 100000000, 100000000,
                                         1000000, 100000, 100000000, 100000000};
/// Digital input bit location in binary data
const uint32_t digital_input_bits[NUM_DIGITAL_INPUTS] = {
    DIGIN1_BIT, DIGIN2_BIT, DIGIN3_BIT, DIGIN4_BIT, DIGIN5_BIT, DIGIN6_BIT};

/**
 * Init meter window
 */
void meter_init(void) {
    // init ncurses mode
    initscr();
    // hide cursor
    curs_set(0);

    mvprintw(1, 1, "Starting RocketLogger Meter ...");
    refresh();
}

/**
 * Stop meter window
 */
void meter_stop(void) { endwin(); }

/**
 * Print data buffer in meter window
 * @param conf Pointer to current {@link rl_conf} configuration
 * @param buffer_addr
 * @param sample_size Size of samples in buffer
 */
void meter_print_buffer(struct rl_conf* conf, void* buffer_addr,
                        uint32_t sample_size) {

    // clear screen
    erase();

    // counter variables
    uint32_t j = 0;
    uint32_t k = 0;
    uint32_t l = 0;
    uint32_t i = 0; // currents
    uint32_t v = 0; // voltages

    uint32_t num_channels = count_channels(conf->channels);

    // data
    int64_t value = 0;
    int32_t dig_data[2];
    int32_t channel_data[num_channels];

    // number of samples to average
    uint32_t avg_number = conf->sample_rate / conf->update_rate;

    // read digital channels
    dig_data[0] = (int32_t)(*((int8_t*)(buffer_addr)));
    dig_data[1] = (int32_t)(*((int8_t*)(buffer_addr + 1)));
    buffer_addr += PRU_DIG_SIZE;

    // read, average and scale values (if channel selected)
    for (j = 0; j < NUM_CHANNELS; j++) {
        if (conf->channels[j] == CHANNEL_ENABLED) {
            value = 0;
            if (sample_size == 4) {
                for (l = 0; l < avg_number; l++) {
                    value += *((int32_t*)(buffer_addr + sample_size * j +
                                          l * (NUM_CHANNELS * sample_size +
                                               PRU_DIG_SIZE)));
                }
            } else {
                for (l = 0; l < avg_number; l++) {
                    value += *((int16_t*)(buffer_addr + sample_size * j +
                                          l * (NUM_CHANNELS * sample_size +
                                               PRU_DIG_SIZE)));
                }
            }
            value = value / (int64_t)avg_number;
            channel_data[k] =
                (int32_t)(((int32_t)value + calibration.offsets[j]) *
                          calibration.scales[j]);
            k++;
        }
    }

    // display values
    mvprintw(1, 28, "RocketLogger Meter");

    for (j = 0; j < NUM_CHANNELS; j++) {
        if (conf->channels[j] == CHANNEL_ENABLED) {
            if (is_current(j)) {
                // current
                mvprintw(i * 2 + 5, 10, "%s:", channel_names[j]);
                mvprintw(i * 2 + 5, 15, "%12.6f%s",
                         ((float)channel_data[v + i]) / channel_scales[j],
                         channel_units[j]);
                i++;
            } else {
                // voltage
                mvprintw(v * 2 + 5, 55, "%s:", channel_names[j]);
                mvprintw(v * 2 + 5, 60, "%9.6f%s",
                         ((float)channel_data[v + i]) / channel_scales[j],
                         channel_units[j]);
                v++;
            }
        }
    }

    // display titles, range information
    if (i > 0) { // currents
        mvprintw(3, 10, "Currents:");

        // display range information
        mvprintw(3, 33, "Low range:");
        if ((dig_data[0] & I1L_VALID_BIT) > 0) {
            mvprintw(5, 33, "I1L invalid");
        } else {
            mvprintw(5, 33, "I1L valid");
        }
        if ((dig_data[1] & I2L_VALID_BIT) > 0) {
            mvprintw(11, 33, "I2L invalid");
        } else {
            mvprintw(11, 33, "I2L valid");
        }
    }

    if (v > 0) { // voltages
        mvprintw(3, 55, "Voltages:");
    }

    // digital inputs
    if (conf->digital_inputs > 0) {
        mvprintw(20, 10, "Digital Inputs:");

        j = 0;
        for (; j < 3; j++) {
            mvprintw(20 + 2 * j, 30, "%s:", digital_input_names[j]);
            mvprintw(20 + 2 * j, 38, "%d",
                     (dig_data[0] & digital_input_bits[j]) > 0);
        }
        for (; j < 6; j++) {
            mvprintw(20 + 2 * (j - 3), 50, "%s:", digital_input_names[j]);
            mvprintw(20 + 2 * (j - 3), 58, "%d",
                     (dig_data[1] & digital_input_bits[j]) > 0);
        }
    }

    refresh();
}
