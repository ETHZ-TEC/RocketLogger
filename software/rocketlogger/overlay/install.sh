#!/bin/bash
# Compile and install the RocketLogger device tree overlay

# clone beaglebone device tree repo
if [ ! -d bb.org-overlays ]; then
  git clone https://github.com/beagleboard/bb.org-overlays.git
fi

# copy RocketLogger device tree overlay and compile binary
cp -f BB-FLOCKLAB2.dts bb.org-overlays/src/arm
(cd bb.org-overlays && make src/arm/BB-FLOCKLAB2.dtbo)

# deploy device tree binary
sudo cp -f bb.org-overlays/src/arm/BB-FLOCKLAB2.dtbo /lib/firmware

# switch from rproc to uio PRU interface (see also <https://gist.github.com/jonlidgard/1d9e0e92b4f219f3f40edfed260b851e>)
sudo sed "s~^uboot_overlay_pru=/lib/firmware/AM335X-PRU-RPROC~#uboot_overlay_pru=/lib/firmware/AM335X-PRU-RPROC~g" -i /boot/uEnv.txt
sudo sed "s~^#uboot_overlay_pru=/lib/firmware/AM335X-PRU-UIO~uboot_overlay_pru=/lib/firmware/AM335X-PRU-UIO~g" -i /boot/uEnv.txt

# disable HDMI video, HDMI audio, wireless and ADC device tree overlay
sudo sed "s~^#disable_uboot_overlay_video=1~disable_uboot_overlay_video=1~g" -i /boot/uEnv.txt
sudo sed "s~^#disable_uboot_overlay_audio=1~disable_uboot_overlay_audio=1~g" -i /boot/uEnv.txt
sudo sed "s~^#disable_uboot_overlay_wireless=1~disable_uboot_overlay_wireless=1~g" -i /boot/uEnv.txt
sudo sed "s~^#disable_uboot_overlay_adc=1~disable_uboot_overlay_adc=1~g" -i /boot/uEnv.txt

# disable cape universal device tree overlay
sudo sed "s~^enable_uboot_cape_universal=1~#enable_uboot_cape_universal=1~g" -i /boot/uEnv.txt

# enable FlockLab2 device tree overlay
sudo sed "s~^#dtb_overlay=/lib/firmware/<file8>.dtbo~dtb_overlay=/lib/firmware/BB-FLOCKLAB2.dtbo~g" -i /boot/uEnv.txt

# PRU shared memory size configuration
sudo cp -f flocklab2.conf /etc/modprobe.d/
