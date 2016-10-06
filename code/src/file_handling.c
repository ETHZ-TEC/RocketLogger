#include "file_handling.h"

char channel_names[10][15] = {"I1H [nA]","I1M [nA]","I1L [10pA]","V1 [uV]","V2 [uV]","I2H [nA]","I2M [nA]","I2L [10pA]","V3 [uV]","V4 [uV]"};


time_t create_timestamp(struct rl_conf* conf) {
	struct timeval time;
	gettimeofday(&time, NULL);
	time.tv_sec -= 1 / conf->update_rate; // adjust with buffer latency
	return time.tv_sec;
}



// HEADER

void setup_header(struct header* file_header, struct rl_conf* conf, struct pru_data_struct* pru_data, int pru_sample_rate) {
	
	int j;
	int MASK = 1;
	int channels = 0;
	for(j=0; j<NUM_CHANNELS; j++) {
		if(conf->channels[j] > 0) {
			channels = channels | MASK;
		}
		MASK = MASK << 1;
	}
		
	// get header length
	if(conf->file_format == BIN) {
		file_header->header_length = HEADERLENGTH;
	} else if (conf->file_format == CSV) {
		file_header->header_length = HEADERLENGTH + 1;
	} else {
		rl_log(ERROR, "failed to update header, wrong file format");
	}
	
	file_header->number_samples = 0; // number of samples taken
	file_header->buffer_size = pru_data->buffer_size;
	file_header->rate = pru_sample_rate;
	file_header->channels = channels;
	file_header->precision = pru_data->precision;
	
}

// store old header // TODO: for new header
int store_header(FILE* data, struct header* file_header, struct rl_conf* conf) {
	
	if (conf->file_format == BIN) {
		fwrite(file_header, sizeof(struct header), 1, data);
		
	} else if (conf->file_format == CSV){
		// information
		fprintf(data, "Header Length:,%d\n", file_header->header_length);
		fprintf(data, "Number Samples:,%-12d\n", file_header->number_samples);
		fprintf(data, "Buffer Size:,%d\n", file_header->buffer_size);
		fprintf(data, "Rate:,%d\n", file_header->rate);
		fprintf(data, "Channels:,%d\n", file_header->channels);
		fprintf(data, "Precision:,%d\n", file_header->precision);
		
		// title row
		fprintf(data,"Time");
		fprintf(data,",LOW1");
		fprintf(data,",LOW2");
		
		// channel names
		int i;
		for(i=0; i<NUM_CHANNELS; i++) {
			if(conf->channels[i] > 0) {
				fprintf(data,",%s",channel_names[i]);
			}
		}
		fprintf(data,"\n");
	} else {
		rl_log(ERROR, "failed to store header, wrong file format");
	}
	
	return SUCCESS;
}

// update sample number of old header // TODO: for new header
int update_sample_number(FILE* data, struct header* file_header, struct rl_conf* conf) {
	
	// store file pointer position
	long pos = ftell(data);
	rewind(data);
	
	// only store header until number samples updated
	if (conf->file_format == BIN) {
		fwrite(file_header, 2, sizeof(int), data);
		
	} else if (conf->file_format == CSV){
		fprintf(data, "Header Length:,%d\n", file_header->header_length);
		fprintf(data, "Number Samples:,%-12d\n", file_header->number_samples);
		
	} else {
		rl_log(ERROR, "failed to update header, wrong file format");
	}
	
	fseek(data, pos, SEEK_SET);
	
	
	return SUCCESS;
}

// setup new header
void setup_header_new(struct file_header_new* header, struct rl_conf* conf, struct pru_data_struct* pru_data) {
	header->header_version = HEADER_VERSION;
	header->number_samples = 0;
	header->buffer_size = pru_data->buffer_size;
	header->sample_rate = conf->sample_rate;
	header->precision = pru_data->precision;
	int i;
	for(i=0; i<NUM_CHANNELS; i++) {
		header->channels[i] = conf->channels[i];
	}
}




// FILE STORING

int store_buffer(FILE* data, int fifo_fd, int control_fifo, void* buffer_addr, unsigned int sample_size, int samples_buffer, struct rl_conf* conf) {
	
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
	int line_int[NUM_CHANNELS + 2] = {0,0,0,0,0,0,0,0,0,0,0,0};
	int value_int = 0;
	
	// csv data TODO: defines
	char line_char[200] = "";
	char value_char[20];
	
	// webserver data
	int avg_buffer_size = ceil_div(samples_buffer, WEB_BUFFER_SIZE); // size of buffer to average
	float web_data[WEB_BUFFER_SIZE][NUMBER_WEB_CHANNELS];
	memset(web_data, 0, WEB_BUFFER_SIZE * NUMBER_WEB_CHANNELS * sizeof(float)); // reset data (needed due to unfull buffer sizes)
	
	// store buffer
    for(i=0; i<samples_buffer; i++){
		
		// reset values
		strcpy(line_char,"\0");
		k = 2;
		MASK = 1;
		
		// store timestamp
		if (i == 0 && conf->file_format != NO_FILE) {
			if (binary == 1) {
				fwrite(nowtm, 4, 9, data);
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
		line_int[0] = ((line_int[0] & I1L_VALID_BIT) > 0);
		line_int[1] = ((line_int[1] & I2L_VALID_BIT) > 0);
		
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
}



// WEBSERVER STORING


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
void collapse_data(float* data_out, int* data_in, int channels, struct rl_conf* conf) {
	
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