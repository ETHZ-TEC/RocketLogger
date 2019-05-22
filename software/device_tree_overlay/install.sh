#!/bin/bash
# Compile and install the RocketLogger device tree overlay

# clone beaglebone device tree repo
git clone https://github.com/beagleboard/bb.org-overlays.git

# copy RocketLogger device tree overlay and compile binary
cp ROCKETLOGGER.dts bb.org-overlays/src/arm
(cd bb.org-overlays && make src/arm/ROCKETLOGGER.dtbo)

# deploy device tree binary
sudo cp bb.org-overlays/src/arm/ROCKETLOGGER.dtbo /lib/firmware

# switch from rproc to uio PRU interface (see also <https://gist.github.com/jonlidgard/1d9e0e92b4f219f3f40edfed260b851e>)
sudo sed "s~uboot_overlay_pru=/lib/firmware/AM335X-PRU-RPROC~#uboot_overlay_pru=/lib/firmware/AM335X-PRU-RPROC~g" -i /boot/uEnv.txt
sudo sed "s~#uboot_overlay_pru=/lib/firmware/AM335X-PRU-UIO~uboot_overlay_pru=/lib/firmware/AM335X-PRU-UIO~g" -i /boot/uEnv.txt

# enable RocketLogger device tree overlay
sudo sed "s~#dtb_overlay=/lib/firmware/<file8>.dtbo~dtb_overlay=/lib/firmware/ROCKETLOGGER.dtbo~g" -i /boot/uEnv.txt

# PRU shared memory size configuration
sudo cp rocketlogger.conf /etc/modprobe.d/