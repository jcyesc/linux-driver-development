
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/types.h>
#include <linux/wait.h>

#define MAX_KEY_STATES 256

static char *INTERRUPT_NAME = "CUSTOM_PIN_23_INT";
static char hello_keys_buf[MAX_KEY_STATES];
static int buf_rd;
static int buf_wr;

struct key_priv {
	struct device *dev;
	struct gpio_desc *gpio;
	struct miscdevice int_miscdevice;
	wait_queue_head_t wq_data_available;
	int irq;
};

/*
 * Interrupt handler
 *
 * Note: While the interrupt handler is processing the interrupt, if another interrupt
 * of this kind is generated, it is not processed till the interrupt handler has completed.
 *
 * When the interrupt happens, there is an interrupt register that is cleared, if another
 * interrupt happens while processing the interrupt, the interrupt register is set and when
 * the interrupt handler has finished, it is executed again. This means that if there are
 * several interrupts while the interrupt handler is being processed, only one it is processed
 * after the first execution of the interrupt handler.
 *
 * In order to test this, press the switch several times during.
 *
 * In order to avoid the bouncing of the switch a delay could be added:
 *
 *   mdelay(5000);
 */
static irqreturn_t keys_interrupt_handler(int irq, void *data) {
	int val;
	struct key_priv *priv = data;
	dev_info(priv->dev, "hello_keys_isr() starts. Key %s, buf_wr = %d, buf_rd = %d\n",
			INTERRUPT_NAME, buf_wr, buf_rd);


	val = gpiod_get_value(priv->gpio);
	dev_info(priv->dev, "Button state: 0x%08X\n", val);

	if (val == 1)
		hello_keys_buf[buf_wr++] = 'P';
	else
		hello_keys_buf[buf_wr++] = 'R';

	if (buf_wr >= MAX_KEY_STATES)
		buf_wr = 0;

	/* Wake up the process */
	wake_up_interruptible(&priv->wq_data_available);

	dev_info(priv->dev, "hello_keys_isr() ends. Key %s, buf_wr = %d, buf_rd = %d\n",
				INTERRUPT_NAME, buf_wr, buf_rd);

	return IRQ_HANDLED;
}

static int keys_dev_read(struct file *file, char __user *buff,
		size_t count, loff_t *off) {
	int ret;
	int output_size = 2;
	char ch[output_size];
	struct key_priv *priv;

	if (count < output_size) {
		dev_err(priv->dev, "keys_dev_read() counts has invalid size.\n");
		return -EINVAL;
	}

	priv = container_of(file->private_data, struct key_priv, int_miscdevice);

	dev_info(priv->dev, "keys_dev_read() starts loff_t = %ld, buf_wr = %d, buf_rd = %d\n",
			(long) *off, buf_wr, buf_rd);

	/*
	 * Sleep the process.
	 * The condition is checked each time the waitqueue is woken up.
	 */
	ret = wait_event_interruptible(priv->wq_data_available, buf_wr != buf_rd);
	if(ret)
		return ret;

	/* Send values to user application  */
	ch[0] = hello_keys_buf[buf_rd];
	ch[1] = '\n';
	if (copy_to_user(buff, &ch, output_size)) {
		return -EFAULT;
	}

	buf_rd++;
	if(buf_rd >= MAX_KEY_STATES)
		buf_rd = 0;
	*off += 1;

	dev_info(priv->dev, "keys_dev_read() ends loff_t = %ld, buf_wr = %d, buf_rd = %d\n",
		(long) *off, buf_wr, buf_rd);

	return output_size;
}

static const struct file_operations my_dev_fops = {
	.owner = THIS_MODULE,
	.read = keys_dev_read,
};

static int __init my_probe(struct platform_device *pdev) {
	int ret_val;
	struct key_priv *priv;
	struct device *dev = &pdev->dev;

	dev_info(dev, "my_probe() function is called.\n");

	/* Allocate new structure representing device */
	priv = devm_kzalloc(dev, sizeof(struct key_priv), GFP_KERNEL);
	priv->dev = dev;

	platform_set_drvdata(pdev, priv);

	/* Init the wait queue head */
	init_waitqueue_head(&priv->wq_data_available);

	/* Get Linux IRQ number from device tree using 2 methods */
	priv->gpio = devm_gpiod_get(dev, NULL, GPIOD_IN);
	if (IS_ERR(priv->gpio)) {
		dev_err(dev, "devm_gpiod_get() failed\n");
		return PTR_ERR(priv->gpio);
	}

	priv->irq = gpiod_to_irq(priv->gpio);
	if (priv->irq < 0) {
		dev_err(dev, "gpiod_to_irq() failed to get an IRQ");
		return priv->irq;
	}
	dev_info(dev, "gpiod_to_irq() returns IRQ number: %d\n", priv->irq);

	/* Second method to get the Linux IRQ number
	 *
	 * This method works fine the first time when the module is loaded,
	 * however, after removing the module and loading for second time,
	 * the function platform_get_irq() fails.
	 *
	priv->irq = platform_get_irq(pdev, 0);
	if (priv->irq < 0) {
		dev_err(dev, "platform_get_irq() failed to get an IRQ");
		return priv->irq;
	}
	dev_info(dev, "platform_get_irq() returns IRQ number: %d\n", priv->irq);
	*/

	ret_val = devm_request_irq(dev, priv->irq, keys_interrupt_handler,
		IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, INTERRUPT_NAME, priv);
	if (ret_val) {
		dev_err(dev, "Failed to request interrupt %d, error %d\n", priv->irq, ret_val);
		return ret_val;
	}

	priv->int_miscdevice.name = "miscdevice_name_wait";
	priv->int_miscdevice.minor = MISC_DYNAMIC_MINOR;
	priv->int_miscdevice.fops = &my_dev_fops;

	ret_val = misc_register(&priv->int_miscdevice);
	if (ret_val != 0) {
		dev_err(dev, "Could not register the misc device miscdevice_name_wait\n");
		return ret_val;
	}

	dev_info(dev, "my_probe() function finished.\n");
	return 0;
}

static int __exit my_remove(struct platform_device *pdev) {
	struct key_priv *priv = platform_get_drvdata(pdev);
	dev_info(&pdev->dev, "my_remove() function is called.\n");
	devm_free_irq(&pdev->dev, priv->irq, &pdev->dev);
	misc_deregister(&priv->int_miscdevice);
	dev_info(&pdev->dev, "my_remove() function finished.");

	return 0;
}

static const struct of_device_id of_ids[] = {
	{ .compatible = "button_dev,intkeywait" },
	{},
};

MODULE_DEVICE_TABLE(of, of_ids);

static struct platform_driver interrupt_wait_platform_driver = {
	.probe = my_probe,
	.remove = my_remove,
	.driver = {
		.name = "int_wait_platform_driver",
		.of_match_table = of_ids,
		.owner = THIS_MODULE,
	},
};

module_platform_driver(interrupt_wait_platform_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Juan Yescas");
MODULE_DESCRIPTION("This is a platform driver that sends to user space \
	the number of times we press the switch using INTs");

