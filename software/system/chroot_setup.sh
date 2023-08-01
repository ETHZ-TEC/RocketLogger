#!/bin/bash
# Patch a BeagleBone file system image with the RocketLogger installation
# Usage: ./chroot_setup.sh <image> [<hostname>]
# * <image> specifys the (uncompressed) BeagleBone file system image to patch
# * <hostname> optionally specifies the the hostname to assign to the device
#   during setup, if not provided the default hostname used is: rocketlogger
# Note: needs to be executed in a (virtualized) arm-v7 environment

IMAGE=/dev/null
HOSTNAME="rocketlogger"


# check arguments
if [ $# -lt 1 ]; then
  echo "Usage: ./chroot_setup.sh <image> [<hostname>]"
  exit -1
else
  IMAGE=$1
fi
if [ $# -ge 2 ]; then
  HOSTNAME=$2
fi

echo "> Deploy RocketLogger system to image '${IMAGE}'"


# mark mounted git repository with differing file ownership as safe
git config --global --add safe.directory '*'

ROOTFS=`mktemp --directory`
ROOT_HOME=`mktemp --directory`
APT_CACHE=`mktemp --directory`
REPO_PATH=`git rev-parse --show-toplevel`
REPO_BRANCH=`git rev-parse --abbrev-ref HEAD`


## grow image filesystem
echo "> Grow system partition size to fit RocketLogger installation"

# grow image size and partition by 700 MB (500 MB additional system packages, 200 MB binary and web interface)
dd if=/dev/zero of=${IMAGE} bs=1M count=700 conv=fsync,notrunc oflag=append,sync status=progress
sfdisk ${IMAGE} <<-__EOF__
4M,,L,*
__EOF__
sync ${IMAGE} && sleep 3

# mount partition and grow filesystem to partition size
echo "> Setup loop device for ${IMAGE}"
DISK=`losetup --verbose --offset=$((512*8192)) --find --show ${IMAGE}`

# verify system image mount was successful
MOUNT=$?
if [ $MOUNT -ne 0 ]; then
  echo "[ !! ] System image mount for partition resize failed (code $MOUNT). MANUALLY CHECK CONSOLE OUTPUT AND VERIFY SYSTEM CONFIGURATION."
  exit $MOUNT
else
  echo "[ OK ] System image mount for partition resize was successful."
fi

e2fsck -f ${DISK}
resize2fs ${DISK}
sync ${DISK}


## mount filesystem
echo "> Setup and configure filesystems"

# mount the beaglebone base image to patch
echo "> Mount system partition to ${ROOTFS}"
mount ${DISK} ${ROOTFS}

# verify system image mount was successful
MOUNT=$?
df --sync ${ROOTFS}
if [ $MOUNT -ne 0 ]; then
  echo "[ !! ] System image mount failed (code $MOUNT). MANUALLY CHECK CONSOLE OUTPUT AND VERIFY SYSTEM CONFIGURATION."
  losetup --detach ${DISK}
  exit $MOUNT
else
  echo "[ OK ] System image mount was successful."
fi

echo "> Bind system mounts"
mount --bind /dev ${ROOTFS}/dev
mount --bind /sys ${ROOTFS}/sys
mount --bind /proc ${ROOTFS}/proc
mount --bind /dev/pts ${ROOTFS}/dev/pts

# map resolv.conf for network connectivity
echo "> Bind temporary network configuration"
mkdir --parents ${ROOTFS}/run/connman
touch ${ROOTFS}/run/connman/resolv.conf
mount --bind /etc/resolv.conf ${ROOTFS}/run/connman/resolv.conf

# bind temporary root home and apt cache
mount --bind ${ROOT_HOME} ${ROOTFS}/root
mount --bind ${APT_CACHE} ${ROOTFS}/var/cache/apt

# clone RocketLogger repository
echo "> Clone RocketLogger repository"
git clone --branch ${REPO_BRANCH} ${REPO_PATH} ${ROOTFS}/root/rocketlogger


## setup operating system
echo "> Chroot to configure operating system"
chroot ${ROOTFS} /bin/bash -c "cd /root/rocketlogger/software/system/setup && ./setup.sh ${HOSTNAME}"

# verify system configuration was successful
CONFIG=$?
df --sync ${ROOTFS}
if [ $CONFIG -ne 0 ]; then
  echo "[ !! ] System configuration failed (code $CONFIG). MANUALLY CHECK CONSOLE OUTPUT AND VERIFY SYSTEM CONFIGURATION."
  losetup --detach ${DISK}
  exit $CONFIG
else
  echo "[ OK ] System configuration was successful."
fi


## install RocketLogger software
echo "> Chroot to build and install rocketlogger"
chroot ${ROOTFS} /bin/bash -c "cd /root/rocketlogger/software/rocketlogger && meson builddir && ninja -C builddir && sudo ninja -C builddir install"

# verify software installation configuration was successful
INSTALL=$?
df --sync ${ROOTFS}
if [ $INSTALL -ne 0 ]; then
  echo "[ !! ] Software installation failed (code $INSTALL). MANUALLY CHECK CONSOLE OUTPUT AND VERIFY SOFTWARE INSTALLATION."
  losetup --detach ${DISK}
  exit $INSTALL
else
  echo "[ OK ] Software installation was successful."
fi


## install RocketLogger web interface
echo "> Chroot to build and install web interface"
chroot ${ROOTFS} /bin/bash -c "cd /root/rocketlogger/software/node_server && ./install.sh"

# verify web interface installation configuration was successful
WEB=$?
df --sync ${ROOTFS}
if [ $WEB -ne 0 ]; then
  echo "[ !! ] Web interface installation failed (code $WEB). MANUALLY CHECK CONSOLE OUTPUT AND VERIFY WEB INTERFACE INSTALLATION."
  losetup --detach ${DISK}
  exit $WEB
else
  echo "[ OK ] Web interface installation was successful."
fi


## post installation cleanups
echo "> Clean up installation files"

# fix user home permissions
chroot ${ROOTFS} /bin/bash -c "chown --recursive rocketlogger:rocketlogger /home/rocketlogger/"

# unmount and clean up temporary root home and apt cache
umount ${ROOTFS}/root
rm --force --recursive ${ROOT_HOME}
umount ${ROOTFS}/var/cache/apt
rm --force --recursive ${APT_CACHE}


## unmount filesystem
echo "> Sync and unmount filesystem"
sync ${ROOTFS}

# unmount and cleanup mapped resolve.conf
umount ${ROOTFS}/run/connman/resolv.conf
rm --force --recursive ${ROOTFS}/run/connman

# unmount system
umount ${ROOTFS}/dev/pts
umount ${ROOTFS}/proc
umount ${ROOTFS}/sys
umount ${ROOTFS}/dev
umount ${ROOTFS}

# detatch loop device
losetup --detach ${DISK}
sync ${DISK}

# clean up
rmdir ${ROOTFS}

# patching completed successfully
exit 0
