
#ifndef NETGPIO_NET_GPIO_CONTROLLER_H_
#define NETGPIO_NET_GPIO_CONTROLLER_H_

#define NUM_NET_GPIOS 8

int net_gpio_controller_config_leds(void __iomem *ioremap_addr, struct device *dev);

int net_gpio_controller_set_led_brightness(int gpio_net_values[], int num_gpios);

#endif /* NETGPIO_NET_GPIO_CONTROLLER_H_ */
