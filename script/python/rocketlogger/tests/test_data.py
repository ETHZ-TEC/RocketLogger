"""
RocketLogger data file import tests.

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

import os.path
from unittest import TestCase

import numpy as np
# select matplotlib backend not requiring dispaly _before_ importing pyplot
import matplotlib as mpl
mpl.use('Agg')
import matplotlib.pyplot as plt
import rocketlogger.data as rld

from rocketlogger.data import RocketLoggerData, RocketLoggerDataError, \
    RocketLoggerDataWarning, RocketLoggerFileError


_TEST_FILE_DIR = 'data'
_FULL_TEST_FILE = os.path.join(_TEST_FILE_DIR, 'test-full.rld')
_SINGLE_BLOCK_FILE = os.path.join(_TEST_FILE_DIR, 'test-single-block.rld')
_ANALOG_TEST_FILE = os.path.join(_TEST_FILE_DIR, 'test-analog-only.rld')
_HIGH_CURRENT_TEST_FILE = os.path.join(_TEST_FILE_DIR, 'test-high-current.rld')
_STEPS_TEST_FILE = os.path.join(_TEST_FILE_DIR, 'test-steps.rld')
_INCOMPATIBLE_TEST_FILE = os.path.join(_TEST_FILE_DIR, 'test-unsupported.rld')
_SINGLE_TEST_FILE = os.path.join(_TEST_FILE_DIR, 'test-v3-only.rld')
_SPLIT_TEST_FILE = os.path.join(_TEST_FILE_DIR, 'test-split.rld')


class TestDecimation(TestCase):

    def test_binary_decimation(self):
        data_in = np.ones((100))
        data_in[0:2:100] = 0
        data_ref = np.ones((10))
        data_out = rld._decimate_binary(data_in, 10)
        self.assertAlmostEqual(sum(abs(data_out - data_ref)), 0)

    def test_min_decimation(self):
        data_in = np.ones((100))
        data_in[0:100:10] = 0
        data_ref = np.zeros((10))
        data_out = rld._decimate_min(data_in, 10)
        print(data_in)
        print(data_out)
        self.assertEqual(sum(abs(data_out - data_ref)), 0)

    def test_max_decimation(self):
        data_in = np.zeros((100))
        data_in[0:100:10] = 1
        data_ref = np.ones((10))
        data_out = rld._decimate_max(data_in, 10)
        print(data_in)
        print(data_out)
        self.assertAlmostEqual(sum(abs(data_out - data_ref)), 0)

    def test_mean_decimation(self):
        data_in = np.arange(0, 100)
        data_ref = 4.5 + np.arange(0, 100, 10)
        data_out = rld._decimate_mean(data_in, 10)
        self.assertAlmostEqual(sum(abs(data_out - data_ref)), 0)


class TestFileImport(TestCase):

    def test_normal(self):
        data = RocketLoggerData(_FULL_TEST_FILE)
        self.assertEqual(data.get_data().shape, (5000, 16))

    def test_no_file(self):
        with self.assertRaises(NotImplementedError):
            self.data = RocketLoggerData()

    def test_inexistend_file(self):
        with self.assertRaises(FileNotFoundError):
            self.data = RocketLoggerData('nonexistent-data.rld')

    def test_wrong_magic(self):
        with self.assertRaises(RocketLoggerFileError):
            self.data = RocketLoggerData(_INCOMPATIBLE_TEST_FILE)

    def test_with_decimation(self):
        data = RocketLoggerData(_FULL_TEST_FILE, decimation_factor=10)
        self.assertEqual(data._header['data_block_size'], 100)

    def test_with_invalid_decimation(self):
        with self.assertRaises(RocketLoggerDataError):
            RocketLoggerData(_FULL_TEST_FILE, decimation_factor=3)

    def test_direct_import(self):
        data = RocketLoggerData(_FULL_TEST_FILE, memory_mapped=False)
        self.assertEqual(data.get_data('V1').shape, (5000, 1))

    def test_direct_import_with_decimation(self):
        data = RocketLoggerData(_FULL_TEST_FILE, memory_mapped=False,
                                decimation_factor=10)
        self.assertEqual(data._header['data_block_size'], 100)
        self.assertEqual(data._header['sample_count'], 500)
        self.assertEqual(data._header['sample_rate'], 100)
        self.assertEqual(data.get_data('V1').shape, (500, 1))

    def test_direct_vs_memory_mapped(self):
        data_mm = RocketLoggerData(_FULL_TEST_FILE, memory_mapped=True)
        data_ff = RocketLoggerData(_FULL_TEST_FILE, memory_mapped=False)
        self.assertEqual(data_mm._header, data_ff._header)
        self.assertEqual(len(data_mm._data), len(data_ff._data))
        for i in range(len(data_mm._data)):
            arrays_equal = np.array_equal(data_mm._data[i], data_ff._data[i])
            self.assertTrue(arrays_equal)

    def test_single_block_import(self):
        data = RocketLoggerData(_SINGLE_BLOCK_FILE, memory_mapped=False)
        self.assertEqual(data._header['data_block_count'], 1)
        self.assertEqual(data.get_data('V1').shape, (1000, 1))


class TestChannelMerge(TestCase):

    def test_channel_names(self):
        data = RocketLoggerData(_FULL_TEST_FILE)
        self.assertEqual(data.get_channel_names(),
                         sorted(['DI1', 'DI2', 'DI3', 'DI4', 'DI5', 'DI6',
                                 'I1L_valid', 'I2L_valid',
                                 'V1', 'V2', 'V3', 'V4',
                                 'I1L', 'I1H', 'I2L', 'I2H']))

    def test_merge_drop_channel_names(self):
        data = RocketLoggerData(_FULL_TEST_FILE)
        data.merge_channels(keep_channels=False)
        self.assertEqual(data.get_channel_names(),
                         sorted(['DI1', 'DI2', 'DI3', 'DI4', 'DI5', 'DI6',
                                 'V1', 'V2', 'V3', 'V4', 'I1', 'I2']))

    def test_merge_keep_channel_names(self):
        data = RocketLoggerData(_FULL_TEST_FILE)
        data.merge_channels(keep_channels=True)
        self.assertEqual(data.get_channel_names(),
                         sorted(['DI1', 'DI2', 'DI3', 'DI4', 'DI5', 'DI6',
                                 'I1L_valid', 'I2L_valid',
                                 'V1', 'V2', 'V3', 'V4',
                                 'I1L', 'I1H', 'I1', 'I2L', 'I2H', 'I2']))

    def test_merge_inexistent_channels(self):
        data = RocketLoggerData(_SINGLE_TEST_FILE)
        with self.assertWarns(RocketLoggerDataWarning):
            data.merge_channels(keep_channels=False)

    def test_merge_channel_count(self):
        data = RocketLoggerData(_FULL_TEST_FILE)
        self.assertEqual(len(data._header['channels']), 16)
        data.merge_channels(keep_channels=False)
        self.assertEqual(len(data._header['channels']), 12)

    def test_merge_keep_channel_count(self):
        data = RocketLoggerData(_FULL_TEST_FILE)
        self.assertEqual(len(data._header['channels']), 16)
        data.merge_channels(keep_channels=True)
        self.assertEqual(len(data._header['channels']), 18)

    def test_merge_calculation_overflow(self):
        data = RocketLoggerData(_HIGH_CURRENT_TEST_FILE)
        # manual merge with large dtype
        ch_data = data.get_data(['I2L', 'I2H'])
        ch_valid = data.get_data('I2L_valid').flatten()
        ch_merged = ch_valid * ch_data[:, 0] + (1 - ch_valid) * ch_data[:, 1]
        data.merge_channels(keep_channels=False)
        self.assertAlmostEqual(
            sum(abs(data.get_data('I2').flatten() - ch_merged)), 0)

    def test_merge_keep_calculation_overflow(self):
        data = RocketLoggerData(_HIGH_CURRENT_TEST_FILE)
        # manual merge with large dtype
        ch_data = data.get_data(['I2L', 'I2H'])
        ch_valid = data.get_data('I2L_valid').flatten()
        ch_merged = ch_valid * ch_data[:, 0] + (1 - ch_valid) * ch_data[:, 1]
        data.merge_channels(keep_channels=True)
        self.assertAlmostEqual(
            sum(abs(data.get_data('I2').flatten() - ch_merged)), 0)


class TestDataHandling(TestCase):

    def setUp(self):
        self.data = RocketLoggerData(_FULL_TEST_FILE)

    def tearDown(self):
        del(self.data)

    def test_load(self):
        self.assertIsInstance(self.data, RocketLoggerData)

    def test_get_single_channel(self):
        temp = self.data.get_data('V1')
        self.assertEqual(temp.shape[1], 1)

    def test_get_invalid_channel(self):
        with self.assertRaises(KeyError):
            self.data.get_data('A')

    def test_get_all_channels(self):
        temp = self.data.get_data('all')
        self.assertEqual(temp.shape[1], len(self.data.get_channel_names()))

    def test_get_set_of_channels(self):
        channel_names = ['DI1', 'DI3', 'I1L_valid', 'V1', 'V3', 'I1L']
        temp = self.data.get_data(channel_names)
        self.assertEqual(temp.shape[1], len(channel_names))

    def test_get_relative_time(self):
        temp = self.data.get_time(absolute_time=False)
        self.assertEqual(temp.shape[0], self.data._header['sample_count'])
        self.assertIsInstance(temp[0], float)

    def test_get_absolute_local_time(self):
        temp = self.data.get_time(absolute_time=True, time_reference='local')
        self.assertEqual(temp.shape[0], self.data._header['sample_count'])
        self.assertEqual(temp.dtype, np.dtype('<M8[ns]'))

    def test_get_absolute_network_time(self):
        temp = self.data.get_time(absolute_time=True, time_reference='network')
        self.assertEqual(temp.shape[0], self.data._header['sample_count'])
        self.assertEqual(temp.dtype, np.dtype('<M8[ns]'))


class TestDataPlot(TestCase):

    def setUp(self):
        self.data = RocketLoggerData(_STEPS_TEST_FILE)

    def tearDown(self):
        plt.close('all')
        del(self.data)

    def test_load(self):
        self.assertIsInstance(self.data, RocketLoggerData)

    def test_header_field_count(self):
        self.assertEqual(len(self.data._header), 14)

    def test_header_channel_count(self):
        self.assertEqual(len(self.data._header['channels']), 16)

    def test_plot_all(self):
        self.data.plot()

    def test_plot_merged(self):
        self.data.merge_channels()
        self.data.plot()

    def test_plot_invalid(self):
        with self.assertRaises(RocketLoggerDataError):
            self.data.plot(['A'])

    def test_plot_voltage(self):
        self.data.plot(['voltages'])

    def test_plot_current(self):
        self.data.plot(['currents'])

    def test_plot_digital(self):
        self.data.plot(['digital'])

    def test_plot_single(self):
        self.data.plot('V1')

    def test_plot_multi_single(self):
        self.data.plot(['V1', 'I1L', 'DI1'])


class TestDataPlotDecimate(TestCase):

    def setUp(self):
        self.data = RocketLoggerData(_STEPS_TEST_FILE, decimation_factor=10)

    def tearDown(self):
        plt.close('all')
        del(self.data)

    def test_load(self):
        self.assertIsInstance(self.data, RocketLoggerData)

    def test_header_field_count(self):
        self.assertEqual(len(self.data._header), 14)

    def test_header_channel_count(self):
        self.assertEqual(len(self.data._header['channels']), 16)

    def test_plot_all(self):
        self.data.plot()

    def test_plot_merged(self):
        self.data.merge_channels()
        self.data.plot()

    def test_plot_invalid(self):
        with self.assertRaises(RocketLoggerDataError):
            self.data.plot(['A'])

    def test_plot_voltage(self):
        self.data.plot(['voltages'])

    def test_plot_current(self):
        self.data.plot(['currents'])

    def test_plot_digital(self):
        self.data.plot(['digital'])

    def test_plot_single(self):
        self.data.plot('V1')

    def test_plot_multi_single(self):
        self.data.plot(['V1', 'I1L', 'DI1'])


class TestFullFile(TestCase):

    def setUp(self):
        self.data = RocketLoggerData(_FULL_TEST_FILE)

    def tearDown(self):
        del(self.data)

    def test_load(self):
        self.assertIsInstance(self.data, RocketLoggerData)

    def test_header_field_count(self):
        self.assertEqual(len(self.data._header), 14)

    def test_header_channel_count(self):
        self.assertEqual(len(self.data._header['channels']), 16)

    def test_data_size(self):
        self.assertEqual(self.data.get_data().shape, (5000, 16))

    def test_channel_names(self):
        self.assertEqual(self.data.get_channel_names(),
                         sorted(['DI1', 'DI2', 'DI3', 'DI4', 'DI5', 'DI6',
                                 'I1L_valid', 'I2L_valid',
                                 'V1', 'V2', 'V3', 'V4',
                                 'I1L', 'I1H', 'I2L', 'I2H']))


class TestJoinFile(TestCase):

    def setUp(self):
        self.data = RocketLoggerData(_SPLIT_TEST_FILE)

    def tearDown(self):
        del(self.data)

    def test_load(self):
        self.assertIsInstance(self.data, RocketLoggerData)

    def test_header_field_count(self):
        self.assertEqual(len(self.data._header), 14)

    def test_header_channel_count(self):
        self.assertEqual(len(self.data._header['channels']), 16)

    def test_data_size(self):
        self.assertEqual(self.data.get_data().shape, (3 * 128000, 16))

    def test_channel_names(self):
        self.assertEqual(self.data.get_channel_names(),
                         sorted(['DI1', 'DI2', 'DI3', 'DI4', 'DI5', 'DI6',
                                 'I1L_valid', 'I2L_valid',
                                 'V1', 'V2', 'V3', 'V4',
                                 'I1L', 'I1H', 'I2L', 'I2H']))


class TestNoJoinFile(TestCase):

    def setUp(self):
        self.data = RocketLoggerData(_SPLIT_TEST_FILE, join_files=False)

    def tearDown(self):
        del(self.data)

    def test_load(self):
        self.assertIsInstance(self.data, RocketLoggerData)

    def test_header_field_count(self):
        self.assertEqual(len(self.data._header), 14)

    def test_header_channel_count(self):
        self.assertEqual(len(self.data._header['channels']), 16)

    def test_data_size(self):
        self.assertEqual(self.data.get_data().shape, (128000, 16))

    def test_channel_names(self):
        self.assertEqual(self.data.get_channel_names(),
                         sorted(['DI1', 'DI2', 'DI3', 'DI4', 'DI5', 'DI6',
                                 'I1L_valid', 'I2L_valid',
                                 'V1', 'V2', 'V3', 'V4',
                                 'I1L', 'I1H', 'I2L', 'I2H']))
