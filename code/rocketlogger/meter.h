#include <stdint.h>
#include <ncurses.h>

#include "types.h"
#include "log.h"
#include "util.h"

void meter_init();

void meter_stop();

void meter_print_buffer(struct rl_conf* conf, void* virt_addr, uint32_t sample_size);
