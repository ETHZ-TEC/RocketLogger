#!/bin/bash
# RocketLogger nodejs web interface install script
#
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

BOOTSTRAP_VERSION=4.4.1
POPPER_VERSION=1.16.0
JQUERY_VERSION=3.4.1
SOCKETIO_VERSION=2.3.0
PLOTLY_VERSION=1.53.0

## package install
echo "> Install required system packages"
sudo apt install --assume-yes \
  nodejs                      \
  npm


## system configuration
echo "> Disable existing web services"

# disable other web services
sudo systemctl stop apache2.service bonescript-autorun.service cloud9.service cloud9.socket nginx.service
sudo systemctl disable apache2.service bonescript-autorun.service cloud9.service cloud9.socket nginx.service


## nodejs package dependencies
echo "> Setup nodejs environment"

# create traget install directory if not existing
mkdir -p ${INSTALL_WEB_DIR} ${INSTALL_DATA_DIR}

# install npm servers side packages
npm install --prefix ${INSTALL_WEB_DIR} \
  express                               \
  nunjucks                              \
  gulp                                  \
  socket.io                             \
  zeromq

# install client side dependencies
npm install --prefix ${INSTALL_WEB_DIR} \
  bootstrap@${BOOTSTRAP_VERSION}        \
  popper.js@${POPPER_VERSION}           \
  jquery@${JQUERY_VERSION}              \
  socket.io-client@${SOCKETIO_VERSION}  \
  plotly.js@${PLOTLY_VERSION}


## create webserver install folder and install npm packets
echo "> Install RocketLogger nodejs web interface"

# copy webserver data
rsync -aP ./static ${INSTALL_WEB_DIR}/
rsync -aP ./templates ${INSTALL_WEB_DIR}/
rsync -aP ./*.js ${INSTALL_WEB_DIR}/


# # create web interface login if not existing
# if [ -f $WEB_AUTH_FILE ]; then
#   echo "> Found existing web interface authentication config, skipping setup"
# else
#   echo "> Configure the web interface authentication for user 'rocketlogger':"
#   htpasswd -c $WEB_AUTH_FILE rocketlogger
# fi


## install web interface service and restart
echo "> Install and start RocketLogger web interface as service"
sudo install --mode=644 ${SERVICE_CONFIG} ${INSTALL_SERVICE_DIR}
sudo systemctl enable rocketlogger-web
sudo systemctl restart rocketlogger-web

echo "> Done installing RocketLogger web interface."
