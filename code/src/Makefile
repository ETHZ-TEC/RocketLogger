#
#   RocketLogger Makefile
#
all: rocketlogger rl_deamon SPI.bin

install: rocketlogger rl_deamon SPI.bin
		sudo rm -f /bin/rocketlogger
		cp rocketlogger /bin/rocketlogger
		sudo rm -f /bin/rl_deamon
		cp rl_deamon /bin/rl_deamon
		sudo rm -f /lib/firmware/SPI.bin
		cp SPI.bin /lib/firmware/SPI.bin
		
		echo '#!/bin/bash' > /etc/init.d/rocketlogger
		echo '/bin/rl_deamon &' >> /etc/init.d/rocketlogger
		chmod +x /etc/init.d/rocketlogger
		update-rc.d rocketlogger defaults
				
		echo 'options uio_pruss extram_pool_sz=0x300000' > /etc/modprobe.d/rl_pru.conf

rocketlogger : rocketlogger.c util.o calibration.o pwm.o gpio.o meter.o file_handling.o rl_hw.o pru.o lib_util.o rl_lib.o rl_util.o
		gcc -Wall -O3 util.o calibration.o pwm.o gpio.o rl_hw.o pru.o lib_util.o rl_lib.o rl_util.o meter.o file_handling.o rocketlogger.c -o rocketlogger -pthread -lprussdrv -lrt -lncurses

rl_deamon : rl_deamon.c util.o calibration.o pwm.o gpio.o meter.o file_handling.o rl_hw.o pru.o lib_util.o rl_lib.o rl_util.o
		gcc -Wall -O3 util.o calibration.o pwm.o gpio.o rl_hw.o pru.o lib_util.o rl_lib.o rl_util.o meter.o file_handling.o rl_deamon.c -o rl_deamon -pthread -lprussdrv -lrt -lncurses

util.o : util.c
		gcc -Wall -O3 -c util.c
		
calibration.o : calibration.c
		gcc -Wall -O3 -c calibration.c
		
pwm.o : pwm.c
		gcc -Wall -O3 -c pwm.c
		
gpio.o : gpio.c
		gcc -Wall -O3 -c gpio.c
		
rl_hw.o : rl_hw.c
		gcc -Wall -O3 -c rl_hw.c

meter.o : meter.c
		gcc -Wall -O3 -c meter.c

file_handling.o : file_handling.c
		gcc -Wall -O3 -c file_handling.c
		
pru.o : pru.c
		gcc -Wall -O3 -c pru.c
		
lib_util.o : lib_util.c
		gcc -Wall -O3 -c lib_util.c
		
rl_lib.o : rl_lib.c
		gcc -Wall -O3 -c rl_lib.c
		
rl_util.o : rl_util.c
		gcc -Wall -O3 -c rl_util.c

SPI.bin : SPI.p
		pasm -b SPI.p SPI

clean :
		rm *.o
		rm *.bin