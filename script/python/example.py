#!/usr/bin/env python3
"""
RocketLogger Data File read demo script.

Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group

"""

from rocketlogger.rocketlogger import RocketLoggerFile


data_file = 'test_data/test.rld'

# minimal example
r1 = RocketLoggerFile(data_file)
print(r1._header)
print(r1._data[0].shape)

# with import decimation
r2 = RocketLoggerFile(data_file, decimation_factor=2)
print(r2._header)
print(r2._data[0].shape)
