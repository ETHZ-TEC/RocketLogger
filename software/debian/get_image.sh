#!/bin/bash
# Download the beagle bone image from the beaglebone archive
#
# Copyright (c) 2016-2019, Swiss Federal Institute of Technology (ETH Zurich)
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

# latest console testing image
URL_DIRECTORY="https://rcn-ee.com/rootfs/bb.org/testing/2018-11-25/stretch-console/"
IMAGE_FILE="bone-debian-9.6-console-armhf-2018-11-25-1gb.img.xz"
IMAGE_SHA256="86143c5a21be3452436bae89d2f5cc3c9c3512545546b43e81769329aab3ff9a"

# # latest iot official release
# URL_DIRECTORY="https://debian.beagleboard.org/images/"
# IMAGE_FILE="bone-debian-9.5-iot-armhf-2018-10-07-4gb.img.xz"
# IMAGE_SHA256="52363c654b7a1187656b08c5686af7564c956d6c60c7df5bf4af098f7df395e0"

# download image
wget --progress=bar "$URL_DIRECTORY$IMAGE_FILE"

# check downloaded file hash
SHA=`sha256sum "$IMAGE_FILE" | awk '{print $1}'`
if [[ "$SHA" == "$IMAGE_SHA256" ]]; then
  echo "SHA256 hash checked successfully."
  exit 0
else
  echo "SHA256 hash verification failed!"
  exit 1
fi
