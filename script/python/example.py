#!/usr/bin/env python3
"""
RocketLogger Data File read demo script.

Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group

"""

from rocketlogger.rocketlogger import RocketLoggerFile


data_file = 'test_data/test.rld'

# minimal example
r = RocketLoggerFile(data_file)

# with import decimation
r = RocketLoggerFile(data_file, decimation_factor=2)
print(r._header)
print(r._data[0].shape)

# with channel merging
r = RocketLoggerFile(data_file)
print(r._header)
print(r._data[0].shape)
r.merge_channels(keep_channels=False)
print(r._header)
print(r._data[0].shape)

# with plotting
r = RocketLoggerFile(data_file)
r.plot(['voltages', 'currents'])
r.plot(['digital'])

# straight loading, merging, plotting
RocketLoggerFile(data_file).merge_channels().plot()
