import os
import sys
import numpy as np
from rocketlogger.calibration import (
    _CALIBRATION_FILE_DTYPE,
    _CALIBRATION_FILE_MAGIC,
    _CALIBRATION_FILE_VERSION,
    _CALIBRATION_FILE_HEADER_LENGTH,
    _CALIBRATION_PRU_CYCLES_OFFSET,
    _CALIBRATION_PRU_CYCLES_SCALE,
)


# data format of the calibration file
_LEGACY_FILE_DTYPE = np.dtype(
    [
        ("timestamp", "<datetime64[s]"),
        ("offset", ("<i4", 8)),
        ("scale", ("<f8", 8)),
    ]
)

# old to new channel enumeration mapping
_CHANNEL_REMAPPING = [2, 3, 6, 7, 1, 0, 5, 4]

if __name__ == "__main__":

    # handle the only argument
    if len(sys.argv) != 2:
        raise TypeError("script takes exactly one argument, the calibration to convert")
    filename = str(sys.argv[1])

    # load and rename legacy calibration file
    data_legacy = np.fromfile(filename, dtype=_LEGACY_FILE_DTYPE)

    # assemble new calibration file structure
    offset_new = np.concatenate(
        (
            data_legacy["offset"].squeeze()[_CHANNEL_REMAPPING],
            [_CALIBRATION_PRU_CYCLES_OFFSET],
        )
    )
    scale_new = np.concatenate(
        (
            data_legacy["scale"].squeeze()[_CHANNEL_REMAPPING],
            [_CALIBRATION_PRU_CYCLES_SCALE],
        )
    )
    filedata = np.array(
        [
            (
                _CALIBRATION_FILE_MAGIC,
                _CALIBRATION_FILE_VERSION,
                _CALIBRATION_FILE_HEADER_LENGTH,
                data_legacy["timestamp"].squeeze(),
                offset_new,
                scale_new,
            )
        ],
        dtype=_CALIBRATION_FILE_DTYPE,
    )

    # rename the old calibration file and write new calibration format
    os.rename(filename, filename + "_v1")
    filedata.tofile(filename)
