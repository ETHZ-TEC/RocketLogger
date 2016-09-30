#ifndef RL_TYPES_H
#define RL_TYPES_H

// TODO: try to remove
#define MAP_SIZE 0x0FFFFFFF
#define MAP_MASK (MAP_SIZE - 1)

#define MAX_PATH_LENGTH 100

#define NUM_CHANNELS 10
#define NUM_I_CHANNELS 2
#define NUM_TOT_I_CHANNELS 6
#define NUM_V_CHANNELS 4

// ---------------------------------------------- RL STATES ---------------------------------------------------------// 

/*// TODO remove
#define RL_SAMPLES 1
#define RL_CONTINUOUS 2
#define RL_METER 3
#define RL_STOP 4
#define RL_STATUS 5
#define RL_CALIBRATE 6
#define RL_DATA 7*/

// ---------------------------------------------- Conf Struct ------------------------------------------------------// 

// TODO remove
/*struct rl_conf {
	int state;
	int rate;
	int update_rate;
	int number_samples;
	int channels;
	int force_high_channels;
	int enable_web_server;
	int binary_file;
	int store;
	char file[MAX_PATH_LENGTH];
	char channels_string[19];
	char force_high_channels_string[3];
};*/

enum rl_state {NEW_OFF, NEW_RUNNING, ERROR};
enum rl_mode {IDLE, LIMIT, NEW_CONTINUOUS, METER, STATUS, STOPPED, DATA, CALIBRATE, SET_DEFAULT, PRINT_DEFAULT}; // TODO: change NEW_CONTINUOUS
enum rl_file_format {NO_FILE, CSV, BIN};

struct rl_conf_new {
	enum rl_mode mode;
	int sample_rate;
	int update_rate;
	int number_samples;
	int channels[NUM_CHANNELS];
	int force_high_channels[NUM_I_CHANNELS];
	int enable_web_server;
	enum rl_file_format file_format;
	char file_name[MAX_PATH_LENGTH];
};

struct rl_status {
	enum rl_state state;
	int samples_taken;
	int buffer_number;
};


// ----- GLOBAL VARIABLES ----- //

//int running;

struct rl_status status;

// TODO: combine
int offsets16[NUM_CHANNELS];
int offsets24[NUM_CHANNELS];

double scales24[NUM_CHANNELS];
double scales16[NUM_CHANNELS];


#endif