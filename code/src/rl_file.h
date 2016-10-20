#ifndef RL_FILE_H
#define RL_FILE_H

#include <stdint.h>

#include "types.h"


// Defines

/// MAC address length in bytes
#define MAC_ADDRESS_LENGTH 6

/**
 * Channel scaling definitions
 */
#define RL_SCALE_PICO -12
#define RL_SCALE_NANO -9
#define RL_SCALE_MICRO -6
#define RL_SCALE_MILLI -3
#define RL_SCALE_NONE 0
#define RL_SCALE_KILO 3
#define RL_SCALE_MEGA 6
#define RL_SCALE_GIGA 9
#define RL_SCALE_TERA 12


// Constants

/// File header magic number (ascii %RLB)
const uint32_t RL_FILE_MAGIC = 0x25524C42;

/// File format version of current implementation
const uint8_t RL_FILE_VERSION = 0x01;

/// Maximum number of binary channels supported (TODO: remove?)
const uint8_t RL_FILE_BIN_MAX_CHANNEL_COUNT = 8;

/// Maximum number of channels supported (TODO: remove?)
const uint8_t RL_FILE_MAX_CHANNEL_COUNT = 16;

/// Maximum channel description length
const uint8_t RL_FILE_CHANNEL_NAME_LENGTH = 16;


// Types

/**
 * Data unit definition
 */
typedef enum rl_unit {
	RL_UNIT_UNDEFINED = 0,
	RL_UNIT_VOLT,
	RL_UNIT_AMPERE,
	RL_UNIT_BINARY,
	//RL_UNIT_WATT,
} rl_unit;




/**
 * File header definition for the binary file.
 */
struct rl_file_header {
	
	// constant size fields
	
	struct rl_file_lead_in lead_in;

	// dynamic size fields

	/// comment field
	char* comment;

	/// Channels definitions (binary and normal)
	rl_file_channel* channel;
};

/**
 * File header lead in (constant size) definition for the binary file.
 */
struct rl_file_lead_in {
	/// file magic constant
	const uint32_t magic = MAGIC_NUMBER;

	/// file version number
	uint8_t file_version = 0x01;

	/// channel version number
	uint8_t channel_version = 0x01;

	/// total size of the header in bytes
	uint16_t header_length = sizeof(rl_file_header) / sizeof(uint8_t);

	/// size of the data blocks in the file in rows
	uint32_t data_block_size = 0;

	/// number of data blocks stored in the file
	uint32_t data_block_count = 0;

	/// total sample count
	uint64_t sample_count = 0;

	/// the sample rate of the measurement
	uint16_t sample_rate = 0;
	
	/// instrument id (mac address)
	uint8_t mac_address[MAC_ADDRESS_LENGTH];

	/// start time of the measurement in UNIT time, UTC
	uint32_t start_time = 0;

	/// comment length
	uint32_t comment_length = 0;

	/// binary channel count
	uint16_t bin_channel_count = RL_FILE_BIN_MAX_CHANNEL_COUNT;

	/// number of channels in the file
	uint16_t channel_count = RL_FILE_MAX_CHANNEL_COUNT;
	
};


/**
 * Channel definition for the binary file header.
 */
struct rl_file_channel {
	
	/// channel unit (binary, voltage or current)
	rl_unit unit = RL_UNIT_UNDEFINED;
	
	/// datum size in bytes (for voltage and current)
	uint8_t data_size = 0;
	
	/// valid link (for low-range current channels)
	uint8_t valid_data_channel = -1;

	/// channel scale (in power of ten, for voltage and current)
	int32_t channel_scale = RL_SCALE_NONE;

	// channel name/description
	char name[RL_FILE_CHANNEL_NAME_LENGTH];
};


// --------------------------------------------------------
/*
#define MAX_CHANNEL_NAME 50
struct channel_header {
	char name[MAX_CHANNEL_NAME];
	int type; //current, voltage, digital input, range ???
	int unit; //scaling ???
	int offset; //position in data block???
	// valid info?
	// range forced?
	// ...
};

#define MAGIC_NUMBER 0x42420000
#define HEADER_VERSION 0
#define MAX_CHANNEL_COUNT 1 // TODO: adapt
struct file_header_new {
	// header information
	int version_number;
	int total_header_length; // ???
	int header_length;
	// data information
	int buffer_count;
	int buffer_size;
	// additional information
	int sample_rate;
	int precision; //??
	// channel information
	int channel_count;
	int channel_header_size;
	struct channel_header channel[MAX_CHANNEL_COUNT]; // variable channel count?
};
*/

#endif /* RL_FILE_H */