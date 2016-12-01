#include "web.h"

/**
 * Create shared memory for data exchange with web server
 * @return pointer to shared memory, NULL in case of failure
 */
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

/**
 * Open existing shared memory for data exchange with web server
 * @return pointer to shared memory, NULL in case of failure
 */
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

/**
 * Reset web data ring buffer
 * @param buffer Pointer to ring buffer to reset
 * @param element_size Desired element size in bytes
 * @param length Buffer length in elements
 */
void reset_buffer(struct ringbuffer* buffer, int element_size, int length) {
	buffer->element_size = element_size;
	buffer->length = length;
	buffer->filled = 0;
	buffer->head = 0;
}

/**
 * Add element to ring buffer
 * @param buffer Pointer to ring buffer
 * @param data Pointer to data array to add
 */
void buffer_add(struct ringbuffer* buffer, int64_t* data) {
	memcpy((buffer->data) + buffer->head*buffer->element_size/sizeof(int64_t), data, buffer->element_size);
	if(buffer->filled < buffer->length) {
		buffer->filled++;
	}
	buffer->head = (buffer->head + 1) % buffer->length;
	
}

/**
 * Get pointer to a specific element of a ringbuffer
 * @param buffer Pointer to ring buffer
 * @param num Element number (0 corresponds to the newest element)
 * @return pointer to desired element
 */
int64_t* buffer_get(struct ringbuffer* buffer, int num) {
	int pos = ((int)buffer->head + (int)buffer->length - 1 - num) % (int)buffer->length;
	
	return buffer->data + pos*buffer->element_size/sizeof(int64_t);
}
