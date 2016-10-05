#
#   RocketLogger Makefile
#
all: rocketlogger rl_deamon SPI.bin

install: rocketlogger rl_deamon SPI.bin
		cp rocketlogger /bin/rocketlogger
		cp rl_deamon /bin/rl_deamon
		cp SPI.bin /lib/firmware/SPI.bin

rocketlogger : rocketlogger.c util.o calibration.o pwm.o gpio.o rl_hw.o pru.o lib_util.o rl_lib.o rl_util.o
		gcc -Wall -O3 util.o calibration.o pwm.o gpio.o rl_hw.o pru.o lib_util.o rl_lib.o rl_util.o rocketlogger.c -o rocketlogger -pthread -lprussdrv -lrt -lncurses

rl_deamon : rl_deamon.c util.o calibration.o pwm.o gpio.o rl_hw.o pru.o lib_util.o rl_lib.o rl_util.o
		gcc -Wall -O3 util.o calibration.o pwm.o gpio.o rl_hw.o pru.o lib_util.o rl_lib.o rl_util.o rl_deamon.c -o rl_deamon -pthread -lprussdrv -lrt -lncurses

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