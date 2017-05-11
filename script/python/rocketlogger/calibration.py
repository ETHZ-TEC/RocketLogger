"""
RocketLogger Calibration Support.

Handling and generation of RocketLogger calibration files.

Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group

"""

import numpy as np
import matplotlib.pyplot as plt
from scipy import interpolate

from .data import RocketLoggerData


class RocketLoggerCalibration:
    """
    RocketLogger calibration handling class.

    Importing, fitting and generation of RocketLogger calibration data files.
    """

    _data = []
    _filename = None
    _header = {}
    _timestamps_monotonic = []
    _timestamps_realtime = []

    def __init__(self, filename, decimation_factor=1):
        """
        Constructor to create a RockerLoggerData object form data file.

        filename:           The filename of the file to import
        decimation_factor:  Decimation factor for values read (default: 1)
        """
        if filename and isfile(filename):
            self.load_file(filename, decimation_factor)
