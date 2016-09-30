Infos for installing RocketLogger (Makefile available):

# compile hw libraries
gcc -Wall -O3 -o gpio.o -c gpio.c
gcc -Wall -O3 -o pwm.o -c pwm.c
gcc -Wall -O3 -o util.o -c util.c
gcc -Wall -O3 -o rl_hw.o -c rl_hw.c
gcc -Wall -O3 -o calibration.o -c calibration.c
gcc -Wall -O3 -o pru.o -c pru.c -lprussdrv

# compile rl library
gcc -Wall -O3 -o lib_util.o -c lib_util.c
gcc -Wall -O3 -o rl_lib.o -c rl_lib.c -lprussdrv

# compile cli
gcc -Wall -O3 -o rl_util.o -c rl_util.c
gcc -Wall -O3 util.o calibration.o pwm.o gpio.o rl_hw.o pru.o lib_util.o rl_lib.o rl_util.o rocketlogger.c -o rocketlogger -pthread -lprussdrv -lrt

# compile deamon
gcc -Wall -O3 util.o calibration.o pwm.o gpio.o rl_hw.o pru.o lib_util.o rl_lib.o rl_util.o rl_deamon.c -o rl_deamon -pthread -lprussdrv -lrt


-> copy rocketlogger and rl_deamon to /bin/

Create init script (/etc/init.d/rocketlogger) and make it executable:
	#!/bin/bash
	bin/rl_deamon &

Put run-level:
update-rc.d rocketlogger defaults
