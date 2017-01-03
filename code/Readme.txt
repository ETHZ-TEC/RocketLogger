Infos for installing RocketLogger (Makefile available):

1) Install Prerequesites
	+ libncurses5-dev

2) Copy + compile code
	sudo make install (-> code copied to /bin/)

3) Create init script (/etc/init.d/rocketlogger) and make it executable:
	#!/bin/bash
	bin/rl_deamon &

4) Put run-level:
	update-rc.d rocketlogger defaults