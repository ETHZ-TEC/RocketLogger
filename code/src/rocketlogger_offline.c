#include <poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>

char pathname[100] = "/sys/class/gpio/gpio26/value";

int gpio_setup() {
	
	// TODO
	
	return 1;
}

int interrupt_handler(int value) {
	
	if (value == 1) {
		printf("Interrupt! Value = %d\n",value); // TODO: remove
		
		int rl_status = 1; // TODO: replace with function
		
		if (rl_status == 1) {
			system("rocketlogger stop");
		} else {
			system("rocketlogger sample 10");
		}
	}
	
	return 1;
	
}

int interrupt_listener() {
	
	// open gpio value file
	int fd = open(pathname, O_RDONLY);
	if (fd < 0) {
		printf("Error: could not open GPIO value file!\n");
		return -1;
	}
	
	// set up polling struct
	struct pollfd fds;
	fds.fd = fd;
	fds.events = POLLPRI;
	int timeout = -1; // infinite timeout
	int nfds = 1;
	int ret;
	
	// dummy read (enables blocking polling)
	char buf[2] = "";
	read(fds.fd, &buf, 1);
	
	while(1) {
		
		// wait on gpio change
		ret = poll(&fds, nfds, timeout);
		if (ret < 0) {
		  printf("Error: poll failed!\n");
		  return -1;
		}
		
		// read value
		lseek(fds.fd, 0, SEEK_SET);
		read(fds.fd, &buf, 1);
		interrupt_handler(atoi(buf));
	
	}
	
	close(fd);
	return 1;
	
}


int main(int argc, char **argv) {
	gpio_setup();
	return interrupt_listener();
}

