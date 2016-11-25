#!/bin/bash
# RocketLogger webserver configuration

# remove apache server
rm -f /etc/init.d/apache2

# disable bonescript stuff
systemctl disable bonescript.service              
systemctl disable bonescript.socket
systemctl disable bonescript-autorun.service

# copy lighttpd server configuration
cp -f lighttpd.conf /etc/lighttpd/lighttpd.conf

# add webserver data
rm -rf /var/www/*
cp -f data/* /var/www/
mkdir -p /var/www/data /var/www/log

# download additional stuff (bootstrap, jquery, flot)
wget http://www.flotcharts.org/downloads/flot-0.8.3.zip
unzip flot-0.8.3.zip
cp -rf flot /var/www

wget https://github.com/twbs/bootstrap/releases/download/v3.3.7/bootstrap-3.3.7-dist.zip
unzip bootstrap-3.3.7-dist.zip
mkdir -p /var/www/bootstrap
cp -rf bootstrap-3.3.7-dist/* /var/www/bootstrap

wget http://code.jquery.com/jquery-2.2.4.js
cp -f jquery-2.2.4.js /var/www/jquery.js