#!/bin/bash
# RocketLogger device tree overlay configuration

# compile and copy device tree overlay
dtc -O dtb -o ROCKETLOGGER-00A0.dtbo -b 0 -@ ROCKETLOGGER.dts
cp ROCKETLOGGER-00A0.dtbo /lib/firmware

# load device tree overlay at boot
cp capemgr /etc/default/capemgr