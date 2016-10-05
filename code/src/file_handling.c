#include "file_handling.h"

char channel_names[10][15] = {"I1H [nA]","I1M [nA]","I1L [10pA]","V1 [uV]","V2 [uV]","I2H [nA]","I2M [nA]","I2L [10pA]","V3 [uV]","V4 [uV]"};


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
		int MASK = 1;
		for(i=0; i<NUM_CHANNELS; i++) {
			if((file_header->channels & MASK) > 0) {
				fprintf(data,",%s",channel_names[i]);
			}
			MASK = MASK << 1;
		}
		fprintf(data,"\n");
	} else {
		rl_log(ERROR, "failed to store header, wrong file format");
	}
	
	return SUCCESS;
}