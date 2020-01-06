
/*
 * Led blink control driver
 *
 * This driver will create a new class called "keyled". Several led devices will
 * be created under the "keyled" class, and also several sysfs entries will be created
 * under each led device. We will control each led device by writing from user space
 * to the sysfs entries under each led device registered to the "keyled" class. The
 * led devices will be controlled writing to the sysfs entries under
 * /sys/class/keyled/<led_device>/ directory.
 *
 * The blinking value "period" of each led device will be incremented or decremented via
 * interrupts by using two buttons. A kernel thread will manage the led blinking, toggling
 * the output value of the GPIO connected to the led.
 */

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/spinlock.h>
#include <linux/types.h>

#define LED_NAME_LEN			32
#define NUMBER_OF_INTERRUPTS	2
static const char *INT_NAME_1 = "MIKROBUS_KEY1";
static const char *INT_NAME_2 = "MIKROBUS_KEY2";

/* Specific LED private structure */
struct led_device {
	char name[LED_NAME_LEN];
	struct gpio_desc *led_desc; /* each LED gpio_desc */
	struct device *dev;
	struct led_config *config; /* pointer to the global private struct */
};

/* Global configuration of the led */
struct led_config {
	u32 num_leds;
	u8 led_flag;
	u8 task_flag;
	u32 period;
	spinlock_t period_lock;
	struct task_struct *task; 	/* kthread task_struct */
	struct class *led_class;  	/* the keyled class */
	struct device *dev;
	dev_t led_devt;		  	    /* first device identifier */
	struct led_device *leds[];	/* pointers to each led_device struct */
};

/* kthread function */
static int led_flash(void *data){
	unsigned long flags = 0;
	u32 value = 0;
	u32 period;
	struct led_device *led_dev = (struct led_device *) data;
	dev_info(led_dev->dev, "led_flash() is starting - kthread\n");

	while(!kthread_should_stop()) {
		spin_lock_irqsave(&led_dev->config->period_lock, flags);
		period = led_dev->config->period;
		spin_unlock_irqrestore(&led_dev->config->period_lock, flags);
		value = !value;
		gpiod_set_value(led_dev->led_desc, value);
		msleep(period / 2);
	}

	gpiod_set_value(led_dev->led_desc, 1); /* switch off the led */
	dev_info(led_dev->dev, "led_flash() has completed\n");
	return 0;
};

/*
 * sysfs methods
 */

/*
 * This function is used to controll the led_device (turn it on/off).
 *
 * This function is assigned when the "struct dev_attr_set_led_name" is
 * declared using DEVICE_ATTR_WO macro.
 */
static ssize_t set_led_store(struct device *dev,
	struct device_attribute *attr,
	char *buf,
	size_t count)
{
	int i;
	char *buffer = buf;
	struct led_device *led_count;
	struct led_device *current_led_device = (struct led_device *) dev_get_drvdata(dev);

	/* replace \n added from terminal with \0 */
	buffer[count - 1] = '\0';

	if (current_led_device->config->task_flag == 1) {
			kthread_stop(current_led_device->config->task);
			current_led_device->config->task_flag = 0;
	}

	if(!strcmp(buffer, "on")) {
		if (current_led_device->config->led_flag == 1) {
			/* Turn off all the leds */
			for (i = 0; i < current_led_device->config->num_leds; i++) {
				led_count = current_led_device->config->leds[i];
				gpiod_set_value(led_count->led_desc, 1);
			}

			/* Turn on current_led_device */
			gpiod_set_value(current_led_device->led_desc, 0);
		} else {
			gpiod_set_value(current_led_device->led_desc, 0);
			current_led_device->config->led_flag = 1;
		}
	}
	else if (!strcmp(buffer, "off")) {
		/* Turn off current_led_device */
		gpiod_set_value(current_led_device->led_desc, 1);
	} else {
		dev_info(current_led_device->dev, "Bad led value.\n");
		return -EINVAL;
	}

	return count;
}
static DEVICE_ATTR_WO(set_led);

/* blinking ON the specific LED running a kthread */
static ssize_t blink_on_led_store(struct device *dev,
				  struct device_attribute *attr,
				  char *buf, size_t count)
{
	int i;
	char *buffer = buf;
	struct led_device *led_count;
	struct led_device *current_led_device = (struct led_device *) dev_get_drvdata(dev);

	/* replace \n added from terminal with \0 */
	buffer[count - 1] = '\0';

	if (current_led_device->config->led_flag == 1) {
		/* Turn off all the leds */
		for (i = 0; i < current_led_device->config->num_leds; i++) {
			led_count = current_led_device->config->leds[i];
			gpiod_set_value(led_count->led_desc, 1);
		}
	}

	if(!strcmp(buffer, "on")) {
		if (current_led_device->config->task_flag == 0)
		{
			current_led_device->config->task = kthread_run(led_flash,
					current_led_device,
					"Led_Flash_Thread");
			if(IS_ERR(current_led_device->config->task)) {
				dev_info(current_led_device->dev, "Failed to create the task\n");
				return PTR_ERR(current_led_device->config->task);
			}
		} else {
			dev_info(current_led_device->dev, "Led device is busy.\n");
			return -EBUSY;
		}
	} else {
		dev_info(current_led_device->dev, "Bad led value.\n");
		return -EINVAL;
	}

	current_led_device->config->task_flag = 1;

	dev_info(current_led_device->dev, "Blink_on_led exited\n");
	return count;
}
static DEVICE_ATTR_WO(blink_on_led);

/* switch off the blinking of any led */
static ssize_t blink_off_led_store(struct device *dev,
				   struct device_attribute *attr,
				   char *buf, size_t count)
{
	int i;
	char *buffer = buf;
	struct led_device *current_led_device = (struct led_device *) dev_get_drvdata(dev);
	struct led_device *led_count;

	/* replace \n added from terminal with \0 */
	buffer[count - 1] = '\0';

	if(!strcmp(buffer, "off")) {
		if (current_led_device->config->task_flag == 1) {
			kthread_stop(current_led_device->config->task);
			/* Turn off all the leds. */
			for (i = 0; i < current_led_device->config->num_leds; i++) {
				led_count = current_led_device->config->leds[i];
				gpiod_set_value(led_count->led_desc, 1);
			}
		} else
			return 0;
	} else {
		dev_info(current_led_device->dev, "Bad led value.\n");
		return -EINVAL;
	}

	current_led_device->config->task_flag = 0;
	return count;
}
static DEVICE_ATTR_WO(blink_off_led);

/* set the blinking period */
static ssize_t set_period_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	unsigned long flags = 0;
	int ret;
	int period;
	struct led_device *current_led_device = (struct led_device *) dev_get_drvdata(dev);
	dev_info(current_led_device->dev, "set_period_store() is startig\n");

	ret = sscanf(buf, "%u", &period);
	if (ret < 1 || period < 10 || period > 10000) {
		dev_err(dev, "Invalid period value value\n");
		return -EINVAL;
	}

	spin_lock_irqsave(&current_led_device->config->period_lock, flags);
	current_led_device->config->period = period;
	spin_unlock_irqrestore(&current_led_device->config->period_lock, flags);

	dev_info(current_led_device->dev, "set_period_store() has completed\n");
	return count;
}
static DEVICE_ATTR_WO(set_period);

/*
 * Declare the sysfs structures
 *
 * The variables:
 *
 *   dev_attr_set_led
 *   dev_attr_blink_on_led
 *   dev_attr_blink_off_led
 *   dev_attr_set_period
 *
 * are defined using DEVICE_ATTR_WO().
 *
 */
static struct attribute *led_attrs[] = {
    &dev_attr_set_led.attr,
	&dev_attr_blink_on_led.attr,
	&dev_attr_blink_off_led.attr,
    &dev_attr_set_period.attr,
    NULL,
};

static const struct attribute_group led_group = {
        .attrs = led_attrs,
};

static const struct attribute_group *led_groups[] = {
        &led_group,
        NULL,
};

/*
 * Allocate space for the global private struct
 * and the three local LED private structs
 */
static inline int sizeof_keyled_priv(int num_leds)
{
	return sizeof(struct led_config) +
			(sizeof(struct led_device*) * num_leds);
}

/* First interrupt handler */
static irqreturn_t KEY_ISR1(int irq, void *data)
{
	struct led_config *priv = data;
	dev_info(priv->dev, "KEY_ISR1() interrupt MIKROBUS_KEY1 received. key: %s\n",
			 INT_NAME_1);

	spin_lock(&priv->period_lock);
	priv->period = priv->period + 10;
	if ((priv->period < 10) || (priv->period > 10000))
		priv->period = 10;
	spin_unlock(&priv->period_lock);

	dev_info(priv->dev, "KEY_ISR1() Led period is %d\n", priv->period);

	return IRQ_HANDLED;
}

/* Second interrupt handler */
static irqreturn_t KEY_ISR2(int irq, void *data)
{
	struct led_config *priv = data;
	dev_info(priv->dev, "KEY_ISR2() interrupt MIKROBUS_KEY2 received. key: %s\n",
			 INT_NAME_2);

	spin_lock(&priv->period_lock);
	priv->period = priv->period - 10;
	if ((priv->period < 10) || (priv->period > 10000))
		priv->period = 10;
	spin_unlock(&priv->period_lock);

	dev_info(priv->dev, "KEY_ISR2() Led period is %d\n", priv->period);
	return IRQ_HANDLED;
}

/* Create the LED devices under the sysfs keyled entry. */
struct led_device *led_device_register(const char *name,
	int count,
	struct device *parent,
	dev_t led_devt,
	struct class *led_class)
{
	struct led_device *current_led_device;
	dev_t major_and_minor;
	int ret;

	/* First allocate a new led device */
	current_led_device = devm_kzalloc(parent, sizeof(struct led_device), GFP_KERNEL);
	if (!current_led_device)
		return ERR_PTR(-ENOMEM);

	/* Get the minor number of each device */
	major_and_minor = MKDEV(MAJOR(led_devt), count);

	/* Create the device and init the device's data */
	current_led_device->dev = device_create(led_class, parent, major_and_minor,
				 current_led_device, "%s", name);
	if (IS_ERR(current_led_device->dev)) {
		dev_err(current_led_device->dev, "unable to create device %s\n", name);
		ret = PTR_ERR(current_led_device->dev);
		return ERR_PTR(ret);
	}

	dev_info(current_led_device->dev, "Major number is %d for %s\n", MAJOR(led_devt), name);
	dev_info(current_led_device->dev, "Minor number is %d for %s\n", MINOR(major_and_minor), name);

	/* To recover later from each sysfs entry use dev_set_drvdata()*/
	dev_set_drvdata(current_led_device->dev, current_led_device);

	strncpy(current_led_device->name, name, LED_NAME_LEN);

	dev_info(current_led_device->dev, "Led %s added\n", current_led_device->name);

	return current_led_device;
}

static int __init my_probe(struct platform_device *pdev)
{
	int count;
	int ret;
	int i;
	unsigned int major;
	struct fwnode_handle *child;

	struct device *dev = &pdev->dev;
	struct led_config *config;

	dev_info(dev, "my_probe() is starting.\n");

	count = device_get_child_node_count(dev);
	if (!count)
		return -ENODEV;

	dev_info(dev, "There are %d nodes\n", count);

	/* Allocate all the private structures */
	config = devm_kzalloc(dev, sizeof_keyled_priv(count - NUMBER_OF_INTERRUPTS), GFP_KERNEL);
	if (!config)
		return -ENOMEM;

	/* Allocate 3 device numbers */
	alloc_chrdev_region(&config->led_devt, 0, count - NUMBER_OF_INTERRUPTS, "keyled_class");
	major = MAJOR(config->led_devt);
	dev_info(dev, "Major number is %d\n", major);

	/* Create the LED class */
	config->led_class = class_create(THIS_MODULE, "keyled");
	if (!config->led_class) {
		dev_info(dev, "failed to allocate class\n");
		return -ENOMEM;
	}

	/* Set attributes for this class */
	config->led_class->dev_groups = led_groups;
	config->dev = dev;

	spin_lock_init(&config->period_lock);

	/* Parse all the DT nodes */
	device_for_each_child_node(dev, child){
		int irq;
		int flags;
		struct gpio_desc *keyd;
		const char *label_name;
		const char *colour_name;
		const char *trigger;
		struct led_device *new_led_device;

		fwnode_property_read_string(child, "label", &label_name);

		/* Parsing the DT LED nodes */
		if (strcmp(label_name,"led") == 0) {
			fwnode_property_read_string(child, "colour", &colour_name);

			/*
			 * Create led devices under keyled class
             * priv->num_leds is 0 for the first iteration
             * used to set the minor number of each device
             * increased to the end of the iteration
             */
			new_led_device = led_device_register(colour_name,
				config->num_leds,
				dev,
				config->led_devt,
				config->led_class);
			if (!new_led_device) {
				ret = PTR_ERR(new_led_device);
				fwnode_handle_put(child);

				for (i = 0; i < config->num_leds; i++) {
					device_destroy(config->led_class,
						       MKDEV(MAJOR(config->led_devt), i));
				}
				class_destroy(config->led_class);
				return ret;
			}

			new_led_device->led_desc = devm_fwnode_get_gpiod_from_child(dev, NULL, child, 0, "led");
			if (IS_ERR(new_led_device->led_desc)) {
				fwnode_handle_put(child);
			    ret = PTR_ERR(new_led_device->led_desc);
			    goto error;
			}
			new_led_device->config = config;
			config->leds[config->num_leds] = new_led_device;
			config->num_leds++;

			/* set direction to output */
			gpiod_direction_output(new_led_device->led_desc, 1);

			/* set led state to off */
			gpiod_set_value(new_led_device->led_desc, 1);
		}

		/* Parsing the interrupt nodes */
		else if (strcmp(label_name,"MIKROBUS_KEY_1") == 0) {
			keyd = devm_fwnode_get_gpiod_from_child(dev, NULL, child, 0, "MIKROBUS_KEY_1");
			gpiod_direction_input(keyd);
			fwnode_property_read_string(child, "trigger", &trigger);
			if (strcmp(trigger, "falling") == 0)
				flags = IRQF_TRIGGER_FALLING;
			else if (strcmp(trigger, "rising") == 0)
				flags = IRQF_TRIGGER_RISING;
			else if (strcmp(trigger, "both") == 0)
				flags = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING;
			else
				return -EINVAL;

			irq = gpiod_to_irq(keyd);
			if (irq < 0)
				return irq;

			ret = devm_request_irq(dev, irq, KEY_ISR1,
					       flags, "ISR1", config);
			if (ret) {
				dev_err(dev, "Failed to request interrupt %d, error %d\n",
				        irq, ret);
				return ret;
			}
			dev_info(dev, "IRQ number: %d\n", irq);
		}
		else if (strcmp(label_name,"MIKROBUS_KEY_2") == 0) {
			keyd = devm_fwnode_get_gpiod_from_child(dev, NULL, child, 0, "MIKROBUS_KEY_2");
			gpiod_direction_input(keyd);
			fwnode_property_read_string(child, "trigger", &trigger);
			if (strcmp(trigger, "falling") == 0)
				flags = IRQF_TRIGGER_FALLING;
			else if (strcmp(trigger, "rising") == 0)
				flags = IRQF_TRIGGER_RISING;
			else if (strcmp(trigger, "both") == 0)
				flags = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING;
			else
				return -EINVAL;

			irq = gpiod_to_irq(keyd);
			if (irq < 0)
				return irq;

			ret = devm_request_irq(dev, irq, KEY_ISR2,
					       flags, "ISR2", config);
			if (ret < 0) {
				dev_err(dev, "Failed to request interrupt %d, error %d\n",
					irq, ret);
				goto error;
			}
			dev_info(dev, "IRQ number: %d\n", irq);
		}
		else {
			dev_info(dev, "Bad device tree value\n");
			ret = -EINVAL;
			goto error;
		}
	}

	dev_info(dev, "Finished processing the device tree\n");

	/* reset period to 10 */
	config->period = 10;

	dev_info(dev, "Led period is %d\n", config->period);

	platform_set_drvdata(pdev, config);

	dev_info(dev, "my_probe() has completed.\n");

	return 0;

error:
	/* Unregister everything in case of errors */
	for (i = 0; i < config->num_leds; i++) {
		device_destroy(config->led_class, MKDEV(MAJOR(config->led_devt), i));
	}
	class_destroy(config->led_class);
	unregister_chrdev_region(config->led_devt, config->num_leds);
	return ret;
}

static int __exit my_remove(struct platform_device *pdev)
{

	int i;
	struct led_device *led_count;
	struct led_config *config = platform_get_drvdata(pdev);
	dev_info(&pdev->dev, "my_remove() function is called.\n");

	if (config->task_flag == 1) {
		kthread_stop(config->task);
		config->task_flag = 0;
	}

	if (config->led_flag == 1) {
		for (i = 0; i < config->num_leds; i++) {
			led_count = config->leds[i];
			gpiod_set_value(led_count->led_desc, 1);
		}
	}

	for (i = 0; i < config->num_leds; i++) {
		device_destroy(config->led_class, MKDEV(MAJOR(config->led_devt), i));
	}
	class_destroy(config->led_class);
	unregister_chrdev_region(config->led_devt, config->num_leds);
	dev_info(&pdev->dev, "my_remove() function is exited.\n");
	return 0;

}

static const struct of_device_id of_ids[] = {
	{ .compatible = "led_blink,control" },
	{},
};

MODULE_DEVICE_TABLE(of, of_ids);

static struct platform_driver my_platform_driver = {
	.probe = my_probe,
	.remove = my_remove,
	.driver = {
		.name = "led_blink_control",
		.of_match_table = of_ids,
		.owner = THIS_MODULE,
	}
};

module_platform_driver(my_platform_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Juan Yescas");
MODULE_DESCRIPTION("This is a platform led blink control driver that decreases \
		   and increases the led flashing period");

