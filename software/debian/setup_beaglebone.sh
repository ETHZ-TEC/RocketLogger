#!/bin/bash
# Basic operating system configuration of a new BeagleBone Black/Green/Green Wireless
#
# Copyright (c) 2016-2017, Swiss Federal Institute of Technology (ETH Zurich)
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

# store current working directory
SCRIPT_DIR=`pwd`

# need to run as root
echo "> Checking root permission"
if [[ $(id -u) -ne 0 ]]; then
  echo "Please run as root"
  exit 1
fi


## default login
echo "> Setting default user to 'rocketlogger'"

# change default user to rocketlogger
usermod -l rocketlogger debian

# change default user group
groupmod -n rocketlogger debian

# set default password
echo "rocketlogger:beaglebone" | chpasswd

# change home directory to /home/rocketlogger
usermod -d /home/rocketlogger -m rocketlogger

# add rocketlogger user to admin and sudo group for super user commands
usermod -a -G admin rocketlogger
usermod -a -G sudo rocketlogger

# display updated user configuration
id rocketlogger


## user permission
echo "> Setting user permissions"

# configure sudoers
cp -f sudo/sudoers /etc/


## security
echo "> Updating some security and permission settings"

# copy more secure ssh configuration
cp -f ssh/sshd_config /etc/ssh/

# copy public keys for log in
mkdir -p /home/rocketlogger/.ssh/
chmod 700 /home/rocketlogger/.ssh/
cp -f user/rocketlogger.default_rsa.pub /home/rocketlogger/.ssh/
cat /home/rocketlogger/.ssh/rocketlogger.default_rsa.pub > /home/rocketlogger/.ssh/authorized_keys

# change ssh welcome message
echo "RocketLogger v1.1" > /etc/issue.net

# make user owner of its own files
chown rocketlogger:rocketlogger -R /home/rocketlogger/


## network configuration
echo "> Updating network configuration"

# copy network interface configuration
cp -f network/interfaces /etc/network/

# copy dhcp server configuration
cp -f network/isc-dhcp-server /etc/default/isc-dhcp-server
cp -f network/dhcpd.conf /etc/dhcp/dhcpd.conf

# create RL folder
mkdir -p /etc/rocketlogger


## updates
echo "> Updating system"

# update packages
apt-get update --assume-yes
apt-get upgrade --assume-yes

# install necessary dependencies
apt-get install --assume-yes \
  unzip                      \
  git                        \
  make                       \
  gcc                        \
  g++                        \
  ti-pru-cgt-installer       \
  device-tree-compiler       \
  am335x-pru-package         \
  ntp                        \
  apache2                    \
  lighttpd                   \
  php5-cgi                   \
  libncurses5-dev            \
  libi2c-dev                 \
  linux-headers-$(uname -r)


## grow file system
echo "> Grow file system size"

# use pre installed script that comes with image
cd /opt/scripts/tools/ && git pull && sudo ./grow_partition.sh
cd "${SCRIPT_DIR}"


## done
echo "Platform initialized. A reboot is required to apply all changes."

# reboot now?
REBOOT=-1
while [ $REBOOT -lt 0 ]; do
  read -p "Do you want to reboot now (recommended) [y/n]? " ANSWER
  case "$ANSWER" in
    y|yes)
      REBOOT="1"
      break;;
    n|no)
      REBOOT="0"
      break;;
    *) echo "invalid input!";;
  esac
done

# reboot or done
if [ $REBOOT -eq 1 ]; then
  echo "Rebooting now..."
  reboot
else
  echo "Done. Please reboot manually."
fi

