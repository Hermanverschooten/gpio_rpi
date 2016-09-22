#ifndef GPIO_PORT_H
#define GPIO_PORT_H
/*
 * GPIO handling definitions and prototypes
 */
enum gpio_state {
    GPIO_OUTPUT,
    GPIO_INPUT,
    GPIO_INPUT_WITH_INTERRUPTS
};

struct gpio {
    enum gpio_state state;
    int fd;
    int pin_number;
};
#endif
