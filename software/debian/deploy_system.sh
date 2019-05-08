#!/bin/bash
# Basic operating system configuration of a new BeagleBone Black/Green/Green Wireless
# Usage: ./deploy_system.sh <beaglebone-host-address> [<hostname>]
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

HOSTNAME="rocketlogger"
REBOOT_TIMEOUT=20

# check arguments
if [ $# -lt 1 ]; then
  echo "Usage: ./deploy_system.sh <beaglebone-host-address> [<hostname>]"
  exit -1
fi
if [ $# -ge 2 ]; then
  HOSTNAME=$2
fi


HOST=$1
echo "Deploy system configuration on host '${HOST}'"


# copy system configuration scripts
echo "Copying system configuration scripts. You will be asked for the default user password, which is 'temppwd'."
scp -F /dev/null -P 22 -r config debian@${HOST}:

# verify copy new key worked
COPY=$?

if [ $COPY -ne 0 ]; then
  echo "[ !! ] Copy system configuration scripts failed (code $COPY). CHECK SSH CONFIGURATION."
  exit $COPY
else
  echo "[ OK ] Copy system configuration scripts successful."
fi


# grow file system size and reboot
echo "Set hostname and grow file system size. You will be asked twice for the user password, which is 'temppwd'."
ssh -F /dev/null -p 22 -t debian@${HOST} "sudo sed s/arm/${HOSTNAME}/g -i /etc/hostname /etc/hosts; cd /opt/scripts/tools/ && sudo ./grow_partition.sh"

# verify grow file system size worked
GROW=$?

if [ $GROW -ne 0 ]; then
  echo "[ !! ] Grow file system size failed (code $GROW). MANUALLY CHECK CONSOLE OUTPUT AND VERIFY SSH CONFIGURATION."
  exit $GROW
else
  echo "[ OK ] Grow file system size was successful."
fi


# wait for system to reboot
echo -n "Waiting for the system to reboot..."
sleep 5
while [[ $REBOOT_TIMEOUT -gt 0 ]]; do
  REBOOT_TIMEOUT=`expr $REBOOT_TIMEOUT - 1`
  echo -n "."
  ping -c1 -W2 bb-20.ethz.ch > /dev/null
  # break timeout loop on success
  if [ $? -eq 0 ]; then
    sleep 2
    echo " done."
    break
  fi
done

# check for connectibity loss
if [ $REBOOT_TIMEOUT -eq 0 ]; then
  echo ""
  echo "[ !! ] System reboot timed out. MANUALLY CHECK CONSOLE OUTPUT AND VERIFY SSH CONFIGURATION."
  exit 1
fi

# waiting for ssh login
echo "Waiting for ssh connection. You will be aked for the default user password, which is 'temppwd', when the system is ready."
sleep 5
ssh -F /dev/null -p 22 -t debian@${HOST} 'exit'

# perform system configuration
echo "Run system configuration. You will be aked for the default user password two times, which is 'temppwd'."
ssh -F /dev/null -p 22 -t debian@${HOST} '(cd config && sudo ./install.sh); exit $?'

# verify system configuration worked
CONFIG=$?

if [ $CONFIG -ne 255 ]; then
  echo "[ !! ] System configuration failed (code $CONFIG). MANUALLY CHECK CONSOLE OUTPUT AND VERIFY SSH CONFIGURATION."
  exit $CONFIG
else
  echo "[ OK ] System configuration was successful."
fi
