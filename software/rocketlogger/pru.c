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

#include <stdint.h>

#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>

#include "file_handling.h"
#include "util.h"
#include "web.h"

#include "pru.h"

// PRU TIMEOUT WRAPPER

/// PRU access mutex
pthread_mutex_t waiting = PTHREAD_MUTEX_INITIALIZER;

/// Notification variable
pthread_cond_t done = PTHREAD_COND_INITIALIZER;

/**
 * Wait on PRU event
 * @param voidEvent PRU event to wait on
 */
void* pru_wait_event(void* voidEvent) {

    unsigned int event = *((unsigned int*)voidEvent);

    // allow the thread to be killed at any time
    int oldtype;
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);

    // wait for pru event
    prussdrv_pru_wait_event(event);

    // notify main program
    pthread_cond_signal(&done);

    return NULL;
}

/**
 * Wrapper for PRU event waiting with time out
 * @param event PRU event to wait on
 * @param timeout Time out in seconds
 * @return error code of pthread timedwait function
 */
int pru_wait_event_timeout(unsigned int event, unsigned int timeout) {

    struct timespec abs_time;
    pthread_t tid;
    int err;

    pthread_mutex_lock(&waiting);

    // pthread cond_timedwait expects an absolute time to wait until
    clock_gettime(CLOCK_REALTIME, &abs_time);
    abs_time.tv_sec += timeout;

    pthread_create(&tid, NULL, pru_wait_event, (void*)&event);

    err = pthread_cond_timedwait(&done, &waiting, &abs_time);

    if (!err) {
        pthread_mutex_unlock(&waiting);
    }

    return err;
}

// PRU MEMORY MAPPING
/**
 * Map PRU memory into user space
 */
void* pru_map_memory(void) {

    // get pru memory location and size
    unsigned int pru_memory = read_file_value(MMAP_FILE "addr");
    unsigned int size = read_file_value(MMAP_FILE "size");

    // memory map file
    int fd;
    if ((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) {
        rl_log(ERROR, "failed to open /dev/mem");
        return NULL;
    }

    // map shared memory into userspace
    void* pru_mmap = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
                          (off_t)pru_memory);

    if (pru_mmap == (void*)-1) {
        rl_log(ERROR, "failed to map base address");
        return NULL;
    }

    close(fd);

    return pru_mmap;
}

/**
 * Unmap PRU memory from user space
 * @param pru_mmap Pointer to mapped memory
 * @return {@link SUCCESS} on success, {@link FAILURE} otherwise
 */
int pru_unmap_memory(void* pru_mmap) {

    // get pru memory size
    unsigned int size = read_file_value(MMAP_FILE "size");

    if (munmap(pru_mmap, size) == -1) {
        rl_log(ERROR, "failed to unmap memory");
        return FAILURE;
    }

    return SUCCESS;
}

// PRU INITIALISATION

/**
 * Write state to PRU
 * @param state PRU state to write
 */
void pru_set_state(rl_pru_state state) {

    prussdrv_pru_write_memory(PRUSS0_PRU0_DATARAM, 0, (unsigned int*)&state,
                              sizeof(int));
}

/**
 * PRU initiation
 * @return {@link SUCCESS} on success, {@link FAILURE} otherwise
 */
int pru_init(void) {

    // init PRU
    tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;
    prussdrv_init();
    if (prussdrv_open(PRU_EVTOUT_0) == -1) {
        rl_log(ERROR, "failed to open PRU");
        return FAILURE;
    }
    prussdrv_pruintc_init(&pruss_intc_initdata);

    return SUCCESS;
}

/**
 * PRU data struct setup
 * @param pru Pointer to {@link pru_data_struct} to setup
 * @param conf Pointer to current {@link rl_conf} configuration
 * @param avg_factor Average factor for sampling rates smaller than minimal ADC
 * rate
 * @return {@link SUCCESS} on success, {@link FAILURE} otherwise
 */
int pru_data_setup(struct pru_data_struct* pru, struct rl_conf* conf,
                   uint32_t avg_factor) {

    uint32_t pru_sample_rate;

    // set state
    if (conf->mode == LIMIT) {
        pru->state = PRU_LIMIT;
    } else {
        pru->state = PRU_CONTINUOUS;
    }

    // set sampling rate configuration
    switch (conf->sample_rate) {
    case 1:
        pru_sample_rate = K1;
        pru->precision = PRECISION_HIGH;
        pru->sample_size = SIZE_HIGH;
        break;
    case 10:
        pru_sample_rate = K1;
        pru->precision = PRECISION_HIGH;
        pru->sample_size = SIZE_HIGH;
        break;
    case 100:
        pru_sample_rate = K1;
        pru->precision = PRECISION_HIGH;
        pru->sample_size = SIZE_HIGH;
        break;
    case 1000:
        pru_sample_rate = K1;
        pru->precision = PRECISION_HIGH;
        pru->sample_size = SIZE_HIGH;
        break;
    case 2000:
        pru_sample_rate = K2;
        pru->precision = PRECISION_HIGH;
        pru->sample_size = SIZE_HIGH;
        break;
    case 4000:
        pru_sample_rate = K4;
        pru->precision = PRECISION_HIGH;
        pru->sample_size = SIZE_HIGH;
        break;
    case 8000:
        pru_sample_rate = K8;
        pru->precision = PRECISION_HIGH;
        pru->sample_size = SIZE_HIGH;
        break;
    case 16000:
        pru_sample_rate = K16;
        pru->precision = PRECISION_HIGH;
        pru->sample_size = SIZE_HIGH;
        break;
    case 32000:
        pru_sample_rate = K32;
        pru->precision = PRECISION_LOW;
        pru->sample_size = SIZE_HIGH;
        break;
    case 64000:
        pru_sample_rate = K64;
        pru->precision = PRECISION_LOW;
        pru->sample_size = SIZE_HIGH;
        break;
    default:
        rl_log(ERROR, "wrong sample rate");
        return FAILURE;
    }

    // set buffer infos
    pru->sample_limit = conf->sample_limit * avg_factor;
    pru->buffer_size = (conf->sample_rate * avg_factor) / conf->update_rate;

    uint32_t buffer_size_bytes =
        pru->buffer_size * (pru->sample_size * NUM_CHANNELS + PRU_DIG_SIZE) +
        PRU_BUFFER_STATUS_SIZE;
    pru->buffer0_location = read_file_value(MMAP_FILE "addr");
    pru->buffer1_location = pru->buffer0_location + buffer_size_bytes;

    // set commands
    pru->number_commands = NUMBER_ADC_COMMANDS;
    pru->commands[0] = RESET;
    pru->commands[1] = SDATAC;
    pru->commands[2] = WREG | CONFIG3 | CONFIG3DEFAULT; // write configuration
    pru->commands[3] = WREG | CONFIG1 | CONFIG1DEFAULT | pru_sample_rate;

    // set channel gains
    pru->commands[4] = WREG | CH1SET | GAIN2;  // High Range A
    pru->commands[5] = WREG | CH2SET | GAIN2;  // High Range B
    pru->commands[6] = WREG | CH3SET | GAIN1;  // Medium Range
    pru->commands[7] = WREG | CH4SET | GAIN1;  // Low Range A
    pru->commands[8] = WREG | CH5SET | GAIN1;  // Low Range B
    pru->commands[9] = WREG | CH6SET | GAIN1;  // Voltage 1
    pru->commands[10] = WREG | CH7SET | GAIN1; // Voltage 2
    pru->commands[11] = RDATAC;                // continuous reading

    return SUCCESS;
}

/**
 * Main PRU sampling function
 * @param data_file File pointer to data file
 * @param ambient_file File pointer to ambient file
 * @param conf Pointer to current {@link rl_conf} configuration
 * @param file_comment Comment to store in the file header
 * @return {@link SUCCESS} on success, {@link FAILURE} otherwise
 */
int pru_sample(FILE* data_file, FILE* ambient_file, struct rl_conf* conf,
               char* file_comment) {

    // average (for low rates)
    uint32_t avg_factor = 1;
    if (conf->sample_rate < MIN_ADC_RATE) {
        avg_factor = MIN_ADC_RATE / conf->sample_rate;
    }

    // METER
    if (conf->mode == METER) {
        meter_init();
    }

    // WEBSERVER
    int sem_id = -1;
    struct web_shm* web_data = (struct web_shm*)-1;

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

    // Map the PRU's interrupts
    tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;
    prussdrv_pruintc_init(&pruss_intc_initdata);

    // setup PRU
    struct pru_data_struct pru;
    pru_data_setup(&pru, conf, avg_factor);
    unsigned int number_buffers =
        ceil_div(conf->sample_limit * avg_factor, pru.buffer_size);
    unsigned int buffer_size_bytes =
        pru.buffer_size * (pru.sample_size * NUM_CHANNELS + PRU_DIG_SIZE) +
        PRU_BUFFER_STATUS_SIZE;

    // check memory size
    unsigned int max_size = read_file_value(MMAP_FILE "size");
    if (2 * buffer_size_bytes > max_size) {
        rl_log(ERROR, "not enough memory allocated. Run:\n  rmmod uio_pruss\n  "
                      "modprobe uio_pruss extram_pool_sz=0x%06x",
               2 * buffer_size_bytes);
        pru.state = PRU_OFF;
    }

    // map PRU memory into userspace
    void* buffer0 = pru_map_memory();
    void* buffer1 = buffer0 + buffer_size_bytes;

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
    // channel array
    struct rl_file_channel ambient_file_channel[conf->ambient.sensor_count];

    if (conf->ambient.enabled == AMBIENT_ENABLED) {

        ambient_setup_lead_in(&(ambient_file_header.lead_in), conf);

        // channel array
        ambient_file_header.channel = ambient_file_channel;

        // complete file header
        ambient_setup_header(&ambient_file_header, conf, file_comment);

        // store header
        file_store_header_bin(ambient_file, &ambient_file_header);
    }

    // EXECUTION

    // write configuration to PRU memory
    prussdrv_pru_write_memory(PRUSS0_PRU0_DATARAM, 0, (unsigned int*)&pru,
                              sizeof(struct pru_data_struct));

    // run SPI on PRU0
    if (prussdrv_exec_program(0, PRU_CODE) < 0) {
        rl_log(ERROR, "PRU code not found");
        pru.state = PRU_OFF;
    }

    // wait for first PRU event
    if (pru_wait_event_timeout(PRU_EVTOUT_0, PRU_TIMEOUT) == ETIMEDOUT) {
        // timeout occured
        rl_log(ERROR, "PRU not responding");
        pru.state = PRU_OFF;
    }

    // clear event
    prussdrv_pru_clear_event(PRU_EVTOUT_0, PRU0_ARM_INTERRUPT);

    void* buffer_addr;
    struct time_stamp timestamp_monotonic;
    struct time_stamp timestamp_realtime;
    uint32_t buffers_lost = 0;
    uint32_t buffer_samples_count; // number of samples per buffer
    uint32_t num_files = 1;        // number of files stored
    uint8_t skipped_buffers = 0;   // skipped buffers for ambient reading

    // sampling started
    status.sampling = SAMPLING_ON;
    write_status(&status);

    // continuous sampling loop
    for (uint32_t i = 0;
         status.sampling == SAMPLING_ON && status.state == RL_RUNNING &&
         !(conf->mode == LIMIT && i >= number_buffers);
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
                char* file_ending = file_name;
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
            buffer_addr = buffer0;
        } else {
            buffer_addr = buffer1;
        }

        // select buffer size
        if (i < number_buffers - 1 || pru.sample_limit % pru.buffer_size == 0) {
            buffer_samples_count = pru.buffer_size; // full buffer size
        } else {
            buffer_samples_count =
                pru.sample_limit % pru.buffer_size; // unfull buffer size
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

        // check for overrun (compare buffer numbers)
        uint32_t buffer_index = *((uint32_t*)buffer_addr);
        if (buffer_index != i) {
            buffers_lost += (buffer_index - i);
            rl_log(WARNING,
                   "overrun: %d samples (%d buffer) lost (%d in total)",
                   (buffer_index - i) * pru.buffer_size, buffer_index - i,
                   buffers_lost);
            i = buffer_index;
        }

        // handle the buffer
        file_handle_data(data_file, buffer_addr + 4, pru.sample_size,
                         buffer_samples_count, &timestamp_realtime,
                         &timestamp_monotonic, conf);

        // process data for web when enabled
        if (conf->enable_web_server == 1) {
            web_handle_data(web_data, sem_id, buffer_addr + 4, pru.sample_size,
                            buffer_samples_count, &timestamp_realtime, conf);
        }

        // update and write header
        if (conf->file_format != NO_FILE) {
            // update the number of samples stored
            file_header.lead_in.data_block_count += 1;
            file_header.lead_in.sample_count +=
                buffer_samples_count / avg_factor;

            if (conf->file_format == BIN) {
                file_update_header_bin(data_file, &file_header);
            } else if (conf->file_format == CSV) {
                file_update_header_csv(data_file, &file_header);
            }
        }

        // handle ambient data
        if (skipped_buffers + 1 >= conf->update_rate) { // always 1 Sps
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
            skipped_buffers = 0;
        } else {
            skipped_buffers++;
        }

        // update and write state
        status.samples_taken += buffer_samples_count / avg_factor;
        status.buffer_number = i + 1 - buffers_lost;
        write_status(&status);

        // notify web clients
        // Note: There is a possible race condition here, which might result in
        // one web client not getting notified once, do we care?
        if (conf->enable_web_server == 1) {
            int num_web_clients = semctl(sem_id, WAIT_SEM, GETNCNT);
            set_sem(sem_id, WAIT_SEM, num_web_clients);
        }

        // print meter output
        if (conf->mode == METER) {
            meter_print_buffer(conf, buffer_addr + 4, pru.sample_size);
        }
    }

    // FILE FINISH (flush)
    if (conf->file_format != NO_FILE && status.state != RL_ERROR) {
        // print info
        rl_log(INFO, "stored %llu samples to file", status.samples_taken);

        printf("Stored %llu samples to file.\n", status.samples_taken);

        fflush(data_file);
    }

    // PRU FINISH (unmap memory)
    pru_unmap_memory(buffer0);

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

/**
 * Stop and shut down PRU operation.
 * @note When sampling in continuous mode, this has to be called before {@link
 * pru_close}.
 */
void pru_stop(void) {

    // write OFF to PRU state (so PRU can clean up)
    pru_set_state(PRU_OFF);

    // wait for interrupt (if no ERROR occured) and clear event
    if (status.state != RL_ERROR) {
        pru_wait_event_timeout(PRU_EVTOUT_0, PRU_TIMEOUT);
        prussdrv_pru_clear_event(PRU_EVTOUT_0, PRU0_ARM_INTERRUPT);
    }
}

/**
 * Disable PRU
 */
void pru_close(void) {

    // Disable PRU and close memory mappings
    prussdrv_pru_disable(0);
    prussdrv_exit();
}
