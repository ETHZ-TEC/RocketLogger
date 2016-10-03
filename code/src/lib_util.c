#include "rl_util.h"

#define NUMBER_SAMPLE_RATES 7
int possible_sample_rates[NUMBER_SAMPLE_RATES] = {1,2,4,8,16,32,64};

#define NUMBER_UPDATE_RATES 4
int possible_update_rates[NUMBER_UPDATE_RATES] = {1,2,5,10};

int check_sample_rate(int sample_rate) {
	int i;
	for(i=0; i<NUMBER_SAMPLE_RATES; i++) {
		if(possible_sample_rates[i] == sample_rate){
			return 1;
		}
	}
	return -1;
}

int check_update_rate(int update_rate) {
	int i;
	for(i=0; i<NUMBER_UPDATE_RATES; i++) {
		if(possible_update_rates[i] == update_rate){
			return 1;
		}
	}
	return -1;
}

int read_config(struct rl_conf* conf) {
	
	// check if config file existing
	if(open(CONFIG_FILE, O_RDWR) <= 0) {
		//conf->mode = IDLE;
		return 0;
	}
	
	// open config file
	FILE* file = fopen(CONFIG_FILE, "r");
	if(file == NULL) {
		printf("Error opening configuration file.\n");
		return -1;
	}
	// read values
	fread(conf, sizeof(struct rl_conf), 1, file);
	
	//close file
	fclose(file);
	return 1;
}

int write_config(struct rl_conf* conf) {
	
	// open config file
	FILE* file = fopen(CONFIG_FILE, "w");
	if(file == NULL) {
		printf("Error creating configuration file.\n");
		return -1;
	}
	// write values
	fwrite(conf, sizeof(struct rl_conf), 1, file);
	
	//close file
	fclose(file);
	return 1;
}

// print data in json format for easy reading in javascript
void print_json(float data[], int length) {
	char str[150]; // TODO: adjustable length
	char val[20];
	int i;
	sprintf(str, "[\"%f\"", data[0]);
	for (i=1; i < length; i++) {
		sprintf(val, ",\"%f\"", data[i]);
		strcat(str, val);
	}
	strcat(str, "]\n");
	printf(str);
}

void print_channels_new(int channels[NUM_CHANNELS]) {
	
	// floats needed for print_json function
	float iChannels[6] = {0,0,0,0,0,0};
	float vChannels[4] = {0,0,0,0};
	
	// currents
	if((channels[0]) > 0 ) {
		iChannels[0] = 1;
	}
	if((channels[1]) > 0) {
		iChannels[1] = 1;
	}
	if((channels[2]) > 0 ) {
		iChannels[2] = 1;
	}
	if((channels[5]) > 0) {
		iChannels[3] = 1;
	}
	if((channels[6]) > 0 ) {
		iChannels[4] = 1;
	}
	if((channels[7]) > 0) {
		iChannels[5] = 1;
	}
	
	// voltages
	if((channels[3]) > 0) {
		vChannels[0] = 1;
	}
	if((channels[4]) > 0) {
		vChannels[1] = 1;
	}
	if((channels[8]) > 0) {
		vChannels[2] = 1;
	}
	if((channels[9]) > 0) {
		vChannels[3] = 1;
	}
	
	// print
	print_json(vChannels, 4);
	print_json(iChannels, 6);
	
}

pid_t get_pid() {
	
	// open file
	pid_t pid;
	FILE* file = fopen(PID_FILE, "r");
	if(file == NULL) { // no pid found -> no process running
		return -1;
	}
	
	// read pid
	fread(&pid, sizeof(pid_t), 1, file); // get PID of background process
	
	//close file
	fclose(file);
	
	return pid;
}

void set_pid(pid_t pid) {
	
	// open file
	FILE* file = fopen(PID_FILE, "w");
	if(file == NULL) {
		printf("Error creating pid file.\n");
		return;
	}
	
	// write pid
	fwrite(&pid, sizeof(pid_t), 1, file);
	
	//close file
	fclose(file);
}