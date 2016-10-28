#include "sem.h"
#include "types.h"
#include "rl_lib.h"

#define ARG_COUNT 4

/// RL struct for web request (unused)
struct rl_web_req {
	uint32_t id;
	uint8_t get_data;
	uint8_t t_scale;
	uint32_t last_time;
};

/// RL struct for web response
struct rl_web_resp {
	uint32_t id;
	uint8_t t_scale;
	uint32_t time;
	uint32_t data_length;
	float* data;
};

int main(int argc, char* argv[]) {
	
	// parse arguments
	
	if(argc != ARG_COUNT + 1) {
		// TODO: log
		exit(FAILURE);
	}
	
	// TODO: check if numbers
	uint32_t id = atoi(argv[1]);
	uint8_t get_data = atoi(argv[2]);
	uint32_t t_scale = atoi(argv[3]);
	uint32_t last_time = atoi(argv[4]);
	
	//struct rl_web_req req = {id, get_data, t_scale, last_time};
	//struct rl_web_resp resp;
	
	
	// get and print status
	printf("%d\n", id);
	int state =  rl_get_status(1, 1);
	
	// only get data, if requested and running
	if(state != RL_RUNNING || get_data == 0) {
		exit(EXIT_SUCCESS);
	}
	
	// open semaphore
	int sem_id = open_sem();
	if(sem_id < 0) {
		// error already logged
		exit(EXIT_FAILURE);
	}
	
	// get current time
	wait_sem(sem_id, DATA_SEM, SEM_TIME_OUT);
	// TODO: read time
	uint32_t time = 0;
	uint8_t data_read = 0;
	if(time > last_time) {
		// TODO read data last_time
		
		data_read = 1;
	}
	// TODO: set_sem
	
	// wait on new data
	if(data_read == 0) {
		wait_sem(sem_id, WAIT_SEM, SEM_TIME_OUT);
	}
	
	exit(EXIT_SUCCESS);
}