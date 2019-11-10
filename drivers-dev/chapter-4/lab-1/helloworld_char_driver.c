
#include <linux/module.h>

/* Header files to support character devices. */
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/types.h>

/* Define a major number */
#define MY_MAJOR_NUM 202

static struct cdev my_char_dev;

/*
 * File Operations
 *
 * Implementation for the File Operations open(), close() and ioctl().
 */

/* Implements the open() callback. */
static int my_char_dev_open(struct inode *inode, struct file *file) {
	pr_info("my_char_dev_open() is called.\n");
	return 0;
}

/* Implements the close() callback. */
static int my_char_dev_close(struct inode *indoe, struct file *file) {
	pr_info("my_char_dev_close() is called.\n");
	return 0;
}

/* Implements the ioctl() callback. */
static long my_char_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
	pr_info("my_char_dev_ioctl() is called. cmd = %d, arg = %ld\n", cmd, arg);
	return 0;
}

/*
 * End of the File Operations implementation.
 */

/* Declare a file operations structure */
static const struct file_operations my_char_dev_fops = {
	.owner = THIS_MODULE,
	.open = my_char_dev_open,
	.release = my_char_dev_close,
	.unlocked_ioctl = my_char_dev_ioctl,
};

static int __init hello_init(void) {
	int ret;
	/* MKDEV macro is used to get the major and minor number. */
	dev_t dev = MKDEV(MY_MAJOR_NUM, 0);
	unsigned num_of_char_devices = 1; /* Number of consecutive device numbers. */

	pr_info("Hello world char driver init()\n");

	/* Allocate device numbers */
	ret = register_chrdev_region(dev, num_of_char_devices, "my_char_device");
	if (ret < 0) {
		pr_info("Unable to allocate major number %d\n", MY_MAJOR_NUM);
		return ret;
	}

	/* Initialize the cdev structure and add it to the kernel space. */
	cdev_init(&my_char_dev, &my_char_dev_fops);
	ret = cdev_add(&my_char_dev, dev, 1 /* number of consecutive minor numbers */);
	if (ret < 0) {
		unregister_chrdev_region(dev, num_of_char_devices);
		pr_info("Unable to add the char device to cdev\n");
		return ret;
	}

	return 0;
}

static void __exit hello_exit(void) {
	pr_info("Hello world char driver exit()\n");
	cdev_del(&my_char_dev);
	unregister_chrdev_region(MKDEV(MY_MAJOR_NUM, 0), 1 /* number of char devices*/);
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Juan Yescas");
MODULE_DESCRIPTION("This is driver for a custom character device. It defines several file_operations to interact.");


