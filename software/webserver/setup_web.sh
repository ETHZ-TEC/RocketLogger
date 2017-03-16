#!/bin/bash
# RocketLogger webserver setup script
#
# Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group
#

# disable apache server
rm -f /etc/init.d/apache2

# disable bonescript stuff
systemctl disable bonescript.service
systemctl disable bonescript.socket
systemctl disable bonescript-autorun.service

# copy lighttpd server configuration
cp -f lighttpd.conf /etc/lighttpd/lighttpd.conf

# copy webserver data
rm -rf /var/www/*
cp -rf data/* /var/www/
mkdir -p /var/www/data /var/www/log

# download and copy dependencies (bootstrap, jquery, flot)
mkdir -p /var/www/js/vendor/

wget -N https://github.com/twbs/bootstrap/releases/download/v3.3.7/bootstrap-3.3.7-dist.zip
unzip -o bootstrap-3.3.7-dist.zip
cp -rf bootstrap-3.3.7-dist/fonts /var/www/
cp -f bootstrap-3.3.7-dist/css/*.min.css /var/www/css/
cp -f bootstrap-3.3.7-dist/js/*.min.js /var/www/js/vendor/

wget -N https://code.jquery.com/jquery-3.1.1.min.js
cp -f jquery-3.1.1.min.js /var/www/js/vendor/

wget -N http://www.flotcharts.org/downloads/flot-0.8.3.zip
unzip -o flot-0.8.3.zip
cp -f flot/jquery.flot*.min.js /var/www/js/vendor/

