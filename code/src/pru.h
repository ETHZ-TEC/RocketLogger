#ifndef ROCKETLOGGER_H
#define ROCKETLOGGER_H


// ---------------------------------------------- Includes ----------------------------------------------------------//

/*#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <math.h>
#include <time.h> 
#include <pthread.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <prussdrv.h>
#include <pruss_intc_mapping.h>*/
//#include <ncurses.h>

#include "log.h"
#include "util.h"
#include "types.h"
#include "calibration.h"
#include "meter.h"
#include "file_handling.h"
#include "sem.h"
#include "web.h"



// ---------------------------------------------- ADC DEFINES -------------------------------------------------------//

// ADS131E08S commands (all extended to 32 bits for PRU use)
#define WAKEUP 0x02000000
#define STANDBY 0x04000000
#define RESET 0x06000000
#define START 0x08000000
#define STOP  0x0A000000
#define OFFSETCAL 0x1A000000

#define RDATAC 0x10000000
#define SDATAC 0x11000000
#define RDATA 0x12000000

#define RREG 0x20000000
#define WREG 0x40000000

// ADS131E08S registers
#define ID 0x00000000
#define CONFIG1 0x01000000
#define CONFIG2 0x02000000
#define CONFIG3 0x03000000

#define CH1SET 0x05000000
#define CH2SET 0x06000000
#define CH3SET 0x07000000
#define CH4SET 0x08000000
#define CH5SET 0x09000000
#define CH6SET 0x0A000000
#define CH7SET 0x0B000000
#define CH8SET 0x0C000000

// Gains
#define GAIN1 0x1000
#define GAIN2 0x2000
#define GAIN12 0x6000

// ADS131E08S sample rates
#define K1 0x0600
#define K2 0x0500
#define K4 0x0400
#define K8 0x0300
#define K16 0x0200
#define K32 0x0100
#define K64 0x0000

// ADS131E08S config default values
#define CONFIG1DEFAULT 0x9000
#define CONFIG2DEFAULT 0xE000
#define CONFIG3DEFAULT 0xE800

// Enable internal reference
#define INTERNALREFERENCE 0x8000

// ---------------------------------------------- FILES ------------------------------------------------------------//

// memory map
#define MMAP_FILE			"/sys/class/uio/uio0/maps/map1/"
#define PRU_CODE			"/lib/firmware/rocketlogger_spi.bin"

// ---------------------------------------------- PRU DEFINES ---------------------------------------------------//

#define PRECISION_HIGH 24
#define PRECISION_LOW 16

#define SIZE_HIGH 4
#define SIZE_LOW 2

#define PRU_TIMEOUT 3 // 3s PRU timeout 

// PRU states
typedef enum pru_state {
	PRU_OFF = 0,
	PRU_LIMIT = 1,
	PRU_CONTINUOUS = 3
} rl_pru_state;

/// number of ADC commands
#define NUMBER_ADC_COMMANDS 12

/// PRU data struct
struct pru_data_struct {
	rl_pru_state state;
	uint32_t precision;
	uint32_t sample_size;
	uint32_t buffer0_location;
	uint32_t buffer1_location;
	uint32_t buffer_size;
	uint32_t sample_limit;
	uint32_t add_currents;
	uint32_t number_commands;
	uint32_t commands[NUMBER_ADC_COMMANDS];
};

// ----------------------------------------------  FUNCTIONS ------------------------------------------------//

void *pru_wait_event(void* voidEvent);
int pru_wait_event_timeout(unsigned int event, unsigned int timeout);

void pru_set_state(rl_pru_state state);
int pru_init();
int pru_setup(struct pru_data_struct* pru, struct rl_conf* conf);

int pru_sample(FILE* data, struct rl_conf* conf);

int pru_stop(); // stop pru when in continuous mode (has to be done before close)
int pru_close();


#endif
