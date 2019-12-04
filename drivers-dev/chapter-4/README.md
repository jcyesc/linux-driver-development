
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
2. devtmpfs 
3.  Miscellaneous framework.


## LAB 1.a (uses mknod) - helloworld_char_driver.c

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

## LAB 1.b - Adding the Module to the Kernel Build

So far we have been building drivers as `loadable kernel module (LKM)`, which
was loaded during run-time.In this lab, we make the driver a part of the kernel source tree
and have the driver built into the kernel binary image. This way the driver is already loaded
when the new kernel is booted.


The files used for this lab are:

- linux-kernel/drivers/char/hello_kernel_module.c
- linux-kernel/drivers/char/Makefile
- linux-kernel/drivers/char/Kconfig

Note: Once that the Kconfig and Makefile files have been modified, run the following command:

```shell
make menuconfig ARCH=arm
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
it will built into the kernel and it won't be a module, so you can't see it with `lsmod` or under
`/lib/modules/version/kernel/drivers`.


## LAB 2 - "class" charater driver

In this kernel module lab we will create a device node using `devtmpfs` instead of doing it
manually. In this driver, we will add an entry in the `/sys/class` directory.

Device files are created by the kernel via the `devtmpfs` filesystem. Any driver that wishes to
register a device node will go through the devtmps (via the core driver) to do it. When a devtmpfs
instance is mounted on /dev, the device node will initially be created with a fixed name,
permissions, and owner. All device nodes are owned by root and have the default mode of 0600.

After devtmpfs is loaded, the rules in the following files will be applied to the device nodes:

-/etc/udev/rules.d
-/lib/udev/rules.d
-/run/udev/rules.d

The `CONFIG_DEVTMPFS_MOUNT` kernel configuration option makes the kernel mount devimpfs automatically
at boot time, except when booting on an initramfs.

The files used for this lab are:

- chapter-4/lab-2/helloworld_class_driver.c
- chapter-4/lab-2/Makefile (builds the .ko file)
- chapter-4-apps/lab-2/app_for_helloworld_class_driver.c
- chapter-4-apps/lab-2/Makefile (builds the binary for the app)

After `copying` the `helloworld_class_driver.ko` and `app_for_helloworld_class_driver` in the
Raspberry Pi, execute the following commands:

```shell
sudo insmod helloworld_class_driver.ko
ls /sys/class
ls /sys/class/my_custom_char_class
ls /sys/class/my_custom_char_class/my_char_class_dev
cat /sys/class/my_custom_char_class/my_char_class_dev/dev /* See assigned major and minor numbers */
ls -l /dev /* verify that my_char_class_dev is created under /dev */
./app_for_helloworld_class_driver /* We'll get "denied permission */
sudo chmod 755 /dev/my_char_class_dev
./app_for_helloworld_class_driver
sudo rmmod helloworld_class_driver.ko
```

## Lab 3 - "Miscellaneous Framework" character driver 

The `Misc Framework` is an interface exported by the Linux Kernel that allows modules
to register their individual minor numbers.

The device driver implemented as a miscellaneous character uses the major number allocated
by the Linux kernel for `miscellaneous devices`. This eliminates the need to define an unique
major number for the driver; this is important, as a confilct between major numbers has become
increasingly likely, and use of the misc device class is an effective tactic. Each probed device
is dynamically assigned a minor number, and is listed with a directory entry within the sysfs
pseudo-filesystem under `/sys/class/misc/`.

`Major number 10` is officially assigned to the misc driver. Modules can register individual
minor numbers with the misc driver and take care of a small device, needing only a single entry
point.

The files used for this lab are:

- chapter-4/lab-3/helloworld_misc_char_driver.c
- chapter-4/lab-3/Makefile (builds the .ko file)
- chapter-4-apps/lab-3/app_for_helloworld_misc_char_driver.c
- chapter-4-apps/lab-3/Makefile (builds the binary for the app)

After `copying` the `helloworld_misc_char_driver.ko` and `app_for_helloworld_misc_char_driver` in the
Raspberry Pi, execute the following commands:

```shell
sudo insmod helloworld_misc_char_driver.ko
ls /sys/class/misc /* check that my_misc_dev is created under misc class foleder */
ls /sys/class/misc/my_misc_dev
cat /sys/class/misc/my_misc_dev/dev /* See the major and minor numbers */
ls -l /dev /* verify that my_misc_dev is created under /dev */
./app_for_helloworld_misc_char_driver /* We'll get "denied permission */
sudo chmod 755 /dev/my_char_class_dev
./app_for_helloworld_misc_char_driver
sudo rmmod helloworld_misc_char_driver.ko
```

