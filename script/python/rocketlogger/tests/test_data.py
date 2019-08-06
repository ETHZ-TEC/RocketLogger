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

from rocketlogger.data import RocketLoggerData, RocketLoggerDataError, \
    RocketLoggerDataWarning, RocketLoggerFileError
import os.path
from unittest import TestCase
import unittest
import numpy as np
import rocketlogger.data as rld

import os
if os.environ.get("MATPLOTLIB_AVAILABLE") == "true":
    import matplotlib.pyplot as plt
if os.environ.get("PANDAS_AVAILABLE") == "true":
    import pandas as pd


_TEST_FILE_DIR = 'data'
_FULL_TEST_FILE = os.path.join(_TEST_FILE_DIR, 'test_full.rld')
_SINGLE_BLOCK_FILE = os.path.join(_TEST_FILE_DIR, 'test_single_block.rld')
_MIN_BLOCK_SIZE_FILE = os.path.join(_TEST_FILE_DIR, 'test_min_block_size.rld')
_ANALOG_TEST_FILE = os.path.join(_TEST_FILE_DIR, 'test_analog_only.rld')
_HIGH_CURRENT_TEST_FILE = os.path.join(_TEST_FILE_DIR, 'test_high_current.rld')
_STEPS_TEST_FILE = os.path.join(_TEST_FILE_DIR, 'test_steps.rld')
_INCOMPATIBLE_TEST_FILE = os.path.join(_TEST_FILE_DIR, 'test_unsupported.rld')
_INEXISTENT_TEST_FILE = os.path.join(_TEST_FILE_DIR, 'test_inexistent.rld')
_SINGLE_TEST_FILE = os.path.join(_TEST_FILE_DIR, 'test_v3_only.rld')
_TRUNCATED_TEST_FILE = os.path.join(_TEST_FILE_DIR, 'test_truncated.rld')
_SPLIT_TEST_FILE = os.path.join(_TEST_FILE_DIR, 'test_split.rld')


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
            RocketLoggerData()

    def test_inexistent_file(self):
        with self.assertRaises(FileNotFoundError):
            RocketLoggerData(_INEXISTENT_TEST_FILE)

    def test_overload_existing(self):
        data = RocketLoggerData(_FULL_TEST_FILE)
        with self.assertRaises(RocketLoggerDataError):
            data.load_file(_FULL_TEST_FILE)

    def test_wrong_magic(self):
        with self.assertRaises(RocketLoggerFileError):
            RocketLoggerData(_INCOMPATIBLE_TEST_FILE)

    def test_header_dict(self):
        data = RocketLoggerData(_FULL_TEST_FILE)
        header = {'data_block_count': 5,
                  'data_block_size': 1000,
                  'file_version': 2,
                  'mac_address': '12:34:56:78:90:ab',
                  'sample_count': 5000,
                  'sample_rate': 1000,
                  'start_time': np.datetime64('2017-05-10T09:05:17.438817080')}
        self.assertDictEqual(data.get_header(), header)

    def test_header_dict_with_decimation(self):
        data = RocketLoggerData(_FULL_TEST_FILE, decimation_factor=10)
        header = {'data_block_count': 5,
                  'data_block_size': 100,
                  'file_version': 2,
                  'mac_address': '12:34:56:78:90:ab',
                  'sample_count': 500,
                  'sample_rate': 100,
                  'start_time': np.datetime64('2017-05-10T09:05:17.438817080')}
        self.assertDictEqual(data.get_header(), header)

    def test_with_decimation(self):
        data = RocketLoggerData(_FULL_TEST_FILE, decimation_factor=10)
        self.assertEqual(data._header['data_block_size'], 100)

    def test_with_invalid_decimation(self):
        with self.assertRaises(ValueError):
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

    def test_min_block_size_import(self):
        data = RocketLoggerData(_MIN_BLOCK_SIZE_FILE, memory_mapped=False)
        self.assertEqual(data._header['data_block_size'], 1)
        self.assertEqual(data.get_data('V1').shape, (5, 1))


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


class TestSingleChannelFile(TestCase):

    def setUp(self):
        self.data = RocketLoggerData(_SINGLE_TEST_FILE)

    def tearDown(self):
        del(self.data)

    def test_load(self):
        self.assertIsInstance(self.data, RocketLoggerData)

    def test_header_field_count(self):
        self.assertEqual(len(self.data._header), 14)

    def test_header_channel_count(self):
        self.assertEqual(len(self.data._header['channels']), 1)

    def test_data_size(self):
        self.assertEqual(self.data.get_data().shape, (5000, 1))

    def test_channel_names(self):
        self.assertEqual(self.data.get_channel_names(), ['V3'])


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


class TestJoinExclude(TestCase):

    def test_exclude_all(self):
        with self.assertRaises(RocketLoggerDataError):
            RocketLoggerData(_SPLIT_TEST_FILE, exclude_part=np.arange(0, 3))

    def test_exclude_non_join(self):
        with self.assertRaises(ValueError):
            RocketLoggerData(_SPLIT_TEST_FILE, join_files=False,
                             exclude_part=np.arange(0, 3))


class TestJoinExcludeFirst(TestCase):

    def setUp(self):
        self.full_reference = RocketLoggerData(_SPLIT_TEST_FILE)
        self.data = RocketLoggerData(_SPLIT_TEST_FILE, exclude_part=0)

    def tearDown(self):
        del(self.full_reference)
        del(self.data)

    def test_load(self):
        self.assertIsInstance(self.data, RocketLoggerData)

    def test_data_values(self):
        reference_range = np.arange(128000, 3*128000)
        reference = self.full_reference.get_data()[reference_range]
        self.assertTrue(np.array_equal(self.data.get_data(), reference))

    def test_data_timestamp_monotonic(self):
        reference_range = np.arange(2, 3*2)
        reference = self.full_reference._timestamps_monotonic[reference_range]
        self.assertTrue(np.array_equal(
            self.data._timestamps_monotonic, reference))

    def test_data_timestamp_realtime(self):
        reference_range = np.arange(2, 3*2)
        reference = self.full_reference._timestamps_realtime[reference_range]
        self.assertTrue(np.array_equal(
            self.data._timestamps_realtime, reference))


class TestJoinExcludeLast(TestCase):

    def setUp(self):
        self.full_reference = RocketLoggerData(_SPLIT_TEST_FILE)
        self.data = RocketLoggerData(_SPLIT_TEST_FILE, exclude_part=2)

    def tearDown(self):
        del(self.full_reference)
        del(self.data)

    def test_load(self):
        self.assertIsInstance(self.data, RocketLoggerData)

    def test_data_values(self):
        reference_range = np.arange(2*128000)
        reference = self.full_reference.get_data()[reference_range]
        self.assertTrue(np.array_equal(self.data.get_data(), reference))

    def test_data_timestamp_monotonic(self):
        reference_range = np.arange(2*2)
        reference = self.full_reference._timestamps_monotonic[reference_range]
        self.assertTrue(np.array_equal(
            self.data._timestamps_monotonic, reference))

    def test_data_timestamp_realtime(self):
        reference_range = np.arange(2*2)
        reference = self.full_reference._timestamps_realtime[reference_range]
        self.assertTrue(np.array_equal(
            self.data._timestamps_realtime, reference))


class TestJoinExcludeMultiple(TestCase):

    def setUp(self):
        self.full_reference = RocketLoggerData(_SPLIT_TEST_FILE)
        self.data = RocketLoggerData(_SPLIT_TEST_FILE, exclude_part=[0, 2])

    def tearDown(self):
        del(self.full_reference)
        del(self.data)

    def test_load(self):
        self.assertIsInstance(self.data, RocketLoggerData)

    def test_data_values(self):
        reference_range = np.arange(128000, 2*128000)
        reference = self.full_reference.get_data()[reference_range]
        self.assertTrue(np.array_equal(self.data.get_data(), reference))

    def test_data_timestamp_monotonic(self):
        reference_range = np.arange(2, 2*2)
        reference = self.full_reference._timestamps_monotonic[reference_range]
        self.assertTrue(np.array_equal(
            self.data._timestamps_monotonic, reference))

    def test_data_timestamp_realtime(self):
        reference_range = np.arange(2, 2*2)
        reference = self.full_reference._timestamps_realtime[reference_range]
        self.assertTrue(np.array_equal(
            self.data._timestamps_realtime, reference))


class TestJoinExcludeInt(TestCase):

    def setUp(self):
        self.full_reference = RocketLoggerData(_SPLIT_TEST_FILE)
        self.data = RocketLoggerData(_SPLIT_TEST_FILE, exclude_part=1)

    def tearDown(self):
        del(self.full_reference)
        del(self.data)

    def test_load(self):
        self.assertIsInstance(self.data, RocketLoggerData)

    def test_data_values(self):
        reference_range = np.hstack((np.arange(128000),
                                     np.arange(2*128000, 3*128000)))
        reference = self.full_reference.get_data()[reference_range]
        self.assertTrue(np.array_equal(self.data.get_data(), reference))

    def test_data_timestamp_monotonic(self):
        reference_range = np.hstack((np.arange(2), np.arange(2*2, 3*2)))
        reference = self.full_reference._timestamps_monotonic[reference_range]
        self.assertTrue(np.array_equal(
            self.data._timestamps_monotonic, reference))

    def test_data_timestamp_realtime(self):
        reference_range = np.hstack((np.arange(2), np.arange(2*2, 3*2)))
        reference = self.full_reference._timestamps_realtime[reference_range]
        self.assertTrue(np.array_equal(
            self.data._timestamps_realtime, reference))


class TestJoinExcludeList(TestCase):

    def setUp(self):
        self.full_reference = RocketLoggerData(_SPLIT_TEST_FILE)
        self.data = RocketLoggerData(_SPLIT_TEST_FILE, exclude_part=[1])

    def tearDown(self):
        del(self.full_reference)
        del(self.data)

    def test_load(self):
        self.assertIsInstance(self.data, RocketLoggerData)

    def test_data_values(self):
        reference_range = np.hstack((np.arange(128000),
                                     np.arange(2*128000, 3*128000)))
        reference = self.full_reference.get_data()[reference_range]
        self.assertTrue(np.array_equal(self.data.get_data(), reference))

    def test_data_timestamp_monotonic(self):
        reference_range = np.hstack((np.arange(2), np.arange(2*2, 3*2)))
        reference = self.full_reference._timestamps_monotonic[reference_range]
        self.assertTrue(np.array_equal(
            self.data._timestamps_monotonic, reference))

    def test_data_timestamp_realtime(self):
        reference_range = np.hstack((np.arange(2), np.arange(2*2, 3*2)))
        reference = self.full_reference._timestamps_realtime[reference_range]
        self.assertTrue(np.array_equal(
            self.data._timestamps_realtime, reference))


class TestJoinExcludeArray(TestCase):

    def setUp(self):
        self.full_reference = RocketLoggerData(_SPLIT_TEST_FILE)
        self.data = RocketLoggerData(
            _SPLIT_TEST_FILE, exclude_part=np.arange(1, 2))

    def tearDown(self):
        del(self.full_reference)
        del(self.data)

    def test_load(self):
        self.assertIsInstance(self.data, RocketLoggerData)

    def test_data_values(self):
        reference_range = np.hstack((np.arange(128000),
                                     np.arange(2*128000, 3*128000)))
        reference = self.full_reference.get_data()[reference_range]
        self.assertTrue(np.array_equal(self.data.get_data(), reference))

    def test_data_timestamp_monotonic(self):
        reference_range = np.hstack((np.arange(2), np.arange(2*2, 3*2)))
        reference = self.full_reference._timestamps_monotonic[reference_range]
        self.assertTrue(np.array_equal(
            self.data._timestamps_monotonic, reference))

    def test_data_timestamp_realtime(self):
        reference_range = np.hstack((np.arange(2), np.arange(2*2, 3*2)))
        reference = self.full_reference._timestamps_realtime[reference_range]
        self.assertTrue(np.array_equal(
            self.data._timestamps_realtime, reference))


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


class TestRecoveryFile(TestCase):

    def test_no_recovery(self):
        with self.assertRaises(RocketLoggerDataError):
            RocketLoggerData(_TRUNCATED_TEST_FILE)

    def test_header_dict(self):
        with self.assertWarns(RocketLoggerDataWarning):
            data = RocketLoggerData(_TRUNCATED_TEST_FILE, recovery=True)
        header = {'data_block_count': 4,
                  'data_block_size': 1000,
                  'file_version': 2,
                  'mac_address': '12:34:56:78:90:ab',
                  'sample_count': 4000,
                  'sample_rate': 1000,
                  'start_time': np.datetime64('2017-05-10T09:09:33.455293012')}
        self.assertDictEqual(data.get_header(), header)

    def test_header_dict_with_decimation(self):
        with self.assertWarns(RocketLoggerDataWarning):
            data = RocketLoggerData(_TRUNCATED_TEST_FILE, recovery=True,
                                    decimation_factor=10)
        header = {'data_block_count': 4,
                  'data_block_size': 100,
                  'file_version': 2,
                  'mac_address': '12:34:56:78:90:ab',
                  'sample_count': 400,
                  'sample_rate': 100,
                  'start_time': np.datetime64('2017-05-10T09:09:33.455293012')}
        self.assertDictEqual(data.get_header(), header)

    def test_with_decimation(self):
        with self.assertWarns(RocketLoggerDataWarning):
            data = RocketLoggerData(_TRUNCATED_TEST_FILE, recovery=True,
                                    decimation_factor=10)
        self.assertEqual(data._header['data_block_size'], 100)

    def test_with_invalid_decimation(self):
        with self.assertRaises(ValueError):
            RocketLoggerData(_TRUNCATED_TEST_FILE, recovery=True,
                             decimation_factor=3)

    def test_direct_import(self):
        with self.assertWarns(RocketLoggerDataWarning):
            data = RocketLoggerData(_TRUNCATED_TEST_FILE, recovery=True,
                                    memory_mapped=False)
        self.assertEqual(data.get_data('V3').shape, (4000, 1))

    def test_direct_import_with_decimation(self):
        with self.assertWarns(RocketLoggerDataWarning):
            data = RocketLoggerData(_TRUNCATED_TEST_FILE, recovery=True,
                                    memory_mapped=False, decimation_factor=10)
        self.assertEqual(data._header['data_block_size'], 100)
        self.assertEqual(data._header['sample_count'], 400)
        self.assertEqual(data._header['sample_rate'], 100)
        self.assertEqual(data.get_data('V3').shape, (400, 1))


class TestChannelHandling(TestCase):

    def setUp(self):
        self.data = RocketLoggerData(_FULL_TEST_FILE)

    def tearDown(self):
        del(self.data)

    def test_get_single_channel_index(self):
        index = self.data._get_channel_index('V1')
        self.assertEqual(index, 10)

    def test_get_invalid_channel_index(self):
        temp = self.data._get_channel_index('A')
        self.assertEqual(temp, None)

    def test_get_channel_name(self):
        name = self.data._get_channel_name(10)
        self.assertEqual(name, 'V1')

    def test_get_channel_name_list(self):
        names = self.data._get_channel_name([10, 11])
        self.assertListEqual(names, ['V1', 'V2'])

    def test_channel_add(self):
        channel_info = self.data._header['channels'][10].copy()
        channel_info['name'] = 'A1'
        self.data.add_channel(channel_info,
                              np.ones_like(self.data.get_data('V1').squeeze()))
        self.assertTrue('A1' in self.data.get_channel_names())

    def test_channel_add_binary(self):
        channel_info = self.data._header['channels'][0].copy()
        channel_info['name'] = 'B1'
        self.data.add_channel(
            channel_info, np.ones_like(self.data.get_data('DI1').squeeze()))
        self.assertTrue('B1' in self.data.get_channel_names())

    def test_channel_add_existing(self):
        channel_info = self.data._header['channels'][10].copy()
        with self.assertRaises(ValueError):
            self.data.add_channel(
                channel_info,
                np.ones_like(self.data.get_data('V1').squeeze()))

    def test_channel_add_no_unit(self):
        channel_info = self.data._header['channels'][10].copy()
        del(channel_info['unit_index'])
        with self.assertRaises(ValueError):
            self.data.add_channel(
                channel_info,
                np.ones_like(self.data.get_data('V1').squeeze()))

    def test_channel_add_no_scale(self):
        channel_info = self.data._header['channels'][10].copy()
        del(channel_info['scale'])
        with self.assertRaises(ValueError):
            self.data.add_channel(
                channel_info,
                np.ones_like(self.data.get_data('V1').squeeze()))

    def test_channel_add_no_data_size(self):
        channel_info = self.data._header['channels'][10].copy()
        del(channel_info['data_size'])
        with self.assertRaises(ValueError):
            self.data.add_channel(
                channel_info,
                np.ones_like(self.data.get_data('V1').squeeze()))

    def test_channel_add_no_name(self):
        channel_info = self.data._header['channels'][10].copy()
        del(channel_info['name'])
        with self.assertRaises(ValueError):
            self.data.add_channel(
                channel_info,
                np.ones_like(self.data.get_data('V1').squeeze()))

    def test_channel_add_no_link(self):
        channel_info = self.data._header['channels'][10].copy()
        del(channel_info['valid_link'])
        with self.assertRaises(ValueError):
            self.data.add_channel(
                channel_info,
                np.ones_like(self.data.get_data('V1').squeeze()))

    def test_channel_add_invalid_info(self):
        with self.assertRaises(TypeError):
            self.data.add_channel(
                None,
                np.ones_like(self.data.get_data('V1').squeeze()))

    def test_channel_add_invalid_unit(self):
        channel_info = self.data._header['channels'][10].copy()
        channel_info['name'] = 'A1'
        channel_info['unit_index'] = -1
        with self.assertRaises(ValueError):
            self.data.add_channel(
                channel_info,
                np.ones_like(self.data.get_data('V1').squeeze()))

    def test_channel_add_invalid_scale(self):
        channel_info = self.data._header['channels'][10].copy()
        channel_info['name'] = 'A1'
        channel_info['scale'] = 'micro'
        with self.assertRaises(TypeError):
            self.data.add_channel(
                channel_info,
                np.ones_like(self.data.get_data('V1').squeeze()))

    def test_channel_add_invalid_data_size(self):
        channel_info = self.data._header['channels'][10].copy()
        channel_info['name'] = 'A1'
        channel_info['data_size'] = 'int8'
        with self.assertRaises(TypeError):
            self.data.add_channel(
                channel_info,
                np.ones_like(self.data.get_data('V1').squeeze()))

    def test_channel_add_invalid_link(self):
        channel_info = self.data._header['channels'][10].copy()
        channel_info['name'] = 'A1'
        channel_info['valid_link'] = None
        with self.assertRaises(TypeError):
            self.data.add_channel(
                channel_info,
                np.ones_like(self.data.get_data('V1').squeeze()))

    def test_channel_add_inexisting_link(self):
        channel_info = self.data._header['channels'][10].copy()
        channel_info['name'] = 'A1'
        channel_info['valid_link'] = len(self.data._header['channels'])
        with self.assertRaises(ValueError):
            self.data.add_channel(
                channel_info,
                np.ones_like(self.data.get_data('V1').squeeze()))

    def test_channel_add_invalid_name(self):
        channel_info = self.data._header['channels'][10].copy()
        channel_info['name'] = 100
        with self.assertRaises(TypeError):
            self.data.add_channel(
                channel_info,
                np.ones_like(self.data.get_data('V1').squeeze()))

    def test_channel_add_overlength_name(self):
        channel_info = self.data._header['channels'][10].copy()
        channel_info['name'] = 'over_length_channel_name'
        with self.assertRaises(ValueError):
            self.data.add_channel(
                channel_info,
                np.ones_like(self.data.get_data('V1').squeeze()))

    def test_channel_remove(self):
        self.data.remove_channel('V4')
        self.assertFalse('V4' in self.data.get_channel_names())

    def test_channel_add_remove(self):
        channel_info = self.data._header['channels'][10].copy()
        channel_info['name'] = 'A1'
        self.data.add_channel(channel_info,
                              np.ones_like(self.data.get_data('V1').squeeze()))
        self.assertTrue('A1' in self.data.get_channel_names())
        self.data.remove_channel('A1')
        self.assertFalse('A1' in self.data.get_channel_names())

    def test_channel_remove_inexistent(self):
        with self.assertRaises(KeyError):
            self.data.remove_channel('A')


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

    def test_merge_header_only(self):
        data = RocketLoggerData(_FULL_TEST_FILE, header_only=True)
        with self.assertRaises(TypeError):
            data.merge_channels()

    def test_merge_drop_remerge(self):
        data = RocketLoggerData(_FULL_TEST_FILE)
        data.merge_channels(keep_channels=False)
        with self.assertWarns(RocketLoggerDataWarning):
            data.merge_channels()

    def test_merge_keep_remerge(self):
        data = RocketLoggerData(_FULL_TEST_FILE)
        data.merge_channels(keep_channels=True)
        with self.assertRaises(RocketLoggerDataError):
            data.merge_channels()

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

    def test_header_dict(self):
        temp = self.data.get_header()
        header = {'data_block_count': 5,
                  'data_block_size': 1000,
                  'file_version': 2,
                  'mac_address': '12:34:56:78:90:ab',
                  'sample_count': 5000,
                  'sample_rate': 1000,
                  'start_time': np.datetime64('2017-05-10T09:05:17.438817080')}
        self.assertDictEqual(temp, header)

    def test_get_comment(self):
        temp = self.data.get_comment()
        self.assertEqual(temp, 'This is a comment')

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

    def test_get_single_channel_unit(self):
        temp = self.data.get_unit('V1')
        self.assertListEqual(temp, ['voltage'])

    def test_get_invalid_channel_unit(self):
        with self.assertRaises(KeyError):
            self.data.get_unit('A')

    def test_get_all_channels_unit(self):
        temp = self.data.get_unit('all')
        channel_units = ['binary'] * 6 + (['current'] * 2 +
                                          ['data valid (binary)']) * 2 + \
            ['voltage'] * 4
        self.assertListEqual(temp, channel_units)

    def test_get_set_of_channels_unit(self):
        channel_names = ['DI1', 'DI3', 'I1L_valid', 'V1', 'V3', 'I1L']
        channel_units = ['binary', 'binary', 'data valid (binary)',
                         'voltage', 'voltage', 'current']
        temp = self.data.get_unit(channel_names)
        self.assertListEqual(temp, channel_units)

    def test_get_single_channel_validity(self):
        temp = self.data.get_validity('V1')
        self.assertEqual(temp.shape[1], 1)
        self.assertTrue(np.all(temp))

    def test_get_invalid_channel_validity(self):
        with self.assertRaises(KeyError):
            self.data.get_validity('A')

    def test_get_all_channels_validity(self):
        temp = self.data.get_validity('all')
        self.assertEqual(temp.shape[1], len(self.data.get_channel_names()))

    def test_get_set_of_channels_validity(self):
        channel_names = ['DI1', 'DI3', 'I1L_valid', 'V1', 'V3', 'I1L']
        temp = self.data.get_validity(channel_names)
        self.assertEqual(temp.shape[1], len(channel_names))

    def test_get_default_time(self):
        temp = self.data.get_time()
        self.assertEqual(temp.shape[0], self.data._header['sample_count'])
        self.assertIsInstance(temp[0], float)

    def test_get_relative_time(self):
        temp = self.data.get_time(time_reference='relative')
        self.assertEqual(temp.shape[0], self.data._header['sample_count'])
        self.assertIsInstance(temp[0], float)

    def test_get_absolute_local_time(self):
        temp = self.data.get_time(time_reference='local')
        self.assertEqual(temp.shape[0], self.data._header['sample_count'])
        self.assertEqual(temp.dtype, np.dtype('<M8[ns]'))

    def test_get_absolute_network_time(self):
        temp = self.data.get_time(time_reference='network')
        self.assertEqual(temp.shape[0], self.data._header['sample_count'])
        self.assertEqual(temp.dtype, np.dtype('<M8[ns]'))

    def test_get_filename(self):
        temp = self.data.get_filename()
        self.assertEqual(temp, os.path.abspath(_FULL_TEST_FILE))


class TestHeaderOnlyImport(TestCase):

    def test_normal(self):
        data = RocketLoggerData(_FULL_TEST_FILE, header_only=True)
        self.assertEqual(data._header['sample_count'], 5000)

    def test_no_file(self):
        with self.assertRaises(NotImplementedError):
            RocketLoggerData(header_only=True)

    def test_inexistent_file(self):
        with self.assertRaises(FileNotFoundError):
            RocketLoggerData(_INEXISTENT_TEST_FILE, header_only=True)

    def test_overload_existing(self):
        data = RocketLoggerData(_FULL_TEST_FILE, header_only=True)
        with self.assertRaises(RocketLoggerDataError):
            data.load_file(_FULL_TEST_FILE, header_only=True)

    def test_wrong_magic(self):
        with self.assertRaises(RocketLoggerFileError):
            RocketLoggerData(_INCOMPATIBLE_TEST_FILE, header_only=True)

    def test_header_dict(self):
        data = RocketLoggerData(_FULL_TEST_FILE, header_only=True)
        header = {'data_block_count': 5,
                  'data_block_size': 1000,
                  'file_version': 2,
                  'mac_address': '12:34:56:78:90:ab',
                  'sample_count': 5000,
                  'sample_rate': 1000,
                  'start_time': np.datetime64('2017-05-10T09:05:17.438817080')}
        self.assertDictEqual(data.get_header(), header)

    def test_header_dict_with_decimation(self):
        data = RocketLoggerData(
            _FULL_TEST_FILE, decimation_factor=10, header_only=True)
        header = {'data_block_count': 5,
                  'data_block_size': 100,
                  'file_version': 2,
                  'mac_address': '12:34:56:78:90:ab',
                  'sample_count': 500,
                  'sample_rate': 100,
                  'start_time': np.datetime64('2017-05-10T09:05:17.438817080')}
        self.assertDictEqual(data.get_header(), header)

    def test_with_decimation(self):
        data = RocketLoggerData(
            _FULL_TEST_FILE, decimation_factor=10, header_only=True)
        self.assertEqual(data._header['data_block_size'], 100)

    def test_with_invalid_decimation(self):
        with self.assertRaises(ValueError):
            RocketLoggerData(
                _FULL_TEST_FILE, decimation_factor=3, header_only=True)

    def test_direct_import(self):
        data = RocketLoggerData(_FULL_TEST_FILE, header_only=True)
        self.assertEqual(data._header['data_block_size'], 1000)
        self.assertEqual(data._header['sample_count'], 5000)
        self.assertEqual(data._header['sample_rate'], 1000)

    def test_direct_import_with_decimation(self):
        data = RocketLoggerData(_FULL_TEST_FILE, header_only=True,
                                decimation_factor=10)
        self.assertEqual(data._header['data_block_size'], 100)
        self.assertEqual(data._header['sample_count'], 500)
        self.assertEqual(data._header['sample_rate'], 100)

    def test_single_block_import(self):
        data = RocketLoggerData(_SINGLE_BLOCK_FILE, header_only=True)
        self.assertEqual(data._header['data_block_count'], 1)

    def test_get_single_channel_unit(self):
        data = RocketLoggerData(_FULL_TEST_FILE, header_only=True)
        temp = data.get_unit('V1')
        self.assertListEqual(temp, ['voltage'])

    def test_get_invalid_channel_unit(self):
        data = RocketLoggerData(_FULL_TEST_FILE, header_only=True)
        with self.assertRaises(KeyError):
            data.get_unit('A')

    def test_get_all_channels_unit(self):
        data = RocketLoggerData(_FULL_TEST_FILE, header_only=True)
        temp = data.get_unit('all')
        channel_units = ['binary'] * 6 + (['current'] * 2 +
                                          ['data valid (binary)']) * 2 + \
            ['voltage'] * 4
        self.assertListEqual(temp, channel_units)

    def test_get_set_of_channels_unit(self):
        data = RocketLoggerData(_FULL_TEST_FILE, header_only=True)
        channel_names = ['DI1', 'DI3', 'I1L_valid', 'V1', 'V3', 'I1L']
        channel_units = ['binary', 'binary', 'data valid (binary)',
                         'voltage', 'voltage', 'current']
        temp = data.get_unit(channel_names)
        self.assertListEqual(temp, channel_units)

    def test_get_data(self):
        data = RocketLoggerData(_FULL_TEST_FILE, header_only=True)
        with self.assertRaises(TypeError):
            data.get_data()

    def test_get_time(self):
        data = RocketLoggerData(_FULL_TEST_FILE, header_only=True)
        with self.assertRaises(TypeError):
            data.get_time()

    def test_get_validity(self):
        data = RocketLoggerData(_FULL_TEST_FILE, header_only=True)
        with self.assertRaises(TypeError):
            data.get_validity()


@unittest.skipUnless(os.environ.get("PANDAS_AVAILABLE") == "true",
                     "requires optional pandas dependency")
class TestDataframe(TestCase):

    def setUp(self):
        self.data = RocketLoggerData(_FULL_TEST_FILE)

    def tearDown(self):
        del(self.data)

    def test_load(self):
        self.assertIsInstance(self.data, RocketLoggerData)

    def test_get_dataframe(self):
        temp = self.data.get_dataframe()
        self.assertIsInstance(temp, pd.DataFrame)
        self.assertEqual(temp.shape[1], len(self.data.get_channel_names()))
        self.assertListEqual(list(temp.columns), self.data.get_channel_names())

    def test_get_single_channel(self):
        temp = self.data.get_dataframe('V1')
        self.assertEqual(temp.shape[1], 1)
        self.assertListEqual(list(temp.columns), (['V1']))

    def test_get_invalid_channel(self):
        with self.assertRaises(KeyError):
            self.data.get_dataframe('A')

    def test_get_all_channels(self):
        temp = self.data.get_dataframe('all')
        self.assertIsInstance(temp, pd.DataFrame)
        self.assertEqual(temp.shape[1], len(self.data.get_channel_names()))
        self.assertListEqual(list(temp.columns), self.data.get_channel_names())

    def test_get_set_of_channels(self):
        channel_names = ['DI1', 'DI3', 'I1L_valid', 'V1', 'V3', 'I1L']
        temp = self.data.get_dataframe(channel_names)
        self.assertIsInstance(temp, pd.DataFrame)
        self.assertEqual(temp.shape[1], len(channel_names))
        self.assertListEqual(list(temp.columns), channel_names)

    def test_get_default_time(self):
        temp = self.data.get_dataframe()
        index = temp.index
        self.assertIsInstance(temp, pd.DataFrame)
        self.assertEqual(temp.shape[0], self.data._header['sample_count'])
        self.assertEqual(index.shape[0], self.data._header['sample_count'])
        self.assertIsInstance(index[0], float)

    def test_get_relative_time(self):
        temp = self.data.get_dataframe(time_reference='relative')
        index = temp.index
        self.assertIsInstance(temp, pd.DataFrame)
        self.assertEqual(temp.shape[0], self.data._header['sample_count'])
        self.assertEqual(index.shape[0], self.data._header['sample_count'])
        self.assertIsInstance(index[0], float)

    def test_get_absolute_local_time(self):
        temp = self.data.get_dataframe(time_reference='local')
        index = temp.index
        self.assertIsInstance(temp, pd.DataFrame)
        self.assertEqual(temp.shape[0], self.data._header['sample_count'])
        self.assertEqual(index.shape[0], self.data._header['sample_count'])
        self.assertEqual(index.dtype, np.dtype('<M8[ns]'))

    def test_get_absolute_network_time(self):
        temp = self.data.get_dataframe(time_reference='network')
        index = temp.index
        self.assertIsInstance(temp, pd.DataFrame)
        self.assertEqual(temp.shape[0], self.data._header['sample_count'])
        self.assertEqual(index.shape[0], self.data._header['sample_count'])
        self.assertEqual(index.dtype, np.dtype('<M8[ns]'))


@unittest.skipUnless(os.environ.get("MATPLOTLIB_AVAILABLE") == "true",
                     "requires optional matplotlib dependency")
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
        with self.assertRaises(KeyError):
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


@unittest.skipUnless(os.environ.get("MATPLOTLIB_AVAILABLE") == "true",
                     "requires optional matplotlib dependency")
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
        with self.assertRaises(KeyError):
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
