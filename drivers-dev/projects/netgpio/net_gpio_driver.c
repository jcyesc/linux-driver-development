
/* Net GPIO Driver
 *
 * This driver processes the data from user's space and based on that
 * it turns on and off the leds. This driver is thread safe and uses
 * locks to prevent the data from corruption.
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/mod_devicetable.h>
#include <linux/types.h>

extern void net_gpio_controller_process(void);

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
	pr_info("net_gpio_write() is executing.\n");

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
	pr_info("net_gpio_probe() is executing.\n");
	ret = misc_register(&net_gpio_miscdevice);

	if (ret != 0) {
		pr_err("could not register the misc device net_gpio_miscdevice");
		return ret;
	}

	pr_info("netgpio: old minor %i, new minor %i\n",
		MISC_DYNAMIC_MINOR, net_gpio_miscdevice.minor);

	net_gpio_controller_process();


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

