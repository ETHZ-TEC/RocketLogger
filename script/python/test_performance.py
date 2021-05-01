#!/usr/bin/env python3
"""
RocketLogger data file processing performance profiling.

Copyright (c) 2017-2020, ETH Zurich, Computer Engineering Group
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

import cProfile
import os
from rocketlogger.data import RocketLoggerData


# data files (need to first generate some using the highest sampling rate)
file_100mb = os.path.join("performance", "data.rld")
file_10gb = os.path.join("performance", "test-large.rld")


# profiler output config
sorting = "cumulative"


# profile 1
print("=== Profile loading file: 8x100MB, memory mapped ===")
profiler = cProfile.Profile()
profiler.enable()

r = RocketLoggerData(file_100mb, memory_mapped=True)

profiler.disable()
profiler.print_stats(sort=sorting)


# profile 2
print("=== Profile loading file: 8x100MB, memory mapped, decimate 10 ===")
profiler = cProfile.Profile()
profiler.enable()

r = RocketLoggerData(file_100mb, memory_mapped=True, decimation_factor=10)

profiler.disable()
profiler.print_stats(sort=sorting)


# profile 3
print("=== Profile loading file: 8x100MB, direct ===")
profiler = cProfile.Profile()
profiler.enable()

r = RocketLoggerData(file_100mb, memory_mapped=False)

profiler.disable()
profiler.print_stats(sort=sorting)


# profile 4
print("=== Profile loading file: 8x100MB, direct, decimate 10 ===")
profiler = cProfile.Profile()
profiler.enable()

r = RocketLoggerData(file_100mb, memory_mapped=False, decimation_factor=10)

profiler.disable()
profiler.print_stats(sort=sorting)


# profile 5
print("=== Profile loading file: 1x10GB, memory mapped ===")
profiler = cProfile.Profile()
profiler.enable()

r = RocketLoggerData(file_10gb, memory_mapped=True)

profiler.disable()
profiler.print_stats(sort=sorting)


# profile 6
print("=== Profile loading file: 1x10GB, memory mapped, decimate 10 ===")
profiler = cProfile.Profile()
profiler.enable()

r = RocketLoggerData(file_10gb, memory_mapped=True, decimation_factor=10)

profiler.disable()
profiler.print_stats(sort=sorting)


# profile 7
print("=== Profile loading file: 1x10GB, memory mapped, decimate 6400 ===")
profiler = cProfile.Profile()
profiler.enable()

r = RocketLoggerData(file_10gb, memory_mapped=True, decimation_factor=6400)

profiler.disable()
profiler.print_stats(sort=sorting)


# profile 8
print("=== Profile loading file: 1x10GB, direct ===")
profiler = cProfile.Profile()
profiler.enable()

r = RocketLoggerData(file_10gb, memory_mapped=False)

profiler.disable()
profiler.print_stats(sort=sorting)

# profile 9
print("=== Profile loading file: 1x10GB, direct, decimate 10 ===")
profiler = cProfile.Profile()
profiler.enable()

r = RocketLoggerData(file_10gb, memory_mapped=False, decimation_factor=10)

profiler.disable()
profiler.print_stats(sort=sorting)


# profile 10
print("=== Profile loading file: 1x10GB, direct, decimate 6400 ===")
profiler = cProfile.Profile()
profiler.enable()

r = RocketLoggerData(file_10gb, memory_mapped=False, decimation_factor=6400)

profiler.disable()
profiler.print_stats(sort=sorting)
