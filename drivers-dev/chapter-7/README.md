
# Chapter 7 - Handling Interrupts in Device Drivers


## Deferred Work

The Linux kernel performs operations in two contexts:

* Process context
    * workqueues
    * botton half of threaded interrupts
* Interrupt context
    * softirqs
    * tasklets
    * timers

Deferred work is a class of kernel faciliies that allows one to schedule code to be
executed at a later time. This schedule code can run either in process context using
`workqueues` or `threaded interrupts`, both methods using kernel threads. `softirqs`,
`tasklets` and `timers` run in interrupt context. 

To summarize, the main types of deferred work are kernel threads and softirqs. Work
queues and bottom-half of threaded irqs are implemented on top of kernel threads that
are able to block and tasklets and timers on top of softirqs that cannot block functions.


## Lab 1 - Handling interrupts

In this lab, we will implement a driver that manages an interrupt.
We will use a pushbutton (switch) as an interrupt generator. The driver
will handle button presses. Each time we press the button, an interrupt
will be generated and handled by the platform driver.

Note that in `handler_button_interrupt_driver.c` it was necessary
to include a delay to avoid the bouncing that happens when we click the switch.

```c
mdelay(500); /* Wait 500ms to avoid bouncing. */
```

The files used for this lab are:

- chapter-7/lab-1/handle_button_interrupt_driver.c
- chapter-7/lab-2/Makefile (builds the .ko file)
- linux-kernel/arch/arm/boot/dts/bcm2710-rpi-3-b.dts

>Note: Remember to build the device tree (DT) and install the modules.

After `copying` the `handle_button_interrupt_driver.ko` in the Raspberry Pi,
execute the following commands:

```shell
tar -f /var/log/syslog

# in another shell
sudo insmod handle_button_interrupt_driver.ko

# Press the button to generate the interrupt (pin 23)

sudo rmmod handle_button_interrupt_driver.ko
```

# Timer Lab - Deferred Work

This driver will schedule a task every 1000ms (1 second). It will
use `timer_setup()` and `mod_timer()` to configure the timer.

In addition to this, it will define an attribute to control the delays of the 
timer. See `set_period()` and `DEVICE_ATTR()`.

The files used for this lab are:

- chapter-7/timer-lab/timer_deferred_work_driver.c
- chapter-7/timer-lab/Makefile (builds the .ko file)
- linux-kernel/arch/arm/boot/dts/bcm2710-rpi-3-b.dts

>Note: Remember to build the device tree (DT) and install the modules.

After `copying` the `timer_deferred_work_driver.ko` in the Raspberry Pi,
execute the following commands:

```shell
tar -f /var/log/syslog

# in another shell

sudo insmod timer_deferred_work_driver.ko 

ls /dev/timer_deferred_work_misc_dev 
ls /sys/class/misc/timer_deferred_work_misc_dev
ls /sys/devices/platform/soc/soc\:timer_work_dts
 tree /sys/class/misc/timer_deferred_work_misc_dev
sudo chmod 777 /sys/devices/platform/soc/soc\:timer_work_dts/period 
echo 5000 > /sys/devices/platform/soc/soc\:timer_work_dts/period 
echo 10000 > /sys/devices/platform/soc/soc\:timer_work_dts/period 
sudo rmmod timer_deferred_work_driver 
```

