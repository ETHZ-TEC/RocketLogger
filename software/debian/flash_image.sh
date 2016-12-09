#!/bin/bash
## Flash the beagle bone image

# read command line arguments
IMAGE_FILE=$1
SDCARD_DEV=$2


# need to run as root
echo "> Checking root permission"
if [[ $(id -u) -ne 0 ]]; then
  echo "Please run as root"
  exit 1
fi

# decompress and write SD card image, sync file system at the end
xz --decompress --stdout ${IMAGE_FILE} | dd bs=1M status=progress of=${SDCARD_DEV}
echo -n "Syncing file system..."
sync
echo " done."

