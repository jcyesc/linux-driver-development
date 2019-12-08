
/* Net GPIO Driver
 *
 * This driver processes the data from user's space and based on that
 * it turns on and off the leds. This driver is thread safe and uses
 * locks to prevent the data from corruption.
 */

#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/miscdevice.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/types.h>

extern int net_gpio_controller_config_leds(void __iomem *ioremap_addr, struct device *dev);
extern int net_gpio_controller_set_led_brightness(int gpio_net_values[], int num_gpios);

/**
 * File Operations for the NET GPIO Driver.
 */

/* Opens the device "/dev/netgpio" and returns a file descriptor. */
static int net_gpio_open(struct inode *inode, struct file *file) {
	pr_info("net_gpio_open() is executing.\n");

	return 0;
}

/* Closes the device "/dev/netgpio". */
static int net_gpio_close(struct inode *inode, struct file *file) {
	pr_info("net_gpio_close() is executing.\n");

	return 0;
}

/* Writes to the device "/dev/netgpio". */
static ssize_t net_gpio_write(struct file *file, const char __user *buff, size_t count, loff_t *ppos) {
	int net_gpio_values0[8] = {0, 0, 0, 0, 0, 0, 0, 1};
	int net_gpio_values1[8] = {0, 0, 0, 0, 0, 0, 1, 0};
	int net_gpio_values2[8] = {0, 0, 0, 0, 0, 1, 0, 0};
	int net_gpio_values3[8] = {0, 0, 0, 0, 1, 0, 0, 0};
	int net_gpio_values4[8] = {0, 0, 0, 1, 0, 0, 0, 0};
	int net_gpio_values5[8] = {0, 0, 1, 0, 0, 0, 0, 0};
	int net_gpio_values6[8] = {0, 1, 0, 0, 0, 0, 0, 0};
	int net_gpio_values7[8] = {1, 0, 0, 0, 0, 0, 0, 0};

	pr_info("net_gpio_write() is executing.\n");

	net_gpio_controller_set_led_brightness(net_gpio_values0, 8);
	mdelay(200);

	net_gpio_controller_set_led_brightness(net_gpio_values1, 8);
	mdelay(200);

	net_gpio_controller_set_led_brightness(net_gpio_values2, 8);
	mdelay(200);

	net_gpio_controller_set_led_brightness(net_gpio_values3, 8);
	mdelay(200);

	net_gpio_controller_set_led_brightness(net_gpio_values4, 8);
	mdelay(200);

	net_gpio_controller_set_led_brightness(net_gpio_values5, 8);
	mdelay(200);

	net_gpio_controller_set_led_brightness(net_gpio_values6, 8);
	mdelay(200);

	net_gpio_controller_set_led_brightness(net_gpio_values7, 8);
	mdelay(200);

	net_gpio_controller_set_led_brightness(net_gpio_values7, 8);
	mdelay(200);

	net_gpio_controller_set_led_brightness(net_gpio_values6, 8);
	mdelay(200);

	net_gpio_controller_set_led_brightness(net_gpio_values5, 8);
	mdelay(200);

	net_gpio_controller_set_led_brightness(net_gpio_values4, 8);
	mdelay(200);

	net_gpio_controller_set_led_brightness(net_gpio_values3, 8);
	mdelay(200);

	net_gpio_controller_set_led_brightness(net_gpio_values2, 8);
	mdelay(200);

	net_gpio_controller_set_led_brightness(net_gpio_values1, 8);
	mdelay(200);

	net_gpio_controller_set_led_brightness(net_gpio_values0, 8);
	mdelay(200);

	return count;
}

/* Reads from the device "/dev/netgpio". */
static ssize_t net_gpio_read(struct file *file, char __user *buff, size_t count, loff_t *ppos) {
	pr_info("net_gpio_read() is executing.\n");

	return count;
}

/* File operations for the device "/dev/netgpio". */
static const struct file_operations net_gpio_fops = {
	.owner = THIS_MODULE,
	.open = net_gpio_open,
	.release = net_gpio_close,
	.read = net_gpio_read,
	.write = net_gpio_write,
};

/*
 * End the definition of the File Operations for the NET GPIO Driver.
 */

/* Defines the Misc device and sets the file operations. */
static struct miscdevice net_gpio_miscdevice = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "netgpio",
	.fops = &net_gpio_fops,
};

/* Registers the miscellaneous device. */
static int __init net_gpio_probe(struct platform_device *pdev) {
	int ret;
	struct resource *mem_resource;
	unsigned int resource_number = 0;
	void __iomem *ioremap_addr;
	struct device *net_gpio_dev = &pdev->dev;

	pr_info("net_gpio_probe() is executing.\n");

	/* Registers the miscellaneous device. */
	ret = misc_register(&net_gpio_miscdevice);
	if (ret != 0) {
		pr_err("could not register the misc device net_gpio_miscdevice");
		return ret;
	}

	pr_info("netgpio: old minor %i, new minor %i\n",
		MISC_DYNAMIC_MINOR, net_gpio_miscdevice.minor);

	/* Gets the IO memory resource */
	mem_resource = platform_get_resource(pdev, IORESOURCE_MEM, resource_number);
	if (!mem_resource) {
		pr_err("IORESOURCE_MEM is not defined \n");
		return -EINVAL;
	}

	pr_info("mem_resource->start = 0x%08lx\n", (long unsigned int) mem_resource->start);
	pr_info("mem_resource->end = 0x%08lx\n", (long unsigned int) mem_resource->end);

	/* ioremaps the memory region */
	ioremap_addr = devm_ioremap(net_gpio_dev, mem_resource->start, resource_size(mem_resource));
	if (!ioremap_addr) {
		pr_err("dev_ioremap() failed \n");
		return -ENOMEM;
	}

	/* Configures and sets the pins. */
	net_gpio_controller_config_leds(ioremap_addr, net_gpio_dev);

	return 0;
}

/* Deregisters the miscellaneous device. */
static int __exit net_gpio_remove(struct platform_device *pdev) {
	pr_info("net_gpio_remove() function is called.\n");
	misc_deregister(&net_gpio_miscdevice);
	return 0;
}

/* Declare a list of devices supported by the driver */
static const struct of_device_id supported_devices[] = {
	{ .compatible = "custom,netgpio" },
	{},
};

MODULE_DEVICE_TABLE(of, supported_devices);

/* Define platform driver structure */
static struct platform_driver net_gpio_platform_driver = {
	.probe = net_gpio_probe,
	.remove = net_gpio_remove,
	.driver = {
		.name = "custom,netgpio",
		.of_match_table = supported_devices,
		.owner = THIS_MODULE,
	},
};


/* Register the platform driver */
module_platform_driver(net_gpio_platform_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Juan Yescas");
MODULE_DESCRIPTION("Net GPIO platform driver to control pin output.");

