"""
RocketLogger Python Library
Importing and plotting of rld data files.
"""

from datetime import timedelta, datetime, timezone
from math import ceil
from os.path import isfile, splitext

import numpy as np
# import matplotlib.pyplot as plt


class RocketLoggerData:
    """
    RocketLogger Data handling class
    """

    ROCKETLOGGER_FILE_MAGIC = 0x444C5225

    TIMESTAMP_SECONDS_BYTES = 8
    TIMESTAMP_NANOSECONDS_BYTES = 8

    FILE_MAGIC_BYTES = 4
    FILE_VERSION_BYTES = 2
    HEADER_LENGHT_BYTES = 2
    DATA_BLOCK_SIZE_BYTES = 4
    DATA_BLOCK_COUNT_BYTES = 4
    SAMPLE_COUNT_BYTES = 8
    SAMPLE_RATE_BYTES = 2
    MAC_ADDRESS_BYTES = 6
    COMMENT_LENGTH_BYTES = 4
    CHANNEL_BINARY_COUNT_BYTES = 2
    CHANNEL_ANALOG_COUNT_BYTES = 2

    CHANNEL_UNIT_INDEX_BYTES = 4
    CHANNEL_SCALE_BYTES = 4
    CHANNEL_DATA_BYTES_BYTES = 2
    CHANNEL_VALID_LINK_BYTES = 2
    CHANNEL_NAME_BYTES = 16

    CHANNEL_UNIT_NAMES = ['Undefined',
                          'Voltage',
                          'Current',
                          'Binary',
                          'Range Valid (binary)']

    def read_uint(self, file_handle, data_width):
        """
        Read an unsigned integer of defined data_size from file.
        file_handle:    The file handle to read from at current position.
        data_width:     The width in bytes of the integer to read.
        return value:   The integer read and decoded.
        """
        return int.from_bytes(file_handle.read(data_width), byteorder='little', signed=False)

    def read_int(self, file_handle, data_width):
        """
        Read a signed integer of defined data_size from file.
        file_handle:    The file handle to read from at current position.
        data_width:     The width in bytes of the integer to read.
        return value:   The integer read and decoded.
        """
        return int.from_bytes(file_handle.read(data_width), byteorder='little', signed=True)

    def read_str(self, file_handle, length):
        """
        Read a string of defined length from file.
        file_handle:    The file handle to read from at current position.
        data_width:     The length of the string to read.
        return value:   The string read and decoded.
        """
        return file_handle.read(length).decode(encoding='ascii').rstrip('\x00')

    def read_timestamp(self, file_handle):
        """
        Read a timestamp from the file.
        file_handle:    The file handle to read from at current position.
        return value:   The read date and time as datetime object.
        """
        seconds = self.read_uint(file_handle, self.TIMESTAMP_SECONDS_BYTES)
        nanoseconds = self.read_uint(file_handle, self.TIMESTAMP_NANOSECONDS_BYTES)
        timestamp = datetime.fromtimestamp(seconds, timezone.utc)
        timestamp = timestamp + timedelta(milliseconds=nanoseconds/1000)
        return timestamp

    def __init__(self, filename, decimation_factor=1):
        """
        Constructor to create a RockerLoggerData object form data file
        filename:           The filename of the file to import
        decimation_factor:  Decimation factor for values read
        """
        if filename and isfile(filename):
            self.read_file(filename, decimation_factor)
        else:
            self._channels = []

    def read_file_header(self, file_handle):
        """
        Read a RocketLogger data file's header, including comment and channels.
        file_handle:    The file handle to read from, with pointer positioned at file start
        return value:   Named struct containing the read file header data.
        """
        header = dict()

        header['file_magic'] = self.read_uint(file_handle, self.FILE_MAGIC_BYTES)
        header['file_version'] = self.read_uint(file_handle, self.FILE_VERSION_BYTES)

        # file consistency check
        if header['file_magic'] != self.ROCKETLOGGER_FILE_MAGIC:
            print('Invalid RocketLogger data file, file magic missmatch {0:x}.'
                  .format(header['file_magic']))

        # read static header fields
        header['header_length'] = self.read_uint(file_handle, self.HEADER_LENGHT_BYTES)
        header['data_block_size'] = self.read_uint(file_handle, self.DATA_BLOCK_SIZE_BYTES)
        header['data_block_count'] = self.read_uint(file_handle, self.DATA_BLOCK_COUNT_BYTES)
        header['sample_count'] = self.read_uint(file_handle, self.SAMPLE_COUNT_BYTES)
        header['sample_rate'] = self.read_uint(file_handle, self.SAMPLE_RATE_BYTES)
        header['mac_address'] = file_handle.read(self.MAC_ADDRESS_BYTES)
        header['start_time'] = self.read_timestamp(file_handle)
        header['comment_length'] = self.read_uint(file_handle, self.COMMENT_LENGTH_BYTES)
        header['channel_binary_count'] = self.read_uint(file_handle,
                                                        self.CHANNEL_BINARY_COUNT_BYTES)
        header['channel_analog_count'] = self.read_uint(file_handle,
                                                        self.CHANNEL_ANALOG_COUNT_BYTES)

        # header consistency checks
        if header['comment_length'] % 4 > 0:
            print('Missaligned comment length {0}.'.format(header['comment_length']))
            # header['comment_length'] = 4 * ceil(header['comment_length'] / 4)

        if header['sample_count'] != header['data_block_count'] * header['data_block_size']:
            print('Inconsistency in number of samples taken!')
            return dict()

        # read comment field
        header['comment'] = self.read_str(file_handle, header['comment_length'])

        # read channel headers
        header['channels'] = list()
        for ch in range(header['channel_binary_count'] + header['channel_analog_count']):
            channel = dict()
            channel['unit_index'] = self.read_int(file_handle, self.CHANNEL_UNIT_INDEX_BYTES)
            channel['scale'] = self.read_int(file_handle, self.CHANNEL_SCALE_BYTES)
            channel['data_width'] = self.read_uint(file_handle, self.CHANNEL_DATA_BYTES_BYTES)
            channel['valid_link'] = self.read_uint(file_handle, self.CHANNEL_VALID_LINK_BYTES)
            channel['name'] = self.read_str(file_handle, self.CHANNEL_NAME_BYTES)

            try:
                channel['unit'] = self.CHANNEL_UNIT_NAMES[channel['unit_index']]
            except:
                channel['unit_index'] = 0
                channel['unit'] = self.CHANNEL_UNIT_NAMES[0]

            header['channels'].append(channel)

        return header

    def read_file_data_block(self, file_handle, file_header):
        """
        Read a data block at the current position in the RocketLogger data file
        file_handle:    The file handle to read from, with pointer positioned at begining of block
        file_header:    The file's header with the data alignment details.
        return value:   Tuple of the block timestamp and data array containing the read data block.
        """

        timestamp = datetime.min
        data = np.zeros((file_header['data_block_size'], file_header['channel_binary_count'] +
                         file_header['channel_analog_count']))

        # decode block timestamp
        # TODO: convert to date time
        timestamp = self.read_timestamp(file_handle)

        # decode block data
        total_channel_count = ceil(file_header['channel_binary_count'] / 32) + \
            file_header['channel_analog_count']
        data_read_count = file_header['data_block_size'] * total_channel_count

        # TODO: read data
        data = np.fromfile(file_handle, dtype=np.uint32, count=data_read_count)

        # TODO: split and decode channels
        pass

        # TODO: decimation
        pass

        return timestamp, data

    def read_file(self, filename, decimation_factor=1):
        """
        Read a RocketLogger Data file and return an RLD object.
        filename:           The filename of the file to import
        decimation_factor:  Decimation factor for values read
        """

        file_basename, file_extension = splitext(filename)
        file_name = filename
        file_number = 0

        while isfile(file_name):
            with open(file_name, 'rb') as file_handle:
                # read file header
                header = self.read_file_header(file_handle)

                # consistency check: file stream position matches header size
                if 'header_length' not in header:
                    print('ERROR: No header read, aborting file import now.')
                    return

                stream_position = file_handle.tell()
                if stream_position != header['header_length']:
                    print('ERROR: File position {0} does not match header size {1}'
                          .format(stream_position, header['header_length']))
                    return

                # decimation
                if (header['data_block_size'] % decimation_factor) > 0:
                    print('ERROR: Decimation factor needs to be divider of the buffer size.')
                    return

                header['sample_count'] = ceil(header['sample_count'] / decimation_factor)
                header['data_block_size'] = ceil(header['data_block_size'] / decimation_factor)
                header['sample_rate'] = ceil(header['sample_rate'] / decimation_factor)

                # multi-file header
                if file_number == 0:
                    self._header = header
                else:
                    self._header['sample_count'] = self._header['sample_count'] + \
                        header['sample_count']
                    self._header['data_block_count'] = self._header['data_block_count'] + \
                        header['data_block_count']

                # channels: read data
                block_timestamp, block_data = self.read_file_data_block(file_handle, header)

            file_name = '{0}-p{1}.{2}'.format(file_basename, file_number, file_extension)
            file_number = file_number + 1

        print('Read {0} file(s)'.format(file_number))
