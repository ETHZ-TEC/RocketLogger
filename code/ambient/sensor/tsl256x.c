#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "tsl256x.h"

/// TSL256x sensor ID
uint8_t tsl256x_id = 0;

/// TSL256x timing configuration
uint8_t tsl256x_timing = 0;

int TSL256x_init(int i2c_bus) {
  TSL256x_initComm(i2c_bus);

  // enable power
  uint8_t command = TSL256X_CMD_CMD | TSL256X_REG_CONTROL;
  int result = i2c_smbus_write_byte_data(i2c_bus, command, 0x03);
  if (result < 0) {
    return result;
  }

  // get sensor ID
  result = TSL256x_getID(i2c_bus, &tsl256x_id);
  if (result < 0) {
    return result;
  }

  return 0;
}

int TSL256x_getLux(int i2c_bus, double* lux) {
  uint16_t data[2] = { 0 };
  *lux = 0.0;

  int result = TSL256x_readValues(i2c_bus, data);
  if (result < 0) {
    return result;
  }

  *lux = TSL256x_calculateLux(data[0], data[1]);

  return 0;
}

/**
 * Initiate an I2C communication
 * @param i2c_bus The I2C bus used for communication
 * @return Status code
 */
int TSL256x_initComm(int i2c_bus) {
  return ioctl(i2c_bus, I2C_SLAVE, TSL256X_I2C_ADDRESS);
}

/**
 * Get the device ID
 * @param i2c_bus The I2C bus used for communication
 * @param id The device id read
 * @return Status code
 */
int TSL256x_getID(int i2c_bus, uint8_t* id) {
  int result = TSL256x_initComm(i2c_bus);
  if (result < 0) {
    return result;
  }

  uint8_t command = TSL256X_CMD_CMD | TSL256X_REG_ID;
  result = i2c_smbus_read_byte_data(i2c_bus, command);
  if (result < 0) {
    *id = 0;
    return result;
  }

  *id = (uint8_t)result;
  return 0;
}

/**
 * Read raw sensor value output
 * @param data Sensor data buffer
 * @return Status code
 */
int TSL256x_readValues(int i2c_bus, uint16_t* data) {
  int result = TSL256x_initComm(i2c_bus);
  if (result < 0) {
    return result;
  }

  uint8_t command = TSL256X_CMD_CMD | TSL256X_REG_DATA0LOW;
  result = i2c_smbus_read_word_data(i2c_bus, command);
  if (result < 0) {
    return result;
  }
  else {
    data[0] = (uint16_t)result;
  }

  command = TSL256X_CMD_CMD | TSL256X_REG_DATA1LOW;
  result = i2c_smbus_read_word_data(i2c_bus, command);
  if (result < 0) {
    return result;
  }
  else {
    data[1] = (uint16_t)result;
  }

  return 0;
}

double TSL256x_calculateLux(uint16_t channel_ambient, uint16_t channel_ir) {
  // 0 Lux (prevent division by 0)
  if (channel_ambient == 0) {
    return 0;
  }

  // TODO: include scaling based on timing configuration
  double ch0 = (double)channel_ambient;
  double ch1 = (double)channel_ir;
  double ratio = ch1 / ch0;
  double lux = 0;

  if (tsl256x_id & TSL256X_ID_PACKAGE) {
    // TSL256x T, FN, and CL Package
    if (ratio <= 0.50) {
      lux = 0.0304 * ch0 - 0.062 * ch0 * pow(ratio, 1.4);
    }
    else if (ratio <= 0.61) {
      lux = 0.0224 * ch0 - 0.031 * ch1;
    }
    else if (ratio <= 0.80) {
      lux = 0.0128 * ch0 - 0.0153 * ch1;
    }
    else if (ratio <= 1.30) {
      lux = 0.00146 * ch0 - 0.00112 * ch1;
    }
  }
  else {
    // TSL256x CS Package
    if (ratio <= 0.52) {
      lux = 0.0315 * ch0 - 0.0593 * ch0 * pow(ratio, 1.4);
    }
    else if (ratio <= 0.65) {
      lux = 0.0229 * ch0 - 0.0291 * ch1;
    }
    else if (ratio <= 0.80) {
      lux = 0.0157 * ch0 - 0.0180 * ch1;
    }
    else if (ratio <= 1.30) {
      lux = 0.00338 * ch0 - 0.00260 * ch1;
    }
  }

  return lux;
}
