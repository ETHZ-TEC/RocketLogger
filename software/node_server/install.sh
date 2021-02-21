#!/bin/bash
# RocketLogger nodejs web interface install script
#
# Copyright (c) 2021, Lukas Sigrist <lsigrist@mailbox.org>
# Copyright (c) 2020, ETH Zurich, Computer Engineering Group
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

## install directories
INSTALL_WEB_DIR=/home/rocketlogger/web/
INSTALL_DATA_DIR=/home/rocketlogger/data
INSTALL_SERVICE_DIR=/etc/systemd/system/

SERVICE_CONFIG=rocketlogger-web.service


## package install
echo "> Install required system packages"

# install latest nodejs and compiler for compiled dependencies
sudo apt-get install --assume-yes \
  g++                             \
  make                            \
  nodejs


## copy and install nodejs packages
echo "> Setup nodejs environment"

# create traget install directory if not existing
mkdir --parents ${INSTALL_WEB_DIR} ${INSTALL_DATA_DIR}

## create webserver install folder and install npm packets
echo "> Copy RocketLogger nodejs web interface"
cp --force --recursive --verbose *.js ${INSTALL_WEB_DIR}
cp --force --recursive --verbose static ${INSTALL_WEB_DIR}
cp --force --recursive --verbose templates ${INSTALL_WEB_DIR}
cp --force --recursive --verbose package.json ${INSTALL_WEB_DIR}

echo "> Install RocketLogger nodejs web interface dependencies"
npm install --production --prefix ${INSTALL_WEB_DIR} ${INSTALL_WEB_DIR}


## install web interface service and restart
echo "> Install and start RocketLogger web interface as service"
sudo install --mode=644 ${SERVICE_CONFIG} ${INSTALL_SERVICE_DIR}
sudo systemctl enable rocketlogger-web
sudo systemctl restart rocketlogger-web

echo "> Done installing RocketLogger web interface."
