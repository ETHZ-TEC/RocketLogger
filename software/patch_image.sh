#!/bin/bash
# patch a BeagleBone image to install the RocketLogger system

# register ARM executables
docker run --rm --privileged docker/binfmt:a7996909642ee92942dcd6cff44b9b95f08dad64

# inspect existing builder
docker buildx inspect --bootstrap

# build minimal armv7 debian image with compiler tools
docker buildx build --platform linux/arm/v7 -t beaglebone_builder .

# change to system folder to download and extract flasher image
cd system
source ./get_image.sh
xz --decompress --keep --force "${IMAGE_FLASHER_FILE}"
IMAGE=`basename "${IMAGE_FLASHER_FILE}" ".xz"`
cd ..

# make sure loop kernel module is loaded for mouting filesystem
modprobe loop

# build rocketlogger binary
docker run --privileged --platform linux/arm/v7  \
    --mount type=bind,source="$(pwd)/rocketlogger",target=/home/rocketlogger \
    --mount type=bind,source="$(pwd)/system",target=/home/system  \
    --tty beaglebone_builder /bin/bash -c "cd /home/system && ./chroot_setup.sh ${IMAGE}"
