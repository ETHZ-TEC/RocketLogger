#!/bin/bash
# RocketLogger webserver setup script
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

WEB_ROOT="/var/www"
WEB_SOURCE=`pwd`"/data"

# disable apache web server
rm -f /etc/init.d/apache2

# disable bonescript stuff
systemctl disable bonescript.service
systemctl disable bonescript.socket
systemctl disable bonescript-autorun.service

# copy lighttpd server configuration
cp -f lighttpd.conf /etc/lighttpd/lighttpd.conf

# copy webserver data
mkdir -p ${WEB_ROOT}
rm -f ${WEB_ROOT}/index.html
rsync -aP ${WEB_SOURCE}/ ${WEB_ROOT}/

# create data and log dirs
mkdir -p ${WEB_ROOT}/data ${WEB_ROOT}/log
#rm -rf /var/www/*
#cp -rf data/* /var/www/
#mkdir -p /var/www/data /var/www/log

# download and copy dependencies (bootstrap, jquery, flot)
mkdir -p ${WEB_ROOT}/css/ ${WEB_ROOT}/js/vendor/

wget -N https://github.com/twbs/bootstrap/releases/download/v3.3.7/bootstrap-3.3.7-dist.zip
wget -N https://code.jquery.com/jquery-3.1.1.min.js
wget -N http://www.flotcharts.org/downloads/flot-0.8.3.zip

unzip -o bootstrap-3.3.7-dist.zip
cp -rf bootstrap-3.3.7-dist/fonts /var/www/
cp -f bootstrap-3.3.7-dist/css/*.min.css /var/www/css/
cp -f bootstrap-3.3.7-dist/js/*.min.js /var/www/js/vendor/
rm -r bootstrap-*-dist

cp -f jquery-3.1.1.min.js /var/www/js/vendor/
rm jquery-3.1.1.min.js

unzip -o flot-0.8.3.zip
cp -f flot/jquery.flot*.min.js /var/www/js/vendor/
rm -r flot

