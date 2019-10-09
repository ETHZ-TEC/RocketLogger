#!/bin/bash
# RocketLogger webserver setup script
#
# Copyright (c) 2016-2019, ETH Zurich, Computer Engineering Group
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

WEB_ROOT="/var/www/rocketlogger"
WEB_SOURCE=`pwd`"/data"
WEB_AUTH_FILE=/home/rocketlogger/.htpasswd

BOOTSTRAP_VERSION=4.3.1
POPPER_VERSION=1.15.0
JQUERY_VERSION=3.4.1
FLOT_VERSION=3.2.9

## package install
echo "> Installing web interface dependencies"
sudo apt install --assume-yes \
  python3                     \
  python3-dev                 \
  virtualenvwrapper
  # apache2                     \
  # lighttpd                    \
  # php-cgi


## system configuration
echo "> Configuring system web services"

# disable other web services, enable lighttp
sudo systemctl stop apache2.service bonescript-autorun.service cloud9.service cloud9.socket nginx.service
sudo systemctl disable apache2.service bonescript-autorun.service cloud9.service cloud9.socket nginx.service
sudo systemctl enable lighttpd.service
sudo systemctl start lighttpd.service

# copy lighttpd server configuration
sudo cp -f lighttpd.conf /etc/lighttpd/lighttpd.conf


## webserver configuration
echo "> Deploying RocketLogger web interface"

# copy webserver data
sudo mkdir -p ${WEB_ROOT}
sudo rm -f ${WEB_ROOT}/index.html
sudo rsync -aP ${WEB_SOURCE}/ ${WEB_ROOT}/

# create default data and log dirs
sudo mkdir -p ${WEB_ROOT}/data ${WEB_ROOT}/log

# download dependencies (bootstrap, popper, jquery, flot)
sudo mkdir -p ${WEB_ROOT}/css/ ${WEB_ROOT}/js/vendor/

wget -N https://github.com/twbs/bootstrap/releases/download/v${BOOTSTRAP_VERSION}/bootstrap-${BOOTSTRAP_VERSION}-dist.zip
wget -N https://github.com/twbs/bootstrap/releases/download/v${BOOTSTRAP_VERSION}/bootstrap-${BOOTSTRAP_VERSION}-dist.zip
wget -N https://code.jquery.com/jquery-${JQUERY_VERSION}.min.js
wget -N https://unpkg.com/popper.js@${POPPER_VERSION}/dist/umd/popper.min.js
wget -N https://github.com/flot/flot/blob/v${FLOT_VERSION}/source/jquery.flot.js
wget -N https://github.com/flot/flot/blob/v${FLOT_VERSION}/source/jquery.flot.time.js

# copy relevant file and clean up
unzip -o bootstrap-${BOOTSTRAP_VERSION}-dist.zip
sudo cp -f bootstrap-${BOOTSTRAP_VERSION}-dist/css/bootstrap.min.css ${WEB_ROOT}/css/bootstrap-${BOOTSTRAP_VERSION}.min.css
sudo cp -f bootstrap-${BOOTSTRAP_VERSION}-dist/js/bootstrap.min.js ${WEB_ROOT}/js/vendor/bootstrap-${BOOTSTRAP_VERSION}.min.js
rm -r bootstrap-${BOOTSTRAP_VERSION}-dist
rm bootstrap-${BOOTSTRAP_VERSION}-dist.zip

sudo cp -f popper.min.js ${WEB_ROOT}/js/vendor/popper-${POPPER_VERSION}.min.js
rm popper.min.js

sudo cp -f jquery-${JQUERY_VERSION}.min.js ${WEB_ROOT}/js/vendor/jquery-${JQUERY_VERSION}.min.js
rm jquery-${JQUERY_VERSION}.min.js

sudo cp -f jquery.flot*.js ${WEB_ROOT}/js/vendor/
rm jquery.flot*.js

# create web interface login if not existing
if [ -f $WEB_AUTH_FILE ]; then
  echo "> Found existing web interface authentication config, skipping setup"
else
  echo "> Configure the web interface authentication for user 'rocketlogger':"
  htpasswd -c $WEB_AUTH_FILE rocketlogger
fi


## restart webserver to reload configuration
echo "> Restarting web server"
sudo systemctl restart lighttpd.service

echo "> Done installing RocketLogger web interface. Reboot as required."
