#include "gpio.h"

// gpio unexport
int gpio_unexport(int num) {
	// open gpio value file
	int fd = open(GPIO_PATH "unexport", O_WRONLY);
	if (fd < 0) {
		printf("Error: could not open GPIO unexport file!\n");
		return -1;
	}
	
	// export gpio
	char buf[5] = "";
	int len = snprintf(buf, sizeof(buf),"%d",num);
	write(fd, buf, len);
	
	// close file
	close(fd);
	
	return 1;
}

// gpio export
int gpio_export(int num) {
	// open gpio value file
	int fd = open(GPIO_PATH "export", O_WRONLY);
	if (fd < 0) {
		printf("Error: could not open GPIO export file!\n");
		return -1;
	}
	
	// export gpio
	char buf[5] = "";
	int len = snprintf(buf, sizeof(buf),"%d",num);
	write(fd, buf, len);
	
	// close file
	close(fd);
	
	return 1;
}

// set direction
int gpio_dir(int num, enum direction dir) {
	
	// open gpio direction file
	char file_name[MAX_PATH_LENGTH];
	sprintf(file_name, GPIO_PATH "gpio%d/direction",num);
	int fd = open(file_name, O_WRONLY);
	if (fd < 0) {
		printf("Error: could not open GPIO direction file!\n");
		return -1;
	}
	
	// set direction
	if(dir == OUT) {
		write(fd, "out", 4);
	} else {
		write(fd, "in", 3);
	}
	// close file
	close(fd);
	
	return 1;
}


// set interrupt direction
int gpio_interrupt(int num, enum edge e) {
	
	// open gpio edge file
	char file_name[MAX_PATH_LENGTH];
	sprintf(file_name, GPIO_PATH "gpio%d/edge",num);
	int fd = open(file_name, O_WRONLY);
	if (fd < 0) {
		printf("Error: could not open GPIO edge file!\n");
		return -1;
	}
	
	// set edge
	switch (e) {
		case NONE:
			write(fd, "none", 5);
			break;
		case RISING:
			write(fd, "rising", 7);
			break;
		case FALLING:
			write(fd, "falling", 8);
			break;
		case BOTH:
			write(fd, "both", 5);
			break;
			
	}
	
	// close file
	close(fd);
	
	return 1;
}

// set gpio value
int gpio_set_value(int num, int val) {
	
	// open gpio value file
	char file_name[MAX_PATH_LENGTH];
	sprintf(file_name, GPIO_PATH "gpio%d/value",num);
	int fd = open(file_name, O_WRONLY);
	if (fd < 0) {
		printf("Error: could not open GPIO value file!\n");
		return -1;
	}
	
	// set value
	if(val == 0) {
		write(fd, "0", 2);
	} else {
		write(fd, "1", 2);
	}
	
	// close file
	close(fd);
	
	return 1;
}

// get gpio value
int gpio_get_value(int num) {
	
	// open gpio value file
	char file_name[MAX_PATH_LENGTH];
	sprintf(file_name, GPIO_PATH "gpio%d/value",num);
	int fd = open(file_name, O_RDONLY);
	if (fd < 0) {
		printf("Error: could not open GPIO value file!\n");
		return -1;
	}
	
	// read value
	char buf[2] = "";
	read(fd, &buf, 1);
	
	// close file
	close(fd);
	
	return atoi(buf);
}

// wait on gpio change (returns new value)
int gpio_wait_interrupt(int num, int timeout) {
	
	// open gpio value file
	char file_name[MAX_PATH_LENGTH];
	sprintf(file_name, GPIO_PATH "gpio%d/value",num);
	int fd = open(file_name, O_RDONLY);
	if (fd < 0) {
		printf("Error: could not open GPIO value file!\n");
		return -1;
	}
	
	// set up polling struct
	struct pollfd fds;
	fds.fd = fd;
	fds.events = POLLPRI;
	int nfds = 1;
	int ret;
	
	// dummy read (enables blocking polling)
	char buf[2] = "";
	read(fds.fd, &buf, 1);
		
	// wait on gpio change
	ret = poll(&fds, nfds, timeout);
	if (ret < 0) {
	  printf("Error: poll failed!\n");
	  return -1;
	}
	
	// read value
	lseek(fds.fd, 0, SEEK_SET);
	read(fds.fd, &buf, 1);
	
	close(fd);
	
	return atoi(buf);
}

/*int main(int argc, char **argv) {
	gpio_export(26);
	gpio_dir(26, IN);
	gpio_interrupt(26, RISING);
	int val = gpio_wait_interrupt(26);
	printf("Interrupt! Value = %d\n",val);
	gpio_unexport(26);
	
	gpio_export(44);
	gpio_dir(44,OUT);
	gpio_set_value(44,0);
	
	gpio_export(45);
	gpio_dir(45,OUT);
	gpio_set_value(45,1);
	
	gpio_export(26);
	gpio_dir(26, IN);
	int val = gpio_get_value(26);
	printf("GPIO26 = %d",val);
	
	return 1;
}*/