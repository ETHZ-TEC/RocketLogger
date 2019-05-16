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
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>

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

int pru_init_data(pru_data_t *const pru, struct rl_conf const *const conf,
                  uint32_t aggregates) {
    // zero aggregates value is also considered no aggregation
    if (aggregates == 0) {
        aggregates = 1;
    }

    // set state
    if (conf->mode == LIMIT) {
        pru->state = PRU_FINITE;
    } else {
        pru->state = PRU_CONTINUOUS;
    }

    // set sampling rate configuration
    uint32_t pru_sample_rate;
    switch (conf->sample_rate) {
    case 1:
        pru_sample_rate = K1;
        pru->adc_precision = PRU_PRECISION_HIGH;
        break;
    case 10:
        pru_sample_rate = K1;
        pru->adc_precision = PRU_PRECISION_HIGH;
        break;
    case 100:
        pru_sample_rate = K1;
        pru->adc_precision = PRU_PRECISION_HIGH;
        break;
    case 1000:
        pru_sample_rate = K1;
        pru->adc_precision = PRU_PRECISION_HIGH;
        break;
    case 2000:
        pru_sample_rate = K2;
        pru->adc_precision = PRU_PRECISION_HIGH;
        break;
    case 4000:
        pru_sample_rate = K4;
        pru->adc_precision = PRU_PRECISION_HIGH;
        break;
    case 8000:
        pru_sample_rate = K8;
        pru->adc_precision = PRU_PRECISION_HIGH;
        break;
    case 16000:
        pru_sample_rate = K16;
        pru->adc_precision = PRU_PRECISION_HIGH;
        break;
    case 32000:
        pru_sample_rate = K32;
        pru->adc_precision = PRU_PRECISION_LOW;
        break;
    case 64000:
        pru_sample_rate = K64;
        pru->adc_precision = PRU_PRECISION_LOW;
        break;
    default:
        rl_log(ERROR, "invalid sample rate");
        return FAILURE;
    }

    // set buffer infos
    pru->sample_limit = conf->sample_limit * aggregates;
    pru->buffer_length = (conf->sample_rate * aggregates) / conf->update_rate;

    uint32_t buffer_size_bytes =
        pru->buffer_length * (PRU_SAMPLE_SIZE * NUM_CHANNELS + PRU_DIG_SIZE) +
        PRU_BUFFER_STATUS_SIZE;

    void *pru_extmem_base;
    prussdrv_map_extmem(&pru_extmem_base);
    uint32_t pru_extmem_phys =
        (uint32_t)prussdrv_get_phys_addr(pru_extmem_base);
    pru->buffer0_ptr = pru_extmem_phys;
    pru->buffer1_ptr = pru_extmem_phys + buffer_size_bytes;

    // setup commands to send for ADC initialization
    pru->adc_command_count = PRU_ADC_COMMAND_COUNT;
    pru->adc_command[0] = RESET;
    pru->adc_command[1] = SDATAC;
    pru->adc_command[2] =
        WREG | CONFIG3 | CONFIG3DEFAULT; // write configuration
    pru->adc_command[3] = WREG | CONFIG1 | CONFIG1DEFAULT | pru_sample_rate;

    // set channel gains
    pru->adc_command[4] = WREG | CH1SET | GAIN2;  // High Range A
    pru->adc_command[5] = WREG | CH2SET | GAIN2;  // High Range B
    pru->adc_command[6] = WREG | CH3SET | GAIN1;  // Medium Range
    pru->adc_command[7] = WREG | CH4SET | GAIN1;  // Low Range A
    pru->adc_command[8] = WREG | CH5SET | GAIN1;  // Low Range B
    pru->adc_command[9] = WREG | CH6SET | GAIN1;  // Voltage 1
    pru->adc_command[10] = WREG | CH7SET | GAIN1; // Voltage 2
    pru->adc_command[11] = RDATAC;                // continuous reading

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
               struct rl_conf const *const conf,
               char const *const file_comment) {
    int res;

    // average (for low rates)
    uint32_t aggregates = 1;
    if (conf->sample_rate < MIN_ADC_RATE) {
        aggregates = (uint32_t)(MIN_ADC_RATE / conf->sample_rate);
    }

    // METER
    if (conf->mode == METER) {
        meter_init();
    }

    // WEBSERVER
    int sem_id = -1;
    struct web_shm *web_data = (struct web_shm *)-1;

    if (conf->enable_web_server == 1) {
        // semaphores
        sem_id = create_sem(SEM_KEY, NUM_SEMS);
        set_sem(sem_id, DATA_SEM, 1);

        // shared memory
        web_data = web_create_shm();

        // determine web channels count (merged)
        int num_web_channels = count_channels(conf->channels);
        if (conf->channels[I1H_INDEX] == CHANNEL_ENABLED &&
            conf->channels[I1L_INDEX] == CHANNEL_ENABLED) {
            num_web_channels--;
        }
        if (conf->channels[I2H_INDEX] == CHANNEL_ENABLED &&
            conf->channels[I2L_INDEX] == CHANNEL_ENABLED) {
            num_web_channels--;
        }
        if (conf->digital_inputs == DIGITAL_INPUTS_ENABLED) {
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
    pru_init_data(&pru, conf, aggregates);

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
    file_setup_lead_in(&(file_header.lead_in), conf);

    // channel array
    int total_channel_count = file_header.lead_in.channel_bin_count +
                              file_header.lead_in.channel_count;
    struct rl_file_channel file_channel[total_channel_count];
    file_header.channel = file_channel;

    // complete file header
    file_setup_header(&file_header, conf, file_comment);

    // store header
    if (conf->file_format == BIN) {
        file_store_header_bin(data_file, &file_header);
    } else if (conf->file_format == CSV) {
        file_store_header_csv(data_file, &file_header);
    }

    // AMBIENT FILE STORING

    // file header lead-in
    struct rl_file_header ambient_file_header;

    if (conf->ambient.enabled == AMBIENT_ENABLED) {

        ambient_setup_lead_in(&(ambient_file_header.lead_in), conf);

        // allocate channel array
        ambient_file_header.channel =
            malloc(conf->ambient.sensor_count * sizeof(struct rl_file_channel));

        // complete file header
        ambient_setup_header(&ambient_file_header, conf, file_comment);

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
    if (prussdrv_exec_program(0, PRU_BINARY_FILE) < 0) {
        rl_log(ERROR, "PRU code not found");
        pru.state = PRU_OFF;
    }

    // wait for PRU startup event
    if (pru_wait_event_timeout(PRU_EVTOUT_0, PRU_TIMEOUT) == ETIMEDOUT) {
        // timeout occurred
        rl_log(ERROR, "PRU not responding");
        pru.state = PRU_OFF;
    }

    // clear event
    prussdrv_pru_clear_event(PRU_EVTOUT_0, PRU0_ARM_INTERRUPT);

    void const *buffer_addr;
    struct time_stamp timestamp_monotonic;
    struct time_stamp timestamp_realtime;
    uint32_t buffers_lost = 0;
    uint32_t buffer_samples_count;    // number of samples per buffer
    uint32_t num_files = 1;           // number of files stored
    uint8_t skipped_buffer_count = 0; // skipped buffers for ambient reading
    bool web_server_skip = false;

    // buffers to read in finite mode
    uint32_t buffer_read_count =
        ceil_div(conf->sample_limit * aggregates, pru.buffer_length);

    // sampling started
    status.sampling = SAMPLING_ON;
    write_status(&status);

    // continuous sampling loop
    for (uint32_t i = 0;
         status.sampling == SAMPLING_ON && status.state == RL_RUNNING &&
         !(conf->mode == LIMIT && i >= buffer_read_count);
         i++) {

        if (conf->file_format != NO_FILE) {

            // check if max file size reached
            uint64_t file_size = (uint64_t)ftello(data_file);
            uint64_t margin =
                conf->sample_rate * sizeof(int32_t) * (NUM_CHANNELS + 1) +
                sizeof(struct time_stamp);

            if (conf->max_file_size != 0 &&
                file_size + margin > conf->max_file_size) {

                // close old data file
                fclose(data_file);

                // determine new file name
                char file_name[MAX_PATH_LENGTH];
                char new_file_ending[MAX_PATH_LENGTH];
                strcpy(file_name, conf->file_name);

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
                if (conf->file_format == BIN) {
                    file_store_header_bin(data_file, &file_header);
                } else if (conf->file_format == CSV) {
                    file_store_header_csv(data_file, &file_header);
                }

                rl_log(INFO, "new datafile: %s", file_name);

                // AMBIENT FILE
                if (conf->ambient.enabled == AMBIENT_ENABLED) {
                    // close old ambient file
                    fclose(ambient_file);

                    // determine new file name
                    strcpy(file_name, conf->ambient.file_name);

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
            if (pru_wait_event_timeout(PRU_EVTOUT_0, PRU_TIMEOUT) ==
                ETIMEDOUT) {
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
        timestamp_realtime.nsec -= (uint64_t)1e9 / conf->update_rate;
        if (timestamp_realtime.nsec < 0) {
            timestamp_realtime.sec -= 1;
            timestamp_realtime.nsec += (uint64_t)1e9;
        }
        timestamp_monotonic.nsec -= (uint64_t)1e9 / conf->update_rate;
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
                         &timestamp_monotonic, conf);

        // process data for web when enabled
        if (conf->enable_web_server == 1 && !web_server_skip) {
            res = web_handle_data(
                web_data, sem_id, buffer_addr + PRU_BUFFER_STATUS_SIZE,
                buffer_samples_count, &timestamp_realtime, conf);
            if (res == FAILURE) {
                // disable web interface on failure, but continue sampling
                web_server_skip = true;
                rl_log(WARNING, "Web server connection failed. "
                                "Disabling web interface.");
            }
        }

        // update and write header
        if (conf->file_format != NO_FILE) {
            // update the number of samples stored
            file_header.lead_in.data_block_count += 1;
            file_header.lead_in.sample_count +=
                buffer_samples_count / aggregates;

            if (conf->file_format == BIN) {
                file_update_header_bin(data_file, &file_header);
            } else if (conf->file_format == CSV) {
                file_update_header_csv(data_file, &file_header);
            }
        }

        // handle ambient data
        if (skipped_buffer_count + 1 >= conf->update_rate) { // always 1 Sps
            if (conf->ambient.enabled == AMBIENT_ENABLED) {

                // fetch and write data
                ambient_store_data(ambient_file, &timestamp_realtime,
                                   &timestamp_monotonic, conf);

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
        write_status(&status);

        // notify web clients
        /**
         * @todo: There is a possible race condition here, which might result in
         * one web client not getting notified once, do we care?
         */
        if (conf->enable_web_server == 1) {
            int num_web_clients = semctl(sem_id, WAIT_SEM, GETNCNT);
            set_sem(sem_id, WAIT_SEM, num_web_clients);
        }

        // print meter output
        if (conf->mode == METER) {
            meter_print_buffer(conf, buffer_addr + PRU_BUFFER_STATUS_SIZE);
        }
    }

    // FILE FINISH (flush)
    // flush ambient data and cleanup file header
    if (conf->ambient.enabled == AMBIENT_ENABLED) {
        fflush(ambient_file);
        free(ambient_file_header.channel);
    }

    if (conf->file_format != NO_FILE && status.state != RL_ERROR) {
        fflush(data_file);
        rl_log(INFO, "stored %llu samples to file", status.samples_taken);
        printf("Stored %llu samples to file.\n", status.samples_taken);
    }

    // WEBSERVER FINISH
    // unmap shared memory
    if (conf->enable_web_server == 1) {
        remove_sem(sem_id);
        shmdt(web_data);
    }

    // METER FINISH
    if (conf->mode == METER) {
        meter_stop();
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
