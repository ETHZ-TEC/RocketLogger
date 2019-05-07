#!/usr/bin/env python3
"""
RocketLogger Data File read demo script.

Copyright (c) 2016-2019, Swiss Federal Institute of Technology (ETH Zurich)
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
from datetime import date

from calibration.smu import SMU2450

# update with the IP or address of the SMU2450 device
SMU2450_HOSTNAME = 'localhost'

DATA_DIR = '/home/rocketlogger/.config/rocketlogger/'

ROCKETLOGGER_CALIBRATION_V = ('rocketlogger sample 75000 '
    '-r 1000 -ch 2,3,6,7 -fhr 0 -d 0 -a 0 -g 0 -w 0 -f "{}" '
    '-C \'RocketLogger voltage calibration measurement using automated SMU2450 sweep.\'')
ROCKETLOGGER_CALIBRATION_I1L = ('rocketlogger sample 75000 '
    '-r 1000 -ch 1 -fhr 0 -d 0 -a 0 -g 0 -w 0 -f "{}" '
    '-C \'RocketLogger current I1 low calibration measurement using automated SMU2450 sweep.\'')
ROCKETLOGGER_CALIBRATION_I2L = ('rocketlogger sample 75000 '
    '-r 1000 -ch 5 -fhr 0 -d 0 -a 0 -g 0 -w 0 -f "{}" '
    '-C \'RocketLogger current I2 low calibration measurement using automated SMU2450 sweep.\'')
ROCKETLOGGER_CALIBRATION_IH = ('rocketlogger sample 75000 '
    '-r 1000 -ch 0,4 -fhr 1,2 -d 0 -a 0 -g 0 -w 0 -f "{}" '
    '-C \'RocketLogger high current calibration measurement using automated SMU2450 sweep.\'')


if __name__ == "__main__":
    smu = SMU2450(SMU2450_HOSTNAME)
    smu.connect()

    if len(sys.argv) != 2:
        raise TypeError('need exactly one argument')
    
    # create data dir if inexistent
    # os.makedirs(DATA_DIR, exist_ok=True)

    if str(sys.argv[1]).lower() == 'v':
        filename = DATA_DIR + '{}_calibration_v.rld'.format(date.today())
        rocketlogger_command = ROCKETLOGGER_CALIBRATION_V.format(filename)

        smu.calibrate_voltage()

    if str(sys.argv[1]).lower() == 'i1l':
        filename = DATA_DIR + '{}_calibration_i1l.rld'.format(date.today())
        rocketlogger_command = ROCKETLOGGER_CALIBRATION_I1L.format(filename)

        smu.calibrate_current_low()

    if str(sys.argv[1]).lower() == 'i2l':
        filename = DATA_DIR + '{}_calibration_i2l.rld'.format(date.today())
        rocketlogger_command = ROCKETLOGGER_CALIBRATION_IL.format(filename)

        smu.calibrate_current_low()

    if str(sys.argv[1]).lower() == 'ih':
        filename = DATA_DIR + '{}_calibration_ih.rld'.format(date.today())
        rocketlogger_command = ROCKETLOGGER_CALIBRATION_IH.format(filename)

        smu.setup_current_high_sweep()

    print(rocketlogger_command)
    # os.system(rocketlogger_command)

    print('Calibration measurement done.')
    print('Data saved to {}'.format(filename))
    
    smu.disconnect()
