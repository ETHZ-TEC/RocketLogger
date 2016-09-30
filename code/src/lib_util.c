#include "rl_util.h"


int read_config(struct rl_conf_new* conf) {
	
	// check if config file existing
	if(open(CONFIG_FILE, O_RDWR) <= 0) {
		return 0;
	}
	
	// open config file
	FILE* file = fopen(CONFIG_FILE, "r");
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

int write_config(struct rl_conf_new* conf) {
	
	// open config file
	FILE* file = fopen(CONFIG_FILE, "w");
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

pid_t get_pid() {
	
	// open file
	pid_t pid;
	FILE* file = fopen(PID_FILE, "r");
	if(file == NULL) { // no pid found -> no process running
		//printf("Error opening pid file.\n");
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