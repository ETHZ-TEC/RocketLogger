#include <stdio.h>
#include <stdlib.h>

#include "rl_lib.h"
#include "rl_util.h"

struct rl_conf conf;
int set_as_default;

enum rl_mode get_mode(char* mode) { // TODO: move to util
	
	// MODES
	if (strcmp(mode, "sample") == 0) {
		return LIMIT;
	} else if(strcmp(mode, "cont") == 0) {
		return CONTINUOUS;
	} else if(strcmp(mode, "meter") == 0) {
		return METER;
	} else if(strcmp(mode, "status") == 0) {
		return STATUS;
	} else if(strcmp(mode, "calibrate") == 0) {
		return CALIBRATE;
	} else if(strcmp(mode, "data") == 0) {
		return DATA;
	} else if(strcmp(mode, "stop") == 0) {
		return STOPPED;
	} else if(strcmp(mode, "set") == 0) {
		return SET_DEFAULT;
	} else if(strcmp(mode, "print") == 0) {
		return PRINT_DEFAULT;
	} else if(strcmp(mode, "help") == 0 || strcmp(mode, "h") == 0 || strcmp(mode, "-h") == 0) {
		return HELP;
	}
	
	return NO_MODE;
}

enum rl_option get_option(char* option) { // TODO: move to util
	if (strcmp(option, "f") == 0) {
		return FILE_NAME;
	} else if(strcmp(option, "r") == 0) {
		return SAMPLE_RATE;
	} else if(strcmp(option, "u") == 0) {
		return UPDATE_RATE;
	} else if(strcmp(option, "ch") == 0) {
		return CHANNEL;
	} else if(strcmp(option, "fhr") == 0) {
		return FHR;
	} else if(strcmp(option, "w") == 0) {
		return WEB;
	} else if(strcmp(option, "b") == 0) {
		return BINARY_FILE;
	} else if(strcmp(option, "s") == 0) {
		return DEF_CONF;
	}
	
	
	
	return NO_OPTION;
}

int parse_args(int argc, char* argv[]) { //, struct rl_conf* conf) { // TODO: move to util

	int i; // argument count variable
	set_as_default = 0;
	
	// need at least 2 arguments
	if (argc < 2) {
		printf("Error: no mode\n");
		return FAILURE;
	}
	
	// MODE
	conf.mode = get_mode(argv[1]);
	if(conf.mode == NO_MODE) {
		printf("Error: wrong mode\n");
		return FAILURE;
	}
	
	if(conf.mode == LIMIT) {
		// parse sample limit
		if (argc > 2 && isdigit(argv[2][0]) && atoi(argv[2]) > 0) { 
			conf.sample_limit = atoi(argv[2]);
			i = 3;
		} else {
			printf("Error: no possible sample limit\n");
			return FAILURE;
		}
	} else {
		i = 2;
	}
	
	// disable webserver as default for non-continuous mode
	if(conf.mode == STATUS || conf.mode == LIMIT) {
		conf.enable_web_server = 0; 
	}
	
	// stop parsing for some modes
	if (conf.mode == STOPPED || conf.mode == DATA || conf.mode == PRINT_DEFAULT || conf.mode == HELP) {
		return SUCCESS;
	}
	
	// reset default configuration
	if (conf.mode == SET_DEFAULT && isdigit(argv[i][0]) && atoi(argv[i]) == 0) {
		reset_config(&conf);
		conf.mode = SET_DEFAULT;
		return SUCCESS;
	}

	
	// OPTIONS
	for (; i<argc; i++) {
		if (argv[i][0] == '-') {
			switch(get_option(&argv[i][1])) {
				
				case FILE_NAME:
					if (argc > ++i) {
						if (isdigit(argv[i][0]) && atoi(argv[i]) == 0) { // no file write
							conf.file_format = NO_FILE;
						} else {
							strcpy(conf.file_name, argv[i]);
						}
						break;
					} else {
						printf("Error: no file name\n");
						return FAILURE;
					}
					break;
				
				case SAMPLE_RATE:
					if (argc > ++i && isdigit(argv[i][0])) {
						conf.sample_rate = atoi(argv[i]);
						if(check_sample_rate(conf.sample_rate) == FAILURE) { // check if rate allowed
							printf("Error: wrong sampling rate\n");
							return FAILURE;
						}
						break;
					} else {
						printf("Error: no sampling rate\n");
						return FAILURE;
					}
				
				case UPDATE_RATE:
					if (argc > ++i && isdigit(argv[i][0])) {
						conf.update_rate = atoi(argv[i]);
						if(check_update_rate(conf.update_rate) == FAILURE) { // check if rate allowed
							printf("Error: wrong update rate\n");
							return FAILURE;
						}
						break;
					} else {
						printf("Error: no update rate\n");
						return FAILURE;
					}
				
				case CHANNEL:
					if (argc > ++i) {
						
						// check first channel number
						char* c = argv[i];
						if(isdigit(c[0]) && atoi(c) >= 0 && atoi(c) <= 9) {
							
							// reset default channel selection
							memset(conf.channels, 0, sizeof(conf.channels));
							conf.channels[atoi(c)] = 1;
							
						} else {
							printf("Error: wrong channel number\n");
							return FAILURE;
						}
						
						// loop
						int j;
						for (j=1; j < 2*(NUM_CHANNELS-1) && argv[i][j] == ','; j=j+2){
							
							//check channel number
							char* c = &argv[i][j+1];
							if (isdigit(c[0]) && atoi(c) >= 0 && atoi(c) < NUM_CHANNELS) { // TODO: use defines
								conf.channels[atoi(c)] = 1;
							} else {
								printf("Error: wrong channel number\n");
								return FAILURE;
							}
						}													
						break;
					} else {
						printf("Error: no channel number\n");
						return FAILURE;
					}
				
				case FHR:
					if (argc > ++i) {
						
						// check first number
						char* c = argv[i];
						if(isdigit(c[0]) && atoi(c) < 3 && atoi(c) >= 0) {
							
							// reset default forced channel selection
							memset(conf.force_high_channels, 0, sizeof(conf.force_high_channels));
							if(atoi(c) > 0) {
								conf.force_high_channels[atoi(c) - 1] = 1;
							}
						} else {
							printf("Error: wrong force-channel number\n");
							return FAILURE;
						}
						// check second number
						if (argv[i][1] == ',') {
							
							char* c = &argv[i][2];
							if (atoi(c) < 3 && atoi(c) > 0) {
								conf.force_high_channels[atoi(c) - 1] = 1;
							} else {
								printf("Error: wrong force-channel number\n");
								return FAILURE;
							}
						}
						break;
					} else {
						printf("Error: no force channel number\n");
						return FAILURE;
					}
				
				case WEB:
					conf.enable_web_server = 1;
					break;
				
				case BINARY_FILE:
					conf.file_format = BIN;	
					break;
				
				case DEF_CONF:
					set_as_default = 1;
					break;
				
				case NO_OPTION:
					printf("Error: wrong option\n");
					return FAILURE;
				
				default:
					printf("Error: wrong option\n");
					return FAILURE;
			}
		} else {
			printf("Error: use -[option] [value]\n");
			return FAILURE;
		}
	}
	
	return SUCCESS;
}


int main(int argc, char* argv[]) {
	
	// check if root
	if(getuid() != 0){
		printf("Error: you must run this program as root\n");
		return FAILURE;
	}
	
	// get default config
	read_default_config(&conf);
	
	// parse arguments
	if (parse_args(argc, argv) == FAILURE) {
		print_usage(&conf);
		exit(EXIT_FAILURE);
	}
	
	
	// store config as default
	if(set_as_default == 1) {
		write_default_config(&conf);
	}
	
	switch (conf.mode) {
		case LIMIT:
			if(rl_get_status(0,0) == RUNNING) {
				printf("Error: RocketLogger already running\n Run:  rocketlogger stop\n\n");
				exit(EXIT_FAILURE);
			}
			print_config(&conf);
			printf("Start sampling ...\n");
			break;
			
		case CONTINUOUS:
			if(rl_get_status(0,0) == RUNNING) {
				printf("Error: RocketLogger already running\n Run:  rocketlogger stop\n\n");
				exit(EXIT_FAILURE);
			}
			print_config(&conf);
			printf("Data acquisition running in background ...\n  Stop with:   rocketlogger stop\n\n");
			break;
		
		case METER:
			if(rl_get_status(0,0) == RUNNING) {
				printf("Error: RocketLogger already running\n Run:  rocketlogger stop\n\n");
				exit(EXIT_FAILURE);
			}
			break;
		
		case STATUS:
			rl_get_status(1,conf.enable_web_server);
			exit(EXIT_SUCCESS);
		
		case STOPPED:
			if(rl_get_status(0,0) != RUNNING) {
				printf("Error: RocketLogger not running\n");
				exit(EXIT_FAILURE);
			}
			printf("Stopping RocketLogger ...\n");
			rl_stop();
			exit(EXIT_SUCCESS);
		
		case DATA:
			if(rl_get_status(0,0) != RUNNING) {
				printf("Error: RocketLogger not running\n");
				exit(EXIT_FAILURE);
			}
			rl_get_data();
			exit(EXIT_SUCCESS);
		
		case CALIBRATE:
			if(rl_get_status(0,0) == RUNNING) {
				printf("Warning: reset will not affect current measurement\n");
			}
			rl_reset_calibration();
			exit(EXIT_SUCCESS);
		
		case SET_DEFAULT:
			write_default_config(&conf);
			print_config(&conf);
			exit(EXIT_SUCCESS);
		
		case PRINT_DEFAULT:
			print_config(&conf);
			exit(EXIT_SUCCESS);
		
		case HELP:
			print_usage(&conf);
			exit(EXIT_SUCCESS);
		
		default:
			print_usage(&conf);
			exit(EXIT_FAILURE);
	}
	
	// start the sampling
	rl_sample(&conf);
	
	exit(EXIT_SUCCESS);
	
}