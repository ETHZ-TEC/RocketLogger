#include "meter.h"

char meter_names[10][15] = {"I1H","I1M","I1L","V1","V2","I2H","I2M","I2L","V3","V4"};
char channel_units[10][15] = {"mA","mA","uA","V","V","mA","mA","uA","V","V"};
int channel_scales[10] = {1000000, 1000000, 100000, 1000000, 1000000,1000000, 1000000, 100000, 1000000, 1000000};

char digital_input_names[6][15] = {"DigIn1", "DigIn2", "DigIn3", "DigIn4", "DigIn5", "DigIn6"};
int digital_input_bits[6] = {DIGIN1_BIT, DIGIN2_BIT, DIGIN3_BIT, DIGIN4_BIT, DIGIN5_BIT, DIGIN6_BIT};

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

void print_meter(struct rl_conf* conf, void* buffer_addr, unsigned int sample_size) {

	// clear screen
	erase();
	
	// counter variables
	int j = 0;
	int k = 2;
	int l = 0;
	int i = 0; // currents
	int v = 0; // voltages
	
	// data
	long value = 0;
	int line[12];
	
	// number of samples to average
	int avg_number = conf->sample_rate*RATE_SCALING / conf->update_rate;
	
	
	// read status
	line[0] = (int) (*((int8_t *) (buffer_addr)));
	line[1] = (int) (*((int8_t *) (buffer_addr + 1)));
	buffer_addr += STATUS_SIZE;
	
	
	// read, average and scale values (if channel selected)
	for(j=0; j<NUM_CHANNELS; j++) {
		if(conf->channels[j] > 0) {
			value = 0;
			if(sample_size == 4) {
				for(l=0; l<avg_number; l++) { // TODO: combine (with sample_size)
					value += *( (int32_t *) (buffer_addr + 4*j + l*(NUM_CHANNELS*sample_size + STATUS_SIZE)) );
				}
				value = value / (long)avg_number;
				line[k] = (int) (( (int) value + offsets[j] ) * scales[j]);
			} else {
				for(l=0; l<avg_number; l++) {
					value += *( (int16_t *) (buffer_addr + 2*j + l*(NUM_CHANNELS*sample_size + STATUS_SIZE)) );
				}
				value = value / (long)avg_number;
				line[k] = (int) (( value + offsets[j] ) * scales[j]);
			}
			k++;
		}
	}
	
	// display values
	mvprintw(1, 28, "RocketLogger Meter");
	
	for(j=0; j<NUM_CHANNELS; j++) {
		if(conf->channels[j] > 0) {
			if(is_current(j)) {
				// current
				mvprintw(i*2 + 5, 10, "%s:", meter_names[j]);
				mvprintw(i*2 + 5, 15, "%12.6f%s", ((float) line[v+i+2]) / channel_scales[j], channel_units[j]);
				i++;
			} else {
				// voltage
				mvprintw(v*2 + 5, 55, "%s:", meter_names[j]);
				mvprintw(v*2 + 5, 60, "%9.6f%s", ((float) line[v+i+2]) / channel_scales[j], channel_units[j]);
				v++;
			}
		}
		
	}
	
	// display titles, range information
	if(i > 0) { // currents
		mvprintw(3, 10, "Currents:");
		
		// display range information
		mvprintw(3, 33, "Low range:");
		if((line[0] & I1L_VALID_BIT) > 0) {
			mvprintw(5, 33, "I1L invalid");
		} else {
			mvprintw(5, 33, "I1L valid");
		}
		if((line[1] & I2L_VALID_BIT) > 0) {
			mvprintw(11, 33, "I2L invalid");
		} else {
			mvprintw(11, 33, "I2L valid");
		}
	}
	
	if(v > 0) { // voltages
		mvprintw(3, 55, "Voltages:");
	}
	
	// digital inputs
	mvprintw(20, 10, "Digital Inputs:");
	
	j=0;
	for( ; j<3; j++) {
		mvprintw(20 + 2*j, 30, "%s:", digital_input_names[j]);
		mvprintw(20 + 2*j, 38, "%d", (line[0] & digital_input_bits[j]) > 0);
	}
	for( ; j<6; j++) {
		mvprintw(20 + 2*(j-3), 50, "%s:", digital_input_names[j]);
		mvprintw(20 + 2*(j-3), 58, "%d", (line[1] & digital_input_bits[j]) > 0);
	}
	
    refresh();
}