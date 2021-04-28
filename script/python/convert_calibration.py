#!/usr/bin/env python3
"""
Convert legacy RocketLogger calibration data files to new format.

Copyright (c) 2019-2020, ETH Zurich, Computer Engineering Group
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
