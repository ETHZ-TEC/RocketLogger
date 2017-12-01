#!/bin/bash
# Download the beagle bone image from the beaglebone archive
#
# Copyright (c) 2017, ETH Zurich, Computer Engineering Group
#

URL_DIRECTORY="http://debian.beagleboard.org/images/rcn-ee.net/rootfs/bb.org/release/2016-06-15/console/"
IMAGE_FILE="bone-debian-7.11-console-armhf-2016-06-15-2gb.img.xz"
IMAGE_SHA256="cfceb64083cf63ed49ad75c3b5f5665cef65eaa67c86420a2c4d27bddc22d1ee"

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

