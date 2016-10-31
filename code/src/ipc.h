#ifndef IPC_H
#define IPC_H

#include "types.h"
#include "util.h"

#define WEB_RING_BUFFER_COUNT 4
#define WEB_BUFFER_ELEMENTS 10

struct ringbuffer {
	uint32_t size;
	uint32_t filled;
	uint32_t head;
	int32_t data[NUM_WEB_CHANNELS * WEB_BUFFER_SIZE * WEB_BUFFER_ELEMENTS];
};

struct web_shm {
	int64_t time;
	struct ringbuffer buffer[WEB_RING_BUFFER_COUNT];
};



struct web_shm* create_web_shm();
struct web_shm* open_web_shm();

void reset_buffer(struct ringbuffer* buffer, int size);

void buffer_add(struct ringbuffer* buffer, int32_t* data);

int32_t* buffer_get(struct ringbuffer* buffer, int num);


#endif