/*#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>*/

#include "types.h"
#include "rl_file.h"
#include "util.h"
#include "sem.h"
#include "web.h"


#define MAC_ADDRESS_FILE "/sys/class/net/eth0/address" // TODO: move to types.h?

// DEFINES


// channels (remove)
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

/// Mask to mask binary inputs read from PRU
#define VALID_MASK 0x1
#define BINARY_MASK 0xE

#define H_L_SCALE 100

#define CSV_LINE_LENGTH 200
#define CSV_VALUE_LENGTH 50
// FUNCTIONS

void setup_lead_in(struct rl_file_lead_in* lead_in, struct rl_conf* conf);
void setup_header(struct rl_file_header* file_header, struct rl_conf* conf);
void store_header(FILE* data, struct rl_file_header* file_header);
void store_header_csv(FILE* data, struct rl_file_header* file_header);
void update_header(FILE* data, struct rl_file_header* file_header);
int store_buffer(FILE* data, void* buffer_addr, unsigned int sample_size, int samples_buffer, struct rl_conf* conf, int sem_id, struct web_shm* web_data);
