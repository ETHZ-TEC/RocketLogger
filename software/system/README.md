# RocketLogger Base System Setup

There are two alternative ways to setup the RocketLogger system:
1. remotely configuring the booted live system and install the software after a reboot (legacy, one by one)
2. patching the image locally with configuration and software and flash the ready to use image (pre-built image for mass deployment)

## Remote Setup

1. download the image using the `get_image.sh` script
2. insert SD card and flash the downloaded image to it using `flash_image.sh <image.img.xz> <disk-device>`, where `<image.img.xz>` is the downloaded image file and `<disk-device>` the SD card device, typically `/dev/sdX` or `/dev/mmcblkX` (without any partition suffix!)
3. insert the card to the rocketlogger and wait for the image to be copied to internal memory, i.e. until all LED are turned off again.
4. remove the SD card and reboot the system by power cycle
5. reboot and remotely setup using `remote_setup.sh <ip-addr> [<hostname>]`, where `<ip-addr>` is the network address the logger is reachable and `[<hostname>]` an optional hostname to configure during setup.


## Patch BeagleBone System Image

1. use the `patch_image.sh` script from the parent directory to download and patch the BeagleBone flasher image
2. flash the resulting `rocketlogger-*.img` file: insert SD card and flash `rocketlogger-*.img` image to it using `flash_image.sh rocketlogger-*.img <disk-device>`, where `<disk-device>` the SD card device, typically `/dev/sdX` or `/dev/mmcblkX` (without any partition suffix!)
3. insert the card to the rocketlogger and wait for the image to be copied to internal memory, i.e. until all LED are turned off again.
4. remove the SD card and reboot the system by power cycle
