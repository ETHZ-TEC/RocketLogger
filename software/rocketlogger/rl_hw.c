/**
 * Copyright (c) 2016-2018, Swiss Federal Institute of Technology (ETH Zurich)
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
 * ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "sensor/sensor.h"

#include "rl_hw.h"

/**
 * Initiate all hardware modules
 * @param conf Pointer to current {@link rl_conf} configuration
 */
void hw_init(struct rl_conf* conf) {

    // PWM
    pwm_setup();
    if (conf->sample_rate < MIN_ADC_RATE) {
        pwm_setup_range_clock(MIN_ADC_RATE);
    } else {
        pwm_setup_range_clock(conf->sample_rate);
    }
    pwm_setup_adc_clock();

    // GPIO
    // force high range
    gpio_export(FHR1_GPIO);
    gpio_export(FHR2_GPIO);
    gpio_dir(FHR1_GPIO, OUT);
    gpio_dir(FHR2_GPIO, OUT);
    gpio_set_value(FHR1_GPIO,
                   (conf->force_high_channels[0] == CHANNEL_DISABLED));
    gpio_set_value(FHR2_GPIO,
                   (conf->force_high_channels[1] == CHANNEL_DISABLED));
    // leds
    gpio_export(LED_STATUS_GPIO);
    gpio_export(LED_ERROR_GPIO);
    gpio_dir(LED_STATUS_GPIO, OUT);
    gpio_dir(LED_ERROR_GPIO, OUT);
    gpio_set_value(LED_STATUS_GPIO, 1);
    gpio_set_value(LED_ERROR_GPIO, 0);

    // PRU
    pru_init();

    // SENSORS
    if (conf->ambient.enabled == AMBIENT_ENABLED) {
        Sensors_initSharedBus();
        conf->ambient.sensor_count =
            Sensors_scan(conf->ambient.available_sensors);
    }

    // STATE
    status.state = RL_RUNNING;
    status.sampling = SAMPLING_OFF;
    status.samples_taken = 0;
    status.buffer_number = 0;
    status.conf = *conf;
    write_status(&status);
}

/**
 * Close all hardware modules
 * @param conf Pointer to current {@link rl_conf} configuration
 */
void hw_close(struct rl_conf* conf) {

    // PWM
    pwm_close();

    // GPIO
    // force high range
    gpio_unexport(FHR1_GPIO);
    gpio_unexport(FHR2_GPIO);
    // leds (not unexport!)
    gpio_set_value(LED_STATUS_GPIO, 0);

    // PRU
    if (conf->mode != LIMIT) {
        pru_stop();
    }
    pru_close();

    // SENSORS
    if (conf->ambient.enabled == AMBIENT_ENABLED) {
        Sensors_close(conf->ambient.available_sensors);
        Sensors_closeSharedBus();
    }

    // RESET SHARED MEM
    status.samples_taken = 0;
    status.buffer_number = 0;
    write_status(&status);
}

/**
 * Hardware sampling function
 * @param conf Pointer to current {@link rl_conf} configuration
 * @param file_comment Comment to store in the file header
 * @return {@link SUCCESS} on success, {@link FAILURE} otherwise
 */
int hw_sample(struct rl_conf* conf, char* file_comment) {

    // open data file
    FILE* data = (FILE*)-1;
    if (conf->file_format != NO_FILE) { // open file only if storing requested
        data = fopen64(conf->file_name, "w+");
        if (data == NULL) {
            rl_log(ERROR, "failed to open data-file");
            return FAILURE;
        }
    }

    // open ambient file
    FILE* ambient_file = (FILE*)-1;
    if (conf->ambient.enabled == AMBIENT_ENABLED) {
        ambient_file = fopen64(conf->ambient.file_name, "w+");
        if (data == NULL) {
            rl_log(ERROR, "failed to open ambient-file");
            return FAILURE;
        }
    }

    // read calibration
    if (read_calibration(conf) == FAILURE) {
        rl_log(WARNING, "no calibration file, returning uncalibrated values");
    }

    // SAMPLE
    if (pru_sample(data, ambient_file, conf, file_comment) == FAILURE) {
        // error ocurred
        gpio_set_value(LED_ERROR_GPIO, 1);
    }

    // close data file
    if (conf->file_format != NO_FILE) {
        fclose(data);
    }
    if (conf->ambient.enabled == AMBIENT_ENABLED) {
        fclose(ambient_file);
    }

    return SUCCESS;
}
