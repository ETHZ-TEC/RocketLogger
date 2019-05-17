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

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pruss_intc_mapping.h>
#include <prussdrv.h>
#include <pthread.h>
#include <sys/types.h>

#include "ads131e0x.h"
#include "ambient.h"
#include "file_handling.h"
#include "log.h"
#include "meter.h"
#include "sem.h"
#include "types.h"
#include "util.h"
#include "web.h"

#include "pru.h"

/**
 * Internal pthread for waiting on PRU events.
 *
 * @param voidEvent Pointer to event to wait for
 * @return unused, always returns NULL
 */
void *pru_wait_event_thread(void *);

/// PRU access mutex for timed out PRU event wait
pthread_mutex_t waiting = PTHREAD_MUTEX_INITIALIZER;

/// Notification variable for PRU event wait
pthread_cond_t done = PTHREAD_COND_INITIALIZER;

int pru_init(void) {
    tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;

    // initialize and open PRU device
    prussdrv_init();
    int ret = prussdrv_open(PRU_EVTOUT_0);
    if (ret != 0) {
        rl_log(ERROR, "failed to open PRUSS driver");
        return FAILURE;
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

int pru_data_init(pru_data_t *const pru_data, rl_config_t const *const config,
                  uint32_t aggregates) {
    // zero aggregates value is also considered no aggregation
    if (aggregates == 0) {
        aggregates = 1;
    }

    // set state
    if (config->mode == LIMIT) {
        pru_data->state = PRU_FINITE;
    } else {
        pru_data->state = PRU_CONTINUOUS;
    }

    // set sampling rate configuration
    uint32_t adc_sample_rate;
    switch (config->sample_rate) {
    case 1:
        adc_sample_rate = ADS131E0X_K1;
        break;
    case 10:
        adc_sample_rate = ADS131E0X_K1;
        break;
    case 100:
        adc_sample_rate = ADS131E0X_K1;
        break;
    case 1000:
        adc_sample_rate = ADS131E0X_K1;
        break;
    case 2000:
        adc_sample_rate = ADS131E0X_K2;
        break;
    case 4000:
        adc_sample_rate = ADS131E0X_K4;
        break;
    case 8000:
        adc_sample_rate = ADS131E0X_K8;
        break;
    case 16000:
        adc_sample_rate = ADS131E0X_K16;
        break;
    case 32000:
        adc_sample_rate = ADS131E0X_K32;
        break;
    case 64000:
        adc_sample_rate = ADS131E0X_K64;
        break;
    default:
        rl_log(ERROR, "invalid sample rate %d", config->sample_rate);
        return FAILURE;
    }

    // set data format precision depending on sample rate
    if (adc_sample_rate == ADS131E0X_K32 || adc_sample_rate == ADS131E0X_K64) {
        pru_data->adc_precision = ADS131E0X_PRECISION_LOW;
    } else {
        pru_data->adc_precision = ADS131E0X_PRECISION_HIGH;
    }

    // set buffer infos
    pru_data->sample_limit = config->sample_limit * aggregates;
    pru_data->buffer_length =
        (config->sample_rate * aggregates) / config->update_rate;

    uint32_t buffer_size_bytes =
        pru_data->buffer_length *
            (PRU_SAMPLE_SIZE * NUM_CHANNELS + PRU_DIG_SIZE) +
        PRU_BUFFER_STATUS_SIZE;

    void *pru_extmem_base;
    prussdrv_map_extmem(&pru_extmem_base);
    uint32_t pru_extmem_phys =
        (uint32_t)prussdrv_get_phys_addr(pru_extmem_base);
    pru_data->buffer0_ptr = pru_extmem_phys;
    pru_data->buffer1_ptr = pru_extmem_phys + buffer_size_bytes;

    // setup commands to send for ADC initialization
    pru_data->adc_command_count = PRU_ADC_COMMAND_COUNT;

    // reset and stop sampling
    pru_data->adc_command[0] = (ADS131E0X_RESET << 24);
    pru_data->adc_command[1] = (ADS131E0X_SDATAC << 24);

    // configure registers
    pru_data->adc_command[2] = ((ADS131E0X_WREG | ADS131E0X_CONFIG3) << 24) |
                               (ADS131E0X_CONFIG3_DEFAULT << 8);
    pru_data->adc_command[3] =
        ((ADS131E0X_WREG | ADS131E0X_CONFIG1) << 24) |
        ((ADS131E0X_CONFIG1_DEFAULT | adc_sample_rate) << 8);

    // set channel gains (CH1-7, CH8 unused)
    pru_data->adc_command[4] = ((ADS131E0X_WREG | ADS131E0X_CH1SET) << 24) |
                               (ADS131E0X_GAIN2 << 8); // High Range A
    pru_data->adc_command[5] = ((ADS131E0X_WREG | ADS131E0X_CH2SET) << 24) |
                               (ADS131E0X_GAIN2 << 8); // High Range B
    pru_data->adc_command[6] = ((ADS131E0X_WREG | ADS131E0X_CH3SET) << 24) |
                               (ADS131E0X_GAIN1 << 8); // Medium Range
    pru_data->adc_command[7] = ((ADS131E0X_WREG | ADS131E0X_CH4SET) << 24) |
                               (ADS131E0X_GAIN1 << 8); // Low Range A
    pru_data->adc_command[8] = ((ADS131E0X_WREG | ADS131E0X_CH5SET) << 24) |
                               (ADS131E0X_GAIN1 << 8); // Low Range B
    pru_data->adc_command[9] = ((ADS131E0X_WREG | ADS131E0X_CH6SET) << 24) |
                               (ADS131E0X_GAIN1 << 8); // Voltage 1
    pru_data->adc_command[10] = ((ADS131E0X_WREG | ADS131E0X_CH7SET) << 24) |
                                (ADS131E0X_GAIN1 << 8); // Voltage 2

    // start continuous reading
    pru_data->adc_command[11] = (ADS131E0X_RDATAC << 24);

    return SUCCESS;
}

int pru_set_state(pru_state_t state) {
    int res = prussdrv_pru_write_memory(PRUSS0_PRU0_DATARAM, 0,
                                        (unsigned int *)&state, sizeof(state));
    // PRU memory write fence
    __sync_synchronize();
    return res;
}

void *pru_wait_event_thread(void *voidEvent) {
    unsigned int event = *((unsigned int *)voidEvent);

    // allow the thread to be killed at any time
    int oldtype;
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);

    // wait for pru event
    prussdrv_pru_wait_event(event);

    // notify main program
    pthread_cond_signal(&done);

    return NULL;
}

int pru_wait_event_timeout(unsigned int event, unsigned int timeout) {
    struct timespec abs_time;
    pthread_t tid;

    pthread_mutex_lock(&waiting);

    // pthread conditional_timed wait expects an absolute time to wait until
    clock_gettime(CLOCK_REALTIME, &abs_time);
    abs_time.tv_sec += timeout;

    pthread_create(&tid, NULL, pru_wait_event_thread, (void *)&event);

    int res = pthread_cond_timedwait(&done, &waiting, &abs_time);
    if (res <= 0) {
        pthread_mutex_unlock(&waiting);
    }

    return res;
}

int pru_sample(FILE *data_file, FILE *ambient_file,
               rl_config_t const *const config,
               char const *const file_comment) {
    int res;

    // average (for low rates)
    uint32_t aggregates = 1;
    if (config->sample_rate < MIN_ADC_RATE) {
        aggregates = (uint32_t)(MIN_ADC_RATE / config->sample_rate);
    }

    // METER
    if (config->mode == METER) {
        meter_init();
    }

    // WEBSERVER
    int sem_id = -1;
    struct web_shm *web_data = (struct web_shm *)-1;

    if (config->web_interface_enable) {
        // semaphores
        sem_id = sem_create(SEM_KEY, NUM_SEMS);
        sem_set(sem_id, DATA_SEM, 1);

        // shared memory
        web_data = web_create_shm();

        // determine web channels count (merged)
        int num_web_channels = count_channels(config->channels);
        if (config->channels[I1H_INDEX] && config->channels[I1L_INDEX]) {
            num_web_channels--;
        }
        if (config->channels[I2H_INDEX] && config->channels[I2L_INDEX]) {
            num_web_channels--;
        }
        if (config->digital_input_enable) {
            num_web_channels += NUM_DIGITAL_INPUTS;
        }
        web_data->num_channels = num_web_channels;

        // web buffer sizes
        int buffer_sizes[WEB_RING_BUFFER_COUNT] = {BUFFER1_SIZE, BUFFER10_SIZE,
                                                   BUFFER100_SIZE};

        for (int i = 0; i < WEB_RING_BUFFER_COUNT; i++) {
            int web_buffer_element_size =
                buffer_sizes[i] * num_web_channels * sizeof(int64_t);
            int web_buffer_length = NUM_WEB_POINTS / buffer_sizes[i];
            web_buffer_reset(&web_data->buffer[i], web_buffer_element_size,
                             web_buffer_length);
        }
    }

    // PRU SETUP

    // initialize PRU data structure
    pru_data_t pru;
    res = pru_data_init(&pru, config, aggregates);
    if (res != SUCCESS) {
        rl_log(ERROR, "failed initializing PRU data structure");
        return FAILURE;
    }

    // check max PRU buffer size
    uint32_t pru_extmem_size = (uint32_t)prussdrv_extmem_size();
    uint32_t pru_extmem_size_demand =
        2 *
        (pru.buffer_length * (PRU_SAMPLE_SIZE * NUM_CHANNELS + PRU_DIG_SIZE) +
         PRU_BUFFER_STATUS_SIZE);
    if (pru_extmem_size_demand > pru_extmem_size) {
        rl_log(ERROR, "insufficient PRU memory allocated/available.\n"
                      "update uio_pruss configuration:"
                      "options uio_pruss extram_pool_sz=0x%06x",
               pru_extmem_size_demand);
        pru.state = PRU_OFF;
    }

    // get user space mapped PRU memory addresses
    void const *buffer0_ptr = prussdrv_get_virt_addr(pru.buffer0_ptr);
    void const *buffer1_ptr = prussdrv_get_virt_addr(pru.buffer1_ptr);

    // DATA FILE STORING

    // file header lead-in
    struct rl_file_header file_header;
    file_setup_lead_in(&(file_header.lead_in), config);

    // channel array
    int total_channel_count = file_header.lead_in.channel_bin_count +
                              file_header.lead_in.channel_count;
    rl_file_channel_t file_channel[total_channel_count];
    file_header.channel = file_channel;

    // complete file header
    file_setup_header(&file_header, config, file_comment);

    // store header
    if (config->file_format == RL_FILE_BIN) {
        file_store_header_bin(data_file, &file_header);
    } else if (config->file_format == RL_FILE_CSV) {
        file_store_header_csv(data_file, &file_header);
    }

    // AMBIENT FILE STORING

    // file header lead-in
    struct rl_file_header ambient_file_header;

    if (config->ambient.enabled) {

        ambient_setup_lead_in(&(ambient_file_header.lead_in), config);

        // allocate channel array
        ambient_file_header.channel = malloc(config->ambient.sensor_count *
                                             sizeof(rl_file_channel_t));

        // complete file header
        ambient_setup_header(&ambient_file_header, config, file_comment);

        // store header
        file_store_header_bin(ambient_file, &ambient_file_header);
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
        rl_log(ERROR, "Failed starting PRU, binary not found");
        return FAILURE;
    }

    // wait for PRU startup event
    res = pru_wait_event_timeout(PRU_EVTOUT_0, PRU_TIMEOUT);
    if (res == ETIMEDOUT) {
        rl_log(ERROR, "Failed starting PRU, PRU not responding");
        return FAILURE;
    }

    // clear event
    prussdrv_pru_clear_event(PRU_EVTOUT_0, PRU0_ARM_INTERRUPT);

    void const *buffer_addr;
    rl_timestamp_t timestamp_monotonic;
    rl_timestamp_t timestamp_realtime;
    uint32_t buffers_lost = 0;
    uint32_t buffer_samples_count;    // number of samples per buffer
    uint32_t num_files = 1;           // number of files stored
    uint8_t skipped_buffer_count = 0; // skipped buffers for ambient reading
    bool web_server_skip = false;

    // buffers to read in finite mode
    uint32_t buffer_read_count =
        ceil_div(config->sample_limit * aggregates, pru.buffer_length);

    // sampling started
    status.sampling = RL_SAMPLING_ON;
    res = write_status(&status);
    if (res == FAILURE) {
        rl_log(WARNING, "Failed writing status");
    }

    // continuous sampling loop
    for (uint32_t i = 0;
         status.sampling == RL_SAMPLING_ON && status.state == RL_RUNNING &&
         !(config->mode == LIMIT && i >= buffer_read_count);
         i++) {

        if (config->file_format != RL_FILE_NONE) {

            // check if max file size reached
            uint64_t file_size = (uint64_t)ftello(data_file);
            uint64_t margin =
                config->sample_rate * sizeof(int32_t) * (NUM_CHANNELS + 1) +
                sizeof(rl_timestamp_t);

            if (config->max_file_size != 0 &&
                file_size + margin > config->max_file_size) {

                // close old data file
                fclose(data_file);

                // determine new file name
                char file_name[MAX_PATH_LENGTH];
                char new_file_ending[MAX_PATH_LENGTH];
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
                file_header.lead_in.data_block_count = 0;
                file_header.lead_in.sample_count = 0;

                // store header
                if (config->file_format == RL_FILE_BIN) {
                    file_store_header_bin(data_file, &file_header);
                } else if (config->file_format == RL_FILE_CSV) {
                    file_store_header_csv(data_file, &file_header);
                }

                rl_log(INFO, "Creating new data file: %s", file_name);

                // AMBIENT FILE
                if (config->ambient.enabled) {
                    // close old ambient file
                    fclose(ambient_file);

                    // determine new file name
                    strcpy(file_name, config->ambient.file_name);

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
                    file_store_header_bin(ambient_file, &ambient_file_header);

                    rl_log(INFO, "new ambient-file: %s", file_name);
                }

                num_files++;
            }
        }

        // select current buffer
        if (i % 2 == 0) {
            buffer_addr = buffer0_ptr;
        } else {
            buffer_addr = buffer1_ptr;
        }

        // select buffer size
        if (i < buffer_read_count - 1 ||
            pru.sample_limit % pru.buffer_length == 0) {
            buffer_samples_count = pru.buffer_length; // full buffer size
        } else {
            buffer_samples_count =
                pru.sample_limit % pru.buffer_length; // unfull buffer size
        }

        // Wait for event completion from PRU
        // only check for timeout on first buffer (else it does not work!)
        if (i == 0) {
            res = pru_wait_event_timeout(PRU_EVTOUT_0, PRU_TIMEOUT);
            if (res == ETIMEDOUT) {
                // timeout occurred
                rl_log(ERROR, "ADC not responding");
                break;
            }
        } else {
            prussdrv_pru_wait_event(PRU_EVTOUT_0);
        }

        // timestamp received data
        create_time_stamp(&timestamp_realtime, &timestamp_monotonic);

        // adjust time with buffer latency
        timestamp_realtime.nsec -= (uint64_t)1e9 / config->update_rate;
        if (timestamp_realtime.nsec < 0) {
            timestamp_realtime.sec -= 1;
            timestamp_realtime.nsec += (uint64_t)1e9;
        }
        timestamp_monotonic.nsec -= (uint64_t)1e9 / config->update_rate;
        if (timestamp_monotonic.nsec < 0) {
            timestamp_monotonic.sec -= 1;
            timestamp_monotonic.nsec += (uint64_t)1e9;
        }

        // clear event
        prussdrv_pru_clear_event(PRU_EVTOUT_0, PRU0_ARM_INTERRUPT);

        // PRU memory sync before accessing data
        __sync_synchronize();

        // check for overrun (compare buffer numbers)
        uint32_t buffer_index = *((uint32_t *)buffer_addr);
        if (buffer_index != i) {
            buffers_lost += (buffer_index - i);
            rl_log(WARNING,
                   "overrun: %d samples (%d buffer) lost (%d in total)",
                   (buffer_index - i) * pru.buffer_length, buffer_index - i,
                   buffers_lost);
            i = buffer_index;
        }

        // handle the buffer
        file_handle_data(data_file, buffer_addr + PRU_BUFFER_STATUS_SIZE,
                         buffer_samples_count, &timestamp_realtime,
                         &timestamp_monotonic, config);

        // process data for web when enabled
        if (config->web_interface_enable && !web_server_skip) {
            res = web_handle_data(
                web_data, sem_id, buffer_addr + PRU_BUFFER_STATUS_SIZE,
                buffer_samples_count, &timestamp_realtime, config);
            if (res == FAILURE) {
                // disable web interface on failure, but continue sampling
                web_server_skip = true;
                rl_log(WARNING, "Web server connection failed. "
                                "Disabling web interface.");
            }
        }

        // update and write header
        if (config->file_format != RL_FILE_NONE) {
            // update the number of samples stored
            file_header.lead_in.data_block_count += 1;
            file_header.lead_in.sample_count +=
                buffer_samples_count / aggregates;

            if (config->file_format == RL_FILE_BIN) {
                file_update_header_bin(data_file, &file_header);
            } else if (config->file_format == RL_FILE_CSV) {
                file_update_header_csv(data_file, &file_header);
            }
        }

        // handle ambient data
        if (skipped_buffer_count + 1 >= config->update_rate) { // always 1 Sps
            if (config->ambient.enabled) {

                // fetch and write data
                ambient_store_data(ambient_file, &timestamp_realtime,
                                   &timestamp_monotonic, config);

                // update and write header
                ambient_file_header.lead_in.data_block_count += 1;
                ambient_file_header.lead_in.sample_count +=
                    AMBIENT_DATA_BLOCK_SIZE;
                file_update_header_bin(ambient_file, &ambient_file_header);
            }
            skipped_buffer_count = 0;
        } else {
            skipped_buffer_count++;
        }

        // update and write state
        status.samples_taken += buffer_samples_count / aggregates;
        status.buffer_number = i + 1 - buffers_lost;
        res = write_status(&status);
        if (res == FAILURE) {
            rl_log(WARNING, "Failed writing status");
        }

        // notify web clients
        /**
         * @todo: There is a possible race condition here, which might result in
         * one web client not getting notified once, do we care?
         */
        if (config->web_interface_enable) {
            int num_web_clients = sem_get(sem_id, WAIT_SEM);
            sem_set(sem_id, WAIT_SEM, num_web_clients);
        }

        // print meter output
        if (config->mode == METER) {
            meter_print_buffer(config, buffer_addr + PRU_BUFFER_STATUS_SIZE);
        }
    }

    // FILE FINISH (flush)
    // flush ambient data and cleanup file header
    if (config->ambient.enabled) {
        fflush(ambient_file);
        free(ambient_file_header.channel);
    }

    if (config->file_format != RL_FILE_NONE && status.state != RL_ERROR) {
        fflush(data_file);
        rl_log(INFO, "stored %llu samples to file", status.samples_taken);
        printf("Stored %llu samples to file.\n", status.samples_taken);
    }

    // WEBSERVER FINISH
    // unmap shared memory
    if (config->web_interface_enable) {
        sem_remove(sem_id);
        web_close_shm(web_data);
    }

    // METER FINISH
    if (config->mode == METER) {
        meter_deinit();
    }

    // STATE
    if (status.state == RL_ERROR) {
        return FAILURE;
    }

    return SUCCESS;
}

void pru_stop(void) {

    // write OFF to PRU state (so PRU can clean up)
    pru_set_state(PRU_OFF);

    // wait for interrupt (if no ERROR occurred) and clear event
    if (status.state != RL_ERROR) {
        pru_wait_event_timeout(PRU_EVTOUT_0, PRU_TIMEOUT);
        prussdrv_pru_clear_event(PRU_EVTOUT_0, PRU0_ARM_INTERRUPT);
    }
}
