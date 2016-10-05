#include <stdint.h>
#include <ncurses.h>

#include "types.h"

void meter_init();

void meter_stop();

void print_meter(struct rl_conf* conf, void* virt_addr, unsigned int sample_size);