
/*
 * LED UIO Platform device driver
 *
 * In this driver, we will develop a UIO user space driver that controls a LED. The
 * main function of an UIO driver is to expose the hardware registers to user space and
 * does nothing within kernel space to control them. The LED will be controlled directly
 * from the UIO user space driver by accessing to the memory mapped registers of the
 * device. This kernel driver will obtain the register addresses from the device tree and
 * initializes the struct uio_info with these parameters. We will also register the UIO
 * device within the prove() function of the kernel driver.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/uio_driver.h>

struct uio_info the_uio_info;

/**
 * Populates the struct uio_info and associate it with struct device
 * using the function uio_register_device();
 */
static int __init my_probe(struct platform_device *pdev)
{
	int ret;
	struct resource *resource;
	struct device *dev = &pdev->dev;
	void __iomem *ioremap_addr;

	dev_info(dev, "my_probe() enter \n");

	/* Get the first memory resource from device tree */
	resource = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!resource) {
		dev_err(dev, "IORESOURCE_MEM, 0 does not exist\n");
		return -EINVAL;
	}
	dev_info(dev, "resource->start = 0x%08lx\n", (long unsigned int) resource->start);
	dev_info(dev, "resource->end = 0x%08lx\n", (long unsigned int) resource->end);

	/* ioremap our memory region and get virtual address */
	ioremap_addr = devm_ioremap(dev, resource->start, resource_size(resource));
	if (!ioremap_addr) {
		dev_err(dev, "devm_ioremap() failed \n");
		return -ENOMEM;
	}

	/* initialize uio_info struct uio_mem array */
	the_uio_info.name = "led_uio";
	the_uio_info.version = "1.0";
	the_uio_info.mem[0].memtype = UIO_MEM_PHYS;
	the_uio_info.mem[0].addr = resource->start; /* physical address needed for the kernel user mapping */
	the_uio_info.mem[0].size = resource_size(resource);
	the_uio_info.mem[0].name = "demo_uio_driver_hw_region";
	the_uio_info.mem[0].internal_addr = ioremap_addr; /* virtual address for internal driver use */

	/* register the uio device */
	ret = uio_register_device(&pdev->dev, &the_uio_info);
	if (ret != 0) {
		dev_info(dev, "Could not register device `led_uio`...");
	}

	dev_info(dev, "my_probe() exit \n");

	return 0;
}

static int __exit my_remove(struct platform_device *pdev)
{
	uio_unregister_device(&the_uio_info);
	dev_info(&pdev->dev, "my_remove() exit\n");

	return 0;
}

static const struct of_device_id my_of_ids[] = {
	{ .compatible = "arrow,UIO" },
	{},
};

MODULE_DEVICE_TABLE(of, my_of_ids);

static struct platform_driver my_platform_driver = {
	.probe = my_probe,
	.remove = my_remove,
	.driver = {
		.name = "my_uio_platform_driver",
		.of_match_table = my_of_ids,
		.owner = THIS_MODULE,
	}
};

module_platform_driver(my_platform_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Juan Yescas");
MODULE_DESCRIPTION("This is a UIO platform driver that turns \
		the LED on/off without using system calls");

