
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
