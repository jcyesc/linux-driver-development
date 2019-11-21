/*
 * This file will install 1 platform device that contains 3 nodes (red, green and blue_.
 * It will use the LED subsystem so the read() and write() functions don't need to be implemented.
 *
 * The sequence of steps is the following:
 *
 * 1. Kernel reads the DT (bcm2710-rpi-b.dts) to include the configuration for LEDs.
 * 2. After insmod led_rgb_class_platform_driver is executed, the platform driver is loaded.
 * 3. The probe() method is called 1 time.
 * 4. For every node in the device (ledclassRGB), we create a led_classdev and led_dev object
 *    that contains configuration information such as register, status (on/off). The devm_led_classdev_register
 *    method is called three times to register the 3 led_classdev objects created. In addition to
 *    this, the led_control() callback function is assigned to led_classdev.brightness_set = led_control;
 * 5. When echo 1 > /sys/class/leds/green/brightness is executed, it sets the led green ON.
 */


#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/leds.h>

#define GPIO_27			27
#define GPIO_22			22
#define GPIO_26			26

#define GPFSEL2_offset 	 0x08
#define GPSET0_offset    0x1C
#define GPCLR0_offset 	 0x28

/* to set and clear each individual LED */
#define GPIO_27_INDEX 	1 << (GPIO_27 % 32)
#define GPIO_22_INDEX 	1 << (GPIO_22 % 32)
#define GPIO_26_INDEX 	1 << (GPIO_26 % 32)

/* select the output function */
#define GPIO_27_FUNC	1 << ((GPIO_27 % 10) * 3)
#define GPIO_22_FUNC 	1 << ((GPIO_22 % 10) * 3)
#define GPIO_26_FUNC 	1 << ((GPIO_26 % 10) * 3)

#define FSEL_27_MASK 	0b111 << ((GPIO_27 % 10) * 3) /* red since bit 21 (FSEL27) */
#define FSEL_22_MASK 	0b111 << ((GPIO_22 % 10) * 3) /* green since bit 6 (FSEL22) */
#define FSEL_26_MASK 	0b111 << ((GPIO_26 % 10) * 3) /* blue since bit 18 (FSEL26) */

#define GPIO_SET_FUNCTION_LEDS (GPIO_27_FUNC | GPIO_22_FUNC | GPIO_26_FUNC)
#define GPIO_MASK_ALL_LEDS 	(FSEL_27_MASK | FSEL_22_MASK | FSEL_26_MASK)
#define GPIO_SET_ALL_LEDS (GPIO_27_INDEX  | GPIO_22_INDEX  | GPIO_26_INDEX)

/* Represents the led device. */
struct led_dev {
	u32 led_mask; /* different mask if led is R, G or B */
	void __iomem *base;
	struct led_classdev cdev;
};

/* Sets the brightness of the led represented by {@code led_dev}. */
static void led_control(struct led_classdev *led_cdev, enum led_brightness b)
{
	struct led_dev *led = container_of(led_cdev, struct led_dev, cdev);

	pr_info("led_control() executing for the class %s", led->cdev.name);

	iowrite32(GPIO_SET_ALL_LEDS, led->base + GPCLR0_offset);

	if (b != LED_OFF) {
		/* LED ON */
		iowrite32(led->led_mask, led->base + GPSET0_offset);
	} else {
		iowrite32(led->led_mask, led->base + GPCLR0_offset); /* LED OFF */
	}
}

/*
 * The probe() function performs the following actions:
 *
 * 1. The platform_get_resource() function gets the I/O registers resource describe by the
 *    DT reg property.
 * 2. The dev_ioremap() functions maps the area of register addresses to kernel's virtual
 *    address.
 * 3. The for_each_child_of_node() function walks for each sub-node of the main node
 *    allocating a private structure for each one by using the devm_kzalloc() function, then
 *    initializes the struct led_classdev field of each allocated private structure.
 * 4. The devm_led_classdev_register() function registers each LED class device to the LED
 *    subsystem.
 */
static int __init ledclass_probe(struct platform_device *pdev)
{
	void __iomem *g_ioremap_addr;
	struct device_node *child;
	struct resource *resource;
	u32 GPFSEL_read;
	u32 GPFSEL_write;
	struct device *dev = &pdev->dev;
	int ret = 0;
	int child_count;
	unsigned int resouce_number = 0;

	pr_info("ledclass_probe() executing for platform_device %s", pdev->name);

	/* get our first memory resource from platform device */
	resource = platform_get_resource(pdev, IORESOURCE_MEM, resouce_number);
	if (!resource) {
		pr_err("IORESOURCE_MEM, 0 does not exist\n");
		return -EINVAL;
	}

	pr_info("r->start = 0x%08lx\n", (long unsigned int) resource->start);
	pr_info("r->end = 0x%08lx\n", (long unsigned int) resource->end);

	/* ioremap our memory region. */
	g_ioremap_addr = devm_ioremap(dev, resource->start, resource_size(resource));
	if (!g_ioremap_addr) {
		pr_err("ioremap failed \n");
		return -ENOMEM;
	}

	child_count = of_get_child_count(dev->of_node);
	if (!child_count)
		return -EINVAL;

	pr_info("there are %d nodes\n", child_count);

	/* Start: Configure LED registers as output and clear all the leds, so output is low */
	GPFSEL_read = ioread32(g_ioremap_addr + GPFSEL2_offset); /* read actual value */

	GPFSEL_write = (GPFSEL_read & ~GPIO_MASK_ALL_LEDS) |
					  (GPIO_SET_FUNCTION_LEDS & GPIO_MASK_ALL_LEDS);

	iowrite32(GPFSEL_write, g_ioremap_addr + GPFSEL2_offset); /* set dir leds to output */
	iowrite32(GPIO_SET_ALL_LEDS, g_ioremap_addr + GPCLR0_offset); /* Clear all the leds, output is low */
	/* End: Configure LED registers as output and clear all the leds, so output is low */

	for_each_child_of_node(dev->of_node, child) {

		struct led_dev *led_device;
		struct led_classdev *class_dev;
		/*
		 * The allocated memory is associated with the device and release automatically
		 * when the device is close.
		 */
		led_device = devm_kzalloc(dev, sizeof(*led_device), GFP_KERNEL);
		if (!led_device)
			return -ENOMEM;

		class_dev = &led_device->cdev;

		led_device->base = g_ioremap_addr;

		of_property_read_string(child, "label", &class_dev->name);

		if (strcmp(class_dev->name, "red") == 0) {
			led_device->led_mask = GPIO_27_INDEX;
			led_device->cdev.default_trigger = "heartbeat";
		}
		else if (strcmp(class_dev->name, "green") == 0) {
			led_device->led_mask = GPIO_22_INDEX;
		}
		else if (strcmp(class_dev->name, "blue") == 0) {
			led_device->led_mask = GPIO_26_INDEX;
		}
		else {
			pr_info("Bad device tree value\n");
			return -EINVAL;
		}

		/* Disable timer trigger until led is on */
		led_device->cdev.brightness = LED_OFF;
		led_device->cdev.brightness_set = led_control;

		ret = devm_led_classdev_register(dev, &led_device->cdev);
		if (ret) {
			dev_err(dev, "failed to register the led %s\n", class_dev->name);
			of_node_put(child);
			return ret;
		}
	}

	pr_info("leds_probe() exit\n");

	return 0;
}

static int __exit ledclass_remove(struct platform_device *pdev)
{
	pr_info("leds_remove() enter\n");
	pr_info("leds_remove() exit\n");

	return 0;
}

static const struct of_device_id my_of_ids[] = {
	{ .compatible = "arrow,RGBclassleds"},
	{},
};

MODULE_DEVICE_TABLE(of, my_of_ids);

static struct platform_driver led_platform_driver = {
	.probe = ledclass_probe,
	.remove = ledclass_remove,
	.driver = {
		.name = "RGBclassleds",
		.of_match_table = my_of_ids,
		.owner = THIS_MODULE,
	}
};

static int led_rgb_class_init(void)
{
	int ret;
	pr_info("led_rgb_class_init() enter\n");

	ret = platform_driver_register(&led_platform_driver);
	if (ret !=0) {
		pr_err("platform value returned %d\n", ret);
		return ret;
	}

	pr_info("led_rgb_class_init() exit\n");
	return 0;
}

static void led_rgb_class_exit(void)
{
	pr_info("led_rgb_class_exit() enter\n");

	platform_driver_unregister(&led_platform_driver);

	pr_info("led_rgb_class_exit() exit\n");
}

module_init(led_rgb_class_init);
module_exit(led_rgb_class_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Juan Yescas");
MODULE_DESCRIPTION("This is a driver that turns on/off RGB leds \
		   using the LED subsystem");

