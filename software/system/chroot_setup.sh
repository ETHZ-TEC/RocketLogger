#!/bin/bash
# Patch operating system of a local BeagleBone system image
# Usage: ./chroot_setup.sh <image.img> [<hostname>]
# Note: needs to be executed in an (virtualized) arm-v7 environment

ROOTFS=/media
IMAGE=/dev/null
HOSTNAME="rocketlogger"

REPO_PATH=`git rev-parse --show-toplevel`
REPO_BRANCH=`git rev-parse --abbrev-ref HEAD`

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


## grow image filesystem
echo "> Grow system partition size to fit RocketLogger installation"

# grow image size and partition by 900 MB
dd if=/dev/zero of=${IMAGE} bs=1M count=900 oflag=append conv=notrunc status=progress
sfdisk ${IMAGE} <<-__EOF__
4M,,L,*
__EOF__
sync ${IMAGE}

# mount partition and grow filesystem to partition size
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
losetup --detach ${DISK}


## mount filesystem
echo "> Setup and configure filesystems"

# mount the beaglebone base image to patch
echo "> Mount image file '${IMAGE}' to ${ROOTFS}"
mount -o loop,offset=$((512*8192)) "${IMAGE}" ${ROOTFS}

# verify system image mount was successful
MOUNT=$?
if [ $MOUNT -ne 0 ]; then
  echo "[ !! ] System image mount failed (code $MOUNT). MANUALLY CHECK CONSOLE OUTPUT AND VERIFY SYSTEM CONFIGURATION."
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


# clone RocketLogger repository
echo "> Clone RocketLogger repository"
git clone --branch ${REPO_BRANCH} ${REPO_PATH} ${ROOTFS}/root/rocketlogger
rm --force --recursive ${ROOTFS}/root/rocketlogger/hardware ${ROOTFS}/root/rocketlogger/script


## setup operating system
echo "> Chroot to configure operating system"
chroot ${ROOTFS} /bin/bash -c "cd /root/rocketlogger/software/system/setup && ./setup.sh ${HOSTNAME}"

# verify system configuration was successful
CONFIG=$?
if [ $CONFIG -ne 0 ]; then
  echo "[ !! ] System configuration failed (code $CONFIG). MANUALLY CHECK CONSOLE OUTPUT AND VERIFY SYSTEM CONFIGURATION."
  exit $CONFIG
else
  echo "[ OK ] System configuration was successful."
fi


## install RocketLogger software
echo "> Chroot to build and install rocketlogger"
chroot ${ROOTFS} /bin/bash -c "cd /root/rocketlogger/software/rocketlogger && meson builddir && cd builddir && meson setup --wipe && ninja && sudo meson install --no-rebuild --only-changed"

# verify software installation configuration was successful
INSTALL=$?
if [ $INSTALL -ne 0 ]; then
  echo "[ !! ] Software installation failed (code $INSTALL). MANUALLY CHECK CONSOLE OUTPUT AND VERIFY SOFTWARE INSTALLATION."
  exit $INSTALL
else
  echo "[ OK ] Software installation was successful."
fi


## install RocketLogger web interface
echo "> Chroot to build and install web interface"
chroot ${ROOTFS} /bin/bash -c "cd /root/rocketlogger/software/node_server && ./install.sh"

# verify web interface installation configuration was successful
WEB=$?
if [ $WEB -ne 0 ]; then
  echo "[ !! ] Web interface installation failed (code $WEB). MANUALLY CHECK CONSOLE OUTPUT AND VERIFY WEB INTERFACE INSTALLATION."
  exit $WEB
else
  echo "[ OK ] Web interface installation was successful."
fi


## post installation cleanups
echo "> Clean up installation files"

# fix user home permissions
chroot ${ROOTFS} /bin/bash -c "chown --recursive rocketlogger:rocketlogger /home/rocketlogger/"

# cleanup root home
rm --force --recursive ${ROOTFS}/root/*/ ${ROOTFS}/root/.*/


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

# patching completed successfully
exit 0
