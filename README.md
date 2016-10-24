RocketLogger
============

The RocketLogger is a data logger that...
* is portable
* has high dynamic-range current measurement
* environmental logging
* supports remote-control for long-term measurements
* is easily extensible with new features
* also provides much more...


Project Organization
--------------------
The project consists of two parts
* The [RocketLogger Cape](hardware), an analog current and voltage measurement front-end designed as extension board ("Cape") for the [BeagleBone Green](https://beagleboard.org/green/),
* The [RocketLogger Software Stack](software) that provides all management functionality for data logging, including low level C API, an command line utility, and an easy-to-use web interface.


Prerequisites
-------------

### Hardware Design
 * Altium Designer installation (version 16.1 was used for the PCB design)

More details regarding the hardware design can be found on the [RocketLogger Hardware](wikis/hardware) wiki page.

### Software Installation
 * To compile the RocketLogger software the following system components are required (install using `apt`):
   `ntp gcc libncurses5-dev libi2c-dev clang`
 * Additionally the linux header for the Linux kernel verison used on the BeagleBone should be installed.
   they can be installed using the follwing command on the target system:
```
apt install linux-headers-$(uname -r)
```
 * The low level functions to communicate with the Cape make use of the Programmable Real-Time Unit (PRU).
   A PRU compiler and application loader library are required to compile and deploy this functionality.
   The official BeagleBone image comes with these tools pre-installed (TODO: check if valid for PRUSSDRV).
   Otherwise the compiler can be downloaded at [PRU Code Generation Tools](http://software-dl.ti.com/codegen/non-esd/downloads/download.htm#PRU)
   or install them as add-on for the [Code Composer Studio](http://processors.wiki.ti.com/index.php/Download_CCS).
   The instructions how to install the PRUSSDRV User Space Library are found at [PRU Linux Application Loader](http://processors.wiki.ti.com/index.php/PRU_Linux_Application_Loader).
 * For using the remote web interface these additional systems components will be needed:
```
lighttp php5-cgi
```

For further details regarding software installation and system configuration check the [RocketLogger Software Stack](wikis/software) wiki page.
