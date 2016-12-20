#!/usr/bin/env python3
"""
RocketLogger Data File read demo script
"""

from rocketlogger.rocketlogger import RocketLoggerData


data_file = 'test_data/LWB_total_1ksps.rld'

r = RocketLoggerData(data_file)
