# RocketLogger

The RocketLogger is a mixed-signal data logger designed for measuring and validating energy
harvesting based devices. It features the portability required for measurements in the field,
accurate voltage and current measurements with a very high dynamic range, and simultaneously
sampled digital inputs to track the state of the measured device.


## Getting Started

To install a new RocketLogger system or the Python Support Library for measurement post processing
refer to the [Installation](#installation) section. The latest RocketLogger releases are available
from <https://github.com/ETHZ-TEC/RocketLogger/releases>.

If you want to learn more about the details of the RocketLogger head to the documentation linked
in the [Documentation](#documentation) section.


## Project Structure

The RocketLogger project consists of three parts:
* [RocketLogger Cape](hardware), an analog current and voltage measurement front-end designed as
  extension board, a so-called *Cape*, for the [BeagleBone Green](https://beagleboard.org/green/),
* [RocketLogger Software Stack](software) that provides all management functionality for data
  logging, including low level C API, an command line utility, and an easy-to-use web interface,
* [RocketLogger Scripts](script) that provides scripts to import and process RocketLogger Data
  (RLD) files and to generate the calibration data files.


## Installation

The [RocketLogger releases](https://github.com/ETHZ-TEC/RocketLogger/releases) are accompanies by
a pre-built RocketLogger flasher image that allows installation of ready to use software on a
RocketLogger device:

1. flash the RocketLogger flasher image `rocketlogger-flasher-vX.X.X.img.xz` onto an SD card
   using the `software/system/flash_image.sh` script
2. insert the SD card into the RocketLogger and wait for the installation to complete and the
   RocketLogger to power off
3. Remove the SD card, power on the RocketLogger again, and start your first measurements

For more details on the installation process and alterative deployment options, refer to the
wiki page [RocketLogger Installation](https://github.com/ETHZ-TEC/RocketLogger/wiki/software)

The Python support library is available from the [Python Package Index](https://pypi.org) and 
is installed using pip:
```bash
python -m pip install rocketlogger
```

For the manual installation of individual software components, see the respective README files:
* [RocketLogger software](software/rocketlogger/README.md#installation)
* [Node.js web interface](software/node_server/README.md#installation)
* [Base operating system](software/system/README.md#installation)
* [Python support library](script/python/README.md#installation)


## Documentation

The documentation for the RocketLogger is found in the wiki pages at
<https://github.com/ETHZ-TEC/RocketLogger/wiki>.


## Versioning

The RocketLogger project uses semantic versioning for its releases, see also <https://semver.org>.


## Dependencies

### Hardware Design

The hardware was designed using Altium Designer (version 16.1 was used for the PCB design).

More details regarding the hardware design can be found in the wiki at
[RocketLogger Hardware Design Data](https://github.com/ETHZ-TEC/RocketLogger/wiki/design-data).


### Software Stack

The RocketLogger software is based on the [Debian](https://www.debian.org) operating system for
the BeagleBone platform provided by [BeagleBoard.org](https://beagleboard.org).

To compile the RocketLogger software the following system components are required (to be installed
using `apt-get`):

```bash
curl device-tree-compiler gcc g++ git make meson/stretch-backports ninja-build ntp pkg-config ti-pru-cgt-installer libi2c-dev libncurses5-dev libzmq3-dev
```
with recent meson release from the Debian *stretch-backports* repository.
In addition the software relies on the following software repositories for libgpiod, Linux device tree
headers, and PRU user space driver:
* <https://git.kernel.org/pub/scm/libs/libgpiod/libgpiod.git>
* <https://github.com/beagleboard/bb.org-overlays.git>
* <https://github.com/beagleboard/am335x_pru_package.git>

To deploy the optional Node.js web interface for remote control, additionally relies on the
following system packages:

```bash
g++ make nginx nodejs
```
with the Node.js LTS version 14 from the [NodeSource Node.js](https://github.com/nodesource/distributions#readme)
repository.

The cross compilation and operating system patching scripts in addition required a Docker
installation, more specifically [Docker Buildx](https://docs.docker.com/buildx/working-with-buildx/)
and privileged permissions for system patching.


## Project Contributors

The following list of people contributed in the development of the RocketLogger project:
Andres Gomez,
Dominic Bernath,
Lothar Thiele,
Lukas Sigrist,
Matthias Leubin,
Roman Lim,
Stefan Lippuner.

RocketLogger logo design by: Ivanna Gomez.

The RocketLogger project is currently maintained by: Lukas Sigrist.

## License

The RocketLogger Project is released under [3-clause BSD license](https://opensource.org/licenses/BSD-3-Clause).
For more details refer to the [LICENSE](LICENSE) file.
