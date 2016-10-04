#ifndef UTIL_H
#define UTIL_H

#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h> 

#include "types.h"

int read_status(struct rl_status* status);
int write_status(struct rl_status* status);

// standard functions
int ceil_div(int n, int d);
int count_bits(int x);
int input_available();

void sig_handler(int signo);

void* memory_map(unsigned int addr, size_t size);
int memory_unmap(void* ptr, size_t size);

int read_file_value(char filename[]);

void rl_error(rl_error_type type, char message[]);

#endif