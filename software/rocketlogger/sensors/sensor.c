/**
 * Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group
 */

#include <stdint.h>

#include <linux/i2c-dev.h>

#include "../log.h"

#include "sensor.h"


int sensor_bus = -1;

// Sensor Registry
// Add sensors connected to this list
struct rl_sensor sensor_registry[SENSOR_REGISTRY_SIZE] = {
	{ 
		"TSL4531_left",
		TSL4531_I2C_ADDRESS_LEFT,
		TSL4531_CHANNEL_DEFAULT,
		RL_UNIT_LUX,
		RL_SCALE_NONE,
		&TSL4531_init,
		&TSL4531_close,
		&TSL4531_read,
		&TSL4531_getValue,
	},
	{ 
		"TSL4531_right",
		TSL4531_I2C_ADDRESS_RIGHT,
		TSL4531_CHANNEL_DEFAULT,
		RL_UNIT_LUX,
		RL_SCALE_NONE,
		&TSL4531_init,
		&TSL4531_close,
		&TSL4531_read,
		&TSL4531_getValue,
	},
};


/**
 * Open a new handle of the I2C bus.
 * @return The bus handle
 */
int Sensors_openBus(void) {
    int bus = open(I2C_BUS_FILENAME, O_RDWR);
	if (bus < 0) {
		rl_log(ERROR, "failed to open the I2C bus", bus);
	}
    return bus;
}

/**
 * Close a I2C sensor bus.
 * @param bus The I2C bus to close
 * @return Status (error) code
 */
int Sensors_closeBus(int bus) {
	int result = close(bus);
	if (result < 0) {
		rl_log(ERROR, "failed to close the I2C bus", bus);
	}
    return result;
}

/**
 * Initialize the shared I2C sensor bus.
 * @return Status (error) code
 */
int Sensors_initSharedBus(void) {
    sensor_bus = Sensors_openBus();
    return sensor_bus;
}

/**
 * Get the shared I2C bus handle.
 * @return The I2C bus handle
 */
int Sensors_getSharedBus(void) {
    return sensor_bus;
}

/**
 * Initiate an I2C communication with a device.
 * @param sensor_address The I2C address of the device
 * @return Status (error) code
 */
int Sensors_initSharedComm(uint8_t device_address) {
	return ioctl(sensor_bus, I2C_SLAVE, device_address);
}

/**
 * Close the shared I2C sensor bus.
 * @return Status (error) code
 */
void Sensors_closeSharedBus(void) {
	Sensors_closeBus(sensor_bus);
    int sensor_bus = -1;
    (void)sensor_bus; // suppress unused warning
}

/**
 * Scan the I2C sensor for sensor in the registry and initialize them.
 * @param sensors_available List of sensors of the registry available
 * @return Number of sensors from the regisry found on the bus
 */
uint8_t Sensors_scan(int8_t sensors_available[]) {

	// log message
	char message[MAX_MESSAGE_LENGTH] = "Found ambient sensors:\n\t- ";

	// Scan for available sensors //
	uint8_t sensor_count = 0;
	
	// scan
	for (unsigned int i = 0; i < SENSOR_REGISTRY_SIZE; i++) {
		int result = sensor_registry[i].init(sensor_registry[i].address);
		if (result == SUCCESS) {
			// sensor available
			sensors_available[i] = (int8_t)sensor_registry[i].address;
			sensor_count++;

			// message
			strcat(message, sensor_registry[i].name);
			strcat(message, "\n\t- ");
		} else {
			// sensor not available
			sensors_available[i] = -1;
		}
	}

	// message & return
	if (sensor_count == 0) {
		rl_log(WARNING, "no ambient sensor found...");
	} else {
		message[strlen(message) - 3] = 0;
		rl_log(INFO, "%s", message);
		printf("\n\n%s\n", message);
	}
	return sensor_count;
}

/**
 * Close all sensors used on the I2C bus.
 * @param sensors_available List of available (previously initialized) sensors
 */
void Sensors_close(int8_t sensors_available[]) {
	int i;
	for (i = 0; i < SENSOR_REGISTRY_SIZE; i++) {
		if (sensors_available[i] >= 0) {
			sensor_registry[i].close(sensor_registry[i].address);
		}
	}
}
