
# Chapter 4 - Character Drivers

Typically, an operating system is designed to hide the underlying hardware details from the user
or user application. Applications do, however, require the ability to access data that is captured
by hardware peripherals, as well as the ability to drive peripherals with output. Since the peripheral
registers are accessible only by the Linux kernel, only the kernel is able to collect dat streams
as they are capture by these peripherals.

## Device Nodes or Virtual Files

Linux requires a mechanism to transfer data from the kernel to user space. This transfer of data is
handled via `device nodes`, which are also known as `virtual files`. Device nodes exist within the
root filesystem, though they are not true files. When a user reads from a device node, the kernel copies
the data stream captured by the underlying driver into the aplciation memory space. When a user writes
to a device node, the kernel copies the data stream provided by the application into the data buffers
of the driver, which are eventually output via the underlying hardware. These virtual files can be
"opened" and "read from" or "written to" by the user applciation using standard `system calls`.

Linux supports three type of devices:

1. Character devices
2. Block devices
3. Network devices

## Major and Minor numbers

In Linux, every device is identified by two numbers, a `major` number and a `minor` number. These
numbers can be seen by invoking `ls -l /dev`. Every device driver registers its major number with
the kernel and is completely responsible for managing its minor numbers. When accessing a device file,
the major number selects which device driver is being called to perform the input/output operation.
The major number is used by the kernel to identify the correct device driver when the device is accessed.
The role of the minor number is device dependent, and is handled internally within the driver.

For instance, the i.MX7D has several hardware UART ports. The same driver can be used to control all the
UARTS, but each physical UART needs its own device node, so the device nodes for these UARTs will all have the
same major number, but will have unique minor numbers.

## How to create devices nodes

There are two ways to create device nodes or virtual files:

1. mknod (not preferred)
2. devtmpfs and the miscellaneous framework.


## LAB 4.1 (uses mknod) - helloworld_char_driver.c




The files used for this lab are:

- chapter-4/helloworld_char_driver.c
- chapter-4/Makefile
- chapter-4-apps/app_for_helloworld_char_driver.c
- chapter-4-apps/Makefile

After compiling the `helloworld_char_driver.c` and the
app (`chapter-4-apps/app_for_helloworld_char_driver.c`), we need to copy
this files in the the rootfs of the Raspberry PI (/home/pi) and then execute
the following commands:

```shell
sudo insmod helloworld_char_driver.ko
cat /proc/devices /* see allocation 202 "my_char_device" */
ls -la /dev  /* my_char_device is not created under /dev yet */
sudo mknod /dev/my_char_device c 202 0 /* create my_char_device under /dev */
./app_for_helloworld_char_driver
dmesg
rmmod helloworld_char_driver.ko
```
















































