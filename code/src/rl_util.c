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
	} else if(strcmp(option, "d") == 0) {
		return DIGITAL_INPUTS;
	} else if(strcmp(option, "s") == 0) {
		return DEF_CONF;
	} else if(strcmp(option, "format") == 0) {
		return FILE_FORMAT;
	}
	
	return NO_OPTION;
}

int parse_channels(int channels[], char* value) {
	
	// check first channel number
	if(isdigit(value[0]) && atoi(value) >= 0 && atoi(value) <= 9) {
		
		// reset default channel selection
		memset(channels, 0, sizeof(int) * NUM_CHANNELS);
		channels[atoi(value)] = 1;
		
	} else if(strcmp(value, "all") == 0) {
		// all channels
		int i;
		for(i=0; i<NUM_CHANNELS; i++) {
			channels[i] = CHANNEL_ENABLED;
		}
	}else {
		rl_log(ERROR, "wrong channel number");
		return FAILURE;
	}
	
	// loop
	int j;
	for (j=1; j < 2*(NUM_CHANNELS-1) && value[j] == ','; j=j+2){
		
		//check channel number
		char* c = &value[j+1];
		if (isdigit(c[0]) && atoi(c) >= 0 && atoi(c) < NUM_CHANNELS) {
			channels[atoi(c)] = 1;
		} else {
			rl_log(ERROR, "wrong channel number");
			return FAILURE;
		}
	}
	
	return SUCCESS;
}

int parse_args(int argc, char* argv[], struct rl_conf* conf, int* set_as_default) {

	int i; // argument count variable
	int no_file = 0;
	*set_as_default = 0;
	
	// need at least 2 arguments
	if (argc < 2) {
		rl_log(ERROR, "no mode");
		return FAILURE;
	}
	
	// MODE
	conf->mode = get_mode(argv[1]);
	if(conf->mode == NO_MODE) {
		rl_log(ERROR, "wrong mode");
		return FAILURE;
	}
	
	if(conf->mode == LIMIT) {
		// parse sample limit
		if (argc > 2 && isdigit(argv[2][0]) && atoi(argv[2]) > 0) { 
			conf->sample_limit = atoi(argv[2]);
			i = 3;
		} else {
			rl_log(ERROR, "no possible sample limit");
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
	if (conf->mode == STOPPED || conf->mode == PRINT_DEFAULT || conf->mode == HELP) {
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
						rl_log(ERROR, "no file nam");
						return FAILURE;
					}
					break;
				
				case SAMPLE_RATE:
					if (argc > ++i && isdigit(argv[i][0])) {
						conf->sample_rate = atoi(argv[i]);
						if(check_sample_rate(conf->sample_rate) == FAILURE) { // check if rate allowed
							rl_log(ERROR, "wrong sampling rate");
							return FAILURE;
						}
					} else {
						rl_log(ERROR, "no sampling rate");
						return FAILURE;
					}
					break;
				
				case UPDATE_RATE:
					if (argc > ++i && isdigit(argv[i][0])) {
						conf->update_rate = atoi(argv[i]);
						if(check_update_rate(conf->update_rate) == FAILURE) { // check if rate allowed
							rl_log(ERROR, "wrong update rate");
							return FAILURE;
						}
					} else {
						rl_log(ERROR, "no update rate");
						return FAILURE;
					}
					break;
				
				case CHANNEL:
					if (argc > ++i) {
						if(parse_channels(conf->channels, argv[i]) == FAILURE) {
							return FAILURE;
						}
					} else {
						rl_log(ERROR, "no channel number");
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
							rl_log(ERROR, "wrong force-channel number");
							return FAILURE;
						}
						// check second number
						if (argv[i][1] == ',') {
							
							char* c = &argv[i][2];
							if (atoi(c) < 3 && atoi(c) > 0) {
								conf->force_high_channels[atoi(c) - 1] = 1;
							} else {
								rl_log(ERROR, "wrong force-channel number");
								return FAILURE;
							}
						}
					} else {
						rl_log(ERROR, "no force channel number");
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
				
				case DIGITAL_INPUTS:
					if(argc > i+1 && isdigit(argv[i+1][0]) && atoi(argv[i+1]) == 0) {
						i++;
						conf->digital_inputs = DIGITAL_INPUTS_DISABLED;
					} else {
						conf->digital_inputs = DIGITAL_INPUTS_ENABLED;
					}
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
								rl_log(ERROR, "wrong file format");
								return FAILURE;
							}
						} else {
							rl_log(INFO, "file format ignored");
						}
					} else {
						rl_log(ERROR, "no file format");
						return FAILURE;
					}
					break;
				
				case NO_OPTION:
					rl_log(ERROR, "wrong option");
					return FAILURE;
				
				default:
					rl_log(ERROR, "wrong option");
					return FAILURE;
			}
		} else {
			rl_log(ERROR, "use -[option] [value]");
			return FAILURE;
		}
	}
	
	return SUCCESS;
}


// help

void print_usage() {
	printf("\nUsage:\n");
	printf("  rocketlogger [mode] -[option] [value]\n\n");
	printf("  Modes:\n");
	printf("    sample [number]    Acquires [number] of samples.\n");
	printf("    cont               Continuously acquires samples.\n");
	printf("    meter              Starts RocketLogger Meter.\n");
	printf("    status             Get status of RocketLogger.\n");
	printf("    calibrate          Reset RocketLogger calibration.\n");
	printf("    stop               Stops RocketLogger.\n");
	printf("    set                Set default configuration of RocketLogger (use normal options).\n");
	printf("                         Use 'set 0' to reset the default configuration.\n");
	printf("    conf               Print default configuration of RocketLogger.\n");
	
	printf("\n  Options:\n");
	printf("    -r [number]        Acquisition rate selection.\n");
	printf("                         Possible rates: 1, 2, 4, 8, 16, 32, 64 (in kSps)\n");
	printf("    -u [number]        Data update rate selection.\n");
	printf("                         Possible update rates: 1, 2, 5, 10 (in Hz)\n");
	printf("    -ch [number1,...]  Channel selection.\n");
	printf("                       Possible channels ('-ch all' to enable all):\n");
	printf("                         0: I1H\t\t4: I2H\n");
	printf("                         1: I1L\t\t5: I2L\n");
	printf("                         2: V1 \t\t6: V3\n");
	printf("                         3: V2 \t\t7: V4\n");
	printf("    -fhr [0,1,2]       Force high-range.\n");
	printf("                         0: no channel, 1: I1, 2: I2\n");
	printf("    -f [file]          Stores data to specified file.\n");
	printf("                         '-f 0' will disable file storing.\n");
	printf("    -d                 Log digital inputs.\n");
	printf("                         Use '-d 0' to disable digital input logging.\n");
	printf("    -format [format]   Select file format: csv, bin.\n");
	printf("    -w                 Enable webserver plotting.\n");
	printf("                         Use '-w 0' to disable webserver plotting.\n");
	printf("    -s                 Set configuration as default.\n");
	
	printf("\n");
}


// configuration handling

void print_config(struct rl_conf* conf) {
	printf("\nRocketLogger Configuration:\n");
	rl_print_config(conf);
	printf("\n");
	
}


void reset_config(struct rl_conf* conf) {
	conf->mode = CONTINUOUS;
	conf->sample_rate = 1;
	conf->update_rate = 1;
	conf->sample_limit = 0;
	conf->digital_inputs = DIGITAL_INPUTS_ENABLED;
	conf->enable_web_server = 1;
	conf->file_format = BIN;
	
	strcpy(conf->file_name, "/var/www/data/data.rld");
	
	int i;
	for(i=0; i<NUM_CHANNELS; i++) {
		conf->channels[i] = CHANNEL_ENABLED;
	}
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
		rl_log(ERROR, "failed to open configuration file");
		return FAILURE;
	}
	// read values
	fread(conf, sizeof(struct rl_conf), 1, file);
	
	// reset mode
	//if(conf->sample_limit == 0) { // TODO: check
		conf->mode = CONTINUOUS;
	/*} else {
		conf->mode = LIMIT;
	}*/
	
	//close file
	fclose(file);
	return SUCCESS;
}

int write_default_config(struct rl_conf* conf) {
	
	// open config file
	FILE* file = fopen(DEFAULT_CONFIG, "w");
	if(file == NULL) {
		rl_log(ERROR, "failed to create configuration file");
		return FAILURE;
	}
	// write values
	fwrite(conf, sizeof(struct rl_conf), 1, file);
	
	//close file
	fclose(file);
	return SUCCESS;
}	






// conf file read helpers

// TODO: add digital inputs

void remove_newline(char *line) {
    int new_line = strlen(line) - 1;
    if (line[new_line] == '\n') {
        line[new_line] = '\0';
	}
}

int parse_value(char* name, char* value, struct rl_conf* conf) {
	if(name[0] == '#') { // comment
		return SUCCESS;
	} else if(strcmp(name, "SAMPLE_RATE") == 0) {
		printf("Sampling rate = %d\n", atoi(value)); // TODO: integer test
		
	} else if(strcmp(name, "UPDATE_RATE") == 0) {
		printf("Update rate = %d\n", atoi(value)); // TODO: integer test
		
	} else if(strcmp(name, "SAMPLE_LIMIT") == 0) {
		printf("Sample limit = %d\n", atoi(value)); // TODO: integer test
		
	} else if(strcmp(name, "CHANNELS") == 0) {
		printf("Channels = %s\n", value); // TODO: parse
		return parse_channels(conf->channels, value);
		
	} else if(strcmp(name, "FORCE_HIGH_CHANNELS") == 0) {
		printf("Forced channels = %s\n", value); // TODO: parse
		
	} else if(strcmp(name, "ENABLE_WEB_SERVER") == 0) {
		printf("Web server = %s\n", value); // TODO: parse
		
	} else if(strcmp(name, "FILE_FORMAT") == 0) {
		printf("File format = %s\n", value);// TODO: parse
		
	} else if(strcmp(name, "FILE_NAME") == 0) {
		printf("File name = %s\n", value);
		
	} else {
		return FAILURE;
	}
	
	return SUCCESS;
}

int read_default_config_new(struct rl_conf* conf) {
	
	char line[MAX_LINE_LENGTH];
	FILE* fp;
	
	fp = fopen(DEFAULT_CONFIG_NEW, "r");
	if (fp == NULL) {
		rl_log(ERROR, "failed to open default configuration file");
		return FAILURE;
	}
	
	
	int j;
	int i;
	for(j=0; fgets(line, MAX_LINE_LENGTH, fp) != NULL; j++) {
		
		if(line[0] != '\n') { // ignore empty lines
		
			remove_newline(line);
			
			char* word;
			char words[MAX_WORDS_PER_LINE][MAX_WORD_LENGTH];
			
			// extract words
			word = strtok(line, " ");
			for(i=0; i<MAX_WORDS_PER_LINE && word; i++) {
				
				strcpy(words[i], word);
				word = strtok(NULL, " ");
				
			}
			
			
			if(words[0][0] != '#'  && i<MAX_WORDS_PER_LINE) {
				rl_log(ERROR, "config file parsing failed near line %d\n", j+1);
				return FAILURE;
			} else {
			
				// parse value
				if(parse_value(words[0], words[2], conf) == FAILURE) {
					rl_log(ERROR, "config file parsing failed near line %d\n", j+1);
					return FAILURE;
				}
			}
		}
	}
	
	fclose(fp);
	
	return SUCCESS;
}
