/**
 * Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group
 */

#ifndef ROCKETLOGGER_H
#define ROCKETLOGGER_H

// ---------------------------------------------- Includes
// ----------------------------------------------------------//

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <pruss_intc_mapping.h>
#include <prussdrv.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "ambient.h"
#include "calibration.h"
#include "file_handling.h"
#include "log.h"
#include "meter.h"
#include "sem.h"
#include "types.h"
#include "util.h"
#include "web.h"

// ---------------------------------------------- ADC DEFINES
// -------------------------------------------------------//

/**
 * ADS131E08S command (extended to 32 bits for PRU use) definitions
 */
#define WAKEUP 0x02000000
#define STANDBY 0x04000000
#define RESET 0x06000000
#define START 0x08000000
#define STOP 0x0A000000
#define OFFSETCAL 0x1A000000
#define RDATAC 0x10000000
#define SDATAC 0x11000000
#define RDATA 0x12000000
#define RREG 0x20000000
#define WREG 0x40000000

/**
 * ADS131E08S register definitions
 */
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

/**
 * ADS131E08S gain settings
 */
#define GAIN1 0x1000
#define GAIN2 0x2000
#define GAIN12 0x6000

/**
 * ADS131E08S sampling rates
 */
#define K1 0x0600
#define K2 0x0500
#define K4 0x0400
#define K8 0x0300
#define K16 0x0200
#define K32 0x0100
#define K64 0x0000

/**
 * ADS131E08S configuration default value defines
 */
#define CONFIG1DEFAULT 0x9000
#define CONFIG2DEFAULT 0xE000
#define CONFIG3DEFAULT 0xE800

// ---------------------------------------------- FILES
// ------------------------------------------------------------//

/// Memory map file
#define MMAP_FILE "/sys/class/uio/uio0/maps/map1/"
/// PRU binary file location
#define PRU_CODE "/lib/firmware/rocketlogger_spi.bin"

// ---------------------------------------------- PRU DEFINES
// ---------------------------------------------------//

/**
 * ADS131E08S precision defines({@link PRECISION_HIGH} for low sampling rates,
 * {@link PRECISION_LOW} for high ones)
 */
#define PRECISION_HIGH 24
#define PRECISION_LOW 16

/**
 * Sample size definitions
 */
#define SIZE_HIGH 4
#define SIZE_LOW 2

/// PRU time out in seconds
#define PRU_TIMEOUT 3

/**
 * PRU state definition
 */
typedef enum pru_state {
    PRU_OFF = 0,       //!< PRU off
    PRU_LIMIT = 1,     //!< Limited sampling mode
    PRU_CONTINUOUS = 3 //!< Continuous sampling mode
} rl_pru_state;

/// Number of ADC commands
#define NUMBER_ADC_COMMANDS 12

/**
 * Struct for data exchange with PRU
 */
struct pru_data_struct {
    /// Current PRU state
    rl_pru_state state;
    /// ADC precision (in bit)
    uint32_t precision;
    /// Sample size in shared memory
    uint32_t sample_size;
    /// Pointer to shared buffer 0
    uint32_t buffer0_location;
    /// Pointer to shared buffer 1
    uint32_t buffer1_location;
    /// Shared buffer size
    uint32_t buffer_size;
    /// Samples to take (0 for continuous)
    uint32_t sample_limit;
    /// Number of ADC commands to send
    uint32_t number_commands;
    /// ADC commands to send
    uint32_t commands[NUMBER_ADC_COMMANDS];
};

// ----------------------------------------------  FUNCTIONS
// ------------------------------------------------//

void* pru_wait_event(void* voidEvent);
int pru_wait_event_timeout(unsigned int event, unsigned int timeout);

void pru_set_state(rl_pru_state state);
int pru_init(void);
int pru_data_setup(struct pru_data_struct* pru, struct rl_conf* conf,
                   uint32_t avg_factor);

int pru_sample(FILE* data, FILE* ambient_file, struct rl_conf* conf);

void pru_stop(
    void); // stop pru when in continuous mode (has to be done before close)
void pru_close(void);

#endif
