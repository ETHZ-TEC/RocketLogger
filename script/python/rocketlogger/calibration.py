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

import numpy as np

from .data import RocketLoggerData, RocketLoggerFileError

ROCKETLOGGER_CALIBRATION_FILE = '/etc/rocketlogger/calibration.dat'
"""Default RocketLogger calibration file location."""

# # scales (per bit) for the values, that are stored in the binary files
# _ROCKETLOGGER_FILE_SCALE_V = 1e-8
# _ROCKETLOGGER_FILE_SCALE_IL = 1e-11
# _ROCKETLOGGER_FILE_SCALE_IH = 1e-9

# hardware design values of voltage/current increment per ADC LSB
_ROCKETLOGGER_ADC_STEP_V = -1.22e-6
_ROCKETLOGGER_ADC_STEP_IL = 1.755e-10
_ROCKETLOGGER_ADC_STEP_IH = 3.18e-08

# data format of the calibration file
_CALIBRATION_FILE_DTYPE = np.dtype([
    ('file_magic', '<u4'),
    ('file_version', '<u2'),
    ('header_length', '<u2'),
    ('calibration_time', '<M8[s]'),
    ('offset', ('<i4', 8)),
    ('scale', ('<f8', 8)),
])

# calibration file format magic
_CALIBRATION_FILE_MAGIC = 0x434C5225

# calibration file format version
_CALIBRATION_FILE_VERSION = 0x02

# calibration file header length
_CALIBRATION_FILE_HEADER_LENGTH = 0x10

# data format channel name sequence
_CALIBRATION_CHANNEL_NAMES = ['V1', 'V2', 'V3', 'V4',
                              'I1L', 'I1H', 'I2L', 'I2H']

# channels with positive scales
_CALIBRATION_CHANNEL_SCALES_POSITIVE = ([False] * 4 + [True] * 4)

# RocketLogger calibration scale scales for channels
_CALIBRATION_CHANNEL_SCALES = ([1e-8] * 4 + [1e-11, 1e-9] * 2)


def regression_linear(measurement, reference, zero_weight=1):
    """
    Perform linear regression with extra weight on zero values.

    :param reference: the reference value to calibrate for

    :param measurement: the measurement to calibrate on the reference

    :returns: (offset, scale) tuple of offset and scale values
    """

    weight = 1 + np.isclose(np.abs(reference), 0) * (zero_weight - 1)
    poly_coeffs = np.polyfit(measurement, reference, 1, w=weight)
    offset = round(poly_coeffs[1] / poly_coeffs[0])
    scale = poly_coeffs[0]

    return (offset, scale)


def _extract_setpoint_measurement(measurement_data, setpoint_step,
                                  filter_window_length=150,
                                  filter_threshold_relative=0.01):
    """
    Extract aggregated set-points from a measurement trace.

    :param measurement_data: vector of measurement data to extract set-points
                             from

    :param setpoint_step: the set-point step (in measurement units)

    :param filter_window_length: width of the square filter window used for
                                 set-point filtering

    :param filter_threshold_relative: set-point step relative threshold for
                                      stable measurement detection

    :returns: vector of the extracted set-point measurements
    """
    # find stable measurement intervals
    measurement_diff_abs = np.abs(np.diff(np.hstack([0, measurement_data])))
    measurement_stable = (np.abs(measurement_diff_abs) <
                          filter_threshold_relative * setpoint_step)
    filter_window = np.ones(filter_window_length) / filter_window_length
    measurement_filtered = (np.convolve(measurement_stable, filter_window,
                                        mode='same') >= np.sum(filter_window))

    # if no set-point is found report error
    if not np.any(measurement_filtered):
        raise Warning('No set-points found. You might want to check the '
                      'set-point step scale?')

    # extract set-point measurement intervals
    setpoint_boundary = np.diff(np.hstack([0, measurement_filtered]))
    setpoint_start = np.array((setpoint_boundary > 0).nonzero()) - \
        int(filter_window_length / 2)
    setpoint_end = np.array((setpoint_boundary < 0).nonzero()) + \
        int(filter_window_length / 2)

    setpoint_trim = (setpoint_start > filter_window_length) & \
        (setpoint_end < len(measurement_data) - filter_window_length)
    setpoint_intervals = zip(setpoint_start[setpoint_trim],
                             setpoint_end[setpoint_trim])

    # aggregate measurements by set-point interval
    setpoint_mean = [np.mean(measurement_data[start:end])
                     for start, end in setpoint_intervals]
    return setpoint_mean


class RocketLoggerCalibrationError(Exception):
    """RocketLogger calibration related errors."""

    pass


class RocketLoggerCalibrationSetup:
    """
    RocketLogger calibration measurement setup helper class.
    """

    def __init__(self, setpoint_count, setpoint_step_voltage,
                 setpoint_step_current_low, setpoint_step_current_high,
                 dual_sweep):
        """
        Initialize RocketLogger calibration setup class.

        :param setpoint_count: number of calibration set-points per channel

        :param setpoint_step_voltage: voltage step between set-points

        :param setpoint_step_current_low: low current step between set-points

        :param setpoint_step_current_high: high current step between set-points

        :param min_stable_samples: min number of stable samples required

        :param dual_sweep: set True if dual sweep (up/down) is used
        """

        # check for valid sweep measurement points
        if not ((setpoint_count - 1) % 2) == 0:
            raise ValueError(
                'Linear sweep expects odd number of measurement points')
        if dual_sweep and not ((setpoint_count - 1) % 4) == 0:
            raise ValueError(
                'Dual sweep expects multiple of 4 + 1 measurement points')

        self._setpoint_count = setpoint_count
        self._step_voltage = setpoint_step_voltage
        self._step_current_low = setpoint_step_current_low
        self._step_current_high = setpoint_step_current_high
        self._dual_sweep = dual_sweep

    def get_voltage_setpoints(self, calibration=False):
        """
        Get the real voltage set-points used in the measurement setup.

        :param calibration: set True to get step in estimated ADC bits

        :returns: vector of the calibration set-points in volt or ADC bits
        """
        return self._get_setpoints(self.get_voltage_step(calibration))

    def get_current_low_setpoints(self, calibration=False):
        """
        Get the real low current set-points used in the measurement setup.

        :param calibration: set True to get step in estimated ADC bits

        :returns: vector of the calibration set-points in ampere or ADC bits
        """
        return self._get_setpoints(self.get_current_low_step(calibration))

    def get_current_high_setpoints(self, calibration=False):
        """
        Get the real low current set-points used in the measurement setup.

        :param calibration: set True to get step in estimated ADC bits

        :returns: vector of the calibration set-points in ampere or ADC bits
        """
        return self._get_setpoints(self.get_current_high_step(calibration))

    def get_voltage_step(self, calibration=False):
        """Get the absolute voltage step used in the measurement setup.

        :param calibration: set True to get step in estimated ADC bits

        :returns: the absolute set-point step value in volt or ADC bits
        """
        if calibration:
            return abs(self._step_voltage / _ROCKETLOGGER_ADC_STEP_V)
        return abs(self._step_voltage)

    def get_current_low_step(self, calibration=False):
        """Get the absolute low current step used in the measurement setup.

        :param calibration: set True to get step in estimated ADC bits

        :returns: the absolute set-point step value in ampere or ADC bits
        """
        if calibration:
            return abs(self._step_current_low / _ROCKETLOGGER_ADC_STEP_IL)
        return abs(self._step_current_low)

    def get_current_high_step(self, calibration=False):
        """Get the absolute low current step used in the measurement setup.

        :param calibration: set True to get step in estimated ADC bits

        :returns: the absolute set-point step value in ampere or ADC bits
        """
        if calibration:
            return abs(self._step_current_high / _ROCKETLOGGER_ADC_STEP_IH)
        return abs(self._step_current_high)

    def get_setpoint_count(self):
        """
        Get the number of set-points used in the setup set-point.

        :returns: the number of set-points
        """
        return self._setpoint_count

    def _get_max_setpoint(self, setpoint_step):
        """
        Get the maximum absolute value of the extreme set-point.

        :param setpoint_step: the step size between set-points

        :returns: maximum absolute set-point value
        """
        if self._dual_sweep:
            return setpoint_step * (self._setpoint_count - 1) / 4
        else:
            return setpoint_step * (self._setpoint_count - 1) / 2

    def _get_setpoints(self, setpoint_step):
        """
        Generate the set-points using a given step size.

        :param setpoint_step: the step size to use for the set-point generation

        :returns: vector of the set-point values
        """
        max_value = self._get_max_setpoint(setpoint_step)
        setpoints = np.arange(-max_value, max_value + setpoint_step,
                              setpoint_step)

        # add down sweep for dual sweep setups
        if self._dual_sweep:
            setpoints = np.hstack([setpoints, setpoints[-2::-1]])
        return setpoints


CALIBRATION_SETUP_SMU2450 = RocketLoggerCalibrationSetup(
    setpoint_count=201,
    setpoint_step_voltage=100e-3,
    setpoint_step_current_low=20e-6,
    setpoint_step_current_high=2e-3,
    dual_sweep=True,
)
"""Reference calibration setup using the Keithley 2450 SMU setup."""

CALIBRATION_SETUP_BASIC = RocketLoggerCalibrationSetup(
    setpoint_count=3,
    setpoint_step_voltage=5.0,
    setpoint_step_current_low=2e-3,
    setpoint_step_current_high=0.5,
    dual_sweep=False,
)
"""Basic calibration setup with 3 point measurements (min, zero, max)."""


class RocketLoggerCalibration:
    """
    RocketLogger calibration support class.

    File reading and basic data processing support for binary RocketLogger data
    files.
    """

    def __init__(self, *args):
        """
        Initialize RocketLogger Calibration helper class.

        With 5 parameters it can be used as shortcut for
        :read_calibration_file(): to load an existing calibration.
        With 5 parameters it can be used as shortcut for
        :load_measurement_data(): to load calibration measurement data.
        """
        self._data_v = None
        self._data_i1l = None
        self._data_i1h = None
        self._data_i2l = None
        self._data_i2h = None

        self._calibration_time = None
        self._calibration_scale = None
        self._calibration_offset = None
        self._calibration_residuals = None

        self._error_offset = None
        self._error_scale = None
        self._error_rmse = None

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

        # data load helper function
        def _rld_copy_or_load(data):
            if type(data) is RocketLoggerData:
                return data
            elif os.path.isfile(data):
                return RocketLoggerData(data)
            raise ValueError('"{}" is not valid measurement data nor an '
                             'existing data file.'.format(data))

        # store the loaded data in class
        self._data_v = _rld_copy_or_load(data_v)
        self._data_i1l = _rld_copy_or_load(data_i1l)
        self._data_i1h = _rld_copy_or_load(data_i1h)
        self._data_i2l = _rld_copy_or_load(data_i2l)
        self._data_i2h = _rld_copy_or_load(data_i2h)

        # reset existing error calculations, keep calibration values if loaded
        self._error_offset = None
        self._error_scale = None
        self._error_rmse = None

    def recalibrate(self, setup, fix_signs=True, target_offset_error=1,
                    regression_algorithm=regression_linear, **kwargs):
        """
        Perform channel calibration with loaded measurement data, overwriting
        any loaded calibration parameters.

        :param setup: Calibration setup used for the measurements using the
                      RocketLoggerCalibrationSetup helper class to describe

        :param fix_signs: Set True to automatically fix sign error in
                          calibration scales

        :param target_offset_error: Factor in [1, inf) specifying the multiple
                                    of the zero error to use as offset error
                                    for the error calculations

        :param regression_algorithm: Algorithm to use for the regression,
                                     taking the set-point measurement and
                                     reference values as arguments and
                                     providing the resulting offset and scale
                                     as output

        :param kwargs: Optional names arguments passed to the regression
                       algorithm function
        """
        self._check_data_loaded()
        if target_offset_error < 1:
            raise ValueError('target_offset_error factor needs to be >= 1.')

        # forced reference information
        v_ref = setup.get_voltage_setpoints()
        ih_ref = setup.get_current_high_setpoints()
        il_ref = setup.get_current_low_setpoints()
        reference = [v_ref] * 4 + [il_ref, ih_ref] * 2

        v_step = setup.get_voltage_step(calibration=True)
        ih_step = setup.get_current_high_step(calibration=True)
        il_step = setup.get_current_low_step(calibration=True)
        reference_steps = [v_step] * 4 + [il_step, ih_step] * 2

        # extract measurement information
        measurement_data = [
            self._data_v, self._data_v, self._data_v, self._data_v,
            self._data_i1l, self._data_i1h, self._data_i2l, self._data_i2h]
        measurement_traces = [data.get_data(channel).squeeze()
                              for data, channel
                              in zip(measurement_data,
                                     _CALIBRATION_CHANNEL_NAMES)]
        timestamp = np.datetime64(min([x._header['start_time']
                                       for x in measurement_data]))
        measurement = [_extract_setpoint_measurement(t, s)
                       for t, s in zip(measurement_traces, reference_steps)]

        # perform regression
        regression_result = [regression_algorithm(x, y)
                             for x, y in zip(measurement, reference)]
        offsets, scales = zip(*regression_result)

        # calculate residuals and errors
        residual = [(x + offset) * scale - y for x, y, offset, scale
                    in zip(measurement, reference, offsets, scales)]
        rmse_errors = [np.sqrt(np.dot(res, res)) for res in residual]
        offset_errors = [
            np.max(np.abs(res[np.isclose(ref, 0)])) * target_offset_error
            for res, ref in zip(residual, reference)]
        scale_errors = [
            np.abs(np.fmin(np.abs(res - offset_error),
                           np.abs(res + offset_error)) / ref)
            for res, ref, offset_error
            in zip(residual, reference, offset_errors)]

        # store data
        self._calibration_offset = np.array(offsets, dtype='<i4')
        self._calibration_scale = np.array(
            [scale / file_scale for scale, file_scale
             in zip(scales, _CALIBRATION_CHANNEL_SCALES)], dtype='<f8')
        self._calibration_time = np.array(timestamp, dtype='datetime64[s]')
        self._error_offset = np.array(offset_errors)
        self._error_scale = np.array(
            [np.max(scale_error[np.abs(ref) > 0.1 * np.abs(np.max(ref))])
             for scale_error, ref in zip(scale_errors, reference)])
        self._error_rmse = np.array(rmse_errors)

        # check and invert scales where necessary
        if fix_signs:
            for ch, pos in enumerate(_CALIBRATION_CHANNEL_SCALES_POSITIVE):
                if pos and self._calibration_scale[ch] < 0:
                    self._calibration_scale[ch] = -self._calibration_scale[ch]

    def print_statistics(self):
        """
        Print statistics of the calibration.
        """
        print('RocketLogger Calibration Statistics')
        try:
            self._check_calibration_exists()
            print('Measurement time:   {}'.format(self._calibration_time))
            print()
            print('Calibration values:')
            print('  Channel  :  Offset [bit]  :  Scale [unit/bit]')
            for i, name in enumerate(_CALIBRATION_CHANNEL_NAMES):
                print('  {:7s}  :  {:12d}  :  {:16.6f}'.format(
                    name, self._calibration_offset[i],
                    self._calibration_scale[i]))
        except RocketLoggerCalibrationError:
            print('no calibration data available')
        print()
        try:
            self._check_errors_calculation_exists()
            print('Error calculation values:')
            print('  Channel  :  Offset [unit]  :  Scale [%]  :  RMSE [unit]')
            for i, name in enumerate(_CALIBRATION_CHANNEL_NAMES):
                print('  {:7s}  :  {:13.6g}  :  {:9.5f}  :  {:11.6g}'.format(
                    name, self._error_offset[i], 100 * self._error_scale[i],
                    self._error_rmse[i]))
        except RocketLoggerCalibrationError:
            print('no error calculation data available')

    def read_calibration_file(self, filename=ROCKETLOGGER_CALIBRATION_FILE):
        """
        Load an existing calibration file.

        :param filename: Name of the file to read the calibration values from
        """
        data = np.fromfile(filename, dtype=_CALIBRATION_FILE_DTYPE).squeeze()

        # file consistency check
        if not data:
            raise RocketLoggerFileError(
                'Invalid RocketLogger calibration file, unable to read data.')
        if data['file_magic'] != _CALIBRATION_FILE_MAGIC:
            raise RocketLoggerFileError('Invalid RocketLogger calibration '
                                        'file, invalid file magic {:x}.'
                                        .format(data['file_magic']))

        if data['file_version'] != _CALIBRATION_FILE_VERSION:
            raise RocketLoggerFileError(
                'Unsupported RocketLogger data file version {}.'
                .format(data['file_version']))

        if data['header_length'] != _CALIBRATION_FILE_HEADER_LENGTH:
            raise RocketLoggerFileError('Invalid RocketLogger calibration '
                                        'file, invalid header length {:x}.'
                                        .format(data['header_length']))

        self._calibration_time = data['calibration_time']
        self._calibration_offset = data['offset']
        self._calibration_scale = data['scale']

        if self._calibration_time > np.datetime64('now'):
            raise RocketLoggerCalibrationError(
                'Invalid calibration time in calibration file')
        if len(self._calibration_offset) != 8:
            raise RocketLoggerCalibrationError(
                'Invalid offset data in calibration file')
        if len(self._calibration_scale) != 8:
            raise RocketLoggerCalibrationError(
                'Invalid scale data in calibration file')

        # reset existing error calculations, keep measurement data if loaded
        self._error_offset = None
        self._error_scale = None
        self._error_rmse = None

    def write_calibration_file(self, filename='calibration.dat'):
        """
        Write the calibration to file.

        :param filename: Name of the file to write the calibration values to
        """
        self._check_calibration_exists()

        # assemble file data write to file
        filedata = np.array([(_CALIBRATION_FILE_MAGIC,
                              _CALIBRATION_FILE_VERSION,
                              _CALIBRATION_FILE_HEADER_LENGTH,
                              self._calibration_time,
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
        stdout = sys.stdout
        with open(filename, 'w') as sys.stdout:
            self.print_statistics()
        sys.stdout = stdout

    def _check_data_loaded(self):
        """Check for loaded calibration measurements, raise error if not."""
        if any([self._data_v is None, self._data_i1l is None,
                self._data_i1h is None, self._data_i2l is None,
                self._data_i2h is None]):
            raise RocketLoggerCalibrationError(
                'No data measurement data loaded for recalibration. '
                'Use load_measurement_data() first to load data.')

    def _check_calibration_exists(self):
        """Check for existing calibration data, raise error if not."""
        if any([self._calibration_time is None,
                self._calibration_offset is None,
                self._calibration_scale is None]):
            raise RocketLoggerCalibrationError(
                'No calibration data available. Perform recalibration first.')

    def _check_errors_calculation_exists(self):
        """Check for existing calibration data, raise error if not."""
        if any([self._error_offset is None, self._error_scale is None,
                self._error_rmse is None]):
            raise RocketLoggerCalibrationError(
                'No error calculation data available. '
                'Perform calculation first.')

    def __eq__(self, other):
        """Return True if other is the same calibration."""
        if isinstance(other, RocketLoggerCalibration):
            return all(
                [self._calibration_time == other._calibration_time,
                 np.array_equal(self._calibration_scale,
                                other._calibration_scale),
                 np.array_equal(self._calibration_offset,
                                other._calibration_offset)])
        return False
