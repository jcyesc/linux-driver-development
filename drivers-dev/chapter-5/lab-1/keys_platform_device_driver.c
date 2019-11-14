
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/mod_devicetable.h>

/* file_operations function implementations */

static int my_dev_open(struct inode *inode, struct file *file) {
	pr_info("my_dev_open() is called.\n");
	return 0;
}

static int my_dev_close(struct inode *inode, struct file *file) {
	pr_info("my_dev_close() is called.\n");
	return 0;
}

static long my_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
	pr_info("my_dev_ioctl() is called. cmd = %d, arg = %ld\n", cmd, arg);
	return 0;
}

/* End of file_operations function implementations */

static const struct file_operations my_dev_fops = {
	.owner = THIS_MODULE,
	.open = my_dev_open,
	.release = my_dev_close,
	.unlocked_ioctl = my_dev_ioctl,
};

static struct miscdevice keys_miscdevice = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "keys_dev",
	.fops = &my_dev_fops,
};

/* Add probe() function */
static int __init my_probe(struct platform_device *pdev) {
	int ret;
	pr_info("my_probe() function is called.\n");
	ret = misc_register(&keys_miscdevice);

	if (ret != 0) {
		pr_err("could not register the misc device keys_dev");
		return ret;
	}

	pr_info("keys_dev: old minor %i, new minor %i\n",
		MISC_DYNAMIC_MINOR, keys_miscdevice.minor);
	return 0;
}

/* Add remove() function */
static int __exit my_remove(struct platform_device *pdev) {
	pr_info("my_remove() function is called.\n");
	misc_deregister(&keys_miscdevice);
	return 0;
}

/* Declare a list of devices supported by the driver */
static const struct of_device_id my_of_ids[] = {
	{ .compatible = "arrow,hellokeys"},
	{},
};

MODULE_DEVICE_TABLE(of, my_of_ids);

/* Define platform driver structure */
static struct platform_driver my_platform_driver = {
	.probe = my_probe,
	.remove = my_remove,
	.driver = {
		.name ="keys_driver",
		.of_match_table = my_of_ids,
		.owner = THIS_MODULE,
	},
};

/* Register our platform driver. */
module_platform_driver(my_platform_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Juan Yescas");
MODULE_DESCRIPTION("This is a keys platform driver.");

