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
 * Altium Designer installation (version 16 was used for the PCB design)

### Software Installation
 * To compile the RocketLogger software the following system components are requred (install using `apt`):
```
ntp gcc libncurses5-dev libi2c-dev
```
 * Additionally the PRU compiler for comiling the low level functions is required.
   If you use an official BeagleBone image it comes pre-installed,
   otherwise you have to follow the instructions on [PRU compiler (TBD)](https://beagleboard.org/)
 * For using the remote web interface these additional systems components will be needed:
```
lighttp php5-cgi
```

For further details regarding software installation and system configuration check the [RocketLogger Software Stack](software) page.
