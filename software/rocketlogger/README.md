# RocketLogger Software Documentation


## Component Overview

The RocketLogger software consists of multiple components that are summarized below.

### RocketLogger Binary

The RocketLogger tool is implemented in @ref rocketlogger.c.
This tool provides an extensive command line interface to control and monitor the sampling.


### RocketLogger Daemon

The RocketLogger service daemon is implemented in @ref rocketloggerd.c.
This service is responsible for configuring the user space GPIOs, control the power supply of the
cape, handles the user button actions and observes the RocketLogger sampling status.


### PRU Firmware

The firmware sources for the Programmable Real-time Unit (PRU) are located in the @ref pru
subfolder. The firmware is built and installed along with the rest of the software.

### Device Tree Overlay

The necessary device tree overlay sources for the RocketLogger Cape hardware are contained in the
@ref overlay subfolder and installed together with the above software components (see also @ref overlay/README.md).


### System Configuration

The system configuration files provided in the @ref config subfolder provide the necessary
`uio_pruss` module configuration, uEnv boot configuration, and RocketLogger service specification,
as well as as default calibration parameters. These configuration files are deployed together with
the other software components.


## Installation

To build and install the RocketLogger software components and its configuration use:

```bash
meson builddir
cd builddir
ninja
sudo meson install --no-rebuild
```


## Documentation

The documentation for the RocketLogger is found in the wiki pages at
<https://github.com/ETHZ-TEC/RocketLogger/wiki>.


## Dependencies

### Build Tools

For building the software, the following build tools are required (provided by the listed Debian
packages):

* *Linux device tree* compiler - `device-tree-compiler` package
* *GNU C* compiler - `gcc` package
* *Meson* build system (version >= 0.55) - `meson/buster-backports` package
* *ninja* build system (version >= 0.10) - `ninja-build/buster-backports` package
* *TI PRU* code generation tools - `ti-pru-cgt-installer` package


#### Cross Compilation
If not developing directly on the BeagleBone, there is the option to cross compile using
`Docker Buildx`. The `build_cross.sh` helper script and the
required `Dockerfile` can be used for this purpose.
To use this cross-compilation option, `Docker Buildx` and a system configured to run
privileged Docker containers is required. More information on the docker configuration is
found at [Docker Buildx](https://docs.docker.com/buildx/working-with-buildx/) and in the
[docker run reference](https://docs.docker.com/engine/reference/run/#runtime-privilege-and-linux-capabilities).


### Software Libraries

The following libraries (provided by the corresponding Debian packages) are required to build
the software:

* *libgpiod* - `libgpiod-dev` package
* *libi2c* - `libi2c-dev` package
* *libncurses* - `libncurses5-dev` package
* *libzeromq* - `libzmq3-dev` package
* BeagleBone device tree overlay headers - included as a [Meson subproject](https://github.com/beagleboard/bb.org-overlays.git)
* Linux PRU user space driver *prussdrv* - included as a [Meson subproject](https://github.com/beagleboard/am335x_pru_package.git)


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
