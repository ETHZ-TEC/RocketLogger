# RocketLogger Base System Setup

The RocketLogger system relies on Debian as base operating system, with dedicated system
configuration and package installation. This software component provides the necessary
scripts to setup and configure the operating system.


## Installation
There are two alternative ways to setup the RocketLogger system:
1. remotely configuring the booted live system and install the software after a reboot (legacy
   installation to deploy system, requiring individual deployment)
2. patching the image locally with configuration and software and flash the ready to use image
   (pre-built image for mass deployment of many devices)

It is highly recommended to use the external SD card for configuration and measurement data
storage. This provides more storage for data and prevents the system from getting unresponsive
due to running out of system disk space.


### Remote Setup

1. download the image using the `get_image.sh` script
2. insert the SD card into your computer and flash the downloaded image to it using
   `flash_image.sh <image.img.xz> <disk-device>`, where `<image.img.xz>` is the downloaded image
   file and `<disk-device>` the SD card device, typically `/dev/sdX` or `/dev/mmcblkX` (without
   any partition suffix!)
3. insert the SD card into the RocketLogger device and power it up
4. wait for the image to be copied to internal memory, i.e. until all LED are turned off again
5. remove the SD card and reboot the system by power cycle
6. reboot and remotely setup using `remote_setup.sh <ip-addr> [<hostname>]`, where `<ip-addr>` is
   the network address the logger is reachable and `[<hostname>]` an optional hostname to
   configure during setup.
7. optionally, but highly recommended, set up the external SD card as configuration and
   measurement data storage as described below
8. power up the RocketLogger and start your measurements.


### Patch BeagleBone System Image

1. (skip this step when using a prebuilt RocketLogger image) use the `patch_image.sh` script from 
   the parent directory to download and patch the BeagleBone flasher image (requires Docker Buildx)
3. flash the resulting `rocketlogger-*.img` file: insert the SD card into your computer and flash
   the `rocketlogger-*.img` image to it using `flash_image.sh rocketlogger-*.img <disk-device>`,
   where `<disk-device>` the SD card device, typically `/dev/sdX` or `/dev/mmcblkX` (without any
   partition suffix!)
4. insert the SD card into the RocketLogger device and power it up
5. wait for the image to be copied to internal memory, i.e. until all LED are turned off again
6. remove the SD card and reboot the system
7. optionally, but highly recommended, set up the external SD card as configuration and
   measurement data storage as described below
8. power up the RocketLogger and start your measurements.


### Set up External SD Card for Data and Configuration Storage

1. (re-)insert the SD card into your computer and prepare the RocketLogger SD card using
   `prepare_sdcard.sh <disk-device>`, where `<disk-device>` is the SD card device, typically
   `/dev/sdX` or `/dev/mmcblkX` (without any partition suffix!)
2. copy any existing configuration or calibration files into the `rocktlogger/config` folder
   on the newly set up SD card
3. Insert the SD card into the RocketLogger, reboot, and start your measurements


## Dependencies

The operation system is based on the minimal console images for the BeagleBone platform, provided
by [BeagleBoard.org](https://beagleboard.org). The latest official BeagleBone images are available
at <https://beagleboard.org/latest-images>.

The system installation scripts itself rely on Linux system utility binaries and partitioning and
filesystem tools that are typically included with any basic Linux based operating system.
Tools that might not come preinstalled with your favorite Linux operating system installation are:
`curl`, `dd`, `git`, `xz`

The local patching of a BeagleBone system image requires `Docker Buildx` and a system configured
to run privileged Docker containers. More information on the docker configuration is found at
[Docker Buildx](https://docs.docker.com/buildx/working-with-buildx/) and in the
[docker run reference](https://docs.docker.com/engine/reference/run/#runtime-privilege-and-linux-capabilities).
