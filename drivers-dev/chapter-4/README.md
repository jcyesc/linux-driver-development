
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


## LAB 1 (uses mknod) - helloworld_char_driver.c

In this kernel module lab, you will interact with user space through an `app_for_helloworld_char_driver` user application. We will use open()
and ioctl() system calls in the application, and write its corresponding driver's callback
operations on the kernel side, providing the communication between the user and kernel space.

In this lab, we will create an user application to interact with the driver. Finally, we
will handle file operations in the driver to service requests from user space.

In the kernel, a character type device is represented by `struct cdev`, a structure used to
register it in the system.

The files used for this lab are:

- chapter-4/lab-1/helloworld_char_driver.c
- chapter-4/lab-1/Makefile
- chapter-4-apps/lab-1/app_for_helloworld_char_driver.c
- chapter-4-apps/lab-1/Makefile

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

## LAB 2 - Adding the Module to the Kernel Build

So far we have been building drivers as `loadable kernel module (LKM)`, which
was loaded during run-time.In this lab, we make the driver a part of the kernel source tree
and have the driver built into the kernel binary image. This way the driver is already loaded
when the new kernel is booted.

Note: Once that the Kconfig and Makefile files have been modified, run the following command:

```shell
menu menuconfig ARCH=arm
```

Then go to `main menu -> Device Drivers -> Character devices -> Hello kernel module defined in the kernel source tree`. Hit <spacebar> once to see a <*> appear next to the new configuration. The other option is <m>, that means module.

To verify that the module has been selected to be compiled, run the following command to check
if the hello world module appears:

```shell
more .config | grep HELLO
```
> WARNING: Don't execute `make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- bcm2709_defconfig` after
>         executing `make menuconfig ARCH=arm`, otherwise the .config file will be rewritten.

There are several ways to know if the module was compiled:

1. After executing the following command:

```shell
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- zImage -j 4 modules dtbs
```

you can see a message like:

   CC      drivers/char/hello_kernel_module.o
  	...

2. Complied modules are installed into `/lib/modules/version/kernel/`, where each directory
under `kernel/` corresponds to the mdoule's location in the kernel source tree. The kernel version
is shown at the end of the output of the command `make modules_install`.

3. You can also run the command `lsmod`.

Note about `tristate` in Kconfig file.

The `tristate` line in Kconfig means that it can be built into the kernel (Y), built as a
module (M), or not built at all (N). To remove the option of building as a module use the
directive `bool` instead of `tristate`. If the option (Y) or <*> was picked, it means that
it will built into the kernel and it won't be a module, so you can't see it with `lsmod`.



