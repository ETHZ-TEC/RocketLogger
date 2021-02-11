#!/bin/bash
# Download the beagle bone image from the beaglebone archive
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

# Debian buster console image
URL_DIRECTORY="https://rcn-ee.com/rootfs/bb.org/testing/2021-02-08/buster-console/"
IMAGE_FLASHER_FILE="bone-eMMC-flasher-debian-10.8-console-armhf-2021-02-08-1gb.img.xz"
IMAGE_FLASHER_SHA256="d3341ccb1a95cdf49ee7e3d7d824b94d971cfff1ed940bdc36870e64f379ce23"

# download image
wget --continue --progress=bar "${URL_DIRECTORY}${IMAGE_FLASHER_FILE}"

# check downloaded file hash
SHA=`sha256sum "${IMAGE_FLASHER_FILE}" | awk '{print $1}'`
if [[ "$SHA" != "${IMAGE_FLASHER_SHA256}" ]]; then
  echo "SHA256 hash verification failed!"
  exit 1
else
  echo "SHA256 hash checked successfully."
fi
