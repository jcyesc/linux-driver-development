
# Chapter 5 - Platform Drivers

On embedded systems, devices are often not connected through a bus. As a consequence,
this devices can't be enumerated or hotplugged.

However, we still want all of these devices to be part of the device model. Such devices, instead
of being dynamically detected, must be statically described:

1. By `direct instantiation` of struct platform_device structures, as done on a few old
ARM non-Device Tree based platforms. Definition is done in the board-specific or SoC specific
code.
2. In the `Device Tree`, a hardware description file used on some architectures. The device
drivers match with the physical devices describe in the .dts file. After this matching the
driver's `probe()` function is called. An `of_match_table` has to be included in the driver's
code to allow this matching.

Amongst the non-discoverable devices, a huge family is directly part of a system-on-chip: UART
controllers, Ethernet controllers, SPI controllers, graphic or audio devices, etc. In the
Linux kernel, a special bus, called the `platform bus` has been created to handle such
devices. It supports platform drivers that handle platform devices. It works like any other
bus (USB, PCI), except that devices are enumerated statically instead of being discovered
dynamically.

Each platform driver is responsible for instantiating and registering an instance of a
`struct platform_driver` structure within the device model core. Platform drivers follow
the standard driver model convention, where discovery/enumeration is handled outside the drivers,
and drivers provide `probe()` and `remove()` methods. They support power management and shutdown
notifications using the standard conventions.

### probe() function
In the `struct platform_driver` we can see a function pointer variable that points to a function
name `probe()`. The probe() function is called when the `bus driver` pairs the `device` to the
`device driver`. The probe() function is responsible of initializing the device and registering it in
the appropriate kernel framework:

1. The probe() function gets a pointer to a device structure as an argument
(for example, struct pci_dev *, struct usb_dev *, struct platform_device *, struct i2c_client *).
2. It initializes the device, maps I/O memory, allocates buffers, registers interrupt handlers,
timers, and so on..
3. It registers the device to specific framework(s), (for example, network, misc, serial, input,
industrial).

### suspend()/resume()

The `suspend()/resume()` functions in struct platform driver, are used by devices that support low
power management features.

## Exchanging Data between Kernel and User Space

Modern operating systems not only prevent onne process from accessing another process but also
prevent processes from accidentally accesing or manipulating kernel data and services (as the
kernel is shared by all the processes). Operating systems achieve this protection by segmenting
the whole memory into two logical halves, the user and kernel space. This bifurcation ensures
that all processes that are assigned address spaces are mapped to the user space section of memory
and kernel data and services run in kernel space. `System calls` are the kernel's interfaces to
expose its services to applciation processes; they are also called kernel entry points. As system
calls are implemented in kernel space, the respective handlers are provided through APIs in user
space. When a process requests a kernel service throught a system call, the kernell will execute on
behalf of the caller process. The kernel is now said to be executing in `process context`. Similarly,
the kernel also responds to interrupts raised by other hardware entities: here, the kernel executes in
`interrupt context`. When in interrupt context, the kernel is not running on behalf of any process.

A driver for a device is the interface between an application and hardware. As a result, you often
have to access a given user-space driver device. Accessing process address space from the kernel
cannot be done directly (by de-referencing a user-space pointer). Direct access of an user-space
pointer can lead to incorrect behavior (depending on architecture, an user-space pointer may not
be valid or mapped to kernel-space), a kernel oops (the user-mode pointer can refer to a non-resident
memory area) or security issues. Proper access to user-space data is done by calling the
macros/functions below:

- A single value:

```c
get_user(type val, type *address)
put_user(type val, type *address)
```

- A buffer

```c
unsigned long copy_to_user(void __user *to, const void *from, unsigned long n)
unsigned long copy_from_user(void *to, const void __user *from, unsgined long n)
```

## MMIO (Memory-Mapped I/O) Device Access

A peripheral device is controlled by writing and reading its registers. Often, a device has multiple
registers that can be accessed at consecutive addresses either in the memory address space (MMIO)
or in the I/O address space (PIO). See below the main differences between Port I/O and Memory-Mapped I/O:

* MMIO
  * Same address bus to address memory and I/O devices
  * Access to the I/O devices by using regular instructions
  * Most widely used I/O method across the different architectures supported by Linux
* PIO
  * Different address spaces for memory and I/O devices
  * Uses a special class of CPU instructions to access I/O devices
  * Example on x86: IN and OUT instructions

The Linux driver cannot access physical I/O addresses directly - MMU mapping is needed. To access
I/O memory, drivers need to have a virtual address that the processor can handle, because
I/O memory is not mapped by default in virtual memory.

Below are the functions used to get I/O virtual addresses:

- Map and remove mapping by using ioremap()/iounmap() functions.

```c
void __iomem *ioremap(phys_addr_t offset, unsigned long size)
void iounmap(void *address)
```

- Map and remove mapping attached to the driver device by using the devm_ioremap()/devm_iounmap().

```c
void __iomem *devm_ioremap(struct device *dev, resource_size_t offset, unsigned long size)
void devm_iounmap(struct device *dev, void __iomem *addr)
```

Each `struct device` manages a linked list of resources via its included
`struct list_head_devres_head` structure. Calling a managed resource allocator like `devm_kzalloc()`
involves adding the resource to the list. The resources are released in reverse order when the `probe()`
function exits with an error status or after the `remove()` function returns. The use of managed functions
in the `probe()` function remove the needed resource releases on error handling, replacing goto's and other
resource releases with just a return. It also removes resource releases in `remove()` function.

Deferencing the pointer returned by `devm_ioremap()` is not reliable. Cache and synchronization issues
might occur. The kernel provides functions to read and write to virtual address. To PCI-style, little-endian
accesses, conversion being done automatically use the functions below:

```c
unsigned int ioread8(void __iomem *addr)
unsigned int ioread16(void __iomem *addr)
unsigned int ioread32(void __iomem *addr)
void iowrite8(u8 value, void __iomem *addr)
void iowrite16(u16 value, void __iomem *addr)
void iowrite32(u32 value, void __iomem *addr)
```

## LAB 1 - "Platform Device" module

The functionality of this platform driver is the same as the misc char driver, but this time
we will register the char device in the `probe()` function instead of `init()` function. When
the kernel module is loaded, the `platform device driver` registers itself with the
`platform bus driver` by using the platform_driver_register() function. The probe() function is
called when the platform device driver matches the value of one of its compatible char strings
(included in one of its of_device_id structures) with the compatible property value of the DT device
node. The process of associating a device with a device driver is called `binding`.

The files used for this lab are:

- chapter-5/lab-1/keys_platform_device_driver.c
- chapter-5/lab-1/Makefile (builds the .ko file)
- chapter-5-apps/lab-1/app_for_keys_platform_device_driver.c
- chapter-5-apps/lab-1/Makefile (builds the binary for the app)
- linux-kernel/arch/arm/boot/dts/bcm2710-rpi-3-b.dts

>Note: Remember to build the device tree (DT) and install the modules.

After `copying` the `keys_platform_device_driver.ko` and `app_for_keys_platform_device_driver` in the
Raspberry Pi, execute the following commands:

```shell
sudo insmod keys_platform_device_driver.ko
sudo find /sys -name "hellokeys"
ls -la /sys/bus/platform/drivers/keys_driver/soc:hellokeys
ls -la /sys/bus/platform/devices /* See the devices at platform bus */
ls -l /sys/module/hellokeys/drivers
ls /sys/class/misc
ls /sys/class/misc/keys_dev
cat /sys/class/misc/keys_dev/dev /* The major number 10 is assigned by the misc framework*/
ls -l /dev  /* Verify that keys_dev is created under /dev */
sudo chmod 755 /dev/keys_dev
./app_for_keys_platform_device_driver
sudo rmmod keys_platform_device_driver.ko
```

## LAB 2 - "RGB LED Platform Device" module

In this lab, we willcontrol several LEDs mapping from physical to virtual addresses.
We will create a character device per each LED by using the misc framework and control
the LEDs exchanging data between kernel and user spaces by using `write()` and `read()`
driver's operations. We will use the `copy_to_user()` and `copy_from_user()` functions
to exchange character arrays between kernel and user spaces.

### Device Tree for the BCM2837 Processor

From the Raspberrly Pi 3 Model B board we will use the GPIOs 22, 26 and 27. Each pin
configuration node lists the pin(s) to which it applies, and one or more of the mux function
to select on those pin(s), and pull-up/down configuration. These are the properties found in
`linux-kernel/arch/arm/boot/dts/bcm2710-rpi-3-b.dts`.

- `brcm,pins`: An array of cells. Each cell contains the ID of a pin. Valid IDs are the integer
GPIO IDs; 0, 1, 2, ... , 53.
- `brcm,function`: Integer, containing the function to mux to the pin(s):
	1: GPIO out
	2: alt5
	3: alt4
	4: alt0
	5: alt1
	6: alt2
	7: alt3
- `brcm,pull`: Intege, representing the pull-down/up to apply to the pin(s):
	0: none
	1: down
	2: up

The files used for this lab are:

- chapter-5/lab-2/led_rgb_platform_driver.c
- chapter-5/lab-2/Makefile (builds the .ko file)
- linux-kernel/arch/arm/boot/dts/bcm2710-rpi-3-b.dts

>Note: Remember to build the device tree (DT) and install the modules.

After `copying` the `led_rgb_platform_driver.ko` in the Raspberry Pi,
execute the following commands:

```shell
tar -f /var/log/syslog

# in another shell
sudo insmod led_rgb_platform_driver.ko
ls /dev/led* /* see led devices*/
sudo chmod 777 /dev/ledblue
sudo chmod 777 /dev/ledred
sudo chmod 777 /dev/ledgreen
echo on > /dev/ledblue   /* set led blue ON */
echo on > /dev/ledred    /* set led red ON */
echo on > /dev/ledgreen  /* set led green ON */
echo off > /dev/ledgreen /* set led green OFF */
echo off > /dev/ledred   /* set led red OFF */
cat /dev/ledblue
cat /dev/ledgreen
cat /dev/ledred
sudo rmmod led_rgb_platform_driver.ko
```

## LAB 3 - "RGB LED class Platform Device Driver and Led subsystem"

In this lab we will use the LED subsystem to control the leds. We will also
create an input in the device tree `bcm2710-rpi-3-b.dts` that will contain
3 nodes, one for every led (green, red, blue).

We will use in this new driver the `for_each_child_of_node()` function to walk
through sub nodes of the main node. Only the main node will have the `compatible`
property, so after the match of the device and driver is done, the `probe()` function
will be called only once retrieving the info included in all the sub nodes. The LED RGB
device has a `reg` property hat includes a GPIO register base address and the size of
the address region it is assigned. After the driver and the device are probed, the
`platform_get_resource()` function returns a `struct resource` filled with the `reg`
property values soo that you an use these values later in the driver code, mapping them in
the virtual space by using the `devm_ioremap()` function.

The files used for this lab are:

- chapter-5/lab-3/led_rgb_class_platform_driver.c
- chapter-5/lab-3/Makefile (builds the .ko file)
- linux-kernel/arch/arm/boot/dts/bcm2710-rpi-3-b.dts

>Note: Remember to build the device tree (DT) and install the modules.

After `copying` the `led_rgb_class_platform_driver.ko` in the Raspberry Pi,
execute the following commands:

```shell
tar -f /var/log/syslog

# in another shell
sudo insmod led_rgb_class_platform_driver.ko
ls /sys/class/leds /* see led devices*/
sudo chmod 777 /sys/class/leds/red/brightness
sudo chmod 777 /sys/class/leds/green/brightness
sudo chmod 777 /sys/class/leds/green/trigger
sudo chmod 777 /sys/class/leds/blue/brightness
echo 1 > /sys/class/leds/red/brightness   /* set led red ON */
echo 1 > /sys/class/leds/green/brightness   /* set led green ON and turn off red */
echo 1 > /sys/class/leds/blue/brightness   /* set led blue ON and turn off green */
echo timer > /sys/class/leds/green/trigger   /* set the timer trigger and see the led green blinking */
sudo rmmod led_rgb_class_platform_driver.ko
```


