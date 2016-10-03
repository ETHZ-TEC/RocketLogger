#include "calibration.h"

// reset calibration
/**
 * Set all offsets to default state (0).
 * @return Indicates success.
 */
int reset_offsets() {
	int i;
	for (i=0; i< NUM_CALIBRATION_VALUES; i++) {
		offsets24[i] = 0;
		offsets16[i] = 0;
	}
	return SUCCESS;
}

/**
 * Set all scales to default state (1).
 * @return Indicates success.
 */
int reset_scales() {
	int i;
	for (i=0; i< NUM_CALIBRATION_VALUES; i++) {
		scales24[i] = 1;
		scales16[i] = 1;
	}
	return SUCCESS;
}

/**
 * Read in calibration file.
 * @return Indicates success.
 */
int read_calibration() {
	// check if calibration file existing
	if(open(CALIBRATION_FILE, O_RDWR) <= 0) {
		printf("Warning: no calibration file, returning uncalibrated values.\n");
		reset_offsets();
		reset_scales();
		return SUCCESS;
	}
	// open calibration file
	FILE* file = fopen(CALIBRATION_FILE, "r");
	if(file == NULL) {
		printf("Error: Error opening calibration file");
		return FAILURE;
	}
	// read values
	fread(offsets24, sizeof(int), NUM_CALIBRATION_VALUES, file);
	fread(scales24, sizeof(double), NUM_CALIBRATION_VALUES, file);
	
	// calculate values for high rates
	int i;
	for (i=0; i<NUM_CALIBRATION_VALUES; i++) {
		offsets16[i] = offsets24[i]/256;
		scales16[i] = scales24[i]*256;
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
		printf("Error: Error opening calibration file");
		return FAILURE;
	}
	// write values
	fwrite(offsets24, sizeof(int), NUM_CALIBRATION_VALUES, file);
	fwrite(scales24, sizeof(double), NUM_CALIBRATION_VALUES, file);
	
	//close file
	fclose(file);
	return SUCCESS;
}