#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#include "types.h"

#define GPIO_PATH "/sys/class/gpio/"
#define MIN_BUTTON_TIME 100000 // minimal time a button needs to be pressed (in us)

enum direction {IN, OUT};
enum edge {NONE, RISING, FALLING, BOTH};

// gpio unexport
int gpio_unexport(int num);

// gpio export
int gpio_export(int num);

// set direction
int gpio_dir(int num, enum direction dir);

// gpio value
int gpio_set_value(int num, int val);
int gpio_get_value(int num);

// interrupt
int gpio_interrupt(int num, enum edge e);
int gpio_wait_interrupt(int num, int timeout); // timout<0 -> infinite
