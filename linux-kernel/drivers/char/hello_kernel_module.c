/*
 * This file defines a kernel module that is part of the kernel source tree
 * and have the driver built into the kernel binary image. In this way, the driver
 * is already loaded when the new kernel is booted.
 */

#include <linux/module.h>
#include <linux/time.h>

static struct timeval start_time;


static int hello_init(void) {
	do_gettimeofday(&start_time);
	pr_info("Loading the hello_kernel_module at %ld seconds ...\n",
			start_time.tv_sec);

	return 0;
}

static void hello_exit(void) {
	struct timeval end_time;
	do_gettimeofday(&end_time);
	pr_info("Unloading the hello_kernel module after %ld seconds ...\n",
			end_time.tv_sec - start_time.tv_sec);
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Juan Yescas");
MODULE_DESCRIPTION("Module that is loaded during booting time and keeps track of the seconds that has been up");

