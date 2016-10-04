#include "rl_util.h"

// argument parsing

enum rl_mode get_mode(char* mode) {
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
	} else if(strcmp(mode, "conf") == 0) {
		return PRINT_DEFAULT;
	} else if(strcmp(mode, "help") == 0 || strcmp(mode, "h") == 0 || strcmp(mode, "-h") == 0) {
		return HELP;
	}
	
	return NO_MODE;
}

enum rl_option get_option(char* option) {
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
	} else if(strcmp(option, "format") == 0) {
		return FILE_FORMAT;
	}
	
	return NO_OPTION;
}

int parse_args(int argc, char* argv[], struct rl_conf* conf, int* set_as_default) {

	int i; // argument count variable
	int no_file = 0;
	*set_as_default = 0;
	
	// need at least 2 arguments
	if (argc < 2) {
		printf("Error: no mode\n");
		return FAILURE;
	}
	
	// MODE
	conf->mode = get_mode(argv[1]);
	if(conf->mode == NO_MODE) {
		printf("Error: wrong mode\n");
		return FAILURE;
	}
	
	if(conf->mode == LIMIT) {
		// parse sample limit
		if (argc > 2 && isdigit(argv[2][0]) && atoi(argv[2]) > 0) { 
			conf->sample_limit = atoi(argv[2]);
			i = 3;
		} else {
			printf("Error: no possible sample limit\n");
			return FAILURE;
		}
	} else {
		i = 2;
	}
	
	// disable webserver as default for non-continuous mode
	if(conf->mode == STATUS || conf->mode == LIMIT) {
		conf->enable_web_server = 0; 
	}
	
	// stop parsing for some modes
	if (conf->mode == STOPPED || conf->mode == DATA || conf->mode == PRINT_DEFAULT || conf->mode == HELP) {
		return SUCCESS;
	}
	
	// reset default configuration
	if (conf->mode == SET_DEFAULT && isdigit(argv[i][0]) && atoi(argv[i]) == 0) {
		reset_config(conf);
		conf->mode = SET_DEFAULT;
		return SUCCESS;
	}

	
	// OPTIONS
	for (; i<argc; i++) {
		if (argv[i][0] == '-') {
			switch(get_option(&argv[i][1])) {
				
				case FILE_NAME:
					if (argc > ++i) {
						if (isdigit(argv[i][0]) && atoi(argv[i]) == 0) { // no file write
							no_file = 1;
							conf->file_format = NO_FILE;
						} else {
							strcpy(conf->file_name, argv[i]);
						}
					} else {
						printf("Error: no file name\n");
						return FAILURE;
					}
					break;
				
				case SAMPLE_RATE:
					if (argc > ++i && isdigit(argv[i][0])) {
						conf->sample_rate = atoi(argv[i]);
						if(check_sample_rate(conf->sample_rate) == FAILURE) { // check if rate allowed
							printf("Error: wrong sampling rate\n");
							return FAILURE;
						}
					} else {
						printf("Error: no sampling rate\n");
						return FAILURE;
					}
					break;
				
				case UPDATE_RATE:
					if (argc > ++i && isdigit(argv[i][0])) {
						conf->update_rate = atoi(argv[i]);
						if(check_update_rate(conf->update_rate) == FAILURE) { // check if rate allowed
							printf("Error: wrong update rate\n");
							return FAILURE;
						}
					} else {
						printf("Error: no update rate\n");
						return FAILURE;
					}
					break;
				
				case CHANNEL:
					if (argc > ++i) {
						
						// check first channel number
						char* c = argv[i];
						if(isdigit(c[0]) && atoi(c) >= 0 && atoi(c) <= 9) {
							
							// reset default channel selection
							memset(conf->channels, 0, sizeof(conf->channels));
							conf->channels[atoi(c)] = 1;
							
						} else {
							printf("Error: wrong channel number\n");
							return FAILURE;
						}
						
						// loop
						int j;
						for (j=1; j < 2*(NUM_CHANNELS-1) && argv[i][j] == ','; j=j+2){
							
							//check channel number
							char* c = &argv[i][j+1];
							if (isdigit(c[0]) && atoi(c) >= 0 && atoi(c) < NUM_CHANNELS) {
								conf->channels[atoi(c)] = 1;
							} else {
								printf("Error: wrong channel number\n");
								return FAILURE;
							}
						}													
					} else {
						printf("Error: no channel number\n");
						return FAILURE;
					}
					break;
				
				case FHR:
					if (argc > ++i) {
						
						// check first number
						char* c = argv[i];
						if(isdigit(c[0]) && atoi(c) < 3 && atoi(c) >= 0) {
							
							// reset default forced channel selection
							memset(conf->force_high_channels, 0, sizeof(conf->force_high_channels));
							if(atoi(c) > 0) {
								conf->force_high_channels[atoi(c) - 1] = 1;
							}
						} else {
							printf("Error: wrong force-channel number\n");
							return FAILURE;
						}
						// check second number
						if (argv[i][1] == ',') {
							
							char* c = &argv[i][2];
							if (atoi(c) < 3 && atoi(c) > 0) {
								conf->force_high_channels[atoi(c) - 1] = 1;
							} else {
								printf("Error: wrong force-channel number\n");
								return FAILURE;
							}
						}
					} else {
						printf("Error: no force channel number\n");
						return FAILURE;
					}
					break;
				
				case WEB:
					if(argc > i+1 && isdigit(argv[i+1][0]) && atoi(argv[i+1]) == 0) {
						i++;
						conf->enable_web_server = 0;
					} else {
						conf->enable_web_server = 1;
						}
					break;
				
				case BINARY_FILE:
					conf->file_format = BIN;	
					break;
				
				case DEF_CONF:
					*set_as_default = 1;
					break;
				
				case FILE_FORMAT:
					if(argc > ++i) {
						if(no_file == 0) { // ignore format, when no file is written
							if(strcmp(argv[i],"csv") == 0) {
								conf->file_format = CSV;
							} else if(strcmp(argv[i],"bin") == 0) {
								conf->file_format = BIN;
							} else {
								printf("Error: wrong file format\n");
								return FAILURE;
							}
						} else {
							printf("Warning: file format ignored\n");
						}
					} else {
						printf("Error: no file format\n");
						return FAILURE;
					}
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


// help

void print_usage(struct rl_conf* conf) {
	printf("\nUsage:\n");
	printf("  rocketlogger [mode] -[option] [value]\n\n");
	printf("  Modes:\n");
	printf("    sample [number]    Acquires [number] of samples.\n");
	printf("    cont               Continuously acquires samples.\n");
	printf("    meter              Starts RocketLogger Meter.\n");
	printf("    status             Get status of RocketLogger.\n");
	printf("    calibrate          Reset RocketLogger calibration.\n");
	printf("    data               Get current data.\n");
	printf("    stop               Stops RocketLogger.\n");
	printf("    set                Set default configuration of RocketLogger (use normal options).\n");
	printf("                         Use set 0 to reset the default configuration.\n");
	printf("    conf               Print default configuration of RocketLogger.\n");
	
	printf("\n  Options:\n");
	printf("    -r [number]        Acquisition rate selection.\n");
	printf("                         Possible rates: 1, 2, 4, 8, 16, 32, 64 (in kSps)\n");
	printf("    -u [number]        Data update rate selection.\n");
	printf("                         Possible update rates: 1, 2, 5, 10 (in Hz)\n");
	printf("    -ch [number1,...]  Channel selection.\n");
	printf("                       Possible channels:\n");
	printf("                         0: I1H\t\t5: I2H\n");
	printf("                         1: I1M\t\t6: I2M\n");
	printf("                         2: I1L\t\t7: I2L\n");
	printf("                         3: V1 \t\t8: V3\n");
	printf("                         4: V2 \t\t9: V4\n");
	printf("    -fhr [0,1,2]       Force high-range.\n");
	printf("                         0: no channel, 1: I1, 2: I2\n");
	printf("    -f [file]          Stores data to specified file.\n");
	printf("                         -f 0 will disable file storing.\n");
	printf("    -b                 Set output file to binary.\n"); // TODO: adapt webserver and remove
	printf("    -format [format]   Select file format: csv, bin.\n");
	printf("    -w                 Enable webserver plotting.\n");
	printf("                         Use -w 0 to disable webserver plotting.\n");
	printf("    -s                 Set configuration as default.\n");
	
	printf("\n");
}


// configuration handling

void print_config(struct rl_conf* conf) {
	printf("\nRocketLogger Configuration:\n");
	rl_print_config(conf, 0);
	printf("\n");
	
}


void reset_config(struct rl_conf* conf) {
	conf->mode = CONTINUOUS;
	conf->sample_rate = 1;
	conf->update_rate = 1;
	conf->sample_limit = 0;
	conf->enable_web_server = 1;
	conf->file_format = BIN;
	
	strcpy(conf->file_name, "/var/www/data/data.dat");
	
	memset(conf->channels, 1, sizeof(conf->channels));
	memset(conf->force_high_channels, 0, sizeof(conf->force_high_channels));
	
}

int read_default_config(struct rl_conf* conf) {
	
	// check if config file existing
	if(open(DEFAULT_CONFIG, O_RDWR) <= 0) {
		reset_config(conf);
		return UNDEFINED;
	}
	
	// open config file
	FILE* file = fopen(DEFAULT_CONFIG, "r");
	if(file == NULL) {
		printf("Error opening configuration file.\n");
		return FAILURE;
	}
	// read values
	fread(conf, sizeof(struct rl_conf), 1, file);
	
	// reset mode
	conf->mode = CONTINUOUS;
	
	//close file
	fclose(file);
	return SUCCESS;
}

int write_default_config(struct rl_conf* conf) {
	
	// open config file
	FILE* file = fopen(DEFAULT_CONFIG, "w");
	if(file == NULL) {
		printf("Error creating configuration file.\n");
		return FAILURE;
	}
	// write values
	fwrite(conf, sizeof(struct rl_conf), 1, file);
	
	//close file
	fclose(file);
	return SUCCESS;
}