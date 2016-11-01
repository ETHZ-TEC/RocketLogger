#include "sem.h"
#include "types.h"
#include "rl_lib.h"

#define ARG_COUNT 4

/// RL struct for web request (unused)
struct rl_web_req {
	uint32_t id;
	uint8_t get_data;
	uint8_t t_scale;
	int64_t last_time;
};

/// RL struct for web response
struct rl_web_resp {
	uint32_t id;
	uint8_t t_scale;
	int64_t time;
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
	int64_t last_time = atoi(argv[4]);
	
	//struct rl_web_req req = {id, get_data, t_scale, last_time};
	//struct rl_web_resp resp;
	
	
	// get and print status
	printf("%d\n", id);
	int state =  rl_get_status(1, 1);
	
	// only get data, if requested and running
	// TODO: and only if web enabled
	if(state != RL_RUNNING || get_data == 0) {
		exit(EXIT_SUCCESS);
	}
	
	// open semaphore
	int sem_id = open_sem();
	if(sem_id < 0) {
		// error already logged
		exit(EXIT_FAILURE);
	}
	
	// open shared memory (TODO: function)
	int shm_id = shmget(SHMEM_DATA_KEY, sizeof(uint32_t), SHMEM_PERMISSIONS);
	if (shm_id == -1) {
		rl_log(ERROR, "In read_status: failed to get shared status memory id; %d message: %s", errno, strerror(errno));
		return FAILURE;
	}
	int64_t* web_data = (int64_t*) shmat(shm_id, NULL, 0);
	
	if (web_data == (void *) -1) {
		rl_log(ERROR, "In pru_sample: failed to map shared status memory; %d message: %s", errno, strerror(errno));
		return FAILURE;
	}
	
	
	uint8_t data_read = 0;
	
	while(data_read == 0) {
		
		// get current time
		wait_sem(sem_id, DATA_SEM, SEM_TIME_OUT);
		int64_t time = *web_data;
		set_sem(sem_id, DATA_SEM, 1);
		
		if(time > last_time) {
			
			// TODO read data last_time
			printf("reading data\n");
			
			data_read = 1;
		} else {
			
			if(last_time > time) {
				// assume outdated time stamp
				last_time = 0;
			}
			
			printf("waiting on data\n");
			// wait on new data
			if(data_read == 0) {
				if(wait_sem(sem_id, WAIT_SEM, SEM_TIME_OUT) != SUCCESS) {
					// time-out or error
					break;
				}
			}
		}
	}
	
	// send data
	
	// unmap shared memory
	shmdt(web_data);
	
	exit(EXIT_SUCCESS);
}