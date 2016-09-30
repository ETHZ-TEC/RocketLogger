#
#   RocketLogger Makefile
#
all: rocketlogger rl_deamon

rocketlogger : util.o calibration.o pwm.o gpio.o rl_hw.o pru.o lib_util.o rl_lib.o rl_util.o
		gcc -Wall -O3 util.o calibration.o pwm.o gpio.o rl_hw.o pru.o lib_util.o rl_lib.o rl_util.o rocketlogger.c -o rocketlogger -pthread -lprussdrv -lrt

rl_deamon : util.o calibration.o pwm.o gpio.o rl_hw.o pru.o lib_util.o rl_lib.o rl_util.o
		gcc -Wall -O3 util.o calibration.o pwm.o gpio.o rl_hw.o pru.o lib_util.o rl_lib.o rl_util.o rl_deamon.c -o rl_deamon -pthread -lprussdrv -lrt

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


clean :
		rm *.o