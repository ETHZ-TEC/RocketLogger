/**
 * Copyright (c) 2016-2017, ETH Zurich, Computer Engineering Group
 */

#include "gpio.h"

/**
 * Unexport (deactivate) a GPIO.
 * @param num Linux GPIO number.
 * @return {@link SUCCESS} in case of success, {@link FAILURE} otherwise.
 */
int gpio_unexport(int num) {
	// open gpio value file
	int fd = open(GPIO_PATH "unexport", O_WRONLY);
	if (fd < 0) {
		rl_log(ERROR, "could not open GPIO unexport file");
		return FAILURE;
	}
	
	// export gpio
	char buf[5] = "";
	int len = snprintf(buf, sizeof(buf),"%d",num);
	write(fd, buf, len);
	
	// close file
	close(fd);
	
	return SUCCESS;
}


/**
 * Export (activate) a GPIO.
 * @param num Linux GPIO number.
 * @return {@link SUCCESS} in case of success, {@link FAILURE} otherwise.
 */
int gpio_export(int num) {
	// open gpio value file
	int fd = open(GPIO_PATH "export", O_WRONLY);
	if (fd < 0) {
		rl_log(ERROR, "could not open GPIO export file");
		return FAILURE;
	}
	
	// export gpio
	char buf[5] = "";
	int len = snprintf(buf, sizeof(buf),"%d",num);
	write(fd, buf, len);
	
	// close file
	close(fd);
	
	return SUCCESS;
}

/**
 * Set direction of GPIO.
 * @param num Linux GPIO number.
 * @param dir Direction.
 * @return {@link SUCCESS} in case of success, {@link FAILURE} otherwise.
 */
int gpio_dir(int num, rl_direction dir) {
	
	// open gpio direction file
	char file_name[MAX_PATH_LENGTH];
	sprintf(file_name, GPIO_PATH "gpio%d/direction",num);
	int fd = open(file_name, O_WRONLY);
	if (fd < 0) {
		rl_log(ERROR, "could not open GPIO direction file");
		return FAILURE;
	}
	
	// set direction
	if(dir == OUT) {
		write(fd, "out", 4);
	} else {
		write(fd, "in", 3);
	}
	// close file
	close(fd);
	
	return SUCCESS;
}


/**
 * Set direction of GPIO interrupt edge.
 * @param num Linux GPIO number.
 * @param edge Edge direction.
 * @return {@link SUCCESS} in case of success, {@link FAILURE} otherwise.
 */
int gpio_interrupt(int num, rl_edge edge) {
	
	// open gpio edge file
	char file_name[MAX_PATH_LENGTH];
	sprintf(file_name, GPIO_PATH "gpio%d/edge",num);
	int fd = open(file_name, O_WRONLY);
	if (fd < 0) {
		rl_log(ERROR, "could not open GPIO edge file");
		return FAILURE;
	}
	
	// set edge
	switch (edge) {
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
	
	return SUCCESS;
}

/**
 * Set value of GPIO.
 * @param num Linux GPIO number.
 * @param val Value (0 or 1).
 * @return {@link SUCCESS} in case of success, {@link FAILURE} otherwise.
 */
int gpio_set_value(int num, int val) {
	
	// open gpio value file
	char file_name[MAX_PATH_LENGTH];
	sprintf(file_name, GPIO_PATH "gpio%d/value",num);
	int fd = open(file_name, O_WRONLY);
	if (fd < 0) {
		rl_log(ERROR, "could not open GPIO value file");
		return FAILURE;
	}
	
	// set value
	if(val == 0) {
		write(fd, "0", 2);
	} else {
		write(fd, "1", 2);
	}
	
	// close file
	close(fd);
	
	return SUCCESS;
}

/**
 * Get GPIO value.
 * @param num Linux GPIO number.
 * @return GPIO value, {@link FAILURE} on failure.
 */
int gpio_get_value(int num) {
	
	// open gpio value file
	char file_name[MAX_PATH_LENGTH];
	sprintf(file_name, GPIO_PATH "gpio%d/value",num);
	int fd = open(file_name, O_RDONLY);
	if (fd < 0) {
		rl_log(ERROR, "could not open GPIO value file");
		return FAILURE;
	}
	
	// read value
	char buf[2] = "";
	read(fd, &buf, 1);
	
	// close file
	close(fd);
	
	return atoi(buf);
}

/**
 * Wait on GPIO interrupt.
 * @param num Linux GPIO number.
 * @param timeout Maximum waiting time (in ms). Set to 0 for infinite time out.
 * @return {@link SUCCESS} in case of interrupt, {@link FAILURE} otherwise.
 */
int gpio_wait_interrupt(int num, int timeout) {
	
	// open gpio value file
	char file_name[MAX_PATH_LENGTH];
	sprintf(file_name, GPIO_PATH "gpio%d/value",num);
	int fd = open(file_name, O_RDONLY);
	if (fd < 0) {
		rl_log(ERROR, "could not open GPIO value file");
		return FAILURE;
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
	  rl_log(ERROR, "GPIO poll failed");
	  return FAILURE;
	}
	
	// wait for signal to settle
	usleep(MIN_BUTTON_TIME);
	
	// read value
	lseek(fds.fd, 0, SEEK_SET);
	read(fds.fd, &buf, 1);
	
	close(fd);
	
	return atoi(buf);
}
