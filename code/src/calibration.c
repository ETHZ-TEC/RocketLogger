#include "calibration.h"

// reset calibration
/**
 * Set all offsets to default state (0).
 */
void reset_offsets() {
	int i;
	for (i=0; i< NUM_CHANNELS; i++) {
		calibration.offsets[i] = 0;
	}
}

/**
 * Set all scales to default state (1).
 */
void reset_scales() {
	int i;
	for (i=0; i< NUM_CHANNELS; i++) {
		calibration.scales[i] = 1;
	}
}

/**
 * Read in calibration file.
 * @param conf Pointer to {@link rl_conf} struct.
 * @return {@link SUCCESS} in case of success, {@link FAILURE} otherwise.
 */
void read_calibration(struct rl_conf* conf) {

	// open calibration file
	FILE* file = fopen(CALIBRATION_FILE, "r");
	if(file == NULL) {
		// no calibration file available
		rl_log(WARNING, "no calibration file, returning uncalibrated values");
		reset_offsets();
		reset_scales();
		status.calibration_time = 0;
		return;
	}
	// read calibration
	fread(&calibration, sizeof(struct rl_calibration), 1, file);

	// reset calibration, if ignored
	if(conf->calibration == CAL_IGNORE) {
		reset_offsets();
		reset_scales();
	}

	// store timestamp to conf and status
	status.calibration_time = calibration.time;

	// calculate values for high rates
	if(conf->sample_rate == 32000 || conf->sample_rate == 64000) {
		int i;
		for (i=0; i<NUM_CHANNELS; i++) {
			calibration.offsets[i] = calibration.offsets[i]/256;
			calibration.scales[i] = calibration.scales[i]*256;
		}
	}
	
	//close file
	fclose(file);
}

