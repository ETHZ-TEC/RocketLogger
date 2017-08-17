#!/usr/bin/env python3
"""
RocketLogger Data File read demo script.

Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group

"""

from rocketlogger.data import RocketLoggerData


data_file = 'data/test-full.rld'

# minimal example
r = RocketLoggerData(data_file)

# with import decimation
r = RocketLoggerData(data_file, decimation_factor=2)
print(r._header)
print(r._data[0].shape)

# with channel merging
r = RocketLoggerData(data_file)
print(r._header)
print(r._data[0].shape)
r.merge_channels(keep_channels=False)
print(r._header)
print(r._data[0].shape)

# with plotting
r = RocketLoggerData(data_file)
r.plot(['voltages', 'currents'])
r.plot(['digital'])

# straight loading, merging, plotting
rld_merged = RocketLoggerData(data_file).merge_channels()
rld_merged.plot()
