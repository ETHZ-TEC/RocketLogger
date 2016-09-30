#include <stdio.h>
#include <stdlib.h>

#include "rl_lib.h"
#include "rl_util.h"

//struct rl_conf conf = {0, 1, HZ1, 0, ALL,  0, 0, 0, 1, "/var/www/data/data.dat", "all",""};
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
							//conf.state = RL_SAMPLES;
							//conf.number_samples = atoi(argv[i+1]);
							
							confn.mode = LIMIT;
							confn.number_samples = atoi(argv[i+1]);
							i++;
							break;
						} else {
							return -1;
						}
						break;
					case 'a': // status
						//conf.state = RL_STATUS;
						confn.mode = STATUS;
						break;
					case 'o': // stop
						//conf.state = RL_STOP;
						confn.mode = STOPPED;
						return 1;
					/*case 'd': // data
						conf.state = RL_DATA;
						// TODO for confn
						return 1;*/
					default: 
						return -1;
				}
			}
			break;
		case 'c':
			if (argv[i][1] == 'o') { // continuous mode
				//conf.state = RL_CONTINUOUS;
				confn.mode = NEW_CONTINUOUS;
				break;
			} else {
				//conf.state = RL_CALIBRATE;
				confn.mode = CALIBRATE;
				break;
			}
			
		case 'm': // meter mode
			//conf.state = RL_METER;
			confn.mode = METER;
			break;
		case 'd': // data
			//conf.state = RL_DATA;
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
					
					//strcpy(conf.channels_string,argv[i+1]);
					//conf.channels = 1 << atoi(&argv[i+1][0]);
					confn.channels[atoi(&argv[i+1][0])] = 1; // TODO: optimize?
					int j;
					for (j=1; j<18 && argv[i+1][j] == ','; j=j+2){
						//conf.channels = conf.channels | 1 << atoi(&argv[i+1][j+1]);
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
						
						//strcpy(conf.force_high_channels_string,argv[i+1]);
						//conf.force_high_channels = atoi(&argv[i+1][0]);
						confn.force_high_channels[atoi(&argv[i+1][0]) - 1] = 1; // TODO: optimize?
						if (argv[i+1][1] == ',') {
							if (atoi(&argv[i+1][2]) < 3 && atoi(&argv[i+1][2]) > 0) {
								//conf.force_high_channels = conf.force_high_channels | atoi(&argv[i+1][2]);
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
							//conf.store = 0;
							confn.file_format = NO_FILE;
						} else {
							//strcpy(conf.file, argv[i+1]);
							strcpy(confn.file_name, argv[i+1]);
						}
						i++;
						//file = 1;
						break;
					} else {
						return -1;
					}
				}
			case 'r': // acquisition rate
				if (argc > i+1 && isdigit(argv[i+1][0])) {
					int rate = atoi(argv[i+1]);
					if (rate == 1 || rate == 2 || rate == 4 || rate == 8 || rate == 16 || rate == 32 || rate == 64) { //TODO: in rl_lib as well
						//conf.rate = rate;
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
						//conf.update_rate = update_rate;
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
				//conf.enable_web_server = 1;	
				confn.enable_web_server = 1;
				break;
			case 'b': // binary output file
				//conf.binary_file = 1;
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
	/*if (file == 0) { // no file name provided
		if (conf.binary_file == 1) {
			strcpy(conf.file, "/var/www/data/data.dat");
		} else {
			strcpy(conf.file, "/var/www/data/data.csv");
		}
	}*/
	return 1;
}

void print_conf() { // TODO: move to rl_util
	printf("RocketLogger Configuration:\n");
	print_config(&confn, 0);
	
	/*									printf("RocketLogger %s Configuration:\n",mode_names[conf.state-1]);
	if (conf.state == RL_SAMPLES)		printf("   - Samples:         %d\n",conf.number_samples);
										printf("   - Rate:            %dkSps\n", conf.rate);
										printf("   - Channels:        %s\n", conf.channels_string);
	if (conf.force_high_channels != 0)	printf("   - Forced channels: %s\n", conf.force_high_channels_string);
										printf("   - File:            %s\n", conf.file);
										printf("   - Update rate:     %dHz\n", conf.update_rate);
	if (conf.enable_web_server == 1)	printf("   - Webserver:       enabled\n");
	
	printf("\n------------------------\n\n");
	
	int i;
										printf("RocketLogger Configuration:\n");
	if (confn.mode == LIMIT) {			printf("   - Samples:         %d\n",confn.number_samples);}
	else {								printf("   - Samples:         no limit\n");}
										printf("   - Rate:            %dkSps\n", confn.sample_rate);
										printf("   - Update rate:     %dHz\n", confn.update_rate);
										printf("   - File:            %s\n", confn.file_name); // TODO: check if no file writing
										// TODO: add file type
	if (confn.enable_web_server == 1)	printf("   - Webserver:       enabled\n");
	
	// channels
										printf("   - Channels:        ");
	for(i=0; i<NUM_CHANNELS; i++) {
		if (confn.channels[i] > 0) {
			printf("%d,", i);
		}
	}
	printf("\n");
	
	// forced channels
	if (confn.force_high_channels[0] > 0 || confn.force_high_channels[1] > 0) {
										printf("   - Forced channels: ");
		for(i=0; i<NUM_I_CHANNELS; i++) {
			if (confn.force_high_channels[i] > 0) {
				printf("%d,", i+1);
			}
		}
		printf("\n");
	}*/
	
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
			print_conf();
			printf("\nStart sampling ...\n");
			rl_sample(&confn);
			break;
			
		case NEW_CONTINUOUS:
			if(is_running()) {
				printf("Error: RocketLogger already running\n Run:  rocketlogger stop\n\n");
				return -1;
			}
			print_conf();
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
			print_conf();
			break;
		
		case PRINT_DEFAULT:
			print_conf();
			break;
		
		default:
			print_usage();
			return -1;
	}
	
	// start rocketlogger
/*	switch (conf.state) {
		case RL_STOP:
			rl_stop();
			break;
		case RL_SAMPLES:
			if(is_running()) {
				return -1;
			}
			print_conf();
			printf("\nStart sampling ...\n");
			rl_sample(&confn);
			break;
		case RL_CONTINUOUS:
			if(is_running()) {
				return -1;
			}
			print_conf();
			printf("\n");
			rl_continuous(&confn);
			break;
		case RL_METER:
			if(is_running()) {
				return -1;
			}
			rl_meter(&confn);
			break;
		case RL_STATUS:
			if(confn.enable_web_server == 1) {
				rl_get_status(1,1);
			} else {
				rl_get_status(1,0);
			}
			break;
		case RL_CALIBRATE:
			if(is_running()) {
				return -1;
			}
			rl_reset_calibration();
			break;
		case RL_DATA:
			rl_get_data();
			break;
		default:
			print_usage();
			return -1;
	}*/
	
	return 1;
	
}