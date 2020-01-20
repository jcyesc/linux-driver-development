
/**
 * Note: This driver doesn't work properly in the Raspberry Pi. After an app
 * call ioctl(), the driver doesn't respond anymore and the DMA transaction is
 * not executed.
 *
 * DMA Driver
 *
 * The goal of the driver is to create a mmap and copy the data from the mmap
 * to another part in the memory using a DMA transaction.
 *
 * - We will use the dma_ioctl() kernel callback insteam of dma_write() to manage
 * the DMA transaction.
 *
 * - The dma_mmap() callback function is added to the driver to do the mapping of
 * the kernel buffer.
 *
 * - The virtual address of the process will be returned to the user space by using
 * mmap() system call. Any text can be written from the user application to the returned
 * virtual memory buffer. After that, the ioctl() system call manages the DMA transaction
 * sending the written text from the dma_src buffer to the dma_dst buffer without any
 * CPU intervention.
 */

#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <uapi/asm-generic/errno-base.h>

struct dma_info
{
	struct miscdevice dma_misc_device;
	struct device *dev;
	char *wbuf;
	char *rbuf;
	struct dma_chan *dma_m2m_chan;
	struct completion dma_m2m_ok;
	dma_addr_t dma_src;
	dma_addr_t dma_dst;
};

#define DMA_BUF_SIZE  (1024*63)

static void dma_m2m_callback(void *data)
{
	struct dma_info *dma_priv = data;
	dev_info(dma_priv->dev, "%s finished DMA transaction\n" ,__func__);
	complete(&dma_priv->dma_m2m_ok);
}

static int dma_open(struct inode * inode, struct file * file)
{
	struct dma_info *dma_info;

	pr_info("dma_open() starting\n");

	dma_info = container_of(file->private_data,
		struct dma_info, dma_misc_device);

	dma_info->wbuf = kzalloc(DMA_BUF_SIZE, GFP_DMA);
	if(!dma_info->wbuf) {
		dev_err(dma_info->dev, "error allocating wbuf !!\n");
		return -ENOMEM;
	}

	dma_info->rbuf = kzalloc(DMA_BUF_SIZE, GFP_DMA);
	if(!dma_info->rbuf) {
		dev_err(dma_info->dev, "error allocating rbuf !!\n");
		return -ENOMEM;
	}

	dma_info->dma_src = dma_map_single(dma_info->dev,
		dma_info->wbuf, DMA_BUF_SIZE, DMA_TO_DEVICE);

	pr_info("dma_open() finishing\n");
	return 0;
}

static long dma_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct dma_async_tx_descriptor *dma_m2m_desc;
	struct dma_device *dma_dev;
	struct dma_info *dma_info;
	dma_cookie_t cookie;

	pr_info("dma_ioctl() starting\n");

	dma_info = container_of(file->private_data,
				struct dma_info, dma_misc_device);

	dma_dev = dma_info->dma_m2m_chan->device;

//	dma_info->dma_src = dma_map_single(dma_info->dev, dma_info->wbuf,
//		DMA_BUF_SIZE, DMA_TO_DEVICE);
	dma_info->dma_dst = dma_map_single(dma_info->dev, dma_info->rbuf,
		DMA_BUF_SIZE, DMA_TO_DEVICE);

	dma_m2m_desc = dma_dev->device_prep_dma_memcpy(dma_info->dma_m2m_chan,
		dma_info->dma_dst,
		dma_info->dma_src,
		DMA_BUF_SIZE,
		DMA_CTRL_ACK | DMA_PREP_INTERRUPT);

	dev_info(dma_info->dev, "successful descriptor obtained");

	dma_m2m_desc->callback = dma_m2m_callback;
	dma_m2m_desc->callback_param = dma_info;

	init_completion(&dma_info->dma_m2m_ok);

	cookie = dmaengine_submit(dma_m2m_desc);

	if (dma_submit_error(cookie)){
		dev_err(dma_info->dev, "Failed to submit DMA\n");
		return -EINVAL;
	};

	dma_async_issue_pending(dma_info->dma_m2m_chan);
	wait_for_completion(&dma_info->dma_m2m_ok);
	dma_async_is_tx_complete(dma_info->dma_m2m_chan, cookie, NULL, NULL);

	dma_unmap_single(dma_info->dev, dma_info->dma_src,
					 DMA_BUF_SIZE, DMA_TO_DEVICE);
	dma_unmap_single(dma_info->dev, dma_info->dma_dst,
					 DMA_BUF_SIZE, DMA_TO_DEVICE);

	if(strncmp(dma_info->wbuf, dma_info->rbuf, DMA_BUF_SIZE)) {
		dev_err(dma_info->dev, "Buffer copy failed!\n");
		kfree(dma_info->wbuf);
		kfree(dma_info->rbuf);
		return -EINVAL;
	}

	dev_info(dma_info->dev, "buffer copy passed!\n");
	dev_info(dma_info->dev, "wbuf is %s\n", dma_info->wbuf);
	dev_info(dma_info->dev, "rbuf is %s\n", dma_info->rbuf);

	kfree(dma_info->wbuf);
	kfree(dma_info->rbuf);

	pr_info("dma_ioctl() finishing\n");

	return 0;
}

static int dma_mmap(struct file *file, struct vm_area_struct *vma) {

	struct dma_info *dma_info;

	pr_info("dma_mmap() starting\n");

	dma_info = container_of(file->private_data,
		struct dma_info, dma_misc_device);

	if(remap_pfn_range(vma, vma->vm_start, dma_info->dma_src >> PAGE_SHIFT,
			vma->vm_end - vma->vm_start, vma->vm_page_prot)) {
		return -EAGAIN;
	}

	pr_info("dma_mmap() finishing\n");

	return 0;
}

struct file_operations dma_fops = {
	.owner 			= THIS_MODULE,
	.open  			= dma_open,
	.unlocked_ioctl	= dma_ioctl,
	.mmap		 	= dma_mmap,
};

static int __init dma_probe(struct platform_device *pdev)
{
	int ret;
	struct dma_info *dma_device;
	dma_cap_mask_t dma_m2m_mask;
	struct dma_slave_config dma_m2m_config = {0};

	dev_info(&pdev->dev, "platform_probe enter\n");

	dma_device = devm_kzalloc(&pdev->dev, sizeof(struct dma_info), GFP_KERNEL);

	dma_device->dma_misc_device.minor = MISC_DYNAMIC_MINOR;
	dma_device->dma_misc_device.name = "dma_mmap";
	dma_device->dma_misc_device.fops = &dma_fops;

	dma_device->dev = &pdev->dev;

	dma_cap_zero(dma_m2m_mask);
	dma_cap_set(DMA_MEMCPY, dma_m2m_mask);

	dma_device->dma_m2m_chan = dma_request_channel(dma_m2m_mask, 0, NULL);
	if (!dma_device->dma_m2m_chan) {
		dev_err(&pdev->dev, "Error opening the SDMA memory to memory channel\n");
		return -EINVAL;
	}

	dma_m2m_config.direction = DMA_MEM_TO_MEM;
	dma_m2m_config.dst_maxburst = 2;
	dma_m2m_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	dmaengine_slave_config(dma_device->dma_m2m_chan, &dma_m2m_config);

	ret = misc_register(&dma_device->dma_misc_device);
	if (ret) return ret;

	platform_set_drvdata(pdev, dma_device);

	dev_info(&pdev->dev, "platform_probe exit\n");

	return 0;
}

static int __exit dma_remove(struct platform_device *pdev)
{
	struct dma_info *dma_device = platform_get_drvdata(pdev);
	dev_info(&pdev->dev, "platform_remove enter\n");
	misc_deregister(&dma_device->dma_misc_device);
	dma_release_channel(dma_device->dma_m2m_chan);
	dev_info(&pdev->dev, "platform_remove exit\n");
	return 0;
}

static const struct of_device_id my_of_ids[] = {
	{ .compatible = "dma,dma_m2m"},
	{},
};
MODULE_DEVICE_TABLE(of, my_of_ids);

static struct platform_driver my_platform_driver = {
	.probe = dma_probe,
	.remove = dma_remove,
	.driver = {
		.name = "dma_m2m_mmap",
		.of_match_table = my_of_ids,
		.owner = THIS_MODULE,
	}
};

static int dma_init(void)
{
	int ret_val;
	pr_info("demo_init() starting\n");

	ret_val = platform_driver_register(&my_platform_driver);
	if (ret_val !=0)
	{
		pr_err("platform value returned %d\n", ret_val);
		return ret_val;

	}
	pr_info("demo_init() finishing()\n");
	return 0;
}

static void dma_exit(void)
{
	pr_info("demo_exit() starting\n");
	platform_driver_unregister(&my_platform_driver);
	pr_info("demo_exit() finishing\n");
}

module_init(dma_init);
module_exit(dma_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Juan Yescas");
MODULE_DESCRIPTION("This is a DMA mmap memory to memory driver");



