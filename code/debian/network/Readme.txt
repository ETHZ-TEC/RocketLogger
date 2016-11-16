RocketLogger AP/WEB Setup:

1) Copy interfaces to /etc/network/interfaces
2) Copy hostapd.conf to /etc/hostapd/hostapd.conf (here you can set your AP settings)
3) Copy isc-dhcp-server to /etc/default/isc-dhcp-server
4) Copy dhcpd.conf to /etc/dhcp/dhcpd.conf
5) Copy lighttpd.conf to /etc/lighttpd/lighttpd.conf
6) Create password file: `htpasswd -c /home/rocketlogger/.htpasswd 'username'`
   Add new users: `htpasswd /home/rocketlogger/.htpasswd 'username'`
7) Reboot

HINT: Type #iwconfig to check, which wlan is used -> change it in first 3 files