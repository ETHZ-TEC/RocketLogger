#!/bin/bash
# Basic operating system configuration of a new BeagleBone Black/Green/Green Wireless
#
# Copyright (c) 2016-2018, Swiss Federal Institute of Technology (ETH Zurich)
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

# need to run on a beaglebone
echo "> Check beaglebone platform"
if [[ $(hostname) -ne 'beaglebone' ]]; then
  echo "Need to run this scritp on the beaglebone. Aborting."
  exit 1
fi

# need to run as root
echo "> Checking root permission"
if [[ $(id -u) -ne 0 ]]; then
  echo "Please run as root. Aborting."
  exit 1
fi


## default login
echo "> Create new user 'rocketlogger'"

# add new rocketlogger user with home directory and bash shell
useradd --create-home --shell /bin/bash rocketlogger
# set default password
echo "rocketlogger:beaglebone" | chpasswd

# add rocketlogger user to admin and sudo group for super user commands
usermod --append --groups admin rocketlogger
usermod --append --groups sudo rocketlogger

# display updated user configuration
id rocketlogger


## default login
echo "> Disable default user 'debian'"

# set expiration date in the past to disable logins
chage -E 1970-01-01 debian


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
echo "RocketLogger v1.99" > /etc/issue.net

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


## updates and software dependencies
echo "> Updating system and installing software dependencies"

# update packages
apt update --assume-yes
apt upgrade --assume-yes

# install fundamental dependencies
apt install --assume-yes     \
  unzip                      \
  git                        \
  make                       \
  gcc                        \
  g++                        \
  ti-pru-cgt-installer       \
  device-tree-compiler       \
  ntp                        \
  apache2                    \
  lighttpd                   \
  php-cgi                    \
  libncurses5-dev            \
  libi2c-dev                 \
  linux-headers-$(uname -r)


## reboot
echo "Platform initialized. System will reboot to apply configuration changes."
reboot && exit 0
