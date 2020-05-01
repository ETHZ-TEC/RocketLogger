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

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <linux/limits.h>
#include <pruss_intc_mapping.h>
#include <prussdrv.h>
#include <sys/types.h>
#include <unistd.h>

#include "calibration.h"
#include "log.h"
#include "meter.h"
#include "rl.h"
#include "rl_file.h"
#include "rl_socket.h"
#include "sem.h"
#include "sensor/sensor.h"
#include "util.h"

#include "pru.h"

int pru_init(void) {
    tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;

    // initialize and open PRU device
    prussdrv_init();
    int ret = prussdrv_open(PRU_EVTOUT_0);
    if (ret != 0) {
        rl_log(RL_LOG_ERROR, "failed to open PRUSS driver");
        return ERROR;
    }

    // setup PRU interrupt mapping
    prussdrv_pruintc_init(&pruss_intc_initdata);

    return SUCCESS;
}

void pru_deinit(void) {
    // disable PRU and close memory mappings
    prussdrv_pru_disable(0);
    prussdrv_exit();
}

int pru_control_init(pru_control_t *const pru_control,
                     rl_config_t const *const config, uint32_t aggregates) {
    // zero aggregates value is also considered no aggregation
    if (aggregates == 0) {
        aggregates = 1;
    }

    // set state
    if (config->sample_limit > 0) {
        pru_control->state = PRU_STATE_SAMPLE_FINITE;
    } else {
        pru_control->state = PRU_STATE_SAMPLE_CONTINUOUS;
    }

    // set native sample rate in kSPS
    if (config->sample_rate <= RL_SAMPLE_RATE_MIN) {
        pru_control->sample_rate = RL_SAMPLE_RATE_MIN / 1000;
    } else {
        pru_control->sample_rate = config->sample_rate / 1000;
    }

    // set sample limit and data buffer size
    pru_control->sample_limit = config->sample_limit * aggregates;
    pru_control->buffer_length =
        1000 * pru_control->sample_rate / config->update_rate;

    // get shared buffer addresses
    uint32_t buffer_size_bytes =
        pru_control->buffer_length *
            (PRU_SAMPLE_SIZE * RL_CHANNEL_COUNT + PRU_DIGITAL_SIZE) +
        PRU_BUFFER_STATUS_SIZE;

    void *pru_extmem_base;
    prussdrv_map_extmem(&pru_extmem_base);
    uint32_t pru_extmem_phys =
        (uint32_t)prussdrv_get_phys_addr(pru_extmem_base);
    pru_control->buffer0_addr = pru_extmem_phys;
    pru_control->buffer1_addr = pru_extmem_phys + buffer_size_bytes;

    return SUCCESS;
}

int pru_set_state(pru_state_t state) {
    int res = prussdrv_pru_write_memory(PRUSS0_PRU0_DATARAM, 0,
                                        (unsigned int *)&state, sizeof(state));
    // PRU memory write fence
    __sync_synchronize();
    return res;
}

int pru_sample(FILE *data_file, FILE *ambient_file,
               rl_config_t const *const config) {
    int res;

    // average (for low rates)
    uint32_t aggregates = 1;
    if (config->sample_rate < RL_SAMPLE_RATE_MIN) {
        aggregates = (uint32_t)(RL_SAMPLE_RATE_MIN / config->sample_rate);
    }

    // PRU SETUP

    // initialize PRU data structure
    pru_control_t pru;
    res = pru_control_init(&pru, config, aggregates);
    if (res != SUCCESS) {
        rl_log(RL_LOG_ERROR, "failed initializing PRU data structure");
        return ERROR;
    }

    // check max PRU buffer size
    uint32_t pru_extmem_size = (uint32_t)prussdrv_extmem_size();
    uint32_t pru_extmem_size_demand =
        2 * (pru.buffer_length *
                 (PRU_SAMPLE_SIZE * RL_CHANNEL_COUNT + PRU_DIGITAL_SIZE) +
             PRU_BUFFER_STATUS_SIZE);
    if (pru_extmem_size_demand > pru_extmem_size) {
        rl_log(RL_LOG_ERROR,
               "insufficient PRU memory allocated/available.\n"
               "update uio_pruss configuration:"
               "options uio_pruss extram_pool_sz=0x%06x",
               pru_extmem_size_demand);
        pru.state = PRU_STATE_OFF;
    }

    // get user space mapped PRU memory addresses
    void const *buffer0 = prussdrv_get_virt_addr(pru.buffer0_addr);
    void const *buffer1 = prussdrv_get_virt_addr(pru.buffer1_addr);

    // data file headers
    rl_file_header_t data_file_header;
    rl_file_header_t ambient_file_header;

    // DATA FILE STORING
    if (config->file_enable) {

        // data file header lead-in
        rl_file_setup_data_lead_in(&(data_file_header.lead_in), config);

        // channel array
        int total_channel_count = data_file_header.lead_in.channel_bin_count +
                                  data_file_header.lead_in.channel_count;
        rl_file_channel_t file_channel[total_channel_count];
        data_file_header.channel = file_channel;

        // complete file header
        rl_file_setup_data_header(&data_file_header, config);

        // store header
        if (config->file_format == RL_FILE_FORMAT_RLD) {
            rl_file_store_header_bin(data_file, &data_file_header);
        } else if (config->file_format == RL_FILE_FORMAT_CSV) {
            rl_file_store_header_csv(data_file, &data_file_header);
        }

        // AMBIENT FILE STORING

        // file header lead-in
        if (config->ambient_enable) {

            rl_file_setup_ambient_lead_in(&(ambient_file_header.lead_in),
                                          config);

            // allocate channel array
            ambient_file_header.channel =
                malloc(rl_status.sensor_count * sizeof(rl_file_channel_t));

            // complete file header
            rl_file_setup_ambient_header(&ambient_file_header, config);

            // store header
            rl_file_store_header_bin(ambient_file, &ambient_file_header);
        }
    }
    // EXECUTION

    // write configuration to PRU memory
    prussdrv_pru_write_memory(PRUSS0_PRU0_DATARAM, 0, (unsigned int *)&pru,
                              sizeof(pru));

    // PRU memory write fence
    __sync_synchronize();

    // run SPI on PRU0
    res = prussdrv_exec_program(0, PRU_BINARY_FILE);
    if (res < 0) {
        rl_log(RL_LOG_ERROR, "Failed starting PRU, binary not found");
        return ERROR;
    }

    // wait for PRU event (returns 0 on timeout, -1 on error with errno)
    res = prussdrv_pru_wait_event_timeout(PRU_EVTOUT_0, PRU_TIMEOUT_US);
    if (res < 0) {
        // error checking interrupt occurred
        rl_log(RL_LOG_ERROR, "Failed waiting for PRU interrupt");
        return ERROR;
    } else if (res == 0) {
        // low level ADC timeout occurred
        rl_log(RL_LOG_ERROR, "Failed starting PRU, PRU not responding");
        return ERROR;
    }

    // initialize interactive measurement display when enabled
    if (config->interactive_enable) {
        meter_init();
    }

    // create daemon after if requested to run in background
    if (config->background_enable) {
        if (daemon(1, 1) < 0) {
            rl_log(RL_LOG_ERROR, "failed to create background process");
            return ERROR;
        }
    }

    // write PID in file (only after potential forking using daemon)
    pid_t pid = getpid();
    rl_pid_set(pid);

    // clear event
    prussdrv_pru_clear_event(PRU_EVTOUT_0, PRU0_ARM_INTERRUPT);

    // CHANNEL DATA MEMORY ALLOCATION
    int32_t *const analog_buffer = (int32_t *)malloc(
        pru.buffer_length * RL_CHANNEL_COUNT * sizeof(int32_t));
    uint32_t *const digital_buffer =
        (uint32_t *)malloc(pru.buffer_length * sizeof(uint32_t));
    int32_t sensor_buffer[SENSOR_REGISTRY_SIZE];

    pru_buffer_t const *pru_buffer = NULL;
    size_t buffer_size; // number of data samples per buffer
    size_t sensor_buffer_size = 0;
    uint32_t sensor_rate_counter = 0;
    rl_timestamp_t timestamp_monotonic;
    rl_timestamp_t timestamp_realtime;
    uint32_t buffers_lost = 0;
    uint32_t num_files = 1; // number of files stored
    bool web_failure_disable = false;

    // buffers to read in finite mode
    uint32_t buffer_read_count =
        div_ceil(config->sample_limit * aggregates, pru.buffer_length);

    // sampling started
    rl_status.sampling = true;
    res = rl_status_write(&rl_status);
    if (res < 0) {
        rl_log(RL_LOG_WARNING, "Failed writing status");
    }

    // continuous sampling loop
    for (uint32_t i = 0; rl_status.sampling &&
                         !(config->sample_limit > 0 && i >= buffer_read_count);
         i++) {

        if (config->file_enable) {

            // check if max file size reached
            uint64_t file_size = (uint64_t)ftello(data_file);
            if (config->file_size > 0 &&
                file_size + rl_status.disk_use_rate > config->file_size) {

                // close old data file
                fclose(data_file);

                // determine new file name
                char file_name[PATH_MAX];
                char new_file_ending[PATH_MAX];
                strcpy(file_name, config->file_name);

                // search for last .
                char target = '.';
                char *file_ending = file_name;
                while (strchr(file_ending, target) != NULL) {
                    file_ending = strchr(file_ending, target);
                    file_ending++; // Increment file_ending, otherwise we'll
                                   // find target at the same location
                }
                file_ending--;

                // add file number
                sprintf(new_file_ending, "_p%d", num_files);
                strcat(new_file_ending, file_ending);
                strcpy(file_ending, new_file_ending);

                // open new data file
                data_file = fopen64(file_name, "w+");

                // update header for new file
                data_file_header.lead_in.data_block_count = 0;
                data_file_header.lead_in.sample_count = 0;

                // store header
                if (config->file_format == RL_FILE_FORMAT_RLD) {
                    rl_file_store_header_bin(data_file, &data_file_header);
                } else if (config->file_format == RL_FILE_FORMAT_CSV) {
                    rl_file_store_header_csv(data_file, &data_file_header);
                }

                rl_log(RL_LOG_INFO, "Creating new data file: %s", file_name);

                // AMBIENT FILE
                if (config->ambient_enable) {
                    // close old ambient file
                    fclose(ambient_file);

                    // determine new file name
                    char *ambient_file_name =
                        rl_file_get_ambient_file_name(config->file_name);
                    strcpy(file_name, ambient_file_name);

                    // search for last .
                    file_ending = file_name;
                    while (strchr(file_ending, target) != NULL) {
                        file_ending = strchr(file_ending, target);
                        file_ending++; // Increment file_ending, otherwise we'll
                                       // find target at the same location
                    }
                    file_ending--;

                    // add file number
                    sprintf(new_file_ending, "_p%d", num_files);
                    strcat(new_file_ending, file_ending);
                    strcpy(file_ending, new_file_ending);

                    // open new ambient file
                    ambient_file = fopen64(file_name, "w+");

                    // update header for new file
                    ambient_file_header.lead_in.data_block_count = 0;
                    ambient_file_header.lead_in.sample_count = 0;

                    // store header
                    rl_file_store_header_bin(ambient_file,
                                             &ambient_file_header);

                    rl_log(RL_LOG_INFO, "new ambient-file: %s", file_name);
                }

                num_files++;
            }
        }

        // select current buffer
        if (i % 2 == 0) {
            pru_buffer = buffer0;
        } else {
            pru_buffer = buffer1;
        }

        // select buffer size, repecting non-full last buffer
        if (i < buffer_read_count - 1 ||
            pru.sample_limit % pru.buffer_length == 0) {
            buffer_size = (size_t)pru.buffer_length;
        } else {
            // last buffer is not fully used
            buffer_size = (size_t)(pru.sample_limit % pru.buffer_length);
        }

        // trigger new ambient sensor read out if enabled
        sensor_buffer_size = 0;
        if (config->ambient_enable) {
            // rate limit sampling of the ambient sensors
            if (sensor_rate_counter < RL_SENSOR_SAMPLE_RATE) {
                // trigger sensor readout
                sensor_buffer_size =
                    sensors_read(sensor_buffer, rl_status.sensor_available);
            }
            // increment rate limiting counter
            sensor_rate_counter =
                (sensor_rate_counter + RL_SENSOR_SAMPLE_RATE) %
                config->update_rate;
        }

        // wait for PRU event indicating new data (repeat wait on interrupts)
        do {
            // wait for PRU event (returns 0 on timeout, -1 on error with errno)
            res = prussdrv_pru_wait_event_timeout(PRU_EVTOUT_0, PRU_TIMEOUT_US);
            // timestamp received data
            create_time_stamp(&timestamp_realtime, &timestamp_monotonic);
        } while (res < 0 && errno == EINTR);
        if (res < 0) {
            // error checking interrupt occurred
            rl_log(RL_LOG_ERROR, "Failed waiting for PRU interrupt", errno);
            rl_status.error = true;
            break;
        } else if (res == 0) {
            // low level ADC timeout occurred
            rl_log(RL_LOG_ERROR, "ADC not responsive");
            rl_status.error = true;
            break;
        }

        // adjust data timestamps with buffer latency (adjusted relative
        // nanoseconds)
        timestamp_realtime.nsec -= (int64_t)2048e3 * 490 / config->update_rate;
        if (timestamp_realtime.nsec < 0) {
            timestamp_realtime.sec -= 1;
            timestamp_realtime.nsec += (int64_t)1e9;
        }
        timestamp_monotonic.nsec -= (int64_t)2048e3 * 490 / config->update_rate;
        if (timestamp_monotonic.nsec < 0) {
            timestamp_monotonic.sec -= 1;
            timestamp_monotonic.nsec += (int64_t)1e9;
        }

        // clear event
        prussdrv_pru_clear_event(PRU_EVTOUT_0, PRU0_ARM_INTERRUPT);

        // PRU memory sync before accessing data
        __sync_synchronize();

        // check for overrun (compare buffer numbers)
        if (pru_buffer->index != i) {
            buffers_lost += (pru_buffer->index - i);
            rl_log(RL_LOG_WARNING,
                   "overrun: %d samples (%d buffer) lost (%d in total)",
                   (pru_buffer->index - i) * pru.buffer_length,
                   pru_buffer->index - i, buffers_lost);
            i = pru_buffer->index;
        }

        // process new data: copy data and apply calibration
        for (size_t i = 0; i < buffer_size; i++) {
            // get PRU data buffer pointer
            pru_data_t const *const pru_data = &(pru_buffer->data[i]);

            // get local data buffer pointers
            int32_t *const analog_data = analog_buffer + i * RL_CHANNEL_COUNT;
            uint32_t *const digital_data = digital_buffer + i;

            // copy digital channel data
            *digital_data = pru_data->channel_digital;

            // copy and calibrate analog channel data
            for (int j = 0; j < RL_CHANNEL_COUNT; j++) {
                analog_data[j] = (int32_t)(
                    (pru_data->channel_analog[j] + rl_calibration.offsets[j]) *
                    rl_calibration.scales[j]);
            }
        }

        // update and write state
        rl_status.sample_count += buffer_size / aggregates;
        rl_status.buffer_count = i + 1 - buffers_lost;
        res = rl_status_write(&rl_status);
        if (res < 0) {
            rl_log(RL_LOG_WARNING, "Failed writing status");
        }

        // process data for web when enabled
        if (config->web_enable && !web_failure_disable) {
            res = rl_socket_handle_data(analog_buffer, digital_buffer,
                                        buffer_size, &timestamp_realtime,
                                        &timestamp_monotonic, config);
            if (res < 0) {
                // disable web interface on failure, but continue sampling
                web_failure_disable = true;
                rl_log(RL_LOG_WARNING, "Web server connection failed. "
                                       "Disabling web interface.");
            }
        }

        // update and write header
        if (config->file_enable) {
            // write the data buffer to file
            int block_count = rl_file_add_data_block(
                data_file, analog_buffer, digital_buffer, buffer_size,
                &timestamp_realtime, &timestamp_monotonic, config);

            // stop sampling on file error
            if (block_count < 0) {
                rl_log(RL_LOG_ERROR, "Adding data block to data file failed.");
                rl_status.error = true;
                break;
            }

            // update and store data file header
            data_file_header.lead_in.data_block_count += block_count;
            data_file_header.lead_in.sample_count +=
                block_count * (buffer_size / aggregates);

            if (config->file_format == RL_FILE_FORMAT_RLD) {
                rl_file_update_header_bin(data_file, &data_file_header);
            } else if (config->file_format == RL_FILE_FORMAT_CSV) {
                rl_file_update_header_csv(data_file, &data_file_header);
            }
        }

        // handle ambient data if enabled and available
        if (config->ambient_enable && sensor_buffer_size > 0) {
            // fetch and write data
            int block_count = rl_file_add_ambient_block(
                ambient_file, sensor_buffer, sensor_buffer_size,
                &timestamp_realtime, &timestamp_monotonic, config);

            // stop sampling on file error
            if (block_count < 0) {
                rl_log(RL_LOG_ERROR,
                       "Adding data block to ambient file failed.");
                rl_status.error = true;
                break;
            }

            // update and write header
            ambient_file_header.lead_in.data_block_count += block_count;
            ambient_file_header.lead_in.sample_count +=
                block_count * RL_FILE_AMBIENT_DATA_BLOCK_SIZE;
            rl_file_update_header_bin(ambient_file, &ambient_file_header);
        }

        // print meter output if enabled
        if (config->interactive_enable) {
            meter_print_buffer(analog_buffer, digital_buffer, buffer_size,
                               &timestamp_realtime, &timestamp_monotonic,
                               config);
        }
    }

    // stop PRU
    pru_stop();

    // sampling stopped, update status
    rl_status.sampling = false;
    res = rl_status_write(&rl_status);
    if (res < 0) {
        rl_log(RL_LOG_WARNING, "Failed writing status");
    }

    // CLEANUP CHANNEL DATA MEMORY ALLOCATION
    free(analog_buffer);
    free(digital_buffer);

    // deinitialize interactive measurement display when enabled
    if (config->interactive_enable) {
        meter_deinit();
    }

    // FILE FINISH (flush)
    // flush ambient data and cleanup file header
    if (config->ambient_enable) {
        fflush(ambient_file);
        free(ambient_file_header.channel);
    }

    if (config->file_enable && !(rl_status.error)) {
        fflush(data_file);
        rl_log(RL_LOG_INFO, "stored %llu samples to file",
               rl_status.sample_count);
    }

    // STATE
    if (rl_status.error) {
        return ERROR;
    }

    return SUCCESS;
}

void pru_stop(void) {

    // write OFF to PRU state (so PRU can clean up)
    pru_set_state(PRU_STATE_OFF);

    // wait for interrupt (if no ERROR occurred) and clear event
    if (!(rl_status.error)) {
        // wait for PRU event (returns 0 on timeout, -1 on error with errno)
        prussdrv_pru_wait_event_timeout(PRU_EVTOUT_0, PRU_TIMEOUT_US);
        prussdrv_pru_clear_event(PRU_EVTOUT_0, PRU0_ARM_INTERRUPT);
    }
}
