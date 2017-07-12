/**
 * Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group
 */

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
void *pru_wait_event(void *voidEvent) {

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

    pthread_create(&tid, NULL, pru_wait_event, (void *)&event);

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
void *map_pru_memory(void) {

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
    void *pru_mmap = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
                          (off_t)pru_memory);

    if (pru_mmap == (void *)-1) {
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
int unmap_pru_memory(void *pru_mmap) {

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

    prussdrv_pru_write_memory(PRUSS0_PRU0_DATARAM, 0, (unsigned int *)&state,
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
int pru_data_setup(struct pru_data_struct *pru, struct rl_conf *conf,
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
        pru->sample_size = SIZE_LOW;
        break;
    case 64000:
        pru_sample_rate = K64;
        pru->precision = PRECISION_LOW;
        pru->sample_size = SIZE_LOW;
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
 * @param data File pointer to data file
 * @param conf Pointer to current {@link rl_conf} configuration
 * @return {@link SUCCESS} on success, {@link FAILURE} otherwise
 */
int pru_sample(FILE *data, struct rl_conf *conf) {

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
    struct web_shm *web_data = (struct web_shm *)-1;

    if (conf->enable_web_server == 1) {
        // semaphores
        sem_id = create_sem();
        set_sem(sem_id, DATA_SEM, 1);

        // shared memory
        web_data = create_web_shm();

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

        int i;
        for (i = 0; i < WEB_RING_BUFFER_COUNT; i++) {
            int web_buffer_element_size =
                buffer_sizes[i] * num_web_channels * sizeof(int64_t);
            int web_buffer_length = NUM_WEB_POINTS / buffer_sizes[i];
            reset_buffer(&web_data->buffer[i], web_buffer_element_size,
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
    void *buffer0 = map_pru_memory();
    void *buffer1 = buffer0 + buffer_size_bytes;

    // FILE STORING

    // file header lead-in
    struct rl_file_header file_header;
    setup_lead_in(&(file_header.lead_in), conf);

    // channel array
    int total_channel_count = file_header.lead_in.channel_bin_count +
                              file_header.lead_in.channel_count;
    struct rl_file_channel file_channel[total_channel_count];
    file_header.channel = file_channel;

    // complete file header
    setup_header(&file_header, conf);

    // store header
    if (conf->file_format == BIN) {
        store_header_bin(data, &file_header);
    } else if (conf->file_format == CSV) {
        store_header_csv(data, &file_header);
    }

    // EXECUTION

    // write configuration to PRU memory
    prussdrv_pru_write_memory(PRUSS0_PRU0_DATARAM, 0, (unsigned int *)&pru,
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

    unsigned int i;
    uint32_t buffer_lost = 0;
    void *buffer_addr;
    uint32_t samples_buffer; // number of samples per buffer
    uint32_t num_files = 1;  // number of files stored

    // sampling started
    status.sampling = SAMPLING_ON;
    write_status(&status);

    // continuous sampling loop
    for (i = 0; status.sampling == SAMPLING_ON && status.state == RL_RUNNING &&
                !(conf->mode == LIMIT && i >= number_buffers);
         i++) {

        if (conf->file_format != NO_FILE) {

            // check if max file size reached
            uint64_t file_size = (uint64_t)ftello(data);
            uint64_t margin =
                conf->sample_rate * sizeof(int32_t) * (NUM_CHANNELS + 1) +
                sizeof(struct time_stamp);

            if (conf->max_file_size != 0 &&
                file_size + margin > conf->max_file_size) {

                // close old data file
                fclose(data);

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
                sprintf(new_file_ending, "_p%d", num_files++);
                strcat(new_file_ending, file_ending);
                strcpy(file_ending, new_file_ending);

                // open new data file
                data = fopen(file_name, "w+");

                // update header for new file
                file_header.lead_in.data_block_count = 0;
                file_header.lead_in.sample_count = 0;

                // store header
                if (conf->file_format == BIN) {
                    store_header_bin(data, &file_header);
                } else if (conf->file_format == CSV) {
                    store_header_csv(data, &file_header);
                }

                rl_log(INFO, "new datafile: %s", file_name);
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
            samples_buffer = pru.buffer_size; // full buffer size
        } else {
            samples_buffer =
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

        // clear event
        prussdrv_pru_clear_event(PRU_EVTOUT_0, PRU0_ARM_INTERRUPT);

        // check for overrun (compare buffer numbers)
        uint32_t buffer = *((uint32_t *)buffer_addr);
        if (buffer != i) {
            buffer_lost += (buffer - i);
            rl_log(WARNING,
                   "overrun: %d samples (%d buffer) lost (%d in total)",
                   (buffer - i) * pru.buffer_size, buffer - i, buffer_lost);
            i = buffer;
        }

        // handle the buffer
        handle_data_buffer(data, buffer_addr + 4, pru.sample_size,
                           samples_buffer, conf, sem_id, web_data);

        // update and write header
        if (conf->file_format != NO_FILE) {
            // update the number of samples stored
            file_header.lead_in.data_block_count += 1;
            file_header.lead_in.sample_count += samples_buffer / avg_factor;

            if (conf->file_format == BIN) {
                update_header_bin(data, &file_header);
            } else if (conf->file_format == CSV) {
                update_header_csv(data, &file_header);
            }
        }

        // update and write state
        status.samples_taken += samples_buffer / avg_factor;
        status.buffer_number = i + 1 - buffer_lost;
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

        fflush(data);
    }

    // PRU FINISH (unmap memory)
    unmap_pru_memory(buffer0);

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
 * Stop and shut down PRU operation
 */
void pru_stop(void) {

    // write OFF to PRU state (so PRU can clean up)
    pru_set_state(PRU_OFF);

    // wait for interrupt (if no ERROR occured)
    if (status.state != RL_ERROR) {
        pru_wait_event_timeout(PRU_EVTOUT_0, PRU_TIMEOUT);
        prussdrv_pru_clear_event(PRU_EVTOUT_0,
                                 PRU0_ARM_INTERRUPT); // clear event
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
