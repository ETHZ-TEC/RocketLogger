#define MAX_PATH_LENGTH 100

enum rl_state {OFF, ON, ERROR};
enum rl_mode {LIMIT, CONTINUOUS, METER};
enum rl_file_format {NO_FILE, CSV, BIN};

struct rl_conf {
	enum rl_mode mode;
	int sample_rate;
	int update_rate;
	int number_samples;
	int channels[10];
	int force_high_channels[2];
	int enable_web_server;
	enum rl_file_format file_format;
	char file_name[100];
};

struct rl_status {
	enum rl_state state;
	int samples_taken;
	int buffer_number;
};