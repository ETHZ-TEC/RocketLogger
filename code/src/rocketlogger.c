#include <stdio.h>
#include <stdlib.h>

#include "rl_lib.h"
#include "rl_util.h"



int main(int argc, char* argv[]) {
	
	struct rl_conf conf;
	int set_as_default;
	
	// get default config
	read_default_config(&conf);
	
	// parse arguments
	if (parse_args(argc, argv, &conf, &set_as_default) == FAILURE) {
		print_usage(&conf);
		exit(EXIT_FAILURE);
	}
	
	// store config as default
	if(set_as_default == 1) {
		write_default_config(&conf);
	}
	
	switch (conf.mode) {
		case LIMIT:
			if(rl_get_status() == RL_RUNNING) {
				rl_log(ERROR, "RocketLogger already running\n Run:  rocketlogger stop\n");
				exit(EXIT_FAILURE);
			}
			print_config(&conf);
			printf("Start sampling ...\n");
			break;
			
		case CONTINUOUS:
			if(rl_get_status() == RL_RUNNING) {
				rl_log(ERROR, "RocketLogger already running\n Run:  rocketlogger stop\n");
				exit(EXIT_FAILURE);
			}
			print_config(&conf);
			printf("Data acquisition running in background ...\n  Stop with:   rocketlogger stop\n\n");
			break;
		
		case METER:
			if(rl_get_status() == RL_RUNNING) {
				rl_log(ERROR, "RocketLogger already running\n Run:  rocketlogger stop\n");
				exit(EXIT_FAILURE);
			}
			break;
		
		case STATUS: {
			struct rl_status status;
			rl_read_status(&status);
			rl_print_status(&status);
			return status.state;
		}
		
		case STOPPED:
			if(rl_get_status() != RL_RUNNING) {
				rl_log(ERROR, "RocketLogger not running");
				exit(EXIT_FAILURE);
			}
			printf("Stopping RocketLogger ...\n");
			rl_stop();
			exit(EXIT_SUCCESS);
		
		case CALIBRATE:
			rl_log(ERROR, "no calibration implemented");
			exit(EXIT_FAILURE);
		
		case SET_DEFAULT:
			write_default_config(&conf);
			if(rl_get_status() == RL_RUNNING) {
				printf("\n");
				rl_log(WARNING, "change will not affect current measurement");
			}
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
	rl_start(&conf);
		
	exit(EXIT_SUCCESS);
	
}
