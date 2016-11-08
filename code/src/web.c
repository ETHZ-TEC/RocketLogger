#include "web.h"

struct web_shm* create_web_shm() {

	int shm_id = shmget(SHMEM_DATA_KEY, sizeof(struct web_shm), IPC_CREAT | SHMEM_PERMISSIONS);
	if (shm_id == -1) {
		rl_log(ERROR, "In create_web_shm: failed to get shared data memory id; %d message: %s", errno, strerror(errno));
		return NULL;
	}
	struct web_shm* web_data = (struct web_shm*) shmat(shm_id, NULL, 0);

	if (web_data == (void *) -1) {
		rl_log(ERROR, "In create_web_shm: failed to map shared data memory; %d message: %s", errno, strerror(errno));
		return NULL;
	}
	
	return web_data;
}

struct web_shm* open_web_shm() {

	int shm_id = shmget(SHMEM_DATA_KEY, sizeof(struct web_shm), SHMEM_PERMISSIONS);
	if (shm_id == -1) {
		rl_log(ERROR, "In create_web_shm: failed to get shared data memory id; %d message: %s", errno, strerror(errno));
		return NULL;
	}
	struct web_shm* web_data = (struct web_shm*) shmat(shm_id, NULL, 0);

	if (web_data == (void *) -1) {
		rl_log(ERROR, "In create_web_shm: failed to map shared data memory; %d message: %s", errno, strerror(errno));
		return NULL;
	}
	
	return web_data;
}


void reset_buffer(struct ringbuffer* buffer, int element_size, int length) {
	//int i;
	//for(i=0; i<WEB_RING_BUFFER_COUNT; i++) {
		buffer->element_size = element_size;
		buffer->length = length;
		buffer->filled = 0;
		buffer->head = 0;
	//}
}

void buffer_add(struct ringbuffer* buffer, int32_t* data) {
	memcpy((buffer->data) + buffer->head*buffer->element_size/sizeof(int32_t), data, buffer->element_size);
	if(buffer->filled < buffer->length) {
		buffer->filled++;
	}
	buffer->head = (buffer->head + 1) % buffer->length;
	
}

int32_t* buffer_get(struct ringbuffer* buffer, int num) {
	int pos = ((int)buffer->head + (int)buffer->length - 1 - num) % (int)buffer->length;
	
	return buffer->data + pos*buffer->element_size/sizeof(int32_t);
}
