#include "rl_util.h"

#define NUMBER_SAMPLE_RATES 7
int possible_sample_rates[NUMBER_SAMPLE_RATES] = {1,2,4,8,16,32,64};

#define NUMBER_UPDATE_RATES 4
int possible_update_rates[NUMBER_UPDATE_RATES] = {1,2,5,10};

int check_sample_rate(int sample_rate) {
	int i;
	for(i=0; i<NUMBER_SAMPLE_RATES; i++) {
		if(possible_sample_rates[i] == sample_rate){
			return SUCCESS;
		}
	}
	return FAILURE;
}

int check_update_rate(int update_rate) {
	int i;
	for(i=0; i<NUMBER_UPDATE_RATES; i++) {
		if(possible_update_rates[i] == update_rate){
			return SUCCESS;
		}
	}
	return FAILURE;
}

pid_t get_pid() {
	
	// open file
	pid_t pid;
	FILE* file = fopen(PID_FILE, "r");
	if(file == NULL) { // no pid found -> no process running
		return FAILURE;
	}
	
	// read pid
	fread(&pid, sizeof(pid_t), 1, file); // get PID of background process
	
	//close file
	fclose(file);
	
	return pid;
}

int set_pid(pid_t pid) {
	
	// open file
	FILE* file = fopen(PID_FILE, "w");
	if(file == NULL) {
		rl_log(ERROR, "failed to create pid file");
		return FAILURE;
	}
	
	// write pid
	fwrite(&pid, sizeof(pid_t), 1, file);
	
	//close file
	fclose(file);
	
	return SUCCESS;
}