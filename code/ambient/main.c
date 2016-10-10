#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

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

  int status = TSL256x_init(i2c_file);
  if (status < 0) {
    perror("Failed to initialize light sensor.\n");
    /* ERROR HANDLING: you can check errno to see what went wrong */
    exit(1);
  }

  printf("Start reading sensor every second for 1min...\n");
  for (int i = 0; i < 60; i++) {
    double lux = 0;
    sleep(1);
    TSL256x_getLux(i2c_file, &lux);
    printf("Read sensor value:\t% 9.2f\n", lux);
  }

  printf("Test program run complete\n");

  return 0;
}
