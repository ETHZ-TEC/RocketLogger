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

from rocketlogger.data import RocketLoggerData, RocketLoggerDataError, \
    RocketLoggerDataWarning, RocketLoggerFileError
from rocketlogger.calibration import RocketLoggerCalibration, \
    RocketLoggerCalibrationError


_TEST_FILE_DIR = 'data'
_CALIBRATION_FILE = os.path.join(_TEST_FILE_DIR, 'test_calibration_sample.dat')
_VOLTAGE_FILE = os.path.join(_TEST_FILE_DIR, 'test_calibration_v.rld')
_CURRENT_LO1_FILE = os.path.join(_TEST_FILE_DIR, 'test_calibration_il1.rld')
_CURRENT_LO2_FILE = os.path.join(_TEST_FILE_DIR, 'test_calibration_il2.rld')
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
        cal1 = RocketLoggerCalibration(_CALIBRATION_FILE)
        cal1.write_calibration_file(_TEMP_FILE)
        cal2 = RocketLoggerCalibration(_TEMP_FILE)
        self.assertEqual(cal1, cal2)

    @classmethod
    def tearDownClass(cls):
        os.remove(_TEMP_FILE)
