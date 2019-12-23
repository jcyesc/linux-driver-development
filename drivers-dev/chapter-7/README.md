
# Chapter 7 - Handling Interrupts in Device Drivers


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

