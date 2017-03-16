"""
RocketLogger Python Library
Importing and plotting of rld data files.

Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group

"""

from datetime import datetime, timezone
from math import ceil
from os.path import isfile, splitext

import numpy as np
# import matplotlib.pyplot as plt


_ROCKETLOGGER_FILE_MAGIC = 0x444C5225

_TIMESTAMP_SECONDS_BYTES = 8
_TIMESTAMP_NANOSECONDS_BYTES = 8
_TIMESTAMP_BYTES = _TIMESTAMP_SECONDS_BYTES + _TIMESTAMP_NANOSECONDS_BYTES

_FILE_MAGIC_BYTES = 4
_FILE_VERSION_BYTES = 2
_HEADER_LENGHT_BYTES = 2
_DATA_BLOCK_SIZE_BYTES = 4
_DATA_BLOCK_COUNT_BYTES = 4
_SAMPLE_COUNT_BYTES = 8
_SAMPLE_RATE_BYTES = 2
_MAC_ADDRESS_BYTES = 6
_COMMENT_LENGTH_BYTES = 4
_CHANNEL_BINARY_COUNT_BYTES = 2
_CHANNEL_ANALOG_COUNT_BYTES = 2

_CHANNEL_UNIT_INDEX_BYTES = 4
_CHANNEL_SCALE_BYTES = 4
_CHANNEL_DATA_BYTES_BYTES = 2
_CHANNEL_VALID_LINK_BYTES = 2
_CHANNEL_NAME_BYTES = 16

_CHANNEL_UNIT_NAMES = ['Undefined',
                       'Voltage',
                       'Current',
                       'Binary',
                       'Range Valid (binary)']
_CHANNEL_IS_BINARY = [False, False, False, True, True]


def _read_uint(file_handle, data_size):
    """
    Read an unsigned integer of defined data_size from file.
    file_handle:    The file handle to read from at current position.
    data_size:      The data size in bytes of the integer to read.
    return value:   The integer read and decoded.
    """
    return int.from_bytes(file_handle.read(data_size), byteorder='little', signed=False)


def _read_int(file_handle, data_size):
    """
    Read a signed integer of defined data_size from file.
    file_handle:    The file handle to read from at current position.
    data_size:      The data size in bytes of the integer to read.
    return value:   The integer read and decoded.
    """
    return int.from_bytes(file_handle.read(data_size), byteorder='little', signed=True)


def _read_str(file_handle, length):
    """
    Read a string of defined length from file.
    file_handle:    The file handle to read from at current position.
    length:         The length of the string to read.
    return value:   The string read and decoded.
    """
    raw_bytes = file_handle.read(length)
    return raw_bytes.split(b'\x00')[0].decode(encoding='ascii')


def _read_timestamp(file_handle):
    """
    Read a timestamp from the file.
    file_handle:    The file handle to read from at current position.
    return value:   The read date and time as datetime object.
    """
    seconds = _read_int(file_handle, _TIMESTAMP_SECONDS_BYTES)
    nanoseconds = _read_int(file_handle, _TIMESTAMP_NANOSECONDS_BYTES)
    return datetime.fromtimestamp(seconds + 1e-9*nanoseconds, timezone.utc)


class RocketLoggerData:
    """
    RocketLogger Data handling class
    """

    def __init__(self, filename, decimation_factor=1):
        """
        Constructor to create a RockerLoggerData object form data file
        filename:           The filename of the file to import
        decimation_factor:  Decimation factor for values read
        """
        if filename and isfile(filename):
            self.load_file(filename, decimation_factor)
        else:
            self._header = dict()
            self._timstamps_realtime = list()
            self._timestamps_monotonic = list()
            self._data = np.zeros((0, 0))

    def _read_file_header(self, file_handle):
        """
        Read a RocketLogger data file's header, including comment and channels.
        file_handle:    The file handle to read from, with pointer positioned at file start
        return value:   Named struct containing the read file header data.
        """
        header = dict()

        header['file_magic'] = _read_uint(file_handle, _FILE_MAGIC_BYTES)
        header['file_version'] = _read_uint(file_handle, _FILE_VERSION_BYTES)

        # file consistency check
        if header['file_magic'] != _ROCKETLOGGER_FILE_MAGIC:
            print('Invalid RocketLogger data file, file magic missmatch {0:x}.'
                  .format(header['file_magic']))

        # read static header fields
        header['header_length'] = _read_uint(file_handle, _HEADER_LENGHT_BYTES)
        header['data_block_size'] = _read_uint(file_handle, _DATA_BLOCK_SIZE_BYTES)
        header['data_block_count'] = _read_uint(file_handle, _DATA_BLOCK_COUNT_BYTES)
        header['sample_count'] = _read_uint(file_handle, _SAMPLE_COUNT_BYTES)
        header['sample_rate'] = _read_uint(file_handle, _SAMPLE_RATE_BYTES)
        header['mac_address'] = bytearray(file_handle.read(_MAC_ADDRESS_BYTES))
        header['start_time'] = _read_timestamp(file_handle)
        header['comment_length'] = _read_uint(file_handle, _COMMENT_LENGTH_BYTES)
        header['channel_binary_count'] = _read_uint(file_handle, _CHANNEL_BINARY_COUNT_BYTES)
        header['channel_analog_count'] = _read_uint(file_handle, _CHANNEL_ANALOG_COUNT_BYTES)

        # header consistency checks
        if header['comment_length'] % 4 > 0:
            print('Comment length unaligned {0}.'.format(header['comment_length']))

        if header['sample_count'] != header['data_block_count'] * header['data_block_size']:
            print('Inconsistency in number of samples taken!')
            return dict()

        # read comment field
        header['comment'] = _read_str(file_handle, header['comment_length'])

        # read channel headers
        header['channels'] = list()
        for ch in range(header['channel_binary_count'] + header['channel_analog_count']):
            channel = dict()
            channel['unit_index'] = _read_int(file_handle, _CHANNEL_UNIT_INDEX_BYTES)
            channel['scale'] = _read_int(file_handle, _CHANNEL_SCALE_BYTES)
            channel['data_size'] = _read_uint(file_handle, _CHANNEL_DATA_BYTES_BYTES)
            channel['valid_link'] = _read_uint(file_handle, _CHANNEL_VALID_LINK_BYTES)
            channel['name'] = _read_str(file_handle, _CHANNEL_NAME_BYTES)

            try:
                channel['unit'] = _CHANNEL_UNIT_NAMES[channel['unit_index']]
            except:
                channel['unit_index'] = 0
                channel['unit'] = _CHANNEL_UNIT_NAMES[0]

            header['channels'].append(channel)

        return header

    def _read_file_data(self, file_handle, file_header, decimation=1):
        """
        Read a data block at the current position in the RocketLogger data file
        file_handle:    The file handle to read from, with pointer positioned at begining of block
        file_header:    The file's header with the data alignment details.
        return value:   Tuple of the realtime timestamp, monotonic timestamp and
                        the actual data array containing the read sample data.
        """

        # generate data type to read from header info
        total_bin_bytes = 4 * ceil(file_header['channel_binary_count'] / 32)

        if total_bin_bytes > 0:
            type_names = ['bin']
            type_formats = ['<u{0}'.format(total_bin_bytes)]
        else:
            type_names = []
            type_formats = []

        for channel in file_header['channels']:
            if not _CHANNEL_IS_BINARY[channel['unit_index']]:
                type_names.append(channel['name'])
                type_formats.append('<i{0}'.format(channel['data_size']))

        # read raw data from file
        data_type = np.dtype({'names': type_names, 'formats': type_formats})

        # allocate the data
        timestamps_realtime = list()
        timestamps_monotonic = list()
        data = np.empty((file_header['data_block_count'] * file_header['data_block_size'],
                        file_header['channel_binary_count'] + file_header['channel_analog_count']))

        # iterate over all data blocks
        for block_index in range(file_header['data_block_count']):
            block_offset_bytes = file_header['header_length'] + block_index * \
                (2 * _TIMESTAMP_BYTES + file_header['data_block_size'] * data_type.itemsize)
            data_index_start = block_index * file_header['data_block_size']
            data_index_end = data_index_start + file_header['data_block_size']

            # read block timestamp
            file_handle.seek(block_offset_bytes)
            timestamps_realtime.append(_read_timestamp(file_handle))
            timestamps_monotonic.append(_read_timestamp(file_handle))

            # access file data memory mapped
            # data_raw = np.fromfile(file_handle, dtype=data_type,
            #                        count=file_header['data_block_size'])
            file_data = np.memmap(file_handle, offset=block_offset_bytes +
                                  2 * _TIMESTAMP_BYTES, mode='r', dtype=data_type,
                                  shape=file_header['data_block_size'])

            # extract binary channels
            # binary_data_bool = np.unpackbits(file_data['bin']).reshape(
            #     (file_header['data_block_size'], 8 * total_bin_bytes))

            # TODO: map to data type used ot store binary value
            for binary_channel_index in range(file_header['channel_binary_count']):
                channel_index = binary_channel_index
                data[data_index_start:data_index_end, channel_index] = np.array(
                    2**binary_channel_index & file_data['bin'], dtype=data.dtype)

            for analog_channel_index in range(file_header['channel_analog_count']):
                channel_index = file_header['channel_binary_count'] + analog_channel_index
                channel_name = type_names[analog_channel_index + (total_bin_bytes > 0)]
                data[data_index_start:data_index_end, channel_index] = \
                    10**file_header['channels'][channel_index]['scale'] * file_data[channel_name]

            # extract analog channels
            pass

            # decimate block data
            # TODO: decimantion
            pass

        return timestamps_realtime, timestamps_monotonic, data

    def load_file(self, filename, decimation_factor=1):
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
                header = self._read_file_header(file_handle)

                # consistency check: file stream position matches header size
                if 'header_length' not in header:
                    print('ERROR: No header read, aborting file import now.')
                    return

                stream_position = file_handle.tell()
                if stream_position != header['header_length']:
                    print('ERROR: File position {0} does not match header size {1}'
                          .format(stream_position, header['header_length']))
                    return

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

                # channels: read actual sampled data
                timestamps_realtime, timestamps_monotonic, data = \
                    self._read_file_data(file_handle, header)

                # create on first file, append on following
                if file_number > 0:
                    self._timstamps_realtime.append(timestamps_realtime)
                    self._timstamps_monotonic.append(timestamps_monotonic)
                    self._data = np.vstack((self._data, data))
                else:
                    self._timstamps_realtime = timestamps_realtime
                    self._timstamps_monotonic = timestamps_monotonic
                    self._data = data

            file_number = file_number + 1
            file_name = '{0}_p{1}{2}'.format(file_basename, file_number, file_extension)

        print('Read {0} file(s)'.format(file_number))

    def get_channel_names(self):
        """
        Get the names of all the channels loaded from file
        return value:   List of all channel names
        """
        channel_names = list()
        for channel in self._header['channels']:
            channel_names.append(channel['name'])

        return channel_names

    def get_channel_index(self, channel_name):
        """
        Get the index of the channel
        channel_name:   Name of the channel
        return value:   The index of the channel, None if not found
        """

        channel_names = [channel['name'] for channel in self._header['channels']]
        try:
            channel_index = channel_names.index(channel_name)
        except ValueError:
            channel_index = None

        return channel_index

    def get_data(self, channel_names=['all']):
        """
        Get the data of the specified channels, by default of all channels.
        channel_names:  The names of the channels for which the data shall be returned
        return value:   A numpy array containing the data requested
        """
        if isinstance(channel_names, str):
            channel_names = [channel_names]

        values = np.empty((self._data.shape[0], len(channel_names)))

        for channel_name in channel_names:
            data_index = self.get_channel_index(channel_name)

            if data_index is not None:
                values[:, channel_names.index(channel_name)] = self._data[:, data_index]
            else:
                print('Channel "{0}" not found. Data invalid for this entry'.format(channel_name))

        return values
