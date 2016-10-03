#include "rl_util.h"

void print_usage(struct rl_conf* conf) {
	printf("\nUsage:\n");
	printf("  rocketlogger [mode] -[option] [value]\n\n");
	printf("  Modes:\n");
	printf("    sample [number]    Acquires [number] of samples.\n");
	printf("    cont               Continuously acquires samples.\n");
	printf("    meter              Starts RocketLogger Meter.\n");
	printf("    status             Get status of RocketLogger.\n");
	printf("    calibrate          Reset RocketLogger calibration.\n");
	printf("    data               Get current data.\n");
	printf("    stop               Stops RocketLogger.\n");
	printf("    set                Set default configuration of RocketLogger (use normal options).\n");
	printf("                       Use set 0 to reset the default configuration.\n");
	printf("    print              Print default configuration of RocketLogger.\n");
	
	printf("\n  Options:\n");
	printf("    -f [file]          Stores data to specified file.\n");
	printf("                       -f 0 will disable file storing.\n");
	printf("    -r [number]        Acquisition rate selection.\n");
	printf("                       Possible rates: 1, 2, 4, 8, 16, 32, 64 (in kSps)\n");
	printf("    -u [number]        Data update rate selection.\n");
	printf("                       Possible update rates: 1, 2, 5, 10 (in Hz)\n");
	printf("    -ch [number1,...]  Channel selection.\n");
	printf("                       Possible channels:\n");
	printf("                         0: I1H\t\t5: I2H\n");
	printf("                         1: I1M\t\t6: I2M\n");
	printf("                         2: I1L\t\t7: I2L\n");
	printf("                         3: V1 \t\t8: V3\n");
	printf("                         4: V2 \t\t9: V4\n");
	printf("    -fhr [0,1,2]       Force high-range.\n");
	printf("                         0: no channel, 1: I1, 2: I2\n");
	printf("    -w                 Enable webserver plotting.\n"); // TODO: allow disable
	printf("    -b                 Set output file to binary.\n"); // TODO: expand selection
	printf("    -s                 Set configuration as default.\n");
	
	printf("\n");
}

void print_config(struct rl_conf* conf) {
	printf("\nRocketLogger Configuration:\n");
	rl_print_config(conf, 0);
	printf("\n");
	
}


void reset_config(struct rl_conf* conf) {
	conf->mode = CONTINUOUS;
	conf->sample_rate = 1;
	conf->update_rate = 1;
	conf->sample_limit = 0;
	conf->enable_web_server = 1;
	conf->file_format = BIN;
	
	strcpy(conf->file_name, "/var/www/data/data.dat");
	
	memset(conf->channels, 1, sizeof(conf->channels));
	memset(conf->force_high_channels, 0, sizeof(conf->force_high_channels));
	
}

int read_default_config(struct rl_conf* conf) {
	
	// check if config file existing
	if(open(DEFAULT_CONFIG, O_RDWR) <= 0) {
		reset_config(conf);
		return UNDEFINED;
	}
	
	// open config file
	FILE* file = fopen(DEFAULT_CONFIG, "r");
	if(file == NULL) {
		printf("Error opening configuration file.\n");
		return FAILURE;
	}
	// read values
	fread(conf, sizeof(struct rl_conf), 1, file);
	
	// reset mode
	conf->mode = CONTINUOUS;
	
	//close file
	fclose(file);
	return SUCCESS;
}

int write_default_config(struct rl_conf* conf) {
	
	// open config file
	FILE* file = fopen(DEFAULT_CONFIG, "w");
	if(file == NULL) {
		printf("Error creating configuration file.\n");
		return FAILURE;
	}
	// write values
	fwrite(conf, sizeof(struct rl_conf), 1, file);
	
	//close file
	fclose(file);
	return SUCCESS;
}