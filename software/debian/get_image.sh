#!/bin/bash
# Download the beagle bone image from the beaglebone archive
#
# Copyright (c) 2016-2019, ETH Zurich, Computer Engineering Group
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

# Debian buster IoT image
URL_DIRECTORY="https://rcn-ee.com/rootfs/bb.org/testing/2019-08-25/buster-console/"
IMAGE_FILE="bone-debian-10.0-console-armhf-2019-08-25-1gb.img.xz"
IMAGE_SHA256="c38b6d02949c4143411694e82f5f944e2536b3594ae10ceacce4f1c192796669"

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
