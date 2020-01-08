
/*
 * Linked List memory allocation
 *
 * In this kernel module lab, we will allocate in kernel memory a
 * circular single linked list composed of several nodes. Each node will
 * be composed of two variables:
 *
 * 1. A buffer pointer that points to a memory buffer allocated with devm_kmalloc()
 * using a "for" loop.
 *
 * 2. A next pointer that points to the next node of the linked list.
 *
 * The linked list will be managed through the items of a structure name liste. The
 * driver's write() callback function will get the characters written to the user space
 * console. These characters fill each node buffer of the linked list starting from the
 * first member. It will be moved to the next node when the node is filled with the
 * selected buffer size (variable BlockSize). The driver will write again to the first
 * node buffer of the linked list when the last node buffer has been filled.
 *
 * You can read all the values written to the nodes via the driver's read() callback
 * function. The reading starts from the first node buffer to the last written node buffer
 * of the linked list. After exiting the read() function, all the liste pointers point to the
 * first node of the linked list and the cur_read_offset and cur_write_offset variables are set
 * to zero to start writing again from the first node.
 */

#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/uaccess.h>

static int BlockNumber = 10;
static int BlockSize = 5;
static int size_to_read = 0;
static int node_count= 1;
static int cnt = 0;

typedef struct dnode
{
    char *buffer;
    struct dnode *next;
} data_node;

typedef struct  lnode
{
	data_node *head;
    data_node *cur_write_node;
    data_node *cur_read_node;
    int cur_read_offset;
    int cur_write_offset;
}liste;

static liste newListe;

static int createlist (struct platform_device *pdev)
{
    data_node *newNode, *previousNode, *headNode;
    int i;

    /* new node creation */
    newNode = devm_kmalloc(&pdev->dev, sizeof(data_node), GFP_KERNEL);
    if (newNode)
    	newNode->buffer = devm_kmalloc(&pdev->dev, BlockSize*sizeof(char), GFP_KERNEL);
    if (!newNode || !newNode->buffer)
    	return -ENOMEM;

    newNode->next = NULL;

    newListe.head = newNode;
    headNode = newNode;
    previousNode = newNode;


    for (i = 1; i < BlockNumber; i++)
    {
        newNode = devm_kmalloc(&pdev->dev, sizeof(data_node), GFP_KERNEL);
        if (newNode)
        	newNode->buffer = devm_kmalloc(&pdev->dev, BlockSize*sizeof(char), GFP_KERNEL);
        if (!newNode || !newNode->buffer)
        	return -ENOMEM;
        newNode->next = NULL;
        previousNode->next = newNode;
        previousNode = newNode;
    }

    newNode->next = headNode;

    newListe.cur_read_node = headNode;
    newListe.cur_write_node = headNode;
    newListe.cur_read_offset = 0;
    newListe.cur_write_offset = 0;

    return 0;
}

static ssize_t my_dev_write(struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
	int size_to_copy;
	pr_info("my_dev_write() is called.\n");
	pr_info("node_number_%d\n", node_count);

	if ((*(offset) == 0) || (node_count == 1))
	{
	    size_to_read += size;
	}

	if (size < BlockSize - newListe.cur_write_offset)
		size_to_copy = size;
	else
		size_to_copy = BlockSize - newListe.cur_write_offset;

	if(copy_from_user(newListe.cur_write_node->buffer + newListe.cur_write_offset, buf, size_to_copy))
	{
		return -EFAULT;
	}
	*(offset) += size_to_copy;
	newListe.cur_write_offset +=  size_to_copy;
	if (newListe.cur_write_offset == BlockSize)
	{
	    newListe.cur_write_node = newListe.cur_write_node->next;
	    newListe.cur_write_offset = 0;
	    node_count = node_count+1;
	    if (node_count > BlockNumber)
	    {
	    	newListe.cur_read_node = newListe.cur_write_node;
	    	newListe.cur_read_offset = 0;
		node_count = 1;
		cnt = 0;
		size_to_read = 0;
	    }
	 }
	return size_to_copy;
}


static ssize_t my_dev_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{

	int size_to_copy;
	int read_value;

	read_value = (size_to_read - (BlockSize * cnt));

	if ((*offset) < size_to_read)
	{
		if (read_value < BlockSize - newListe.cur_read_offset)
			size_to_copy = read_value;
		else
			size_to_copy = BlockSize - newListe.cur_read_offset;
		if(copy_to_user(buf, newListe.cur_read_node->buffer + newListe.cur_read_offset, size_to_copy))
		{
			return -EFAULT;
		}
		newListe.cur_read_offset += size_to_copy;
		(*offset)+=size_to_copy;

		if (newListe.cur_read_offset == BlockSize)
		{
			cnt = cnt+1;
			newListe.cur_read_node = newListe.cur_read_node->next;
			newListe.cur_read_offset = 0;
		}
		return size_to_copy;
	}
	else
	{
		msleep(250);
		newListe.cur_read_node = newListe.head;
		newListe.cur_write_node = newListe.head;
		newListe.cur_read_offset = 0;
		newListe.cur_write_offset = 0;
		node_count = 1;
		cnt = 0;
		size_to_read = 0;
		return 0;
	}
}

static int my_dev_open(struct inode *inode, struct file *file)
{
	pr_info("my_dev_open() is called.\n");
	return 0;
}
static int my_dev_close(struct inode *inode, struct file *file)
{
	pr_info("my_dev_close() is called.\n");
	return 0;
}

static const struct file_operations my_dev_fops = {
	.owner = THIS_MODULE,
	.open = my_dev_open,
	.write = my_dev_write,
	.read = my_dev_read,
	.release = my_dev_close,
};

static struct miscdevice helloworld_miscdevice = {
		.minor = MISC_DYNAMIC_MINOR,
		.name = "mydev",
		.fops = &my_dev_fops,
};

static int __init my_probe(struct platform_device *pdev)
{
	int ret_val;
	pr_info("platform_probe enter\n");
	ret_val = createlist(pdev);
	if (ret_val != 0) {
		pr_err("could not create the list");
		return ret_val;
	}

	ret_val = misc_register(&helloworld_miscdevice);
	if (ret_val != 0)
	{
		pr_err("could not register the misc device mydev");
		return ret_val;
	}
	pr_info("mydev: got minor %i\n",helloworld_miscdevice.minor);

	return 0;
}

static int __exit my_remove(struct platform_device *pdev)
{
	misc_deregister(&helloworld_miscdevice);
	pr_info("platform_remove exit\n");
	return 0;
}

static const struct of_device_id my_of_ids[] = {
	{ .compatible = "arrow,hellokeys"},
	{},
};

MODULE_DEVICE_TABLE(of, my_of_ids);

static struct platform_driver my_platform_driver = {
	.probe = my_probe,
	.remove = my_remove,
	.driver = {
		.name = "hellokeys",
		.of_match_table = my_of_ids,
		.owner = THIS_MODULE,
	}
};

static int demo_init(void)
{
	int ret_val;
	pr_info("demo_init enter\n");

	ret_val = platform_driver_register(&my_platform_driver);
	if (ret_val !=0)
	{
		pr_err("platform value returned %d\n", ret_val);
		return ret_val;

	}
	pr_info("demo_init exit\n");
	return 0;
}

static void demo_exit(void)
{
	pr_info("demo_exit enter\n");

	platform_driver_unregister(&my_platform_driver);

	pr_info("demo_exit exit\n");
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Juan Yescas");
MODULE_DESCRIPTION("This is a platform driver that writes in and read from a linked list of several buffers ");

