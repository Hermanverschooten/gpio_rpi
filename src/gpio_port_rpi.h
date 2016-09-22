#ifndef GPIO_PORT_RPI_H
#define GPIO_PORT_RPI_H

void init_gpio_mem(void);
void gpio_pullup(struct gpio *pin, char *mode_str);

#endif
