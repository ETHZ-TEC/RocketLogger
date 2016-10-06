#include "pru.h"

int test_mode = 0;



// PRU TIMEOUT WRAPPER

pthread_mutex_t calculating = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t done = PTHREAD_COND_INITIALIZER;

void *pru_wait_event(void* voidEvent) {

	unsigned int event = *((unsigned int *) voidEvent);

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
	int err;

	pthread_mutex_lock(&calculating);

	// pthread cond_timedwait expects an absolute time to wait until
	clock_gettime(CLOCK_REALTIME, &abs_time);
	abs_time.tv_sec += timeout;

	pthread_create(&tid, NULL, pru_wait_event, (void *) &event);

	// TODO: pthread_cond_timedwait can return spuriously: this should be in a loop for production code
	err = pthread_cond_timedwait(&done, &calculating, &abs_time);

	if (!err) {
		pthread_mutex_unlock(&calculating);
	}

	return err;
}





// PRU MEMORY MAPPING

void* map_pru_memory() {
	
	unsigned int size = read_file_value(MMAP_FILE "size");
	
	// get pru memory location
	unsigned int pru_memory = read_file_value(MMAP_FILE "addr");
	off_t base = (off_t)pru_memory & ~PRU_MAP_MASK;
	
	// memory file
	int fd;
	if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1){
		rl_log(ERROR, "failed to open /dev/mem");
		return NULL;
    }
	
	// map shared memory into userspace
	//void* map_base = mmap(0, PRU_MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, base);
	void* map_base = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (off_t)pru_memory);
	
    if(map_base == (void *) -1) {
		rl_log(ERROR, "failed to map base address");
		return NULL;
    }
		
	close(fd);
	
	return map_base; // + ( (off_t)pru_memory & PRU_MAP_MASK);
}

int unmap_pru_memory(void* buffer) {
	
	unsigned int size = read_file_value(MMAP_FILE "size");
	
	unsigned int pru_memory = read_file_value(MMAP_FILE "addr");
	void* map_base = buffer - ( (off_t) pru_memory & PRU_MAP_MASK);
	
	//if(munmap(map_base, PRU_MAP_SIZE) == -1) {
	if(munmap(buffer, size) == -1) {
		rl_log(ERROR, "failed to unmap memory");
		return FAILURE;
    }
    
	return SUCCESS;
	
}




// PRU INITIALISATION

// set state to PRU
int pru_set_state(enum pru_states state){ // TODO void functions
		
	prussdrv_pru_write_memory(PRUSS0_PRU0_DATARAM, 0, (unsigned int*) &state, sizeof(int));
	
	return SUCCESS;
}

// PRU initiation
int pru_init() {
	
	// init PRU
	tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;
	prussdrv_init ();
	if (prussdrv_open(PRU_EVTOUT_0) == -1) {  
		rl_log(ERROR, "failed to open PRU");  
		return FAILURE;  
	}
	prussdrv_pruintc_init(&pruss_intc_initdata);
	
	return SUCCESS;
}

int pru_setup(struct pru_data_struct* pru, struct rl_conf* conf, unsigned int* pru_sample_rate) { // TODO: remove pru_sample_rate

	// set state
	if(conf->mode == LIMIT) {
		pru->state = PRU_LIMIT;
	} else {
		pru->state = PRU_CONTINUOUS;
	}
	
	// set sampling rate configuration
	switch (conf->sample_rate) {
		case 1:
			*pru_sample_rate = K1;
			pru->precision = PRECISION_HIGH;
			pru->sample_size = SIZE_HIGH;
			break;
		case 2:
			*pru_sample_rate = K2;
			pru->precision = PRECISION_HIGH;
			pru->sample_size = SIZE_HIGH;
			break;
		case 4:
			*pru_sample_rate = K4;
			pru->precision = PRECISION_HIGH;
			pru->sample_size = SIZE_HIGH;
			break;
		case 8:
			*pru_sample_rate = K8;
			pru->precision = PRECISION_HIGH;
			pru->sample_size = SIZE_HIGH;
			break;
		case 16:
			*pru_sample_rate = K16;
			pru->precision = PRECISION_HIGH;
			pru->sample_size = SIZE_HIGH;
			break;
		case 32:
			*pru_sample_rate = K32;
			pru->precision = PRECISION_LOW;
			pru->sample_size = SIZE_LOW;
			break;
		case 64:
			*pru_sample_rate = K64;
			pru->precision = PRECISION_LOW;
			pru->sample_size = SIZE_LOW;
			break;
		default:
			rl_log(ERROR, "wrong sample rate");
			return FAILURE;
	}
	
	// set buffer infos
	pru->sample_limit = conf->sample_limit;
	pru->buffer_size = (conf->sample_rate * 1000) / conf->update_rate;
	
	unsigned int buffer_size_bytes = pru->buffer_size * (pru->sample_size * NUM_CHANNELS + STATUS_SIZE) + BUFFERSTATUSSIZE;
	pru->buffer0_location = read_file_value(MMAP_FILE "addr");
	pru->buffer1_location = pru->buffer0_location + buffer_size_bytes;
	pru->add_currents = ADD_CURRENTS;
	
	
	// set commands
	pru->number_commands = NUMBER_PRU_COMMANDS;
	pru->commands[0] = RESET;
	pru->commands[1] = SDATAC;
	pru->commands[2] = WREG|CONFIG3|CONFIG3DEFAULT;						// write configuration
	pru->commands[3] = WREG|CONFIG1|CONFIG1DEFAULT | *pru_sample_rate;
	pru->commands[4] = WREG|CH1SET|GAIN2;								// set channel gains
	pru->commands[5] = WREG|CH2SET|GAIN1;
	pru->commands[6] = WREG|CH3SET|GAIN1;
	pru->commands[7] = WREG|CH4SET|GAIN1;
	pru->commands[8] = WREG|CH5SET|GAIN1;
	pru->commands[9] = RDATAC;											// continuous reading
	
	return SUCCESS;
}




// MAIN SAMPLE FUNCTION

int pru_sample(FILE* data, struct rl_conf* conf) {
	
	// TODO temporary solution !!!!!!!!
	int j;
	int MASK = 1;
	int channels = 0;
	for(j=0; j<NUM_CHANNELS; j++) {
		if(conf->channels[j] > 0) {
			channels = channels | MASK;
		}
		MASK = MASK << 1;
	}
	
	// running state
	status.state = RL_RUNNING;
	status.samples_taken = 0;
	status.buffer_number = 0;
	
	// start meter window
	if(conf->mode == METER) {
		meter_init();
	}
	
	// set up fifo for webserver data
	int fifo_fd = -1;
	int control_fifo = -1;
	
	if (conf->enable_web_server == 1) {
		
		// data fifo
		fifo_fd = open(FIFO_FILE, O_NONBLOCK | O_RDWR);
		if (fifo_fd < 0) {
			rl_log(ERROR, "could not open FIFO");
		}
		
		// control fifo
		control_fifo = open(CONTROL_FIFO, O_NONBLOCK | O_RDWR);
		if (control_fifo < 0) {
			rl_log(ERROR, "could not open control FIFO");
		}
	}
	
	// Map the PRU's interrupts
	tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;
	prussdrv_pruintc_init(&pruss_intc_initdata);
	
	
	// setup PRU
	struct pru_data_struct pru;
	unsigned int pru_sample_rate; // TODO: remove
	pru_setup(&pru, conf, &pru_sample_rate);
	unsigned int number_buffers = ceil_div(conf->sample_limit, pru.buffer_size);
	unsigned int buffer_size_bytes = pru.buffer_size * (pru.sample_size * NUM_CHANNELS + STATUS_SIZE) + BUFFERSTATUSSIZE;
	
	int store = ( (conf->file_format != NO_FILE) && (conf->mode !=  METER) );
	
	
	// store old file header // TODO: functions
	struct header file_header;
	if(store == 1) {
		
		// get header length
		if(conf->file_format == BIN) {
			file_header.header_length = HEADERLENGTH;
		} else if (conf->file_format == CSV) {
			file_header.header_length = HEADERLENGTH + 1;
		} else {
			rl_log(ERROR, "failed to update header, wrong file format");
		}
		
		file_header.number_samples = 0; // number of samples taken
		file_header.buffer_size = pru.buffer_size;
		file_header.rate = pru_sample_rate;
		file_header.channels = channels;
		file_header.precision = pru.precision;
		
		// store header
		store_header(data, &file_header, conf);
	}
	
	
	// new file header (unused): TODO: store_header_new, update_header_new function
	struct file_header_new header_new;
	setup_header(&header_new, conf, &pru);
	
	
	// check memory size
	unsigned int max_size = read_file_value(MMAP_FILE "size");
	if(2*buffer_size_bytes > max_size) {
		rl_log(ERROR, "not enough memory allocated. Run:\n  rmmod uio_pruss\n  modprobe uio_pruss extram_pool_sz=0x%06x", 2*buffer_size_bytes);
		pru.state = PRU_OFF;
		status.state = RL_OFF;
	}
		
	// map PRU memory into userspace
	void* buffer0 = map_pru_memory();
	void* buffer1 = buffer0 + buffer_size_bytes; 
	
	// write configuration to PRU memory
	prussdrv_pru_write_memory(PRUSS0_PRU0_DATARAM, 0, (unsigned int*) &pru, sizeof(struct pru_data_struct));	

	// run SPI on PRU0
	prussdrv_exec_program (0, PRU_CODE);

	int i;
	void* buffer_addr;
	unsigned int samples_buffer; // number of samples per buffer
	
	// continuous sampling loop
	for(i=0; status.state == RL_RUNNING && !(conf->mode == LIMIT && i>=number_buffers); i++) {
		
		// select current buffer
		if(i%2 == 0) {
			buffer_addr = buffer0;
		} else {
			buffer_addr = buffer1;
		}
		// select buffer size
		if(i < number_buffers-1 || pru.sample_limit % pru.buffer_size == 0) {
			samples_buffer = pru.buffer_size; // full buffer size
		} else {
			samples_buffer = pru.sample_limit % pru.buffer_size; // unfull buffer size
		}
		
		// Wait for event completion from PRU
		if (test_mode == 0) {
			// only check for timout on first buffer (else it does not work!) -> TODO: check
			if (i == 0) {
				if(pru_wait_event_timeout(PRU_EVTOUT_0, PRU_TIMEOUT) == ETIMEDOUT) {
					// timeout occured
					rl_log(ERROR, "ADC timout. Stopping ...");
					break;
				}
			} else {
				prussdrv_pru_wait_event(PRU_EVTOUT_0);
			}
		} else {
			sleep(1);
		}
		
		// clear event
		prussdrv_pru_clear_event(PRU_EVTOUT_0, PRU0_ARM_INTERRUPT);

		
		// check for overrun (compare buffer numbers)
		if (test_mode == 0) {
			int buffer = *((uint32_t*) buffer_addr);
			if (buffer != i) {
				rl_log(WARNING, "overrun: %d samples (%d buffer) lost", (buffer - i) * pru.buffer_size, buffer - i);
				i = buffer;
			}
		}
		
		// store the buffer
		store_buffer(data, fifo_fd, control_fifo, buffer_addr+4, pru.sample_size, samples_buffer, conf);
		
		// update and write header
		if (store == 1) {
			// update the number of samples stored
			file_header.number_samples += samples_buffer;
			update_sample_number(data, &file_header, conf);
			
		}
		
		// update and write state
		status.samples_taken += samples_buffer;
		status.buffer_number = i;
		write_status(&status);
		
		// print meter output
		if(conf->mode == METER) {
			print_meter(conf, buffer_addr+4, pru.sample_size);
		}
	}
	
	// stop meter window
	if(conf->mode == METER) {
		meter_stop();
	}
	
	// flush data if no error occured
	if (store == 1 && status.state != RL_ERROR) {
		// print info
		rl_log(INFO,  "stored %d samples to file", status.samples_taken);
		
		printf("Stored %d samples to file.\n", status.samples_taken);
		
		fflush(data);
	}
	
	// unmap memory
	unmap_pru_memory(buffer0);
	
	// close FIFOs
	if (conf->enable_web_server == 1) {
		close(fifo_fd);
		close(control_fifo);
	}
	
	if(status.state == RL_ERROR) {
		return FAILURE;
	}
	
	return SUCCESS;
	
}




// PRU STOPPING

// stop PRU when in continuous mode
int pru_stop() {
	
	// write OFF to PRU state (so PRU can clean up)
	pru_set_state(PRU_OFF);
	
	// wait for interrupt (if no ERROR occured)
	if(status.state != RL_ERROR) {
		pru_wait_event_timeout(PRU_EVTOUT_0, PRU_TIMEOUT);
		prussdrv_pru_clear_event(PRU_EVTOUT_0, PRU0_ARM_INTERRUPT); // clear event
	}
	
	return SUCCESS;
}

// cleanup PRU
int pru_close() {
	
	// Disable PRU and close memory mappings 
	prussdrv_pru_disable(0);
	prussdrv_exit();
	return SUCCESS;
}
