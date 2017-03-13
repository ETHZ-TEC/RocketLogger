#include "types.h"
#include "util.h"
#include "gpio.h"
#include "pwm.h"
#include "pru.h"

/// Linux GPIO number for forcing I1 high
#define FHR1_GPIO 30
/// Linux GPIO number for forcing I2 high
#define FHR2_GPIO 60
/// Linux GPIO number of status LED
#define LED_STATUS_GPIO 45
/// Linux GPIO number of error LED
#define LED_ERROR_GPIO 44

void hw_init(struct rl_conf* conf);
void hw_close(struct rl_conf* conf);
int hw_sample(struct rl_conf* conf);
