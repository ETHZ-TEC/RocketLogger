#!/bin/bash
# Format an SD card and initialize folders and permission for RocketLogger data storage.

# configuration
ROCKETLOGGER_UID=1001
ROCKETLOGGER_GID=1001

# read command line arguments
SDCARD_DEV=$1


# need to run as root
echo "> Checking root permission"
if [[ $(id -u) -ne 0 ]]; then
  echo "Need to run as root (e.g. using sudo)"
  exit 1
fi

# check arguments
if [ $# -lt 1 ]; then
  echo "Usage: ./prepare_sdcard.sh <sdcard-dev>"
  exit 1
fi

# partition SD card for RocketLogger data storage
echo "> Re-partition SD card"
sfdisk ${SDCARD_DEV} <<-__EOF__
,,L,
__EOF__
PARTITION=$?
sync ${SDCARD_DEV}

# verify SD card partitioning was successful
if [ $PARTITION -ne 0 ]; then
  echo "> SD card partitioning failed (code $PARTITION). MANUALLY CHECK CONSOLE OUTPUT AND VERIFY SYSTEM CONFIGURATION."
  exit $PARTITION
fi

# format SD card for RocketLogger data storage
echo "> Format new SD card partition"
if [ -b ${SDCARD_DEV}1 ]; then
  SDCARD_PART=${SDCARD_DEV}1
elif [ -b ${SDCARD_DEV}p1 ]; then
  SDCARD_PART=${SDCARD_DEV}p1
else
  echo "> Unable to determine SD card partition block device! MANUALLY CHECK CONSOLE OUTPUT AND VERIFY SYSTEM CONFIGURATION."
  exit 1
fi
dd if=/dev/zero of=${SDCARD_PART} bs=1M count=1 conv=fsync  # erase potentially existing partion data
mkfs.ext4 -L rocketlogger ${SDCARD_PART}
FORMAT=$?
sync ${SDCARD_PART}

# verify SD card formatting was successful
if [ $FORMAT -ne 0 ]; then
  echo "> SD card formatting failed (code $FORMAT). MANUALLY CHECK CONSOLE OUTPUT AND VERIFY SYSTEM CONFIGURATION."
  exit $FORMAT
fi

# mount newly formatted SD card partition
MOUNT_POINT=`mktemp --directory`
echo "> Mount formatted SD card to $MOUNT_POINT"
mount ${SDCARD_PART} ${MOUNT_POINT}
MOUNT=$?

# verify image patching was successful
if [ $MOUNT -ne 0 ]; then
  echo "> SD card mounting failed (code $MOUNT). MANUALLY CHECK CONSOLE OUTPUT AND VERIFY SYSTEM CONFIGURATION."
  exit $MOUNT
fi

# generate folder structures and set permissions
echo "> Generate SD card folder structure and set permissions"
mkdir --parents ${MOUNT_POINT}/rocketlogger/config
mkdir --parents ${MOUNT_POINT}/rocketlogger/data
chown --recursive ${ROCKETLOGGER_UID}:${ROCKETLOGGER_GID} ${MOUNT_POINT}/rocketlogger
chmod --recursive 775 ${MOUNT_POINT}/rocketlogger

# generate folder structures and set permissions
echo "> Sync file system and unmount"
sync ${SDCARD_PART}
umount ${MOUNT_POINT}
rmdir ${MOUNT_POINT}


# hint on the next setup step
echo ">> Preparation of SD card for RocketLogger data storage completed. To continue:"
echo "   * copy existing calibration file to \`rocketlogger/config\` on the SD card,"
echo "   * power down the RocketLogger device and insert the SD card,"
echo "   * start the RocketLogger and start your measurements."
exit 0
