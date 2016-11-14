#include "rl_lib.h"

void rl_reset_calibration() {
	
	// set zero offset/scale (raw data)
	remove(CALIBRATION_FILE);
	reset_offsets();
	reset_scales();
	write_calibration();
	
}

void rl_print_config(struct rl_conf* conf) {
	
	char file_format_names[3][10] = {"no file", "csv", "binary"};

										printf("  Sampling rate:   %dkSps\n", conf->sample_rate);
										printf("  Update rate:     %dHz\n", conf->update_rate);
	if(conf->enable_web_server == 1)	printf("  Webserver:       enabled\n");
	else								printf("  Webserver:       disabled\n");
	if(conf->digital_inputs == 1)		printf("  Digital inputs:  enabled\n");
	else								printf("  Digital inputs:  disabled\n");
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
	if (conf->sample_limit == 0) {		printf("  Sample limit:    no limit\n");
	} else {							printf("  Sample limit:    %d\n", conf->sample_limit);}
}

void rl_print_status(struct rl_status* status) {
	
	if(status->state == RL_OFF) {
		printf("\nRocketLogger IDLE\n\n");
	} else {
		printf("\nRocketLogger Status: RUNNING\n");
		rl_print_config(&(status->conf));
		printf("  Samples taken:   %d\n\n", status->samples_taken);
	}
}

// get status of RL (returns 1 when running)
enum rl_state rl_get_status(int print) {
	
	struct rl_status status;
	
	// get pid
	pid_t pid = get_pid();
	if(pid == FAILURE || kill(pid, 0) < 0) {
		// process not running
		// TODO: handle ERROR
		status.state = RL_OFF;
	} else {
		// read status
		read_status(&status);
	}
	
	// print config if requested
	if(print == 1) {
		rl_print_status(&status);
	}
	
	return status.state;
}

int rl_read_status(struct rl_status* status) {
	
	// get pid
	pid_t pid = get_pid();
	if(pid == FAILURE || kill(pid, 0) < 0) {
		// process not running
		// TODO: handle ERROR
		status->state = RL_OFF;
	} else {
		// read status
		read_status(status);
	}
	
	return status->state;
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
				rl_log(ERROR, "failed to create background process");
				return SUCCESS;
			}
			break;
		case METER:
			// set meter config
			conf->update_rate = METER_UPDATE_RATE;
			conf->sample_limit = 0;
			conf->enable_web_server = 0;
			conf->file_format = NO_FILE;
			break;
		default:
			rl_log(ERROR, "wrong mode");
			return FAILURE;
	}
	
	// check input
	if(check_sample_rate(conf->sample_rate) == FAILURE) {
		rl_log(ERROR, "wrong sampling rate");
		return FAILURE;
	}
	if(check_update_rate(conf->update_rate) == FAILURE) {
		rl_log(ERROR, "wrong update rate");
		return FAILURE;
	}
	if(conf->update_rate != 1 && conf->enable_web_server == 1) {
		rl_log(WARNING, "webserver plot does not work with update rates >1. Disabling webserver ...");
		conf->enable_web_server = 0;
	}
	
	// create FIFOs if not existing
	if(open(FIFO_FILE, O_RDWR) <= 0) {
		if(mkfifo(FIFO_FILE,O_NONBLOCK) < 0) {
			rl_log(ERROR, "could not create FIFO");
			return FAILURE;
		}
	}
	if(open(CONTROL_FIFO, O_RDWR) <= 0) {
		if(mkfifo(CONTROL_FIFO,O_NONBLOCK) < 0) {
			rl_log(ERROR, "could not create control FIFO");
			return FAILURE;
		}
	}
	
	// store PID to file
	pid_t pid = getpid();
	set_pid(pid);
	
	// register signal handler (for stopping)
	if (signal(SIGQUIT, sig_handler) == SIG_ERR || signal(SIGINT, sig_handler) == SIG_ERR) {
        rl_log(ERROR, "can't register signal handler");
		return FAILURE;
	}
	
	// INITIATION
	hw_init(conf);
	
	rl_log(INFO, "sampling started");
	
	// SAMPLING
	hw_sample(conf);
	
	rl_log(INFO, "sampling finished");
	
	// FINISH
	hw_close(conf);
	
	// remove fifos
	remove(FIFO_FILE);
	remove(CONTROL_FIFO);
	 
	return SUCCESS;
	
}


// stop function (to stop continuous mode)
int rl_stop() {
	
	// check if running
	if(rl_get_status(0) != RL_RUNNING) {
		rl_log(ERROR, "RocketLogger not running");
		return FAILURE;
	}
	
	// ged pid
	pid_t pid = get_pid();
	
	// send stop signal
	kill(pid, SIGQUIT);
	
	return SUCCESS;
}