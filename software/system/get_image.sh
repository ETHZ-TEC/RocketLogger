#!/bin/bash
# Download the BeagleBone image from the archive
# Usage: ./get_image.sh
#
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
URL_DIRECTORY="https://rcn-ee.com/rootfs/bb.org/testing/2022-10-01/buster-console-armhf/"
IMAGE_FLASHER_FILE="bone-eMMC-flasher-debian-10.13-console-armhf-2022-10-01-1gb.img.xz"
IMAGE_FLASHER_SHA256="c382544a149a9b7a3438cffca40b10f728a712f015b2ef6b3c8c446dcbfd069a"

# download image
curl --continue-at - --remote-name "${URL_DIRECTORY}${IMAGE_FLASHER_FILE}"

# check downloaded file hash
if [[ `echo "${IMAGE_FLASHER_SHA256}  ${IMAGE_FLASHER_FILE}" | sha256sum --check --strict` ]]; then
  echo "SHA256 hash checked successfully."
else
  echo "SHA256 hash verification failed!"
  exit 1
fi
