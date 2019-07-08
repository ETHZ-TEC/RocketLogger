import os
import sys
import numpy as np
from rocketlogger.calibration import _CALIBRATION_FILE_DTYPE, \
    _CALIBRATION_FILE_MAGIC, _CALIBRATION_FILE_VERSION, \
    _CALIBRATION_FILE_HEADER_LENGTH


# data format of the calibration file
_LEGACY_FILE_DTYPE = np.dtype([
    ('timestamp', '<M8[s]'),
    ('offset', ('<i4', 8)),
    ('scale', ('<f8', 8)),
])

# old to new channel enumeration mapping
_CHANNEL_REMAPPING = [2, 3, 6, 7, 1, 0, 5, 4]

if __name__ == "__main__":

    # handle the only argument
    if len(sys.argv) != 2:
        raise TypeError(
            'script takes exactly one arugment, the calibraiton to convert')
    filename = str(sys.argv[1])

    # load and rename legacy calibration file
    data_legacy = np.fromfile(filename, dtype=_LEGACY_FILE_DTYPE)

    # assemble new calibraiton file structure
    filedata = np.array([(_CALIBRATION_FILE_MAGIC,
                          _CALIBRATION_FILE_VERSION,
                          _CALIBRATION_FILE_HEADER_LENGTH,
                          data_legacy['timestamp'].squeeze(),
                          data_legacy['offset'].squeeze()[_CHANNEL_REMAPPING],
                          data_legacy['scale'].squeeze()[_CHANNEL_REMAPPING])],
                        dtype=_CALIBRATION_FILE_DTYPE)

    # rename the old calibration file and write new calibration format
    os.rename(filename, filename + '_v1')
    filedata.tofile(filename)
