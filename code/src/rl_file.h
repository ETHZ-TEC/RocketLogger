#ifndef RL_FILE_H
#define RL_FILE_H

#include <stdint.h>

#include "types.h"


// Defines


// Constants

/// File header magic number (ascii %RLB)
const uint32_t RL_FILE_MAGIC = 0x25524C42;

/// File format version of current implementation
const uint8_t RL_FILE_VERSION = 0x01;


/// Maximum number of channels supported
const uint8_t RL_FILE_MAX_CHANNEL_COUNT = 16;

/// Maximum channel description lengRL_FILE_CHANNELS_NAME_LENGTHth
const uint8_t RL_FILE_CHANNEL_NAME_LENGTH = 16;


// Types

/**
 * Data unit definition
 */
typedef enum rl_unit {
	RL_UNIT_UNDEFINED = 0,
	RL_UNIT_VOLT,
	RL_UNIT_AMPERE,
	RL_UNIT_WATT,
} rl_unit;

/**
 * Data scaling definitions
 */
typedef enum {
	RL_SCALE_PICO = -12,
	RL_SCALE_NANO = -9,
	RL_SCALE_MICRO = -6,
	RL_SCALE_MILLI = -3,
	RL_SCALE_NONE = 0,
	RL_SCALE_KILO = 3,
	RL_SCALE_MEGA = 6,
	RL_SCALE_GIGA = 9,
	RL_SCALE_TERA = 12,
} rl_scale;

/**
 * File header definition for the binary file.
 */
struct rl_file_header {
	/// file magic constant
	const uint32_t magic = MAGIC_NUMBER;

	/// file version number
	uint8_t file_version = 0x01;

	/// file version number
	uint8_t channel_version = 0x01;

	/// total size of the header in bytes
	uint16_t header_length = sizeof(rl_file_header) / sizeof(uint8_t);

	/// size of the data blocks in the file in rows (or better bytes?)
	uint32_t data_block_size = 0;

	/// number of data blocks stored in the file
	uint32_t data_block_count = 0;

	/// total sample count
	uint64_t sample_count = 0;

	/// the sample rate of the measurement
	uint16_t sample_rate = 0;

	/// start time of the measurement in UNIT time, UTC
	uint32_t start_time = 0;

	/// instrument id (number)
	// TODO

	/// comment length
	// TODO

	/// binary channel count
	// TODO

	/// number of channels in the file
	uint8_t channel_count = RL_FILE_MAX_CHANNEL_COUNT;


	// dynamic size fields

	/// comment field
	char* comment;

	/// Channels definitions
	rl_file_channel* channel;
}

/**
 * Channel definition for the binary file header.
 */
struct rl_file_channel {
	/// datum size in bytes
	uint8_t data_size;


	/// unit
	
/// valid link

	/// datum offset
	uint32_t data_offset; /// TODO: even possible without knowing the data size?

	/// datum scale
	uint32_t data_scale; /// TODO: as enum? even possible without knowing the data size?

	// channel name/description
	char name[RL_FILE_CHANNEL_NAME_LENGTH];
};


// --------------------------------------------------------

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


#endif /* RL_FILE_H */