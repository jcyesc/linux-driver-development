
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