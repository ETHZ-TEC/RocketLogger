#include <stdio.h>
#include <stdlib.h>
#include "rl_lib.h"

struct rl_conf conf = {0, 1, HZ1, 0, ALL,  0, 0, 0, 1, "/var/www/data/data.dat", "all",""};

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
	printf("  Options:\n");
	printf("    -f [file]          Stores data to specified file (default: %s)\n", conf.file);
	printf("                       -f 0 will disable file storing.\n");
	printf("    -r [number]        Acquisition rate selection (default 1kSps).\n");
	printf("                       Possible rates: 1, 2, 4, 8, 16, 32, 64 (in kSps)\n");
	printf("    -u [number]        Data update rate selection (default 1Hz).\n");
	printf("                       Possible update rates: 1, 2, 5, 10 (in Hz)\n");
	printf("    -ch [number1,...]  Channel selection (default: all).\n");
	printf("                       Possible channels:\n");
	printf("                         0: I1H\t\t5: I2H\n");
	printf("                         1: I1M\t\t6: I2M\n");
	printf("                         2: I1L\t\t7: I2L\n");
	printf("                         3: V1 \t\t8: V3\n");
	printf("                         4: V2 \t\t9: V4\n");
	printf("    -fhr [1,2]         Force high-range.\n");
	printf("                       1: I1, 2: I2\n");
	printf("    -w                 Enable webserver plotting.\n");
	printf("    -b                 Set output file to binary.\n");
	
}

int parse_args(int argc, char* argv[]) {
	
	// need at least 2 arguments
	if (argc < 2) {
		print_usage();
		return -1;
	}
	
	int file = 0;
	int i = 1;
	// modes
	switch (argv[i][0]) {
		case 's':
			switch (argv[i][2]) {
				case 'm': // sampling mode
					if (argc > i+1 && isdigit(argv[i+1][0]) && atoi(argv[i+1]) > 0) { 
						conf.state = RL_SAMPLES;
						conf.number_samples = atoi(argv[i+1]);
						i++;
						break;
					} else {
						print_usage();
						return -1;
					}
					break;
				case 'a': // status
					conf.state = RL_STATUS;
					break;
				case 'o': // stop
					conf.state = RL_STOP;
					return 1;
				case 'd': // data
					conf.state = RL_DATA;
					return 1;
				default: 
					print_usage();
					return -1;
			}
			break;
		case 'c':
			if (argv[i][1] == 'o') { // continuous mode
				conf.state = RL_CONTINUOUS;
				break;
			} else {
				conf.state = RL_CALIBRATE;
				break;
			}
			
		case 'm': // meter mode
			conf.state = RL_METER;
			break;
		case 'd': // data
			conf.state = RL_DATA;
			break;
		default:
			print_usage();
			return -1;
		
	}
	i++;
	// options
	for (; i<argc && argv[i][0] == '-'; i++) {
		switch (argv[i][1]) {
			case 'c': // channel selection
				if (argc > i+1 && isdigit(argv[i+1][0])) {
					strcpy(conf.channels_string,argv[i+1]);
					conf.channels = 1 << atoi(&argv[i+1][0]);
					int j;
					for (j=1; j<18 && argv[i+1][j] == ','; j=j+2){
						conf.channels = conf.channels | 1 << atoi(&argv[i+1][j+1]);
					}													
					i++;
					break;
				} else {
					print_usage();
					return -1;
				}
			case 'f':
				if (argv[i][2] == 'h') { // force high range
					if (argc > i+1 && isdigit(argv[i+1][0]) && atoi(&argv[i+1][0]) < 3 && atoi(&argv[i+1][0]) > 0) {
						strcpy(conf.force_high_channels_string,argv[i+1]);
						conf.force_high_channels = atoi(&argv[i+1][0]);
						if (argv[i+1][1] == ',') {
							if (atoi(&argv[i+1][2]) < 3 && atoi(&argv[i+1][2]) > 0) {
								conf.force_high_channels = conf.force_high_channels | atoi(&argv[i+1][2]);
							} else {
								print_usage();
								return -1;
							}
						}
						i++;
						break;
					} else {
						print_usage();
						return -1;
					}
				} else { // output file
					if (argc > i+1) {
						if (isdigit(argv[i+1][0]) && atoi(&argv[i+1][0]) == 0) { // no file write
							conf.store = 0;
						} else {
							strcpy(conf.file, argv[i+1]);
						}
						i++;
						file = 1;
						break;
					} else {
						print_usage();
						return -1;
					}
				}
			case 'r': // acquisition rate
				if (argc > i+1 && isdigit(argv[i+1][0])) {
					int rate = atoi(argv[i+1]);
					if (rate == 1 || rate == 2 || rate == 4 || rate == 8 || rate == 16 || rate == 32 || rate == 64) {
						conf.rate = rate;
					} else {
						printf("Wrong sample rate.\n");
						rl_log("Error: wrong sample rate chosen.\n");
						return -1;
					}			
					i++;
					break;
				} else {
					print_usage();
					return -1;
				}
			case 'u': // update rate
				if (argc > i+1 && isdigit(argv[i+1][0])) {
					int update_rate = atoi(argv[i+1]);
					if (update_rate == 1 || update_rate == 2 || update_rate == 5 || update_rate == 10) {
						conf.update_rate = update_rate;
					} else {
						printf("Wrong update rate.\n");
						rl_log("Error: wrong update rate chosen.\n");
						return -1;
					}
					i++;
					break;
				} else {
					print_usage();
					return -1;
				}
			case 'w': // webserver
				conf.enable_web_server = 1;	
				break;
			case 'b': // binary output file
				conf.binary_file = 1;	
				break;
			case 'h': // help
				print_usage();
				return -1;
			
			default:
				print_usage();
				return -1;
		}
	}
	if (file == 0) { // no file name provided
		if (conf.binary_file == 1) {
			strcpy(conf.file, "/var/www/data/data.dat");
		} else {
			strcpy(conf.file, "/var/www/data/data.csv");
		}
	}
}

void print_header() {
										printf("Starting RocketLogger %s\n",mode_names[conf.state-1]);
	if (conf.state == RL_SAMPLES)		printf("   - Samples:         %d\n",conf.number_samples);
										printf("   - Rate:            %dkSps\n", conf.rate);
										printf("   - Channels:        %s\n", conf.channels_string);
	if (conf.force_high_channels != 0)	printf("   - Forced channels: %s\n", conf.force_high_channels_string);
										printf("   - File:            %s\n", conf.file);
										printf("   - Update rate:     %dHz\n", conf.update_rate);
	if (conf.enable_web_server == 1)	printf("   - Webserver:       enabled\n");
}

int running() {
	// check if RL already running
	if(rl_get_status(0) == 1) {
		printf("Error: RocketLogger already running!\n Run:  rocketlogger stop\n\n");
		rl_log("Error: RocketLogger already running.\n");
		return 1;
	}
	return 0;
}

int main(int argc, char* argv[]) {
	
	// init log file
	rl_log_init();
	rl_log("--- Rocketlogger Log File ---\n");
	
	// check if root
	if(getuid()!=0){
		printf("You must run this program as root. Exiting.\n");
		rl_log("Error: program not running as root.\n");
		return -1;
	}
	
	// parse arguments
	if (parse_args(argc, argv) < 0) {
		return -1;
	}
	rl_log("Info: argument parsed correctly.\n");
	
	// start rocketlogger
	switch (conf.state) {
		case RL_STOP:
			rl_stop();
			break;
		case RL_SAMPLES:
			if(running()) {
				return -1;
			}
			print_header();
			//printf("Press ENTER to stop sampling!\n");
			rl_sample(&conf);
			break;
		case RL_CONTINUOUS:
			if(running()) {
				return -1;
			}
			print_header();
			rl_continuous(&conf);
			break;
		case RL_METER:
			if(running()) {
				return -1;
			}
			rl_meter(&conf);
			break;
		case RL_STATUS:
			if(conf.enable_web_server == 1) {
				rl_get_status_web();
			} else {
				rl_get_status(1);
			}
			break;
		case RL_CALIBRATE:
			if(running()) {
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
	}
	
	return 1;
	
}