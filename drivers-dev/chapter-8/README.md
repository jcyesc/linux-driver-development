
# Chapter 8 - Allocating Memory with Linux Drivers



## Lab 1 - Linked List memory allocation

In this kernel module lab, we will allocate in kernel memory a
circular single linked list composed of several nodes. Each node will
be composed of two variables:

1. A buffer pointer that points to a memory buffer allocated with devm_kmalloc()
using a "for" loop.
2. A next pointer that points to the next node of the linked list.

The linked list will be managed through the items of a structure name liste. The
driver's write() callback function will get the characters written to the user space
console. These characters fill each node buffer of the linked list starting from the
first member. It will be moved to the next node when the node is filled with the
selected buffer size (variable BlockSize). The driver will write again to the first
node buffer of the linked list when the last node buffer has been filled.
 *
You can read all the values written to the nodes via the driver's read() callback
function. The reading starts from the first node buffer to the last written node buffer
of the linked list. After exiting the read() function, all the liste pointers point to the
first node of the linked list and the cur_read_offset and cur_write_offset variables are set
to zero to start writing again from the first node.

The files used for this lab are:

- chapter-8/lab-1-linked-list/linked_list_kmalloc.c
- chapter-8/lab-1-linked-list/Makefile (builds the .ko file)
- linux-kernel/arch/arm/boot/dts/bcm2710-rpi-3-b.dts

>Note: Remember to build the device tree (DT) and install the modules.


```shell
sudo insmod linked_list_kmalloc_driver.ko
echo onetwothree > /dev/mydev
sudo chmod 777 /dev/mydev
echo onetwothree > /dev/mydev
cat /dev/mydev
cat /dev/mydev
more /dev/mydev
echo anothertext > /dev/mydev
more /dev/mydev
cat /dev/mydev
sudo rmmod linked_list_kmalloc_driver
```
