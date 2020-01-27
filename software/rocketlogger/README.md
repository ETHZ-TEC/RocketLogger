# RocketLogger Software Documentation

The RocketLogger software includes the three following binaries, which make use of the RocketLogger library.


### RocketLogger Binary

Main RocketLogger binary defined in [rocketlogger.c](@ref rocketlogger.c). Provides an extensive command line interface to control an monitor the sampling.


### RocketLogger Daemon

RocketLogger daemon program defined in [rocketloggerd.c](@ref rocketloggerd.c). Initializes the user space GPIOs, controls the power supply of the cape, and handles the user button interrupt to start and stop RocketLogger measurements.


### RocketLogger Server

RocketLogger server program defined in [rl_server.c](@ref rl_server.c). Returns status and current sampling data (if available) when running and default configuration otherwise, for use in a webserver.


## Dependencies

### Build Tools

For building the software the following build tools are required:

- GNU Make - `make`
- GNU C compiler - `gcc`
- GNU C++ compiler - `g++`
- TI PRU Assembler - install from source <https://github.com/beagleboard/am335x_pru_package.git>


### Software Libraries

The following libraries are required to build the software (and the corresponding Debian package):

- `libi2c` - `libi2c-dev` 
- `libncurses` - `libncurses5-dev`
- `librt` - part of libc
- `prussdrv` - install from source <https://github.com/beagleboard/am335x_pru_package.git>
- Linux headers - `linux-headers-$(uname -r)`


## License

```
Copyright (c) 2016-2020, ETH Zurich, Computer Engineering Group
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
```
