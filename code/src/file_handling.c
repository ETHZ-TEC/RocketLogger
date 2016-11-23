#include "file_handling.h"

const char* channel_names[NUM_CHANNELS] = {"I1H","I1L","V1","V2","I2H","I2L","V3","V4"};
const char* digital_input_names[NUM_DIGITAL_INPUTS] = {"DigIn1", "DigIn2", "DigIn3", "DigIn4", "DigIn5", "DigIn6"};
const char* valid_info_names[NUM_I_CHANNELS] = {"I1L_valid", "I2L_valid"};

/// Global variable to determine i1l valid channel
int i1l_valid_channel = 0;
/// Global variable to determine i2l valid channel
int i2l_valid_channel = 0;


void create_time_stamp(struct time_stamp* time_real, struct time_stamp* time_monotonic) {
	
	struct timespec spec_real;
	struct timespec spec_monotonic;
	
	// get time stamp of real-time and monotonic clock
	int ret1 = clock_gettime(CLOCK_REALTIME, &spec_real);
	int ret2 = clock_gettime(CLOCK_MONOTONIC_RAW, &spec_monotonic);
	
	if( ret1 < 0 || ret2 < 0 ) {
		rl_log(ERROR, "failed to get time");
	}
	
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
	uint16_t channel_count = count_channels(conf->channels);
	// number binary channels
	uint16_t channel_bin_count = 0;
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
	uint32_t comment_length = strlen(RL_FILE_COMMENT) * sizeof(int8_t);
	// timestamps
	struct time_stamp time_real;
	struct time_stamp time_monotonic;
	create_time_stamp(&time_real, &time_monotonic);
	
	
	// lead_in setup
	lead_in->magic = RL_FILE_MAGIC;
	lead_in->file_version = RL_FILE_VERSION;
	lead_in->header_length = sizeof(struct rl_file_lead_in) + comment_length + (channel_count + channel_bin_count) * sizeof(struct rl_file_channel);
	lead_in->data_block_size = conf->sample_rate / conf->update_rate;
	lead_in->data_block_count = 0; // needs to be updated
	lead_in->sample_count = 0; // needs to be updated
	lead_in->sample_rate = conf->sample_rate;
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


void setup_header(struct rl_file_header* file_header, struct rl_conf* conf) {
	
	// comment
	char* comment = RL_FILE_COMMENT;
	file_header->comment = comment;
	
	// channels
	setup_channels(file_header, conf);
	
}

void store_header(FILE* data, struct rl_file_header* file_header) {
	
	int total_channel_count = file_header->lead_in.channel_bin_count + file_header->lead_in.channel_count;
	
	// write lead in
	fwrite(&(file_header->lead_in), sizeof(struct rl_file_lead_in), 1, data);
	// write comment
	fwrite(file_header->comment, file_header->lead_in.comment_length, 1, data);
	// write channel information
	fwrite(file_header->channel, sizeof(struct rl_file_channel), total_channel_count, data);
	
}

void store_header_csv(FILE* data, struct rl_file_header* file_header) {
	// lead in
	fprintf(data, "RocketLogger CSV File\n");
	fprintf(data, "File Version,%u\n", (uint32_t) file_header->lead_in.file_version);
	fprintf(data, "Block Size,%u\n", (uint32_t) file_header->lead_in.data_block_size);
	fprintf(data, "Block Count,%-20u\n", (uint32_t) file_header->lead_in.data_block_count);
	fprintf(data, "Sample Count,%-20llu\n", (uint64_t) file_header->lead_in.sample_count);
	fprintf(data, "Sample Rate,%u\n", (uint32_t) file_header->lead_in.sample_rate);
	fprintf(data, "MAC Address,%02x", (uint32_t) file_header->lead_in.mac_address[0]);
	int i;
	for(i=1; i<MAC_ADDRESS_LENGTH; i++) {
		fprintf(data, ":%02x", (uint32_t) file_header->lead_in.mac_address[i]);
	}
	fprintf(data, "\n");
	time_t time = (time_t) file_header->lead_in.start_time.sec;
	fprintf(data, "Start Time,%s", ctime(&time));
	fprintf(data, "Comment,%s\n", file_header->comment);
	fprintf(data, "\n");
	
	// channels
	for(i=0; i<(file_header->lead_in.channel_count + file_header->lead_in.channel_bin_count); i++) {
		fprintf(data, ",%s", file_header->channel[i].name);
		switch(file_header->channel[i].channel_scale) {
			case RL_SCALE_MILLI:
				fprintf(data, " [m");
				break;
			case RL_SCALE_MICRO:
				fprintf(data, " [u");
				break;
			case RL_SCALE_NANO:
				fprintf(data, " [n");
				break;
			case RL_SCALE_TEN_PICO:
				fprintf(data, " [10p");
				break;
			default:
				break;
		}
		switch(file_header->channel[i].unit) {
			case RL_UNIT_VOLT:
				fprintf(data, "V]");
				break;
			case RL_UNIT_AMPERE:
				fprintf(data, "A]");
				break;
			default:
				break;
		}
	}
	fprintf(data, "\n");
}

void update_header(FILE* data, struct rl_file_header* file_header) {
	
	// seek to beginning and rewrite lead_in
	rewind(data);
	fwrite(&(file_header->lead_in), sizeof(struct rl_file_lead_in), 1, data);
	fseek(data, 0, SEEK_END);
}

void update_header_csv(FILE* data, struct rl_file_header* file_header) {
	rewind(data);
	fprintf(data, "RocketLogger CSV File\n");
	fprintf(data, "File Version,%u\n", (uint32_t) file_header->lead_in.file_version);
	fprintf(data, "Block Size,%u\n", (uint32_t) file_header->lead_in.data_block_size);
	fprintf(data, "Block Count,%-20u\n", (uint32_t) file_header->lead_in.data_block_count);
	fprintf(data, "Sample Count,%-20llu\n", (uint64_t) file_header->lead_in.sample_count);
	fseek(data, 0, SEEK_END);
}


void merge_currents(uint8_t* valid, int64_t* dest, int64_t* src, struct rl_conf* conf) {
	
	int ch_in = 0;
	int ch_out = 0;
	
		
	if(conf->channels[I1H_INDEX] > 0 && conf->channels[I1L_INDEX] > 0) {
		if(valid[0] == 1) {
			dest[ch_out++] = src[++ch_in];
		} else {
			dest[ch_out++] = src[ch_in++]*H_L_SCALE;
		}
		ch_in++;
	} else if(conf->channels[I1H_INDEX] > 0) {
		dest[ch_out++] = src[ch_in++]*H_L_SCALE;
	} else if(conf->channels[I1L_INDEX] > 0){
		dest[ch_out++] = src[ch_in++];
	}
	if(conf->channels[V1_INDEX] > 0) {
		dest[ch_out++] = src[ch_in++];
	}
	if(conf->channels[V2_INDEX] > 0) {
		dest[ch_out++] = src[ch_in++];
	}
	
	if(conf->channels[I2H_INDEX] > 0 && conf->channels[I2L_INDEX] > 0) {
		if(valid[1] == 1) {
			dest[ch_out++] = src[++ch_in];
		} else {
			dest[ch_out++] = src[ch_in++]*H_L_SCALE;
		}
		ch_in++;
	} else if(conf->channels[I2H_INDEX] > 0) {
		dest[ch_out++] = src[ch_in++]*H_L_SCALE;
	} else if(conf->channels[I2L_INDEX] > 0){
		dest[ch_out++] = src[ch_in++];
	}
	
	if(conf->channels[V3_INDEX] > 0) {
		dest[ch_out++] = src[ch_in++];
	}
	if(conf->channels[V4_INDEX] > 0) {
		dest[ch_out++] = src[ch_in++];
	}
}

int test = 0;
int store_buffer(FILE* data, void* buffer_addr, uint32_t sample_size, uint32_t samples_buffer, struct rl_conf* conf, int sem_id, struct web_shm* web_data_ptr) {
	
	// INIT //
	
	uint32_t i;
	uint32_t j;
	uint32_t k;
	
	// bin channels
	uint32_t num_bin_channels;
	if(conf->digital_inputs == DIGITAL_INPUTS_ENABLED) {
		num_bin_channels = NUM_DIGITAL_INPUTS;
	} else {
		num_bin_channels = 0;
	}
	
	// channels
	uint32_t num_channels = count_channels(conf->channels);
	
	// binary data (for samples)
	uint32_t bin_data;
	int32_t channel_data[num_channels];
	int32_t value = 0;
	
	// csv data
	char channel_data_char[CSV_LINE_LENGTH] = "";
	char value_char[CSV_VALUE_LENGTH];
	
	
	// TIMESTAMP //
	
	// create timestamp
	struct time_stamp time_real;
	struct time_stamp time_monotonic;
	create_time_stamp(&time_real, &time_monotonic);
	
	// adjust time with buffer latency
	time_real.sec -= 1 / conf->update_rate;
	time_monotonic.sec -= 1 / conf->update_rate;
	
	// store timestamp
	if (conf->file_format == BIN) {
		fwrite(&time_real, sizeof(struct time_stamp), 1, data);
		fwrite(&time_monotonic, sizeof(struct time_stamp), 1, data);
			
	} else if(conf->file_format == CSV) {
		time_t t = (time_t) time_real.sec;
		strcpy(value_char, ctime(&t));
		value_char[strlen(value_char)-1] = '\0'; // remove \n
		fprintf(data, "%s", value_char);
	}
	
	
	// AVERAGE DATA //
	
	// averaged data (for web and low rates)
	uint32_t avg_length[WEB_RING_BUFFER_COUNT] = {samples_buffer / BUFFER1_SIZE, samples_buffer / BUFFER10_SIZE, samples_buffer / BUFFER100_SIZE};
	int64_t avg_data[WEB_RING_BUFFER_COUNT][num_channels];
	uint32_t bin_avg_data[WEB_RING_BUFFER_COUNT][num_bin_channels];
	uint8_t avg_valid[WEB_RING_BUFFER_COUNT][NUM_I_CHANNELS] = {{1,1},{1,1},{1,1}};
	
	memset(avg_data, 0, sizeof(int64_t) * num_channels * WEB_RING_BUFFER_COUNT);
	memset(bin_avg_data, 0, sizeof(uint32_t) * num_bin_channels * WEB_RING_BUFFER_COUNT);
	
	
	// WEB DATA //
	
	// data for webserver
	uint32_t num_web_channels = 0;
	if(conf->enable_web_server == 1) {
		num_web_channels = web_data_ptr->num_channels;
	}
	int64_t web_data[WEB_RING_BUFFER_COUNT][BUFFER1_SIZE][num_web_channels];

	
	// HANDLE BUFFER //
	
    for(i=0; i<samples_buffer; i++){
		
		// READ //
		
		// reset values
		k = 0;
		bin_data = 0;
		strcpy(channel_data_char,"\0");
		
		
		// read binary channels
		uint8_t bin_adc1 = (*((int8_t *) (buffer_addr)));
		uint8_t bin_adc2 = (*((int8_t *) (buffer_addr + 1)));
		
		buffer_addr += PRU_DIG_SIZE;
		
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
				channel_data[k] = (int32_t) (( value + offsets[j] ) * scales[j]);
				avg_data[BUF1_INDEX][k] += channel_data[k];
				k++;
			}
		}
		buffer_addr+=NUM_CHANNELS*sample_size;
		
		
		// BINARY CHANNELS //
		
		// mask and combine digital inputs, if requestet
		uint32_t bin_channel_pos;
		if(conf->digital_inputs == DIGITAL_INPUTS_ENABLED) {
			bin_data = ((bin_adc1 & BINARY_MASK) >> 1) | ((bin_adc2 & BINARY_MASK) << 2);
			bin_channel_pos = NUM_DIGITAL_INPUTS;
			
			// average digital inputs
			int32_t MASK = 1;
			for(j=0; j<num_bin_channels; j++) {
				if((bin_data & MASK) > 0) {
					bin_avg_data[BUF1_INDEX][j] += 1;
				}
				MASK = MASK << 1;
			}
			
		} else {
			bin_channel_pos = 0;
		}
		
		// mask and combine valid info
		uint8_t valid1 = (~bin_adc1) & VALID_MASK;
		uint8_t valid2 = (~bin_adc2) & VALID_MASK;
		
		if(conf->channels[I1L_INDEX] > 0) {
			bin_data = bin_data | (valid1 << bin_channel_pos);
			bin_channel_pos++;
		}
		if(conf->channels[I2L_INDEX] > 0) {
			bin_data = bin_data | (valid2 << bin_channel_pos);
			bin_channel_pos++;
		}
		
		// average valid info
		for(j=0; j<=BUF100_INDEX; j++) {
			avg_valid[j][0] = avg_valid[j][0] & valid1;
			avg_valid[j][1] = avg_valid[j][1] & valid2;
		}
		
		
		// HANDLE AVERAGE DATA //
		
		if(conf->enable_web_server == 1 || conf->sample_rate < MIN_ADC_RATE) {
			
			// buffer 1
			if((i+1)%avg_length[BUF1_INDEX] == 0) {
				
				// average
				for(j=0; j<num_channels; j++) {
					avg_data[BUF1_INDEX][j] /= avg_length[BUF1_INDEX];
					avg_data[BUF10_INDEX][j] += avg_data[BUF1_INDEX][j];
				}
				
				// merge_currents (for web)
				if(conf->enable_web_server == 1) {
					merge_currents(avg_valid[BUF1_INDEX], &web_data[BUF1_INDEX][i/avg_length[BUF1_INDEX]][num_bin_channels], avg_data[BUF1_INDEX], conf);
				}
				// average bin channels
				for(j=0; j<num_bin_channels; j++) {
					
					bin_avg_data[BUF10_INDEX][j] += bin_avg_data[BUF1_INDEX][j];
					
					// store bin channels for web
					if(conf->enable_web_server == 1) {
						if(bin_avg_data[BUF1_INDEX][j] >= (avg_length[BUF1_INDEX]/2)) {
							web_data[BUF1_INDEX][i/avg_length[BUF1_INDEX]][j] = 1;
						} else {
							web_data[BUF1_INDEX][i/avg_length[BUF1_INDEX]][j] = 0;
						}
					}
				}
				
				// write data to file
				if(conf->sample_rate == 100) {
					// binary data
					if (bin_channel_pos > 0) {
						if (conf->file_format == BIN) {
							uint32_t temp_bin = 0;
							for(j=0; j<num_bin_channels; j++) {
								temp_bin = temp_bin | ((bin_avg_data[BUF1_INDEX][j] >= (avg_length[BUF1_INDEX]/2)) << j);
							}
							if(conf->channels[I1L_INDEX] > 0) {
								temp_bin = temp_bin | (avg_valid[BUF1_INDEX][0] << j++);
							}
							if(conf->channels[I2L_INDEX] > 0) {
								temp_bin = temp_bin | (avg_valid[BUF1_INDEX][1] << j++);
							}
							
							fwrite(&temp_bin, sizeof(uint32_t), 1, data);
							
						} else if (conf->file_format == CSV) {
							for(j=0; j<num_bin_channels; j++) {
								sprintf(value_char, ", %d", (bin_avg_data[BUF1_INDEX][j] >= (avg_length[BUF1_INDEX]/2)));
								strcat(channel_data_char,value_char);
							}
							if(conf->channels[I1L_INDEX] > 0) {
								sprintf(value_char, ", %d", avg_valid[BUF1_INDEX][0]);
								strcat(channel_data_char,value_char);
							}
							if(conf->channels[I2L_INDEX] > 0) {
								sprintf(value_char, ", %d", avg_valid[BUF1_INDEX][1]);
								strcat(channel_data_char,value_char);
							}
						}
					}
					
					// channel data
					if (conf->file_format == BIN) {
						for(j=0; j<num_channels; j++) {
							int32_t tmp = (int32_t) avg_data[BUF1_INDEX][j];
							fwrite(&tmp, sizeof(int32_t), 1, data);
						}
					} else if (conf->file_format == CSV) {
						for(j=0; j < num_channels; j++) {
							sprintf(value_char,",%d",(int32_t) avg_data[BUF1_INDEX][j]);
							strcat(channel_data_char, value_char);
						}
						strcat(channel_data_char,"\n");
						fprintf(data, "%s", channel_data_char);
					}
				}
				
				// reset values
				memset(avg_data[BUF1_INDEX], 0, sizeof(int64_t) * num_channels);
				memset(bin_avg_data[BUF1_INDEX], 0, sizeof(uint32_t) * num_bin_channels);
				avg_valid[BUF1_INDEX][0] = 1;
				avg_valid[BUF1_INDEX][1] = 1;
			}
			
			
			// buffer 10
			if((i+1)%avg_length[BUF10_INDEX] == 0) {
				
				// average
				for(j=0; j<num_channels; j++) {
					avg_data[BUF10_INDEX][j] /= (avg_length[BUF10_INDEX]/avg_length[BUF1_INDEX]);
					avg_data[BUF100_INDEX][j] += avg_data[BUF10_INDEX][j];
				}
				
				// merge_currents (for web)
				if(conf->enable_web_server == 1) {
					merge_currents(avg_valid[BUF10_INDEX], &web_data[BUF10_INDEX][i/avg_length[BUF10_INDEX]][num_bin_channels], avg_data[BUF10_INDEX], conf);
				}
				
				// average bin channels
				for(j=0; j<num_bin_channels; j++) {
					
					bin_avg_data[BUF100_INDEX][j] += bin_avg_data[BUF10_INDEX][j];
					
					// store bin channels for web
					if(conf->enable_web_server == 1) {
						if(bin_avg_data[BUF10_INDEX][j] >= (avg_length[BUF10_INDEX]/2)) {
							web_data[BUF10_INDEX][i/avg_length[BUF10_INDEX]][j] = 1;
						} else {
							web_data[BUF10_INDEX][i/avg_length[BUF10_INDEX]][j] = 0;
						}
					}
				}
				
				// write data to file
				if(conf->sample_rate == 10) {
					// binary data
					if (bin_channel_pos > 0) {
						if (conf->file_format == BIN) {
							uint32_t temp_bin = 0;
							for(j=0; j<num_bin_channels; j++) {
								temp_bin = temp_bin | ((bin_avg_data[BUF10_INDEX][j] >= (avg_length[BUF10_INDEX]/2)) << j);
							}
							if(conf->channels[I1L_INDEX] > 0) {
								temp_bin = temp_bin | (avg_valid[BUF10_INDEX][0] << j++);
							}
							if(conf->channels[I2L_INDEX] > 0) {
								temp_bin = temp_bin | (avg_valid[BUF10_INDEX][1] << j++);
							}
							
							fwrite(&temp_bin, sizeof(uint32_t), 1, data);
							
						} else if (conf->file_format == CSV) {
							for(j=0; j<num_bin_channels; j++) {
								sprintf(value_char, ", %d", (bin_avg_data[BUF10_INDEX][j] >= (avg_length[BUF10_INDEX]/2)));
								strcat(channel_data_char,value_char);
							}
							if(conf->channels[I1L_INDEX] > 0) {
								sprintf(value_char, ", %d", avg_valid[BUF10_INDEX][0]);
								strcat(channel_data_char,value_char);
							}
							if(conf->channels[I2L_INDEX] > 0) {
								sprintf(value_char, ", %d", avg_valid[BUF10_INDEX][1]);
								strcat(channel_data_char,value_char);
							}
						}
					}
					
					// channel data
					if (conf->file_format == BIN) {
						for(j=0; j<num_channels; j++) {
							int32_t tmp = (int32_t) avg_data[BUF10_INDEX][j];
							fwrite(&tmp, sizeof(int32_t), 1, data);
						}
					} else if (conf->file_format == CSV) {
						for(j=0; j < num_channels; j++) {
							sprintf(value_char,",%d",(int32_t) avg_data[BUF10_INDEX][j]);
							strcat(channel_data_char, value_char);
						}
						strcat(channel_data_char,"\n");
						fprintf(data, "%s", channel_data_char);
					}
				}
				
				// reset values
				memset(avg_data[BUF10_INDEX], 0, sizeof(int64_t) * num_channels);
				memset(bin_avg_data[BUF10_INDEX], 0, sizeof(uint32_t) * num_bin_channels);
				avg_valid[BUF10_INDEX][0] = 1;
				avg_valid[BUF10_INDEX][1] = 1;
			}
			
			
			// buffer 100
			if((i+1)%avg_length[BUF100_INDEX] == 0) {
				
				// average
				for(j=0; j<num_channels; j++) {
					avg_data[BUF100_INDEX][j] /= (avg_length[BUF100_INDEX]/avg_length[BUF10_INDEX]);
				}
				
				// merge_currents (for web)
				if(conf->enable_web_server == 1) {
					merge_currents(avg_valid[BUF100_INDEX], &web_data[BUF100_INDEX][i/avg_length[BUF100_INDEX]][num_bin_channels], avg_data[BUF100_INDEX], conf);
				}
				
				// store bin channels for web
				if(conf->enable_web_server == 1) {
					for(j=0; j<num_bin_channels; j++) {
						
						if(bin_avg_data[BUF100_INDEX][j] >= (avg_length[BUF100_INDEX]/2)) {
							web_data[BUF100_INDEX][i/avg_length[BUF100_INDEX]][j] = 1;
						} else {
							web_data[BUF100_INDEX][i/avg_length[BUF100_INDEX]][j] = 0;
						}
					}
				}
				
				// write data to file
				if(conf->sample_rate == 1) {
					// binary data
					if (bin_channel_pos > 0) {
						if (conf->file_format == BIN) {
							uint32_t temp_bin = 0;
							for(j=0; j<num_bin_channels; j++) {
								temp_bin = temp_bin | ((bin_avg_data[BUF100_INDEX][j] >= (avg_length[BUF100_INDEX]/2)) << j);
							}
							if(conf->channels[I1L_INDEX] > 0) {
								temp_bin = temp_bin | (avg_valid[BUF100_INDEX][0] << j++);
							}
							if(conf->channels[I2L_INDEX] > 0) {
								temp_bin = temp_bin | (avg_valid[BUF100_INDEX][1] << j++);
							}
							
							fwrite(&temp_bin, sizeof(uint32_t), 1, data);
							
						} else if (conf->file_format == CSV) {
							for(j=0; j<num_bin_channels; j++) {
								sprintf(value_char, ", %d", (bin_avg_data[BUF100_INDEX][j] >= (avg_length[BUF100_INDEX]/2)));
								strcat(channel_data_char,value_char);
							}
							if(conf->channels[I1L_INDEX] > 0) {
								sprintf(value_char, ", %d", avg_valid[BUF100_INDEX][0]);
								strcat(channel_data_char,value_char);
							}
							if(conf->channels[I2L_INDEX] > 0) {
								sprintf(value_char, ", %d", avg_valid[BUF100_INDEX][1]);
								strcat(channel_data_char,value_char);
							}
						}
					}
					
					// channel data
					if (conf->file_format == BIN) {
						for(j=0; j<num_channels; j++) {
							int32_t tmp = (int32_t) avg_data[BUF100_INDEX][j];
							fwrite(&tmp, sizeof(int32_t), 1, data);
						}
					} else if (conf->file_format == CSV) {
						for(j=0; j < num_channels; j++) {
							sprintf(value_char,",%d",(int32_t) avg_data[BUF100_INDEX][j]);
							strcat(channel_data_char, value_char);
						}
						strcat(channel_data_char,"\n");
						fprintf(data, "%s", channel_data_char);
					}
				}
			}
		}
		
		// WRITE FILE IF HIGH RATE //
		
		// write binary channels
		if(conf->sample_rate >= MIN_ADC_RATE) {
			if (bin_channel_pos > 0) {
				if (conf->file_format == BIN) {
					fwrite(&bin_data, sizeof(uint32_t), 1, data);
					
				} else if (conf->file_format == CSV) {
					int32_t MASK = 1;
					for(j=0; j<bin_channel_pos; j++) {
						sprintf(value_char, ", %d", (bin_data & MASK) > 1);
						strcat(channel_data_char,value_char);
						MASK = MASK << 1;
					}
				}
			}
			
			// store values to file
			if (conf->file_format == BIN) {
				fwrite(channel_data, sizeof(int32_t), num_channels, data);
				
			} else if (conf->file_format == CSV) {
				for(j=0; j < num_channels; j++) {
					sprintf(value_char,",%d",channel_data[j]);
					strcat(channel_data_char, value_char);
				}
				strcat(channel_data_char,"\n");
				fprintf(data, "%s", channel_data_char);
			}
		}
		
    }
	
	
	// WRITE WEB DATA //
	if (conf->enable_web_server == 1) {
		
		// get shared memory access
		if(wait_sem(sem_id, DATA_SEM, SEM_WRITE_TIME_OUT) == TIME_OUT) {
			// disable webserver and continue running
			conf->enable_web_server = 0;
			status.state = RL_RUNNING;
			rl_log(WARNING, "semaphore failure. Webserver disabled");
		} else {
		
			// write time
			web_data_ptr->time = time_real.sec*1000 + time_real.nsec/1000000;
			
			// write data to ring buffer
			buffer_add(&web_data_ptr->buffer[BUF1_INDEX], &web_data[BUF1_INDEX][0][0]);
			buffer_add(&web_data_ptr->buffer[BUF10_INDEX], &web_data[BUF10_INDEX][0][0]);
			buffer_add(&web_data_ptr->buffer[BUF100_INDEX], &web_data[BUF100_INDEX][0][0]);
			
			// release shared memory
			set_sem(sem_id, DATA_SEM, 1);
		
		}
	}
	
	return SUCCESS;
}
