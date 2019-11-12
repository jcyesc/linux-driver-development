/*
 * In this kernel module we will create a device node using the
 * Miscellaneous Framework instead of doing it manually with the "mknod" command or
 * using the devtmpfs.
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>

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

/* End of file_operations function implementations.*/

static const struct file_operations my_dev_fops = {
	.owner = THIS_MODULE,
	.open = my_dev_open,
	.release = my_dev_close,
	.unlocked_ioctl = my_dev_ioctl,
};

/* Declare & initialize struct miscdevice */
static struct miscdevice helloworld_miscdevice = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "my_misc_dev",
	.fops = &my_dev_fops,
};

static int __init hello_init(void) {
	int ret;
	pr_info("Hello world Miscellaneous Framework\n");

	ret = misc_register(&helloworld_miscdevice);
	if(ret != 0) {
		pr_err("could not register the misc device my_misc_dev");
		return ret;
	}

	pr_info("my_misc_dev: got minor     %i\n", helloworld_miscdevice.minor);
	pr_info("my_misc_dev: got node name %s\n", helloworld_miscdevice.nodename);

	return 0;
}

static void __exit hello_exit(void) {
	pr_info("Hello world exit\n");

	/* Unregister the device with the kernel. */
	misc_deregister(&helloworld_miscdevice);
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Juan Yescas");
MODULE_DESCRIPTION("This is the helloworld_misc_char_driver using the Miscellaneous Framework!!");


