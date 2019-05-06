"""
RocketLogger calibration tests.

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

import os.path
from unittest import TestCase

import numpy as np
import rocketlogger.calibration as rlc

from rocketlogger.data import RocketLoggerData
from rocketlogger.calibration import RocketLoggerCalibration, \
    RocketLoggerCalibrationSetup, RocketLoggerCalibrationError, \
    CALIBRATION_SETUP_SMU2450, CALIBRATION_SETUP_BASIC


_TEST_FILE_DIR = 'data'
_CALIBRATION_FILE = os.path.join(_TEST_FILE_DIR, 'test_calibration_sample.dat')
_VOLTAGE_FILE = os.path.join(_TEST_FILE_DIR, 'test_calibration_v.rld')
_CURRENT_LO1_FILE = os.path.join(_TEST_FILE_DIR, 'test_calibration_i1l.rld')
_CURRENT_LO2_FILE = os.path.join(_TEST_FILE_DIR, 'test_calibration_i2l.rld')
_CURRENT_HI_FILE = os.path.join(_TEST_FILE_DIR, 'test_calibration_ih.rld')
_TEMP_FILE = os.path.join(_TEST_FILE_DIR, 'temp_calibration.dat')


class TestCalibrationFile(TestCase):

    def test_file_read(self):
        cal = RocketLoggerCalibration()
        cal.read_calibration_file(_CALIBRATION_FILE)
        reference_time = np.datetime64('2019-01-01T00:00:00', dtype='M8[s]')
        self.assertEqual(cal._calibration_timestamp, reference_time)

    def test_file_read_direct(self):
        cal = RocketLoggerCalibration(_CALIBRATION_FILE)
        reference_time = np.datetime64('2019-01-01T00:00:00', dtype='M8[s]')
        self.assertEqual(cal._calibration_timestamp, reference_time)

    def test_compare_empty(self):
        cal1 = RocketLoggerCalibration()
        cal2 = RocketLoggerCalibration()
        self.assertEqual(cal1, cal2)

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
        with self.assertRaises(RocketLoggerCalibrationError):
            cal.write_calibration_file(_TEMP_FILE)

    def test_file_write_reread(self):
        cal = RocketLoggerCalibration(_CALIBRATION_FILE)
        cal.write_calibration_file(_TEMP_FILE)
        cal_reread = RocketLoggerCalibration(_TEMP_FILE)
        self.assertEqual(cal, cal_reread)

    @classmethod
    def tearDownClass(cls):
        try:
            os.remove(_TEMP_FILE)
        except FileNotFoundError:
            pass


class TestCalibrationSetup(TestCase):

    def test_calibration_setup_empty(self):
        with self.assertRaises(TypeError):
            RocketLoggerCalibrationSetup()

    def test_SMU2540_setup_setpoint_count(self):
        self.assertEqual(CALIBRATION_SETUP_SMU2450.get_setpoint_count(), 201)

    def test_SMU2540_setup_voltage_setpoints(self):
        setpoints = CALIBRATION_SETUP_SMU2450.get_voltage_setpoints()
        setpoints_reference = np.hstack([np.arange(-5, 5.1, 0.1),
                                         np.arange(4.9, -5.1, -0.1)])
        print(setpoints)
        print(setpoints_reference)
        self.assertEqual(np.allclose(setpoints, setpoints_reference), True)

    def test_SMU2540_setup_current_low_setpoints(self):
        setpoints = CALIBRATION_SETUP_SMU2450.get_current_low_setpoints()
        setpoints_reference = np.hstack([np.arange(-1e-3, 1.02e-3, 20e-6),
                                         np.arange(0.98e-3, -1.02e-3, -20e-6)])
        print(setpoints)
        print(setpoints_reference)
        self.assertEqual(np.allclose(setpoints, setpoints_reference), True)

    def test_SMU2540_setup_current_high_setpoints(self):
        setpoints = CALIBRATION_SETUP_SMU2450.get_current_high_setpoints()
        setpoints_reference = np.hstack([np.arange(-0.1, 0.102, 2e-3),
                                         np.arange(0.098, -0.102, -2e-3)])
        print(setpoints)
        print(setpoints_reference)
        self.assertEqual(np.allclose(setpoints, setpoints_reference), True)

    def test_basic_setup_setpoint_count(self):
        self.assertEqual(CALIBRATION_SETUP_BASIC.get_setpoint_count(), 3)

    def test_setpoint_detection_size(self):
        data_measure = \
            (RocketLoggerData(_VOLTAGE_FILE).get_data('V1').squeeze() /
             rlc._ROCKETLOGGER_FILE_SCALE_V)
        setpoint = rlc._extract_setpoint_measurement(
            data_measure,
            CALIBRATION_SETUP_SMU2450.get_voltage_step(calibration=True))
        self.assertEqual(len(setpoint),
                         CALIBRATION_SETUP_SMU2450.get_setpoint_count())

    def test_setpoint_detection_invalid_scale(self):
        data_measure = \
            (RocketLoggerData(_VOLTAGE_FILE).get_data('V1').squeeze() /
             rlc._ROCKETLOGGER_FILE_SCALE_V)
        with self.assertRaises(Warning):
            rlc._extract_setpoint_measurement(
                data_measure,
                CALIBRATION_SETUP_SMU2450.get_voltage_step())


class TestCalibrationProcedure(TestCase):

    def test_calibration_empty(self):
        cal = RocketLoggerCalibration()
        with self.assertRaises(RocketLoggerCalibrationError):
            cal.recalibrate(CALIBRATION_SETUP_SMU2450)

    def test_calibration_by_filename(self):
        cal = RocketLoggerCalibration()
        cal.load_measurement_data(_VOLTAGE_FILE, _CURRENT_LO1_FILE,
                                  _CURRENT_HI_FILE, _CURRENT_LO2_FILE,
                                  _CURRENT_HI_FILE)
        cal.recalibrate(CALIBRATION_SETUP_SMU2450)
        self._check_reference_calibration(cal)

    def test_calibration_by_filename_direct(self):
        cal = RocketLoggerCalibration(_VOLTAGE_FILE, _CURRENT_LO1_FILE,
                                      _CURRENT_HI_FILE, _CURRENT_LO2_FILE,
                                      _CURRENT_HI_FILE)
        cal.recalibrate(CALIBRATION_SETUP_SMU2450)
        self._check_reference_calibration(cal)

    def test_calibration_by_data(self):
        data_v = RocketLoggerData(_VOLTAGE_FILE)
        data_i1l = RocketLoggerData(_CURRENT_LO1_FILE)
        data_i2l = RocketLoggerData(_CURRENT_LO2_FILE)
        data_ih = RocketLoggerData(_CURRENT_HI_FILE)
        cal = RocketLoggerCalibration()
        cal.load_measurement_data(data_v, data_i1l, data_ih, data_i2l, data_ih)
        cal.recalibrate(CALIBRATION_SETUP_SMU2450)
        self._check_reference_calibration(cal)

    def test_calibration_by_data_direct(self):
        data_v = RocketLoggerData(_VOLTAGE_FILE)
        data_i1l = RocketLoggerData(_CURRENT_LO1_FILE)
        data_i2l = RocketLoggerData(_CURRENT_LO2_FILE)
        data_ih = RocketLoggerData(_CURRENT_HI_FILE)
        cal = RocketLoggerCalibration(data_v, data_i1l, data_ih, data_i2l,
                                      data_ih)
        cal.recalibrate(CALIBRATION_SETUP_SMU2450)
        self._check_reference_calibration(cal)

    def test_calibration_write_reread(self):
        cal = RocketLoggerCalibration(_VOLTAGE_FILE, _CURRENT_LO1_FILE,
                                      _CURRENT_HI_FILE, _CURRENT_LO2_FILE,
                                      _CURRENT_HI_FILE)
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
        # reference_time = np.datetime64('2017-05-09T07:36:21', dtype='M8[s]')
        reference_time = np.datetime64('2017-05-09T07:37:49', dtype='M8[s]')
        reference_offset = np.array(
            [7978, 1769, 1079, 860, -3652, 1990, 967, 910])
        reference_scale = np.array(
            [3.15541961e+01, 1.75362819e+01, -1.22411163e+02, -1.22087393e+02,
             3.15646990e+01, 1.75031048e+01, -1.22223889e+02, -1.22244353e+02])
        self.assertEqual(calibration._calibration_timestamp, reference_time)
        self.assertEqual(np.allclose(
            calibration._calibration_offset, reference_offset), True)
        self.assertEqual(np.allclose(
            calibration._calibration_scale, reference_scale), True)
