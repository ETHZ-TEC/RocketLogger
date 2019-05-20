#!/usr/bin/env python3
"""
Automated RocketLogger calibration measurement and generation using SMU2450.

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
from rocketlogger.calibration import RocketLoggerCalibration, CALIBRATION_SETUP_SMU2450

DATA_DIR = '/home/rocketlogger/.config/rocketlogger/'

ROCKETLOGGER_CALIBRATION_V = ('rocketlogger sample 75000 '
    '-r 1000 -ch 2,3,6,7 -fhr 0 -d 0 -a 0 -g 0 -w 0 -c 0 -f "{}" '
    '-C \'RocketLogger voltage calibration measurement using automated SMU2450 sweep.\'')
ROCKETLOGGER_CALIBRATION_I1L = ('rocketlogger sample 75000 '
    '-r 1000 -ch 1 -fhr 0 -d 0 -a 0 -g 0 -w 0 -c 0 -f "{}" '
    '-C \'RocketLogger current I1 low calibration measurement using automated SMU2450 sweep.\'')
ROCKETLOGGER_CALIBRATION_I2L = ('rocketlogger sample 75000 '
    '-r 1000 -ch 5 -fhr 0 -d 0 -a 0 -g 0 -w 0 -c 0 -f "{}" '
    '-C \'RocketLogger current I2 low calibration measurement using automated SMU2450 sweep.\'')
ROCKETLOGGER_CALIBRATION_IH = ('rocketlogger sample 75000 '
    '-r 1000 -ch 0,4 -fhr 1,2 -d 0 -a 0 -g 0 -w 0 -c 0 -f "{}" '
    '-C \'RocketLogger high current calibration measurement using automated SMU2450 sweep.\'')


if __name__ == "__main__":

    # handle first argument
    if len(sys.argv) != 2:
        raise TypeError('need at least one argument')
    action = str(sys.argv[1]).lower()

    # generate filenames
    filename_v = DATA_DIR + '{}_calibration_v.rld'.format(date.today())
    filename_i1l = DATA_DIR + '{}_calibration_i1l.rld'.format(date.today())
    filename_i2l = DATA_DIR + '{}_calibration_i2l.rld'.format(date.today())
    filename_ih = DATA_DIR + '{}_calibration_ih.rld'.format(date.today())
    filename_cal = DATA_DIR + '{}_calibration.dat'.format(date.today())
    filename_log = DATA_DIR + '{}_calibration.log'.format(date.today())

    # for option "cal" perform calibration using todays measurements
    if action == 'cal':
        print('Generating calibration file from measurements.')

        if not os.path.isfile(filename_v):
            raise FileNotFoundError('Missing voltage calibration measurement.')
        elif not os.path.isfile(filename_i1l):
            raise FileNotFoundError('Missing current I1 low calibration measurement.')
        elif not os.path.isfile(filename_i2l):
            raise FileNotFoundError('Missing current I2 low calibration measurement.')
        elif not os.path.isfile(filename_i1l):
            raise FileNotFoundError('Missing current high calibration measurement.')

        # perform calibration and print statistics
        cal = RocketLoggerCalibration(filename_v, filename_i1l, filename_ih,
                                      filename_i2l, filename_ih)
        cal.recalibrate(CALIBRATION_SETUP_SMU2450)
        cal.print_statistics()

        # write calibration file and print statistics
        cal.write_calibration_file(filename_cal)
        cal.write_log_file(filename_log)

    # for option "deploy" install today generated calibration
    elif action == 'deploy':
        print('Deploying generated calibration file.')

        if not os.path.isfile(filename_cal):
            raise FileNotFoundError('Missing calibration file. Generate calibration file first.')

        print('Manual deployment for system wide application needed:')
        print('  sudo cp -f {} /etc/rocketlogger/calibration.dat'.format(filename_cal))
        if os.path.isfile(filename_log):
            print('  sudo cp -f {} /etc/rocketlogger/calibration.log'.format(filename_log))

    # for measurement options conncet to SMU and perform respective measurement
    elif action in ['v', 'i1l', 'i2l', 'ih']:
        print('Running measurement for {action}.')

        # parse hostname argument
        hostname = 'localhost'
        if len(sys.argv) >= 3:
            hostname = sys.argv[2]
        else:
            print('No hostname given as argument #2, assuming localhost')

        # connect SMU
        smu = SMU2450(hostname)
        smu.connect()

        if action == 'v':
            filename = filename_v
            rocketlogger_command = ROCKETLOGGER_CALIBRATION_V.format(filename)
            smu.calibrate_voltage()
        elif action == 'i1l':
            filename = filename_i1l
            rocketlogger_command = ROCKETLOGGER_CALIBRATION_I1L.format(filename)
            smu.calibrate_current_low()
        elif action == 'i2l':
            filename = filename_i2l
            rocketlogger_command = ROCKETLOGGER_CALIBRATION_I2L.format(filename)
            smu.calibrate_current_low()
        elif action == 'ih':
            filename = filename_ih
            rocketlogger_command = ROCKETLOGGER_CALIBRATION_IH.format(filename)
            smu.calibrate_current_high()

        # start rocketlogger measurement
        os.system(rocketlogger_command)

        print('Calibration measurement done.')
        print('Data saved to {}'.format(filename))

        smu.disconnect()

    else:
        print('invalid argument, valid options are: v, i1l, i2l, ih, cal, or deploy')
