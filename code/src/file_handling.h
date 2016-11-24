#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "types.h"
#include "rl_file.h"
#include "log.h"
#include "util.h"
#include "sem.h"
#include "web.h"


// DEFINES


/// Mask for valid bit read from PRU
#define VALID_MASK 0x1
/// Mask for binary inputs read from PRU
#define BINARY_MASK 0xE

/// Current high-low scale difference
#define H_L_SCALE 100

/// Max length of a CSV file line
#define CSV_LINE_LENGTH 200
/// Max length of a CSV value
#define CSV_VALUE_LENGTH 50

// FUNCTIONS
void setup_lead_in(struct rl_file_lead_in* lead_in, struct rl_conf* conf);
void setup_header(struct rl_file_header* file_header, struct rl_conf* conf);
void store_header_bin(FILE* data, struct rl_file_header* file_header);
void store_header_csv(FILE* data, struct rl_file_header* file_header);
void update_header_bin(FILE* data, struct rl_file_header* file_header);
void update_header_csv(FILE* data, struct rl_file_header* file_header);
int store_buffer(FILE* data, void* buffer_addr, uint32_t sample_size, uint32_t samples_buffer, struct rl_conf* conf, int sem_id, struct web_shm* web_data_ptr);
