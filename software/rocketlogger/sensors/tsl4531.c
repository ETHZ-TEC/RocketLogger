#include <errno.h>

#include "sensor.h"

#include "tsl4531.h"


uint8_t TSL4531_sensors[] = TSL4531_I2C_ADDRESSES;

enum TSL4531_range TSL4531_range[sizeof(TSL4531_sensors)] = { TSL4531_RANGE_AUTO };
enum TSL4531_range TSL4531_auto_range[sizeof(TSL4531_sensors)] = { TSL4531_RANGE_MEDIUM };
uint8_t TSL4531_multiplier[sizeof(TSL4531_sensors)] = { TSL4531_MULT_200 };
int32_t TSL4531_values[sizeof(TSL4531_sensors)] = { 0 };


/**
 * Initialize the light sensor
 * @param sensor_address The I2C address of the sensor
 * @return return Status code
 */
int TSL4531_init(uint8_t sensor_address) {
	int sensor_bus = Sensors_getSharedBus();

	if (sensor_bus < 0) {
		rl_log(ERROR, "TSL4531 I2C bus not initialized properly");
		return FAILURE;
	}

	int result = 0;

	result = Sensors_initSharedComm(sensor_address);
	if (result < 0) {
		rl_log(ERROR, "TSL4531 I2C initialization failed");
		return FAILURE;
	}

	uint8_t sensor_id = TSL4531_getID();
	if (sensor_id != TSL4531_ID) {
		rl_log(ERROR, "TSL4531 with wrong sensor ID: %d", sensor_id);
		return FAILURE;
	}

	result = TSL4531_setParameters(sensor_address);
	if (result < 0) {
		rl_log(ERROR, "TSL4531 setting configuraiton failed");
		return FAILURE;
	}

	return SUCCESS;
}

/**
 * Close TSL sensor.
 * @param sensor_address The I2C address of the sensor
 */
void TSL4531_close(uint8_t sensor_address) {
	(void)sensor_address; // suppress unused warning
}

/**
 * Read the sensor values.
 * @param sensor_address The I2C address of the sensor
 * @TODO communication error handling
 */
int TSL4531_read(uint8_t sensor_address) {
	int sensor_index = TSL4531_getIndex(sensor_address);
	int sensor_bus = Sensors_getSharedBus();

	int result = Sensors_initSharedComm(sensor_address);
	if (result < 0) {
		rl_log(ERROR, "TSL4531 I2C communication failed");
		return FAILURE;
	}

	uint8_t data_low = i2c_smbus_read_byte_data(sensor_bus, TSL4531_COMMAND | TSL4531_REG_DATALOW);
	uint8_t data_high = i2c_smbus_read_byte_data(sensor_bus, TSL4531_COMMAND | TSL4531_REG_DATAHIGH);

	TSL4531_values[sensor_index] = TSL4531_multiplier[sensor_index] *
			((((int32_t) data_high) << 8) | ((int32_t) data_low));

	if (TSL4531_range[sensor_index] == TSL4531_RANGE_AUTO) {
		// Auto-Range
		switch (TSL4531_auto_range[sensor_index]) {
		case TSL4531_RANGE_LOW:
			if (TSL4531_values[sensor_index] >= TSL4531_RANGE_LOW_MAX) {
				TSL4531_sendRange(sensor_address, TSL4531_RANGE_MEDIUM);
				TSL4531_auto_range[sensor_index] = TSL4531_RANGE_MEDIUM;
			}
			break;
		case TSL4531_RANGE_MEDIUM:
			if (TSL4531_values[sensor_index] >= TSL4531_RANGE_MEDIUM_MAX) {
				TSL4531_sendRange(sensor_address, TSL4531_RANGE_HIGH);
				TSL4531_auto_range[sensor_index] = TSL4531_RANGE_HIGH;
			} else if (TSL4531_values[sensor_index] < TSL4531_RANGE_LOW_MAX - TSL4531_RANGE_HYSTERESIS) {
				TSL4531_sendRange(sensor_address, TSL4531_RANGE_LOW);
				TSL4531_auto_range[sensor_index] = TSL4531_RANGE_LOW;
			}
			break;
		case TSL4531_RANGE_HIGH:
			if (TSL4531_values[sensor_index] < TSL4531_RANGE_MEDIUM_MAX - TSL4531_RANGE_HYSTERESIS) {
				TSL4531_sendRange(sensor_address, TSL4531_RANGE_MEDIUM);
				TSL4531_auto_range[sensor_index] = TSL4531_RANGE_MEDIUM;
			}
			break;
		default:
			break;
		}
	}

	return SUCCESS;
}

/**
 * Get the values read from the sensor
 * @param sensor_address The I2C address of the sensor
 * @param channel The channel of the sensor to get
 * @return Sensor value in lux
 */
int32_t TSL4531_getValue(uint8_t sensor_address, uint8_t channel) {
	if (channel > 0) {
		return 0;
	}

	int sensor_index = TSL4531_getIndex(sensor_address);

	return TSL4531_values[sensor_index];
}

/**
 * Set range of light sensor
 * @param sensor_address The I2C address of the sensor
 * @param range The range {@link TSL4531_range} to set
 * @return Status code
 * @TODO communication error handling
 */
int TSL4531_setRange(uint8_t sensor_address, int range) {
	int sensor_index = TSL4531_getIndex(sensor_address);
	TSL4531_sendRange(sensor_address, range);
	TSL4531_range[sensor_index] = range;

	return SUCCESS;
}

/**
 * Get current range
 * @param sensor_address The I2C address of the sensor
 * @return current range {@link TSL4531_range}
 */
int TSL4531_getRange(uint8_t sensor_address) {
	int sensor_index = TSL4531_getIndex(sensor_address);

	if(TSL4531_range[sensor_index] == TSL4531_RANGE_AUTO) {
		return TSL4531_auto_range[sensor_index];
	} else {
		return TSL4531_range[sensor_index];
	}
}

/**
 * Get the device ID
 * @param sensor_address The I2C address of the sensor
 * @return Status code
 */
uint8_t TSL4531_getID(void) {
	int sensor_bus = Sensors_getSharedBus();

	int read_result = i2c_smbus_read_byte_data(sensor_bus,
		TSL4531_COMMAND | TSL4531_REG_ID);

	if (read_result < 0) {
		rl_log(ERROR, "TSL4531 I2C error reading ID of sensor");
	}
	return (uint8_t) read_result;
}

/**
 * Set the sensor parameter to default for continuous sensing.
 * @param sensor_address The I2C address of the sensor
 * @return Status code
 * @TODO communication error handling
 */
int TSL4531_setParameters(uint8_t sensor_address) {
	int sensor_bus = Sensors_getSharedBus();

	i2c_smbus_write_byte_data(sensor_bus, TSL4531_COMMAND | TSL4531_REG_CONTROL,
		TSL4531_SAMPLE_CONTINUOUS);

	TSL4531_setRange(sensor_address, TSL4531_RANGE_AUTO);

	return SUCCESS;
}

/**
 * Configure the range of the sensor.
 * @param sensor_address The I2C address of the sensor
 * @param range The range {@link TSL4531_range} to set
 * @return Status code
 * @TODO communication error handling
 */
int TSL4531_sendRange(uint8_t sensor_address, int range) {
	int sensor_index = TSL4531_getIndex(sensor_address);
	int sensor_bus = Sensors_getSharedBus();

	switch (range) {
	case TSL4531_RANGE_LOW:
		i2c_smbus_write_byte_data(sensor_bus, TSL4531_COMMAND | TSL4531_REG_CONFIG,
			TSL4531_INT_TIME_400 | TSL4531_LOW_POWER);
		TSL4531_multiplier[sensor_index] = TSL4531_MULT_400;
		break;
	case TSL4531_RANGE_MEDIUM:
		i2c_smbus_write_byte_data(sensor_bus, TSL4531_COMMAND | TSL4531_REG_CONFIG,
			TSL4531_INT_TIME_200 | TSL4531_LOW_POWER);
		TSL4531_multiplier[sensor_index] = TSL4531_MULT_200;
		break;
	case TSL4531_RANGE_HIGH:
		i2c_smbus_write_byte_data(sensor_bus, TSL4531_COMMAND | TSL4531_REG_CONFIG,
			TSL4531_INT_TIME_100 | TSL4531_LOW_POWER);
		TSL4531_multiplier[sensor_index] = TSL4531_MULT_100;
		break;
	case TSL4531_RANGE_AUTO:
		i2c_smbus_write_byte_data(sensor_bus, TSL4531_COMMAND | TSL4531_REG_CONFIG,
			TSL4531_INT_TIME_200 | TSL4531_LOW_POWER);
		TSL4531_multiplier[sensor_index] = TSL4531_MULT_200;
		TSL4531_auto_range[sensor_index] = TSL4531_RANGE_MEDIUM;
		break;
	default:
		break;
	}

	return SUCCESS;
}

/**
 * Get the index of the sensor with specified address.
 * @param sensor_address The sensor address used to look up the index
 * @return The index of the sensor, or if not found -1
 */
int TSL4531_getIndex(uint8_t sensor_address) {
	unsigned int index = 0;
	while (index < sizeof(TSL4531_sensors)) {
		if (sensor_address == TSL4531_sensors[index]) {
			return (int)index;
		}
		index++;
	}
	return -1;
}
