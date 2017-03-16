/**
 * Standalone test of TSL256x light sensor interfacing using libi2c on linux.
 *
 * Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group
 *
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "sensor/tsl256x.h"

#include "main.h"

/// The I2C file handler
int i2c_file;

/**
 * Main program.
 * @return Exit code
 */
int main(void) {
  printf("Ambient Sensor Readout Test\n");

  printf("Opening I2C device file to get access to the bus...\n");

  i2c_file = open(I2C_FILENAME, O_RDWR);
  if (i2c_file < 0) {
    perror("Failed to open the I2C bus.\n");
    /* ERROR HANDLING: you can check errno to see what went wrong */
    exit(1);
  }

  printf("Configuring I2C device address to prepare communication...\n");

  // try accessing the TSL2561 sensor
  if (ioctl(i2c_file, I2C_SLAVE, TSL256X_I2C_ADDRESS) < 0) {
    perror("Failed to acquire bus access and/or talk to slave.\n");
    /* ERROR HANDLING: you can check errno to see what went wrong */
    exit(1);
  }

  // ----------------------------------------
  printf("Perform SMBus ID register read...\n");

  // read ID register using SMBus read byte command
  int result = 0;
  int command = 0;

  command = TSL256X_CMD_CMD | TSL256X_REG_ID;
  result = i2c_smbus_read_byte_data(i2c_file, command);
  if (result < 0) {
    perror("Failed to read device ID.\n");
    /* ERROR HANDLING: you can check errno to see what went wrong */
    exit(1);
  } else {
    printf(" Device ID:\t%d\n", result);
  }

  // ----------------------------------------
  printf("Powering up sensor...\n");

  // enable power
  command = TSL256X_CMD_CMD | TSL256X_REG_CONTROL;
  result = i2c_smbus_write_byte_data(i2c_file, command, 0x03);
  if (result < 0) {
    perror("Failed to write device control register.\n");
    /* ERROR HANDLING: you can check errno to see what went wrong */
    exit(1);
  }

  // verify power up state
  command = TSL256X_CMD_CMD | TSL256X_REG_CONTROL;
  result = i2c_smbus_read_byte_data(i2c_file, command);
  if (result < 0) {
    perror("Failed to read device control register.\n");
    /* ERROR HANDLING: you can check errno to see what went wrong */
    exit(1);
  } else {
    printf(" Power State:\t%d\tOK: %d\n", result, (result == 0x03));
  }

  // ----------------------------------------
  printf("Powering down sensor...\n");

  // disable power
  command = TSL256X_CMD_CMD | TSL256X_REG_CONTROL;
  result = i2c_smbus_write_byte_data(i2c_file, command, 0x00);
  if (result < 0) {
    perror("Failed to write device control register.\n");
    /* ERROR HANDLING: you can check errno to see what went wrong */
    exit(1);
  }

  // verify power down state
  command = TSL256X_CMD_CMD | TSL256X_REG_CONTROL;
  result = i2c_smbus_read_byte_data(i2c_file, command);
  if (result < 0) {
    perror("Failed to read device control register.\n");
    /* ERROR HANDLING: you can check errno to see what went wrong */
    exit(1);
  } else {
    printf(" Power State:\t%d\tOK: %d\n", result, (result == 0x00));
  }

  // ----------------------------------------
  printf("Test sequence executed successfully :)\n");

  return 0;
}
