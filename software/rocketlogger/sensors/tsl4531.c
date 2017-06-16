#include "tsl4531.h"

/// I2C-Bus file descriptor
int tsl4531_bus[NUM_STB_SENSORS];
/// Integration time multiplier
uint8_t multiplier[NUM_STB_SENSORS] = {TSL4531_MULT_200, TSL4531_MULT_200, TSL4531_MULT_200, TSL4531_MULT_200};

/// Auto-range settings
int range[NUM_STB_SENSORS] = {TSL4531_RANGE_AUTO, TSL4531_RANGE_AUTO, TSL4531_RANGE_AUTO, TSL4531_RANGE_AUTO};
int auto_range[NUM_STB_SENSORS] = {TSL4531_RANGE_MEDIUM, TSL4531_RANGE_MEDIUM, TSL4531_RANGE_MEDIUM, TSL4531_RANGE_MEDIUM};

int stb_i2c_addresses[NUM_STB_SENSORS] = {0x28, 0x29, 0x2A, 0x2B};

/**
 * Initialize the light sensor
 * @return return code
 */
int TSL4531_init(int sensor) {

	tsl4531_bus[sensor] = open(I2C_BUS_FILENAME, O_RDWR);
	if (tsl4531_bus[sensor] < 0) {
		rl_log(ERROR, "failed to open the I2C bus");
		return FAILURE;
	}

	int result = TSL4531_initComm(tsl4531_bus[sensor], sensor);
	if (result < 0) {
		rl_log(ERROR, "TSL4531 I2C initialization failed");
		return FAILURE;
	}

	uint8_t sensor_id = TSL4531_getID(sensor);
	if (sensor_id != TSL4531_ID) {
		//rl_log(ERROR, "wrong sensor ID: %d", sensor_id);
		return FAILURE;
	}

	TSL4531_setParams(sensor);

	return SUCCESS;
}

/**
 * Close TSL sensor
 */
void TSL4531_close(int sensor) {
	close(tsl4531_bus[sensor]);
}

/**
 * Read a value from sensor
 * @return Value in lux
 */
int32_t TSL4531_readValue(int sensor) {

	uint8_t data_low = i2c_smbus_read_byte_data(tsl4531_bus[sensor],
	TSL4531_COMMAND | TSL4531_REG_DATALOW);
	uint8_t data_high = i2c_smbus_read_byte_data(tsl4531_bus[sensor],
	TSL4531_COMMAND | TSL4531_REG_DATAHIGH);

	int32_t value = multiplier[sensor]
			* ((((int32_t) data_high) * 256) + (int32_t) data_low);

	if (range[sensor] == TSL4531_RANGE_AUTO) {
		// Auto-Range
		switch (auto_range[sensor]) {
		case TSL4531_RANGE_LOW:
			if (value >= TSL4531_RANGE_LOW_MAX) {
				TSL4531_sendRange(TSL4531_RANGE_MEDIUM, sensor);
				auto_range[sensor] = TSL4531_RANGE_MEDIUM;
			}
			break;
		case TSL4531_RANGE_MEDIUM:
			if (value >= TSL4531_RANGE_MEDIUM_MAX) {
				TSL4531_sendRange(TSL4531_RANGE_HIGH, sensor);
				auto_range[sensor] = TSL4531_RANGE_HIGH;
			} else if (value < TSL4531_RANGE_LOW_MAX - TSL4531_RANGE_HYSTERESIS) {
				TSL4531_sendRange(TSL4531_RANGE_LOW, sensor);
				auto_range[sensor] = TSL4531_RANGE_LOW;
			}
			break;
		case TSL4531_RANGE_HIGH:
			if (value < TSL4531_RANGE_MEDIUM_MAX - TSL4531_RANGE_HYSTERESIS) {
				TSL4531_sendRange(TSL4531_RANGE_MEDIUM, sensor);
				auto_range[sensor] = TSL4531_RANGE_MEDIUM;
			}
			break;
		default:
			break;
		}
	}

	return value;

}

/**
 * Set range of light sensor
 * @param new_range The range {@link tsl4531_range} to set
 */
void TSL4531_setRange(int new_range, int sensor) {
	TSL4531_sendRange(new_range, sensor);
	range[sensor] = new_range;
}

/**
 * Get current range
 * @return current range {@link tsl4531_range}
 */
int TSL4531_getRange(int sensor) {
	if(range[sensor] == TSL4531_RANGE_AUTO) {
		return auto_range[sensor];
	} else {
		return range[sensor];
	}
}

/**
 * Initiate an I2C communication
 * @param i2c_bus The I2C bus for communication
 * @return Status code
 */
int TSL4531_initComm(int i2c_bus, int sensor) {
	return ioctl(i2c_bus, I2C_SLAVE, stb_i2c_addresses[sensor]);
}

/**
 * Get the device ID
 * @param id The device id read
 * @return Status code
 */
uint8_t TSL4531_getID(int sensor) {

	return (uint8_t) i2c_smbus_read_byte_data(tsl4531_bus[sensor],
	TSL4531_COMMAND | TSL4531_REG_ID);
}

void TSL4531_setParams(int sensor) {

	i2c_smbus_write_byte_data(tsl4531_bus[sensor], TSL4531_COMMAND | TSL4531_REG_CONTROL,
	TSL4531_SAMPLE_CONTINUOUS);

	TSL4531_setRange(TSL4531_RANGE_AUTO, sensor);
}

void TSL4531_sendRange(int new_range, int sensor) {
	switch (new_range) {
	case TSL4531_RANGE_LOW:
		i2c_smbus_write_byte_data(tsl4531_bus[sensor], TSL4531_COMMAND | TSL4531_REG_CONFIG,
		TSL4531_INT_TIME_400 | TSL4531_LOW_POWER);
		multiplier[sensor] = TSL4531_MULT_400;
		break;
	case TSL4531_RANGE_MEDIUM:
		i2c_smbus_write_byte_data(tsl4531_bus[sensor], TSL4531_COMMAND | TSL4531_REG_CONFIG,
		TSL4531_INT_TIME_200 | TSL4531_LOW_POWER);
		multiplier[sensor] = TSL4531_MULT_200;
		break;
	case TSL4531_RANGE_HIGH:
		i2c_smbus_write_byte_data(tsl4531_bus[sensor], TSL4531_COMMAND | TSL4531_REG_CONFIG,
		TSL4531_INT_TIME_100 | TSL4531_LOW_POWER);
		multiplier[sensor] = TSL4531_MULT_100;
		break;
	case TSL4531_RANGE_AUTO:
		i2c_smbus_write_byte_data(tsl4531_bus[sensor], TSL4531_COMMAND | TSL4531_REG_CONFIG,
		TSL4531_INT_TIME_200 | TSL4531_LOW_POWER);
		multiplier[sensor] = TSL4531_MULT_200;
		auto_range[sensor] = TSL4531_RANGE_MEDIUM;
		break;
	default:
		break;
	}
}
