#ifndef ROCKETLOGGER_H
#define ROCKETLOGGER_H

#define _FILE_OFFSET_BITS  64

// ---------------------------------------------- Includes ----------------------------------------------------------//

#include <stdio.h>
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
#include <pruss_intc_mapping.h>
//#include <ncurses.h>

#include "util.h"
#include "types.h"
#include "calibration.h"
#include "meter.h"
#include "file_handling.h"



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
#define DEVICE_FILE			"/etc/rocketlogger/" // TODO: ???
#define FIFO_FILE			"/etc/rocketlogger/fifo"
#define CONTROL_FIFO		"/etc/rocketlogger/control"
#define WEB_DATA_FILE		"/etc/rocketlogger/data"
#define PRU_CODE			"/lib/firmware/SPI.bin"

// ---------------------------------------------- PRU/PWM DEFINES ---------------------------------------------------//

#define PRECISION_HIGH 24
#define PRECISION_LOW 16

#define SIZE_HIGH 4
#define SIZE_LOW 2

#define TIMEOUT 3 // 3s PRU timeout

#define BUFFERSTATUSSIZE 4 //buffer status size in bytes
#define WEB_BUFFER_SIZE 100
#define NUMBER_WEB_CHANNELS 6

// ---------------------------------------------- CHANNEL DEFINES ----------------------------------------------------// 

//#define NUMBERCHANNELS 10

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

// ---------------------------------------------- PRU ------------------------------------------------------------//

// PRU states
enum pru_states {
	PRU_OFF = 0,
	PRU_LIMIT = 1,
	PRU_CONTINUOUS = 3
};

#define NUMBER_PRU_COMMANDS 10
struct pru_data_struct {
	enum pru_states state;
	unsigned int precision;
	unsigned int sample_size;
	unsigned int buffer0_location;
	unsigned int buffer1_location;
	unsigned int buffer_size;
	unsigned int sample_limit;
	unsigned int number_commands;
	unsigned int commands[NUMBER_PRU_COMMANDS];
};

// ---------------------------------------------- PROTOCOL FUNCTIONS ------------------------------------------------//


// store functions
int store_header_bin(FILE* data, struct header* h);
int store_header_csv(FILE* data, struct header* h);

int store_buffer(FILE* data, int fifo_fd, int control_fifo, void* virt_addr, int buffer_number, unsigned int samples_buffer, unsigned int size, int channels, struct timeval* current_time, int store, int binary, int webserver, struct rl_conf* conf);
void average_data(double* data_out, int* data_in, int length, int num_channels);
void collapse_data(float* data_out, int* data_in, int channels);
int store_web_data(int fifo_fd, int control_fifo, float* buffer);

// PRU functions
int pru_set_state(enum pru_states state);
int pru_init();
int pru_stop(); // stop pru when in continuous mode (has to be done before close)
int pru_close();
int pru_sample(FILE* data, struct rl_conf* conf);


#endif