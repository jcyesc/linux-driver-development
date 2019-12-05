
#include <linux/module.h>

void net_gpio_controller_process(void) {
	pr_info("net_gpio_controller_process() is executing.\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Juan Yescas");
MODULE_DESCRIPTION("Net GPIO platform driver to control pin output.");


