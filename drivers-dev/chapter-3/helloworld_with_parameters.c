
#include <linux/module.h>

static int num = 5;

/* S_IRUGO: everyone can read the sysfs entry. */
module_param(num, int, S_IRUGO);

static int __init hello_init(void) {
	pr_info("parameter num = %d\n", num);
	return 0;
}

static void __init hello_exit(void) {
	pr_info("Hello world with parameter exit \n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Juan Yescas");
MODULE_DESCRIPTION("A module that accepts parameters");

