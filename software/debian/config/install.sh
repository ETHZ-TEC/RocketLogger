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
cat user/password | chpasswd

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
cp -f sudo/privacy /etc/sudoers.d/
#cp -f sudo/rocketlogger_web /etc/sudoers.d/
chmod 440 /etc/sudoers.d/*


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
cp -f system/issue.net /etc/issue.net

# make user owner of its own files
chown rocketlogger:rocketlogger -R /home/rocketlogger/


## network configuration
echo "> Updating network configuration"

# copy network interface configuration
cp -f network/interfaces /etc/network/

# create RocketLogger system config folder
mkdir -p /etc/rocketlogger


## updates and software dependencies
echo "> Deactivating and uninstalling potentially conflicting services"

# stop preinstalled web services
sudo systemctl stop bonescript-autorun.service cloud9.service cloud9.socket nginx.service
sudo systemctl disable bonescript-autorun.service cloud9.service cloud9.socket nginx.service

# uninstall preinstalled web services
sudo apt remove --assume-yes --allow-change-held-packages \
  nginx                                                   \
  nodejs?                                                 \
  c9-core-installer                                       \
  bonescript?
sudo apt autoremove --assume-yes


## updates and software dependencies
echo "> Updating system and installing software dependencies"

# update packages
apt update --assume-yes
apt upgrade --assume-yes

# install fundamental dependencies
apt install --assume-yes        \
  unzip                         \
  git                           \
  make                          \
  gcc                           \
  g++                           \
  pru-software-support-package  \
  ti-pru-cgt-installer          \
  device-tree-compiler          \
  ntp                           \
  libncurses5-dev               \
  i2c-tools                     \
  libi2c-dev                    \
  linux-headers-$(uname -r)

# install am355x PRU support package from git
echo "> Manually download, compile and install am335x-pru-package"
git clone https://github.com/beagleboard/am335x_pru_package.git
(cd am335x_pru_package && make && make install)
ldconfig


## cleanup
(cd .. && rm -rf config)


## reboot
echo "Platform initialized. System will reboot to apply configuration changes."
reboot && exit 0
