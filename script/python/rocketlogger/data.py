"""
RocketLogger Data Import Support.

File reading support for RocketLogger data (rld) files.

Copyright (c) 2016-2020, ETH Zurich, Computer Engineering Group
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

from math import ceil, floor
import os
from os.path import isfile, splitext
import warnings

import numpy as np


_ROCKETLOGGER_ADC_CLOCK_SCALE = (100e6 / 49) / 2.048e6

_ROCKETLOGGER_FILE_MAGIC = 0x444C5225

_SUPPORTED_FILE_VERSIONS = [1, 2, 3, 4]

_BINARY_CHANNEL_STUFF_BYTES = 4
_TIMESTAMP_SECONDS_BYTES = 8
_TIMESTAMP_NANOSECONDS_BYTES = 8
_TIMESTAMP_BYTES = _TIMESTAMP_SECONDS_BYTES + _TIMESTAMP_NANOSECONDS_BYTES

_FILE_MAGIC_BYTES = 4
_FILE_VERSION_BYTES = 2
_HEADER_LENGTH_BYTES = 2
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
    0: "unit-less",
    1: "voltage",
    2: "current",
    3: "binary",
    4: "data valid (binary)",
    5: "illuminance",
    6: "temperature",
    7: "integer",
    8: "percent",
    9: "pressure",
    10: "time delta",
    0xFFFFFFFF: "undefined",
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
    10: False,
    0xFFFFFFFF: False,
}
_CHANNEL_VALID_UNLINKED = 65535

_CHANNEL_MERGE_CANDIDATES = [
    {"low": "I1L", "high": "I1H", "merged": "I1"},
    {"low": "I2L", "high": "I2H", "merged": "I2"},
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
    aggregated_values = np.reshape(values[0:count_old], (count_new, decimation_factor))
    return np.logical_not(
        np.sum(aggregated_values, axis=1) < (threshold_value * decimation_factor)
    )


def _decimate_min(values, decimation_factor):
    """
    Decimate binary values, forcing False if occurring.

    :param values: Numpy vector of the values to decimate

    :param decimation_factor: The decimation factor

    :returns: Numpy vector of the decimated values
    """
    count_new = floor(values.shape[0] / decimation_factor)
    count_old = count_new * decimation_factor
    aggregated_values = np.reshape(values[0:count_old], (count_new, decimation_factor))
    return np.sum(aggregated_values, axis=1) >= decimation_factor


def _decimate_max(values, decimation_factor):
    """
    Decimate binary values, forcing True if occurring.

    :param values: Numpy vector of the values to decimate

    :param decimation_factor: The decimation factor

    :returns: Numpy vector of the decimated values
    """
    count_new = floor(values.shape[0] / decimation_factor)
    count_old = count_new * decimation_factor
    aggregated_values = np.reshape(values[0:count_old], (count_new, decimation_factor))
    return np.sum(aggregated_values, axis=1) > 0


def _decimate_mean(values, decimation_factor):
    """
    Decimate analog values, using averaging of values.

    :param values: Numpy vector of the values to decimate

    :param decimation_factor: The decimation factor

    :returns: Numpy vector of the decimated values
    """
    count_new = floor(values.shape[0] / decimation_factor)
    count_old = count_new * decimation_factor
    aggregated_values = np.reshape(values[0:count_old], (count_new, decimation_factor))
    return np.mean(aggregated_values, axis=1)


def _read_uint(file_handle, data_size):
    """
    Read an unsigned integer of defined data_size from file.

    :param file_handle: The file handle to read from at current position

    :param data_size: The data size in bytes of the integer to read

    :returns: The integer read and decoded
    """
    return int.from_bytes(file_handle.read(data_size), byteorder="little", signed=False)


def _read_int(file_handle, data_size):
    """
    Read a signed integer of defined data_size from file.

    :param file_handle: The file handle to read from at current position

    :param data_size: The data size in bytes of the integer to read

    :returns: The integer read and decoded
    """
    return int.from_bytes(file_handle.read(data_size), byteorder="little", signed=True)


def _read_str(file_handle, length):
    """
    Read an ASCII string of defined length from file.

    :param file_handle: The file handle to read from at current position

    :param length: The length of the string to read

    :returns: The string read and decoded
    """
    raw_bytes = file_handle.read(length)
    return raw_bytes.split(b"\x00")[0].decode(encoding="ascii")


def _read_timestamp(file_handle):
    """
    Read a timestamp from the file as nano second datetime64 (Numpy).

    :param file_handle: The file handle to read from at current position

    :returns: The read date and time as nano second datetime64 (Numpy)
    """
    seconds = _read_int(file_handle, _TIMESTAMP_SECONDS_BYTES)
    nanoseconds = _read_int(file_handle, _TIMESTAMP_NANOSECONDS_BYTES)
    timestamp_ns = np.datetime64(seconds, "s") + np.timedelta64(nanoseconds, "ns")
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

    The constructor takes the same parameters as :func:`load_file`.

    :param filename: The filename of the file to import. If numbered
        files following the "<filename>_p#.rld" convention are found, they
        can be joined during import using the `join_files` parameter.

    :param join_files: Enable joining of multiple files, if numbered files
        following the "<filename>_p#.rld" convention are found

    :param exclude_part: Exclude given part(s) of number files when
        joining following the "<filename>_p#.rld" convention.
        Expects a single index or list or Numpy array of indexes to
        exclude. Silently ignores indexes beyond the number of found parts.
        Only applicable when joining multiple files with `join_files=True`.

    :param header_only: Enable to import only header info without data

    :param decimation_factor: Decimation factor for values read

    :param recovery: Attempt recovery of damaged files that have lost or
        incomplete data blocks at the end of the file. This can be
        caused by power supply failures during measurements.

    :param memory_mapped: Set `False` to fall back to read entire file to
        memory at once, instead of using memory mapped reading. Might
        increase file read performance for many smaller files and/or
        some system configurations.
    """

    def __init__(
        self,
        filename=None,
        join_files=True,
        header_only=False,
        exclude_part=None,
        decimation_factor=1,
        recovery=False,
        memory_mapped=True,
    ):
        self._data = []
        self._filename = None
        self._header = {}
        self._timestamps_monotonic = []
        self._timestamps_realtime = []

        if filename is None:
            raise NotImplementedError(
                "RocketLogger data file creation currently unsupported."
            )
        if isfile(filename):
            self.load_file(
                filename,
                join_files=join_files,
                exclude_part=exclude_part,
                header_only=header_only,
                decimation_factor=decimation_factor,
                recovery=recovery,
                memory_mapped=memory_mapped,
            )
        else:
            raise FileNotFoundError(f"File '{filename}' does not exist.")

    def _read_file_header_lead_in(self, file_handle):
        """
        Read a RocketLogger data file's header fixed size lead-in

        :param file_handle: The file handle to read from, with pointer
            positioned at file start

        :returns: Dictionary containing the read file header lead-in data
        """
        header = {}

        header["file_magic"] = _read_uint(file_handle, _FILE_MAGIC_BYTES)
        header["file_version"] = _read_uint(file_handle, _FILE_VERSION_BYTES)

        # file consistency check
        if header["file_magic"] != _ROCKETLOGGER_FILE_MAGIC:
            raise RocketLoggerFileError(
                "Invalid RocketLogger data file, file magic mismatch "
                f"0x{header['file_magic']:08x}."
            )

        if header["file_version"] not in _SUPPORTED_FILE_VERSIONS:
            raise RocketLoggerFileError(
                f"Unsupported RocketLogger data file version {header['file_version']}."
            )

        # read static header fields
        header["header_length"] = _read_uint(file_handle, _HEADER_LENGTH_BYTES)
        header["data_block_size"] = _read_uint(file_handle, _DATA_BLOCK_SIZE_BYTES)
        header["data_block_count"] = _read_uint(file_handle, _DATA_BLOCK_COUNT_BYTES)
        header["sample_count"] = _read_uint(file_handle, _SAMPLE_COUNT_BYTES)
        header["sample_rate"] = _read_uint(file_handle, _SAMPLE_RATE_BYTES)
        header["mac_address"] = bytearray(file_handle.read(_MAC_ADDRESS_BYTES))
        header["start_time"] = _read_timestamp(file_handle)
        header["comment_length"] = _read_uint(file_handle, _COMMENT_LENGTH_BYTES)
        header["channel_binary_count"] = _read_uint(
            file_handle, _CHANNEL_BINARY_COUNT_BYTES
        )
        header["channel_analog_count"] = _read_uint(
            file_handle, _CHANNEL_ANALOG_COUNT_BYTES
        )

        # header consistency checks
        if header["comment_length"] % 4 > 0:
            warnings.warn(
                RocketLoggerDataWarning(
                    f"comment length unaligned {header['comment_length']}."
                )
            )
        if (
            ceil(header["sample_count"] / header["data_block_size"])
            != header["data_block_count"]
        ):
            raise RocketLoggerFileError("inconsistent number of samples taken!")
        elif (
            header["sample_count"]
            < header["data_block_size"] * header["data_block_count"]
        ):
            sample_count_original = header["sample_count"]
            header["data_block_count"] = floor(
                header["sample_count"] / header["data_block_size"]
            )
            header["sample_count"] = (
                header["data_block_count"] * header["data_block_size"]
            )
            sample_count_diff = sample_count_original - header["sample_count"]
            warnings.warn(
                RocketLoggerDataWarning(
                    f"Skipping incomplete data block at end of file ({sample_count_diff} samples)."
                )
            )

        return header

    def _read_file_header(self, file_handle):
        """
        Read a RocketLogger data file's header, including comment and channels.

        :param file_handle: The file handle to read from, with pointer
            positioned after lead-in/at start of comment

        :returns: Dictionary containing the read file header data
        """
        header = self._read_file_header_lead_in(file_handle)

        # read comment field
        header["comment"] = _read_str(file_handle, header["comment_length"])

        # FILE VERSION DEPENDENT FIXES (BACKWARD COMPATIBILITY)
        # - none -

        # read channel headers
        header["channels"] = []
        for ch in range(
            header["channel_binary_count"] + header["channel_analog_count"]
        ):
            channel = {}
            channel["unit_index"] = _read_uint(file_handle, _CHANNEL_UNIT_INDEX_BYTES)
            channel["scale"] = _read_int(file_handle, _CHANNEL_SCALE_BYTES)
            channel["data_size"] = _read_uint(file_handle, _CHANNEL_DATA_BYTES_BYTES)
            channel["valid_link"] = _read_uint(file_handle, _CHANNEL_VALID_LINK_BYTES)
            channel["name"] = _read_str(file_handle, _CHANNEL_NAME_BYTES)

            try:
                channel["unit"] = _CHANNEL_UNIT_NAMES[channel["unit_index"]]
            except KeyError:
                raise KeyError(
                    f"Undefined channel unit with index {channel['unit_index']}."
                )

            # FILE VERSION DEPENDENT FIXES (BACKWARD COMPATIBILITY)
            # fix 1 based indexing of valid channel links for file version <= 2
            if (header["file_version"] <= 2) & (
                channel["valid_link"] != _CHANNEL_VALID_UNLINKED
            ):
                channel["valid_link"] = channel["valid_link"] - 1

            # add channel to header
            header["channels"].append(channel)

        # consistency check: file stream position matches header size
        stream_position = file_handle.tell()
        if stream_position != header["header_length"]:
            raise RocketLoggerFileError(
                f"File position {stream_position} does not match "
                f"header size {header['header_length']}"
            )

        return header

    def _validate_matching_header(self, header1, header2):
        """
        Validate that file headers match, i.e. correspond to the same measurement.

        :param header1: First file header data to compare

        :param header2: Second file header data to compare
        """
        required_matches = [
            "file_version",
            "header_length",
            "data_block_size",
            "sample_rate",
            "mac_address",
            "start_time",
            "comment_length",
            "channel_binary_count",
            "channel_analog_count",
        ]
        for header_field in required_matches:
            if header1[header_field] != header2[header_field]:
                raise RocketLoggerDataError(
                    f"File header not matching at field: {header_field}"
                )

    def _read_file_data(
        self, file_handle, file_header, decimation_factor=1, memory_mapped=True
    ):
        """
        Read data block at the current position in the RocketLogger data file.

        :param file_handle: The file handle to read from, with pointer
            positioned at the beginning of the block

        :param file_header: The file's header with the data alignment details

        :param decimation_factor: Decimation factor for values read

        :param memory_mapped: Set `False` to fall back to read entire file to
            memory at once, instead of using memory mapped reading. Might
            increase file read performance for many smaller files and/or
            some system configurations.

        :returns: Tuple of realtime, monotonic clock based Numpy datetime64
            arrays, and the list of Numpy arrays containing the read channel
            data
        """
        # generate data type to read from header info
        total_bin_bytes = _BINARY_CHANNEL_STUFF_BYTES * ceil(
            file_header["channel_binary_count"] / (_BINARY_CHANNEL_STUFF_BYTES * 8)
        )
        binary_channels_linked = [
            channel["valid_link"] for channel in file_header["channels"]
        ]
        analog_data_formats = []
        analog_data_names = []
        data_formats = []
        data_names = []

        if total_bin_bytes > 0:
            data_names = ["bin"]
            data_formats = [f"<u{total_bin_bytes:d}"]

        for channel in file_header["channels"]:
            if not _CHANNEL_IS_BINARY[channel["unit_index"]]:
                data_format = f"<i{channel['data_size']:d}"
                analog_data_formats.append(data_format)
                data_formats.append(data_format)
                analog_data_names.append(channel["name"])
                data_names.append(channel["name"])

        # read raw data from file
        data_dtype = np.dtype({"names": data_names, "formats": data_formats})
        block_dtype = np.dtype(
            [
                ("realtime_sec", f"<M{_TIMESTAMP_SECONDS_BYTES:d}[s]"),
                ("realtime_ns", f"<m{_TIMESTAMP_NANOSECONDS_BYTES:d}[ns]"),
                ("monotonic_sec", f"<M{_TIMESTAMP_SECONDS_BYTES:d}[s]"),
                ("monotonic_ns", f"<m{_TIMESTAMP_NANOSECONDS_BYTES:d}[ns]"),
                ("data", (data_dtype, (file_header["data_block_size"],))),
            ]
        )

        # access file data, either memory mapped or direct read to memory
        file_handle.seek(file_header["header_length"])
        if memory_mapped:
            file_data = np.memmap(
                file_handle,
                offset=file_header["header_length"],
                mode="r",
                dtype=block_dtype,
                shape=file_header["data_block_count"],
            )
        else:
            file_data = np.fromfile(
                file_handle,
                dtype=block_dtype,
                count=file_header["data_block_count"],
                sep="",
            )

        # reference for data blocks speeds up access time
        block_data = np.array(file_data["data"], copy=False)

        # extract timestamps
        timestamps_realtime = file_data["realtime_sec"] + file_data["realtime_ns"]
        timestamps_monotonic = file_data["monotonic_sec"] + file_data["monotonic_ns"]

        # allocate empty list of channel data
        data = [None] * (
            file_header["channel_binary_count"] + file_header["channel_analog_count"]
        )

        # extract binary channels
        for binary_channel_index in range(file_header["channel_binary_count"]):
            channel_index = binary_channel_index
            data[channel_index] = np.array(
                2 ** binary_channel_index & block_data["bin"], dtype=np.dtype("b1")
            )
            data[channel_index] = data[channel_index].reshape(
                file_header["sample_count"]
            )

            # values decimation
            if decimation_factor > 1:
                # decimate to zero for valid links, threshold otherwise
                if channel_index in binary_channels_linked:
                    data[channel_index] = _decimate_min(
                        data[channel_index], decimation_factor
                    )
                else:
                    data[channel_index] = _decimate_binary(
                        data[channel_index], decimation_factor
                    )

        # extract analog channels
        for analog_channel_index in range(file_header["channel_analog_count"]):
            channel_index = file_header["channel_binary_count"] + analog_channel_index
            data[channel_index] = np.array(
                block_data[analog_data_names[analog_channel_index]],
                dtype=np.dtype(analog_data_formats[analog_channel_index]),
            )
            data[channel_index] = data[channel_index].reshape(
                file_header["sample_count"]
            )

            # values decimation
            if decimation_factor > 1:
                data[channel_index] = _decimate_mean(
                    data[channel_index], decimation_factor
                )

        return timestamps_realtime, timestamps_monotonic, data

    def _get_channel_index(self, channel_name):
        """
        Get the index of a data channel.

        :param channel_name: Name of the channel

        :returns: The index of the channel, `None` if not found
        """
        channel_names = [channel["name"] for channel in self._header["channels"]]
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
            for index in channel_indexes:
                channel_names.append(self._header["channels"][index]["name"])
            return channel_names
        else:
            return self._header["channels"][channel_indexes]["name"]

    def add_channel(self, channel_info, channel_data):
        """
        Add a new data channel to the RocketLogger data structure.

        .. note::

            If a valid channel is to the channel being added, the linked
            channel has to be added first.

        :param channel_info: Channel info structure of the channel to add,
            a dictionary with the following fields designed:

            - "unit_index" -- index of the measurement unit of the channel
            - "scale" -- data unit scale, an integer exponent
            - "data_size" -- size in bytes of the channel data
            - "valid_link" -- index of the valid channel
            - "name" -- name of the channel (max 16 bytes)

        :param channel_data: The actual channel data to add, Numpy array
        """
        if not isinstance(channel_info, dict):
            raise TypeError("Channel info structure is expected to be a dictionary.")

        # check full channel info available
        if "unit_index" not in channel_info:
            raise ValueError("Channel info: unit not defined.")
        if channel_info["unit_index"] not in _CHANNEL_UNIT_NAMES:
            raise ValueError("Channel info: invalid channel unit.")

        if "scale" not in channel_info:
            raise ValueError("Channel info: scale not defined.")
        if not isinstance(channel_info["scale"], int):
            raise TypeError("Channel info: invalid scale.")

        if "data_size" not in channel_info:
            raise ValueError("Channel info: data size not defined.")
        if not isinstance(channel_info["data_size"], int):
            raise TypeError("Channel info: invalid data size.")

        if "valid_link" not in channel_info:
            raise ValueError("Channel info: link not defined.")
        if not isinstance(channel_info["valid_link"], int):
            raise TypeError("Channel info: invalid link.")
        if (channel_info["valid_link"] >= len(self._header["channels"])) & (
            channel_info["valid_link"] != _CHANNEL_VALID_UNLINKED
        ):
            raise ValueError("Channel info: invalid link.")

        if "name" not in channel_info:
            raise ValueError("Channel info: name undefined.")
        if not isinstance(channel_info["name"], str):
            raise TypeError("Channel info: invalid name.")
        if len(channel_info["name"]) > _CHANNEL_NAME_BYTES:
            raise ValueError("Channel info: name too long.")
        if channel_info["name"] in self.get_channel_names():
            raise ValueError(
                f"Cannot add channel with name '{channel_info['name']}', name already in use."
            )

        # check data
        if len(self._header["channels"]) > 0:
            if channel_data.shape != self._data[0].shape:
                raise RocketLoggerDataError(
                    f"Incompatible data size. Expected array of shape {self._data[0].shape}."
                )

        # add channel info and data
        if _CHANNEL_IS_BINARY[channel_info["unit_index"]]:
            self._data.insert(self._header["channel_binary_count"], channel_data)
            self._header["channels"].insert(
                self._header["channel_binary_count"], channel_info
            )
            self._header["channel_binary_count"] = (
                self._header["channel_binary_count"] + 1
            )
        else:
            self._data.append(channel_data)
            self._header["channels"].append(channel_info)
            self._header["channel_analog_count"] = (
                self._header["channel_analog_count"] + 1
            )

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
            raise KeyError(f"Channel '{channel_name}' not found.")

        # update links of channels affected by indexing change
        for channel_info in self._header["channels"]:
            if channel_info["valid_link"] == channel_index:
                channel_info["valid_link"] = _CHANNEL_VALID_UNLINKED
            elif channel_info["valid_link"] > channel_index:
                channel_info["valid_link"] = channel_info["valid_link"] - 1

        # delete actual data and header entry
        del self._data[channel_index]
        del self._header["channels"][channel_index]

        # adjust header fields
        if channel_index < self._header["channel_binary_count"]:
            self._header["channel_binary_count"] = (
                self._header["channel_binary_count"] - 1
            )
        else:
            self._header["channel_analog_count"] = (
                self._header["channel_analog_count"] - 1
            )

    def load_file(
        self,
        filename,
        join_files=True,
        exclude_part=None,
        header_only=False,
        decimation_factor=1,
        recovery=False,
        memory_mapped=True,
    ):
        """
        Read data from a RocketLogger data file.

        :param filename: The filename of the file to import. If numbered
            files following the "<filename>_p#.rld" convention are found, they
            can be joined during import using the `join_files` flag.

        :param join_files: Enable joining of multiple files if numbered files
            following the "<filename>_p#.rld" convention are found

        :param exclude_part: Exclude given part(s) of number files when
            joining following the "<filename>_p#.rld" convention.
            Expects a single index or list or Numpy array of indexes to
            exclude. Silently ignores indexes beyond the number of found parts.
            Only applicable when joining multiple files with `join_files=True`.

        :param header_only: Enable to import only header info without data

        :param decimation_factor: Decimation factor for values read

        :param recovery: Attempt recovery of damaged files that have lost or
            incomplete data blocks at the end of the file. This could e.g. be
            caused by power supply failures during measurements.

        :param memory_mapped: Set `False` to fall back to read entire file to
            memory at once, instead of using memory mapped reading. Might
            increase file read performance for many smaller files and/or
            some system configurations.
        """
        if self._filename is not None:
            raise RocketLoggerDataError(
                "A data file is already loaded. Use separate instance for new file."
            )

        if not join_files and exclude_part is not None:
            raise ValueError("exclude_part on applicable when joining files")

        if exclude_part is None:
            exclude_part = []
        elif isinstance(exclude_part, int):
            exclude_part = [exclude_part]
        elif isinstance(exclude_part, np.ndarray):
            exclude_part = list(exclude_part)

        if not isinstance(exclude_part, list):
            raise ValueError(
                "invalid exclude_part: accepting integer,"
                "a list of integers or an integer Numpy array."
            )

        file_basename, file_extension = splitext(filename)
        file_number = 0
        files_loaded = 0

        while True:
            # skip excluded files
            if file_number in exclude_part:
                file_number = file_number + 1
                continue

            # derive file name from index and abort if next file does not exist
            if file_number == 0:
                file_name = filename
            else:
                file_name = f"{file_basename}_p{file_number}{file_extension}"

            # for multi-part import end import if next file is not found
            if join_files and not isfile(file_name):
                break

            with open(file_name, "rb") as file_handle:
                if files_loaded == 0:
                    # read full header for first file
                    header = self._read_file_header(file_handle)
                else:
                    # read header lead-in only for continuation file
                    header = self._read_file_header_lead_in(file_handle)
                    file_handle.seek(header["header_length"])

                    # consistency check against first file
                    self._validate_matching_header(header, self._header)

                    # reuse previously read header fields
                    header["comment"] = self._header["comment"]
                    header["channels"] = self._header["channels"]

                # validate decimation factor argument
                if (header["data_block_size"] % decimation_factor) > 0:
                    raise ValueError(
                        "Decimation factor needs to be divider of the buffer size."
                    )

                if header_only:
                    # set data arrays to None on header only import
                    self._timestamps_realtime = None
                    self._timestamps_monotonic = None
                    self._data = None
                else:
                    # calculate data block size
                    block_bin_bytes = _BINARY_CHANNEL_STUFF_BYTES * ceil(
                        header["channel_binary_count"]
                        / (_BINARY_CHANNEL_STUFF_BYTES * 8)
                    )
                    block_analog_bytes = sum(
                        [
                            c["data_size"]
                            for c in header["channels"]
                            if not _CHANNEL_IS_BINARY[c["unit_index"]]
                        ]
                    )
                    block_size_bytes = 2 * _TIMESTAMP_BYTES + header[
                        "data_block_size"
                    ] * (block_bin_bytes + block_analog_bytes)

                    # file size and header consistency check and recovery
                    file_size = file_handle.seek(0, os.SEEK_END)
                    file_data_bytes = (
                        header["header_length"]
                        + header["data_block_count"] * block_size_bytes
                    )
                    if file_size != file_data_bytes:
                        data_blocks_recovered = floor(
                            (file_size - header["header_length"]) / block_size_bytes
                        )
                        data_blocks_truncated = (
                            header["data_block_count"] - data_blocks_recovered
                        )
                        if recovery:
                            header["data_block_count"] = data_blocks_recovered
                            header["sample_count"] = (
                                data_blocks_recovered * header["data_block_size"]
                            )
                            warnings.warn(
                                RocketLoggerDataWarning(
                                    f"corrupt data: recovered {data_blocks_recovered}"
                                    f" data blocks, truncating {data_blocks_truncated}"
                                    " incomplete data blocks."
                                )
                            )
                        else:
                            raise RocketLoggerDataError(
                                "corrupt data: file size and header info mismatch, "
                                f"or {data_blocks_truncated} truncated data blocks at "
                                "the file end."
                            )

                    # channels: read actual sampled data
                    (
                        timestamps_realtime,
                        timestamps_monotonic,
                        data,
                    ) = self._read_file_data(
                        file_handle,
                        header,
                        decimation_factor=decimation_factor,
                        memory_mapped=memory_mapped,
                    )

                    # store new data array on first file, append on following
                    if files_loaded > 0:
                        self._timestamps_realtime = np.concatenate(
                            (self._timestamps_realtime, timestamps_realtime)
                        )
                        self._timestamps_monotonic = np.concatenate(
                            (self._timestamps_monotonic, timestamps_monotonic)
                        )
                        for i in range(len(self._data)):
                            self._data[i] = np.concatenate((self._data[i], data[i]))
                    else:
                        self._timestamps_realtime = timestamps_realtime
                        self._timestamps_monotonic = timestamps_monotonic
                        self._data = data

                # multi-file header
                if files_loaded == 0:
                    self._header = header
                else:
                    self._header["sample_count"] = (
                        self._header["sample_count"] + header["sample_count"]
                    )
                    self._header["data_block_count"] = (
                        self._header["data_block_count"] + header["data_block_count"]
                    )

            file_number = file_number + 1
            files_loaded = files_loaded + 1

            # skip looking for next file if joining not enabled
            if not join_files:
                break

        # check valid data loaded
        if files_loaded == 0:
            raise RocketLoggerDataError(
                f"Could not load valid data from '{filename}' "
                "and current import configuration."
            )

        # adjust header files for decimation
        self._header["sample_count"] = round(
            self._header["sample_count"] / decimation_factor
        )
        self._header["data_block_size"] = round(
            self._header["data_block_size"] / decimation_factor
        )
        self._header["sample_rate"] = round(
            self._header["sample_rate"] / decimation_factor
        )

        self._filename = filename

    def get_channel_names(self):
        """
        Get the names of all the channels loaded from file.

        :returns: List of channel names sorted by name
        """
        channel_names = []
        for channel in self._header["channels"]:
            channel_names.append(channel["name"])
        return sorted(channel_names)

    def get_comment(self):
        """
        Get the comment stored in the file header.

        :returns: Comment stored in the file
        """
        return self._header["comment"]

    def get_header(self):
        """
        Get a dictionary with the header information.

        The relevant fields include: "data_block_count", "data_block_size",
        "file_version", "mac_address", "sample_count","sample_rate", and
        "start_time".

        To get the file comment use :func:`get_comment`.

        :returns: Dictionary of relevant header information
        """

        header_dict = {}
        for key, value in self._header.items():
            if key in [
                "data_block_count",
                "data_block_size",
                "file_version",
                "sample_count",
                "sample_rate",
                "start_time",
            ]:
                header_dict[key] = value
            elif key == "mac_address":
                header_dict[key] = ":".join([f"{v:02x}" for v in value])

        return header_dict

    def get_filename(self):
        """
        Get the filename of the loaded data file.

        :returns: The absolute filename of the loaded data file
        """
        return os.path.abspath(self._filename)

    def get_data(self, channel_names=["all"]):
        """
        Get the data of the specified channels, by default of all channels.

        :param channel_names: The names of the channels for which the data
            shall be returned. List of channel names or "all" to select all
            channels.

        :returns: A Numpy array containing the channel's data vectors
        """
        if not isinstance(channel_names, list):
            channel_names = [channel_names]

        if "all" in channel_names:
            channel_names = self.get_channel_names()

        if self._data is None:
            raise TypeError("No data to access for header only imported file.")

        values = np.empty((self._header["sample_count"], len(channel_names)))

        for channel_name in channel_names:
            index = self._get_channel_index(channel_name)
            if index is None:
                raise KeyError(f"Channel '{channel_name}' not found.")
            values[:, channel_names.index(channel_name)] = self._data[index] * 10 ** (
                self._header["channels"][index]["scale"]
            )

        return values

    def get_time(self, time_reference="relative"):
        """
        Get the timestamp of the data.

        Using simple linear interpolation to generating the sample from the
        block timestamps. For local or network timestamps the values for the
        last data block are extrapolated using the average time delta of all
        data blocks.

        :param time_reference: The reference to use for timestamp calculation:

            - "relative" -- Calculate timestamp from sample rate, and the sample
              index relative to the measurement start time (default)
            - "local" -- Get the timestamp of the local oscillator clock
            - "network" -- Get the timestamp of the network synchronized clock

        :returns: A Numpy array containing the timestamps
        """
        if self._timestamps_monotonic is None:
            raise TypeError("No data to access for header only imported file.")

        # get requested timer data as numbers
        if time_reference == "relative":
            timestamps = np.arange(0, self._header["sample_count"]) / (
                self._header["sample_rate"] * _ROCKETLOGGER_ADC_CLOCK_SCALE
            )
        elif time_reference == "local":
            timestamps = self._timestamps_monotonic.astype("<i8")
        elif time_reference == "network":
            timestamps = self._timestamps_realtime.astype("<i8")
        else:
            raise ValueError(f"Time reference '{time_reference}' undefined.")

        # interpolate absolute time references (and extrapolate for last data block)
        if time_reference in ["local", "network"]:
            block_points = np.arange(
                0, self._header["sample_count"] + 1, self._header["data_block_size"]
            )
            block_timestamps = np.concatenate(
                (timestamps, [timestamps[-1] + np.diff(timestamps).mean()])
            )
            data_points = np.arange(0, self._header["sample_count"])

            data_timestamp = np.interp(data_points, block_points, block_timestamps)

            # convert to datetime again
            timestamps = data_timestamp.astype("datetime64[ns]")

        return timestamps

    def get_unit(self, channel_names=["all"]):
        """
        Get the unit of the specified channels, by default of all channels.

        :param channel_names: The names of the channels for which the unit
            shall be returned. List of channel names or "all" to select all
            channels.

        :returns: List of channel units sorted by channel_names list
        """
        if not isinstance(channel_names, list):
            channel_names = [channel_names]

        if "all" in channel_names:
            channel_names = self.get_channel_names()

        channel_units = []
        for channel_name in channel_names:
            index = self._get_channel_index(channel_name)
            if index is None:
                raise KeyError(f"Channel '{channel_name}' not found.")
            channel_units.append(self._header["channels"][index]["unit"])

        return channel_units

    def get_validity(self, channel_names=["all"]):
        """
        Get the validity of the specified channels, by default of all channels.

        :param channel_names: The names of the channels for which the validity
            shall be returned. List of channel names or "all" to select all
            channels.

        :returns: A Numpy array containing the channel's validity vectors
        """
        if not isinstance(channel_names, list):
            channel_names = [channel_names]

        if "all" in channel_names:
            channel_names = self.get_channel_names()

        if self._data is None:
            raise TypeError("No data to access for header only imported file.")

        validity = np.ones(
            (self._header["sample_count"], len(channel_names)), dtype=bool
        )

        for channel_name in channel_names:
            data_index = self._get_channel_index(channel_name)
            if data_index is None:
                raise KeyError(f"Channel '{channel_name}' not found.")
            valid_link = self._header["channels"][data_index]["valid_link"]
            if valid_link != _CHANNEL_VALID_UNLINKED:
                validity[:, channel_names.index(channel_name)] = self._data[valid_link]

        return validity

    def merge_channels(self, keep_channels=False):
        """
        Merge seamlessly switched current channels into a combined channel.

        :param keep_channels: Whether the merged channels are kept

        :returns: Self reference to data object
        """
        if self._data is None:
            raise TypeError(
                "Data operations not permitted for header only imported file."
            )

        merged_channels = False
        for candidate in _CHANNEL_MERGE_CANDIDATES:
            # check if inputs available and linked
            low_index = self._get_channel_index(candidate["low"])
            high_index = self._get_channel_index(candidate["high"])

            # skip if one input unavailable
            if (low_index is None) or (high_index is None):
                continue

            # error if merge target exists
            if self._get_channel_index(candidate["merged"]) is not None:
                raise RocketLoggerDataError(
                    "Name of merged output channel "
                    f"'{candidate['merged']}' already in use."
                )

            # check for channel valid link
            low_channel = self._header["channels"][low_index]
            high_channel = self._header["channels"][high_index]
            low_valid_index = low_channel["valid_link"]
            if low_valid_index > len(self._header["channels"]):
                raise RocketLoggerDataError(
                    f"Channel '{low_channel['name']}' has invalid link ({low_valid_index})."
                )

            # build merged channel info and data
            merged_channel_info = {}
            merged_channel_info["unit"] = low_channel["unit"]
            merged_channel_info["unit_index"] = low_channel["unit_index"]
            merged_channel_info["scale"] = low_channel["scale"]
            merged_channel_info["data_size"] = (
                low_channel["data_size"] + high_channel["data_size"]
            )
            merged_channel_info["valid_link"] = _CHANNEL_VALID_UNLINKED
            merged_channel_info["name"] = candidate["merged"]

            # merge data: force data type to prevent calculation overflow
            merged_data = (1 * self._data[low_valid_index]) * self._data[low_index] + (
                1 * np.logical_not(self._data[low_valid_index])
            ) * self._data[high_index].astype(
                np.dtype(f"<i{merged_channel_info['data_size']:d}")
            ) * 10 ** (
                high_channel["scale"] - low_channel["scale"]
            )

            # add merged channel
            self.add_channel(merged_channel_info, merged_data)

            # drop original channels if not kept, update header info
            if not keep_channels:
                valid_channel_name = self._get_channel_name(low_valid_index)
                self.remove_channel(valid_channel_name)
                self.remove_channel(candidate["low"])
                self.remove_channel(candidate["high"])

            merged_channels = True

        if not merged_channels:
            warnings.warn(RocketLoggerDataWarning("No channels found to merge."))

        return self

    def get_dataframe(self, channel_names=["all"], time_reference="relative"):
        """
        Shortcut to get a pandas dataframe of selected channels indexed with
        timestamps.

        By default all channels are exported and relative timestamps are used
        for indexing. See also :func:`get_data` and :func:`get_time` functions.

        Requires pandas package to be installed.

        :param channel_names: The names of the channels for which the data
            shall be returned. List of channel names or "all" to select all
            channels.

        :param time_reference: The reference to use for timestamp calculation:

            - "relative" -- Calculate timestamp from sample rate, and the sample
              index relative to the measurement start time (default)
            - "local" -- Get the timestamp of the local oscillator clock
            - "network" -- Get the timestamp of the network synchronized clock

        :returns: A pandas dataframe the channel's data as columns, indexed
            by the timestamps of the selected format
        """
        import pandas as pd

        if not isinstance(channel_names, list):
            channel_names = [channel_names]

        if "all" in channel_names:
            channel_names = self.get_channel_names()

        df = pd.DataFrame(
            self.get_data(channel_names=channel_names),
            index=self.get_time(time_reference=time_reference),
            columns=channel_names,
        )
        return df

    def plot(self, channel_names=["all"], show=True):
        """
        Plot the loaded RocketLogger data.

        Requires matplotlib package to be installed.

        :param channel_names: The names of the channels for which the
            data shall be returned. List of channel names (or combination of):

            - "all" -- to select all channels
            - "voltages" -- to select voltage channels
            - "currents" -- to select current channels
            - "digital" -- to select digital channels

        :param show: Whether to show the plot window or not

        :returns: The matplotlib plot object used for plotting
        """
        import matplotlib.pyplot as plt

        if not isinstance(channel_names, list):
            channel_names = [channel_names]

        channels_digital = self._header["channels"][
            0 : self._header["channel_binary_count"]
        ]
        channels_current = [
            channel
            for channel in self._header["channels"]
            if channel["unit"] == "current"
        ]
        channels_voltage = [
            channel
            for channel in self._header["channels"]
            if channel["unit"] == "voltage"
        ]
        channels_others = [
            channel
            for channel in self._header["channels"][
                self._header["channel_binary_count"] :
            ]
            if channel["unit"] not in ["voltage", "current"]
        ]

        plot_current_channels = []
        plot_digital_channels = []
        plot_voltage_channels = []
        plot_other_channels = []

        # get and group channels to plot
        for channel_name in channel_names:
            if channel_name == "all":
                plot_current_channels = channels_current
                plot_digital_channels = channels_digital
                plot_voltage_channels = channels_voltage
                plot_other_channels = channels_others
                break
            elif channel_name == "voltages":
                plot_voltage_channels = channels_voltage
            elif channel_name == "currents":
                plot_current_channels = channels_current
            elif channel_name == "digital":
                plot_digital_channels = channels_digital
            else:
                channel_index = self._get_channel_index(channel_name)
                if channel_index is None:
                    raise KeyError(f"Channel '{channel_name}' not found.")
                if channel_name in [ch["name"] for ch in channels_voltage]:
                    plot_voltage_channels.append(
                        self._header["channels"][channel_index]
                    )
                elif channel_name in [ch["name"] for ch in channels_current]:
                    plot_current_channels.append(
                        self._header["channels"][channel_index]
                    )
                elif channel_name in [ch["name"] for ch in channels_digital]:
                    plot_digital_channels.append(
                        self._header["channels"][channel_index]
                    )

        # prepare plot groups
        plot_groups = [
            plot_voltage_channels,
            plot_current_channels,
            plot_digital_channels,
            plot_other_channels,
        ]
        plot_groups_axis_label = ["voltage [V]", "current [A]", "digital", ""]
        plot_time = self.get_time()

        subplot_count = len([group for group in plot_groups if len(group) > 0])
        fig, ax = plt.subplots(subplot_count, 1, sharex="col")
        if subplot_count == 1:
            ax = [ax]
        plt.suptitle(self._filename)

        # plot
        subplot_index = 0
        for i in range(len(plot_groups)):
            plot_channels = plot_groups[i]
            if len(plot_channels) == 0:
                continue
            plot_channel_names = [channel["name"] for channel in plot_channels]
            plot_channel_data = self.get_data(plot_channel_names)
            ax[subplot_index].plot(plot_time, plot_channel_data)
            ax[subplot_index].set_ylabel(plot_groups_axis_label[i])
            ax[subplot_index].legend(plot_channel_names)
            subplot_index = subplot_index + 1

        ax[subplot_count - 1].set_xlabel("time [s]")

        if show:
            plt.show(block=False)

        return plt
