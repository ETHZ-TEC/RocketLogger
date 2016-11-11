#include "file_handling.h"

char channel_names[NUM_CHANNELS][RL_FILE_CHANNEL_NAME_LENGTH] = {"I1H","I1L","V1","V2","I2H","I2L","V3","V4"};
char digital_input_names[NUM_DIGITAL_INPUTS][RL_FILE_CHANNEL_NAME_LENGTH] = {"DigIn1", "DigIn2", "DigIn3", "DigIn4", "DigIn5", "DigIn6"};
int digital_input_bits[NUM_DIGITAL_INPUTS] = {DIGIN1_BIT, DIGIN2_BIT, DIGIN3_BIT, DIGIN4_BIT, DIGIN5_BIT, DIGIN6_BIT}; // TODO: combine with meter?
char valid_info_names[NUM_I_CHANNELS][RL_FILE_CHANNEL_NAME_LENGTH] = {"I1L_valid", "I2L_valid"};

/// Global variable to determine i1l valid channel
int i1l_valid_channel = 0;
/// Global variable to determine i2l valid channel
int i2l_valid_channel = 0;


time_t create_timestamp(struct rl_conf* conf) {
	struct timeval time;
	gettimeofday(&time, NULL);
	time.tv_sec -= 1 / conf->update_rate; // adjust with buffer latency
	return time.tv_sec;
}

// TODO: rename, adjust with buffer latency, if needed
void create_time_stamp_new(struct time_stamp* time_real, struct time_stamp* time_monotonic) {
	
	struct timespec spec_real;
	struct timespec spec_monotonic;
	
	// get time stamp of real-time and monotonic clock
	clock_gettime(CLOCK_REALTIME, &spec_real);
	clock_gettime(CLOCK_MONOTONIC_RAW, &spec_monotonic);
	
	/*if( TODO ) {
		rl_log(ERROR, "failed to get time");
	}*/
	
	// convert to own time stamp
	time_real->sec = (int64_t) spec_real.tv_sec;
	time_real->nsec = (int64_t) spec_real.tv_nsec;
	time_monotonic->sec = (int64_t) spec_monotonic.tv_sec;
	time_monotonic->nsec = (int64_t) spec_monotonic.tv_nsec;
	
	
}


// NEW HEADER

void get_mac_addr(uint8_t mac_address[MAC_ADDRESS_LENGTH]) {
	
	FILE* fp = fopen (MAC_ADDRESS_FILE, "r");
	
	int i=0;
	fscanf(fp, "%x", &mac_address[i]);
	for(i=1; i<MAC_ADDRESS_LENGTH; i++) {
		fscanf(fp, ":%x", &mac_address[i]);
	}
	fclose(fp);
}

void setup_lead_in(struct rl_file_lead_in* lead_in, struct rl_conf* conf) {
	
	// number channels
	int channel_count = count_channels(conf->channels);
	// number binary channels
	int channel_bin_count = 0;
	if(conf->digital_inputs == DIGITAL_INPUTS_ENABLED) {
		channel_bin_count = NUM_DIGITAL_INPUTS;
	}
	if(conf->channels[I1L_INDEX] > 0) {
		i1l_valid_channel = ++channel_bin_count;
	}
	if(conf->channels[I2L_INDEX] > 0) {
		i2l_valid_channel = ++channel_bin_count;
	}
	// comment length
	int comment_length = strlen(RL_FILE_COMMENT) * sizeof(int8_t);
	// timestamps
	struct time_stamp time_real;
	struct time_stamp time_monotonic;
	create_time_stamp_new(&time_real, &time_monotonic);
	
	
	// lead_in setup
	lead_in->magic = RL_FILE_MAGIC;
	lead_in->file_version = RL_FILE_VERSION;
	lead_in->header_length = sizeof(struct rl_file_lead_in) + comment_length + (channel_count + channel_bin_count) * sizeof(struct rl_file_channel);
	lead_in->data_block_size = conf->sample_rate*RATE_SCALING / conf->update_rate;
	lead_in->data_block_count = 0; // needs to be updated
	lead_in->sample_count = 0; // needs to be updated
	lead_in->sample_rate = conf->sample_rate * RATE_SCALING;
	get_mac_addr(lead_in->mac_address);
	lead_in->start_time = time_real;
	lead_in->comment_length = comment_length;
	lead_in->channel_bin_count = channel_bin_count;
	lead_in->channel_count = channel_count;
	
}

void setup_channels(struct rl_file_header* file_header, struct rl_conf* conf) {
	
	int i;
	int j;
	int total_channel_count = file_header->lead_in.channel_bin_count + file_header->lead_in.channel_count;
	
	// reset channels
	memset(file_header->channel, 0, total_channel_count * sizeof(struct rl_file_channel));
	
	// digital channels
	j=0;
	if(conf->digital_inputs == DIGITAL_INPUTS_ENABLED) {
		for(i=0; i<NUM_DIGITAL_INPUTS; i++) {
			file_header->channel[j].unit = RL_UNIT_BINARY;
			file_header->channel[j].channel_scale = RL_SCALE_NONE;
			file_header->channel[j].data_size = 0;
			file_header->channel[j].valid_data_channel = NO_VALID_DATA;
			strcpy(file_header->channel[j].name, digital_input_names[i]);
			j++;
		}
	}
	
	// range valid channels
	if(conf->channels[I1L_INDEX] > 0) {
		file_header->channel[j].unit = RL_UNIT_RANGE_VALID;
		file_header->channel[j].channel_scale = RL_SCALE_NONE;
		file_header->channel[j].data_size = 0;
		file_header->channel[j].valid_data_channel = NO_VALID_DATA;
		strcpy(file_header->channel[j].name, valid_info_names[0]);
		j++;
	}
	if(conf->channels[I2L_INDEX] > 0) {
		file_header->channel[j].unit = RL_UNIT_RANGE_VALID;
		file_header->channel[j].channel_scale = RL_SCALE_NONE;
		file_header->channel[j].data_size = 0;
		file_header->channel[j].valid_data_channel = NO_VALID_DATA;
		strcpy(file_header->channel[j].name, valid_info_names[1]);
		j++;
	}
	
	// analog channels
	for(i=0; i<NUM_CHANNELS; i++) {
		if(conf->channels[i] > 0) {
			// current
			if(is_current(i)){
				// low
				if(is_low_current(i)) {
					file_header->channel[j].channel_scale = RL_SCALE_TEN_PICO;
					if(i == I1L_INDEX) {
						file_header->channel[j].valid_data_channel = i1l_valid_channel;
					} else {
						file_header->channel[j].valid_data_channel = i2l_valid_channel;
					}
				// high
				} else {
					file_header->channel[j].channel_scale = RL_SCALE_NANO;
					file_header->channel[j].valid_data_channel = NO_VALID_DATA;
				}
				file_header->channel[j].unit = RL_UNIT_AMPERE;
			// voltage
			} else {
				file_header->channel[j].unit = RL_UNIT_VOLT;
				file_header->channel[j].channel_scale = RL_SCALE_MICRO;
				file_header->channel[j].valid_data_channel = NO_VALID_DATA;
			}
			file_header->channel[j].data_size = 4;
			strcpy(file_header->channel[j].name, channel_names[i]);
			j++;
		}
	}
}


void setup_header_new(struct rl_file_header* file_header, struct rl_conf* conf) {
	
	// comment
	char* comment = RL_FILE_COMMENT;
	file_header->comment = comment;
	
	// channels
	setup_channels(file_header, conf);
	
}

void store_header_new(FILE* data, struct rl_file_header* file_header) {
	
	int total_channel_count = file_header->lead_in.channel_bin_count + file_header->lead_in.channel_count;
	
	// write lead in
	fwrite(&(file_header->lead_in), sizeof(struct rl_file_lead_in), 1, data);
	// write comment
	fwrite(file_header->comment, file_header->lead_in.comment_length, 1, data);
	// write channel information
	fwrite(file_header->channel, sizeof(struct rl_file_channel), total_channel_count, data);
	
}

void update_header(FILE* data, struct rl_file_header* file_header) {
	
	// seek to beginning and rewrite lead_in
	rewind(data);
	fwrite(&(file_header->lead_in), sizeof(struct rl_file_lead_in), 1, data);
	fseek(data, 0, SEEK_END);
}

void merge_currents(int8_t valid1, int8_t valid2, int32_t* dest, int64_t* src, struct rl_conf* conf) {
	
	int ch_in = 0;
	int ch_out = 0;
	
		
	if(conf->channels[I1H_INDEX] > 0 && conf->channels[I1L_INDEX] > 0) {
		if(valid1 == 1) {
			dest[ch_out++] = (int32_t) src[++ch_in]/H_L_SCALE;
		} else {
			dest[ch_out++] = (int32_t) src[ch_in++];
		}
		ch_in++;
	} else if(conf->channels[I1H_INDEX] > 0) {
		dest[ch_out++] = (int32_t) src[ch_in++];
	} else if(conf->channels[I1L_INDEX] > 0){
		dest[ch_out++] = (int32_t) src[ch_in++]/H_L_SCALE;
	}
	if(conf->channels[V1_INDEX] > 0) {
		dest[ch_out++] = (int32_t) src[ch_in++];
	}
	if(conf->channels[V2_INDEX] > 0) {
		dest[ch_out++] = (int32_t) src[ch_in++];
	}
	
	if(conf->channels[I2H_INDEX] > 0 && conf->channels[I2L_INDEX] > 0) {
		if(valid2 == 1) {
			dest[ch_out++] = (int32_t) src[++ch_in]/H_L_SCALE;
		} else {
			dest[ch_out++] = (int32_t) src[ch_in++];
		}
		ch_in++;
	} else if(conf->channels[I2H_INDEX] > 0) {
		dest[ch_out++] = (int32_t) src[ch_in++];
	} else if(conf->channels[I2L_INDEX] > 0){
		dest[ch_out++] = (int32_t) src[ch_in++]/H_L_SCALE;
	}
	
	if(conf->channels[V3_INDEX] > 0) {
		dest[ch_out++] = (int32_t) src[ch_in++];
	}
	if(conf->channels[V4_INDEX] > 0) {
		dest[ch_out++] = (int32_t) src[ch_in++];
	}
}

int test = 0;
int store_buffer_new(FILE* data, void* buffer_addr, unsigned int sample_size, int samples_buffer, struct rl_conf* conf, int sem_id, struct web_shm* web_data) {
	
	int i;
	int j;
	int k;
	
	int num_bin_channels;
	if(conf->digital_inputs == DIGITAL_INPUTS_ENABLED) {
		num_bin_channels = NUM_DIGITAL_INPUTS;
	} else {
		num_bin_channels = 0;
	}
	
	int num_channels = count_channels(conf->channels);
	
	// TODO: TEST
	// int num_web_channels = web_data->num_channels;
	
	int num_web_channels = num_channels + num_bin_channels;
	if(conf->channels[I1H_INDEX] > 0 && conf->channels[I1L_INDEX] > 0) {
		num_web_channels--;
	}
	if(conf->channels[I2H_INDEX] > 0 && conf->channels[I2L_INDEX] > 0) {
		num_web_channels--;
	}
	
	
	// create timestamp
	struct time_stamp time_real;
	struct time_stamp time_monotonic;
	create_time_stamp_new(&time_real, &time_monotonic);
	
	// adjust time with buffer latency
	time_real.sec -= 1 / conf->update_rate;
	time_monotonic.sec -= 1 / conf->update_rate;
	
	// store timestamp
	if (conf->file_format != NO_FILE) {
			fwrite(&time_real, sizeof(struct time_stamp), 1, data);
			fwrite(&time_monotonic, sizeof(struct time_stamp), 1, data);
	}
	
	
	// binary data (for status and samples)
	uint32_t bin_data;
	int32_t channel_data[num_channels];
	
	int value = 0;
	
	// data for web interface, TODO: as arrays
	int avg_number = samples_buffer / BUFFER1_SIZE;
	int avg_number10 = samples_buffer / BUFFER10_SIZE;
	int avg_number100 = samples_buffer / BUFFER100_SIZE;
	
	int64_t avg_data[num_channels];
	int64_t avg_data10[num_channels];
	int64_t avg_data100[num_channels];
	
	memset(avg_data, 0, sizeof(int64_t) * num_channels);
	memset(avg_data10, 0, sizeof(int64_t) * num_channels);
	memset(avg_data100, 0, sizeof(int64_t) * num_channels);
		
	int32_t bin_web_data[num_bin_channels];
	int32_t bin_web_data10[num_bin_channels];
	int32_t bin_web_data100[num_bin_channels];
	
	memset(bin_web_data, 0, sizeof(int32_t) * num_bin_channels);
	memset(bin_web_data10, 0, sizeof(int32_t) * num_bin_channels);
	memset(bin_web_data100, 0, sizeof(int32_t) * num_bin_channels);
	
	uint8_t web_valid1 = 1;
	uint8_t web_valid2 = 1;
	uint8_t web_valid1_10 = 1;
	uint8_t web_valid2_10 = 1;
	uint8_t web_valid1_100 = 1;
	uint8_t web_valid2_100 = 1;
	
	int32_t temp_web_data[BUFFER1_SIZE][num_web_channels];
	int32_t temp_web_data10[BUFFER10_SIZE][num_web_channels];
	int32_t temp_web_data100[BUFFER100_SIZE][num_web_channels];

	
	// store buffer
    for(i=0; i<samples_buffer; i++){
		
		// reset values
		k = 0;
		bin_data = 0;
		
		
		// read binary channels
		uint8_t bin_adc1 = (*((int8_t *) (buffer_addr)));
		uint8_t bin_adc2 = (*((int8_t *) (buffer_addr + 1)));
		
		buffer_addr += STATUS_SIZE; // TODO: rename to digital_size ...
		
		// mask and combine digital inputs, if requestet
		int bin_channel_pos;
		if(conf->digital_inputs == DIGITAL_INPUTS_ENABLED) {
			bin_data = ((bin_adc1 & BINARY_MASK) >> 1) | ((bin_adc2 & BINARY_MASK) << 2);
			bin_channel_pos = NUM_DIGITAL_INPUTS;
			
			// average for web
			if(conf->enable_web_server == 1) {
				int j;
				int32_t MASK = 1;
				for(j=0; j<num_bin_channels; j++) {
					if((bin_data & MASK) > 0) {
						bin_web_data[j] += 1;
					}
					MASK = MASK << 1;
				}
			}
			
		} else {
			bin_channel_pos = 0;
		}
		
		// Mask valid info
		uint8_t valid1 = (~bin_adc1) & VALID_MASK;
		uint8_t valid2 = (~bin_adc2) & VALID_MASK;
		
		// web
		if(conf->enable_web_server == 1) {
			web_valid1 = web_valid1 & valid1;
			web_valid2 = web_valid2 & valid2;
			web_valid1_10 = web_valid1_10 & valid1;
			web_valid2_10 = web_valid2_10 & valid2;
			web_valid1_100 = web_valid1_100 & valid1;
			web_valid2_100 = web_valid2_100 & valid2;
		}
		
		if(conf->channels[I1L_INDEX] > 0) {
			bin_data = bin_data | (valid1 << bin_channel_pos);
			bin_channel_pos++;
		}
		if(conf->channels[I2L_INDEX] > 0) {
			bin_data = bin_data | (valid2 << bin_channel_pos);
		}
		
		// write binary channels
		if (conf->file_format != NO_FILE && bin_channel_pos > 0) {
			fwrite(&bin_data, sizeof(uint32_t), 1, data);
		}
		
		// read and scale values (if channel selected)
		for(j=0; j<NUM_CHANNELS; j++) {
			if(conf->channels[j] > 0) {
				if(sample_size == 4) {
					if (TEST_MODE == 1) {
						value = 1000*j + test++;
					} else {
						value = *( (int32_t *) (buffer_addr + sample_size*j) );
					}
				} else {
					value = *( (int16_t *) (buffer_addr + sample_size*j) );
				}
				channel_data[k] = (int) (( value + offsets[j] ) * scales[j]);
				avg_data[k] += channel_data[k];
				k++;
			}
		}
		buffer_addr+=NUM_CHANNELS*sample_size;
		
		// store values to file
		if (conf->file_format != NO_FILE) {
			fwrite(channel_data, sizeof(int32_t), num_channels, data);
		}
		
		if(conf->enable_web_server == 1) {
			if((i+1)%avg_number == 0) {
				
				// average
				int j;
				for(j=0; j<num_channels; j++) {
					avg_data[j] /= avg_number; // TODO: exchange?
					avg_data10[j] += avg_data[j];
				}
				
				// merge_currents
				merge_currents(web_valid1, web_valid2, &temp_web_data[i/avg_number][num_bin_channels], avg_data, conf);
				
				// bin channels
				for(j=0; j<num_bin_channels; j++) {
					
					bin_web_data10[j] += bin_web_data[j];
					
					if(bin_web_data[j] >= (avg_number/2)) {
						temp_web_data[i/avg_number][j] = 1;
					} else {
						temp_web_data[i/avg_number][j] = 0;
					}
					bin_web_data[j] = 0;
				}
				
				// reset values
				memset(avg_data, 0, sizeof(int64_t) * num_channels);
				web_valid1 = 1;
				web_valid2 = 1;
			}
			
			if((i+1)%avg_number10 == 0) {
				
				// average
				int j;
				for(j=0; j<num_channels; j++) {
					avg_data10[j] /= (avg_number10/avg_number);
					avg_data100[j] += avg_data10[j];
				}
				
				// merge_currents
				merge_currents(web_valid1_10, web_valid2_10, &temp_web_data10[i/avg_number10][num_bin_channels], avg_data10, conf);
				
				// bin channels
				for(j=0; j<num_bin_channels; j++) {
					
					bin_web_data100[j] += bin_web_data10[j];
					
					if(bin_web_data10[j] >= (avg_number10/2)) {
						temp_web_data10[i/avg_number10][j] = 1;
					} else {
						temp_web_data10[i/avg_number10][j] = 0;
					}
					bin_web_data10[j] = 0;
				}
				
				// reset values
				memset(avg_data10, 0, sizeof(int64_t) * num_channels);
				web_valid1_10 = 1;
				web_valid2_10 = 1;
			}
			
			if((i+1)%avg_number100 == 0) {
				
				// average
				int j;
				for(j=0; j<num_channels; j++) {
					avg_data100[j] /= (avg_number100/avg_number10);
				}
				
				// merge_currents
				merge_currents(web_valid1_100, web_valid2_100, &temp_web_data100[i/avg_number100][num_bin_channels], avg_data100, conf);
				
				// bin channels
				for(j=0; j<num_bin_channels; j++) {
					
					if(bin_web_data10[j] >= (avg_number100/2)) {
						temp_web_data100[i/avg_number100][j] = 1;
					} else {
						temp_web_data100[i/avg_number100][j] = 0;
					}
				}
			}
		}
    }
	
	if (conf->enable_web_server == 1) {
		
		// get shared memory access
		if(wait_sem(sem_id, DATA_SEM, SEM_WRITE_TIME_OUT) == TIME_OUT) {
			// TODO: do we have to clean up the semaphore?
		} else {
		
			// write time
			web_data->time = time_real.sec;
			
			// write data to ring buffer
			buffer_add(&web_data->buffer[0], &temp_web_data[0][0]);
			buffer_add(&web_data->buffer[1], &temp_web_data10[0][0]);
			buffer_add(&web_data->buffer[2], &temp_web_data100[0][0]);
			
			// release shared memory
			set_sem(sem_id, DATA_SEM, 1);
		
		}
		
		// notify web clients
		// Note: There is a possible race condition here, which might result
		//   in one web client not getting notified once, do we care?
		int num_web_clients = semctl(sem_id, WAIT_SEM, GETNCNT);
		set_sem(sem_id, WAIT_SEM, num_web_clients);
	}
	
	return SUCCESS;
}

// FILE STORING

/*int store_buffer(FILE* data, int fifo_fd, int control_fifo, void* buffer_addr, unsigned int sample_size, int samples_buffer, struct rl_conf* conf) {
	
	int i = 0;
	int j = 0;
	int k = 2;
	int MASK = 1;
	
	// TODO: temporary solution //
	int binary;
	if(conf->file_format == BIN) {
		binary = 1;
	} else {
		binary = 0;
	}
	
	int channels = 0;
	for(j=0; j<NUM_CHANNELS; j++) {
		if(conf->channels[j] > 0) {
			channels = channels | MASK;
		}
		MASK = MASK << 1;
	}
	
	// ------------------------- //
	
	j = 0;
	MASK = 1;
	
	//int num_channels = count_channels(conf->channels);
	int num_channels = count_bits(channels);
	
	// create timestamp
	time_t nowtime = create_timestamp(conf);
	struct tm* nowtm = localtime(&nowtime);
	
	// binary data (for status and samples)
	int line_int[NUM_CHANNELS + 2] = {0,0,0,0,0,0,0,0,0,0};
	int value_int = 0;
	
	// csv data TODO: defines
	char line_char[200] = "";
	char value_char[50];
	
	// webserver data
	int avg_buffer_size = ceil_div(samples_buffer, WEB_BUFFER_SIZE); // size of buffer to average
	float web_data[WEB_BUFFER_SIZE][NUM_WEB_CHANNELS];
	memset(web_data, 0, WEB_BUFFER_SIZE * NUM_WEB_CHANNELS * sizeof(float)); // reset data (needed due to unfull buffer sizes)
	
	// store buffer
    for(i=0; i<samples_buffer; i++){
		
		// reset values
		strcpy(line_char,"\0");
		k = 2;
		MASK = 1;
		
		// store timestamp
		if (i == 0 && conf->file_format != NO_FILE) {
			if (binary == 1) {
				//fwrite(nowtm, 4, 9, data);
			} else {
				sprintf (value_char, "%s", ctime (&nowtime));
				value_char[strlen(value_char)-1] = '\0'; // remove \n
				strcat(line_char,value_char);
			}
		}
		
		// read status -> TODO: put in struct: DigInps, Range, Samples
		line_int[0] = (int) (*((int8_t *) (buffer_addr)));
		line_int[1] = (int) (*((int8_t *) (buffer_addr + 1)));
		buffer_addr += STATUS_SIZE;
		
		// TODO: fix for dig inputs
		//line_int[0] = ((line_int[0] & I1L_VALID_BIT) > 0);
		//line_int[1] = ((line_int[1] & I2L_VALID_BIT) > 0);
		
		// read and scale values (if channel selected)
		for(j=0; j<NUM_CHANNELS; j++) {
			if(conf->channels[j] > 0) {
				if(sample_size == 4) { // TODO: combine (with sample_size)
					value_int = *( (int32_t *) (buffer_addr + 4*j) );
					line_int[k] = (int) (( value_int + offsets[j] ) * scales[j]);
				} else {
					value_int = *( (int16_t *) (buffer_addr + 2*j) );
					line_int[k] = (int) (( value_int + offsets[j] ) * scales[j]);
				}
				k++;
			}
			MASK = MASK << 1;
		}
		buffer_addr+=NUM_CHANNELS*sample_size;
		
		// store values to file
		if (conf->file_format != NO_FILE) {
			if (binary == 1) {
				// binary
				//fwrite(line_int,4,num_channels+2,data);
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
		if (conf->enable_web_server == 1) {
			if (i % avg_buffer_size == 0) { // only store first sample -> TODO: average over entire buffer
				collapse_data(web_data[i/avg_buffer_size], line_int, channels, conf); //collapse data to webserver buffer
			}
		}
    }
	
	// store values for webserver
	if (conf->enable_web_server == 1) {
		store_web_data(fifo_fd, control_fifo, &web_data[0][0]);
	}
	
	return SUCCESS;
}*/