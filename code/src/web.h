#ifndef IPC_H
#define IPC_H

#include "types.h"
#include "util.h"

#define WEB_RING_BUFFER_COUNT 3
//#define WEB_BUFFER_ELEMENTS 10

enum time_scale {
	S1 = 0,
	S10 = 1,
	S100 = 2
};
#define BUFFER1_SIZE 100
#define BUFFER10_SIZE 10
#define BUFFER100_SIZE 1

// TODO: adapt
//#define WEB_BUFFER_SIZE 100

#define NUM_WEB_CHANNELS 6
#define NUM_WEB_POINTS 1000
#define NUM_WEB_DIVS 10



struct ringbuffer {
	uint32_t element_size;
	uint32_t length;
	uint32_t filled;
	uint32_t head;
	int32_t data[NUM_WEB_CHANNELS * NUM_WEB_POINTS]; //WEB_BUFFER_SIZE * WEB_BUFFER_ELEMENTS];
};

struct web_shm {
	int64_t time;
	int32_t num_channels;
	struct ringbuffer buffer[WEB_RING_BUFFER_COUNT];
};



struct web_shm* create_web_shm();
struct web_shm* open_web_shm();

void reset_buffer(struct ringbuffer* buffer, int element_size, int length);

void buffer_add(struct ringbuffer* buffer, int32_t* data);

int32_t* buffer_get(struct ringbuffer* buffer, int num);


#endif