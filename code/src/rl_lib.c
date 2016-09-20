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
	char str[150];
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

// get status of RL (returns 1 when running)
int rl_get_status(int print) {
	
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
	return (conf.state != OFF);
}

// get status of RL (for web usage)
int rl_get_status_web() {
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
}


// main sample function
int rl_sample(struct rl_conf* conf) {
	
	rl_log("Info: RocketLogger started.\n");
	
	// create FIFOs if not existing
	if(open(FIFO_FILE, O_RDWR) <= 0) {
		if(mkfifo(FIFO_FILE,O_NONBLOCK) < 0) {
			printf("Error: could not create FIFO.\n");
			rl_log("Error: could not create FIFO.\n");
			return -1;
		}
	}
	if(open(CONTROL_FIFO, O_RDWR) <= 0) {
		if(mkfifo(CONTROL_FIFO,O_NONBLOCK) < 0) {
			printf("Error: could not create control FIFO.\n");
			rl_log("Error: could not create control FIFO.\n");
			return -1;
		}
	}
	rl_log("Info: webserver FIFO configured.\n");
	
	// store PID to shared memory (needed by stop process)
	key_t MyKey = ftok(DEVICE_FILE, 's'); // create shared memory key  
    int pid_id = shmget(MyKey, sizeof(pid_t), IPC_CREAT | 0666);
	
    pid_t* pid_ptr = (pid_t *) shmat(pid_id, NULL, 0);
    *pid_ptr = getpid();
	
	// register signal handler (for stopping)
	if (signal(SIGQUIT, sig_handler) == SIG_ERR) {
        printf("Can't register signal handler.\n");
		rl_log("Error: can't register signal handler\n");
		return -1;
	}
	
	// register signal handler (for catching Ctrl+C)
	if (signal(SIGINT, sig_handler) == SIG_ERR) {
	        printf("Can't register signal handler.\n");
			return -1;
		}

	// load values of conf struct
	char file[100];
	strcpy(file,conf->file);
	int sample_rate = conf->rate;
	int update_rate = conf->update_rate;
	int number_samples = conf->number_samples;
	int channels = conf->channels;
	int force_high_channels = conf->force_high_channels;
	int webserver = conf->enable_web_server;
	int store = conf->store;
	int meter = 0;
	if (conf->state == RL_METER) {
		store = 0;
		meter = 1;
	}
	int binary = conf->binary_file;
	
	int rate;
	// set rate
	switch (sample_rate) {
		case 1:
			rate = K1;
			break;
		case 2:
			rate = K2;
			break;
		case 4:
			rate = K4;
			break;
		case 8:
			rate = K8;
			break;
		case 16:
			rate = K16;
			break;
		case 32:
			rate = K32;
			break;
		case 64:
			rate = K64;
			break;
		default:
			printf("Wrong sample rate.\n");
			return -1;
	}
	
	// set update rate
	switch (update_rate) {
		case 1:
			update_rate = HZ1;
			break;
		case 2:
			update_rate = HZ2;
			break;
		case 5:
			update_rate = HZ5;
			break;
		case 10:
			update_rate = HZ10;
			break;
		default:
			printf("Wrong update rate.\n");
			return -1;
	}
	
	// write conf to shared memory
	key_t key = ftok(DEVICE_FILE "conf", 's'); // create shared memory key  
    int conf_id = shmget(key, sizeof(struct rl_conf), IPC_CREAT | 0666);
	struct rl_conf* conf_ptr = (struct rl_conf*) shmat(conf_id, NULL, 0);
	*conf_ptr = *conf;
	
	rl_log("Info: configuration loading finished.\n");
	
	// open data file
	FILE* data = (FILE*) -1;
	if (store == 1) { // open file only if storing requested
		data = fopen(file, "w+");
		if(data == NULL) {
			rl_log("Error: data-file opening not possible.\n");
			perror("Error opening data-file");
			return -1;
		}
		// change access rights to data file
		char cmd[50];
		sprintf(cmd, "chmod 777 %s", file);
		system(cmd);
	} 
	
	rl_log("Info: data file opened.\n");
	
	// INITIATION
	// init all modules
	pru_init();
	pwm_init();
	gpio_init();
	
	// set high range force
	force_high_range(force_high_channels);
	
	rl_log("Info: initiation finished.\n");
	
	// SAMPLING
	// pru_sample
	pru_sample(data, rate, update_rate, number_samples, channels, webserver, store, meter, binary);
	
	rl_log("Info: sampling finished.\n");
	
	// FINISH
	// close all modules
	if (number_samples == 0) { // when in continuous mode, pru needs to be stopped
		pru_stop();
	}
	pru_close();
	pwm_close();
	gpio_close();
	
	rl_log("Info: modules closed.\n");
	
	// set RL state to zero
	conf_ptr->state = OFF; // write to shared memory

	// close data file
	if (store == 1) {
		fclose(data);
	}
	
	// remove fifos
	remove(FIFO_FILE);
	remove(CONTROL_FIFO);
	
	// detach shared memory
	shmdt(pid_ptr);
    shmctl(pid_id, IPC_RMID, NULL);
	shmdt(conf_ptr);
    shmctl(conf_id, IPC_RMID, NULL);
	
	rl_log("Success: RocketLogger finished successfully.\n");
	 
	return 1;
	
}

// continuous sample function
int rl_continuous(struct rl_conf* conf) {
	
	printf("Data acquisition running in background ...\n  Stop with:   rocketlogger stop\n\n");
	rl_log("Info: data acquisition in background.\n");
	
	// create deamon to run in background
	if (daemon(1, 1) < 0) {
		rl_log("Error: background process creation not possible.\n");
		perror("Deamon");
		return 1;
	}
	
	// continuous sampling
	conf->number_samples = 0;
	
	rl_sample(conf);
	
	return 1;
	
}

// stop function (to stop continuous mode)
int rl_stop() {
	
	// get pid of running RL
	key_t MyKey = ftok(DEVICE_FILE, 's'); // create shared memory key 
	int shm_id = shmget(MyKey, sizeof(pid_t), 0666);
    pid_t *pid_ptr = (pid_t *) shmat(shm_id, NULL, 0);
	
	// check if shared memory set
	if (pid_ptr == (void *) -1) {
		printf("RocketLogger not running!\n");
		rl_log("Error: RocketLogger not running.\n");
		shmdt(pid_ptr); // detach shared memory
		return 1;
	}
	
	printf("Stopping RocketLogger.\n");
	rl_log("Info: stopping RocketLogger.\n");
    pid_t pid = *pid_ptr;	// get PID of background process
	*pid_ptr = -1;			// reset pid to make sure, rl is not running
	shmdt(pid_ptr);			// detach shared memory
	kill(pid, SIGQUIT);		// send stop signal
	
	return 1;
}

// meter function
int rl_meter(struct rl_conf* conf) {
	
	conf->update_rate = HZ10;
	conf->number_samples = 0;
	conf->enable_web_server = 0;
	
	rl_sample(conf);
	
	return 1;
}
