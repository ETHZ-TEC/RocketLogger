#!/bin/bash
# Remotely configure operating system of a new BeagleBone Black/Green/Green Wireless
# Usage: ./remote_setup.sh <beaglebone-host-address> [<hostname>]
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
REBOOT_TIMEOUT=60

# check arguments
if [ $# -lt 1 ]; then
  echo "Usage: ./remote_setup.sh <beaglebone-host-address> [<hostname>]"
  exit -1
fi
if [ $# -ge 2 ]; then
  HOSTNAME=$2
fi


HOST=$1
echo "> Deploy system on host '${HOST}'"


# copy system configuration scripts
echo "> Copy system configuration scripts. You will be asked for the default user password, which is 'temppwd'."
scp -F /dev/null -P 22 -r setup debian@${HOST}:

# verify copy setup files was successful
COPY=$?
if [ $COPY -ne 0 ]; then
  echo "[ !! ] Copy setup files failed (code $COPY). CHECK SSH CONFIGURATION."
  exit $COPY
else
  echo "[ OK ] Copy setup files successful."
fi

# perform system configuration
echo "> Run system configuration. You will be aked for the default user password two times, which is 'temppwd'."
ssh -F /dev/null -p 22 -t debian@${HOST} "(cd setup && sudo ./install.sh ${HOSTNAME} && cd .. && rm -rf setup && sudo reboot)"

# verify system configuration was successful
CONFIG=$?
if [ $CONFIG -ne 0 ]; then
  echo "[ !! ] System configuration failed (code $CONFIG). MANUALLY CHECK CONSOLE OUTPUT AND VERIFY SYSTEM CONFIGURATION."
  exit $CONFIG
else
  echo "[ OK ] System configuration was successful."
fi

# perform RocketLogger software installation
#TODO: a) await reboot, b) copy sources, c) remotely build and install

# verify software installation configuration was successful
INSTALL=$?
echo "[ !! ] Software skipped as not (yet) supported. Installation software manually."
# if [ $INSTALL -ne 0 ]; then
#   echo "[ !! ] Software installation failed (code $INSTALL). MANUALLY CHECK CONSOLE OUTPUT AND VERIFY SOFTWARE INSTALLATION."
#   exit $INSTALL
# else
#   echo "[ OK ] Software installation was successful."
# fi


# hint on the next setup step
echo ">> Wait for the system to reboot and you are ready to go."
