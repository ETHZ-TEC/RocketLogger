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

#include <stdio.h>

#include "calibration.h"
#include "gpio.h"
#include "log.h"
#include "pru.h"
#include "pwm.h"
#include "sensor/sensor.h"
#include "types.h"
#include "util.h"

#include "rl_hw.h"

void hw_init(rl_config_t *const config) {

    // PWM configuration
    pwm_init();

    if (config->sample_rate < MIN_ADC_RATE) {
        pwm_setup_range_reset(MIN_ADC_RATE);
    } else {
        pwm_setup_range_reset(config->sample_rate);
    }
    pwm_setup_adc_clock();

    // GPIO configuration
    // force high range
    gpio_init(GPIO_FHR1, GPIO_MODE_OUT);
    gpio_init(GPIO_FHR2, GPIO_MODE_OUT);
    gpio_set_value(GPIO_FHR1,
                   (config->force_high_channels[0]));
    gpio_set_value(GPIO_FHR2,
                   (config->force_high_channels[1]));
    // leds
    gpio_init(GPIO_LED_STATUS, GPIO_MODE_OUT);
    gpio_init(GPIO_LED_ERROR, GPIO_MODE_OUT);
    gpio_set_value(GPIO_LED_STATUS, 1);
    gpio_set_value(GPIO_LED_ERROR, 0);

    // PRU
    pru_init();

    // SENSORS
    if (config->ambient.enabled) {
        sensors_init();
        config->ambient.sensor_count =
            sensors_scan(config->ambient.available_sensors);
    }

    // STATE
    status.state = RL_RUNNING;
    status.sampling = RL_SAMPLING_OFF;
    status.samples_taken = 0;
    status.buffer_number = 0;
    status.config = *config;
    write_status(&status);
}

void hw_deinit(rl_config_t const *const config) {

    // PWM
    pwm_deinit();

    // GPIO
    // deinitialize force high range GPIOS
    gpio_deinit(GPIO_FHR1);
    gpio_deinit(GPIO_FHR2);
    // reset LED (do not unexport)
    gpio_set_value(GPIO_LED_STATUS, 0);

    // PRU
    if (config->mode != LIMIT) {
        pru_stop();
    }
    pru_deinit();

    // SENSORS
    if (config->ambient.enabled) {
        sensors_close(config->ambient.available_sensors);
        sensors_deinit();
    }

    // RESET SHARED MEM
    status.samples_taken = 0;
    status.buffer_number = 0;
    write_status(&status);
}

int hw_sample(rl_config_t const *const config, char const *const file_comment) {
    int ret;
    // open data file
    FILE *data = (FILE *)-1;
    if (config->file_format != RL_FILE_NONE) { // open file only if storing requested
        data = fopen64(config->file_name, "w+");
        if (data == NULL) {
            rl_log(ERROR, "failed to open data-file");
            return FAILURE;
        }
    }

    // open ambient file
    FILE *ambient_file = (FILE *)-1;
    if (config->ambient.enabled) {
        ambient_file = fopen64(config->ambient.file_name, "w+");
        if (data == NULL) {
            rl_log(ERROR, "failed to open ambient-file");
            return FAILURE;
        }
    }

    // read calibration
    ret = calibration_load(config);
    if (ret != SUCCESS) {
        rl_log(WARNING, "no calibration file, returning uncalibrated values");
    }

    // SAMPLE
    ret = pru_sample(data, ambient_file, config, file_comment);
    if (ret != SUCCESS) {
        // error occurred
        gpio_set_value(GPIO_LED_ERROR, 1);
    }

    // close data file
    if (config->file_format != RL_FILE_NONE) {
        fclose(data);
    }
    if (config->ambient.enabled) {
        fclose(ambient_file);
    }

    return SUCCESS;
}
