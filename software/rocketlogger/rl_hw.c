/**
 * Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group
 */

#include "rl_hw.h"

/**
 * Initiate all hardware modules
 * @param conf Pointer to current {@link rl_conf} configuration
 */
void hw_init(struct rl_conf* conf) {

    // PWM
    pwm_setup();
    if (conf->sample_rate < MIN_ADC_RATE) {
        range_clock_setup(MIN_ADC_RATE);
    } else {
        range_clock_setup(conf->sample_rate);
    }
    adc_clock_setup();

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

    // RESET SHARED MEM
    status.samples_taken = 0;
    status.buffer_number = 0;
    write_status(&status);
}

/**
 * Hardware sampling function
 * @param conf Pointer to current {@link rl_conf} configuration
 * @return {@link SUCCESS} on success, {@link FAILURE} otherwise
 */
int hw_sample(struct rl_conf* conf) {

    // open data file
    FILE* data = (FILE*)-1;
    if (conf->file_format != NO_FILE) { // open file only if storing requested
        data = fopen(conf->file_name, "w+");
        if (data == NULL) {
            rl_log(ERROR, "failed to open data-file");
            return FAILURE;
        }
    }

    // read calibration
    if (read_calibration(conf) == FAILURE) {
        rl_log(WARNING, "no calibration file, returning uncalibrated values");
    }

    // SAMPLE
    if (pru_sample(data, conf) == FAILURE) {
        // error ocurred
        gpio_set_value(LED_ERROR_GPIO, 1);
    }

    // close data file
    if (conf->file_format != NO_FILE) {
        fclose(data);
    }

    return SUCCESS;
}
