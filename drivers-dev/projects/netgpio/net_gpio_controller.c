
#include <linux/io.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/stddef.h>
#include <linux/types.h>

#define GPIO_22			22
#define GPIO_26			26
#define GPIO_27			27

#define GPFSEL2_offset 	 0x08
#define GPSET0_offset    0x1C
#define GPCLR0_offset 	 0x28

/* to set and clear each individual LED */
#define GPIO_22_INDEX 	1 << (GPIO_22 % 32)
#define GPIO_26_INDEX 	1 << (GPIO_26 % 32)
#define GPIO_27_INDEX 	1 << (GPIO_27 % 32)

/* select the output function */
#define GPIO_22_FUNC 	1 << ((GPIO_22 % 10) * 3)
#define GPIO_27_FUNC	1 << ((GPIO_27 % 10) * 3)
#define GPIO_26_FUNC 	1 << ((GPIO_26 % 10) * 3)

#define FSEL_22_MASK 	0b111 << ((GPIO_22 % 10) * 3) /* green since bit 6 (FSEL22) */
#define FSEL_27_MASK 	0b111 << ((GPIO_27 % 10) * 3) /* red since bit 21 (FSEL27) */
#define FSEL_26_MASK 	0b111 << ((GPIO_26 % 10) * 3) /* blue since bit 18 (FSEL26) */

#define GPIO_SET_FUNCTION_LEDS (GPIO_22_FUNC | GPIO_27_FUNC | GPIO_26_FUNC)
#define GPIO_MASK_ALL_LEDS 	(FSEL_22_MASK | FSEL_27_MASK | FSEL_26_MASK)
#define GPIO_SET_ALL_LEDS (GPIO_22_INDEX | GPIO_27_INDEX | GPIO_26_INDEX)

const int GPIO_INDEX_MASKS[3] = {
	GPIO_22_INDEX,
	GPIO_26_INDEX,
	GPIO_27_INDEX
};

/* Represents the led device. */
struct led_dev {
	u32 mask; /* different mask for the leds */
	void __iomem *mem_base; /* The base of the memory mapped area for the led */
	struct led_classdev cdev;
};

/* Sets the brightness of the led represented by {@code led_dev}. */
static void led_control(struct led_classdev *led_cdev, enum led_brightness b)
{
	struct led_dev *led = container_of(led_cdev, struct led_dev, cdev);

	pr_info("led_control() starts for led name [%s]\n", led->cdev.name);

	if (b != LED_OFF) {
		/* Turn on led */
		iowrite32(led->mask, led->mem_base + GPSET0_offset);
	} else {
		/* Turn off led */
		iowrite32(led->mask, led->mem_base + GPCLR0_offset); /* LED OFF */
	}

	pr_info("led_control() ends for led name [%s]\n", led->cdev.name);
}

/*
 * @return {@code struct led_device*} pointer. The allocated memory for led_device
 * is associated with the device and release automatically when the device is close.
 * If the memory allocation fails, it returns {@code NULL}.
 */
static inline struct led_dev* net_gpio_controller_init_led_device(
	struct device *net_gpio_dev, void __iomem *ioremap_addr, int index_node)
{
	/*
	 * The allocated memory is associated with the device and release automatically
	 * when the device is close
	 */
	struct led_dev *led_device = devm_kzalloc(net_gpio_dev, sizeof(struct led_dev), GFP_KERNEL);
	if (!led_device) {
		pr_err("Memory could not be allocated for struct led_dev\n");
		return NULL;
	}

	led_device->mem_base = ioremap_addr;
	led_device->mask = GPIO_INDEX_MASKS[index_node];
	led_device->cdev.brightness = LED_OFF;
	led_device->cdev.brightness_set = led_control;

	return led_device;
}

/**
 * Configures the {@code struct device_node} by allocating memory to store
 * the configuration of {@code struct led_dev} and the {@code struct led_classdev}
 * and then registering the class inside the {@code net_gpio_dev}.
 */
static inline int net_gpio_controller_config_child_node(
	struct device *net_gpio_dev,
	struct device_node *child_node,
	int index_node,
	void __iomem *ioremap_addr,
	u32 gpfsel_read,
	u32 gpfsel_write)
{
	struct led_dev *led_device;
	struct led_classdev *class_dev;
	int ret;

	pr_info("net_gpio_controller_config_child_node() starts - child_node->name = %s\n", child_node->name);

	led_device = net_gpio_controller_init_led_device(
		net_gpio_dev, ioremap_addr, index_node);
	if (!led_device) {
		return -ENOMEM;
	}

	class_dev = &led_device->cdev;
	/* Setting the child node name into struct led_class_dev */
	of_property_read_string(child_node, "label", &class_dev->name);

	ret = devm_led_classdev_register(net_gpio_dev, class_dev);
	if (ret) {
		dev_err(net_gpio_dev, "failed to register the led %s\n", class_dev->name);
		of_node_put(child_node);
		return -EINVAL;
	}

	pr_info("net_gpio_controller_config_child_node() ends - child_node->name = %s\n", child_node->name);
	return 0;
}

/*
 * Configures the leds that are going to output the data.
 *
 * @return a zero if successful, otherwise there was an error.
 */
int net_gpio_controller_config_leds(void __iomem *ioremap_addr, struct device *net_gpio_dev) {
	int ret;
	int child_count;
	struct device_node *child_node;
	u32 gpfsel_read;
	u32 gpfsel_write;
	int index_node;

	pr_info("net_gpio_controller_config_leds() starts executing.\n");

	child_count = of_get_child_count(net_gpio_dev->of_node);
	if (!child_count) {
		pr_err("The bits are not configured in the device tree source\n");
		return -EINVAL;
	}

	pr_info("There are %d child nodes\n", child_count);

	/* Start: Configure LED registers as output and clear all the leds, so output is low */
	gpfsel_read = ioread32(ioremap_addr + GPFSEL2_offset); /* read actual value */
	gpfsel_write = (gpfsel_read & ~GPIO_MASK_ALL_LEDS) |
					(GPIO_SET_FUNCTION_LEDS & GPIO_MASK_ALL_LEDS);

	iowrite32(gpfsel_write, ioremap_addr + GPFSEL2_offset); /* set dir leds to output */
	iowrite32(GPIO_SET_ALL_LEDS, ioremap_addr + GPCLR0_offset); /* Clear all the leds, output is low */
	/* End: Configure LED registers as output and clear all the leds, so output is low */

	index_node = 0;
	for_each_child_of_node(net_gpio_dev->of_node, child_node) {
		ret = net_gpio_controller_config_child_node(
			net_gpio_dev,
			child_node,
			index_node,
			ioremap_addr,
			gpfsel_read,
			gpfsel_write);
		if (ret) {
			return -EINVAL;
		}

		index_node++;
	}

	pr_info("net_gpio_controller_config_leds() ends executing.\n");

	return 0;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Juan Yescas");
MODULE_DESCRIPTION("Net GPIO platform driver to control led output.");


// TODO: Configure and test the 8 leds
// TODO: Design and implement how the net_gpio_driver is going to control the leds
// (should we return an array of led_device structures)?
// TODO Add locking mechanism
// TODO Write client application
// TODO Add data to the queue





