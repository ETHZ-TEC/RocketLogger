#!/bin/bash
# RocketLogger webserver setup script
#
# Copyright (c) 2016-2019, Swiss Federal Institute of Technology (ETH Zurich)
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

## package install

echo "> Installing web interface dependencies"
sudo apt install --assume-yes \
  apache2                     \
  lighttpd                    \
  php-cgi


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

# download and copy dependencies (bootstrap, jquery, flot)
sudo mkdir -p ${WEB_ROOT}/css/ ${WEB_ROOT}/js/vendor/

wget -N https://github.com/twbs/bootstrap/releases/download/v3.3.7/bootstrap-3.3.7-dist.zip
wget -N https://code.jquery.com/jquery-3.1.1.min.js
wget -N http://www.flotcharts.org/downloads/flot-0.8.3.zip

unzip -o bootstrap-3.3.7-dist.zip
sudo cp -rf bootstrap-3.3.7-dist/fonts ${WEB_ROOT}/
sudo cp -f bootstrap-3.3.7-dist/css/*.min.css ${WEB_ROOT}/css/
sudo cp -f bootstrap-3.3.7-dist/js/*.min.js ${WEB_ROOT}/js/vendor/
rm -r bootstrap-*-dist

sudo cp -f jquery-3.1.1.min.js ${WEB_ROOT}/js/vendor/
rm jquery-3.1.1.min.js

unzip -o flot-0.8.3.zip
sudo cp -f flot/jquery.flot*.min.js ${WEB_ROOT}/js/vendor/
rm -r flot

# create web interface login if not existing
if [ -f $WEB_AUTH_FILE ]; then
  echo "> Found existing web interface authentication config, skipping setup"
else
  echo "> Configure the web interface authentication for user 'rocketlogger':"
  htpasswd -c $WEB_AUTH_FILE rocketlogger
fi


# restart webserver to reload configuration
sudo systemctl restart lighttpd.service

echo "> Done installing RocketLogger web interface. Reboot as required."
