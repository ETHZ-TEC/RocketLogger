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
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <math.h>
#include <time.h> 
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <prussdrv.h>
#include <pruss_intc_mapping.h>


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
#define PWM_FILE 			"/sys/devices/ocp.3/pwm_test_P9_22.12/"
#define GPIO_FILE 			"/sys/class/gpio/"
#define FHR1_FILE 			"/sys/class/gpio/gpio30/"
#define FHR2_FILE 			"/sys/class/gpio/gpio60/"
#define DEVICE_FILE			"/etc/rocketlogger/"
#define LOG_FILE			"/var/www/log/log.txt"
#define FIFO_FILE			"/etc/rocketlogger/fifo"
#define CONTROL_FIFO		"/etc/rocketlogger/control"
#define CALIBRATION_FILE	"/etc/rocketlogger/calibration.dat"
#define TEMP_CALIBRATION	"/etc/rocketlogger/temp_calib.dat"
#define WEB_DATA_FILE		"/etc/rocketlogger/data"
#define PRU_CODE			"/lib/firmware/SPI.bin"

#define CALIBRATION_POINTS 2
#define CALIBRATION_VALUES 1000

// ---------------------------------------------- PRU/PWM DEFINES ---------------------------------------------------//

#define STATUSSIZE 2 //status size in bytes
#define BUFFERSTATUSSIZE 4 //buffer status size in bytes
#define WEB_BUFFER_SIZE 100
#define NUMBER_WEB_CHANNELS 6

#define MAP_SIZE 0x0FFFFFFF
#define MAP_MASK (MAP_SIZE - 1)

// PRU states
#define OFF 0
#define SAMPLES 1
#define CONTINUOUS 3
//#define OFF 4

// PWM states
#define ENABLE 1
#define DISABLE 0

// data precision (in bit)
#define LOW 16
#define HIGH 24

// ---------------------------------------------- CHANNEL DEFINES ----------------------------------------------------// 

#define NUMBERCHANNELS 10

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

// ---------------------------------------------- HEADER STRUCTS ----------------------------------------------------//

#define HEADERLENGTH 6
#define MAXNUMBERSAMPLESLENGTH 12

struct header {
	int header_length;
	int number_samples;
	int buffer_size;
	int rate;
	int channels;
	int precision;
};

// ---------------------------------------------- Calibration Values -------------------------------------------------//

int offsets16[10];
int offsets24[10];

double scales24[10];
double scales16[10];

// ---------------------------------------------- PROTOCOL FUNCTIONS ------------------------------------------------//

// signal functions
void sig_handler(int signo);

// log functions
int rl_log_init();
int rl_log(char line[]);

// calibration functions
int set_default_offsets();
int set_default_scales();
int reset_offsets();
int reset_scales();
int read_calibration();
int write_calibration();

// standard functions
int ceil_div(int n, int d);
int count_bits(int x);
int input_available();
int read_file_value(char filename[]);
int write_sys_value(char filename[], int value);
int write_sys_string(char filename[], char value[]);

// memory map
void* memory_map(unsigned int addr, size_t size);
int memory_unmap(void* ptr, size_t size);

// store functions
int store_header_bin(FILE* data, struct header* h);
int store_header_csv(FILE* data, struct header* h);

int store_buffer(FILE* data, int fifo_fd, int control_fifo, void* virt_addr, int buffer_number, unsigned int number, unsigned int size, int channels, struct timeval* current_time, int store, int binary, int webserver);
void print_meter(void* virt_addr, unsigned int number, unsigned int size, int channels);
void average_data(double* data_out, int* data_in, int length, int num_channels);
void collapse_data(float* data_out, int* data_in, int channels);
int store_web_data(int fifo_fd, int control_fifo, float* buffer);

// PRU functions
int pru_set_state(int state);
int pru_init();
int pru_stop(); // stop pru when in continuous mode (has to be done before close)
int pru_close();
int pru_sample(FILE* data, int rate, int update_rate, int number_samples, int channels, int webserver, int store, int meter, int binary);

// PWM functions
int pwm_init();
int pwm_close();

// GPIO function
int gpio_init();
int force_high_range(int channels);
int gpio_close();


#endif