#!/bin/bash
# Patch a BeagleBone system image with RocketLogger installation and configuration

LOG_FILE="patch_image.log"
GIT_REVISION=`git describe --tags --dirty`


# register ARM executables
docker run --rm --privileged docker/binfmt:a7996909642ee92942dcd6cff44b9b95f08dad64

# build minimal armv7 debian image with compiler tools
docker buildx build --platform linux/arm/v7 --tag rocketlogger_patch .

# change to system folder to download and decompress base system image
source ./get_image.sh
xz --decompress --keep --force "${IMAGE_FLASHER_FILE}"
IMAGE=`basename "${IMAGE_FLASHER_FILE}" ".xz"`

# install RocketLogger system to system image
set -o pipefail # report last non-zero exit code of piped commands
docker run --privileged --platform linux/arm/v7  \
    --mount type=bind,source="$(git rev-parse --show-toplevel)/",target=/home/rocketlogger  \
    --tty rocketlogger_patch /bin/bash -c "cd /home/rocketlogger/software/system && ./chroot_setup.sh ${IMAGE}"  \
  | tee "$(pwd)/${LOG_FILE}"
PATCH=$?
set +o pipefail # revert to default pipe exit code behavior

# verify image patching was successful
if [ $PATCH -ne 0 ]; then
  echo "> Image patching failed (code $PATCH). Check the log file for details: ${LOG_FILE}"
  echo "> Removed incompletely patched system image."
  rm --force "${IMAGE}"
  exit $PATCH
fi

# rename and compress successfully patched image
RL_IMAGE_NAME="rocketlogger-flasher-${GIT_REVISION}.img"
echo "> Rename successfully patched image file to '${RL_IMAGE_NAME}'"
mv --force "${LOG_FILE}" "${RL_IMAGE_NAME}.log"
mv --force "${IMAGE}" "${RL_IMAGE_NAME}"
xz --compress --threads=0 --force --verbose "${RL_IMAGE_NAME}"

# hint on the next setup step
echo ">> Flash the image to an SD card and insert it into a RocketLogger to install the system."
exit 0
