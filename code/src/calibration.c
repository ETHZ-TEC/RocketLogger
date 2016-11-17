#include "calibration.h"

// reset calibration
/**
 * Set all offsets to default state (0).
 * @return Indicates success.
 */
void reset_offsets() {
	int i;
	for (i=0; i< NUM_CHANNELS; i++) {
		offsets[i] = 0;
	}
}

/**
 * Set all scales to default state (1).
 * @return Indicates success.
 */
void reset_scales() {
	int i;
	for (i=0; i< NUM_CHANNELS; i++) {
		scales[i] = 1;
	}
}

/**
 * Read in calibration file.
 * @return Indicates success.
 */
int read_calibration(struct rl_conf* conf) {
	// check if calibration file existing
	if(open(CALIBRATION_FILE, O_RDWR) <= 0) {
		rl_log(WARNING, "no calibration file, returning uncalibrated values");
		reset_offsets();
		reset_scales();
		return SUCCESS;
	}
	// open calibration file
	FILE* file = fopen(CALIBRATION_FILE, "r");
	if(file == NULL) {
		rl_log(ERROR, "failed to open calibration file");
		return FAILURE;
	}
	// read values
	fread(offsets, sizeof(int32_t), NUM_CHANNELS, file);
	fread(scales, sizeof(double), NUM_CHANNELS, file);
	
	// calculate values for high rates
	if(conf->sample_rate == 32000 || conf->sample_rate == 64000) {
		int i;
		for (i=0; i<NUM_CHANNELS; i++) {
			offsets[i] = offsets[i]/256;
			scales[i] = scales[i]*256;
		}
	}
	
	//close file
	fclose(file);
	return SUCCESS;
}

/**
 * Write calibration values to file.
 * @return Indicates success.
 */
int write_calibration() {
	// open calibration file
	FILE* file = fopen(CALIBRATION_FILE, "w+");
	if(file == NULL) {
		rl_log(ERROR, "failed to open calibration file");
		return FAILURE;
	}
	// write values
	fwrite(offsets, sizeof(int32_t), NUM_CHANNELS, file);
	fwrite(scales, sizeof(double), NUM_CHANNELS, file);
	
	//close file
	fclose(file);
	return SUCCESS;
}
