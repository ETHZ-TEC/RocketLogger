/**
 * Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group
 */

#ifndef FILE_HANDLING_H_
#define FILE_HANDLING_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "log.h"
#include "rl_file.h"
#include "sem.h"
#include "types.h"
#include "util.h"

/// CSV value delimiter character
#define CSV_DELIMITER ","

// FUNCTIONS
void file_setup_lead_in(struct rl_file_lead_in* lead_in, struct rl_conf* conf);
void file_setup_header(struct rl_file_header* file_header,
                       struct rl_conf* conf);
void file_store_header_bin(FILE* data, struct rl_file_header* file_header);
void file_store_header_csv(FILE* data, struct rl_file_header* file_header);
void file_update_header_bin(FILE* data, struct rl_file_header* file_header);
void file_update_header_csv(FILE* data, struct rl_file_header* file_header);
void file_handle_data(FILE* data_file, void* buffer_addr,
                      uint32_t sample_data_size, uint32_t samples_count,
                      struct time_stamp* timestamp_realtime,
                      struct time_stamp* timestamp_monotonic,
                      struct rl_conf* conf);

#endif /* FILE_HANDLING_H_ */
