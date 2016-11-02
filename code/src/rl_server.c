#include "sem.h"
#include "types.h"
#include "rl_lib.h"
#include "ipc.h"
#include "util.h"

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

// Global variables
int sem_id;
struct web_shm* web_data;

// functions

void print_json_new(int32_t data[], int length) {
	char str[150]; // TODO: adjustable length
	char val[20];
	int i;
	sprintf(str, "[\"%d\"", data[0]);
	for (i=1; i < length; i++) {
		sprintf(val, ",\"%d\"", data[i]);
		strcat(str, val);
	}
	strcat(str, "]\n");
	printf(str);
}

void print_status(struct rl_status* status) {
	printf("%d\n", status->state);
	if(status->state == RL_RUNNING) {
		printf("%d\n", status->conf.sample_rate);
		printf("%d\n", status->conf.update_rate);
		printf("%d\n", status->conf.digital_inputs);
		printf("%d\n", status->conf.file_format);
		printf("%s\n", status->conf.file_name);
		print_json_new(status->conf.channels, NUM_CHANNELS);
		print_json_new(status->conf.force_high_channels, NUM_I_CHANNELS);
		printf("%d\n", status->samples_taken);
		printf("%d\n", status->conf.enable_web_server);
		
		// TODO: print channels for plot
	}
		
}


void print_data(uint32_t t_scale, int64_t time, int64_t last_time, int8_t num_channels) {
	
	// print time
	printf("%lld\n", time);
	
	// print data length
	int data_length = time - last_time;
	if(data_length > WEB_BUFFER_ELEMENTS) {
		data_length = WEB_BUFFER_ELEMENTS;
	}
	
	// get available buffers
	wait_sem(sem_id, DATA_SEM, SEM_TIME_OUT);
	int buffer_available = web_data->buffer[t_scale].filled;
	set_sem(sem_id, DATA_SEM, 1);
	if(data_length > buffer_available) {
		data_length = buffer_available;
	}
	
	printf("%d\n", data_length);
	
	// print data
	int32_t data[WEB_BUFFER_SIZE][num_channels];
	int i;
	for(i=0; i<data_length; i++) {
		// read data
		wait_sem(sem_id, DATA_SEM, SEM_TIME_OUT);
		int32_t* shm_data = buffer_get(&web_data->buffer[t_scale], i);
		memcpy(&data[0][0], shm_data, web_data->buffer[t_scale].size);
		set_sem(sem_id, DATA_SEM, 1);
		
		// print data
		int j;
		for(j=0; j<WEB_BUFFER_SIZE; j++) {
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
	
	// TODO: check if numbers
	uint32_t id = atoi(argv[1]);
	uint8_t get_data = atoi(argv[2]);
	uint32_t t_scale = atoi(argv[3]);
	int64_t last_time = atoi(argv[4]);
	
	// TODO: expand for multiple time scales
	if (t_scale != 0) {
		rl_log(WARNING, "only time scale 0 implemented");
		t_scale = 0;
	}
	
	
	// get status
	struct rl_status status;
	int state = rl_read_status(&status);
	
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
	
	// determine number of channels
	/*int num_channels = count_channels(status.conf.channels);
	if(status.conf.channels[I1H_INDEX] > 0 && status.conf.channels[I1L_INDEX] > 0) {
		num_channels--;
	}
	if(status.conf.channels[I2H_INDEX] > 0 && status.conf.channels[I2L_INDEX] > 0) {
		num_channels--;
	}*/
	
	// open shared memory
	web_data = open_web_shm();
	
	uint8_t data_read = 0;
	
	while(data_read == 0) {
		
		// get current time
		wait_sem(sem_id, DATA_SEM, SEM_TIME_OUT);
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
					// time-out or error
					// TODO: exit failure?
					break;
				}
			}
		}
	}
	
	// unmap shared memory
	shmdt(web_data);
	
	exit(EXIT_SUCCESS);
}