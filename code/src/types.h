#ifndef RL_TYPES_H
#define RL_TYPES_H


// files
#define CONFIG_FILE "/var/run/rocketlogger.conf"
#define PID_FILE "/var/run/rocketlogger.pid"
#define STATUS_FILE "/var/run/rocketlogger.stat"

// TODO: try to remove
#define MAP_SIZE 0x0FFFFFFF
#define MAP_MASK (MAP_SIZE - 1)

#define MAX_PATH_LENGTH 100

#define NUM_CHANNELS 10
#define NUM_I_CHANNELS 2
#define NUM_TOT_I_CHANNELS 6
#define NUM_V_CHANNELS 4

enum rl_state {NEW_OFF, NEW_RUNNING, ERROR};
enum rl_mode {LIMIT, CONTINUOUS, METER, STATUS, STOPPED, DATA, CALIBRATE, SET_DEFAULT, PRINT_DEFAULT};
enum rl_file_format {NO_FILE, CSV, BIN};

struct rl_conf {
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

struct rl_status status;

// TODO: combine
int offsets16[NUM_CHANNELS];
int offsets24[NUM_CHANNELS];

double scales24[NUM_CHANNELS];
double scales16[NUM_CHANNELS];


#endif