#include "rl_lib.h"

void rl_reset_calibration() {
	
	// set zero offset/scale (raw data)
	remove(CALIBRATION_FILE);
	reset_offsets();
	reset_scales();
	write_calibration();
	
}

// get current data (used for webserver)
int rl_get_data() {
	
	float data[WEB_BUFFER_SIZE][NUMBER_WEB_CHANNELS];
	
	// write ready to control fifo
	int ready = 1;
	int control_fifo = open(CONTROL_FIFO, O_NONBLOCK | O_RDWR);
	write(control_fifo, &ready,  sizeof(int));
	close(control_fifo);
	
	// read data fifo (in blocking mode)
	int fifo_fd = open(FIFO_FILE, O_RDONLY);
	read(fifo_fd, &data[0][0],  sizeof(float) * WEB_BUFFER_SIZE * NUMBER_WEB_CHANNELS);
	close(fifo_fd);
	
	// print buffer
	int i;
	for (i=0; i<WEB_BUFFER_SIZE; i++) {
		print_json(data[i], NUMBER_WEB_CHANNELS);
	}
	
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

// print active channels in json format (for webserver)
void print_channels(int channels) {
	
	// floats needed for print_json function
	float iChannels[6] = {0,0,0,0,0,0};
	float vChannels[4] = {0,0,0,0};
	
	// currents
	if((channels & I1L) > 0 ) {
		iChannels[0] = 1;
	}
	if((channels & I2L) > 0) {
		iChannels[1] = 1;
	}
	if((channels & I1M) > 0 ) {
		iChannels[2] = 1;
	}
	if((channels & I2M) > 0) {
		iChannels[3] = 1;
	}
	if((channels & I1H) > 0 ) {
		iChannels[4] = 1;
	}
	if((channels & I2H) > 0) {
		iChannels[5] = 1;
	}
	
	// voltages
	if((channels & V1) > 0) {
		vChannels[0] = 1;
	}
	if((channels & V2) > 0) {
		vChannels[1] = 1;
	}
	if((channels & V3) > 0) {
		vChannels[2] = 1;
	}
	if((channels & V4) > 0) {
		vChannels[3] = 1;
	}
	
	// print
	print_json(vChannels, 4);
	print_json(iChannels, 6);
	
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

void print_status(struct rl_conf_new* conf, int web) {
	
	if(conf->mode == IDLE) {
		if (web == 1) {
			printf("OFF\n");
		} else {
			printf("RocketLogger IDLE\n");
		}
	} else {
		if (web == 1) {
			printf("RUNNING\n");
			
		} else {
			printf("RocketLogger Status: RUNNING\n");
		}
		print_config(conf, web);
	}
}

void print_config(struct rl_conf_new* conf, int web) {
	
	char file_format_names[3][10] = {"no file", "csv", "binary"};
	
	if (web == 1) {

		printf("%d\n", conf->enable_web_server);
		printf("%d\n", conf->sample_rate);
		printf("%d\n", conf->update_rate);
		printf("%d\n", conf->number_samples);
		print_channels_new(conf->channels);
		printf("%s\n", conf->file_name);
		printf("%d\n",conf->file_format == BIN); // TODO: add no-file field
		
	} else {

											printf("  Sampling rate:   %dkSps\n", conf->sample_rate);
											printf("  Update rate:     %dHz\n", conf->update_rate);
		if (conf->number_samples == 0) {	printf("  Sample limit:    no limit\n");
		} else {							printf("  Sample limit:    %d\n", conf->number_samples);}
		if(conf->enable_web_server == 1)	printf("  Webserver:       enabled\n");
											printf("  File format:     %s\n", file_format_names[conf->file_format]);
		if(conf->file_format != NO_FILE)	printf("  File name:       %s\n", conf->file_name);
											printf("  Channels:        ");
		int i;
		for(i=0; i<NUM_CHANNELS; i++) {
			if (conf->channels[i] > 0) {
				printf("%d,", i);
			}
		}
		printf("\n");
		if (conf->force_high_channels[0] > 0 || conf->force_high_channels[1] > 0) {
											printf("  Forced channels: ");
			for(i=0; i<NUM_I_CHANNELS; i++) {
				if (conf->force_high_channels[i] > 0) {
					printf("%d,", i+1);
				}
			}
			printf("\n");
		}
	}
}

// get status of RL (returns 1 when running)
int rl_get_status(int print, int web) {
	
	// read config
	struct rl_conf_new conf;
	if(read_config(&conf, CONFIG_FILE) == 0) {
		// no config file found -> assume no running
		return 0;
	}
	int status = !(conf.mode == IDLE);
	
	// print config if requested
	if(print == 1) {
		print_status(&conf, web);
	}
	
	return status;
	
	/*
	struct rl_conf conf;
	
	// read conf from shared memory
	key_t key = ftok(DEVICE_FILE "conf", 's');								// create shared memory key  
    int conf_id = shmget(key, sizeof(struct rl_conf), 0666);
	struct rl_conf* conf_ptr = (struct rl_conf*) shmat(conf_id, NULL, 0);	// get pointer to shared memory
	
	if (conf_ptr == (void *) -1) {	// check if shared memory set (shared memory is detached during stop)
		conf.state = OFF;
	} else {
		conf = *conf_ptr;			// read shared memory
	}
	if (print == 1) {				// print status, if requested
									printf("RocketLogger Status:\n");
		if (conf.state == OFF) {	printf("  State:        OFF\n");
		} else {					printf("  State:        RUNNING\n");
									printf("  Webserver:    %d\n",conf.enable_web_server);
									printf("  Rate:         %ukSps\n",conf.rate);
									printf("  Update rate:  %uHz\n",conf.update_rate);
									printf("  Sample limit: %u\n",conf.number_samples);
									printf("  Channels:     %u\n",conf.channels);
									printf("  File:         %s\n",conf.file);
									printf("  Binary file:  %d\n", conf.binary_file);
		}
	
	}
	
	shmdt(conf_ptr); // detach shared memory
	return (conf.state != OFF);*/
}

// get status of RL (for web usage)
/*int rl_get_status_web() {
	
	struct rl_conf conf;
	
	// read conf from shared memory
	key_t key = ftok(DEVICE_FILE "conf", 's');								// create shared memory key  
    int conf_id = shmget(key, sizeof(struct rl_conf), 0666);
	struct rl_conf* conf_ptr = (struct rl_conf*) shmat(conf_id, NULL, 0);	// get pointer to shared memory
	
	if (conf_ptr == (void *) -1) {	// check if shared memory set (shared memory is detached during stop)
		conf.state = OFF;
	} else {
		conf = *conf_ptr;			// read shared memory
	}
	if (conf.state == OFF) {	printf("OFF\n");
	} else {					printf("RUNNING\n");
								printf("%d\n",conf.enable_web_server);
								printf("%u\n",conf.rate);
								printf("%u\n",conf.update_rate);
								printf("%u\n",conf.number_samples);
								print_channels(conf.channels);
								printf("%s\n",conf.file);
								printf("%d\n", conf.binary_file);
	}
	
	shmdt(conf_ptr); // detach shared memory
	return 1;
}*/


// main sample function
int rl_sample(struct rl_conf_new* confn) {
	
	
	// create FIFOs if not existing (TODO: in HW??)
	if(open(FIFO_FILE, O_RDWR) <= 0) {
		if(mkfifo(FIFO_FILE,O_NONBLOCK) < 0) {
			printf("Error: could not create FIFO.\n");
			return -1;
		}
	}
	if(open(CONTROL_FIFO, O_RDWR) <= 0) {
		if(mkfifo(CONTROL_FIFO,O_NONBLOCK) < 0) {
			printf("Error: could not create control FIFO.\n");
			return -1;
		}
	}
	
	/*// store PID to shared memory (needed by stop process)
	key_t MyKey = ftok(DEVICE_FILE, 's'); // create shared memory key  
    int pid_id = shmget(MyKey, sizeof(pid_t), IPC_CREAT | 0666);
	
    pid_t* pid_ptr = (pid_t *) shmat(pid_id, NULL, 0);
    *pid_ptr = getpid();*/
	
	// store PID to file TODO: do in function
	pid_t pid = getpid();
	FILE* file = fopen(PID_FILE, "w");
	if(file == NULL) {
		printf("Error creating pid file.\n");
		return -1;
	}
	fwrite(&pid, sizeof(pid_t), 1, file);
	//close file
	fclose(file);
	
	
	// register signal handler (for stopping)
	if (signal(SIGQUIT, sig_handler) == SIG_ERR) {
        printf("Can't register signal handler.\n");
		return -1;
	}
	
	// register signal handler (for catching Ctrl+C)
	if (signal(SIGINT, sig_handler) == SIG_ERR) {
		printf("Can't register signal handler.\n");
		return -1;
	}
	
	
	/*// write conf to shared memory
	key_t key = ftok(DEVICE_FILE "conf", 's'); // create shared memory key  
    int conf_id = shmget(key, sizeof(struct rl_conf), IPC_CREAT | 0666);
	struct rl_conf* conf_ptr = (struct rl_conf*) shmat(conf_id, NULL, 0);
	*conf_ptr = *conf;*/

	//write conf to file
	write_config(confn, CONFIG_FILE);
	
	
	// INITIATION
	// init all modules
	pru_init();
	hw_init(confn);
	
	
	// SAMPLING
	hw_sample(confn);
	
	// FINISH
	// close all modules
	if(confn->mode != LIMIT) {
		pru_stop();
	}
	/*if (conf->number_samples == 0) { // when in continuous mode, pru needs to be stopped
		pru_stop();
	}*/
	pru_close();
	hw_close();
	
	
	// set RL state to zero
	//conf_ptr->state = OFF; // write to shared memory
	
	confn->mode = IDLE;
	write_config(confn, CONFIG_FILE);
	
	// remove fifos
	remove(FIFO_FILE);
	remove(CONTROL_FIFO);
	
	/*// detach shared memory
	shmdt(pid_ptr);
    shmctl(pid_id, IPC_RMID, NULL);
	shmdt(conf_ptr);
    shmctl(conf_id, IPC_RMID, NULL);*/
	
	 
	return 1;
	
}

// continuous sample function
int rl_continuous(struct rl_conf_new* confn) {
	
	printf("Data acquisition running in background ...\n  Stop with:   rocketlogger stop\n\n");
	
	// create deamon to run in background
	if (daemon(1, 1) < 0) {
		printf("Error: Deamon");
		return 1;
	}
	
	// continuous sampling
	//conf->number_samples = 0;
	confn->number_samples = 0;
	
	rl_sample(confn);
	
	return 1;
	
}

// stop function (to stop continuous mode)
int rl_stop() {
	
	// read config
	struct rl_conf_new conf;
	if( (read_config(&conf, CONFIG_FILE) == 0) || conf.mode == IDLE) {
		printf("RocketLogger not running!\n");
		return 1;
	}
	
	// get PID from file TODO: do in function
	pid_t pid;
	FILE* file = fopen(PID_FILE, "r");
	if(file == NULL) {
		printf("Error opening pid file.\n");
		return -1;
	}
	fread(&pid, sizeof(pid_t), 1, file); // get PID of background process
	//close file
	fclose(file);
	
	kill(pid, SIGQUIT); // send stop signal
	
	return 1;
	
	/*// get pid of running RL
	key_t MyKey = ftok(DEVICE_FILE, 's'); // create shared memory key 
	int shm_id = shmget(MyKey, sizeof(pid_t), 0666);
    pid_t *pid_ptr = (pid_t *) shmat(shm_id, NULL, 0);
	
	// check if shared memory set
	if (pid_ptr == (void *) -1) {
		printf("RocketLogger not running!\n");
		shmdt(pid_ptr); // detach shared memory
		return 1;
	}
	
	printf("Stopping RocketLogger.\n");
    pid_t pid = *pid_ptr;	// get PID of background process
	*pid_ptr = -1;			// reset pid to make sure, rl is not running
	shmdt(pid_ptr);			// detach shared memory
	kill(pid, SIGQUIT);		// send stop signal
	
	return 1;*/
}

// meter function
int rl_meter(struct rl_conf_new* confn) {
	
	/*conf->update_rate = HZ10;
	conf->number_samples = 0;
	conf->enable_web_server = 0;
	conf->store = 0;*/
	
	confn->update_rate = 10;
	confn->number_samples = 0;
	confn->enable_web_server = 0;
	confn->file_format = NO_FILE;
	
	rl_sample(confn);
	
	return 1;
}
