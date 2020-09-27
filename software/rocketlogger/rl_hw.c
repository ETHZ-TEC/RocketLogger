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

#include <stdio.h>

#include "calibration.h"
#include "gpio.h"
#include "log.h"
#include "pru.h"
#include "rl.h"
#include "rl_file.h"
#include "sensor/sensor.h"

#include "rl_hw.h"

void hw_init(rl_config_t const *const config) {
    // GPIO configuration
    // force high range (negative enable)
    gpio_init(GPIO_FHR1, GPIO_MODE_OUT);
    gpio_init(GPIO_FHR2, GPIO_MODE_OUT);
    gpio_set_value(GPIO_FHR1, (config->channel_force_range[0] ? 0 : 1));
    gpio_set_value(GPIO_FHR2, (config->channel_force_range[1] ? 0 : 1));
    // leds
    gpio_init(GPIO_LED_STATUS, GPIO_MODE_OUT);
    gpio_init(GPIO_LED_ERROR, GPIO_MODE_OUT);
    gpio_set_value(GPIO_LED_STATUS, 1);
    gpio_set_value(GPIO_LED_ERROR, 0);

    // PRU
    pru_init();

    // SENSORS (if enabled)
    if (config->ambient_enable) {
        sensors_init();
        rl_status.sensor_count = sensors_scan(rl_status.sensor_available);
    }

    // STATE
    if (config->file_enable) {
        // calculate disk use rate in bytes per second:
        // - int32_t/channel + uint32_t bytes/sample for digital at sample rate
        // - 2 timestamp at update rate
        // - int32_t/sensor channel + 2 timestamps at 1 Hz
        rl_status.disk_use_rate =
            (sizeof(int32_t) * count_channels(config->channel_enable) *
             config->sample_rate) +
            (sizeof(rl_timestamp_t) * 2 * config->update_rate) +
            (sizeof(uint32_t) * (config->digital_enable ? 1 : 0) *
             config->sample_rate) +
            (sizeof(int32_t) * rl_status.sensor_count) +
            (sizeof(rl_timestamp_t) * 2 * (rl_status.sensor_count > 0 ? 1 : 0));
    }
    rl_status_write(&rl_status);
}

void hw_deinit(rl_config_t const *const config) {

    // GPIO (set to default state only, (un)export is handled by daemon)
    // reset force high range GPIOs to force high range (negative enable)
    gpio_set_value(GPIO_FHR1, 0);
    gpio_set_value(GPIO_FHR2, 0);

    // reset status LED, leave error LED in current state
    gpio_set_value(GPIO_LED_STATUS, 0);

    // PRU
    // stop first if running in background
    if (config->background_enable) {
        pru_stop();
    }
    pru_deinit();

    // SENSORS (if enabled)
    if (config->ambient_enable) {
        sensors_close(rl_status.sensor_available);
        sensors_deinit();
    }

    // RESET sampling specific state
    rl_status.disk_use_rate = 0;
    rl_status_write(&rl_status);
}

int hw_sample(rl_config_t const *const config) {
    int ret;
    FILE *data_file = (FILE *)-1;
    FILE *ambient_file = (FILE *)-1;

    // reset calibration if ignored, load otherwise
    if (config->calibration_ignore) {
        calibration_reset_offsets();
        calibration_reset_scales();
    } else {
        ret = calibration_load();
        if (ret < 0) {
            rl_log(RL_LOG_WARNING,
                   "no calibration file, returning uncalibrated values");
        }
    }

    // create/open measurement files
    if (config->file_enable) {
        data_file = fopen64(config->file_name, "w+");
        if (data_file == NULL) {
            rl_log(RL_LOG_ERROR, "failed to open data file '%s'",
                   config->file_name);
            return ERROR;
        }
    }

    if (config->ambient_enable) {
        char *ambient_file_name =
            rl_file_get_ambient_file_name(config->file_name);
        ambient_file = fopen64(ambient_file_name, "w+");
        if (data_file == NULL) {
            rl_log(RL_LOG_ERROR, "failed to open ambient file '%s'",
                   ambient_file);
            return ERROR;
        }
    }

    // SAMPLE
    ret = pru_sample(data_file, ambient_file, config);
    if (ret < 0) {
        // error occurred
        gpio_set_value(GPIO_LED_ERROR, 1);
    }

    // close data files
    if (config->file_enable) {
        fclose(data_file);
    }
    if (config->ambient_enable) {
        fclose(ambient_file);
    }

    return SUCCESS;
}
