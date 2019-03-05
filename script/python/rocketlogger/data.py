"""
RocketLogger Data Import Support.

File reading support for RocketLogger data (rld) files.

Copyright (c) 2016-2019, Swiss Federal Institute of Technology (ETH Zurich)
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""

from datetime import datetime, timezone
from math import ceil, floor
from os.path import isfile, splitext
import warnings

import numpy as np
import matplotlib.pyplot as plt


_ROCKETLOGGER_FILE_MAGIC = 0x444C5225

_SUPPORTED_FILE_VERSIONS = [1, 2, 3]

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

_CHANNEL_UNIT_NAMES = {
    0: 'unitless',
    1: 'voltage',
    2: 'current',
    3: 'binary',
    4: 'range valid (binary)',
    5: 'illuminance',
    6: 'temerature',
    7: 'integer',
    8: 'percent',
    9: 'preasure',
    0xffffffff: 'undefined',
}
_CHANNEL_IS_BINARY = {
    0: False,
    1: False,
    2: False,
    3: True,
    4: True,
    5: False,
    6: False,
    7: False,
    8: False,
    9: False,
    0xffffffff: False,
}
_CHANNEL_VALID_UNLINKED = 65535

_CHANNEL_MERGE_CANDIDATES = [
    {'low': 'I1L', 'high': 'I1H', 'merged': 'I1'},
    {'low': 'I2L', 'high': 'I2H', 'merged': 'I2'},
]


def _decimate_binary(values, decimation_factor, threshold_value=0.5):
    """
    Decimate binary values, using a threshold value.

    :param values: Numpy vector of the values to decimate

    :param decimation_factor: The decimation factor

    :param threshold_value: The threshold value in [0, 1] to use for decimation

    :returns: Numpy vector of the decimated values
    """
    count_new = floor(values.shape[0] / decimation_factor)
    count_old = count_new * decimation_factor
    aggregated_values = np.reshape(values[0:count_old],
                                   (count_new, decimation_factor))
    return np.logical_not(np.sum(aggregated_values, axis=1) <
                          (threshold_value * decimation_factor))


def _decimate_min(values, decimation_factor):
    """
    Decimate binary values, forcing False if occuring.

    :param values: Numpy vector of the values to decimate

    :param decimation_factor: The decimation factor

    :returns: Numpy vector of the decimated values
    """
    count_new = floor(values.shape[0] / decimation_factor)
    count_old = count_new * decimation_factor
    aggregated_values = np.reshape(values[0:count_old],
                                   (count_new, decimation_factor))
    return (np.sum(aggregated_values, axis=1) >= decimation_factor)


def _decimate_max(values, decimation_factor):
    """
    Decimate binary values, forcing True if occuring.

    :param values: Numpy vector of the values to decimate

    :param decimation_factor: The decimation factor

    :returns: Numpy vector of the decimated values
    """
    count_new = floor(values.shape[0] / decimation_factor)
    count_old = count_new * decimation_factor
    aggregated_values = np.reshape(values[0:count_old],
                                   (count_new, decimation_factor))
    return (np.sum(aggregated_values, axis=1) > 0)


def _decimate_mean(values, decimation_factor):
    """
    Decimate analog values, using averaging of values.

    :param values: Numpy vector of the values to decimate

    :param decimation_factor: The decimation factor

    :returns: Numpy vector of the decimated values
    """
    count_new = floor(values.shape[0] / decimation_factor)
    count_old = count_new * decimation_factor
    aggregated_values = np.reshape(values[0:count_old],
                                   (count_new, decimation_factor))
    return np.mean(aggregated_values, axis=1)


def _read_uint(file_handle, data_size):
    """
    Read an unsigned integer of defined data_size from file.

    :param file_handle: The file handle to read from at current position

    :param data_size: The data size in bytes of the integer to read

    :returns: The integer read and decoded
    """
    return int.from_bytes(file_handle.read(data_size),
                          byteorder='little', signed=False)


def _read_int(file_handle, data_size):
    """
    Read a signed integer of defined data_size from file.

    :param file_handle: The file handle to read from at current position

    :param data_size: The data size in bytes of the integer to read

    :returns: The integer read and decoded
    """
    return int.from_bytes(file_handle.read(data_size),
                          byteorder='little', signed=True)


def _read_str(file_handle, length):
    """
    Read an ASCII string of defined length from file.

    :param file_handle: The file handle to read from at current position

    :param length: The length of the string to read

    :returns: The string read and decoded
    """
    raw_bytes = file_handle.read(length)
    return raw_bytes.split(b'\x00')[0].decode(encoding='ascii')


def _read_timestamp(file_handle):
    """
    Read a timestamp from the file and convert to datetime object.

    :param file_handle: The file handle to read from at current position

    :returns: The read date and time as datetime object
    """
    seconds = _read_int(file_handle, _TIMESTAMP_SECONDS_BYTES)
    nanoseconds = _read_int(file_handle, _TIMESTAMP_NANOSECONDS_BYTES)
    return datetime.fromtimestamp(seconds + 1e-9 * nanoseconds, timezone.utc)


def _read_timestamp_datetime64(file_handle):
    """
    Read a timestamp from the file as nano second datetime64 (numpy)

    :param file_handle: The file handle to read from at current position

    :returns: The read date and time as datetime object
    """
    seconds = _read_int(file_handle, _TIMESTAMP_SECONDS_BYTES)
    nanoseconds = _read_int(file_handle, _TIMESTAMP_NANOSECONDS_BYTES)
    timestamp_ns = (np.datetime64(np.datetime64(seconds, 's'), 'ns') +
                    np.timedelta64(nanoseconds, 'ns'))
    return timestamp_ns


class RocketLoggerFileError(IOError):
    """RocketLogger file read/write related errors."""

    pass


class RocketLoggerDataError(Exception):
    """RocketLogger data handling related errors."""

    pass


class RocketLoggerDataWarning(Warning):
    """RocketLogger data handling related warnings."""

    pass


class RocketLoggerData:
    """
    RocketLogger data file handling class.

    File reading and basic data processing support for binary RocketLogger data
    files.
    """

    _data = []
    _filename = None
    _header = {}
    _timestamps_monotonic = []
    _timestamps_realtime = []

    def __init__(self, filename=None, join_files=True, decimation_factor=1,
                 memory_mapped=True):
        """
        Constructor to create a RockerLoggerData object form data file.

        :param filename: The filename of the file to import

        :param join_files: Enalbe joining of multiple files if numbered files
            following the "<filename>_p#.rld" convention are found.

        :param decimation_factor: Decimation factor for values read

        :param memory_mapped: Set False to fall back to read entire file to
            memory at once, instead of using memory mapped reading. Might
            increase file read performance for many smaller files and/or
            some system configurations.
        """
        if filename is None:
            raise NotImplementedError('RocketLogger data file creation '
                                      'currently unsupported.')
        if isfile(filename):
            self.load_file(filename, join_files, decimation_factor,
                           memory_mapped)
        else:
            raise FileNotFoundError('File "{}" does not exist.'.format(
                                    filename))

    def _read_file_header(self, file_handle):
        """
        Read a RocketLogger data file's header, including comment and channels.

        :param file_handle: The file handle to read from, with pointer
            positioned at file start

        :returns: Named struct containing the read file header data.
        """
        header = {}

        header['file_magic'] = _read_uint(file_handle, _FILE_MAGIC_BYTES)
        header['file_version'] = _read_uint(file_handle, _FILE_VERSION_BYTES)

        if header['file_version'] not in _SUPPORTED_FILE_VERSIONS:
            raise RocketLoggerFileError(
                'RocketLogger file version {} is not supported!'
                .format(header['file_version']))

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
        if (ceil(header['sample_count'] / header['data_block_size']) !=
                header['data_block_count']):
            raise RocketLoggerFileError('Inconsistency in number of samples '
                                        'taken!')
        elif (header['sample_count'] < header['data_block_size'] *
                header['data_block_count']):
            old_count = header['sample_count']
            header['data_block_count'] = floor(header['sample_count'] /
                                               header['data_block_size'])
            header['sample_count'] = (header['data_block_count'] *
                                      header['data_block_size'])
            print('Skipping incomplete data block at end of file '
                  '({} samples)'.format(old_count - header['sample_count']))

        # read comment field
        header['comment'] = _read_str(file_handle, header['comment_length'])

        # FILE VERSION DEPENDENT FIXES (BACKWARD COMPATIBILITY)
        # - none -

        # read channel headers
        header['channels'] = []
        for ch in range(header['channel_binary_count'] +
                        header['channel_analog_count']):
            channel = {}
            channel['unit_index'] = _read_uint(file_handle,
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
                raise RocketLoggerDataError('Undefined channel unit {}'.format(
                    channel['unit_index']))

            # FILE VERSION DEPENDENT FIXES (BACKWARD COMPATIBILITY)
            # fix 1 based indexing of valid channel links for file version <= 2
            if ((header['file_version'] <= 2) &
                    (channel['valid_link'] != _CHANNEL_VALID_UNLINKED)):
                channel['valid_link'] = channel['valid_link'] - 1

            # add channel to header
            header['channels'].append(channel)

        return header

    def _read_file_data(self, file_handle, file_header, decimation_factor=1,
                        memory_mapped=True):
        """
        Read data block at the current position in the RocketLogger data file.

        :param file_handle: The file handle to read from, with pointer
            positioned at the begining of the block

        :param file_header: The file's header with the data alignment details.

        :param decimation_factor: Decimation factor for values read

        :param memory_mapped: Set False to fall back to read entire file to
            memory at once, instead of using memory mapped reading. Might
            increase file read performance for many smaller files and/or
            some system configurations.

        :returns: Tuple of realtime, monotonic clock based numpy datetime64
            arrays, and the list of numpy arrays containing the read channel
            data
        """
        # generate data type to read from header info
        total_bin_bytes = _BINARY_CHANNEL_STUFF_BYTES * \
            ceil(file_header['channel_binary_count'] /
                 (_BINARY_CHANNEL_STUFF_BYTES * 8))
        binary_channels_linked = [channel['valid_link'] for
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
        data_dtype = np.dtype({'names': data_names, 'formats': data_formats})
        block_dtype = np.dtype([
            ('realtime_sec', '<M{}[s]'.format(_TIMESTAMP_SECONDS_BYTES)),
            ('realtime_ns', '<m{}[ns]'.format(_TIMESTAMP_NANOSECONDS_BYTES)),
            ('monotonic_sec', '<M{}[s]'.format(_TIMESTAMP_SECONDS_BYTES)),
            ('monotonic_ns', '<m{}[ns]'.format(_TIMESTAMP_NANOSECONDS_BYTES)),
            ('data', (data_dtype, file_header['data_block_size']))])

        # access file data, either memory mapped or direct read to memory
        if memory_mapped:
            file_data = np.memmap(file_handle,
                                  offset=file_header['header_length'],
                                  mode='r',
                                  dtype=block_dtype,
                                  shape=file_header['data_block_count'])
        else:
            file_data = np.fromfile(file_handle,
                                    dtype=block_dtype,
                                    count=file_header['data_block_count'],
                                    sep='')

        # reference for data blocks speeds up access time
        block_data = np.array(file_data['data'], copy=False)

        # extract timestamps
        timestamps_realtime = (file_data['realtime_sec'] +
                               file_data['realtime_ns'])
        timestamps_monotonic = (file_data['monotonic_sec'] +
                                file_data['monotonic_ns'])

        # allocate empty list of channel data
        data = [None] * (file_header['channel_binary_count'] +
                         file_header['channel_analog_count'])

        # extract binary channels
        for binary_channel_index in range(file_header['channel_binary_count']):
            channel_index = binary_channel_index
            data[channel_index] = np.array(2**binary_channel_index &
                                           block_data['bin'],
                                           dtype=np.dtype('b1'))
            data[channel_index] = data[channel_index].reshape(
                file_header['sample_count'])

            # values decimation
            if decimation_factor > 1:
                # decimate to zero for valid links, threshold otherwise
                if channel_index in binary_channels_linked:
                    data[channel_index] = _decimate_min(
                        data[channel_index], decimation_factor)
                else:
                    data[channel_index] = _decimate_binary(
                        data[channel_index], decimation_factor)

        # extract analog channels
        for analog_channel_index in range(file_header['channel_analog_count']):
            channel_index = (file_header['channel_binary_count'] +
                             analog_channel_index)
            data[channel_index] = np.array(
                block_data[analog_data_names[analog_channel_index]],
                dtype=np.dtype(analog_data_formats[analog_channel_index]))
            data[channel_index] = data[channel_index].reshape(
                file_header['sample_count'])

            # values decimation
            if decimation_factor > 1:
                data[channel_index] =\
                    _decimate_mean(data[channel_index], decimation_factor)

        return timestamps_realtime, timestamps_monotonic, data

    def _get_channel_index(self, channel_name):
        """
        Get the index of a data channel.

        :param channel_name: Name of the channel

        :returns: The index of the channel, None if not found
        """
        channel_names = [channel['name'] for channel in
                         self._header['channels']]
        try:
            channel_index = channel_names.index(channel_name)
        except ValueError:
            channel_index = None

        return channel_index

    def _get_channel_name(self, channel_indexes):
        """
        Get the names of a list of channel indexes.

        :param channel_indexes: List or single index of channel(s)

        :returns: The channel name or a list multiple channel names
        """
        if isinstance(channel_indexes, list):
            channel_names = []
            if len(channel_indexes) > 0:
                for index in channel_indexes:
                    channel_names.append(
                        self._header['channels'][index]['name'])
            return channel_names
        else:
            return self._header['channels'][channel_indexes]['name']

    def add_channel(self, channel_info, channel_data):
        """
        Add a new data channel to the RocketLogger data structure.

        .. note::

            If a valid channel is linked to the channel being added, that
            channel has to be added first.

        :param channel_info: Channel info structure of the channel to add

        :param channel_data: The actual channel data to add, numpy array
        """
        if not isinstance(channel_info, dict):
            raise RocketLoggerDataError('Channel info structure is expected '
                                        'to be a dictionary.')

        # check full channel info available
        if 'unit_index' not in channel_info:
            raise RocketLoggerDataError('Channel info: unit not defined.')
        if channel_info['unit_index'] not in _CHANNEL_UNIT_NAMES:
            raise RocketLoggerDataError('Channel info: invalid channel unit.')

        if 'scale' not in channel_info:
            raise RocketLoggerDataError('Channel info: scale not defined.')
        if not isinstance(channel_info['scale'], int):
            raise RocketLoggerDataError('Channel info: invalid scale type.')

        if 'data_size' not in channel_info:
            raise RocketLoggerDataError('Channel info: data size not defined.')
        if not isinstance(channel_info['data_size'], int):
            raise RocketLoggerDataError('Channel info: invalid scale type.')

        if 'valid_link' not in channel_info:
            raise RocketLoggerDataError('Channel info: link not defined.')
        if not isinstance(channel_info['valid_link'], int):
            raise RocketLoggerDataError('Channel info: invalid link type.')
        if ((channel_info['valid_link'] >= len(self._header['channels'])) &
                (channel_info['valid_link'] != _CHANNEL_VALID_UNLINKED)):
            raise RocketLoggerDataError('Channel info: invalid link.')

        if 'name' not in channel_info:
            raise RocketLoggerDataError('Channel info: name undefined.')
        if not isinstance(channel_info['name'], str):
            raise RocketLoggerDataError('Channel info: invalid name type.')
        if len(channel_info['name']) > _CHANNEL_NAME_BYTES:
            raise RocketLoggerDataError('Channel info: name too long.')
        if channel_info['name'] in self.get_channel_names():
            raise RocketLoggerDataError('Cannot add channel, another channel '
                                        'with name "{}" already exists.'
                                        .format(channel_info['name']))

        # check data
        if len(self._header['channels']) > 0:
            if channel_data.shape != self._data[0].shape:
                raise RocketLoggerDataError('Incompatible data size. '
                                            'Expected array of shape {}.'
                                            .format(self._data[0].shape))

        # add channel info and data
        if _CHANNEL_IS_BINARY[channel_info['unit_index']]:
            self._data.insert(
                self._header['channel_binary_count'], channel_data)
            self._header['channels'].insert(
                self._header['channel_binary_count'], channel_info)
            self._header['channel_binary_count'] =\
                self._header['channel_binary_count'] + 1
        else:
            self._data.append(channel_data)
            self._header['channels'].append(channel_info)
            self._header['channel_analog_count'] =\
                self._header['channel_analog_count'] + 1

    def remove_channel(self, channel_name):
        """
        Remove a data channel from the RocketLogger data structure.

        .. note::

            Linked valid channels are not automatically removed. If they shall
            be removed as well, call this function first on the linked channel.

        :param channel_name: Name of the channel
        """
        channel_index = self._get_channel_index(channel_name)
        if channel_index is None:
            raise KeyError('Channel "{}" not found.'.format(channel_name))

        # update links of channels affected by indexing change
        for channel_info in self._header['channels']:
            if channel_info['valid_link'] == channel_index:
                channel_info['valid_link'] = _CHANNEL_VALID_UNLINKED
            elif channel_info['valid_link'] > channel_index:
                channel_info['valid_link'] = channel_info['valid_link'] - 1

        # delete actual data and header entry
        del(self._data[channel_index])
        del(self._header['channels'][channel_index])

        # adjust header fields
        if channel_index < self._header['channel_binary_count']:
            self._header['channel_binary_count'] =\
                self._header['channel_binary_count'] - 1
        else:
            self._header['channel_analog_count'] =\
                self._header['channel_analog_count'] - 1

    def load_file(self, filename, join_files=True, decimation_factor=1,
                  memory_mapped=True):
        """
        Read a RocketLogger data file and return an RLD object.

        :param filename: The filename of the file to import. If numbered
            files following the "<filename>_p#.rld" convention are found, they
            can be joined during import using the `join_files` flag.

        :param join_files: Enalbe joining of multiple files if numbered files
            following the "<filename>_p#.rld" convention are found.

        :param decimation_factor: Decimation factor for values read

        :param memory_mapped: Set False to fall back to read entire file to
            memory at once, instead of using memory mapped reading. Might
            increase file read performance for many smaller files and/or
            some system configurations.
        """
        if self._filename is not None:
            raise RocketLoggerDataError('A data file is already loaded. Use '
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
                    raise RocketLoggerDataError(
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
                                         decimation_factor=decimation_factor,
                                         memory_mapped=memory_mapped)

                # create on first file, append on following
                if file_number > 0:
                    self._timestamps_realtime = np.hstack(
                        (self._timestamps_realtime, timestamps_realtime))
                    self._timestamps_monotonic = np.hstack(
                        (self._timestamps_monotonic, timestamps_monotonic))
                    for i in range(len(self._data)):
                        self._data[i] = np.hstack((self._data[i], data[i]))
                else:
                    self._timestamps_realtime = timestamps_realtime
                    self._timestamps_monotonic = timestamps_monotonic
                    self._data = data

            file_number = file_number + 1
            file_name = '{}_p{}{}'.format(file_basename, file_number,
                                          file_extension)

            # skip looking for next file if joining not enabled
            if not join_files:
                break

        # adjust header files for decimation
        self._header['sample_count'] = \
            round(self._header['sample_count'] / decimation_factor)
        self._header['data_block_size'] = \
            round(self._header['data_block_size'] / decimation_factor)
        self._header['sample_rate'] = \
            round(self._header['sample_rate'] / decimation_factor)

        self._filename = filename
        print('Read {} file(s)'.format(file_number))

    def get_channel_names(self):
        """
        Get the names of all the channels loaded from file.

        :returns: List of channel names sorted by name
        """
        channel_names = []
        for channel in self._header['channels']:
            channel_names.append(channel['name'])
        return sorted(channel_names)

    def get_comment(self):
        """
        Get the comment stored in the file header.

        :returns: Comment stored in the file
        """
        return self._header['comment']

    def get_data(self, channel_names=['all']):
        """
        Get the data of the specified channels, by default of all channels.

        :param channel_names: The names of the channels for which the data
            shall be returned. List of channel names or 'all' to select all
            channels.

        :returns: A numpy array containing the channel's data vectors
        """
        if not isinstance(channel_names, list):
            channel_names = [channel_names]

        if 'all' in channel_names:
            channel_names = self.get_channel_names()

        values = np.empty((self._header['sample_count'], len(channel_names)))

        for channel_name in channel_names:
            data_index = self._get_channel_index(channel_name)
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

        :param absolute_time: Wether the returned timestamps are relative to
            the start time in seconds (default) or absolute timestamps

        :param time_reference: Which clock data to get (for absolute time only,
            i.e. when `absolute_time=True`)

            - 'local' The local oscillator clock (default)
            - 'network' The network synchronized clock

        :returns: A numpy array containing the timestamps
        """
        # transform value to absolute time if requested
        if absolute_time:
            # get requested timer data as numbers
            if time_reference is 'local':
                block_timestamps = self._timestamps_monotonic.astype('<i8')
            elif time_reference is 'network':
                block_timestamps = self._timestamps_realtime.astype('<i8')
            else:
                raise KeyError('Time reference "{}" undefined.'.format(
                               time_reference))

            # interpolate
            block_points = np.arange(0, self._header['sample_count'],
                                     self._header['data_block_size'])
            data_points = np.arange(0, self._header['sample_count'])

            data_timestamp = np.interp(data_points, block_points,
                                       block_timestamps)

            # convert to datetime again
            timestamps = data_timestamp.astype('<M8[ns]')
        else:
            timestamps = (np.arange(0, self._header['sample_count']) /
                          self._header['sample_rate'])

        return timestamps

    def merge_channels(self, keep_channels=False):
        """
        Merge seemlessly switched current channels into a combined channel.

        :param keep_channels: Whether the merged channels are kept

        :returns: Self reference to data object
        """
        merged_channels = False
        for candidate in _CHANNEL_MERGE_CANDIDATES:
            # check if inputs available and linked
            low_index = self._get_channel_index(candidate['low'])
            high_index = self._get_channel_index(candidate['high'])

            # skip if one input unavailable
            if (low_index is None) or (high_index is None):
                continue

            # error if merge target exists
            if self._get_channel_index(candidate['merged']) is not None:
                raise RocketLoggerDataError('Name of merged output channel '
                                            '"{}" already in use.'.format(
                                                candidate['merged']))

            # check for channel valid link
            low_channel = self._header['channels'][low_index]
            high_channel = self._header['channels'][high_index]
            low_valid_index = low_channel['valid_link']
            if low_valid_index > len(self._header['channels']):
                raise RocketLoggerDataError(
                    'Channel "{}" has invalid link ({}).'.format(
                        low_channel['name'], low_valid_index))

            # build merged channel info and data
            merged_channel_info = {}
            merged_channel_info['unit'] = low_channel['unit']
            merged_channel_info['unit_index'] = low_channel['unit_index']
            merged_channel_info['scale'] = low_channel['scale']
            merged_channel_info['data_size'] = (low_channel['data_size'] +
                                                high_channel['data_size'])
            merged_channel_info['valid_link'] = _CHANNEL_VALID_UNLINKED
            merged_channel_info['name'] = candidate['merged']

            # merge data: force data type to prevent calculation overflow
            merged_data = ((1 * self._data[low_valid_index]) *
                           self._data[low_index] +
                           (1 * np.logical_not(self._data[low_valid_index])) *
                           self._data[high_index].astype(
                               np.dtype('<i{}'.format(
                                   merged_channel_info['data_size']))) *
                           10**(high_channel['scale'] - low_channel['scale']))

            # add merged channel
            self.add_channel(merged_channel_info, merged_data)

            # drop original channels if not kept, update header info
            if not keep_channels:
                valid_channel_name = self._get_channel_name(low_valid_index)
                self.remove_channel(valid_channel_name)
                self.remove_channel(candidate['low'])
                self.remove_channel(candidate['high'])

            print('Merged channels "{}" and "{}" to "{}".'.format(
                candidate['low'], candidate['high'], candidate['merged']))
            merged_channels = True

        if not merged_channels:
            warnings.warn('No channels found to merge.',
                          RocketLoggerDataWarning)

        return self

    def plot(self, channel_names=['all'], show=True):
        """
        Plot the loaded RocketLogger data.

        :param channel_names: The names of the channels for which the
            data shall be returned. List of channel names (or combination of):

            - 'all' to select all channels,
            - 'voltages' to select voltag channels,
            - 'currents' to select current channels,
            - 'digital' to select digital channels.

        :param show: Whether to show the plot window or not

        :returns: The matplotlib plot object used for plotting
        """
        if not isinstance(channel_names, list):
            channel_names = [channel_names]

        channels_digital = self._header['channels'][0:self._header[
            'channel_binary_count']]
        channels_current = [channel for channel in self._header['channels']
                            if channel['unit'] is 'current']
        channels_voltage = [channel for channel in self._header['channels']
                            if channel['unit'] is 'voltage']
        channels_others = [channel for channel in self._header[
            'channels'][self._header['channel_binary_count']:]
            if channel['unit'] not in ['voltage', 'current']]

        plot_current_channels = []
        plot_digital_channels = []
        plot_voltage_channels = []
        plot_other_channels = []

        # get and group channels to plot
        for channel_name in channel_names:
            if channel_name is 'all':
                plot_current_channels = channels_current
                plot_digital_channels = channels_digital
                plot_voltage_channels = channels_voltage
                plot_other_channels = channels_others
                break
            elif channel_name is 'voltages':
                plot_voltage_channels = channels_voltage
            elif channel_name is 'currents':
                plot_current_channels = channels_current
            elif channel_name is 'digital':
                plot_digital_channels = channels_digital
            else:
                channel_index = self._get_channel_index(channel_name)
                if channel_index is None:
                    raise RocketLoggerDataError('Channel {} not found'.format(
                        channel_name))
                if channel_name in [ch['name'] for ch in channels_voltage]:
                    plot_voltage_channels.append(self._header['channels'][
                        channel_index])
                elif channel_name in [ch['name'] for ch in channels_current]:
                    plot_current_channels.append(self._header['channels'][
                        channel_index])
                elif channel_name in [ch['name'] for ch in channels_digital]:
                    plot_digital_channels.append(self._header['channels'][
                        channel_index])

        # prepare plot groups
        plot_groups = [plot_voltage_channels, plot_current_channels,
                       plot_digital_channels, plot_other_channels]
        plot_groups_axis_label = ['voltage [V]', 'current [A]', 'digital', '']
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

        if show:
            plt.show(block=False)

        return plt
