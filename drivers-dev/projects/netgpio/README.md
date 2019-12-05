
# Net GPIO project

This project consists in designing and implementing a driver
that performs the following tasks:

- Process the user's space data and based on this, turn on/off the
GPIO pins.
- Provides a locking mechanism to avoid corruption of data.
- Provides queues to process the requests in the order that they come.


## Deploy and test the driver

The files used for this project are:

- projects/netgpio/net_gpio_driver.c
- projects/netgpio/Makefile (builds the .ko file)
- linux-kernel/arch/arm/boot/dts/bcm2710-rpi-3-b.dts

>Note: Remember to build the device tree (DT) and install the modules.

In the host, execute the following command to copy the module:

```shell
scp net_gpio_driver.ko  pi@192.168.2.2:/home/pi/my_modules
```

After `copying` the `net_gpio_driver.ko` file in the Raspberry Pi,
execute the following commands:

```shell
tar -f /var/log/syslog

# in another shell
sudo insmod net_gpio_driver.ko
ls /dev/ /* see netgpio device*/
ls -la /sys/class/misc/netgpio/
sudo chmod 777 /dev/netgpio
echo 1 > /dev/netgpio
sudo rmmod net_gpio_driver.ko
```
