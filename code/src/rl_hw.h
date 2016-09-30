#include "types.h"
#include "gpio.h"
#include "pwm.h"

#define FHR1_GPIO 30
#define FHR2_GPIO 60
#define LED_STATUS_GPIO 45
#define LED_ERROR_GPIO 44

int hw_init(struct rl_conf_new* conf);
int hw_close();