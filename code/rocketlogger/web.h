#ifndef IPC_H
#define IPC_H

#include "types.h"
#include "log.h"
#include "util.h"

/// Number of ring buffers in shared memory
#define WEB_RING_BUFFER_COUNT 3
/// Index of 1s/div buffer
#define BUF1_INDEX 0
/// Index of 10s/div buffer
#define BUF10_INDEX 1
/// Index of 100s/div buffer
#define BUF100_INDEX 2

/**
 * RocketLogger time scale definition
 */
enum time_scale {
	S1 = 0, //!< 100 sample/s
	S10 = 1,//!< 10 samples/s
	S100 = 2//!< 1 samples/s
};

/// Size of 1s/div buffer
#define BUFFER1_SIZE 100
/// Size of 10s/div buffer
#define BUFFER10_SIZE 10
/// Size of 100s/div buffer
#define BUFFER100_SIZE 1

/// Maximum number of channels in web interface (6 analog + 6 digital)
#define NUM_WEB_CHANNELS 12 // 6 analog + 6 digital
/// Number of data points in web plot
#define NUM_WEB_POINTS 1000
/// Number of time divisions in web plot
#define NUM_WEB_DIVS 10


/**
 * Ring buffer for data exchange to web server
 */
struct ringbuffer {
	/// Size of buffer element
	uint32_t element_size;
	///  Size of buffer in elements
	uint32_t length;
	/// Number of elements in buffer
	uint32_t filled;
	/// Current position (in elements)
	uint32_t head;
	/// Data array
	int64_t data[NUM_WEB_CHANNELS * NUM_WEB_POINTS];
};

/**
 * Shared memory struct for data exchange to web server
 */
struct web_shm {
	/// Time stamp of most recent datum (in UNIX time, UTC)
	int64_t time;
	/// Number of channels sampled
	uint32_t num_channels;
	/// Array of ring buffers for different time scales
	struct ringbuffer buffer[WEB_RING_BUFFER_COUNT];
};



struct web_shm* create_web_shm(void);
struct web_shm* open_web_shm(void);

void reset_buffer(struct ringbuffer* buffer, int element_size, int length);

void buffer_add(struct ringbuffer* buffer, int64_t* data);

int64_t* buffer_get(struct ringbuffer* buffer, int num);


#endif
