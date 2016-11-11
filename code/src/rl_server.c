#include "sem.h"
#include "types.h"
#include "rl_lib.h"
#include "web.h"
#include "util.h"
#include "rl_util.h"

#define ARG_COUNT 4
#define MAX_STRING_LENGTH 150
#define MAX_STRING_VALUE 20

// Global variables
int sem_id;
struct web_shm* web_data;

// buffer sizes
int buffer_sizes[WEB_RING_BUFFER_COUNT] = {BUFFER1_SIZE, BUFFER10_SIZE, BUFFER100_SIZE};

// functions
void print_json_new(int32_t data[], int length) {
	char str[MAX_STRING_LENGTH];
	char val[MAX_STRING_VALUE];
	int i;
	sprintf(str, "[\"%d\"", data[0]);
	for (i=1; i < length; i++) {
		sprintf(val, ",\"%d\"", data[i]);
		strcat(str, val);
	}
	strcat(str, "]\n");
	printf("%s",str);
}

void print_status(struct rl_status* status) {
	printf("%d\n", status->state);
	if(status->state != RL_RUNNING) {
		read_default_config(&status->conf);
	}
	printf("%d\n", status->conf.sample_rate);
	printf("%d\n", status->conf.update_rate);
	printf("%d\n", status->conf.digital_inputs);
	printf("%d\n", status->conf.file_format);
	printf("%s\n", status->conf.file_name);
	print_json_new(status->conf.channels, NUM_CHANNELS);
	print_json_new(status->conf.force_high_channels, NUM_I_CHANNELS);
	printf("%d\n", status->samples_taken);
	printf("%d\n", status->conf.enable_web_server);
		
}


void print_data(uint32_t t_scale, int64_t time, int64_t last_time, int8_t num_channels) {
	
	// print time
	printf("%lld\n", time);
	
	// print data length
	int buffer_count = time - last_time;
	
	// get available buffers
	if(wait_sem(sem_id, DATA_SEM, SEM_TIME_OUT) != SUCCESS) {
		return;
	}
	int buffer_available = web_data->buffer[t_scale].filled;
	set_sem(sem_id, DATA_SEM, 1);
		
	if(buffer_count > buffer_available) {
		buffer_count = buffer_available;
	}
	
	int buffer_size = buffer_sizes[t_scale];
	
	printf("%d\n", buffer_count);
	printf("%d\n", buffer_size);
	
	// print data
	int32_t data[buffer_size][num_channels];
	int i;
	for(i=buffer_count-1; i>=0; i--) {
		
		// read data
		if(wait_sem(sem_id, DATA_SEM, SEM_TIME_OUT) != SUCCESS) {
			return;
		}
		int32_t* shm_data = buffer_get(&web_data->buffer[t_scale], i);
		if(web_data->buffer[t_scale].element_size > sizeof(data)) {
			rl_log(ERROR, "In print_data: memcpy is trying to copy to much data.");
		} else {
			memcpy(&data[0][0], shm_data, web_data->buffer[t_scale].element_size);
		}
		set_sem(sem_id, DATA_SEM, 1);
		
		// print data
		// TODO: move this to separate loop, without semaphore lock
		int j;
		for(j=0; j<buffer_size; j++) {
			print_json_new(data[j], num_channels);
		}
	}
}


int main(int argc, char* argv[]) {
	
	// parse arguments
	if(argc != ARG_COUNT + 1) {
		rl_log(ERROR, "in rl_server: not enough arguments");
		exit(FAILURE);
	}
	uint32_t id = atoi(argv[1]);
	uint8_t get_data = atoi(argv[2]);
	uint32_t t_scale = atoi(argv[3]);
	int64_t last_time = atoi(argv[4]);
	
	// check time scale
	if(t_scale != S1 && t_scale != S10 && t_scale != S100) {
		rl_log(WARNING, "unknown time scale");
		t_scale = S1;
	}
	
	// get status
	struct rl_status status;
	int state = rl_read_status(&status);
	
	// print request id and status
	printf("%d\n", id);
	print_status(&status);
	
	// only get data, if requested and running and web enabled
	if(state != RL_RUNNING || status.conf.enable_web_server == 0 || get_data == 0) {
		exit(EXIT_SUCCESS);
		printf("0\n");
	}
	printf("1\n");
	
	// print time scale
	printf("%d\n", t_scale);
	
	// open semaphore
	sem_id = open_sem();
	if(sem_id < 0) {
		// error already logged
		exit(EXIT_FAILURE);
	}
	
	// open shared memory
	web_data = open_web_shm();
	
	// fetch data
	uint8_t data_read = 0;
	while(data_read == 0) {
		
		// get current time
		if(wait_sem(sem_id, DATA_SEM, SEM_TIME_OUT) != SUCCESS) {	
			exit(EXIT_FAILURE);
		}
		int64_t time = web_data->time;
		int8_t num_channels = web_data->num_channels;
		set_sem(sem_id, DATA_SEM, 1);
		
		
		if(time > last_time) {
			
			// read and print data
			print_data(t_scale, time, last_time, num_channels);
			
			data_read = 1;
		} else {
			
			if(last_time > time) {
				// assume outdated time stamp
				last_time = 0;
			}
			
			// wait on new data
			if(data_read == 0) {
				if(wait_sem(sem_id, WAIT_SEM, SEM_TIME_OUT) != SUCCESS) {
					// time-out or error -> stop
					break;
				}
			}
		}
	}
	
	// unmap shared memory
	shmdt(web_data);
	
	exit(EXIT_SUCCESS);
}
