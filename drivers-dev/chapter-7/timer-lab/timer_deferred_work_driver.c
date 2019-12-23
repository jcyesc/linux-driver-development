/*
 * Deferring work using timers
 *
 * This driver will schedule a task every 1000ms (1 second).
 * It will use timer_setup() and mod_timer() to configure the timer.
 *
 * In addition to this, it will define an attribute to control the
 * delays of the timer. See set_period() and DEVICE_ATTR().
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/timer.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>

static struct timer_list work_timer;
static unsigned int timer_period_ms = 1000;

/* Handler function that will be executed every time that time expired. */
static void work_timer_handler(struct timer_list *unused)
{
	static bool on = false;
	unsigned long next;

	on = !on;

	pr_info("work_timer_handler() Work timer at %lu jiffies. Status {%d}", jiffies, on);

	next = jiffies + msecs_to_jiffies(timer_period_ms);
	mod_timer(&work_timer, next);
}

/* Updates the {@code timer_period_ms} with the value that is written in
 * device node attribute.
 */
static ssize_t set_period(struct device* dev,
		struct device_attribute* attr,
		const char* buf,
		size_t count)
{
	unsigned int base = 10;
	long period_value_ms = 0;

	pr_info("set_period() is executing\n");

	/* kstrtol() converts a string to long */
	if (kstrtol(buf, base, &period_value_ms) < 0) {
		return -EINVAL;
	}
	if (period_value_ms < 10) {
		return -EINVAL;
	}

	timer_period_ms = (unsigned int) period_value_ms;
	pr_info("set_period() the new timer_period_ms is %d\n", timer_period_ms);

	return count;
}

/*
 * Defines a device atribute variable (dev_attr_period) that contains the set function
 * that will update the value.
 */
static DEVICE_ATTR(period, S_IWUSR, NULL, set_period);

static struct miscdevice timer_work_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "timer_deferred_work_misc_dev",
};

static int __init platform_device_probe(struct platform_device *pdev)
{
	int result;
	int ret;
	unsigned long next;
	struct device *dev = &pdev->dev;

	dev_info(dev, "platform_probe() is being executed\n");

	/* Set it up the timer. */
	timer_setup(&work_timer, work_timer_handler, 0 /* flags */);
	next = jiffies + msecs_to_jiffies(timer_period_ms);
	result = mod_timer(&work_timer, next);

	/* Creates device node in /sys/devices/platform/soc/soc\:timer_work_dts/period  */
	ret = device_create_file(dev, &dev_attr_period /* Defined in DEVICE_ATTR */);
	if (ret != 0) {
		dev_err(dev, "Failed to create the sysfs entry\n");
		return ret;
	}

	ret = misc_register(&timer_work_misc_device);
	if (ret != 0) {
		dev_err(dev, "could not register the misc device timer_deferred_work_misc_dev");
		return ret;
	}
	dev_info(dev, "timer_deferred_work_misc_dev: got minor %i\n", timer_work_misc_device.minor);

	return 0;
}

static int __exit platform_device_remove(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "platform_remove() is executing\n");
	del_timer(&work_timer);
	device_remove_file(&pdev->dev, &dev_attr_period);
	misc_deregister(&timer_work_misc_device);
	dev_info(&pdev->dev, "platform_remove() has completed\n");

	return 0;
}

static const struct of_device_id my_of_ids[] = {
	{ .compatible = "timer_deferred_work_compatible" },
	{},
};

static struct platform_driver timer_work_driver = {
	.probe = platform_device_probe,
	.remove = platform_device_remove,
	.driver = {
		.name = "timer_deferred_work_platform_driver",
		.of_match_table = my_of_ids,
		.owner = THIS_MODULE,
	},
};

module_platform_driver(timer_work_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Juan Yescas");
MODULE_DESCRIPTION("Platform Driver that uses a timer to defer the work");


