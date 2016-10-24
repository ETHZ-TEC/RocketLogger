/*#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>*/

#include "types.h"
#include "rl_file.h"
#include "util.h"


#define MAC_ADDRESS_FILE "/sys/class/net/eth0/address" // TODO: move to types.h?

// DEFINES

#define ALL 0x3FF

#define I1H 1
#define I1M 2
#define I1L 4
#define I1A (I1H|I1M|I1L) // all i1
#define V1 8
#define V2 16

#define I2H 32
#define I2M 64
#define I2L 128
#define I2A (I2H|I2M|I2L)
#define V3 256
#define V4 512

// force high range channels
#define I1 1
#define I2 2

#define BUFFERSTATUSSIZE 4 //buffer status size in bytes
#define WEB_BUFFER_SIZE 100
#define NUMBER_WEB_CHANNELS 6

/// Mask to mask binary inputs read from PRU
#define VALID_MASK 0x1
#define BINARY_MASK 0xE

// FUNCTIONS

void setup_header(struct header* file_header, struct rl_conf* conf, struct pru_data_struct* pru_data, int pru_sample_rate);
int store_header(FILE* data, struct header* file_header, struct rl_conf* conf);
int update_sample_number(FILE* data, struct header* file_header, struct rl_conf* conf);

void setup_lead_in(struct rl_file_lead_in* lead_in, struct rl_conf* conf);
void setup_header_new(struct rl_file_header* file_header, struct rl_conf* conf);
void store_header_new(FILE* data, struct rl_file_header* file_header);
void update_header(FILE* data, struct rl_file_header* file_header);
int store_buffer_new(FILE* data, void* buffer_addr, unsigned int sample_size, int samples_buffer, struct rl_conf* conf);

int store_buffer(FILE* data, int fifo_fd, int control_fifo, void* buffer_addr, unsigned int sample_size, int samples_buffer, struct rl_conf* conf);
int store_web_data(int fifo_fd, int control_fifo, float* buffer);
void collapse_data(float* data_out, int* data_in, int channels, struct rl_conf* conf);

// ToDo: use, remove?
/*void average_data(double* data_out, int* data_in, int length, int num_channels); */