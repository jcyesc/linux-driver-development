/*
 * In this kernel module we will create a device node using devtmpfs
 * instead of doing it manually with the "mknod" command.
 *
 * An entry to the /sys/class directory will be added. This directory
 * offers a view of the device drivers grouped by class.
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/types.h>  /* dev_t is defined here */

#define DEVICE_NAME "my_char_class_dev"
#define CLASS_NAME "my_custom_char_class"

static struct class* hello_class;
static struct cdev my_dev;
static dev_t dev;
static unsigned count_minor = 1;

/*
 * file_operation function implementations.
 */
static int my_dev_open(struct inode *inode, struct file *file) {
	pr_info("my_dev_open() is called.\n");
	return 0;
}

static int my_dev_close(struct inode *inode, struct file *file) {
	pr_info("my_dev_close() is called.\n");
	return 0;
}

static long my_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
	pr_info("my_dev_ioctl() is called with cmd = %d, arg = %ld\n", cmd, arg);
	return 0;
}

/* Declare a file_operations structure */
static const struct file_operations my_dev_fops = {
		.owner = THIS_MODULE,
		.open = my_dev_open,
		.release = my_dev_close,
		.unlocked_ioctl = my_dev_ioctl,
};

/*
 * Return 0 if success, otherwise a number different than zero.
 */
static int __init hello_init(void) {
	int ret;
	dev_t dev_no;
	int major_no;
	unsigned minor_no = 0; /* First of the requested range of minor numbers. */

	struct device* hello_device;

	pr_info("Hello world class driver init ....\n");

	/*
	 * Allocates a range of char device numbers.  The major number will be
	 * chosen dynamically, and returned (along with the first minor number)
	 * in @dev_no.  Returns zero or a negative error code.
	 */
	ret = alloc_chrdev_region(&dev_no, minor_no, count_minor, DEVICE_NAME);
	if (ret < 0) {
		pr_info("Unable to allocate Major number...\n");
		return ret;
	}

	/*
	 * Get the device identifiers. We could use "dev_no", however this is done
	 * only for educational purposes.
	 */
	major_no = MAJOR(dev_no);
	dev = MKDEV(major_no, minor_no);

	pr_info("Allocated correctly with major number %d\n", major_no);

	/* Initialize the cdev structure and add it to the kernel space. */
	cdev_init(&my_dev, &my_dev_fops);
	ret = cdev_add(&my_dev, dev, count_minor);
	if (ret < 0) {
		unregister_chrdev_region(dev, 1);
		pr_info("Unable to add cdev\n");
		return ret;
	}

	/* Register the device class. */
	hello_class = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(hello_class)) {
		unregister_chrdev_region(dev, count_minor);
		cdev_del(&my_dev);

		pr_info("Failed to register device class\n");
		return PTR_ERR(hello_class);
	}

	pr_info("Device class registered correctly\n");

	/* Create a device node named DEVICE_NAME associated a dev */
	hello_device = device_create(hello_class, NULL, dev, NULL, DEVICE_NAME);
	if (IS_ERR(hello_device)) {
		class_destroy(hello_class);
		unregister_chrdev_region(dev, count_minor);
		cdev_del(&my_dev);

		pr_info("Failed to create the device\n");
		return PTR_ERR(hello_device);
	}

	pr_info("The device is created correctly\n");

	return 0;
}

static void __exit hello_exit(void) {
	device_destroy(hello_class, dev);
	class_destroy(hello_class);
	unregister_chrdev_region(dev, count_minor);
	cdev_del(&my_dev);

	pr_info("Hello world class driver exit ....\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Juan Yescas");
MODULE_DESCRIPTION("This is a module that register a class and a"
		" device and interacts with the ioctl system call");

