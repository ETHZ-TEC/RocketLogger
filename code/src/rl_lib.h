#ifndef RL_LIB_H
#define RL_LIB_H

#include "rl_low_level.h"

// ---------------------------------------------- UPDATE RATES ------------------------------------------------------// 

#define HZ1 1
#define HZ2 2
#define HZ5 5
#define HZ10 10

// ---------------------------------------------- RL STATES ---------------------------------------------------------// 

#define RL_SAMPLES 1
#define RL_CONTINUOUS 2
#define RL_METER 3
#define RL_STOP 4
#define RL_STATUS 5
#define RL_CALIBRATE 6
#define RL_DATA 7

// ---------------------------------------------- Conf Struct ------------------------------------------------------// 

struct rl_conf {
	int state;
	int rate;
	int update_rate;
	int number_samples;
	int channels;
	int force_high_channels;
	int enable_web_server;
	int binary_file;
	int store;
	char file[100];
	char channels_string[19];
	char force_high_channels_string[3];
};

// ---------------------------------------------- Functions ---------------------------------------------------------// 

void rl_reset_calibration();

int rl_get_data();

void print_json(float data[], int length);

int rl_get_status(int print);
int rl_get_status_web();

int rl_sample(struct rl_conf* conf);
int rl_continuous(struct rl_conf* conf);
int rl_meter(struct rl_conf* conf);

int rl_stop();

#endif