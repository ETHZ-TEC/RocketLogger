"""
RocketLogger Calibration Support.

Calibration file generation and accuracy verification.

Copyright (c) 2019, Swiss Federal Institute of Technology (ETH Zurich)
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

import os
import sys

# from datetime import datetime, timezone
# from math import ceil, floor

import numpy as np
# import matplotlib.pyplot as plt

from .data import RocketLoggerData

# channel count and indexes
_ROCKETLOGGER_CHANNELS_COUNT = 8
_ROCKETLOGGER_CHANNELS_VOLTAGE = [2, 3, 6, 7]
_ROCKETLOGGER_CHANNELS_CURRENT_LOW = [1, 5]
_ROCKETLOGGER_CHANNELS_CURRENT_HIGH = [0, 4]

# scales (per bit) for the values, that are stored in the binary files
_ROCKETLOGGER_FILE_SCALE_V = 1e-8
_ROCKETLOGGER_FILE_SCALE_IL = 1e-11
_ROCKETLOGGER_FILE_SCALE_IH = 1e-9

# voltage/current increment per ADC LSB
_ROCKETLOGGER_ADC_STEP_V = -1.22e-6
_ROCKETLOGGER_ADC_STEP_IL = 1.755e-10
_ROCKETLOGGER_ADC_STEP_IH = 3.18e-08


ROCKETLOGGER_CALIBRATION_FILE = '/etc/rocketlogger/calibration.dat'

_CALIBRATION_FILE_DTYPE = np.dtype([
    ('timestamp', '<M8[s]'),
    ('offset', ('<i4', 8)),
    ('scale', ('<f8', 8)),
])

# channels with positive scales
_CALIBRATION_POSITIVE_SCALE_CHANNELS = (_ROCKETLOGGER_CHANNELS_CURRENT_LOW +
                                        _ROCKETLOGGER_CHANNELS_CURRENT_HIGH)


class RocketLoggerCalibrationError(Exception):
    """RocketLogger calibration related errors."""

    pass


class RocketLoggerCalibration:
    """
    RocketLogger calibration support class.

    File reading and basic data processing support for binary RocketLogger data
    files.
    """

    _data_v = None
    _data_i1l = None
    _data_i1h = None
    _data_i2l = None
    _data_i2h = None

    _calibration_timestamp = None
    _calibration_scale = None
    _calibration_offset = None
    _calibration_residuals = None

    _error_offset = None
    _error_scale = None

    def __init__(self, *args):
        """
        Initialize RocketLogger Calibration helper class
        """
        # treat 1 arguments as an existing calibration to load
        if len(args) == 1:
            self.read_calibration_file(*args)

        # treat 5 arguments as measurement data for a new calibration
        if len(args) == 5:
            self.load_measurement_data(*args)

    def load_measurement_data(self, data_v, data_i1l, data_i1h,
                              data_i2l, data_i2h):
        """
        Load calibration measurement data from RocketLoggerData structures or
        RocketLogger data files. Loading new measurement data invalidates
        previously made calibration.

        :param data_v: Voltage V1-V4 calibration measurement data or filename.

        :param data_i1l: Current I1L calibration measurement data or filename.

        :param data_i1h: Current I1H calibration measurement data or filename.

        :param data_i2l: Current I2L calibration measurement data or filename.

        :param data_i2h: Current I2H calibration measurement data or filename.
        """
        for data in [data_v, data_i1l, data_i1h, data_i2l, data_i2h]:
            if type(data) is RocketLoggerData:
                continue
            elif os.path.isfile(data):
                data = RocketLoggerData(data)
            else:
                raise ValueError('"{}" is not valid measurement data nor an '
                                 'existing data file.'.format(data))

        # store the loaded data in class
        self._data_v = data_v
        self._data_i1l = data_i1l
        self._data_i1h = data_i1h
        self._data_i2l = data_i2l
        self._data_i2h = data_i2h

        # reset existing error calculations, keep calibration values if loaded
        self._error_offset = None
        self._error_scale = None

    def recalibrate(self, setup, fix_signs=True):
        """
        Perform channel calibration with loaded measeurement data, overwriting
        any loaded calibration parameters.

        :param setup: The calibration setup used for the measurements using the
                      RocketLoggerCalibrationSetup helper class to describe

        "param fix_signs: Automatically fix sign error in calibrations
        """
        raise NotImplementedError()

        self._check_data_loaded()

        # extract calibration points and revert file scaling

        # perform regression
        self._error_offset = None
        self._calibration_scale = None

        # store values
        self._error_offset = None
        self._calibration_scale = None
        self._calibration_timestamp = min(x._header['start_time'] for x in
                                          [self.data_v, self.data_i1l,
                                           self.data_i1h, self.data_i2l,
                                           self.data_i2h])

        # check and invert signs
        if fix_signs:
            for ch in ROCKETLOGGER_POSITIVE_SCALE_CHANNELS:
                # check and invert if necessary
                if self._calibration_scale[ch] < 0:
                    self._calibration_scale[ch] = -self._calibration_scale[ch]

    def print_statistics(self):
        """
        Print statistics of the calibration.
        """
        self._check_calibration_exists()

        print('RocketLogger Calibration Statistics\n')
        print('\n')
        print('Calibration Time:', self._calibration_timestamp, '\n')
        print('\n')
        if any(self._calibration_offset, self._calibration_scale) is None:
            print('no calibration error data available\n')
        else:
            print('Voltage Channel Calibration Errors\n')
            # for ch=1:4
            #     i = rl_cal.V_INDEX(ch)
            #     print('  Voltage V%i:               %6.3f%% + %9.3f uV\n', ...
            #         ch, 100 * obj.error_scales(i),  1e6 * obj.error_offsets(i))
            print('\n')
            print('Current Channel Calibration Errors\n')
            # for ch=1:2
            #     il = rl_cal.IL_INDEX(ch)
            #     ih = rl_cal.IH_INDEX(ch)
            #     print('  Current I%i (Low Range):   %6.3f%% + %9.3f nA\n', ...
            #         ch, 100 * obj.error_scales(il), 1e9 * obj.error_offsets(il))
            #     print('  Current I%i (High Range):  %6.3f%% + %9.3f nA\n', ...
            #         ch, 100 * obj.error_scales(ih), 1e9 * obj.error_offsets(ih))
            raise NotImplementedError()

    def read_calibration_file(self, filename=ROCKETLOGGER_CALIBRATION_FILE):
        """
        Load an existing calibration file.
        """
        data = np.fromfile(filename, dtype=_CALIBRATION_FILE_DTYPE).squeeze()

        self._calibration_timestamp = data['timestamp']
        self._calibration_offset = data['offset']
        self._calibration_scale = data['scale']

        if self._calibration_timestamp > np.datetime64('now'):
            raise RocketLoggerCalibrationError(
                'Invalid timestamp in calibration file')
        if len(self._calibration_offset) != 8:
            raise RocketLoggerCalibrationError(
                'Invalid offset data in calibration file')
        if len(self._calibration_scale) != 8:
            raise RocketLoggerCalibrationError(
                'Invalid scale data in calibration file')

        # reset existing error calculations, keep measurement data if loaded
        self._error_offset = []
        self._error_scale = []

    def write_calibration_file(self, filename='calibration.dat'):
        """
        Write the calibration to file.

        :param filename: Name of the file to write the calibration values to
        """
        self._check_calibration_exists()

        # assemble file data write to file
        filedata = np.array([(self._calibration_timestamp,
                              self._calibration_offset,
                              self._calibration_scale)],
                            dtype=_CALIBRATION_FILE_DTYPE)
        filedata.tofile(filename)

    def write_log_file(self, filename='calibration.log'):
        """
        Write log of performed recalibration to file. Raises a calibration
        error if no recalibration has been performed beforehand.

        :param filename: Name of the file to write the calibration log to
        """
        # redirect statistics output to log file
        with open(filename, 'w') as sys.stdout:
            self.print_statistics()

    def _check_data_loaded(self):
        """Check for loaded calibration measurements, raise error if not."""
        if any([self.data_v, self.data_i1l, self.data_i1h, self.data_i2l,
                self.data_i2h]) is None:
            raise RocketLoggerCalibrationError(
                'No data measurement data loaded for recalibration.\n' +
                'Use load_measurement_data() first to load data.')

    def _check_calibration_exists(self):
        """Check for existing calibration data, raise error if not."""
        if self._calibration_timestamp is None:
            raise RocketLoggerCalibrationError(
                'No calibration data available. Perform recalibration first.')

    def __eq__(self, other):
        """Return True if other is the same calibration."""
        if isinstance(other, RocketLoggerCalibration):
            return (
                (self._calibration_timestamp == other._calibration_timestamp) &
                np.array_equal(self._calibration_scale,
                               other._calibration_scale) &
                np.array_equal(self._calibration_offset,
                               other._calibration_offset)
            )
        return False
