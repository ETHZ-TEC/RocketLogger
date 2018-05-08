RocketLogger
============
<https://rocketlogger.ethz.ch>


The RocketLogger is a mixed-signal data logger designed for measuring and validating energy harvesting based devices.
It features the portability required for measurements in the field,
accurate voltage and current measurements with a very high dynamic range,
and simultaneously sampled digital inputs to track the state of the measured device.


Project Organization
--------------------

The official RocketLogger website can be found at <https://rocketlogger.ethz.ch>

The mailing list for the RocketLogger project is *rocketlogger* (at) *list.ee.ethz.ch*. Subscribe for project updates at <https://lists.ee.ethz.ch/wws/info/rocketlogger>.

The documentation of all project components is found in the [RocketLogger Wiki](https://gitlab.ethz.ch/tec/public/rocketlogger/wikis/home).


Project Structure
-----------------

The project consists of three parts:
* The [RocketLogger Cape](hardware), an analog current and voltage measurement front-end designed as extension board ("Cape") for the [BeagleBone Green](https://beagleboard.org/green/),
* The [RocketLogger Software Stack](software) that provides all management functionality for data logging, including low level C API, an command line utility, and an easy-to-use web interface.
* The [RocketLogger Scripts](script) that provides scripts to import and process RocketLogger Data (RLD) files and to generate the calibration data files.

The detailed documentation of the project and its individual parts can be found in the [RocketLogger Wiki](https://git.ee.ethz.ch/sigristl/rocketlogger/wikis/).


Prerequisites
-------------

#### Hardware Design
 * Altium Designer installation (version 16.1 was used for the PCB design)

More details regarding the hardware design can be found on the [RocketLogger Hardware Design Data](https://git.ee.ethz.ch/sigristl/rocketlogger/wikis/design-data) wiki page.


#### Software Installation

 * To compile the RocketLogger software the following system components are required (install using `apt-get`):
```
git make gcc g++ ti-pru-cgt-installer device-tree-compiler am335x-pru-package ntp libncurses5-dev libi2c-dev
```
 * Additionally the linux header for the Linux kernel verison used on the BeagleBone should be installed.
   they can be installed using the follwing command on the target system:
```
apt-get install linux-headers-$(uname -r)
```
 * The low level functions to communicate with the Cape make use of the Programmable Real-Time Unit (PRU).
   A PRU compiler and application loader library are required to compile and deploy this functionality.
   The official BeagleBone image comes with these tools pre-installed.
   Otherwise the compiler can be downloaded at [PRU Code Generation Tools](http://software-dl.ti.com/codegen/non-esd/downloads/download.htm#PRU)
   or installed as add-on for the [Code Composer Studio](http://processors.wiki.ti.com/index.php/Download_CCS).
   The instructions how to install the PRUSSDRV User Space Library are found at [PRU Linux Application Loader](http://processors.wiki.ti.com/index.php/PRU_Linux_Application_Loader).
 * For using the remote web interface these additional system components will be needed:
```
apache2 lighttpd php5-cgi unzip
```

If you follow the installation guide on the [RocketLogger Software Installation](https://git.ee.ethz.ch/sigristl/rocketlogger/wikis/software) wiki page, all necessary dependencies are installed during that process.


License
-------

The RocketLogger Project is released under [3-clause BSD license](https://opensource.org/licenses/BSD-3-Clause). For more details please refer the the [LICENSE](LICENSE) file.


Contributors
------------

The RocketLogger was developed at the [Computer Engineering Group](http://www.tec.ethz.ch/) at [ETH Zurich](https://www.ethz.ch/en.html).
The following people contributed to the design and implementation of this project:

#### Core Team (alphabetical order)

* [Andres Gomez](mailto:andres.gomez@tik.ee.ethz.ch)
* Matthias Leubin
* [Roman Lim](http://www.tik.ee.ethz.ch/~rlim/)
* Stefan Lippuner
* [Lukas Sigrist](mailto:lukas.sigrist@tik.ee.ethz.ch)
* [Lothar Thiele](http://www.tik.ee.ethz.ch/~thiele/)


#### Case Design

* Dominic Bernath


#### RocketLogger Logo

* Ivanna Gomez


#### Contact

Lukas Sigrist *lukas.sigrist* (at) *tik.ee.ethz.ch*
