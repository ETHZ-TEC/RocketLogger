#ifndef UTIL_H
#define UTIL_H

/*#include <sys/shm.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <syslog.h>
#include <errno.h>
#include <stdarg.h>*/

#include "types.h"


#define SHMEM_PERMISSIONS 0666

int read_status(struct rl_status* status);
int write_status(struct rl_status* status);

// standard functions
int ceil_div(int n, int d);
int count_bits(int x);

void sig_handler(int signo);

//void* memory_map(unsigned int addr, size_t size);
//int memory_unmap(void* ptr, size_t size);

int read_file_value(char filename[]);

void rl_log(rl_log_type type, const char* format, ... );

#endif