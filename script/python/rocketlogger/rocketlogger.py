"""
RocketLogger Python Library.

Importing and plotting of RocketLogger Data (rld) binary files.

Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group

"""

from datetime import datetime, timezone
from math import ceil, floor
from os.path import isfile, splitext

import numpy as np
import matplotlib.pyplot as plt


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
_CHANNEL_VALID_UNLINKED = 65535

_CHANNEL_MERGE_CANDIDATES = [
    {'low': 'I1L', 'high': 'I1H', 'merged': 'I1'},
    {'low': 'I2L', 'high': 'I2H', 'merged': 'I2'},
]


def _decimate_binary(values, decimation_factor, threshold_value=0.5):
    """
    Decimate binary values, using a threshold value.

    values:             Numpy vector of the values to decimate
    decimation_factor:  The decimation factor
    threshold_value:    The threshold value in [0, 1] to use for decimation
                        (default: 0.5)
    return value:       Numpy vector of the decimated values
    """
    count_new = floor(values.shape[0] / decimation_factor)
    count_old = count_new * decimation_factor
    aggregated_values = np.reshape(values[0:count_old],
                                   (count_new, decimation_factor))
    return np.logical_not(np.sum(aggregated_values, axis=1) <
                          (threshold_value * decimation_factor))
# return np.logical_not(np.mean(aggregated_values, axis=1) < threshold_value)


def _decimate_min(values, decimation_factor):
    """
    Decimate binary values, forcing False if occuring.

    values:             Numpy vector of the values to decimate
    decimation_factor:  The decimation factor
    return value:       Numpy vector of the decimated values
    """
    count_new = floor(values.shape[0] / decimation_factor)
    count_old = count_new * decimation_factor
    aggregated_values = np.reshape(values[0:count_old],
                                   (count_new, decimation_factor))
    return (np.sum(aggregated_values, axis=1) < decimation_factor)


def _decimate_max(values, decimation_factor):
    """
    Decimate binary values, forcing True if occuring.

    values:             Numpy vector of the values to decimate
    decimation_factor:  The decimation factor
    return value:       Numpy vector of the decimated values
    """
    count_new = floor(values.shape[0] / decimation_factor)
    count_old = count_new * decimation_factor
    aggregated_values = np.reshape(values[0:count_old],
                                   (count_new, decimation_factor))
    return (np.sum(aggregated_values, axis=1) > 0)


def _decimate_mean(values, decimation_factor):
    """
    Decimate analog values, using averaging of values.

    values:             Numpy vector of the values to decimate
    decimation_factor:  The decimation factor
    return value:       Numpy vector of the decimated values
    """
    count_new = floor(values.shape[0] / decimation_factor)
    count_old = count_new * decimation_factor
    aggregated_values = np.reshape(values[0:count_old],
                                   (count_new, decimation_factor))
    return np.mean(aggregated_values, axis=1)


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
    _filename = None
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
        binary_channels_linked = [(channel['valid_link'] - 1) for
                                  channel in file_header['channels']]
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
            data[channel_index] = np.empty(file_header['data_block_count'] *
                                           round(file_header['data_block_size']
                                                 / decimation_factor),
                                           dtype=np.dtype('b1'))
        for analog_channel_index in range(file_header['channel_analog_count']):
            channel_index = (file_header['channel_binary_count'] +
                             analog_channel_index)
            data[channel_index] = np.empty(file_header['data_block_count'] *
                                           round(file_header['data_block_size']
                                                 / decimation_factor),
                                           dtype=np.dtype(
                                               analog_data_formats[
                                                   analog_channel_index]))

        # iterate over all data blocks
        for block_index in range(file_header['data_block_count']):
            file_block_offset_bytes = (file_header['header_length'] +
                                       block_index * (2 * _TIMESTAMP_BYTES +
                                       file_header['data_block_size'] *
                                       data_type.itemsize))
            data_index_start = block_index * round(
                file_header['data_block_size'] / decimation_factor)
            data_index_end = data_index_start + round(
                file_header['data_block_size'] / decimation_factor)

            # read block timestamp
            file_handle.seek(file_block_offset_bytes)
            timestamps_realtime.append(_read_timestamp(file_handle))
            timestamps_monotonic.append(_read_timestamp(file_handle))

            # access file data memory mapped
            file_data = np.memmap(file_handle,
                                  offset=file_block_offset_bytes +
                                  2 * _TIMESTAMP_BYTES,
                                  mode='r',
                                  dtype=data_type,
                                  shape=file_header['data_block_size'])

            # extract binary channels
            for binary_channel_index in range(
                    file_header['channel_binary_count']):
                channel_index = binary_channel_index
                binary_values = np.array(2**binary_channel_index &
                                         file_data['bin'],
                                         dtype=data[channel_index].dtype)
                # values decimation
                if decimation_factor > 1:
                    # decimate to zero for valid links, threshold otherwise
                    if channel_index in binary_channels_linked:
                        data[channel_index][data_index_start:data_index_end] =\
                            _decimate_min(binary_values, decimation_factor)
                    else:
                        data[channel_index][data_index_start:data_index_end] =\
                            _decimate_binary(binary_values, decimation_factor)
                else:
                    data[channel_index][data_index_start:data_index_end] =\
                        binary_values

            # extract analog channels
            for analog_channel_index in range(
                    file_header['channel_analog_count']):
                channel_index = (file_header['channel_binary_count'] +
                                 analog_channel_index)
                analog_values = np.array(file_data[analog_data_names[
                                         analog_channel_index]],
                                         dtype=data[channel_index].dtype)
                # values decimation
                if decimation_factor > 1:
                    data[channel_index][data_index_start:data_index_end] =\
                        _decimate_mean(analog_values, decimation_factor)
                else:
                    data[channel_index][data_index_start:data_index_end] =\
                        analog_values

        return timestamps_realtime, timestamps_monotonic, data

    def load_file(self, filename, decimation_factor=1):
        """
        Read a RocketLogger Data file and return an RLD object.

        filename:           The filename of the file to import
        decimation_factor:  Decimation factor for values read (default: 1)
        """
        if self._filename is not None:
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

                # multi-file header
                if file_number == 0:
                    self._header = header
                else:
                    self._header['sample_count'] =\
                        self._header['sample_count'] + header['sample_count']
                    self._header['data_block_count'] =\
                        (self._header['data_block_count'] +
                         header['data_block_count'])

                # channels: read actual sampled data
                timestamps_realtime, timestamps_monotonic, data =\
                    self._read_file_data(file_handle, header,
                                         decimation_factor=decimation_factor)

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

        # adjust header files for decimation
        header['sample_count'] = round(header['sample_count'] /
                                       decimation_factor)
        header['data_block_size'] = round(header['data_block_size'] /
                                          decimation_factor)
        header['sample_rate'] = round(header['sample_rate'] /
                                      decimation_factor)

        self._filename = filename
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

    def get_time(self, absolute_time=False, time_reference='local'):
        """
        Get the timestamp of the data.

        Using simple linear interpolation to generating the sample from the
        block timestamps.

        absolute_time:  Wether the returned timestamps are relative to the
                        start time in seconds (default) or absolute timestamps
        time_reference: Which clock data is being used (absolute time only)
                        - 'local' The local oscillator clock (default)
                        - 'network' The network synchronized clock
        return value:   A numpy array containing the timestamps
        """
        # transform value to absolute time if requested
        if absolute_time:
            raise NotImplementedError('Abolute timestamp handling '
                                      'currently unsupported')

            # get timestamps from requested timer
            if time_reference is 'local':
                timestamp_values = self._timestamps_monotonic
            elif time_reference is 'network':
                timestamp_values = self._timestamps_realtime
            else:
                raise KeyError('Time reference "{}" undefined.'.format(
                            time_reference))

            sample_points = np.arange(0, self._header['data_block_count'] + 1)

            # MATLAB reference implementation
            # points = 0:obj.header.data_block_count;
            # temp_time = [obj.time; obj.time(end) + seconds(1)];
            # interp_points = ((0:(obj.header.sample_count-1)) /
            #                  obj.header.data_block_size);
            # timestamps = interp1(points, temp_time, interp_points)';
        else:
            timestamps = (np.arange(0, self._header['sample_count']) /
                          self._header['sample_rate'])

        return timestamps

    def merge_channels(self, keep_channels=False):
        """
        Merge seemlessly switched current channels into one common channel.

        keep_channels:  Whether the that are merged are kept (default: False)
        return value:   Reference to the class instance itself
        """
        merged_channels = False
        for candidate in _CHANNEL_MERGE_CANDIDATES:
            # check if inputs available and linked
            low_index = self.get_channel_index(candidate['low'])
            high_index = self.get_channel_index(candidate['high'])

            # skip if one input unavailable
            if (low_index is None) or (high_index is None):
                continue

            # error if merge target exists
            if self.get_channel_index(candidate['merged']) is not None:
                raise RocketLoggerFileError('Name of merged output channel '
                                            '"{}" already in use.'.format(
                                                candidate['merged']))

            # check for channel valid link
            low_channel = self._header['channels'][low_index]
            high_channel = self._header['channels'][high_index]
            low_valid_index = low_channel['valid_link'] - 1
            if low_valid_index > len(self._header['channels']):
                raise RocketLoggerFileError('Low range has no valid data')

            # build merged channel info and data
            merged_channel_info = {}
            merged_channel_info['unit'] = low_channel['unit']
            merged_channel_info['unit_index'] = low_channel['unit_index']
            merged_channel_info['scale'] = low_channel['scale']
            merged_channel_info['data_size'] = (low_channel['data_size'] +
                                                high_channel['data_size'])
            merged_channel_info['valid_link'] = _CHANNEL_VALID_UNLINKED
            merged_channel_info['name'] = candidate['merged']

            # add merged channel
            merged_data = np.empty(self._data[low_index].shape, dtype=np.dtype(
                '<i{}'.format(merged_channel_info['data_size'])))
            merged_data = (self._data[low_valid_index] * self._data[low_index]
                           + np.logical_not(self._data[low_valid_index]) *
                           self._data[high_index] *
                           10**(high_channel['scale'] - low_channel['scale']))
            self._data.append(merged_data)
            self._header['channels'].append(merged_channel_info)

            # drop original channels if not kept, update header info
            if keep_channels:
                self._header['channel_analog_count'] =\
                    self._header['channel_analog_count'] + 1
            else:
                del(self._data[low_valid_index])
                del(self._header['channels'][low_valid_index])
                low_index = self.get_channel_index(candidate['low'])
                del(self._data[low_index])
                del(self._header['channels'][low_index])
                high_index = self.get_channel_index(candidate['high'])
                del(self._data[high_index])
                del(self._header['channels'][high_index])
                self._header['channel_binary_count'] =\
                    self._header['channel_binary_count'] - 1
                self._header['channel_analog_count'] =\
                    self._header['channel_analog_count'] - 1

            print('Merged channels "{}" and "{}" to "{}".'.format(
                candidate['low'], candidate['high'], candidate['merged']))
            merged_channels = True

        if not merged_channels:
            print('WARNING: No channels found to merge.')

        return self

    def plot(self, channel_names=['all']):
        """
        Plot the loaded RocketLogger data.

        channel_names:  The names of the channels for which the data shall
                        be returned. List of channel names (or combination of)
                        'all' to select all channels, 'voltages' to select
                        voltage, 'currents' to select current, and 'digital'
                        to select digital channels (default: 'all')
        """
        channels_digital = self._header['channels'][0:self._header['channel_binary_count']]
        channels_current = [channel for channel in self._header['channels']
                            if channel['unit'] is 'Current']
        channels_voltage = [channel for channel in self._header['channels']
                            if channel['unit'] is 'Voltage']

        plot_current_channels = []
        plot_digital_channels = []
        plot_voltage_channels = []

        # get and group channels to plot
        for channel_name in channel_names:
            if channel_name is 'all':
                plot_current_channels = channels_current
                plot_digital_channels = channels_digital
                plot_voltage_channels = channels_voltage
                break
            elif channel_name is 'voltages':
                plot_voltage_channels = channels_voltage
            elif channel_name is 'currents':
                plot_current_channels = channels_current
            elif channel_name is 'digital':
                plot_digital_channels = channels_digital
            else:
                channel_index = self.get_channel_index(channel_name)
                if channel_index is None:
                    raise RocketLoggerFile('Channel {} not found'.format(
                                           channel_name))
                if channel_name is 'voltages':
                    plot_voltage_channels.append(self._header['channels'][
                                                 channel_index])
                elif channel_name is 'currents':
                    plot_current_channels.append(self._header['channels'][
                                                 channel_index])
                elif channel_name is 'digital':
                    plot_digital_channels.append(self._header['channels'][
                                                 channel_index])

        # prepare plot groups
        plot_groups = [plot_voltage_channels, plot_current_channels,
                       plot_digital_channels]
        plot_groups_axis_label = ['voltage [V]', 'current [A]', 'digital']
        plot_time = self.get_time()

        subplot_count = len([group for group in plot_groups if len(group) > 0])
        fig, ax = plt.subplots(subplot_count, 1, sharex='col')
        if subplot_count == 1:
            ax = [ax]
        plt.suptitle(self._filename)

        # plot
        subplot_index = 0
        for i in range(len(plot_groups)):
            plot_channels = plot_groups[i]
            if len(plot_channels) == 0:
                continue
            plot_channel_names = [channel['name'] for channel in plot_channels]
            plot_channel_data = self.get_data(plot_channel_names)
            ax[subplot_index].plot(plot_time, plot_channel_data)
            ax[subplot_index].set_ylabel(plot_groups_axis_label[i])
            ax[subplot_index].legend(plot_channel_names)
            subplot_index = subplot_index + 1

        ax[subplot_count - 1].set_xlabel('time [s]')
        plt.show(block=False)
