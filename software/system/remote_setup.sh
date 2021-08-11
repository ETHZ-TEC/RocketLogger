#!/bin/bash
# Remotely configure operating system of a new BeagleBone Green or Black
# Usage: ./remote_setup.sh <host> [<hostname>]
# * <host> specifys the hostname or IP address of the BeagleBone device to set up
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
REBOOT_TIMEOUT=60

REPO_LOCAL_COPY=`mktemp --directory`
REPO_PATH=`git rev-parse --show-toplevel`
REPO_BRANCH=`git rev-parse --abbrev-ref HEAD`

# check arguments
if [ $# -lt 1 ]; then
  echo "Usage: ./remote_setup.sh <host> [<hostname>]"
  exit -1
fi
if [ $# -ge 2 ]; then
  HOSTNAME=$2
fi


HOST=$1
echo "> Deploy RocketLogger system on remote host '${HOST}'"

# create local repo copy to push to remote
echo "> Create copy of RocketLogger repository HEAD revision"
git clone --branch ${REPO_BRANCH} ${REPO_PATH} ${REPO_LOCAL_COPY}/rocketlogger
rm --force --recursive ${REPO_LOCAL_COPY}/rocketlogger/hardware ${REPO_LOCAL_COPY}/rocketlogger/script ${REPO_LOCAL_COPY}/rocketlogger/.git


## clone RocketLogger repository to remote host
echo "> Copy RocketLogger repository to remote. You will be asked for the default user password, which is 'temppwd'."
scp -F /dev/null -P 22 -r ${REPO_LOCAL_COPY}/rocketlogger debian@${HOST}:

# verify copy setup files was successful
COPY=$?
rm --force --recursive ${REPO_LOCAL_COPY} # clean up local repo copy in any case
if [ $COPY -ne 0 ]; then
  echo "[ !! ] Copy setup files to remote failed (code $COPY). CHECK SSH CONFIGURATION."
  exit $COPY
else
  echo "[ OK ] Copy setup files to remote successful."
fi


## perform system configuration
echo "> Run remote system configuration. You will be asked for the default user password two times, which is 'temppwd'."
ssh -F /dev/null -p 22 -t debian@${HOST} \
  "sudo bash -c \"cd rocketlogger/software/system/setup && ./setup.sh ${HOSTNAME} && cd - && mv rocketlogger /home/rocketlogger && chown --recursive rocketlogger:rocketlogger /home/rocketlogger/\""

# verify system configuration was successful
CONFIG=$?
if [ $CONFIG -ne 0 ]; then
  echo "[ !! ] Remote system configuration failed (code $CONFIG). MANUALLY CHECK CONSOLE OUTPUT AND VERIFY SYSTEM CONFIGURATION."
  exit $CONFIG
else
  echo "[ OK ] Remote system configuration was successful."
fi


## install RocketLogger software
echo "> Build and install rocketlogger on remote host. You will be asked for the new user password, which is 'beaglebone'."
ssh -F /dev/null -p 2322 -i ./setup/user/rocketlogger.default_rsa -o IdentitiesOnly=yes -t rocketlogger@${HOST} \
  "cd /home/rocketlogger/rocketlogger/software/rocketlogger && meson builddir && cd builddir && ninja && sudo meson install"

# verify software installation configuration was successful
INSTALL=$?
if [ $INSTALL -ne 0 ]; then
  echo "[ !! ] Remote software installation failed (code $INSTALL). MANUALLY CHECK CONSOLE OUTPUT AND VERIFY SOFTWARE INSTALLATION."
  exit $INSTALL
else
  echo "[ OK ] Remote software installation was successful."
fi


## install RocketLogger web interface
echo "> Install web interface on remote host. You will be asked for the new user password, which is 'beaglebone'."
ssh -F /dev/null -p 2322 -i ./setup/user/rocketlogger.default_rsa -o IdentitiesOnly=yes -t rocketlogger@${HOST} \
  "cd /home/rocketlogger/rocketlogger/software/node_server && ./install.sh && sudo poweroff"

# verify web interface installation configuration was successful
WEB=$?
if [ $WEB -ne 0 ]; then
  echo "[ !! ] Remote web interface installation failed (code $WEB). MANUALLY CHECK CONSOLE OUTPUT AND VERIFY WEB INTERFACE INSTALLATION."
  exit $WEB
else
  echo "[ OK ] Remote web interface installation was successful."
fi


# hint on the next setup step
echo ">> Wait for the system to reboot and you are ready to go."
