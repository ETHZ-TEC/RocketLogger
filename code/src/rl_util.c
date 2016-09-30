#include "rl_util.h"

int is_running() {
	// check if RL already running
	if(rl_get_status(0,0) == 1) {
		return 1;
	}
	return 0;
}

void print_usage(struct rl_conf* conf) {
	printf("Usage:\n");
	printf("  rocketlogger [mode] -[option] [value]\n\n");
	printf("  Modes:\n");
	printf("    sample [number]    Acquires [number] of samples.\n");
	printf("    continuous         Continuously acquires samples.\n");
	printf("    meter              Starts RocketLogger Meter.\n");
	printf("    status             Get status of RocketLogger.\n");
	printf("    calibrate          Reset RocketLogger calibration.\n");
	printf("    data               Get current data.\n");
	printf("    stop               Stops RocketLogger.\n");
	printf("    set                Set default configuration of RocketLogger (use normal options).\n");
	printf("    print              Print default configuration of RocketLogger.\n");
	
	printf("  Options:\n");
	printf("    -f [file]          Stores data to specified file (default: %s)\n", conf->file_name);
	printf("                       -f 0 will disable file storing.\n");
	printf("    -r [number]        Acquisition rate selection (default %dkSps).\n", conf->sample_rate);
	printf("                       Possible rates: 1, 2, 4, 8, 16, 32, 64 (in kSps)\n");
	printf("    -u [number]        Data update rate selection (default %dHz).\n", conf->update_rate);
	printf("                       Possible update rates: 1, 2, 5, 10 (in Hz)\n");
	printf("    -ch [number1,...]  Channel selection (default: TODO).\n"); // TODO: print channels
	printf("                       Possible channels:\n");
	printf("                         0: I1H\t\t5: I2H\n");
	printf("                         1: I1M\t\t6: I2M\n");
	printf("                         2: I1L\t\t7: I2L\n");
	printf("                         3: V1 \t\t8: V3\n");
	printf("                         4: V2 \t\t9: V4\n");
	printf("    -fhr [1,2]         Force high-range.\n"); // TODO: add 0 to reset fhr
	printf("                       1: I1, 2: I2\n");
	printf("    -w                 Enable webserver plotting.\n"); // TODO: allow disable
	printf("    -b                 Set output file to binary.\n"); // TODO: expand selection
	printf("    -s                 Set configuration as default.\n"); // TODO: reset configuration
	
}

void print_config(struct rl_conf* conf) {
	printf("\nRocketLogger Configuration:\n");
	rl_print_config(conf, 0);
	
}


void reset_config(struct rl_conf* conf) {
	conf->mode = CONTINUOUS;
	conf->sample_rate = 1;
	conf->update_rate = 1;
	conf->number_samples = 0;
	conf->enable_web_server = 1;
	conf->file_format = BIN;
	
	strcpy(conf->file_name, "/var/www/data/data.dat");
	
	memset(conf->channels, 1, sizeof(conf->channels));
	memset(conf->force_high_channels, 0, sizeof(conf->force_high_channels));
	
	return;
}

int read_default_config(struct rl_conf* conf) {
	
	// check if config file existing
	if(open(DEFAULT_CONFIG, O_RDWR) <= 0) {
		reset_config(conf);
		return 0;
	}
	
	// open config file
	FILE* file = fopen(DEFAULT_CONFIG, "r");
	if(file == NULL) {
		printf("Error opening configuration file.\n");
		return -1;
	}
	// read values
	fread(conf, sizeof(struct rl_conf), 1, file);
	
	// reset mode
	conf->mode = CONTINUOUS;
	
	//close file
	fclose(file);
	return 1;
}

int write_default_config(struct rl_conf* conf) {
	
	// open config file
	FILE* file = fopen(DEFAULT_CONFIG, "w");
	if(file == NULL) {
		printf("Error creating configuration file.\n");
		return -1;
	}
	// write values
	fwrite(conf, sizeof(struct rl_conf), 1, file);
	
	//close file
	fclose(file);
	return 1;
}