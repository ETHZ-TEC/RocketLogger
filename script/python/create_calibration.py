#!/usr/bin/env python3
"""
Sample script for generating a RocketLogger calibration file from measurements.

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

from rocketlogger.calibration import RocketLoggerCalibration, CALIBRATION_SETUP_SMU2450


# file input configuration
filename_prefix = "data/test_"

# automatically derive individual measurement file names
calibration_file_v = f"{filename_prefix}calibration_v.rld"
calibration_file_i1l = f"{filename_prefix}calibration_i1l.rld"
calibration_file_i2l = f"{filename_prefix}calibration_i2l.rld"
calibration_file_ih = f"{filename_prefix}calibration_ih.rld"

# calibration output file names
calibration_file = "calibration.dat"
calibration_log_file = "calibration.log"
calibration_file_copy = f"{filename_prefix}calibration.dat"
calibration_log_file_copy = f"{filename_prefix}calibration.log"


# load the calibration measurement files
cal = RocketLoggerCalibration()
cal.load_measurement_data(
    calibration_file_v,
    calibration_file_i1l,
    calibration_file_ih,
    calibration_file_i2l,
    calibration_file_ih,
)

# perform the calibration
cal.recalibrate(CALIBRATION_SETUP_SMU2450)

# print calibration statistics
cal.print_statistics()

# write calibration and log file with and without input file prefix
cal.write_calibration_file(calibration_file)
cal.write_log_file(calibration_log_file)
cal.write_calibration_file(calibration_file_copy)
cal.write_log_file(calibration_log_file_copy)

print("created calibration file.")
