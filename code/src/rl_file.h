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
#define RL_SCALE_TEN_PICO -11
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
#define RL_FILE_MAGIC 0x25524C42 //const uint32_t RL_FILE_MAGIC = 0x25524C42;

/// File format version of current implementation
#define RL_FILE_VERSION 0x01 //const uint8_t RL_FILE_VERSION = 0x01;

/// Maximum channel description length
#define RL_FILE_CHANNEL_NAME_LENGTH 16 //const uint8_t RL_FILE_CHANNEL_NAME_LENGTH = 16;

#define NO_VALID_DATA 0xFFFF //const uint16_t NO_VALID_DATA = 0xFFFF;

/// Comment length
#define RL_FILE_COMMENT "This is a comment"


// Types

/**
 * Data unit definition
 */
typedef enum unit {
	RL_UNIT_UNDEFINED = 0,
	RL_UNIT_VOLT = 1,
	RL_UNIT_AMPERE = 2,
	RL_UNIT_BINARY = 3,
	RL_UNIT_RANGE_VALID = 4,
} rl_unit;

/**
 * Time stamp definition (UNIX time)
 */
struct time_stamp {
	int64_t sec;
	int64_t nsec;
};




/**
 * File header lead in (constant size) definition for the binary file.
 */
struct rl_file_lead_in {
	/// file magic constant
	uint32_t magic; // = RL_FILE_MAGIC;

	/// file version number
	uint16_t file_version; // = RL_FILE_VERSION;

	/// total size of the header in bytes
	uint16_t header_length; // = 0;

	/// size of the data blocks in the file in rows
	uint32_t data_block_size; // = 0;

	/// number of data blocks stored in the file
	uint32_t data_block_count; // = 0;

	/// total sample count
	uint64_t sample_count; // = 0;

	/// the sample rate of the measurement
	uint16_t sample_rate; // = 0;
	
	/// instrument id (mac address)
	uint8_t mac_address[MAC_ADDRESS_LENGTH];

	/// start time of the measurement in UNIT time, UTC
	struct time_stamp start_time; // = 0;

	/// comment length
	uint32_t comment_length; // = 0;

	/// binary channel count
	uint16_t channel_bin_count; // = 0;

	/// number of channels in the file
	uint16_t channel_count; // = 0;
	
};


/**
 * Channel definition for the binary file header.
 */
struct rl_file_channel {
	
	/// channel unit (binary, voltage or current)
	rl_unit unit; // = RL_UNIT_UNDEFINED;
	
	/// channel scale (in power of ten, for voltage and current)
	int32_t channel_scale; // = RL_SCALE_NONE;
	
	/// datum size in bytes (for voltage and current)
	uint16_t data_size; // = 0;
	
	/// link to channel valid data (for low-range current channels)
	uint16_t valid_data_channel; // = NO_VALID_DATA;

	/// channel name/description
	char name[RL_FILE_CHANNEL_NAME_LENGTH];
};

/**
 * File header definition for the binary file.
 */
struct rl_file_header {
	
	/// constant size file header lead in
	struct rl_file_lead_in lead_in;

	/// comment field
	char* comment; // = NULL;

	/// Channels definitions (binary and normal)
	struct rl_file_channel* channel; // = NULL;
};

#endif /* RL_FILE_H */
