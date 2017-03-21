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

_BINARY_CHANNEL_STUFF_BYTES = 4
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

_CHANNEL_UNIT_NAMES = [
    'Undefined',
    'Voltage',
    'Current',
    'Binary',
    'Range Valid (binary)',
]
_CHANNEL_IS_BINARY = [
    False,
    False,
    False,
    True,
    True,
]


def _read_uint(file_handle, data_size):
    """
    Read an unsigned integer of defined data_size from file.

    file_handle:    The file handle to read from at current position.
    data_size:      The data size in bytes of the integer to read.
    return value:   The integer read and decoded.
    """
    return int.from_bytes(file_handle.read(data_size),
                          byteorder='little', signed=False)


def _read_int(file_handle, data_size):
    """
    Read a signed integer of defined data_size from file.

    file_handle:    The file handle to read from at current position.
    data_size:      The data size in bytes of the integer to read.
    return value:   The integer read and decoded.
    """
    return int.from_bytes(file_handle.read(data_size),
                          byteorder='little', signed=True)


def _read_str(file_handle, length):
    """
    Read an ASCII string of defined length from file.

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


class RocketLoggerFileError(IOError):
    """RocketLogger file I/O related errors."""

    pass


class RocketLoggerFile:
    """
    RocketLogger file handling class.

    Currently only supports reading but not modifying files.
    """

    _data = []
    _header = {}
    _timestamps_monotonic = []
    _timestamps_realtime = []

    def __init__(self, filename, decimation_factor=1):
        """
        Constructor to create a RockerLoggerData object form data file.

        filename:           The filename of the file to import
        decimation_factor:  Decimation factor for values read (default: 1)
        """
        if filename and isfile(filename):
            self.load_file(filename, decimation_factor)

    def _read_file_header(self, file_handle):
        """
        Read a RocketLogger data file's header, including comment and channels.

        file_handle:    The file handle to read from, with pointer positioned
                        at file start
        return value:   Named struct containing the read file header data.
        """
        header = {}

        header['file_magic'] = _read_uint(file_handle, _FILE_MAGIC_BYTES)
        header['file_version'] = _read_uint(file_handle, _FILE_VERSION_BYTES)

        # file consistency check
        if header['file_magic'] != _ROCKETLOGGER_FILE_MAGIC:
            raise RocketLoggerFileError(
                'Invalid RocketLogger data file, file magic missmatch {:x}.'
                .format(header['file_magic']))

        # read static header fields
        header['header_length'] = _read_uint(file_handle, _HEADER_LENGHT_BYTES)
        header['data_block_size'] = _read_uint(file_handle,
                                               _DATA_BLOCK_SIZE_BYTES)
        header['data_block_count'] = _read_uint(file_handle,
                                                _DATA_BLOCK_COUNT_BYTES)
        header['sample_count'] = _read_uint(file_handle, _SAMPLE_COUNT_BYTES)
        header['sample_rate'] = _read_uint(file_handle, _SAMPLE_RATE_BYTES)
        header['mac_address'] = bytearray(file_handle.read(_MAC_ADDRESS_BYTES))
        header['start_time'] = _read_timestamp(file_handle)
        header['comment_length'] = _read_uint(file_handle,
                                              _COMMENT_LENGTH_BYTES)
        header['channel_binary_count'] = _read_uint(
            file_handle,
            _CHANNEL_BINARY_COUNT_BYTES)
        header['channel_analog_count'] = _read_uint(
            file_handle,
            _CHANNEL_ANALOG_COUNT_BYTES)

        # header consistency checks
        if header['comment_length'] % 4 > 0:
            print('WARNING: Comment length unaligned {}.'.format(
                  header['comment_length']))
        if (header['sample_count'] !=
                header['data_block_count'] * header['data_block_size']):
            raise RocketLoggerFileError('Inconsistency in number of samples '
                                        'taken!')

        # read comment field
        header['comment'] = _read_str(file_handle, header['comment_length'])

        # read channel headers
        header['channels'] = []
        for ch in range(header['channel_binary_count'] +
                        header['channel_analog_count']):
            channel = {}
            channel['unit_index'] = _read_int(file_handle,
                                              _CHANNEL_UNIT_INDEX_BYTES)
            channel['scale'] = _read_int(file_handle, _CHANNEL_SCALE_BYTES)
            channel['data_size'] = _read_uint(file_handle,
                                              _CHANNEL_DATA_BYTES_BYTES)
            channel['valid_link'] = _read_uint(file_handle,
                                               _CHANNEL_VALID_LINK_BYTES)
            channel['name'] = _read_str(file_handle, _CHANNEL_NAME_BYTES)

            try:
                channel['unit'] = _CHANNEL_UNIT_NAMES[channel['unit_index']]
            except:
                channel['unit_index'] = 0
                channel['unit'] = _CHANNEL_UNIT_NAMES[0]

            header['channels'].append(channel)

        return header

    def _read_file_data(self, file_handle, file_header, decimation_factor=1):
        """
        Read data block at the current position in the RocketLogger data file.

        file_handle:        The file handle to read from, with pointer
                            positioned at the begining of the block
        file_header:        The file's header with the data alignment details.
        decimation_factor:  Decimation factor for values read (default: 1)
        return value:       Tuple of the realtime timestamp, monotonic
                            timestamp and the list of arrays containing the
                            read sample data.
        """
        # generate data type to read from header info
        total_bin_bytes = _BINARY_CHANNEL_STUFF_BYTES * \
            ceil(file_header['channel_binary_count'] /
                 (_BINARY_CHANNEL_STUFF_BYTES * 8))

        analog_data_formats = []
        analog_data_names = []
        data_formats = []
        data_names = []

        if total_bin_bytes > 0:
            data_names = ['bin']
            data_formats = ['<u{}'.format(total_bin_bytes)]

        for channel in file_header['channels']:
            if not _CHANNEL_IS_BINARY[channel['unit_index']]:
                data_format = '<i{}'.format(channel['data_size'])
                analog_data_formats.append(data_format)
                data_formats.append(data_format)
                analog_data_names.append(channel['name'])
                data_names.append(channel['name'])

        # read raw data from file
        data_type = np.dtype({'names': data_names, 'formats': data_formats})

        # allocate the data
        timestamps_realtime = []
        timestamps_monotonic = []
        data = [None] * (file_header['channel_binary_count'] +
                         file_header['channel_analog_count'])

        for binary_channel_index in range(file_header['channel_binary_count']):
            channel_index = binary_channel_index
            data[channel_index] = np.empty((file_header['data_block_count'] *
                                            file_header['data_block_size']),
                                           dtype=np.dtype('b1'))
        for analog_channel_index in range(file_header['channel_analog_count']):
            channel_index = (file_header['channel_binary_count'] +
                             analog_channel_index)
            data[channel_index] = np.empty((file_header['data_block_count'] *
                                            file_header['data_block_size']),
                                           dtype=np.dtype(
                                               analog_data_formats[
                                                   analog_channel_index]))

        # iterate over all data blocks
        for block_index in range(file_header['data_block_count']):
            block_offset_bytes = (file_header['header_length'] +
                                  block_index * (2 * _TIMESTAMP_BYTES +
                                  file_header['data_block_size'] *
                                  data_type.itemsize))
            data_index_start = block_index * file_header['data_block_size']
            data_index_end = data_index_start + file_header['data_block_size']

            # read block timestamp
            file_handle.seek(block_offset_bytes)
            timestamps_realtime.append(_read_timestamp(file_handle))
            timestamps_monotonic.append(_read_timestamp(file_handle))

            # access file data memory mapped
            # data_raw = np.fromfile(file_handle, dtype=data_type,
            #                        count=file_header['data_block_size'])
            file_data = np.memmap(file_handle,
                                  offset=block_offset_bytes +
                                  2 * _TIMESTAMP_BYTES,
                                  mode='r',
                                  dtype=data_type,
                                  shape=file_header['data_block_size'])

            # extract binary channels
            # binary_data_bool = np.unpackbits(file_data['bin']).reshape(
            #     (file_header['data_block_size'], 8 * total_bin_bytes))

            # TODO: map to data type used ot store binary value
            for binary_channel_index in range(
                    file_header['channel_binary_count']):
                channel_index = binary_channel_index
                data[channel_index][data_index_start:data_index_end] = \
                    np.array(2**binary_channel_index & file_data['bin'],
                             dtype=data[channel_index].dtype)

            for analog_channel_index in range(
                    file_header['channel_analog_count']):
                channel_index = (file_header['channel_binary_count'] +
                                 analog_channel_index)
                data[channel_index][data_index_start:data_index_end] = \
                    file_data[analog_data_names[analog_channel_index]]

            # decimate block data
            # TODO: decimantion on import
            if decimation_factor != 1:
                raise NotImplementedError(
                    'Decimation on import not implemented.')

        return timestamps_realtime, timestamps_monotonic, data

    def load_file(self, filename, decimation_factor=1):
        """
        Read a RocketLogger Data file and return an RLD object.

        filename:           The filename of the file to import
        decimation_factor:  Decimation factor for values read (default: 1)
        """
        if len(self._header) > 0:
            raise RocketLoggerFileError('A data file is already loaded. Use '
                                        'separate instance for new file.')

        file_basename, file_extension = splitext(filename)
        file_name = filename
        file_number = 0

        while isfile(file_name):
            with open(file_name, 'rb') as file_handle:
                # read file header
                header = self._read_file_header(file_handle)

                # consistency check: file stream position matches header size
                if 'header_length' not in header:
                    raise RocketLoggerFileError('Invalid file header read.')

                stream_position = file_handle.tell()
                if stream_position != header['header_length']:
                    raise RocketLoggerFileError(
                        'File position {} does not match header size {}'
                        .format(stream_position, header['header_length']))

                if (header['data_block_size'] % decimation_factor) > 0:
                    raise RocketLoggerFileError(
                        'Decimation factor needs to be divider of the buffer '
                        'size.')

                header['sample_count'] = ceil(header['sample_count'] /
                                              decimation_factor)
                header['data_block_size'] = ceil(header['data_block_size'] /
                                                 decimation_factor)
                header['sample_rate'] = ceil(header['sample_rate'] /
                                             decimation_factor)

                # multi-file header
                if file_number == 0:
                    self._header = header
                else:
                    self._header['sample_count'] = \
                        self._header['sample_count'] + header['sample_count']
                    self._header['data_block_count'] = \
                        (self._header['data_block_count'] +
                         header['data_block_count'])

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
            file_name = '{}_p{}{}'.format(file_basename, file_number,
                                          file_extension)

        print('Read {} file(s)'.format(file_number))

    def get_channel_names(self):
        """
        Get the names of all the channels loaded from file.

        return value:   List of all channel names
        """
        channel_names = []
        for channel in self._header['channels']:
            channel_names.append(channel['name'])

        return channel_names

    def get_channel_index(self, channel_name):
        """
        Get the index of the channel.

        channel_name:   Name of the channel
        return value:   The index of the channel, None if not found
        """
        channel_names = [channel['name'] for channel in
                         self._header['channels']]
        try:
            channel_index = channel_names.index(channel_name)
        except ValueError:
            channel_index = None

        return channel_index

    def get_data(self, channel_names=['all']):
        """
        Get the data of the specified channels, by default of all channels.

        channel_names:  The names of the channels for which the data shall
                        be returned. List of channel names or 'all' to select
                        all channels (default: 'all')
        return value:   A numpy array containing the channel's data vectors
        """
        if isinstance(channel_names, str):
            channel_names = [channel_names]

        if 'all' in channel_names:
            channel_names = self.get_channel_names()

        values = np.empty((self._header['sample_count'], len(channel_names)))

        for channel_name in channel_names:
            data_index = self.get_channel_index(channel_name)
            if data_index is None:
                raise KeyError('Channel "{}" not found.'.format(channel_name))
            values[:, channel_names.index(channel_name)] = (
                self._data[data_index] *
                10**(self._header['channels'][data_index]['scale']))

        return values

    def get_time(self, time_reference='local', absolute_time=False):
        """
        Get the timestamp of the data.

        time_reference: Which clock data is being used.
                        - 'local' The local oscillator clock (default)
                        - 'network' The network synchronized clock
        absolute_time:  Wether the returned timestamps are relative to the
                        start time (default) or absolute timestamps
        return value:   A numpy array containing the timestamps
        """
        # get timestamps from requested timer
        if time_reference is 'local':
            timestamp_values = self._timestamps_monotonic
        elif time_reference is 'network':
            timestamp_values = self._timestamps_realtime
        else:
            raise KeyError('Time reference "{}" undefined.'.format(
                           time_reference))

        # transform value to absolute time if requested
        if absolute_time:
            sample_points = np.arange(0, self._header['data_block_count'] + 1)

            # TODO: Interpolate absolute time

            # MATLAB reference implementation
            # points = 0:obj.header.data_block_count;
            # temp_time = [obj.time; obj.time(end) + seconds(1)];
            # interp_points = ((0:(obj.header.sample_count-1)) /
            #                  obj.header.data_block_size);
            # timestamps = interp1(points, temp_time, interp_points)';
            timestamps = (np.arange(0, self._header['sample_count']) /
                          self._header['sample_rate'])
        else:
            # TODO: Interpolate relative time
            timestamps = (np.arange(0, self._header['sample_count']) /
                          self._header['sample_rate'])

        return timestamps

    def merge_channels(self, keep_channels=False):
        """
        Merge seemlessly switched current channels into one common channel.

        keep_channels:  Whether the that are merged are kept (default: False)
        """
        raise NotImplementedError()

    def plot(self, channel_names=['all']):
        """
        Plot the loaded RocketLogger data.

        channel_names:  The names of the channels for which the data shall
                        be returned. List of channel names or 'all' to select
                        all channels (default: 'all')
        return value:   Matplotlib figure handle of the plot
        """
        raise NotImplementedError()
