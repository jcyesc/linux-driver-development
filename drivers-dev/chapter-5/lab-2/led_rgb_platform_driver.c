/*
 * This file will install 3 pin devices and then
 * implement the read() and write() functions from struct file_operations.
 *
 * The sequence of steps is the following:
 *
 * 1. Kernel reads the DT (bcm2710-rpi-b.dts) to include the configuration for 3 pins.
 * 2. After insmod led_rgb_platform_driver is executed, the platform driver is loaded.
 * 3. The probe() method is called 3 times, once for each pin.
 * 4. Every pin configuration is set up in the probe() method.
 * 5. The struct file_operations is assigned to the misc_device. There is one misc_device
 * for each pin.
 * 6. The three pin devices are registered in: /dev/ledblue, /dev/ledred and /dev/ledgreen
 * 7. When echo on > /dev/ledblue is executed, the pin ledblue pin is enabled.
 */
#include <linux/module.h>
#include <linux/fs.h>  /* struct file_operations */
#include <linux/platform_device.h>  /* platform_driver_register(), platform_set_drvdata() */
#include <linux/types.h>
#include <linux/io.h>  /* devm_ioremap(), towrite32() */
#include <linux/of.h>  /* of_property_read_string() */
#include <linux/uaccess.h>  /* copy_from_user(), copy_to_user() */
#include <linux/miscdevice.h>  /* misc_register() */
#include <linux/delay.h>

#define BCM2710_PERI_BASE		0x3F000000
#define GPIO_BASE			(BCM2710_PERI_BASE + 0x200000)	/* GPIO controller */

#define GPIO_27			27
#define GPIO_22			22
#define GPIO_26			26

/* to set and clear each individual LED */
#define GPIO_27_INDEX 	1 << (GPIO_27 % 32)
#define GPIO_22_INDEX 	1 << (GPIO_22 % 32)
#define GPIO_26_INDEX 	1 << (GPIO_26 % 32)

/* select the output function */
#define GPIO_27_FUNC	1 << ((GPIO_27 % 10) * 3)
#define GPIO_22_FUNC 	1 << ((GPIO_22 % 10) * 3)
#define GPIO_26_FUNC 	1 << ((GPIO_26 % 10) * 3)

/* mask the GPIO functions */
#define FSEL_27_MASK 	0b111 << ((GPIO_27 % 10) * 3) /* red since bit 21 (FSEL27) */
#define FSEL_22_MASK 	0b111 << ((GPIO_22 % 10) * 3) /* green since bit 6 (FSEL22) */
#define FSEL_26_MASK 	0b111 << ((GPIO_26 % 10) * 3) /* blue  a partir del bit 18 (FSEL26) */

#define GPIO_SET_FUNCTION_LEDS (GPIO_27_FUNC | GPIO_22_FUNC | GPIO_26_FUNC)
#define GPIO_MASK_ALL_LEDS 	(FSEL_27_MASK | FSEL_22_MASK | FSEL_26_MASK)
#define GPIO_SET_ALL_LEDS (GPIO_27_INDEX  | GPIO_22_INDEX  | GPIO_26_INDEX)

#define GPFSEL2 	 GPIO_BASE + 0x08
#define GPSET0   	 GPIO_BASE + 0x1C
#define GPCLR0 		 GPIO_BASE + 0x28

/* Declare __iomem pointers that will keep virtual addresses */
static void __iomem *GPFSEL2_V;
static void __iomem *GPSET0_V;
static void __iomem *GPCLR0_V;

#define MAX_LED_VALUE 8

static const char *led_on = "on";
static const char *led_off = "off";

/* Represents a led device */
struct led_dev {
	struct miscdevice led_misc_device; /* assign char device for each led */
	u32 led_mask; /* different mask if led is R, G, or B */
	const char *led_name; /* assigned value cannot be modified */
	char led_value[MAX_LED_VALUE];
};

/*
 * Receives and process the values sent from the terminal to control each led. The possible
 * values are on/off.
 */
static ssize_t led_write(struct file *file, const char __user *buff, size_t count, loff_t *ppos) {
	struct led_dev *led_device;

	pr_info("led_write() is called for %s.\n", file->f_path.dentry->d_iname);

	led_device = container_of(file->private_data,
				  struct led_dev, led_misc_device);

	if (count > MAX_LED_VALUE) {
		pr_info("The number of bits to copy [%d] is greater than %d", count, MAX_LED_VALUE);
		return -EFAULT;
	}

	/*
	 * If the led_write() method received a parameter sent by the terminal with the
	 * command:
     * 		echo add
     *
     * the \n character will be added at the end of the command.
     *
	 * led_device->led_value = "on\n" or "off\n after copy_from_user"
	 */
	if(copy_from_user(led_device->led_value, buff, count)) {
		pr_info("Bad copied value\n");
		return -EFAULT;
	}

	/*
	 * we add \0 replacing \n in led_device->led_value in
	 * probe() initialization.
	 */
	led_device->led_value[count-1] = '\0';

	pr_info("This message received from User Space: %s", led_device->led_value);

	if(!strcmp(led_device->led_value, led_on)) {
		iowrite32(led_device->led_mask, GPSET0_V);
	}
	else if (!strcmp(led_device->led_value, led_off)) {
		iowrite32(led_device->led_mask, GPCLR0_V);
	}
	else {
		pr_info("Bad value %s\n", led_device->led_value);
		return -EINVAL;
	}

	pr_info("led_write() is exit.\n");
	return count;
}

/*
 * Reach each LED status on/off
 * use cat from terminal to read
 * led_read() is entered until *ppos > 0
 * twice in this function.
 */
static ssize_t led_read(struct file *file, char __user *buff, size_t count, loff_t *ppos) {

	struct led_dev *led_device;

	pr_info("led_read() is called for %s.\n", file->f_path.dentry->d_iname);

	led_device = container_of(file->private_data, struct led_dev, led_misc_device);

	if(*ppos == 0){
		if(copy_to_user(buff, &led_device->led_value, sizeof(led_device->led_value))){
		pr_info("Failed to return led_value to user space\n");
			return -EFAULT;
		}
		*ppos += 1; /* Increment *ppos to exit the function in the next call */
		return sizeof(led_device->led_value); /* exit first function call */
	}

	pr_info("led_read() is exit.\n");

	return 0; /* exit and do not recall function again */
}

static const struct file_operations led_fops = {
	.owner = THIS_MODULE,
	.read = led_read,
	.write = led_write,
};

/*
 * Reads each node label property in each led_probe() call.
 * led_probe() is called 3 times, once per compatible = "arror,RGBleds"
 * found below each ledred, ledgreen and ledblue node.
 */
static int __init led_probe(struct platform_device *pdev) {

	struct led_dev *led_device;
	int ret;
	char led_val[8] = "off\n";
	static u32 counter = 0; /* Keeps the number of times this function is called. */
	counter++;

	pr_info("leds_probe() enter - times [%d]\n", counter);

	/*
	 * Memory allocated with this function is automatically freed on driver detach.
	 * Like all other devres resources, guaranteed alignment is unsigned long long.
	 */
	led_device = devm_kzalloc(&pdev->dev, sizeof(struct led_dev), GFP_KERNEL);

	/* "label" is defined in the DT (Device Tree) bcm2710-rpi-3-b.dts file */
	of_property_read_string(pdev->dev.of_node, "label", &led_device->led_name);
	pr_info("leds_probe() processing %s\n", led_device->led_name);
	led_device->led_misc_device.minor = MISC_DYNAMIC_MINOR;
	led_device->led_misc_device.name = led_device->led_name;
	led_device->led_misc_device.fops = &led_fops;

	/* Assigns a different mask for each led */
	if (strcmp(led_device->led_name,"ledred") == 0) {
		led_device->led_mask = GPIO_27_INDEX;
	}
	else if (strcmp(led_device->led_name,"ledgreen") == 0) {
		led_device->led_mask = GPIO_22_INDEX;
	}
	else if (strcmp(led_device->led_name,"ledblue") == 0) {
		led_device->led_mask = GPIO_26_INDEX;
	}
	else {
		pr_info("Bad device tree value\n");
		return -EINVAL;
	}

	/* Initialize the led status to off */
	memcpy(led_device->led_value, led_val, sizeof(led_val));

	/* Register each led device. */
	ret = misc_register(&led_device->led_misc_device);
	if (ret) {
		pr_info("There was an error while registering the device %s\n", led_device->led_name);
		return ret; /* misc_register returns 0 if success */
	}

	/*
	 * Attach the private structure to the pdev structure
	 * to recover it in each remove() function call.
	 */
	platform_set_drvdata(pdev, led_device);

	pr_info("leds_probe() exit - %s\n", led_device->led_name);

	return 0;
}

/* The led_remove() function is called 3  times, once per led */
static int __exit led_remove(struct platform_device *pdev) {
	struct led_dev *led_device = platform_get_drvdata(pdev);

	pr_info("leds_remove() enter - %s\n", led_device->led_name);

	misc_deregister(&led_device->led_misc_device);

	pr_info("leds_remove() exit - %s\n", led_device->led_name);

	/*
	 * Memory allocated wit devm_kzalloc() is automatically freed when the
	 * probe() functions exits with an error status or after the remove() function
	 * returns.
	 *
	 * devm_kfree(&pdev->dev, led_device);
	 */

	return 0;
}

static const struct of_device_id my_of_ids[] = {
	{ .compatible = "arrow,RGBleds"},
	{},
};

MODULE_DEVICE_TABLE(of, my_of_ids);

static struct platform_driver led_platform_driver = {
	.probe = led_probe,
	.remove = led_remove,
	.driver = {
		.name = "RGBleds",
		.of_match_table = my_of_ids,
		.owner = THIS_MODULE,
	}
};

static int led_rgb_init(void) {
	int ret;
	u32 GPFSEL_read;
	u32 GPFSEL_write;
	pr_info("led_rgb_init() enter\n");

	ret = platform_driver_register(&led_platform_driver);
	if (ret !=0) {
		pr_err("platform value returned %d\n", ret);
		return ret;
	}

	GPFSEL2_V = ioremap(GPFSEL2, sizeof(u32));
	GPSET0_V =  ioremap(GPSET0, sizeof(u32));
	GPCLR0_V =  ioremap(GPCLR0, sizeof(u32));

	GPFSEL_read = ioread32(GPFSEL2_V); /* read current value */

	/*
	 * set to 0 3 bits of each FSEL and keep equal the rest of bits,
	 * then set to 1 the first bit of each FSEL (function) to set 3 GPIOS to output
	 */
	GPFSEL_write = (GPFSEL_read & ~GPIO_MASK_ALL_LEDS) |
				  (GPIO_SET_FUNCTION_LEDS & GPIO_MASK_ALL_LEDS);

	iowrite32(GPFSEL_write, GPFSEL2_V); /* set leds to output */
	iowrite32(GPIO_SET_ALL_LEDS, GPCLR0_V); /* Clear all the leds, output is low */

	pr_info("led_rgb_init() exit\n");
	return 0;
}

static void led_rgb_exit(void) {
	pr_info("led_rgb_exit() enter\n");

	iowrite32(GPIO_SET_ALL_LEDS, GPCLR0_V); /* Clear all the leds */

	iounmap(GPFSEL2_V);
	iounmap(GPSET0_V);
	iounmap(GPCLR0_V);

	platform_driver_unregister(&led_platform_driver);

	pr_info("led_rgb_exit() exit\n");
}

module_init(led_rgb_init);
module_exit(led_rgb_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Juan Yescas");
MODULE_DESCRIPTION("This is a platform driver that turns on/off \
					three led devices in the Raspberry Pi");
