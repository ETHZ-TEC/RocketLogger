#include <stdio.h>
#include <stdlib.h>

#include "rl_lib.h"
#include "rl_util.h"

struct rl_conf_new conf;
int set_as_default;

int parse_args(int argc, char* argv[]) { //TODO: outsource
	
	// need at least 2 arguments
	if (argc < 2) {
		return -1;
	}
	
	//int file = 0;
	set_as_default = 0;
	int i = 1;
	// modes
	switch (argv[i][0]) {
		case 's':
			if(argv[i][1] == 'e') { //set default configuration
				conf.mode = SET_DEFAULT;
				break;
			} else {
				switch (argv[i][2]) {
					case 'm': // sampling mode
						if (argc > i+1 && isdigit(argv[i+1][0]) && atoi(argv[i+1]) > 0) { 
							
							conf.mode = LIMIT;
							conf.number_samples = atoi(argv[i+1]);
							i++;
							break;
						} else {
							return -1;
						}
						break;
					
					case 'a': // status
						conf.mode = STATUS;
						break;
					
					case 'o': // stop
						conf.mode = STOPPED;
						return 1;
					
					default: 
						return -1;
				}
			}
			break;
		case 'c':
			if (argv[i][1] == 'o') { // continuous mode
				conf.mode = NEW_CONTINUOUS;
				break;
			
			} else {
				conf.mode = CALIBRATE;
				break;
			}
			
		case 'm': // meter mode
			conf.mode = METER;
			break;
		
		case 'd': // data
			conf.mode = DATA;
			return 1;
		
		case 'p': // print default
			conf.mode = PRINT_DEFAULT;
			return 1;
		
		default:
			return -1;
		
	}
	i++;
	// options
	for (; i<argc && argv[i][0] == '-'; i++) {
		switch (argv[i][1]) {
			case 'c': // channel selection
				if (argc > i+1 && isdigit(argv[i+1][0])) {
					
					// reset default channel selection
					memset(conf.channels, 0, sizeof(conf.channels));
					
					conf.channels[atoi(&argv[i+1][0])] = 1; // TODO: optimize?
					int j;
					for (j=1; j<18 && argv[i+1][j] == ','; j=j+2){
						conf.channels[atoi(&argv[i+1][j+1])] = 1;
					}													
					i++;
					break;
				} else {
					return -1;
				}
			
			case 'f':
				if (argv[i][2] == 'h') { // force high range
					if (argc > i+1 && isdigit(argv[i+1][0]) && atoi(&argv[i+1][0]) < 3 && atoi(&argv[i+1][0]) > 0) {
						
						// reset default forced channel selection
						memset(conf.force_high_channels, 0, sizeof(conf.force_high_channels));
						
						conf.force_high_channels[atoi(&argv[i+1][0]) - 1] = 1; // TODO: optimize?
						if (argv[i+1][1] == ',') {
							if (atoi(&argv[i+1][2]) < 3 && atoi(&argv[i+1][2]) > 0) {
								conf.force_high_channels[atoi(&argv[i+1][2]) - 1] = 1;
							} else {
								return -1;
							}
						}
						i++;
						break;
					} else {
						return -1;
					}
				} else { // output file
					if (argc > i+1) {
						if (isdigit(argv[i+1][0]) && atoi(&argv[i+1][0]) == 0) { // no file write
							conf.file_format = NO_FILE;
						} else {
							strcpy(conf.file_name, argv[i+1]);
						}
						i++;
						break;
					} else {
						return -1;
					}
				}
			
			case 'r': // acquisition rate
				if (argc > i+1 && isdigit(argv[i+1][0])) {
					int rate = atoi(argv[i+1]);
					if (rate == 1 || rate == 2 || rate == 4 || rate == 8 || rate == 16 || rate == 32 || rate == 64) { //TODO: in rl_lib as well
						conf.sample_rate = rate;
					} else {
						printf("Wrong sample rate.\n");
						return -1;
					}			
					i++;
					break;
				} else {
					return -1;
				}
			
			case 'u': // update rate
				if (argc > i+1 && isdigit(argv[i+1][0])) {
					int update_rate = atoi(argv[i+1]);
					if (update_rate == 1 || update_rate == 2 || update_rate == 5 || update_rate == 10) { //TODO: in rl_lib as well
						conf.update_rate = update_rate;
					} else {
						printf("Wrong update rate.\n");
						return -1;
					}
					i++;
					break;
				} else {
					return -1;
				}
			
			case 'w': // webserver
				conf.enable_web_server = 1;
				break;
			
			case 'b': // binary output file
				conf.file_format = BIN;				
				break;
			
			case 's': // store conf to default
				set_as_default = 1;				
				break;
			
			case 'h': // help
				return -1;
			
			default:
				return -1;
		}
	}
	
	return 1;
}


int main(int argc, char* argv[]) {
	
	// check if root
	if(getuid()!=0){
		printf("You must run this program as root. Exiting.\n");
		return -1;
	}
	
	// get default config
	read_default_config(&conf, DEFAULT_CONFIG);
	
	// parse arguments
	if (parse_args(argc, argv) < 0) {
		print_usage(&conf);
		return -1;
	}
	
	
	// store config as default
	if(set_as_default == 1) {
		write_default_config(&conf, DEFAULT_CONFIG);
	}
	
	switch (conf.mode) {
		case LIMIT:
			if(is_running()) {
				printf("Error: RocketLogger already running\n Run:  rocketlogger stop\n\n");
				return -1;
			}
			print_config(&conf);
			printf("\nStart sampling ...\n");
			rl_sample(&conf);
			break;
			
		case NEW_CONTINUOUS:
			if(is_running()) {
				printf("Error: RocketLogger already running\n Run:  rocketlogger stop\n\n");
				return -1;
			}
			print_config(&conf);
			printf("\nData acquisition running in background ...\n  Stop with:   rocketlogger stop\n\n");
			rl_continuous(&conf);
			break;
		
		case METER:
			if(is_running()) {
				printf("Error: RocketLogger already running\n Run:  rocketlogger stop\n\n");
				return -1;
			}
			rl_meter(&conf);
			break;
		
		case STATUS:
			if(conf.enable_web_server == 1) {
				rl_get_status(1,1);
			} else {
				rl_get_status(1,0);
			}
			break;
		
		case STOPPED:
			if(!is_running()) {
				printf("Error: RocketLogger not running\n");
				return -1;
			}
			rl_stop();
			break;
		
		case DATA:
			if(!is_running()) {
				printf("Error: RocketLogger not running\n");
				return -1;
			}
			rl_get_data();
			break;
		
		case CALIBRATE:
			if(is_running()) {
				printf("Warning: reset will not affect current measurement\n");
			}
			rl_reset_calibration();
			break;
		
		case SET_DEFAULT:
			write_default_config(&conf, DEFAULT_CONFIG);
			print_config(&conf);
			break;
		
		case PRINT_DEFAULT:
			print_config(&conf);
			break;
		
		default:
			print_usage(&conf);
			return -1;
	}
	
	return 1;
	
}