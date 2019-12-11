
/*
 * Net GPIO Driver
 *
 *  Process the user's input so given a char array, the leds will be
 *  turned on/off. The characters will be processed in multiples of 8.
 *
 *  For example:
 *
 *  echo '10101010 01100110' > /dev/netgpio
 *
 *  will display the first 8 characters and then the last 8 characters.
 *
 *  This driver is thread safe and uses locks to prevent the data from corruption.
 *
 *  Note: The driver doesn't lock at the led level. If it is required to support
 *  lock at led level, it is necessary to add it in the net_gpio_controller.c file.
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
#include <linux/uaccess.h>  /* copy_from_user(), copy_to_user() */
#include <linux/spinlock.h>
#include <linux/spinlock_types.h>

#include "net_gpio_controller.h"

/*
 * Spin lock to protect the messages from being corrupted by another
 * request.
 */
static DEFINE_SPINLOCK(net_gpio_lock);

#define NET_GPIO_MAX_MESSAGE_SIZE 1024
/* Keeps the message sent by the user. */
static char net_gpio_message[NET_GPIO_MAX_MESSAGE_SIZE + 1];
/* Default values to clean the led (pins) in the net_gpio_controller. */
static const int net_gpio_default_values[NET_GPIO_MAX_MESSAGE_SIZE] = {0, 0, 0, 0, 0, 0, 0, 0};

static u64 total_messages_processed = 0;

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

static inline void net_gpio_process_message(size_t count) {
	int net_gpio_values[NUM_NET_GPIOS];
	int index;
	int processed_values;

	processed_values = 0;
	for (index = 0; index < count; index++) {
		char value = net_gpio_message[index];
		if (value == '0' || value == '1') {
			net_gpio_values[processed_values % NUM_NET_GPIOS] = value == '1' ? 1 : 0;
			processed_values++;

			if (processed_values % NUM_NET_GPIOS == 0 && processed_values != 0) {
				net_gpio_controller_set_led_brightness(net_gpio_values, NUM_NET_GPIOS);
				mdelay(200);
			}
		}
	}

	/*
	 * If there are remaining processed values that were not sent to the controller
	 * because they were not multiple of NUM_NET_GPIOS, we fill with ZERO the rest and
	 * send them to the controller.
	 */
	if (processed_values % NUM_NET_GPIOS != 0) {
		for (index = processed_values % NUM_NET_GPIOS; index < NUM_NET_GPIOS; index++) {
			net_gpio_values[index] = 0;
		}
		net_gpio_controller_set_led_brightness(net_gpio_values, NUM_NET_GPIOS);
		mdelay(200);
	}

	total_messages_processed++;
}

/* Writes to the device "/dev/netgpio". */
static ssize_t net_gpio_write(struct file *file, const char __user *buff, size_t count, loff_t *ppos) {
	pr_info("net_gpio_write() is executing.\n");

	if (count >= NET_GPIO_MAX_MESSAGE_SIZE) {
		pr_info("The number of bits to copy [%d] is greater than %d", count, NET_GPIO_MAX_MESSAGE_SIZE);
		return -EFAULT;
	}

	spin_lock(&net_gpio_lock);

	if(copy_from_user(net_gpio_message, buff, count)) {
		pr_info("Bad copied value\n");
		return -EFAULT;
	}

	/* Making sure that the messages always have NULL at the end. */
	net_gpio_message[count] = '\0';
	pr_info("Message received %s\n", net_gpio_message);

	net_gpio_process_message(count);
	net_gpio_controller_set_led_brightness((int *) net_gpio_default_values, NUM_NET_GPIOS);

	spin_unlock(&net_gpio_lock);

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

