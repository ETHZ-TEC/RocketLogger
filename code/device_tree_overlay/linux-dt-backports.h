/*
 * Backport of AM33XX pinctrl bindings used in Linux mainline kernel.
 * http://kernel.org/
 */

/* GPIO pin modes, check user manual for pin specific mapping */
#define MUX_MODE0               0X00
#define MUX_MODE1               0X01
#define MUX_MODE2               0X02
#define MUX_MODE3               0X03
#define MUX_MODE4               0X04
#define MUX_MODE5               0x05
#define MUX_MODE6               0x06
#define MUX_MODE7               0x07

/* GPIO pin configuration */
#define PULL_DISABLE            (1 << 3)
#define PULL_UP                 (1 << 4)
#define INPUT_EN                (1 << 5)

#define PIN_OUTPUT              (PULL_DISABLE)
#define PIN_OUTPUT_PULLUP       (PULL_UP)
#define PIN_OUTPUT_PULLDOWN     0
#define PIN_INPUT               (INPUT_EN | PULL_DISABLE)
#define PIN_INPUT_PULLUP        (INPUT_EN | PULL_UP)
#define PIN_INPUT_PULLDOWN      (INPUT_EN)

/* PWM related defines */
#define PWM_POLARITY_INVERTED   (1 << 0)
