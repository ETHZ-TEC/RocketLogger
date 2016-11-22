#ifndef IPC_H
#define IPC_H

#include "types.h"
#include "log.h"
#include "util.h"

#define WEB_RING_BUFFER_COUNT 3
#define BUF1_INDEX 0
#define BUF10_INDEX 1
#define BUF100_INDEX 2

enum time_scale {
	S1 = 0,
	S10 = 1,
	S100 = 2
};

#define BUFFER1_SIZE 100
#define BUFFER10_SIZE 10
#define BUFFER100_SIZE 1

#define NUM_WEB_CHANNELS 12 // 6 analog + 6 digital
#define NUM_WEB_POINTS 1000
#define NUM_WEB_DIVS 10



struct ringbuffer {
	uint32_t element_size;
	uint32_t length;
	uint32_t filled;
	uint32_t head;
	int64_t data[NUM_WEB_CHANNELS * NUM_WEB_POINTS];
};

struct web_shm {
	int64_t time;
	uint32_t num_channels;
	struct ringbuffer buffer[WEB_RING_BUFFER_COUNT];
};



struct web_shm* create_web_shm();
struct web_shm* open_web_shm();

void reset_buffer(struct ringbuffer* buffer, int element_size, int length);

void buffer_add(struct ringbuffer* buffer, int64_t* data);

int64_t* buffer_get(struct ringbuffer* buffer, int num);


#endif
