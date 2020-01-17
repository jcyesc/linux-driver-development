
/*
 * DMA memory to memory copy
 *
 * This driver will perform the following tasks:
 *
 * - Allocate two buffers: write_buffer and read_buffer
 * - Create a device (/dev/dma_m2m_misc_dev) that the will be used to write to write_buffer.
 * - The write() file operation will set up a DMA transaction memory to memory that will
 * copy the content of write_buffer to read_buffer.
 * - At the end of the DMA transaction the buffers will be compared.
 *
 */

#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/completion.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include <linux/uaccess.h>

struct dma_info
{
	struct miscdevice dma_misc_device;
	struct device *dev;
	char *write_buf;
	char *read_buf;
	struct dma_chan *dma_m2m_chan;
	struct completion dma_m2m_completion;
};

#define DMA_BUF_SIZE  4096

/*
 * This function is called when the DMA transaction has been completed.
 * It verifies that the write_buf and read_buf are equals.
 */
static void dma_m2m_callback(void *data)
{
	struct dma_info *dma_info = (struct dma_info *) data;
	dev_info(dma_info->dev, "%s() finished DMA transaction \n" ,__func__);
	complete(&dma_info->dma_m2m_completion);

	if(strncmp(dma_info->write_buf, dma_info->read_buf, DMA_BUF_SIZE)) {
		dev_err(dma_info->dev, "Buffer copy failed!\n");
		return;
	}

	dev_info(dma_info->dev, "Buffer copy was successful!\n");
	dev_info(dma_info->dev, "write_buf is %s\n", dma_info->write_buf);
	dev_info(dma_info->dev, "read_buf is %s\n", dma_info->read_buf);
}

/*
 * This file operation will start a DMA transaction that will copy
 * the data from write_buf to read_buf. The write_buf contains the
 * data from user_buf.
 */
static ssize_t dma_write(struct file * file,
		const char __user * user_buf,
		size_t count,
		loff_t * offset)
{
	struct dma_async_tx_descriptor *dma_m2m_desc;
	struct dma_device *dma_dev;
	struct dma_info *dma_info;
	dma_cookie_t cookie;
	dma_addr_t dma_src;
	dma_addr_t dma_dst;
	enum dma_status dma_status;

	dma_info = container_of(file->private_data,
		struct dma_info,
		dma_misc_device);
	dev_info(dma_info->dev, "dma_write() is starting\n");

	dma_dev = dma_info->dma_m2m_chan->device;

	if (count > DMA_BUF_SIZE) {
		dev_err(dma_info->dev, "The # of chars is bigger than DMA_BUF_SIZE\n");
		return -EFAULT;
	}

	if(copy_from_user(dma_info->write_buf, user_buf, count)){
		return -EFAULT;
	}

	dev_info(dma_info->dev, "The write_buf string is %s\n", dma_info->write_buf);

	/* Get DMA mappings for memory source and destination. */
	dma_src = dma_map_single(dma_info->dev, dma_info->write_buf,
				DMA_BUF_SIZE, DMA_TO_DEVICE);
	dev_info(dma_info->dev, "dma_src map obtained");

	dma_dst = dma_map_single(dma_info->dev, dma_info->read_buf,
				DMA_BUF_SIZE, DMA_TO_DEVICE);
	dev_info(dma_info->dev, "dma_dst map obtained");

	/* Get DMA descriptor */
	dma_m2m_desc = dma_dev->device_prep_dma_memcpy(dma_info->dma_m2m_chan,
		dma_dst,
		dma_src,
		DMA_BUF_SIZE,
		DMA_CTRL_ACK | DMA_PREP_INTERRUPT);

	dev_info(dma_info->dev, "Successful descriptor obtained");

	/* Configure DMA descriptor */
	dma_m2m_desc->callback = dma_m2m_callback;
	dma_m2m_desc->callback_param = dma_info;

	init_completion(&dma_info->dma_m2m_completion);

	/* Submit DMA request */
	cookie = dmaengine_submit(dma_m2m_desc);
	if (dma_submit_error(cookie)){
		dev_err(dma_info->dev, "Failed to submit DMA\n");
		return -EINVAL;
	};

	/* Flush the transaction to HW */
	dev_info(dma_info->dev, "Issuing DMA transaction\n");
	dma_async_issue_pending(dma_info->dma_m2m_chan);
	wait_for_completion(&dma_info->dma_m2m_completion);

	/* Verifying DMA status */
	dev_info(dma_info->dev, "Verifying DMA status\n");
	dma_status = dma_async_is_tx_complete(dma_info->dma_m2m_chan, cookie, NULL, NULL);
	if (dma_status != DMA_COMPLETE) {
		dev_err(dma_info->dev, "The DMA transaction didn't complete!");
		return -EINVAL;
	}

	dev_info(dma_info->dev, "DMA transaction was successful. The read_buf string is %s\n",
		dma_info->read_buf);

	/* Releasing memory mappings */
	dma_unmap_single(dma_info->dev, dma_src,
			DMA_BUF_SIZE, DMA_TO_DEVICE);
	dma_unmap_single(dma_info->dev, dma_dst,
			DMA_BUF_SIZE, DMA_TO_DEVICE);

	return count;
}

/*
 * File operations for /dev/dma_m2m.
 */
struct file_operations dma_fops = {
	write: dma_write,
};

static int __init dma_probe(struct platform_device *pdev)
{
	struct dma_info *dma_info;
	dma_cap_mask_t dma_m2m_mask;
	int ret;

	dev_info(&pdev->dev, "dma_probe() is executing\n");

	dma_info = devm_kzalloc(&pdev->dev, sizeof(struct dma_info), GFP_KERNEL);

	dma_info->dma_misc_device.minor = MISC_DYNAMIC_MINOR;
	dma_info->dma_misc_device.name = "dma_m2m_misc_dev";
	dma_info->dma_misc_device.fops = &dma_fops;

	dma_info->dev = &pdev->dev;

	dev_info(&pdev->dev, "Allocating write_buf and read_buf buffers\n");
	dma_info->write_buf = devm_kzalloc(&pdev->dev, DMA_BUF_SIZE, GFP_KERNEL);
	if(!dma_info->write_buf) {
		dev_err(&pdev->dev, "error allocating write_buf !!\n");
		return -ENOMEM;
	}

	dma_info->read_buf = devm_kzalloc(&pdev->dev, DMA_BUF_SIZE, GFP_KERNEL);
	if(!dma_info->read_buf) {
		dev_err(&pdev->dev, "error allocating read_buf !!\n");
		return -ENOMEM;
	}

	dev_info(&pdev->dev, "Initializing the DMA memory to memory channel\n");
	dma_cap_zero(dma_m2m_mask);
	dma_cap_set(DMA_MEMCPY, dma_m2m_mask);
	dma_info->dma_m2m_chan = dma_request_channel(dma_m2m_mask, 0, NULL);
	if (!dma_info->dma_m2m_chan) {
		dev_err(&pdev->dev, "Error opening the DMA memory to memory channel\n");
		return -EINVAL;
	}

	ret = misc_register(&dma_info->dma_misc_device);
	if (ret) return ret;

	platform_set_drvdata(pdev, dma_info);

	dev_info(&pdev->dev, "dma_probe() exit\n");

	return 0;
}

static int __exit dma_remove(struct platform_device *pdev)
{
	struct dma_info *dma_info = platform_get_drvdata(pdev);
	dev_info(&pdev->dev, "dma_remove() enter\n");
	misc_deregister(&dma_info->dma_misc_device);
	dma_release_channel(dma_info->dma_m2m_chan);
	dev_info(&pdev->dev, "dma_remove() exit\n");
	return 0;
}

static const struct of_device_id my_of_ids[] = {
	{ .compatible = "dma,dma_m2m"},
	{},
};

MODULE_DEVICE_TABLE(of, my_of_ids);

static struct platform_driver dma_platform_driver = {
	.probe = dma_probe,
	.remove = dma_remove,
	.driver = {
		.name = "dma_platform_driver",
		.of_match_table = my_of_ids,
		.owner = THIS_MODULE,
	}
};

static int mod_init(void)
{
	int ret;
	pr_info("mod_init() enter\n");

	ret = platform_driver_register(&dma_platform_driver);
	if (ret) {
		pr_err("Platform value returned %d\n", ret);
		return ret;

	}
	pr_info("mod_init() exit\n");
	return 0;
}

static void mod_exit(void)
{
	pr_info("mod_exit() enter\n");
	platform_driver_unregister(&dma_platform_driver);
	pr_info("mod_exit() exit\n");

}

module_init(mod_init);
module_exit(mod_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Juan Yescas");
MODULE_DESCRIPTION("This is a driver that shows how to use DMA");

