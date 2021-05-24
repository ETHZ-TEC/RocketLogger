"""
RocketLogger Calibration Support.

Calibration file generation and accuracy verification.

Copyright (c) 2019-2020, ETH Zurich, Computer Engineering Group
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

ROCKETLOGGER_CALIBRATION_FILE = "/etc/rocketlogger/calibration.dat"
"""Default RocketLogger calibration file location."""

# hardware design values of voltage/current increment per ADC LSB
# - voltage: -1 * FS / 2^23 *10 / 3.9 * LSB
# - current low: FS / 2^23 / R_fb / 2 * LSB
# - current high: FS / 2^23 / R_sh / G_1 / 2 / G_2 * LSB
# where FS = 4.0, R_1 = 10k, R_2 = 3.9k, R_fb = 680R, R_sh = 0.05R, G_1 = 75, G_2 = 2
_ROCKETLOGGER_ADC_STEP_V = -1 * 4.0 / 2 ** 23 * 10 / 3.9
_ROCKETLOGGER_ADC_STEP_IL = 4.0 / 2 ** 23 / 680 / 2 / 2
_ROCKETLOGGER_ADC_STEP_IH = 4.0 / 2 ** 23 / 0.05 / 75 / 2 / 2

# calibration file format magic
_CALIBRATION_FILE_MAGIC = 0x434C5225

# calibration file format version
_CALIBRATION_FILE_VERSION = 0x02

# calibration file header length
_CALIBRATION_FILE_HEADER_LENGTH = 0x10

# data format channel name sequence
_CALIBRATION_CHANNEL_NAMES = ["V1", "V2", "V3", "V4", "I1L", "I1H", "I2L", "I2H", "DT"]

# data format of the calibration file
_CALIBRATION_FILE_DTYPE = np.dtype(
    [
        ("file_magic", "<u4"),
        ("file_version", "<u2"),
        ("header_length", "<u2"),
        ("calibration_time", "<datetime64[s]"),
        ("offset", ("<i4", len(_CALIBRATION_CHANNEL_NAMES))),
        ("scale", ("<f8", len(_CALIBRATION_CHANNEL_NAMES))),
    ]
)

# RocketLogger calibration file base units for channel scale
_CALIBRATION_CHANNEL_SCALE_BASE = [1e-8] * 4 + [1e-11, 1e-9] * 2

# RocketLogger calibration design values for channel scale
_CALIBRATION_CHANNEL_SCALE_DEFAULT = [_ROCKETLOGGER_ADC_STEP_V] * 4 + [
    _ROCKETLOGGER_ADC_STEP_IL,
    _ROCKETLOGGER_ADC_STEP_IH,
] * 2

# Calculated PRU cycle counter offset
# (2 MEM write + 1 MEM read + 2 ALU op readout)
# _CALIBRATION_PRU_CYCLES_OFFSET = 2 * 1 + 1 * 4 + 2 * 1
_CALIBRATION_PRU_CYCLES_OFFSET = 3 * 4  # FIXME: empirical value

# Calculated PRU cycle counter scale (200 MHz clock)
_CALIBRATION_PRU_CYCLES_SCALE = 5


def regression_linear(measurement, reference, zero_weight=1):
    """
    Perform linear regression with extra weight on zero values.

    :param reference: the reference value to calibrate for

    :param measurement: the measurement to calibrate on the reference

    :param zero_weight: the relative weight of the zero set-point value
        compared to the other set-points (default: 1, equal weight than all
        other set-points)

    :returns: (offset, scale) tuple of offset and scale values
    """

    weight = 1 + np.isclose(np.abs(reference), 0) * (zero_weight - 1)
    poly_coeffs = np.polyfit(measurement, reference, 1, w=weight)
    offset = round(poly_coeffs[1] / poly_coeffs[0])
    scale = poly_coeffs[0]

    return (offset, scale)


def _extract_setpoint_measurement(
    measurement_data, setpoint_step, filter_window_length=150
):
    """
    Extract aggregated set-points from a measurement trace.

    :param measurement_data: vector of measurement data to extract set-points
        from

    :param setpoint_step: the set-point step (in measurement units)

    :param filter_window_length: width of the square filter window used for
        set-point filtering in number of samples (default: 150 samples)

    :returns: vector of the extracted set-point measurements
    """

    # locate calibration sweep
    measurement_sweep_bound = np.flatnonzero(
        np.abs(np.diff(measurement_data)) > 2 * setpoint_step
    )
    measurement_sweep_start = np.min(measurement_sweep_bound)
    measurement_sweep_end = np.max(measurement_sweep_bound)

    # smooth raw measurements
    smoothing_window_length = filter_window_length // 100
    if smoothing_window_length > 0:
        smoothing_window = np.ones(smoothing_window_length) / smoothing_window_length
        measurement_smooth = np.convolve(
            measurement_data, smoothing_window, mode="same"
        )
    else:
        raise ValueError("filter window length needs to be strictly positive value")

    # find stable measurement intervals using rms of smoothed signal signals
    measurement_diff_abs = np.abs(np.diff(np.concatenate(([0], measurement_smooth))))
    measurement_diff_rms = np.sqrt(
        np.mean(measurement_diff_abs[measurement_diff_abs < setpoint_step] ** 2)
    )
    measurement_stable = measurement_diff_abs < 2 * measurement_diff_rms

    filter_window = np.ones(filter_window_length) / filter_window_length
    filter_threshold = (filter_window_length - 3 / 2) / filter_window_length
    measurement_filtered = (
        np.convolve(measurement_stable, filter_window, mode="same") > filter_threshold
    )

    # extract set-point measurement intervals
    setpoint_boundary = np.diff(np.concatenate(([0], measurement_filtered)))
    setpoint_start = np.flatnonzero(setpoint_boundary > 0) - filter_window_length // 2
    setpoint_end = np.flatnonzero(setpoint_boundary < 0) + filter_window_length // 2

    setpoint_trim = (
        setpoint_start > measurement_sweep_start - filter_window_length // 2
    ) & (setpoint_end < measurement_sweep_end + filter_window_length // 2)
    setpoint_intervals = zip(setpoint_start[setpoint_trim], setpoint_end[setpoint_trim])

    # if no set-point was found report error
    if not np.any(setpoint_trim):
        raise ValueError(
            "No set-points found. You might want to check the set-point step scale?"
        )

    # aggregate measurements by set-point interval
    setpoint_mean = np.array(
        [np.mean(measurement_data[start:end]) for start, end in setpoint_intervals]
    )
    return setpoint_mean


class RocketLoggerCalibrationError(Exception):
    """RocketLogger calibration related errors."""

    pass


class RocketLoggerCalibrationSetup:
    """
    RocketLogger calibration measurement setup helper class.

    :param setpoint_count: number of calibration set-points per channel

    :param setpoint_step_voltage: voltage step in volt between set-points

    :param setpoint_step_current_low: low current step in ampere between set-points

    :param setpoint_step_current_high: high current step in ampere between set-points

    :param setpoint_delay: minimum delay in seconds between switching the
        set-point

    :param dual_sweep: set True if dual sweep (up/down) is used
    """

    def __init__(
        self,
        setpoint_count,
        setpoint_step_voltage,
        setpoint_step_current_low,
        setpoint_step_current_high,
        setpoint_delay,
        dual_sweep,
    ):

        # check for valid sweep measurement points
        if not ((setpoint_count - 1) % 2) == 0:
            raise ValueError("Linear sweep expects odd number of measurement points")
        if dual_sweep and not ((setpoint_count - 1) % 4) == 0:
            raise ValueError("Dual sweep expects multiple of 4 + 1 measurement points")

        self._setpoint_count = setpoint_count
        self._step_voltage = setpoint_step_voltage
        self._step_current_low = setpoint_step_current_low
        self._step_current_high = setpoint_step_current_high
        self._delay = setpoint_delay
        self._dual_sweep = dual_sweep

    def get_voltage_setpoints(self, adc_units=False):
        """
        Get the real voltage set-points used in the measurement setup.

        :param adc_units: set `True` to get step in estimated ADC bits

        :returns: vector of the calibration set-points in volt or ADC bits
        """
        return self._get_setpoints(self.get_voltage_step(adc_units))

    def get_current_low_setpoints(self, adc_units=False):
        """
        Get the real low current set-points used in the measurement setup.

        :param adc_units: set `True` to get step in estimated ADC bits

        :returns: vector of the calibration set-points in ampere or ADC bits
        """
        return self._get_setpoints(self.get_current_low_step(adc_units))

    def get_current_high_setpoints(self, adc_units=False):
        """
        Get the real low current set-points used in the measurement setup.

        :param adc_units: set `True` to get step in estimated ADC bits

        :returns: vector of the calibration set-points in ampere or ADC bits
        """
        return self._get_setpoints(self.get_current_high_step(adc_units))

    def get_voltage_step(self, adc_units=False):
        """
        Get the absolute voltage step used in the measurement setup.

        :param adc_units: set `True` to get step in estimated ADC bits

        :returns: the absolute set-point step value in volt or ADC bits
        """
        if adc_units:
            return abs(self._step_voltage / _ROCKETLOGGER_ADC_STEP_V)
        return abs(self._step_voltage)

    def get_current_low_step(self, adc_units=False):
        """
        Get the absolute low current step used in the measurement setup.

        :param adc_units: set `True` to get step in estimated ADC bits

        :returns: the absolute set-point step value in ampere or ADC bits
        """
        if adc_units:
            return abs(self._step_current_low / _ROCKETLOGGER_ADC_STEP_IL)
        return abs(self._step_current_low)

    def get_current_high_step(self, adc_units=False):
        """
        Get the absolute low current step used in the measurement setup.

        :param adc_units: set `True` to get step in estimated ADC bits

        :returns: the absolute set-point step value in ampere or ADC bits
        """
        if adc_units:
            return abs(self._step_current_high / _ROCKETLOGGER_ADC_STEP_IH)
        return abs(self._step_current_high)

    def get_delay(self):
        """
        Get the minimal delay between changing a set-point.

        :returns: the minimal delay in seconds
        """
        return self._delay

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
        setpoints = np.arange(-max_value, max_value + setpoint_step, setpoint_step)

        # add down sweep for dual sweep setups
        if self._dual_sweep:
            setpoints = np.concatenate((setpoints, setpoints[-2::-1]))
        return setpoints


CALIBRATION_SETUP_SMU2450 = RocketLoggerCalibrationSetup(
    setpoint_count=201,
    setpoint_step_voltage=100e-3,
    setpoint_step_current_low=20e-6,
    setpoint_step_current_high=2e-3,
    setpoint_delay=250e-3,
    dual_sweep=True,
)
"""Reference calibration setup using the Keithley 2450 SMU setup."""

CALIBRATION_SETUP_BASIC = RocketLoggerCalibrationSetup(
    setpoint_count=3,
    setpoint_step_voltage=5.0,
    setpoint_step_current_low=2e-3,
    setpoint_step_current_high=0.5,
    setpoint_delay=250e-3,
    dual_sweep=False,
)
"""Basic calibration setup with 3 point measurements (min, zero, max)."""


class RocketLoggerCalibration:
    """
    RocketLogger calibration support class. Provides the calibration
    measurement data processing to generate calibration files.

    The alternatives to initialize the calibrations class are:

    - Without parameter, a calibration class instance without initialized
        data is created
    - With 1 parameter, a file name, it serves as shortcut for
        :func:`read_calibration_file` to load an existing calibration file.
    - With 5 parameters it serves as shortcut for
        :func:`load_measurement_data` to load calibration measurement data.
    """

    def __init__(self, *args):
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

    def load_design_data(self):
        """
        Load calibration values derived from the design of the RocketLogger.
        """

        # store design calibration values
        _CALIBRATION_CHANNEL_SCALE_DEFAULT = [_ROCKETLOGGER_ADC_STEP_V] * 4 + [
            _ROCKETLOGGER_ADC_STEP_IL,
            _ROCKETLOGGER_ADC_STEP_IH,
        ] * 2
        design_offset = [0] * len(_CALIBRATION_CHANNEL_SCALE_DEFAULT)

        self._calibration_time = np.datetime64(0, "s")
        self._calibration_offset = np.array(design_offset, dtype="<i4")
        self._calibration_scale = np.array(
            [
                scale / scale_base
                for scale, scale_base in zip(
                    _CALIBRATION_CHANNEL_SCALE_DEFAULT, _CALIBRATION_CHANNEL_SCALE_BASE
                )
            ],
            dtype="<f8",
        )

        # append PRU timestamp constant calibration values
        self._calibration_offset = np.concatenate(
            (self._calibration_offset, [_CALIBRATION_PRU_CYCLES_OFFSET])
        )
        self._calibration_scale = np.concatenate(
            (self._calibration_scale, [_CALIBRATION_PRU_CYCLES_SCALE])
        )

        # reset existing error calculations, keep measurement data if loaded
        self._error_offset = None
        self._error_scale = None
        self._error_rmse = None

    def load_measurement_data(self, data_v, data_i1l, data_i1h, data_i2l, data_i2h):
        """
        Load calibration measurement data from RocketLoggerData structures or
        RocketLogger data files. Loading new measurement data invalidates
        previously made calibration.

        :param data_v: Voltage V1-V4 calibration measurement data or filename

        :param data_i1l: Current I1L calibration measurement data or filename

        :param data_i1h: Current I1H calibration measurement data or filename

        :param data_i2l: Current I2L calibration measurement data or filename

        :param data_i2h: Current I2H calibration measurement data or filename
        """

        # data load helper function
        def _rld_copy_or_load(data):
            if type(data) is RocketLoggerData:
                return data
            elif os.path.isfile(data):
                return RocketLoggerData(data)
            raise ValueError(
                f"'{data}' is not valid measurement data nor an existing data file."
            )

        # store the loaded data in class
        self._data_v = _rld_copy_or_load(data_v)
        self._data_i1l = _rld_copy_or_load(data_i1l)
        self._data_i1h = _rld_copy_or_load(data_i1h)
        self._data_i2l = _rld_copy_or_load(data_i2l)
        self._data_i2h = _rld_copy_or_load(data_i2h)

        # validate consistency of data
        self._assert_measurement_consistency()

        # reset existing error calculations, keep calibration values if loaded
        self._error_offset = None
        self._error_scale = None
        self._error_rmse = None

    def recalibrate(
        self,
        setup,
        fix_signs=True,
        target_offset_error=1,
        regression_algorithm=regression_linear,
        **kwargs,
    ):
        """
        Perform channel calibration using loaded measurement data.
        Overwrites any loaded calibration file parameters.

        :param setup: Calibration setup used for the measurements using the
            RocketLoggerCalibrationSetup helper class to describe

        :param fix_signs: Set True to automatically fix sign error in
            calibration scales

        :param target_offset_error: Factor in [1, inf) specifying the multiple
            of the zero error to use as offset error for the error calculations

        :param regression_algorithm: Algorithm to use for the regression,
            taking the set-point measurement and reference values as arguments
            and providing the resulting offset and scale as output

        :param kwargs: Optional names arguments passed to the regression
            algorithm function
        """
        self._assert_data_loaded()
        if target_offset_error < 1:
            raise ValueError("target_offset_error factor needs to be >= 1.")

        # forced reference information
        v_ref = setup.get_voltage_setpoints()
        ih_ref = setup.get_current_high_setpoints()
        il_ref = setup.get_current_low_setpoints()
        reference = [v_ref] * 4 + [il_ref, ih_ref] * 2

        v_step = setup.get_voltage_step(adc_units=True)
        ih_step = setup.get_current_high_step(adc_units=True)
        il_step = setup.get_current_low_step(adc_units=True)
        reference_steps = [v_step] * 4 + [il_step, ih_step] * 2

        # extract measurement information
        measurement_data = [
            self._data_v,
            self._data_v,
            self._data_v,
            self._data_v,
            self._data_i1l,
            self._data_i1h,
            self._data_i2l,
            self._data_i2h,
        ]
        measurement_traces = [
            data.get_data(channel).squeeze()
            for data, channel in zip(measurement_data, _CALIBRATION_CHANNEL_NAMES)
        ]
        timestamp = np.datetime64(
            min([x.get_header()["start_time"] for x in measurement_data])
        )

        # filter window length: 150 ms (=250-100 ms),
        sample_rate = self._data_v.get_header()["sample_rate"]
        setpoint_filter_window = int(0.75 * sample_rate * setup.get_delay())
        measurement = [
            _extract_setpoint_measurement(
                t, s, filter_window_length=setpoint_filter_window
            )
            for t, s in zip(measurement_traces, reference_steps)
        ]

        # perform regression
        regression_result = [
            regression_algorithm(x, y) for x, y in zip(measurement, reference)
        ]
        offsets, scales = zip(*regression_result)

        # calculate residuals and errors
        residual = [
            (x + offset) * scale - y
            for x, y, offset, scale in zip(measurement, reference, offsets, scales)
        ]
        rmse_errors = [np.sqrt(np.dot(res, res)) for res in residual]
        offset_errors = [
            np.max(np.abs(res[np.isclose(ref, 0)])) * target_offset_error
            for res, ref in zip(residual, reference)
        ]
        scale_errors = [
            np.abs(
                np.fmin(np.abs(res - offset_error), np.abs(res + offset_error)) / ref
            )
            for res, ref, offset_error in zip(residual, reference, offset_errors)
        ]

        # store data
        self._calibration_offset = np.array(offsets, dtype="<i4")
        self._calibration_scale = np.array(
            [
                scale / scale_base
                for scale, scale_base in zip(scales, _CALIBRATION_CHANNEL_SCALE_BASE)
            ],
            dtype="<f8",
        )
        self._calibration_time = np.array(timestamp, dtype="<datetime64[s]")
        self._error_offset = np.array(offset_errors)
        self._error_scale = np.array(
            [
                np.max(scale_error[np.abs(ref) > 0.1 * np.abs(np.max(ref))])
                for scale_error, ref in zip(scale_errors, reference)
            ]
        )
        self._error_rmse = np.array(rmse_errors)

        # check and invert scales where necessary
        if fix_signs:
            for ch, scale in enumerate(_CALIBRATION_CHANNEL_SCALE_DEFAULT):
                # XOR signs of design and calculated scale values
                if (scale < 0) != (self._calibration_scale[ch] < 0):
                    self._calibration_scale[ch] = -self._calibration_scale[ch]

        # append PRU timestamp constant calibration values
        self._calibration_offset = np.concatenate(
            (self._calibration_offset, [_CALIBRATION_PRU_CYCLES_OFFSET])
        )
        self._calibration_scale = np.concatenate(
            (self._calibration_scale, [_CALIBRATION_PRU_CYCLES_SCALE])
        )
        # error statistics do not exist for PRU timestamp (design values)
        self._error_offset = np.concatenate((self._error_offset, [np.NaN]))
        self._error_scale = np.concatenate((self._error_scale, [np.NaN]))
        self._error_rmse = np.concatenate((self._error_rmse, [np.NaN]))

    def print_statistics(self):
        """
        Print statistics of the calibration.
        """
        print("RocketLogger Calibration Statistics")
        try:
            self._assert_calibration_exists()
            print(f"Measurement time:   {self._calibration_time}")
            print()
            print("Calibration values:")
            print("  Channel  :  Offset [bit]  :  Scale [unit/bit]")
            for i, name in enumerate(_CALIBRATION_CHANNEL_NAMES):
                print(
                    f"  {name:7s}  :  {self._calibration_offset[i]:12d}  :"
                    f"  {self._calibration_scale[i]:16.6f}"
                )
        except RocketLoggerCalibrationError:
            print("no calibration data available")
        print()
        try:
            self._assert_error_calculation_exists()
            print("Error calculation values:")
            print("  Channel  :  Offset [unit]  :  Scale [%]  :  RMSE [unit]")
            for i, name in enumerate(_CALIBRATION_CHANNEL_NAMES):
                print(
                    f"  {name:7s}  :  {self._error_offset[i]:13.6g}  :  "
                    f"{100 * self._error_scale[i]:9.5f}  :  {self._error_rmse[i]:11.6g}"
                )
        except RocketLoggerCalibrationError:
            print("no error calculation data available")

    def read_calibration_file(self, filename=ROCKETLOGGER_CALIBRATION_FILE):
        """
        Load an existing calibration file.

        :param filename: Name of the file to read the calibration values from
        """
        data = np.fromfile(filename, dtype=_CALIBRATION_FILE_DTYPE).squeeze()

        # file consistency check
        if data.size == 0:
            raise RocketLoggerFileError(
                "Invalid RocketLogger calibration file, unable to read data."
            )
        if data["file_magic"] != _CALIBRATION_FILE_MAGIC:
            raise RocketLoggerFileError(
                f"Invalid RocketLogger calibration file magic 0x{int(data['file_magic']):08x}."
            )

        if data["file_version"] != _CALIBRATION_FILE_VERSION:
            raise RocketLoggerFileError(
                f"Unsupported RocketLogger calibration file version {data['file_version']}."
            )

        if data["header_length"] != _CALIBRATION_FILE_HEADER_LENGTH:
            raise RocketLoggerFileError(
                "Invalid RocketLogger calibration file, invalid header length "
                f"0x{int(data['file_magic']):08x}."
            )

        self._calibration_time = data["calibration_time"]
        self._calibration_offset = data["offset"]
        self._calibration_scale = data["scale"]

        if self._calibration_time > np.datetime64("now"):
            raise RocketLoggerCalibrationError(
                "Invalid calibration time in calibration file"
            )
        if len(self._calibration_offset) != len(_CALIBRATION_CHANNEL_NAMES):
            raise RocketLoggerCalibrationError(
                "Invalid offset data in calibration file"
            )
        if len(self._calibration_scale) != len(_CALIBRATION_CHANNEL_NAMES):
            raise RocketLoggerCalibrationError("Invalid scale data in calibration file")

        # reset existing error calculations, keep measurement data if loaded
        self._error_offset = None
        self._error_scale = None
        self._error_rmse = None

    def write_calibration_file(self, filename="calibration.dat"):
        """
        Write the calibration to file.

        :param filename: Name of the file to write the calibration values to
        """
        self._assert_calibration_exists()

        # assemble file data write to file
        filedata = np.array(
            [
                (
                    _CALIBRATION_FILE_MAGIC,
                    _CALIBRATION_FILE_VERSION,
                    _CALIBRATION_FILE_HEADER_LENGTH,
                    self._calibration_time,
                    self._calibration_offset,
                    self._calibration_scale,
                )
            ],
            dtype=_CALIBRATION_FILE_DTYPE,
        )
        filedata.tofile(filename)

    def write_log_file(self, filename="calibration.log"):
        """
        Write log of performed recalibration to file. Raises a calibration
        error if no recalibration has been performed beforehand.

        :param filename: Name of the file to write the calibration log to
        """
        # redirect statistics output to log file
        stdout = sys.stdout
        with open(filename, "w") as sys.stdout:
            self.print_statistics()
        sys.stdout = stdout

    def _assert_calibration_exists(self):
        """
        Check for existing calibration data, raise error if not.
        """
        if any(
            [
                self._calibration_time is None,
                self._calibration_offset is None,
                self._calibration_scale is None,
            ]
        ):
            raise RocketLoggerCalibrationError(
                "No calibration data available. Perform recalibration first."
            )

    def _assert_data_loaded(self):
        """
        Check for loaded calibration measurements, raise error if not.
        """
        if any(
            [
                self._data_v is None,
                self._data_i1l is None,
                self._data_i1h is None,
                self._data_i2l is None,
                self._data_i2h is None,
            ]
        ):
            raise RocketLoggerCalibrationError(
                "No measurement data loaded for recalibration. "
                "Use load_measurement_data() first to load data."
            )

    def _assert_error_calculation_exists(self):
        """
        Check for existing calibration data, raise error if not.
        """
        if any(
            [
                self._error_offset is None,
                self._error_scale is None,
                self._error_rmse is None,
            ]
        ):
            raise RocketLoggerCalibrationError(
                "No error calculation data available. Perform calculation first."
            )

    def _assert_measurement_consistency(self):
        """
        Validate consistency of loaded measurement data, raise error if not.
        """
        data = [
            self._data_v,
            self._data_i1l,
            self._data_i1h,
            self._data_i2l,
            self._data_i2h,
        ]
        sample_rates = [d.get_header()["sample_rate"] for d in data]
        if not all(x == sample_rates[0] for x in sample_rates):
            raise RocketLoggerCalibrationError(
                "inconsistent sample rate across measurements!"
            )

    def __eq__(self, other):
        """
        Compare calibration data.

        :returns: `True` if other is the same calibration.
        """
        if isinstance(other, RocketLoggerCalibration):
            return all(
                [
                    self._calibration_time == other._calibration_time,
                    np.array_equal(self._calibration_scale, other._calibration_scale),
                    np.array_equal(self._calibration_offset, other._calibration_offset),
                ]
            )
        return False
