#include "rl_util.h"


int read_config(struct rl_conf_new* conf, char file_name[MAX_PATH_LENGTH]) {
	
	// check if config file existing
	if(open(file_name, O_RDWR) <= 0) {
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
	
	//close file
	fclose(file);
	return 1;
}

int write_config(struct rl_conf_new* conf, char file_name[MAX_PATH_LENGTH]) {
	
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