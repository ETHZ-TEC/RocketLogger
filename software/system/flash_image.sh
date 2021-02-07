#!/bin/bash
# Flash a BeagleBone image to an SD card
# Usage: ./flash_image.sh <image.img[.xz]> <sdcard-dev>
#
# Copyright (c) 2021, Lukas Sigrist <lsigrist@mailbox.org>
# Copyright (c) 2016-2020, ETH Zurich, Computer Engineering Group
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

# read command line arguments
IMAGE_FILE=$1
SDCARD_DEV=$2


# need to run as root
echo "> Checking root permission"
if [[ $(id -u) -ne 0 ]]; then
  echo "Need to run as root (e.g. using sudo)"
  exit 1
fi

# check arguments
if [ $# -lt 2 ]; then
  echo "Usage: ./flash_image.sh <image.img[.xz]> <sdcard-dev>"
  exit 1
fi

# decompress (if necessary) and write SD card image, sync file system at the end
echo "> Flashing SD card..."
if [[ ${IMAGE_FILE} =~ ^.*.xz$ ]]; then
  xz --decompress --stdout ${IMAGE_FILE} | dd of=${SDCARD_DEV} bs=4M conv=fsync status=progress
else
  dd if=${IMAGE_FILE} of=${SDCARD_DEV} bs=4M conv=fsync status=progress
fi
echo "> Flashing SD card complete."

# hint on the next setup step
echo ">> Remove SD card and insert it into the RocketLogger to continue."
