/*#include <stdio.h>
#include <stdlib.h>*/

#include "rl_lib.h"
#include "rl_util.h"



int main(int argc, char* argv[]) {
	
	struct rl_conf conf;
	int set_as_default;
	
	// check if root
	if(getuid() != 0){
		rl_log(ERROR, "you must run this program as root");
		return FAILURE;
	}
	
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
			if(rl_get_status(0,0) == RL_RUNNING) {
				rl_log(ERROR, "RocketLogger already running\n Run:  rocketlogger stop\n");
				exit(EXIT_FAILURE);
			}
			print_config(&conf);
			printf("Start sampling ...\n");
			break;
			
		case CONTINUOUS:
			if(rl_get_status(0,0) == RL_RUNNING) {
				rl_log(ERROR, "RocketLogger already running\n Run:  rocketlogger stop\n");
				exit(EXIT_FAILURE);
			}
			print_config(&conf);
			printf("Data acquisition running in background ...\n  Stop with:   rocketlogger stop\n\n");
			break;
		
		case METER:
			if(rl_get_status(0,0) == RL_RUNNING) {
				rl_log(ERROR, "RocketLogger already running\n Run:  rocketlogger stop\n");
				exit(EXIT_FAILURE);
			}
			break;
		
		case STATUS:
			rl_get_status(1,conf.enable_web_server);
			exit(EXIT_SUCCESS);
		
		case STOPPED:
			if(rl_get_status(0,0) != RL_RUNNING) {
				rl_log(ERROR, "RocketLogger not running");
				exit(EXIT_FAILURE);
			}
			printf("Stopping RocketLogger ...\n");
			rl_stop();
			exit(EXIT_SUCCESS);
		
		case DATA:
			if(rl_get_status(0,0) != RL_RUNNING) {
				rl_log(ERROR, "RocketLogger not running");
				exit(EXIT_FAILURE);
			}
			rl_get_data();
			exit(EXIT_SUCCESS);
		
		case CALIBRATE:
			if(rl_get_status(0,0) == RL_RUNNING) {
				printf("\n");
				rl_log(WARNING, "reset will not affect current measurement");
			}
			rl_reset_calibration();
			exit(EXIT_SUCCESS);
		
		case SET_DEFAULT:
			write_default_config(&conf);
			if(rl_get_status(0,0) == RL_RUNNING) {
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
	rl_sample(&conf);
		
	exit(EXIT_SUCCESS);
	
}