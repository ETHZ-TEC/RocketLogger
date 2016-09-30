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

void rl_print_config(struct rl_conf* conf, int web) {
	
	char file_format_names[3][10] = {"no file", "csv", "binary"};
	
	if (web == 1) {

		printf("%d\n", conf->enable_web_server);
		printf("%d\n", conf->sample_rate);
		printf("%d\n", conf->update_rate);
		printf("%d\n", conf->number_samples);
		print_channels_new(conf->channels);
		printf("%s\n", conf->file_name);
		printf("%d\n",conf->file_format == BIN); // TODO: add no-file field
		// TODO: add fhr
		
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
		printf("\n");
	}
}

// get status of RL (returns 1 when running)
int rl_get_status(int print, int web) {
	
	int status;
	struct rl_conf conf;
	
	// get pid
	pid_t pid = get_pid();
	if(pid < 0 || kill(pid, 0) < 0) {
		// process not running
		status = 0;
		conf.mode = IDLE;
	} else {
		// read config
		if(read_config(&conf) == 0) { // no config file found -> not running
			status = 0;
			conf.mode = IDLE;
		} else {
			// TODO: handle ERROR
			status = !(conf.mode == IDLE);
		}
	}
	
	// print config if requested
	if(print == 1) {
		print_status(&conf, web);
	}
	
	return status;
}


// main sample function
int rl_sample(struct rl_conf* conf) {
	
	// check mode
	switch (conf->mode) {
		case LIMIT:
			break;
		case CONTINUOUS:
			// create deamon to run in background
			if (daemon(1, 1) < 0) {
				printf("Error: Deamon");
				return 1;
			}
			break;
		case METER:
			// set meter config
			conf->update_rate = 10;
			conf->number_samples = 0;
			conf->enable_web_server = 0;
			conf->file_format = NO_FILE;
			break;
		default:
			printf("Error: wrong mode\n");
			return -1;
	}
	
	// check input
	if(check_sample_rate(conf->sample_rate) < 0) {
		printf("Error: wrong sampling rate\n");
		return -1;
	}
	if(check_update_rate(conf->update_rate) < 0) {
		printf("Error: wrong update rate\n");
		return -1;
	}
	
	// create FIFOs if not existing
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
	
	// store PID to file
	pid_t pid = getpid();
	set_pid(pid);
	
	// register signal handler (for stopping)
	if (signal(SIGQUIT, sig_handler) == SIG_ERR || signal(SIGINT, sig_handler) == SIG_ERR) {
        printf("Error: can't register signal handler.\n");
		return -1;
	}

	//write conf to file
	write_config(conf);
	
	// INITIATION
	hw_init(conf);
	
	// SAMPLING
	hw_sample(conf);
	
	// FINISH
	hw_close(conf);
	
	// write conf to file
	conf->mode = IDLE;
	write_config(conf);
	
	// remove fifos
	remove(FIFO_FILE);
	remove(CONTROL_FIFO);
	 
	return 1;
	
}


// stop function (to stop continuous mode)
int rl_stop() {
	
	// check if running
	if(rl_get_status(0,0) == 0) {
		printf("RocketLogger not running!\n");
		return -1;
	}
	
	// ged pid
	pid_t pid = get_pid();
	
	// send stop signal
	kill(pid, SIGQUIT);
	
	return 1;
}