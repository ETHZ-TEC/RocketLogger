#include "meter.h"

char meter_names[10][15] = {"I1H","I1M","I1L","V1","V2","I2H","I2M","I2L","V3","V4"};
char meter_units[10][15] = {"mA","mA","uA","V","V","mA","mA","uA","V","V"};
int meter_scales[10] = {1000000, 1000000, 100000, 1000000, 1000000,1000000, 1000000, 100000, 1000000, 1000000};

void meter_init() {
	// init ncurses mode
	initscr();
	// hide cursor
	curs_set(0);
	
	mvprintw(1, 1, "Starting RocketLogger Meter ...");
	refresh();
}

void meter_stop() {
	endwin();
}

void print_meter(struct rl_conf* conf, void* virt_addr, unsigned int sample_size) {

	// clear screen
	erase();
	
	// counter variables
	int j = 0;
	int k = 2;
	int l = 0;
	
	// data
	long value = 0;
	int line[12];
	
	// number of samples to average
	int avg_number = conf->sample_rate*RATE_SCALING / conf->update_rate;
	
	
	// read status
	line[0] = (int) (*((int8_t *) (virt_addr)));
	line[1] = (int) (*((int8_t *) (virt_addr + 1)));
	virt_addr += STATUS_SIZE;
	
	
	// read, average and scale values (if channel selected)
	for(j=0; j<NUM_CHANNELS; j++) {
		if(conf->channels[j] > 0) {
			value = 0;
			if(sample_size == 4) {
				for(l=0; l<avg_number; l++) {
					value += *( (int32_t *) (virt_addr + 4*j + l*(NUM_CHANNELS*sample_size + STATUS_SIZE)) );
				}
				value = value / (long)avg_number;
				line[k] = (int) (( (int) value + offsets24[j] ) * scales24[j]);
			} else {
				for(l=0; l<avg_number; l++) {
					value += *( (int16_t *) (virt_addr + 2*j + l*(NUM_CHANNELS*sample_size + STATUS_SIZE)) );
				}
				value = value / (long)avg_number;
				line[k] = (int) (( value + offsets16[j] ) * scales16[j]);
			}
			k++;
		}
	}
	
	// display values
	mvprintw(1, 28, "RocketLogger Meter");
	
	int i = 0;
	int v = 0;
	for(j=0; j<NUM_CHANNELS; j++) {
		if(conf->channels[j] > 0) {
			if(j == 0 || j == 1 || j == 2 || j == 5 || j == 6 || j == 7) {
				// current
				mvprintw(i*2 + 5, 10, "%s", meter_names[j]);
				mvprintw(i*2 + 5, 15, "%f%s", ((float) line[v+i+2]) / meter_scales[j], meter_units[j]);
				i++;
			} else {
				// voltage
				mvprintw(v*2 + 5, 55, "%s", meter_names[j]);
				mvprintw(v*2 + 5, 60, "%f%s", ((float) line[v+i+2]) / meter_scales[j], meter_units[j]);
				v++;
			}
		}
		
	}
	
	// display titles, range information
	if(i > 0) { // currents
		mvprintw(3, 10, "Currents:");
		
		// display range information
		mvprintw(3, 30, "Low range:");
		if((line[0] & 1) > 0) {
			mvprintw(5, 30, "I1L valid");
		} else {
			mvprintw(5, 30, "I1L invalid");
		}
		if((line[1] & 1) > 0) {
			mvprintw(11, 30, "I2L valid");
		} else {
			mvprintw(11, 30, "I2L invalid");
		}
	}
	
	if(v > 0) { // voltages
		mvprintw(3, 55, "Voltages:");
	}
	
    refresh();
}