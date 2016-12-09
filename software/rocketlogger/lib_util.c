#include "rl_util.h"

/// Number of possible sampling rates
#define NUMBER_SAMPLE_RATES 10
/// Possible sampling rates
int possible_sample_rates[NUMBER_SAMPLE_RATES] = {1, 10, 100, 1000, 2000, 4000, 8000, 16000, 32000, 64000};

/// Number of possible update rates
#define NUMBER_UPDATE_RATES 4
/// Possible update rates
int possible_update_rates[NUMBER_UPDATE_RATES] = {1,2,5,10};

/**
 * Check if provided sampling rate is possible
 * @param sample_rate Sampling rate
 * @return {@link SUCCESS} if possible, {@link FAILURE} otherwise
 */
int check_sample_rate(int sample_rate) {
	int i;
	for(i=0; i<NUMBER_SAMPLE_RATES; i++) {
		if(possible_sample_rates[i] == sample_rate){
			return SUCCESS;
		}
	}
	return FAILURE;
}

/**
 * Check if provided update rate is possible
 * @param update_rate Update rate
 * @return {@link SUCCESS} if possible, {@link FAILURE} otherwise
 */
int check_update_rate(int update_rate) {
	int i;
	for(i=0; i<NUMBER_UPDATE_RATES; i++) {
		if(possible_update_rates[i] == update_rate){
			return SUCCESS;
		}
	}
	return FAILURE;
}

/**
 * Get process ID (PID) of background sampling process
 * @return PID of background process
 */
pid_t get_pid(void) {
	
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

/**
 * Store process ID (PID)
 * @param pid Own PID
 * @return {@link SUCCESS} in case of success, {@link FAILURE} otherwise.
 */
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
