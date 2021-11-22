"""
RocketLogger calibration tests.

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

import os.path
from unittest import TestCase

import numpy as np

from rocketlogger.data import RocketLoggerData, RocketLoggerFileError
from rocketlogger.calibration import (
    RocketLoggerCalibration,
    RocketLoggerCalibrationSetup,
    RocketLoggerCalibrationError,
    CALIBRATION_SETUP_SMU2450,
    CALIBRATION_SETUP_BASIC,
    _extract_setpoint_measurement,
    _CALIBRATION_PRU_CYCLES_OFFSET,
    _CALIBRATION_PRU_CYCLES_SCALE,
)
from tests.test_data import _file_copy_byte_flipped


_TEST_FILE_DIR = "data"
_CALIBRATION_FILE = os.path.join(_TEST_FILE_DIR, "test_calibration_v2.dat")
_CALIBRATION_FILE_V1 = os.path.join(_TEST_FILE_DIR, "test_calibration_v1.dat")
_CALIBRATION_VOLTAGE_FILE = os.path.join(_TEST_FILE_DIR, "test_calibration_v.rld")
_CALIBRATION_CURRENT_LO1_FILE = os.path.join(_TEST_FILE_DIR, "test_calibration_i1l.rld")
_CALIBRATION_CURRENT_LO2_FILE = os.path.join(_TEST_FILE_DIR, "test_calibration_i2l.rld")
_CALIBRATION_CURRENT_HI_FILE = os.path.join(_TEST_FILE_DIR, "test_calibration_ih.rld")
_TEMP_FILE = os.path.join(_TEST_FILE_DIR, "temp_calibration.dat")


class TestCalibrationFile(TestCase):
    def test_file_read(self):
        cal = RocketLoggerCalibration()
        cal.read_calibration_file(_CALIBRATION_FILE)
        reference_time = np.datetime64("2019-01-01T00:00:00", "s")
        self.assertEqual(cal._calibration_time, reference_time)

    def test_file_read_direct(self):
        cal = RocketLoggerCalibration(_CALIBRATION_FILE)
        reference_time = np.datetime64("2019-01-01T00:00:00", "s")
        self.assertEqual(cal._calibration_time, reference_time)

    def test_compare_empty_empty(self):
        cal1 = RocketLoggerCalibration()
        cal2 = RocketLoggerCalibration()
        self.assertEqual(cal1, cal2)

    def test_compare_empty_design(self):
        cal1 = RocketLoggerCalibration()
        cal2 = RocketLoggerCalibration()
        cal2.load_design_data()
        self.assertNotEqual(cal1, cal2)

    def test_compare_empty_read(self):
        cal1 = RocketLoggerCalibration()
        cal2 = RocketLoggerCalibration(_CALIBRATION_FILE)
        self.assertNotEqual(cal1, cal2)

    def test_compare_empty_different_type(self):
        cal = RocketLoggerCalibration()
        self.assertNotEqual("cal", cal)

    def test_compare_design_read(self):
        cal1 = RocketLoggerCalibration()
        cal1.load_design_data()
        cal2 = RocketLoggerCalibration(_CALIBRATION_FILE)
        self.assertNotEqual(cal1, cal2)

    def test_compare_read(self):
        cal1 = RocketLoggerCalibration()
        cal2 = RocketLoggerCalibration()
        cal1.read_calibration_file(_CALIBRATION_FILE)
        cal2.read_calibration_file(_CALIBRATION_FILE)
        self.assertEqual(cal1, cal2)

    def test_compare_read_direct(self):
        cal1 = RocketLoggerCalibration(_CALIBRATION_FILE)
        cal2 = RocketLoggerCalibration(_CALIBRATION_FILE)
        self.assertEqual(cal1, cal2)

    def test_compare_read_to_empty(self):
        cal1 = RocketLoggerCalibration(_CALIBRATION_FILE)
        cal2 = RocketLoggerCalibration()
        self.assertNotEqual(cal1, cal2)

    def test_file_write_empty(self):
        cal = RocketLoggerCalibration()
        with self.assertRaisesRegex(
            RocketLoggerCalibrationError, "No calibration data available"
        ):
            cal.write_calibration_file(_TEMP_FILE)

    def test_invalid_header_magic(self):
        _file_copy_byte_flipped(_CALIBRATION_FILE, _TEMP_FILE, 0x00)
        with self.assertRaisesRegex(RocketLoggerFileError, "file magic"):
            RocketLoggerCalibration(_TEMP_FILE)

    def test_invalid_header_version(self):
        _file_copy_byte_flipped(_CALIBRATION_FILE, _TEMP_FILE, 0x04)
        with self.assertRaisesRegex(RocketLoggerFileError, "calibration file version"):
            RocketLoggerCalibration(_TEMP_FILE)

    def test_invalid_header_length(self):
        _file_copy_byte_flipped(_CALIBRATION_FILE, _TEMP_FILE, 0x06)
        with self.assertRaisesRegex(RocketLoggerFileError, "invalid header length"):
            RocketLoggerCalibration(_TEMP_FILE)

    def test_file_write_reread(self):
        cal = RocketLoggerCalibration(_CALIBRATION_FILE)
        cal.write_calibration_file(_TEMP_FILE)
        cal_reread = RocketLoggerCalibration(_TEMP_FILE)
        self.assertEqual(cal, cal_reread)

    def test_file_write_reread_design(self):
        cal = RocketLoggerCalibration()
        cal.load_design_data()
        cal.write_calibration_file(_TEMP_FILE)
        cal_reread = RocketLoggerCalibration(_TEMP_FILE)
        self.assertEqual(cal, cal_reread)

    @classmethod
    def tearDownClass(cls):
        try:
            os.remove(_TEMP_FILE)
        except FileNotFoundError:
            pass


class TestUnsupportedCalibrationFile(TestCase):
    def test_file_read(self):
        cal = RocketLoggerCalibration()
        with self.assertRaisesRegex(RocketLoggerFileError, "unable to read data"):
            cal.read_calibration_file(_CALIBRATION_FILE_V1)

    def test_file_read_direct(self):
        with self.assertRaisesRegex(RocketLoggerFileError, "unable to read data"):
            RocketLoggerCalibration(_CALIBRATION_FILE_V1)


class TestCalibrationSetup(TestCase):
    def test_calibration_setup_empty(self):
        with self.assertRaisesRegex(TypeError, "required positional argument"):
            RocketLoggerCalibrationSetup()

    def test_calibration_setup_single(self):
        RocketLoggerCalibrationSetup(3, 1, 1, 1, 0.25, False)

    def test_calibration_setup_single_invalid(self):
        with self.assertRaisesRegex(ValueError, "sweep expects odd number"):
            RocketLoggerCalibrationSetup(2, 1, 1, 1, 0.25, True)

    def test_calibration_setup_dual(self):
        RocketLoggerCalibrationSetup(5, 1, 1, 1, 0.25, True)

    def test_calibration_setup_dual_invalid(self):
        with self.assertRaisesRegex(ValueError, "sweep expects multiple of 4"):
            RocketLoggerCalibrationSetup(3, 1, 1, 1, 0.25, True)

    def test_SMU2450_setup_setpoint_count(self):
        self.assertEqual(CALIBRATION_SETUP_SMU2450.get_setpoint_count(), 201)

    def test_SMU2450_setup_voltage_setpoints(self):
        setpoints = CALIBRATION_SETUP_SMU2450.get_voltage_setpoints()
        setpoints_reference = np.concatenate(
            (np.arange(-5, 5.1, 0.1), np.arange(4.9, -5.1, -0.1))
        )
        self.assertTrue(np.allclose(setpoints, setpoints_reference))

    def test_SMU2450_setup_current_low_setpoints(self):
        setpoints = CALIBRATION_SETUP_SMU2450.get_current_low_setpoints()
        setpoints_reference = np.concatenate(
            (np.arange(-1e-3, 1.02e-3, 20e-6), np.arange(0.98e-3, -1.02e-3, -20e-6))
        )
        self.assertTrue(np.allclose(setpoints, setpoints_reference))

    def test_SMU2450_setup_current_high_setpoints(self):
        setpoints = CALIBRATION_SETUP_SMU2450.get_current_high_setpoints()
        setpoints_reference = np.concatenate(
            (np.arange(-0.1, 0.102, 2e-3), np.arange(0.098, -0.102, -2e-3))
        )
        self.assertTrue(np.allclose(setpoints, setpoints_reference))

    def test_basic_setup_setpoint_count(self):
        self.assertEqual(CALIBRATION_SETUP_BASIC.get_setpoint_count(), 3)

    def test_basic_setup_voltage_setpoints(self):
        setpoints = CALIBRATION_SETUP_BASIC.get_voltage_setpoints()
        setpoints_reference = np.arange(-5, 5.1, 5)
        self.assertTrue(np.allclose(setpoints, setpoints_reference))

    def test_basic_setup_current_low_setpoints(self):
        setpoints = CALIBRATION_SETUP_BASIC.get_current_low_setpoints()
        setpoints_reference = np.arange(-2e-3, 2.02e-3, 2e-3)
        self.assertTrue(np.allclose(setpoints, setpoints_reference))

    def test_basic_setup_current_high_setpoints(self):
        setpoints = CALIBRATION_SETUP_BASIC.get_current_high_setpoints()
        setpoints_reference = np.arange(-0.5, 0.502, 0.5)
        self.assertTrue(np.allclose(setpoints, setpoints_reference))

    def test_setpoint_detection_size(self):
        data_measure = (
            RocketLoggerData(_CALIBRATION_VOLTAGE_FILE).get_data("V1").squeeze()
        )
        setpoint = _extract_setpoint_measurement(
            data_measure, CALIBRATION_SETUP_SMU2450.get_voltage_step(adc_units=True)
        )
        self.assertEqual(len(setpoint), CALIBRATION_SETUP_SMU2450.get_setpoint_count())

    def test_setpoint_detection_invalid_scale(self):
        data_measure = (
            RocketLoggerData(_CALIBRATION_VOLTAGE_FILE).get_data("V1").squeeze()
        )
        with self.assertRaisesRegex(ValueError, "No set-points found"):
            _extract_setpoint_measurement(
                data_measure, CALIBRATION_SETUP_SMU2450.get_voltage_step()
            )

    def test_setpoint_detection_zero_length_filter(self):
        data_measure = (
            RocketLoggerData(_CALIBRATION_VOLTAGE_FILE).get_data("V1").squeeze()
        )
        with self.assertRaisesRegex(ValueError, "filter window length"):
            setpoint = _extract_setpoint_measurement(
                data_measure,
                CALIBRATION_SETUP_SMU2450.get_voltage_step(adc_units=True),
                filter_window_length=0,
            )


class TestCalibrationProcedure(TestCase):
    def test_calibration_empty(self):
        cal = RocketLoggerCalibration()
        with self.assertRaisesRegex(
            RocketLoggerCalibrationError, "No measurement data loaded"
        ):
            cal.recalibrate(CALIBRATION_SETUP_SMU2450)

    def test_calibration_by_filename(self):
        cal = RocketLoggerCalibration()
        cal.load_measurement_data(
            _CALIBRATION_VOLTAGE_FILE,
            _CALIBRATION_CURRENT_LO1_FILE,
            _CALIBRATION_CURRENT_HI_FILE,
            _CALIBRATION_CURRENT_LO2_FILE,
            _CALIBRATION_CURRENT_HI_FILE,
        )
        cal.recalibrate(CALIBRATION_SETUP_SMU2450)
        self._check_reference_calibration(cal)

    def test_calibration_by_filename_direct(self):
        cal = RocketLoggerCalibration(
            _CALIBRATION_VOLTAGE_FILE,
            _CALIBRATION_CURRENT_LO1_FILE,
            _CALIBRATION_CURRENT_HI_FILE,
            _CALIBRATION_CURRENT_LO2_FILE,
            _CALIBRATION_CURRENT_HI_FILE,
        )
        cal.recalibrate(CALIBRATION_SETUP_SMU2450)
        self._check_reference_calibration(cal)

    def test_calibration_by_data(self):
        data_v = RocketLoggerData(_CALIBRATION_VOLTAGE_FILE)
        data_i1l = RocketLoggerData(_CALIBRATION_CURRENT_LO1_FILE)
        data_i2l = RocketLoggerData(_CALIBRATION_CURRENT_LO2_FILE)
        data_ih = RocketLoggerData(_CALIBRATION_CURRENT_HI_FILE)
        cal = RocketLoggerCalibration()
        cal.load_measurement_data(data_v, data_i1l, data_ih, data_i2l, data_ih)
        cal.recalibrate(CALIBRATION_SETUP_SMU2450)
        self._check_reference_calibration(cal)

    def test_calibration_by_data_direct(self):
        data_v = RocketLoggerData(_CALIBRATION_VOLTAGE_FILE)
        data_i1l = RocketLoggerData(_CALIBRATION_CURRENT_LO1_FILE)
        data_i2l = RocketLoggerData(_CALIBRATION_CURRENT_LO2_FILE)
        data_ih = RocketLoggerData(_CALIBRATION_CURRENT_HI_FILE)
        cal = RocketLoggerCalibration(data_v, data_i1l, data_ih, data_i2l, data_ih)
        cal.recalibrate(CALIBRATION_SETUP_SMU2450)
        self._check_reference_calibration(cal)

    def test_calibration_file_missing(self):
        with self.assertRaisesRegex(ValueError, "not valid measurement data"):
            RocketLoggerCalibration(
                _TEMP_FILE, _TEMP_FILE, _TEMP_FILE, _TEMP_FILE, _TEMP_FILE
            )

    def test_calibration_file_channel_missing(self):
        cal = RocketLoggerCalibration(
            _CALIBRATION_VOLTAGE_FILE,
            _CALIBRATION_VOLTAGE_FILE,
            _CALIBRATION_VOLTAGE_FILE,
            _CALIBRATION_VOLTAGE_FILE,
            _CALIBRATION_VOLTAGE_FILE,
        )
        with self.assertRaisesRegex(KeyError, "Channel .* not found"):
            cal.recalibrate(CALIBRATION_SETUP_SMU2450)

    def test_calibration_invalid_target_offset_error(self):
        cal = RocketLoggerCalibration()
        cal.load_measurement_data(
            _CALIBRATION_VOLTAGE_FILE,
            _CALIBRATION_CURRENT_LO1_FILE,
            _CALIBRATION_CURRENT_HI_FILE,
            _CALIBRATION_CURRENT_LO2_FILE,
            _CALIBRATION_CURRENT_HI_FILE,
        )
        with self.assertRaisesRegex(ValueError, "target_offset_error"):
            cal.recalibrate(CALIBRATION_SETUP_SMU2450, target_offset_error=0)

    def test_calibration_print_stat(self):
        cal = RocketLoggerCalibration(
            _CALIBRATION_VOLTAGE_FILE,
            _CALIBRATION_CURRENT_LO1_FILE,
            _CALIBRATION_CURRENT_HI_FILE,
            _CALIBRATION_CURRENT_LO2_FILE,
            _CALIBRATION_CURRENT_HI_FILE,
        )
        cal.recalibrate(CALIBRATION_SETUP_SMU2450)
        cal.print_statistics()

    def test_calibration_print_stat_uncalibrated(self):
        cal = RocketLoggerCalibration(
            _CALIBRATION_VOLTAGE_FILE,
            _CALIBRATION_CURRENT_LO1_FILE,
            _CALIBRATION_CURRENT_HI_FILE,
            _CALIBRATION_CURRENT_LO2_FILE,
            _CALIBRATION_CURRENT_HI_FILE,
        )
        cal.print_statistics()

    def test_calibration_write_log(self):
        cal = RocketLoggerCalibration(
            _CALIBRATION_VOLTAGE_FILE,
            _CALIBRATION_CURRENT_LO1_FILE,
            _CALIBRATION_CURRENT_HI_FILE,
            _CALIBRATION_CURRENT_LO2_FILE,
            _CALIBRATION_CURRENT_HI_FILE,
        )
        cal.recalibrate(CALIBRATION_SETUP_SMU2450)
        cal.write_log_file(_TEMP_FILE)

    def test_calibration_write_reread(self):
        cal = RocketLoggerCalibration(
            _CALIBRATION_VOLTAGE_FILE,
            _CALIBRATION_CURRENT_LO1_FILE,
            _CALIBRATION_CURRENT_HI_FILE,
            _CALIBRATION_CURRENT_LO2_FILE,
            _CALIBRATION_CURRENT_HI_FILE,
        )
        cal.recalibrate(CALIBRATION_SETUP_SMU2450)
        cal.write_calibration_file(_TEMP_FILE)
        cal_reread = RocketLoggerCalibration(_TEMP_FILE)
        self.assertEqual(cal, cal_reread)
        self._check_reference_calibration(cal_reread)

    @classmethod
    def tearDownClass(cls):
        try:
            os.remove(_TEMP_FILE)
        except FileNotFoundError:
            pass

    def _check_reference_calibration(self, calibration):
        reference_time = np.datetime64("2017-05-09T07:37:49", "s")
        reference_offset = [
            1075,
            856,
            962,
            906,
            1778,
            7980,
            1988,
            -3654,
            _CALIBRATION_PRU_CYCLES_OFFSET,
        ]
        reference_scale = [
            -122.41154850110402,
            -122.08777736753133,
            -122.22427428168358,
            -122.24473837544376,
            17.536330902313154,
            31.554214628481056,
            17.503108506633023,
            31.56472079317537,
            _CALIBRATION_PRU_CYCLES_SCALE,
        ]
        self.assertEqual(calibration._calibration_time, reference_time)
        self.assertListEqual(list(calibration._calibration_offset), reference_offset)
        self.assertTrue(np.allclose(calibration._calibration_scale, reference_scale))
