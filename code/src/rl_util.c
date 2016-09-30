#include "rl_util.h"

void reset_config(struct rl_conf_new* conf) {
	conf->mode = NEW_CONTINUOUS;
	conf->sample_rate = 1;
	conf->update_rate = 1;
	conf->number_samples = 0;
	conf->enable_web_server = 0;
	conf->file_format = CSV; // TODO: change to BIN
	
	strcpy(conf->file_name, "/var/www/data/data.csv");
	
	memset(conf->channels, 1, sizeof(conf->channels));
	memset(conf->force_high_channels, 0, sizeof(conf->force_high_channels));
	
	return;
}

int read_default_config(struct rl_conf_new* conf, char file_name[MAX_PATH_LENGTH]) { // TODO: move to upper layer
	
	// check if config file existing
	if(open(file_name, O_RDWR) <= 0) {
		//printf("Warning: no default configuration file found.\n");
		reset_config(conf);
		return 0;
	}
	
	// open config file
	FILE* file = fopen(file_name, "r");
	if(file == NULL) {
		printf("Error opening configuration file.\n");
		return -1;
	}
	// read values
	fread(conf, sizeof(struct rl_conf_new), 1, file);
	
	// reset mode
	conf->mode = NEW_CONTINUOUS;
	
	//close file
	fclose(file);
	return 1;
}

int write_default_config(struct rl_conf_new* conf, char file_name[MAX_PATH_LENGTH]) {
	
	// open config file
	FILE* file = fopen(file_name, "w");
	if(file == NULL) {
		printf("Error creating configuration file.\n");
		return -1;
	}
	// write values
	fwrite(conf, sizeof(struct rl_conf_new), 1, file);
	
	//close file
	fclose(file);
	return 1;
}