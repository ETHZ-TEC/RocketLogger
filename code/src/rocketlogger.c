#include <stdio.h>
#include <stdlib.h>

#include "rl_lib.h"
#include "rl_util.h"

struct rl_conf_new confn;

int set_as_default;

char mode_names[2][11] = {"Sample","Continuous"};

void print_usage() {
	printf("Usage:\n");
	printf("  rocketlogger [mode] -[option] [value]\n\n");
	printf("  Modes:\n");
	printf("    sample [number]    Acquires [number] of samples.\n");
	printf("    continuous         Continuously acquires samples.\n");
	printf("    meter              Starts RocketLogger Meter.\n");
	printf("    status             Get status of RocketLogger.\n");
	printf("    calibrate          Reset RocketLogger calibration.\n");
	printf("    data               Get current data.\n");
	printf("    stop               Stops RocketLogger.\n");
	printf("    set                Set default configuration of RocketLogger (use normal options).\n");
	printf("    print              Print default configuration of RocketLogger.\n");
	
	printf("  Options:\n");
	printf("    -f [file]          Stores data to specified file (default: %s)\n", confn.file_name);
	printf("                       -f 0 will disable file storing.\n");
	printf("    -r [number]        Acquisition rate selection (default %dkSps).\n", confn.sample_rate);
	printf("                       Possible rates: 1, 2, 4, 8, 16, 32, 64 (in kSps)\n");
	printf("    -u [number]        Data update rate selection (default %dHz).\n", confn.update_rate);
	printf("                       Possible update rates: 1, 2, 5, 10 (in Hz)\n");
	printf("    -ch [number1,...]  Channel selection (default: TODO).\n"); // TODO: print channels
	printf("                       Possible channels:\n");
	printf("                         0: I1H\t\t5: I2H\n");
	printf("                         1: I1M\t\t6: I2M\n");
	printf("                         2: I1L\t\t7: I2L\n");
	printf("                         3: V1 \t\t8: V3\n");
	printf("                         4: V2 \t\t9: V4\n");
	printf("    -fhr [1,2]         Force high-range.\n"); // TODO: add 0 to reset fhr
	printf("                       1: I1, 2: I2\n");
	printf("    -w                 Enable webserver plotting.\n"); // TODO: allow disable
	printf("    -b                 Set output file to binary.\n"); // TODO: expand selection
	printf("    -s                 Set configuration as default.\n");
	
}

int parse_args(int argc, char* argv[]) {
	
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
				confn.mode = SET_DEFAULT;
				break;
			} else {
				switch (argv[i][2]) {
					case 'm': // sampling mode
						if (argc > i+1 && isdigit(argv[i+1][0]) && atoi(argv[i+1]) > 0) { 
							
							confn.mode = LIMIT;
							confn.number_samples = atoi(argv[i+1]);
							i++;
							break;
						} else {
							return -1;
						}
						break;
					
					case 'a': // status
						confn.mode = STATUS;
						break;
					
					case 'o': // stop
						confn.mode = STOPPED;
						return 1;
					
					default: 
						return -1;
				}
			}
			break;
		case 'c':
			if (argv[i][1] == 'o') { // continuous mode
				confn.mode = NEW_CONTINUOUS;
				break;
			
			} else {
				confn.mode = CALIBRATE;
				break;
			}
			
		case 'm': // meter mode
			confn.mode = METER;
			break;
		
		case 'd': // data
			confn.mode = DATA;
			return 1;
		
		case 'p': // print default
			confn.mode = PRINT_DEFAULT;
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
					memset(confn.channels, 0, sizeof(confn.channels));
					
					confn.channels[atoi(&argv[i+1][0])] = 1; // TODO: optimize?
					int j;
					for (j=1; j<18 && argv[i+1][j] == ','; j=j+2){
						confn.channels[atoi(&argv[i+1][j+1])] = 1;
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
						memset(confn.force_high_channels, 0, sizeof(confn.force_high_channels));
						
						confn.force_high_channels[atoi(&argv[i+1][0]) - 1] = 1; // TODO: optimize?
						if (argv[i+1][1] == ',') {
							if (atoi(&argv[i+1][2]) < 3 && atoi(&argv[i+1][2]) > 0) {
								confn.force_high_channels[atoi(&argv[i+1][2]) - 1] = 1;
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
							confn.file_format = NO_FILE;
						} else {
							strcpy(confn.file_name, argv[i+1]);
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
						confn.sample_rate = rate;
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
						confn.update_rate = update_rate;
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
				confn.enable_web_server = 1;
				break;
			
			case 'b': // binary output file
				confn.file_format = BIN;				
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

void print_config() {
	printf("\nRocketLogger Configuration:\n");
	rl_print_config(&confn, 0);
	
}

int is_running() {
	// check if RL already running
	if(rl_get_status(0,0) == 1) {
		return 1;
	}
	return 0;
}

int main(int argc, char* argv[]) {
	
	// check if root
	if(getuid()!=0){
		printf("You must run this program as root. Exiting.\n");
		return -1;
	}
	
	// get default config
	read_default_config(&confn, DEFAULT_CONFIG);
	
	// parse arguments
	if (parse_args(argc, argv) < 0) {
		print_usage();
		return -1;
	}
	
	
	// store config as default
	if(set_as_default == 1) {
		write_default_config(&confn, DEFAULT_CONFIG);
	}
	
	switch (confn.mode) {
		case LIMIT:
			if(is_running()) {
				printf("Error: RocketLogger already running\n Run:  rocketlogger stop\n\n");
				return -1;
			}
			print_config();
			printf("\nStart sampling ...\n");
			rl_sample(&confn);
			break;
			
		case NEW_CONTINUOUS:
			if(is_running()) {
				printf("Error: RocketLogger already running\n Run:  rocketlogger stop\n\n");
				return -1;
			}
			print_config();
			printf("\n"); // TODO: all user communication in here?
			rl_continuous(&confn);
			break;
		
		case METER:
			if(is_running()) {
				printf("Error: RocketLogger already running\n Run:  rocketlogger stop\n\n");
				return -1;
			}
			rl_meter(&confn);
			break;
		
		case STATUS:
			if(confn.enable_web_server == 1) {
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
			write_default_config(&confn, DEFAULT_CONFIG);
			print_config();
			break;
		
		case PRINT_DEFAULT:
			print_config();
			break;
		
		default:
			print_usage();
			return -1;
	}
	
	return 1;
	
}