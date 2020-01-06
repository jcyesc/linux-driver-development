
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

## Using locks

There are two main types of kernel locks. The fundamental type is the `spinlock`
(include/asm/spinlock.h), which is a very simple single-holder lock; if you can't get the
spinlock, you keep trying (spinning) until you can (disables the preemption in the running
core). Spinlocks are very small and fast, and can be used anywhere.

The second type is a `mutex` (include/linux/mutex.h); it is like a spinlock, but you may block
holding a mutex. If you can't lock a mutex, your task will suspend itself, and be  woken up when
the mutex is released. This means the CPU can do something else while you are waiting.

## Sharing Spinlocks between Interrupt and Process Context

It's possible that a critical section needs to be protected by the same lock in both an interrupt
and in non-interrupt (process) execution context in the kernel. In this case `spin_lock_irqsave()`
and the `spin_unlock_irqrestore()` variants have to be used to protect the critical section. This
has the effect of disabling interrupts on the executing CPU. You can see in the steps below what could
happen if you just used `spin_lock()` in the process context:

1. Process context kernel code acquires the spinlock usin `spin_lock()`.
2. While the spinlock is held, an interrupt comes in on the same CPU and executes.
3. Interrupt Service Routine (ISR) tries to acquire the spinlock, and spins continuously
waiting for it. Process context is blocked in the CPU and there is never a chance to run again
and free the spinlock.

To prevent this, the process context code needs call `spin_lock)irqsave()`, which has the effect of
disabling interrupts on that particular CPU along with the regular disabling of preemption on the
executing CPU.

The ISR can still just call `spin_lock()` instead of `spin_lock_irqsave()` because interrupts are
disabled anyway during ISR on the executing CPU. Often times people use `spin_lock_irqsave()` in an
ISR, that's not necessary.

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

## Lab 2 - Handling interrupts and waiting for a condition


This lab consists on:

- Handling two types of interrupt: FALLING EDGE AND RISING EDGE
- Processing read requests from user space and aleeping if there is not data to read yet
- After pressing the switch, two interrupts will be generated and processed by the
interrupt handler. The interrupt handler will wake up the user's process if there is any.
- using a `wait_queue_head_t` to send the user's process to sleep when there is not
data to be read.

The files used for this lab are:

- chapter-7/lab-2-interrupt-wait/handle_interrupt_wait_driver.c
- chapter-7/lab-2-interrupt-wait/Makefile (builds the .ko file)
- linux-kernel/arch/arm/boot/dts/bcm2710-rpi-3-b.dts

>Note: Remember to build the device tree (DT) and install the modules.

```shell
cd my_modules/
sudo insmod handle_interrupt_wait_driver.ko
more /proc/interrupts
sudo chmod 777 /dev/miscdevice_name_wait 
cat /dev/miscdevice_name_wait

# Press the switch several times
# and see how the type of interrupt appears in the screen.

# Ctrl-c

# Press the switch several times again
cat /dev/miscdevice_name_wait
sudo rmmod handle_interrupt_wait_driver 
```

## Lab 3 - Led blink control driver

This driver will create a new class called "keyled". Several led devices will
be created under the "keyled" class, and also several sysfs entries will be created
under each led device. We will control each led device by writing from user space
to the sysfs entries under each led device registered to the `keyled` class. The
led devices will be controlled writing to the sysfs entries under
`/sys/class/keyled/<led_device>/` directory.
 *
The blinking value `period` of each led device will be incremented or decremented via
interrupts by using two buttons. A kernel thread will manage the led blinking, toggling
the output value of the GPIO connected to the led.

The files used for this lab are:

- chapter-7/lab-3-led-blink-control/led_blink_control_driver.c
- chapter-7/lab-3-led-blink-control/Makefile (builds the .ko file)
- linux-kernel/arch/arm/boot/dts/bcm2710-rpi-3-b.dts

>Note: Remember to build the device tree (DT) and install the modules.

```shell
sudo insmod led_blink_control_driver.ko 
more /proc/interrupts 
ls /sys/class/keyled/
cd /sys/class/keyled/blue
ls
echo on > set_led
sudo chmod 777 *
echo on > set_led
cd /sys/class/keyled/red
sudo chmod 777 *
echo on > set_led 
cd ../green/
sudo chmod 777 *
echo on > set_led 
echo on > blink_on_led 
cd ../red/
echo on > blink_on_led 
echo off > ../green/blink_off_led 
echo on > blink_on_led 
echo off > set_led 
cd ../blue/
ls
echo on > blink_on_led

# Click in the switches to increase or decrease the period.

echo 100 > set_period 
echo 900 > set_period 
echo 90 > set_period 
echo off > blink_off_led 
sudo rmmod led_blink_control_driver 
```
