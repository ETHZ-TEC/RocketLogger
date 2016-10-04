#include "pru.h"

// ---------------------------------------------- CONSTANTS----------------------------------------------------------//


char channel_names[10][15] = {"I1H [nA]","I1M [nA]","I1L [10pA]","V1 [uV]","V2 [uV]","I2H [nA]","I2M [nA]","I2L [10pA]","V3 [uV]","V4 [uV]"};

struct header file_header;

struct pru_data_struct pru;

int test_mode = 1;


// ---------------------------------------------- FUNCTIONS --------------------------------------------------------//


// ------------------------------ STORE FUNCTIONS ------------------------------ //


int store_header(FILE* data, struct header* h, int binary) {
	
	if (binary == 1) {
		fwrite(h, sizeof(struct header), 1, data);
	} else {
		// information
		fprintf(data, "Header Length:,%d\n", h->header_length);
		fprintf(data, "Number Samples:,%-12d\n", h->number_samples);
		fprintf(data, "Buffer Size:,%d\n", h->buffer_size);
		fprintf(data, "Rate:,%d\n", h->rate);
		fprintf(data, "Channels:,%d\n", h->channels);
		fprintf(data, "Precision:,%d\n", h->precision);
		
		// title row
		fprintf(data,"Time");
		fprintf(data,",LOW1");
		fprintf(data,",LOW2");
		
		// channel names
		int i;
		int MASK = 1;
		for(i=0; i<NUM_CHANNELS; i++) {
			if((h->channels & MASK) > 0) {
				fprintf(data,",%s",channel_names[i]);
			}
			MASK = MASK << 1;
		}
		fprintf(data,"\n");
	}
	
	return SUCCESS;
}

int update_sample_number(FILE* data, struct header* h, int binary) {
	
	// store file pointer position
	long pos = ftell(data);
	rewind(data);
	
	// only store header until number samples updated
	if (binary == 1) {
		fwrite(h, 2, sizeof(int), data);
	} else {
		fprintf(data, "Header Length:,%d\n", h->header_length);
		fprintf(data, "Number Samples:,%-12d\n", h->number_samples);
	}
	
	fseek(data, pos, SEEK_SET);
	
	
	return SUCCESS;
}

// buffer
int store_buffer(FILE* data, int fifo_fd, int control_fifo, void* virt_addr, int buffer_number, unsigned int samples_buffer, unsigned int size, int channels, struct timeval* current_time, int store, int binary, int webserver) {
	
	int i = 0;
	int j = 0;
	int k = 2;
	int MASK = 1;
	int num_channels = count_bits(channels);
	
	// binary data
	int line_int[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
	int value_int = 0;
	
	// csv data TODO: defines
	char line_char[200] = "";
	char value_char[20];
	
	// webserver data
	int avg_buffer_size = ceil_div(samples_buffer, WEB_BUFFER_SIZE); // size of buffer to average
	float web_data[WEB_BUFFER_SIZE][NUMBER_WEB_CHANNELS];
	memset(web_data, 0, WEB_BUFFER_SIZE * NUMBER_WEB_CHANNELS * sizeof(float)); // reset data (needed due to  unfull buffer sizes)
	
	// store header
	if(buffer_number == 0 && store == 1) {
		store_header(data, &file_header, binary);
	}
	
	// store buffer
    for(i=0; i<samples_buffer; i++){
		
		// reset values
		strcpy(line_char,"\0");
		k = 2;
		MASK = 1;
		
		// store timestamp
		if (i == 0 && store == 1) {
			time_t nowtime = current_time->tv_sec;
			struct tm *nowtm = localtime(&nowtime);
			
			if (binary == 1) {
				fwrite(nowtm, 4, 9, data);
			} else {
				sprintf (value_char, "%s", ctime (&nowtime));
				value_char[strlen(value_char)-1] = '\0'; // remove \n
				strcat(line_char,value_char);
			}
		}
		
		// read status
		line_int[0] = (int) (*((int8_t *) (virt_addr)));
		line_int[1] = (int) (*((int8_t *) (virt_addr + 1)));
		virt_addr += STATUSSIZE;
		
		// read and scale values (if channel selected)
		for(j=0; j<NUM_CHANNELS; j++) {
			if((channels & MASK) > 0) {
				if(size == 4) {
					value_int = *( (int32_t *) (virt_addr + 4*j) );
					line_int[k] = (int) (( value_int + offsets24[j] ) * scales24[j]);
				} else {
					value_int = *( (int16_t *) (virt_addr + 2*j) );
					line_int[k] = (int) (( value_int + offsets16[j] ) * scales16[j]);
				}
				k++;
			}
			MASK = MASK << 1;
		}
		virt_addr+=NUM_CHANNELS*size;
		
		// store values to file
		if (store == 1) {
			if (binary == 1) {
				// binary
				fwrite(line_int,4,num_channels+2,data);
			} else {
				// csv
				for(j=0; j < num_channels+2; j++) {
					sprintf(value_char,",%d",line_int[j]);
					strcat(line_char, value_char);
				}
				strcat(line_char,"\n");
				fprintf(data, line_char);
			}
		}
		
		// collapse and average values for webserver
		if (webserver == 1) {
			if (i % avg_buffer_size == 0) { // only store first sample
				collapse_data(web_data[i/avg_buffer_size], line_int, channels); //collapse data to webserver buffer
				//Todo: average data
			}
		}
    }
	
	// store values for webserver
	if (webserver == 1) {
		store_web_data(fifo_fd, control_fifo, &web_data[0][0]);
	}
	
	if (store == 1) {
		// update the number of samples stored
		file_header.number_samples += samples_buffer;
		
		update_sample_number(data, &file_header, binary);
		
	}
	
	return SUCCESS;
}

// store webserver data to fifo
int store_web_data(int fifo_fd, int control_fifo, float* buffer) {
	
	// check control fifo
	int tmp;
	int check = read(control_fifo, &tmp, sizeof(int));
	
	// only write to data fifo, if webserver online
	if(check > 0) {
		check = write(fifo_fd, buffer, sizeof(float) * WEB_BUFFER_SIZE * NUMBER_WEB_CHANNELS);
	}
	
	// count number of tokens in control fifo (count will be the number of listeners) ---> not working very nice
	/*int tmp;
	int count = 0;
	int check = read(control_fifo, &tmp, sizeof(int));
	while(check > 3) {
		count++;
		check = read(control_fifo, &tmp, sizeof(int));
	}
	
	printf("Listener count: %d\n", count);
	
	// write data for every user once
	int i;
	for(i=0; i<count; i++) {
		check = write(fifo_fd, buffer, sizeof(float) * WEB_BUFFER_SIZE * NUMBER_WEB_CHANNELS);
	}*/
	
	return SUCCESS;
}

// collapse current channels for webserver plotting (always take highest selected I-channel)
void collapse_data(float* data_out, int* data_in, int channels) {
	
	int j = 2; // first two elements are status
	// specific number of channels
	int num_i1 = count_bits(channels & I1A);
	int num_i2 = count_bits(channels & I2A);
	
	// i1
	if(num_i1 != 0) {
		if ( (channels & I1L) > 0 && num_i1 == 1) {
			data_out[0] = (float)data_in[j]/100000; // conversion to uA
		} else {
			data_out[0] = (float)data_in[j]/1000;
		}
		j += num_i1;
	} else {
		data_out[0] = 0;
	}
	
	// v1,2
	if((channels & V1) > 0) {
		data_out[1] = (float)data_in[j]/1000; // conversion to mV
		j++;
	} else {
		data_out[1] = 0;
	}
	if((channels & V2) > 0) {
		data_out[2] = (float)data_in[j]/1000;
		j++;
	} else {
		data_out[2] = 0;
	}
	
	// i2
	if(num_i2 != 0) {
		if ( (channels & I2L) > 0  && num_i2 == 1) {
			data_out[3] = (float)data_in[j]/100000;
		} else {
			data_out[3] = (float)data_in[j]/1000;
		}
		j += num_i2;
	} else {
		data_out[3] = 0;
	}
	
	// v3,4
	if((channels & V3) > 0) {
		data_out[4] = (float)data_in[j]/1000;
		j++;
	} else {
		data_out[4] = 0;
	}
	if((channels & V4) > 0) {
		data_out[5] = (float)data_in[j]/1000;
	} else {
		data_out[5] = 0;
	}

}

// ToDo: use, remove?
/*void average_data(double* data_out, int* data_in, int length, int num_channels) {
	int i;
	int j;
	long temp_data[num_channels];
	
	// reset data
	for (i=0; i < num_channels; i++) {
		temp_data[i] = 0;
	}
	
	// add data
	for( i=0; i<length; i++ ) {
		for ( j=0; j<num_channels; j++) {
			temp_data[j] += (long) data_in[i*num_channels + j];
		}
	}
	for (i=0; i< num_channels; i++) {
		printf("%-10lu", temp_data[i]);
	}
	printf("\n");
	
	// divide data
	for ( j=0; j<num_channels; j++) {
		data_out[j] = (temp_data[j] / length);
	}
}*/

// meter
void print_meter(void* virt_addr, unsigned int samples_buffer, unsigned int size, int channels) {
	
	// print header
	printf("\n\n\n\n\n\n\n\n\n\nRocketLogger Meter\n\n");
	
	if ((channels & I1A) > 0 ) {
		printf("%-20s","LOW1");
	}
	if ((channels & I2A) > 0 ) {
		printf("LOW2");
	}
	printf("\n");
	
	// print status if channels selected
	if ((channels & I1A) > 0 ) {
		printf("%-20d",(int)(*((int8_t *) (virt_addr))));
	}
	if ((channels & I2A) > 0 ) {
		printf("%-20d",(int)(*((int8_t *) (virt_addr + 1))));
	}
	printf("\n\n");
	virt_addr += STATUSSIZE;
	
	int i;
	int j;
	int k;
	int MASK_NAME = 1;
	int MASK = 1;
	int avg_number = 10;
	long value;
	for(i=0; i<3; i++) {
		if((channels & MASK_NAME) > 0) {
			printf("%-20s", channel_names[i]);
		}
		MASK_NAME = MASK_NAME << 1;
	}
	printf("\n");
	
	// print i1
	for(j=0; j<3; j++) {
		value = 0;
		if((channels & MASK) > 0) {
			if(size == 4) {
				for (k=0; k<avg_number; k++) {
					value += (long) (((*((int32_t *) (virt_addr + k * (4*NUM_CHANNELS + STATUSSIZE) + j*4))) + offsets24[j]) * scales24[j]);
				}
				value = value / (long) avg_number;
			} else {
				for (k=0; k<samples_buffer; k++) {
					value = (long) (((*((int16_t *) (virt_addr + k * (2*NUM_CHANNELS + STATUSSIZE) + j*2))) + offsets16[j]) * scales16[j]); // do not average here
				}
			}
			printf("%-20d", (int) value); //---> for all channels!!
		}
		MASK = MASK << 1;
	}
	printf("\n\n");
	
	// print v1,2
	for(; i<5; i++) {
		if((channels & MASK_NAME) > 0) {
			printf("%-20s", channel_names[i]);
		}
		MASK_NAME = MASK_NAME << 1;
	}
	printf("\n");
	
	for(; j<5; j++) {
		value = 0;
		if((channels & MASK) > 0) {
			if(size == 4) {
				for (k=0; k<avg_number; k++) {
					value += (long) (((*((int32_t *) (virt_addr + k * (4*NUM_CHANNELS + STATUSSIZE) + j*4))) + offsets24[j]) * scales24[j]);
				}
				value = value / (long) avg_number;
			} else {
				for (k=0; k<samples_buffer; k++) {
					value = (long) (((*((int16_t *) (virt_addr + k * (2*NUM_CHANNELS + STATUSSIZE) + j*2))) + offsets16[j]) * scales16[j]); // do not average here
				}
			}
			printf("%-20d", (int) value);
		}
		MASK = MASK << 1;
	}
	printf("\n\n");
	
	
	// print i2
	for(; i<8; i++) {
		if((channels & MASK_NAME) > 0) {
			printf("%-20s", channel_names[i]);
		}
		MASK_NAME = MASK_NAME << 1;
	}
	printf("\n");
	
	for(; j<8; j++) {
		value = 0;
		if((channels & MASK) > 0) {
			if(size == 4) {
				for (k=0; k<avg_number; k++) {
					value += (long) (((*((int32_t *) (virt_addr + k * (4*NUM_CHANNELS + STATUSSIZE) + j*4))) + offsets24[j]) * scales24[j]);
				}
				value = value / (long) avg_number;
			} else {
				for (k=0; k<samples_buffer; k++) {
					value = (long) (((*((int16_t *) (virt_addr + k * (2*NUM_CHANNELS + STATUSSIZE) + j*2))) + offsets16[j]) * scales16[j]); // do not average here
				}
			}
			printf("%-20d", (int) value);
		}
		MASK = MASK << 1;
	}
	printf("\n\n");
	
	
	// print v3,4
	for(; i<10; i++) {
		if((channels & MASK_NAME) > 0) {
			printf("%-20s", channel_names[i]);
		}
		MASK_NAME = MASK_NAME << 1;
	}
	printf("\n");
	
	for(; j<10; j++) {
		value = 0;
		if((channels & MASK) > 0) {
			if(size == 4) {
				for (k=0; k<avg_number; k++) {
					value += (long) (((*((int32_t *) (virt_addr + k * (4*NUM_CHANNELS + STATUSSIZE) + j*4))) + offsets24[j]) * scales24[j]);
				}
				value = value / (long) avg_number;
			} else {
				for (k=0; k<samples_buffer; k++) {
					value = (long) (((*((int16_t *) (virt_addr + k * (2*NUM_CHANNELS + STATUSSIZE) + j*2))) + offsets16[j]) * scales16[j]); // do not average here
				}
			}
			printf("%-20d", (int) value);
		}
		MASK = MASK << 1;
	}
	printf("\n\n\n\n\n\n\n\n\n\n");
}


// ------------------------------  PRU FUNCTIONS ------------------------------ //

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

int pru_wait_event_timeout(unsigned int event, unsigned int timeout) { //TODO: use everywhere
	
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


// set state to PRU
int pru_set_state(enum pru_states state){ // TODO void functions
	
	pru.state = state;
	prussdrv_pru_write_memory(PRUSS0_PRU0_DATARAM, 0, (unsigned int*) &pru, sizeof(int));
	
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

// main sample function
int pru_sample(FILE* data, struct rl_conf* conf) {
	
	
	// TODO temporary solution !!!!!!!!
	int j;
	int MASK = 1;
	int channels = 0;
	int binary;
	int store;
	for(j=0; j<NUM_CHANNELS; j++) {
		if(conf->channels[j] > 0) {
			channels = channels | MASK;
		}
		MASK = MASK << 1;
	}
	if(conf->file_format == BIN) {
		binary = 1;
	} else {
		binary = 0;
	}
	if(conf->file_format == NO_FILE || conf->mode == METER) {
		store = 0;
	} else {
		store = 1;
	}
	
	unsigned int pru_sample_rate;
	unsigned int buffer_size_bytes;
	unsigned int number_buffers;
	
	// running state
	status.state = RL_RUNNING;
	status.samples_taken = 0;
	status.buffer_number = 0;
	
	// set up fifo for webserver data //TODO in hw_layer
	int fifo_fd = -1;
	int control_fifo = -1;
	
	if (conf->enable_web_server == 1) {// TODO: in hw_layer
		
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
	
	// set configuration
	if(conf->mode == LIMIT) {
		pru.state = PRU_LIMIT;
	} else {
		pru.state = PRU_CONTINUOUS;
	}
	
	// set sampling rate configuration
	switch (conf->sample_rate) {
		case 1:
			pru_sample_rate = K1;
			pru.precision = PRECISION_HIGH;
			pru.sample_size = SIZE_HIGH;
			break;
		case 2:
			pru_sample_rate = K2;
			pru.precision = PRECISION_HIGH;
			pru.sample_size = SIZE_HIGH;
			break;
		case 4:
			pru_sample_rate = K4;
			pru.precision = PRECISION_HIGH;
			pru.sample_size = SIZE_HIGH;
			break;
		case 8:
			pru_sample_rate = K8;
			pru.precision = PRECISION_HIGH;
			pru.sample_size = SIZE_HIGH;
			break;
		case 16:
			pru_sample_rate = K16;
			pru.precision = PRECISION_HIGH;
			pru.sample_size = SIZE_HIGH;
			break;
		case 32:
			pru_sample_rate = K32;
			pru.precision = PRECISION_LOW;
			pru.sample_size = SIZE_LOW;
			break;
		case 64:
			pru_sample_rate = K64;
			pru.precision = PRECISION_LOW;
			pru.sample_size = SIZE_LOW;
			break;
		default:
			rl_log(ERROR, "wrong sample rate");
			return FAILURE;
	}
	pru.sample_limit = conf->sample_limit;
	pru.buffer_size = (conf->sample_rate * 1000) / conf->update_rate;
	
	number_buffers = ceil_div(conf->sample_limit, pru.buffer_size);
	buffer_size_bytes = pru.buffer_size * (pru.sample_size * NUM_CHANNELS + STATUSSIZE) + BUFFERSTATUSSIZE;
	
	pru.buffer0_location = read_file_value(MMAP_FILE "addr");
	pru.buffer1_location = pru.buffer0_location + buffer_size_bytes;
	
	pru.number_commands = NUMBER_PRU_COMMANDS;
	
	pru.commands[0] = RESET;
	pru.commands[1] = SDATAC;
	pru.commands[2] = WREG|CONFIG3|CONFIG3DEFAULT;						// write configuration
	pru.commands[3] = WREG|CONFIG1|CONFIG1DEFAULT | pru_sample_rate;
	pru.commands[4] = WREG|CH1SET|GAIN2;								// set channel gains
	pru.commands[5] = WREG|CH2SET|GAIN1;
	pru.commands[6] = WREG|CH3SET|GAIN1;
	pru.commands[7] = WREG|CH4SET|GAIN1;
	pru.commands[8] = WREG|CH5SET|GAIN1;
	pru.commands[9] = RDATAC;											// continuous reading
	
	
	// save file header
	if (binary == 1) {
		file_header.header_length = HEADERLENGTH;
	} else {
		file_header.header_length = HEADERLENGTH + 1;
	}
	file_header.number_samples = 0; // number of samples taken
	file_header.buffer_size = pru.buffer_size;
	file_header.rate = pru_sample_rate;
	file_header.channels = channels;
	file_header.precision = pru.precision;
	
	
	// check memory size
	unsigned int max_size = read_file_value(MMAP_FILE "size");
	if(2*buffer_size_bytes > max_size) {
		char message[100];
		sprintf(message, "not enough memory allocated. Run:\n  rmmod uio_pruss\n  modprobe uio_pruss extram_pool_sz=0x%06x", 2*buffer_size_bytes);
		rl_log(ERROR, message);
		pru.state = PRU_OFF; // TODO: remove?
		status.state = RL_OFF;
	}
	
	// map PRU memory into userspace
	void* map_base = memory_map(pru.buffer0_location, MAP_SIZE); // map base placed on start of block
	void* buffer0 = map_base + ( (off_t) pru.buffer0_location & MAP_MASK);
	void* buffer1 = buffer0 + buffer_size_bytes; 
	
	// write configuration to PRU memory
	prussdrv_pru_write_memory(PRUSS0_PRU0_DATARAM, 0, (unsigned int*) &pru, sizeof(struct pru_data_struct));	

	// run SPI on PRU0
	prussdrv_exec_program (0, PRU_CODE);

	int i;
	void* addr;
	unsigned int samples_buffer; // number of samples per buffer
	
	// continuous sampling loop
	for(i=0; status.state == RL_RUNNING && !(conf->mode == LIMIT && i>=number_buffers); i++) {
		
		// select current buffer
		if(i%2 == 0) {
			addr = buffer0;
		} else {
			addr = buffer1;
		}
		// select buffer size
		if(i < number_buffers-1 || pru.sample_limit % pru.buffer_size == 0) {
			samples_buffer = pru.buffer_size; // full buffer size
		} else {
			samples_buffer = pru.sample_limit % pru.buffer_size;
		}
		
		// Wait for event completion from PRU
		if (test_mode == 0) {
			if(pru_wait_event_timeout(PRU_EVTOUT_0, TIMEOUT) == ETIMEDOUT) {
				// timeout occured
				rl_log(ERROR, "ADC timout. Stopping ...");
				status.state = RL_ERROR;
				break;
			}
		} else {
			sleep(1);
		}
		
		// create timestamp
		struct timeval current_time;
		gettimeofday(&current_time,NULL);
		current_time.tv_sec -= 1 / conf->update_rate; // adjust with buffer latency
		
		// clear event
		prussdrv_pru_clear_event(PRU_EVTOUT_0, PRU0_ARM_INTERRUPT);

		
		// check for overrun (compare buffer numbers)
		if (test_mode == 0) {
			int buffer = *((uint32_t*) addr);
			if (buffer != i) {
				char message[100];
				sprintf(message, "overrun: %d samples (%d buffer) lost", (buffer - i) * pru.buffer_size, buffer - i);
				rl_log(WARNING, message);
				i = buffer;
			}
		}
		
		// store the buffer
		store_buffer(data, fifo_fd, control_fifo, addr+4, i, samples_buffer, pru.sample_size, channels, &current_time, store, binary, conf->enable_web_server);
		
		// update and write state
		status.samples_taken += samples_buffer;
		status.buffer_number = i;
		write_status(&status);
		
		// print meter output
		if(conf->mode == METER) {
			print_meter(addr + 4, samples_buffer, pru.sample_size, channels);
		}
	}
	
	// flush data if no error occured
	if (store == 1 && status.state != RL_ERROR) {
		// print info
		char message[100];
		sprintf(message, "stored %d samples to file", status.samples_taken);
		rl_log(INFO, message);
		
		printf("Stored %d samples to file.\n", status.samples_taken);
		
		fflush(data);
	}
	
	// unmap memory
	memory_unmap(map_base,MAP_SIZE);
	
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

// stop PRU when in continuous mode
int pru_stop() {
	// write OFF to PRU state (so PRU can clean up)
	pru_set_state(PRU_OFF);
	
	// wait for interrupt (if no ERROR occured) -> TODO: use wait&timeout function
	if(status.state != RL_ERROR) {
		pru_wait_event_timeout(PRU_EVTOUT_0, TIMEOUT);
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
