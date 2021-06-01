"""
SMU2450 remote control for automated RocketLogger calibration

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

# pyvisa packet (wrapper for visa backend: pyvisa-py or NI's NI-VISA binary needs to be installed)
import pyvisa


_COMMAND_SETUP_VOLTAGE_SWEEP = """
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
"""

_COMMAND_SETUP_CURRENT_HIGH_SWEEP = """
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
"""

_COMMAND_SETUP_CURRENT_LOW_SWEEP = """
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
"""

_COMMAND_TRIGGER = """
trigger.model.initiate()
"""

_COMMAND_WAIT_COMPLETE = """
waitcomplete()
"""

_COMMAND_ABORT = """
trigger.model.abort()
"""

_COMMAND_BEEP_START = """
beeper.beep(0.2, 600)
delay(0.3)
beeper.beep(0.2, 600)
"""

_COMMAND_BEEP_END = """
beeper.beep(0.8, 600)
"""

# _COMMAND_READ_BUFFER = """
# printbuffer(1, defbuffer1.n, defbuffer1.sourcevalues, defbuffer1.readings)
# """


class SMU2450:
    """
    SMU2450 remote control utility class for RocketLogger calibration source sweeps.

    :param hostname: the hostname or IP address of the SMU2450 to remote control

    :param port: port number of the SMU's remote control interface
    """

    def __init__(self, hostname, port=5025):
        self.hostname = hostname
        self.port = port
        self.socket_address = f"TCPIP::{self.hostname}::{self.port}::SOCKET"
        self.rm = pyvisa.ResourceManager("@py")

    def connect(self):
        """
        Connect to the SMU remote control interface.
        """
        print(f"Connecting to SMU2450 at: {self.socket_address}")
        self.device = self.rm.open_resource(self.socket_address)
        self.device.clear()
        self.device.timeout = 3000

    def disconnect(self):
        """
        Disconnect from the SMU remote control interface.
        """
        if self.device:
            self.device.clear()
            self.device.close()

    def calibrate_voltage(self):
        """
        Perform a voltage sweep for RocketLogger voltage channel calibration measurement.
        """
        self.setup_voltage_sweep()

        self.beep_start()
        self.start()
        self.wait_complete()
        self.beep_end()

    def calibrate_current_low(self):
        """
        Perform a current sweep for RocketLogger low current channel calibration measurement.
        """
        self.setup_current_low_sweep()

        self.beep_start()

        self.start()
        self.wait_complete()
        self.beep_end()

    def calibrate_current_high(self):
        """
        Perform a current sweep for RocketLogger high current channel calibration measurement.
        """
        self.setup_current_high_sweep()

        self.beep_start()
        self.start()
        self.wait_complete()
        self.beep_end()

    def setup_voltage_sweep(self):
        """
        Set up voltage sweep configuration for RocketLogger voltage channel calibration measurement.
        """
        self.device.write(_COMMAND_SETUP_VOLTAGE_SWEEP)

    def setup_current_low_sweep(self):
        """
        Set up current sweep configuration for RocketLogger low current channel calibration measurement.
        """
        self.device.write(_COMMAND_SETUP_CURRENT_LOW_SWEEP)

    def setup_current_high_sweep(self):
        """
        Set up current sweep configuration for RocketLogger high current channel calibration measurement.
        """
        self.device.write(_COMMAND_SETUP_CURRENT_HIGH_SWEEP)

    def start(self):
        """
        Trigger start of the preconfigured sweep.
        """
        self.device.write(_COMMAND_TRIGGER)

    def stop(self):
        """
        Stop or abbort a running sweep.
        """
        self.device.write(_COMMAND_ABORT)

    # def read_buffer(self):
    #     """
    #     Read back the source and measurement buffers of the last sweep.
    #     """
    #     return self.device.query(_COMMAND_READ_BUFFER)

    def wait_complete(self):
        """
        Blocking wait for completion of a running sweep.
        """
        self.device.write(_COMMAND_WAIT_COMPLETE)

    def beep_start(self):
        """
        Signal start of the sweep consisting of two short beeps.
        """
        self.device.write(_COMMAND_BEEP_START)

    def beep_end(self):
        """
        Signal completion of the sweep consisting of one long beep.
        """
        self.device.write(_COMMAND_BEEP_END)
