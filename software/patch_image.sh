#!/bin/bash
# Patch a BeagleBone image to install the RocketLogger system
#
# Copyright (c) 2021, Lukas Sigrist <lsigrist@mailbox.org>
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

# register ARM executables
docker run --rm --privileged docker/binfmt:a7996909642ee92942dcd6cff44b9b95f08dad64

# inspect existing builder
docker buildx inspect --bootstrap

# build minimal armv7 debian image with compiler tools
docker buildx build --platform linux/arm/v7 -t beaglebone_builder .

# change to system folder to download and decompress base system image
cd system
source ./get_image.sh
xz --decompress --keep --force "${IMAGE_FLASHER_FILE}"
IMAGE=`basename "${IMAGE_FLASHER_FILE}" ".xz"`
cd ..

# build rocketlogger binary
docker run --privileged --platform linux/arm/v7  \
    --mount type=bind,source="$(pwd)/rocketlogger",target=/home/rocketlogger  \
    --mount type=bind,source="$(pwd)/system",target=/home/system  \
    --mount type=bind,source="$(pwd)/node_server",target=/home/node_server  \
    --tty beaglebone_builder /bin/bash -c "cd /home/system && ./chroot_setup.sh ${IMAGE}"  \
  | tee "$(pwd)/system/patch_image.log"
