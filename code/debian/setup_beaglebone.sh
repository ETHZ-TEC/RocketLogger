#!/bin/bash
# Basic operating system configuration of a new BeagleBone Black/Green/Green Wireless


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

# add rocketlogger user to admin group for sudo commands
usermod -a -G admin rocketlogger

# display updated user configuration
id rocketlogger


## user permission
echo "> Setting user permissions"

# configure sudoers
cp -f sudo/sudoers /etc/


## security
echo "> Updating some security settings"

# copy more secure ssh configuration
cp -f ssh/sshd_config /etc/ssh/

# copy public keys for log in
mkdir /home/rocketlogger/.ssh/
chmod 700 /home/rocketlogger/.ssh/
cp -f user/rocketlogger@tik.ee.ethz.ch_rsa.pub /home/rocketlogger/.ssh/
cat /home/rocketlogger/.ssh/rocketlogger@tik.ee.ethz.ch_rsa.pub > /home/rocketlogger/.ssh/authorized_keys

# make user owner of its own files
chown rocketlogger:rocketlogger -R /home/rocketlogger/


## network configuration
echo "> Updating network configuration"

# copy network interface configuration
cp -f network/interfaces /etc/network/

# copy wifi ap configuration
echo "TODO: copy hostapd.conf"

# copy dhcp server configuration
echo "TODO: copy isc-dhcp-server"


## done
echo "Done. Please reboot to apply all changes."

