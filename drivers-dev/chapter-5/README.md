
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

