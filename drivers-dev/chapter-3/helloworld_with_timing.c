
#include <linux/module.h>
#include <linux/time.h>

/**
 * This modules tracks the time when the module was
 * started and removed and print that information.
 */

static int num = 10;
static struct timeval start_time;

/* S_IRUGO: everyone can read the sysfs entry. */
module_param(num, int, S_IRUGO);

static void say_hello(void) {
	int i;
	for (i = 1; i <= num; i++) {
		pr_info("[%d/%d] Hello!\n", i, num);
	}
}

static int __init first_init(void) {
	do_gettimeofday(&start_time);
	pr_info("Loading first!\n");
	say_hello();

	return 0;
}

static void __exit first_exit(void) {
	struct timeval end_time;
	do_gettimeofday(&end_time);
	pr_info("Unloading module after %ld seconds\n",
			end_time.tv_sec - start_time.tv_sec);
	say_hello();
}

module_init(first_init);
module_exit(first_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Juan Yescas");
MODULE_DESCRIPTION("Module that keeps track of the seconds that has been up");


