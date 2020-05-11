#!/bin/bash
#
# Copyright (c) 2019, ETH Zurich, Computer Engineering Group
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
# 
# * Neither the name of the copyright holder nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

#
# Compile and install the RocketLogger device tree overlay
#

# clone beaglebone device tree repo
if [ ! -d bb.org-overlays ]; then
  git clone https://github.com/beagleboard/bb.org-overlays.git
  if [ $? -ne 0 ]; then
    echo "Failed to clone repository bb.org-overlays."
    exit 1
  fi
fi

# copy RocketLogger device tree overlay and compile binary
cp -f BB-FLOCKLAB2.dts bb.org-overlays/src/arm
(cd bb.org-overlays && make src/arm/BB-FLOCKLAB2.dtbo)
if [ $? -ne 0 ]; then
  echo "Failed to compile overlays."
  exit 1
fi

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
#sudo sed "s~^enable_uboot_cape_universal=1~#enable_uboot_cape_universal=1~g" -i /boot/uEnv.txt

# enable FlockLab2 device tree overlay
sudo sed "s~^#dtb_overlay=/lib/firmware/<file8>.dtbo~dtb_overlay=/lib/firmware/BB-FLOCKLAB2.dtbo~g" -i /boot/uEnv.txt

# PRU shared memory size configuration
sudo cp -f flocklab2.conf /etc/modprobe.d/
