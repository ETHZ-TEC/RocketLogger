"""
SMU2450 remote control for automated RocketLogger calibration

Copyright (c) 2019, ETH Zurich, Computer Engineering Group
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

# pyvisa packet (wrapper for visa backend: pyvisa-py or NI's NI-VISA binary needs to be installed)
import visa


_COMMAND_SETUP_VOLTAGE_SWEEP = '''
reset()
smu.source.func=smu.FUNC_DC_VOLTAGE
smu.measure.func=smu.FUNC_DC_VOLTAGE
smu.measure.autorange = smu.ON
smu.measure.sense = smu.SENSE_2WIRE
smu.measure.terminals = smu.TERMINALS_FRONT

smu.source.autorange = smu.ON
smu.source.ilimit.level = 1e-3
smu.source.sweeplinear("cal_v", -5, 5, 101, 250e-3, 1, smu.RANGE_AUTO, smu.ON, smu.ON, defbuffer1)

defbuffer1.clear()
'''

_COMMAND_SETUP_CURRENT_HIGH_SWEEP = '''
reset()
smu.source.func=smu.FUNC_DC_CURRENT
smu.source.autorange = smu.ON
smu.source.vlimit.level = 1
smu.source.sweeplinear("cal_ih", -100e-3, 100e-3, 101, 250e-3, 1, smu.RANGE_AUTO, smu.ON, smu.ON, defbuffer1)

smu.measure.func=smu.FUNC_DC_VOLTAGE
smu.measure.autorange = smu.OFF
smu.measure.range = 2
smu.measure.sense = smu.SENSE_2WIRE
smu.measure.terminals = smu.TERMINALS_FRONT

defbuffer1.clear()
'''

_COMMAND_SETUP_CURRENT_LOW_SWEEP = '''
reset()
smu.source.func=smu.FUNC_DC_CURRENT
smu.source.autorange = smu.ON
smu.source.vlimit.level = 1
smu.source.sweeplinear("cal_il", -1e-3, 1e-3, 101, 250e-3, 1, smu.RANGE_AUTO, smu.ON, smu.ON, defbuffer1)

smu.measure.func=smu.FUNC_DC_VOLTAGE
smu.measure.autorange = smu.OFF
smu.measure.range = 2
smu.measure.sense = smu.SENSE_2WIRE
smu.measure.terminals = smu.TERMINALS_FRONT

defbuffer1.clear()
'''

_COMMAND_TRIGGER = '''
trigger.model.initiate()
'''

_COMMAND_WAIT_COMPLETE = '''
waitcomplete()
'''

_COMMAND_ABORT = '''
trigger.model.abort()
'''

_COMMAND_BEEP_START = '''
beeper.beep(0.2, 600)
delay(0.3)
beeper.beep(0.2, 600)
'''

_COMMAND_BEEP_END = '''
beeper.beep(0.8, 600)
'''

_COMMAND_READ_BUFFER = '''
printbuffer(1, defbuffer1.n, defbuffer1.sourcevalues, defbuffer1.readings)
'''


class SMU2450():
    """
    Control the SMU2450 for the calibration source sweeps.
    """

    def __init__(self, hostname, port=5025):
        self.hostname = hostname
        self.port = port
        self.socket_address = 'TCPIP::{}::{}::SOCKET'.format(self.hostname,
                                                             self.port)
        self.rm = visa.ResourceManager('@py')

    def connect(self):
        print('Connecting SMU2450 at: {}'.format(self.socket_address))
        self.device = self.rm.open_resource(self.socket_address)
        self.device.clear()
        self.device.timeout = 3000

    def disconnect(self):
        if self.device:
            self.device.clear()
            self.device.close()

    def calibrate_voltage(self):
        self.setup_voltage_sweep()

        self.beep_start()
        self.start()
        self.wait_complete()
        self.beep_end()

    def calibrate_current_low(self):
        self.setup_current_low_sweep()

        self.beep_start()

        self.start()
        self.wait_complete()
        self.beep_end()

    def calibrate_current_high(self):
        self.setup_current_high_sweep()

        self.beep_start()
        self.start()
        self.wait_complete()
        self.beep_end()

    def setup_voltage_sweep(self):
        self.device.write(_COMMAND_SETUP_VOLTAGE_SWEEP)

    def setup_current_low_sweep(self):
        self.device.write(_COMMAND_SETUP_CURRENT_LOW_SWEEP)

    def setup_current_high_sweep(self):
        self.device.write(_COMMAND_SETUP_CURRENT_HIGH_SWEEP)

    def start(self):
        self.device.write(_COMMAND_TRIGGER)

    def stop(self):
        self.device.write(_COMMAND_ABORT)

    # def read_buffer(self):
    #     return self.device.query(_COMMAND_READ_BUFFER)

    def wait_complete(self):
        self.device.write(_COMMAND_WAIT_COMPLETE)

    def beep_start(self):
        self.device.write(_COMMAND_BEEP_START)

    def beep_end(self):
        self.device.write(_COMMAND_BEEP_END)
