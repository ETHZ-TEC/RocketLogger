#!/bin/bash
# Patch operating system of a local BeagleBone Black/Green/Green Wireless image
# Usage: ./chroot_setup.sh <image.img> [<hostname>]
# Note: needs to be executed in an (virtualized) arm-v7 environment

ROOTFS=/media
IMAGE=/dev/null
HOSTNAME="rocketlogger"

# check arguments
if [ $# -lt 1 ]; then
  echo "Usage: ./chroot_setup.sh <image.img> [<hostname>]"
  exit -1
else
  IMAGE=$1
fi
if [ $# -ge 2 ]; then
  HOSTNAME=$2
fi

echo "> Deploy RocketLogger system to image '${IMAGE}'"


## mount filesystem
echo "> Setup and configure filesystems"

# mount the beaglebone base image to patch
echo "> Mount image file '${IMAGE}' to ${ROOTFS}"
mount -o loop,offset=$((512*8192)) "${IMAGE}" ${ROOTFS}

echo "> Bind system mounts"
mount --bind /dev ${ROOTFS}/dev/
mount --bind /sys ${ROOTFS}/sys/
mount --bind /proc ${ROOTFS}/proc/
mount --bind /dev/pts ${ROOTFS}/dev/pts
mount --bind setup ${ROOTFS}/root/

# map resolv.conf for network connectivity
echo "> Bind temporary network configuration"
mkdir --parents ${ROOTFS}/run/connman
touch ${ROOTFS}/run/connman/resolv.conf
mount --bind /etc/resolv.conf ${ROOTFS}/run/connman/resolv.conf

echo "> Create and mount cache dir"
mkdir /home/cache
mount --bind /home/cache ${ROOTFS}/var/cache


## setup operating system
# chroot to setup system
echo "> Chroot to configure operating system"
chroot ${ROOTFS} /bin/bash -c "cd /root/ && ./setup.sh ${HOSTNAME}"

# verify system configuration was successful
CONFIG=$?
if [ $CONFIG -ne 0 ]; then
  echo "[ !! ] System configuration failed (code $CONFIG). MANUALLY CHECK CONSOLE OUTPUT AND VERIFY SYSTEM CONFIGURATION."
  exit $CONFIG
else
  echo "[ OK ] System configuration was successful."
fi


## install RocketLogger software
# copy software sources
echo "> Copy RocketLogger sources"
cp --recursive /home/rocketlogger/ ${ROOTFS}/home/rocketlogger/
chown --recursive 1001:1001 ${ROOTFS}/home/rocketlogger/

# chroot to install software
echo "> Chroot to build and install rocketlogger"
chroot ${ROOTFS} /bin/bash -c "cd /home/rocketlogger/rocketlogger && meson builddir && cd builddir && meson setup --wipe && ninja && sudo meson install --no-rebuild --only-changed"

# verify software installation configuration was successful
INSTALL=$?
if [ $INSTALL -ne 0 ]; then
  echo "[ !! ] Software installation failed (code $INSTALL). MANUALLY CHECK CONSOLE OUTPUT AND VERIFY SOFTWARE INSTALLATION."
  exit $INSTALL
else
  echo "[ OK ] Software installation was successful."
fi


## mount filesystem
echo "> Sync, cleanup and unmount filesystems"
sync

# unmount and cleanup mapped resolve.conf, cache dir
umount ${ROOTFS}/run/connman/resolv.conf
rm --force --recursive ${ROOTFS}/run/connman
umount ${ROOTFS}/var/cache
rm --force --recursive /home/cache

# unmount
umount ${ROOTFS}/root/
umount ${ROOTFS}/dev/pts
umount ${ROOTFS}/proc/
umount ${ROOTFS}/sys/
umount ${ROOTFS}/dev/
umount ${ROOTFS}


# rename successfully patched image
IMAGE_PATCHED="rocketlogger-${IMAGE}"
echo "> Rename successfullt patched image file to '${IMAGE_PATCHED}'"
mv "${IMAGE}" "${IMAGE_PATCHED}"


# hint on the next setup step
echo ">> Flash the image to an SD card and insert it into any RocketLogger to install the system."
