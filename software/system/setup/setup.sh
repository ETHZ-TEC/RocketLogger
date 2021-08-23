#!/bin/bash
# Basic operating system initalization of a new BeagleBone Black/Green/Green Wireless
# Usage: setup.sh [<hostname>]
# * <hostname> optionally specifies the the hostname to assign to the device
#   during setup, if not provided the default hostname used is: rocketlogger
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

HOSTNAME="rocketlogger"

# need to run as root
echo "> Check root permission"
if [[ $(id -u) -ne 0 ]]; then
  echo "Need to run as root. Aborting."
  exit 1
fi

# check that we run on a clean BeagleBone image
echo "> Check beaglebone platform"
if [[ "$(cat /etc/hostname)" != "beaglebone" ]]; then
  echo "Need to run this script on a clean BeagleBone image. Aborting."
  exit 1
fi

# check whether hostname argument is given
if [ $# -ge 1 ]; then
  HOSTNAME=$1
fi

# check network connectivity
echo "> Check network connectivity"
ping -q -c 1 -W 2 "8.8.8.8" > /dev/null 2>&1
if [ $? -ne 0 ]; then
  echo "No network connectivity! Aborting."
  exit 1
fi

## updates and software dependencies
echo "> Update system and install software dependencies"

# install stretch-backports repository for meson and ninja
echo "deb http://deb.debian.org/debian stretch-backports main" >> /etc/apt/sources.list.d/stretch-backports.list

# update package lists
apt-get update > /dev/null
if [ $? -ne 0 ]; then
  echo "Failed to refresh package list."
  return 1
fi

# update packages
#apt-get -qq --assume-yes upgrade    # can't redirect to /dev/null, apt-get may ask questions during the process...
#if [ $? -ne 0 ]; then
#  echo "Failed to upgrade system packages."
#  return 1
#fi

# install system packages
apt-get install --assume-yes        \
    build-essential                 \
    curl                            \
    device-tree-compiler            \
    gcc                             \
    g++                             \
    git                             \
    make                            \
    meson/stretch-backports         \
    minicom                         \
    ninja-build                     \
    ntp                             \
    pkg-config                      \
    python3                         \
    python3-dev                     \
    python3-pip                     \
    ti-pru-cgt-installer            \
    libi2c-dev                      \
    libncurses5-dev                 \
    libzmq3-dev                     \
    linux-headers-$(uname -r)       \
    snmpd                           \
    unzip                           \
    > /dev/null

# verify system dependencies installation was successful
INSTALL=$?
if [ $INSTALL -ne 0 ]; then
  echo "[ !! ] System dependencies installation failed (code $INSTALL). MANUALLY CHECK CONSOLE OUTPUT AND VERIFY SYSTEM CONFIGURATION."
  exit $INSTALL
else
  echo "[ OK ] System dependencies installation was successful."
fi


## default login
echo "> Create new user 'flocklab'"

# add new flocklab user with home directory and bash shell
useradd --create-home --shell /bin/bash flocklab > /dev/null
# set default password
cat user/password | chpasswd

# add flocklab user to certain groups
usermod --append --groups sudo flocklab
usermod --append --groups admin flocklab
usermod --append --groups dialout flocklab
usermod --append --groups i2c flocklab
usermod --append --groups gpio flocklab
usermod --append --groups pwm flocklab
usermod --append --groups spi flocklab
usermod --append --groups adm flocklab
usermod --append --groups users flocklab
usermod --append --groups disk flocklab

# display updated user configuration
id flocklab

# add user Debian-snmp to group i2c
usermod --append --groups i2c Debian-snmp


## user permission
echo "> Set user permissions"

# configure sudoers
cp --force sudo/privacy /etc/sudoers.d/
chmod 440 /etc/sudoers.d/*


## security
echo "> Update some security and permission settings"

# copy public keys for log in
mkdir --parents /home/flocklab/.ssh/
chmod 700 /home/flocklab/.ssh/
cp --force user/rocketlogger.default_rsa.pub /home/flocklab/.ssh/
cat /home/flocklab/.ssh/rocketlogger.default_rsa.pub > /home/flocklab/.ssh/authorized_keys

# copy more secure ssh configuration
cp --force ssh/sshd_config /etc/ssh/
systemctl reload sshd

# change ssh welcome message
cp --force system/issue.net /etc/issue.net


## filesystem, system and user config directories setup
echo "> Configure SD card mount and prepare configuration folders"

# external SD card mount
mkdir --parents /media/sdcard/
#chown flocklab:flocklab /media/card
#chmod 755 /media/card
echo -e "# mount external sdcard on boot if available" >> /etc/fstab
echo -e "/dev/mmcblk0p1\t/media/sdcard/\tauto\tnofail,noatime,errors=remount-ro\t0\t2" >> /etc/fstab


# create RocketLogger and FlockLab system config folders
mkdir --parents /etc/rocketlogger
mkdir --parents /etc/flocklab

# user configuration and data folder for rocketlogger, bind sdcard folders if available
mkdir --parents /home/flocklab/.config/rocketlogger/
mkdir --parents /home/flocklab/data/
mkdir --parents /var/log/flocklab/
echo -e "# bind FlockLab sdcard folders if available" >> /etc/fstab

echo -e "/media/sdcard/flocklab/config\t/home/flocklab/.config/rocketlogger\tauto\tbind,nofail,noatime,errors=remount-ro\t0\t0" >> /etc/fstab
echo -e "/media/sdcard/flocklab/data\t/home/flocklab/data\tauto\tbind,nofail,noatime,errors=remount-ro\t0\t0" >> /etc/fstab
echo -e "/media/sdcard/flocklab/log\t/var/log/flocklab\tauto\tbind,nofail,noatime,errors=remount-ro\t0\t0" >> /etc/fstab

# make user owner of its own files
chown --recursive flocklab:flocklab /home/flocklab/

# patch flasher tools to include SD card mounts
patch --forward --backup --input=tools/functions.sh.patch /opt/scripts/tools/eMMC/functions.sh


## network configuration
echo "> Update hostname and network configuration"

# change hostname
sed s/beaglebone/${HOSTNAME}/g --in-place /etc/hostname /etc/hosts

# copy network interface configuration
cp --force network/interfaces /etc/network/

# copy SNMP config
cp --force snmp/snmpd.conf /etc/snmp/

## deactivate default login
echo "> Disable default user 'debian'"

# set expiration date in the past to disable logins
chage --expiredate 1970-01-01 debian

## sync filesystem
echo "> Rocketlogger platform initialized. Syncing file system and exit"
sync
